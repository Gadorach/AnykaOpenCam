"""White-light and four-step receipt/admission fixtures. No target/transport."""
from dataclasses import replace
import json
import unittest

from optical_control import (AIM_ACTIONS, ALL_ACTIONS, ControlError, Controls, Request, receipt)
from optical_video import Preview
from test_optical_control import EPOCH, REQUEST_ID, MockDriver, row, encode
from test_optical_video import Clock

AIM_EPOCH = replace(EPOCH, aim_sha256='c' * 64)


def motor(axis, count=0):
    return dict(axis=axis, issued_steps=count, phase=0, result=0,
                coil_readback_ok=int(count > 0), coils_off_verified=int(count > 0),
                max_pulse_steps=4, period_ms=20, physical_position_known=0,
                health='READY' if count else 'UNTESTED')


def aim_row(request, *, ir_on=False, white_on=None):
    result = row(request, on=ir_on or request.action == 'ir-on')
    if white_on is None:
        white_on = request.action == 'white-on'
    result.update(schema='optical-control-v2', aim_sha256=request.epoch.aim_sha256,
                  white_state='ON' if white_on else 'OFF',
                  white_timer_owner='target' if white_on else 'none',
                  white_timer_remaining_ms=4800 if white_on else 0,
                  white_timer_bound_ms=5000 if white_on else 0,
                  white_proof='TARGET_TIMER_ARMED' if white_on else 'OFF_CONFIRMED',
                  motors=[motor(0), motor(1)], motor_effect=None)
    if request.action.startswith('motor'):
        axis = int(request.action[5])
        sign = -1 if request.action.endswith('minus') else 1
        result['motors'][axis] = motor(axis, 4)
        result['motor_effect'] = dict(axis=axis, requested_steps=sign * 4,
                                      before_issued_steps=0, after_issued_steps=4,
                                      issued_delta=4, coils_off_verified=True)
    return result


class AimReceiptTests(unittest.TestCase):
    def test_all_actions_cold_and_positive_four_step(self):
        for action in ('status',) + ALL_ACTIONS:
            request = Request(REQUEST_ID, AIM_EPOCH, action)
            receipt(encode(aim_row(request)), request, 100)

    def test_no_capability_or_schema_downgrade(self):
        request = Request(REQUEST_ID, AIM_EPOCH, 'status')
        with self.assertRaises(ControlError):
            receipt(encode(row(request)), request, 100)
        with self.assertRaises(ControlError):
            receipt(encode(aim_row(request)), replace(request, epoch=EPOCH), 100)
        with self.assertRaises(ControlError):
            receipt(encode(dict(aim_row(request), aim_sha256='d' * 64)), request, 100)
        request = Request(REQUEST_ID, EPOCH, 'white-off')
        with self.assertRaises(ControlError):
            receipt(encode(row(request)), request, 100)

    def test_exact_extra_fields_types_and_target_white_timer(self):
        request = Request(REQUEST_ID, AIM_EPOCH, 'white-on')
        good = aim_row(request)
        for key in good:
            bad = dict(good)
            del bad[key]
            with self.subTest(missing=key), self.assertRaises(ControlError):
                receipt(encode(bad), request, 100)
        for key, wrong in (('white_timer_owner', 'desktop'), ('white_timer_remaining_ms', 0),
                           ('white_timer_remaining_ms', True), ('white_timer_remaining_ms', 5001),
                           ('white_timer_bound_ms', 0), ('white_state', 'OFF'),
                           ('white_proof', 'OFF_CONFIRMED'), ('motor_effect', {})):
            with self.subTest(key=key), self.assertRaises(ControlError):
                receipt(encode(dict(good, **{key: wrong})), request, 100)

    def test_motor_fault_metadata_never_admits(self):
        request = Request(REQUEST_ID, AIM_EPOCH, 'status')
        for key, wrong in (('axis', True), ('axis', 1), ('issued_steps', True),
                           ('phase', 4), ('result', -5), ('coil_readback_ok', True),
                           ('coils_off_verified', 1), ('max_pulse_steps', 32),
                           ('period_ms', 1), ('physical_position_known', 1), ('health', 'HOLD')):
            bad = aim_row(request)
            bad['motors'][0][key] = wrong
            with self.subTest(key=key, wrong=wrong), self.assertRaises(ControlError):
                receipt(encode(bad), request, 100)

    def test_motor_one_write_delta_sign_and_off_proof(self):
        request = Request(REQUEST_ID, AIM_EPOCH, 'motor1-minus')
        for key, wrong in (('axis', 0), ('axis', True), ('requested_steps', 4),
                           ('requested_steps', -32), ('issued_delta', 8),
                           ('before_issued_steps', 0xffffffff), ('after_issued_steps', 8),
                           ('coils_off_verified', 1), ('coils_off_verified', False)):
            bad = aim_row(request)
            bad['motor_effect'][key] = wrong
            with self.subTest(key=key), self.assertRaises(ControlError):
                receipt(encode(bad), request, 100)


class AimControlsTests(unittest.TestCase):
    def setUp(self):
        self.clock = Clock()
        self.driver = MockDriver()
        self.driver.result = lambda request: encode(aim_row(request))
        self.controls = Controls(self.driver, AIM_EPOCH, clock=self.clock, wall=lambda: 100)
        self.live = Preview('aim-one', True, b'synthetic', 1, 'Live')

    def complete(self):
        self.controls.thread.join(1)
        self.controls.poll()

    def admit(self):
        self.controls.observe(self.live, AIM_EPOCH.host)
        self.assertFalse(self.controls.allowed('motor0-plus'))
        self.controls.dispatch('status')
        self.complete()
        self.assertTrue(self.controls.aim_admitted)

    def test_manual_v2_status_required_no_startup_or_queued_motion(self):
        self.assertEqual(self.driver.calls, [])
        self.admit()
        self.assertTrue(all(self.controls.allowed(action) for action in AIM_ACTIONS))
        self.assertTrue(self.controls.dispatch('motor0-minus'))
        self.assertFalse(self.controls.dispatch('motor1-plus'))
        self.complete()
        self.assertFalse(self.controls.allowed('motor1-plus'))
        self.clock.now += 2.01
        self.assertTrue(self.controls.allowed('motor1-plus'))

    def test_independent_white_off_and_motor_while_lights_on(self):
        self.admit()
        self.driver.result = lambda request: encode(aim_row(request, ir_on=True, white_on=True))
        self.controls.dispatch('white-on')
        self.complete()
        self.assertTrue(self.controls.allowed('white-off'))
        self.assertTrue(self.controls.allowed('ir-off'))
        self.assertFalse(self.controls.allowed('white-on'))
        self.clock.now += 2.01
        self.assertTrue(self.controls.allowed('motor0-plus'))
        self.assertTrue(self.controls.allowed('pulse-b'))
        self.clock.now += 6
        self.assertFalse(self.controls.allowed('white-on'))  # No desktop expiry-as-off.

    def test_preview_loss_or_unknown_motor_latches_no_retry(self):
        self.admit()
        self.controls.observe(replace(self.live, ready=False), AIM_EPOCH.host)
        self.assertFalse(self.controls.aim_admitted)
        self.assertFalse(self.controls.allowed('white-off'))
        self.admit()
        def bad(request):
            value = aim_row(request)
            value['motor_effect']['coils_off_verified'] = False
            return encode(value)
        self.driver.result = bad
        self.controls.dispatch('motor1-plus')
        self.complete()
        self.assertTrue(self.controls.fault)
        self.assertFalse(self.controls.close_allowed())
        self.assertFalse(self.controls.dispatch('status'))


if __name__ == '__main__':
    unittest.main()
