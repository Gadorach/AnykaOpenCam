"""Exact composite/native receipt fixtures; no real transport or hardware."""
from dataclasses import replace
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import aim_backend as aim
import optical_uart as uart
from optical_control import ALL_ACTIONS, ControlError, Request
from test_optical_control import EPOCH, REQUEST_ID, encode
from test_optical_uart import state as opt_state
from test_optical_aim import motor
from test_optical_uart import FakeBoot

EPOCH2 = replace(EPOCH, helper_sha256=uart.HELPER_PIN, aim_sha256=aim.TOOLSET_SHA)


def components(request):
    action = request.action
    nonce = request.request_id.replace('-', '')
    opt_action = action if action in ('pulse-a', 'pulse-b', 'ir-on', 'ir-off') else 'status'
    w_action = action if action.startswith('white-') else 'status'
    p_action = action if action.startswith('motor') else 'status'
    opt = dict(action=opt_action, nonce=nonce, result='OK', ioctl_completed=True,
               command_errno=0, snapshot_errno=0, transport_errno=0, keeper_errno=0,
               verification_errno=0, close_errno=0, keeper_pid=0,
               lease_mode='self' if opt_action == 'ir-on' else 'none',
               state=opt_state(opt_action == 'ir-on'))
    on = int(w_action == 'white-on')
    white = dict(action=w_action, nonce=nonce, result='OK', ioctl_completed=True,
                 command_errno=0, snapshot_errno=0, transport_errno=0, verification_errno=0,
                 close_errno=0, lease_mode='self' if on else 'none',
                 state=dict(abi=1, flags=7 if on else 13, fault=0, owned=1,
                            white_latch=on, white_pad=on, direction=1, mux_ok=1,
                            remaining_ms=4800 if on else 0, limit_ms=4900,
                            prior_pad=0, prior_direction=1, sequence=2, monotonic_ns=2000000000))
    moving = p_action != 'status'
    before = [motor(0), motor(1)]
    after = [None, None]
    if moving:
        axis = int(p_action[5])
        after = [motor(0), motor(1)]
        after[axis] = motor(axis, 4)
    ptz = dict(schema='qc07f.ptz.sysfs.v1', action=p_action, nonce=nonce, result='OK',
               stage='complete' if moving else 'status',
               requested_steps=(-4 if p_action.endswith('minus') else 4) if moving else 0,
               write_attempted=moving, write_completed=moving, hold=False,
               errno=0, diagnostic_errno=0, before=before, after=after)
    return opt, white, ptz


def body_for(request, items=None):
    nonce = request.request_id.replace('-', '')
    opt, white, ptz = components(request) if items is None else items
    return [('OB_BEGIN_' + nonce).encode(), encode(opt), ('OB_DONE_' + nonce + ':0').encode(),
            ('WHITE_BEGIN_' + nonce).encode(), encode(white), ('WHITE_DONE_' + nonce + ':0').encode(),
            ('PTZ_BEGIN_' + nonce).encode(), encode(ptz), ('PTZ_DONE_' + nonce + ':0').encode()]


class BackendTests(unittest.TestCase):
    def test_private_v2_profile_requires_exact_native_aiming_admission(self):
        value = dict(schema='optical-aim-epoch-v2', admission='ADMITTED', startup_reviewed=True,
                     source_reviewed=True, recovery_reviewed=True, owners_reviewed=True,
                     feed_retirement_qualified=True, controls_native_inert_qualified=True,
                     bounded_optical_trial_admitted=True, uart_connected=True,
                     white_native_inert_qualified=True, white_OFF_physically_confirmed=True,
                     motor_native_metadata_reviewed=True, bounded_aim_trial_admitted=True,
                     host=EPOCH2.host, generation=EPOCH2.generation, boot_id=EPOCH2.boot_id,
                     expires_at=500, kernel_sha256=uart.KERNEL_PIN, helper_sha256=uart.HELPER_PIN,
                     module_sha256=uart.MODULE_PIN, dispatcher_sha256=aim.DISPATCH_PIN,
                     aim_sha256=aim.TOOLSET_SHA, white_module_sha256=aim.WHITE_MODULE_PIN,
                     white_helper_sha256=aim.WHITE_HELPER_PIN, ptz_helper_sha256=aim.PTZ_HELPER_PIN,
                     expected_pid1='init', expected_shell_pid=165, recorder_part='foundation-20261020a',
                     capture_dir='/PUBLIC-MOCK-NOT-OPENED', diagnostics_dir='/PUBLIC-MOCK-NOT-OPENED')
        def load(row):
            with patch.object(uart.private, 'read_private', return_value=encode(row)), \
                    patch.object(uart.private, 'private_path'), patch.object(aim, 'NATIVE_REVIEWED', True):
                return uart.profile('/PUBLIC-MOCK-NOT-OPENED', now=100)
        self.assertEqual(load(value)[1].aim_sha256, aim.TOOLSET_SHA)
        for key in ('white_native_inert_qualified', 'white_OFF_physically_confirmed',
                    'motor_native_metadata_reviewed', 'bounded_aim_trial_admitted'):
            with self.subTest(key=key), self.assertRaises(ControlError):
                load(dict(value, **{key: False}))
        for key in ('aim_sha256', 'white_module_sha256', 'white_helper_sha256', 'ptz_helper_sha256',
                    'dispatcher_sha256'):
            with self.subTest(key=key), self.assertRaises(ControlError):
                load(dict(value, **{key: '0' * 64}))

    def test_mock_composite_transport_clears_only_complete_verified_epoch(self):
        for good in (True, False):
            with self.subTest(good=good), tempfile.TemporaryDirectory(
                    dir=os.environ.get('TMPDIR', str(uart.ROOT / 'work')), prefix='aim-transport-') as temp:
                lock = Path(temp) / 'operation.lock'
                settings = dict(schema='optical-aim-epoch-v2', expected_pid1='init', expected_shell_pid=165,
                                recorder_part='foundation-20261016b', capture_dir='/PUBLIC-MOCK-NOT-OPENED',
                                diagnostics_dir='/PUBLIC-MOCK-NOT-OPENED')
                request = Request(REQUEST_ID, EPOCH2, 'motor0-plus')
                nonce = REQUEST_ID.replace('-', '')
                items = list(components(request))
                if not good:
                    items[2]['hold'] = True
                FakeBoot.response = (('AIM_BEGIN_' + nonce + '\n').encode() +
                                     b'\n'.join(body_for(request, items)) +
                                     ('\nAIM_DONE_' + nonce + ':0\n').encode())
                FakeBoot.prompt = '~ #'
                with patch.object(uart, 'profile', return_value=(settings, EPOCH2)), \
                        patch.object(uart, 'LOCK', lock), patch.object(uart.private, 'PrivateBoot', FakeBoot), \
                        patch.object(uart.time, 'sleep'), patch.object(aim, 'NATIVE_REVIEWED', True):
                    driver = uart.UARTDriver('/PUBLIC-MOCK-NOT-OPENED')
                    try:
                        if good:
                            row = json.loads(driver.act(request.action, request))
                            self.assertEqual(row['motor_effect']['issued_delta'], 4)
                            self.assertEqual(lock.read_text(), '')
                        else:
                            with self.assertRaises(ControlError):
                                driver.act(request.action, request)
                            self.assertTrue(driver._fault)
                            self.assertEqual(json.loads(lock.read_text())['state'], 'HOLD')
                    finally:
                        if driver._held is not None:
                            driver._held.close()  # Mock teardown, not production recovery.

    def test_all_actions_three_components_exact_and_one_effect(self):
        for action in ('status',) + ALL_ACTIONS:
            request = Request(REQUEST_ID, EPOCH2, action)
            result = json.loads(aim.translate_body(body_for(request), REQUEST_ID.replace('-', ''), request))
            self.assertEqual(result['schema'], 'optical-control-v2')
            self.assertEqual(result['action'], action)
            self.assertEqual(result['release'], '00')
            self.assertEqual(result['aim_sha256'], aim.TOOLSET_SHA)

    def test_missing_duplicate_wrong_component_frame_and_nonce(self):
        request = Request(REQUEST_ID, EPOCH2, 'status')
        good = body_for(request)
        bad = [good[:-1], good + [b'noise'], [*good[:3], *good[:3], *good[6:]]]
        for index in (0, 2, 3, 5, 6, 8):
            item = list(good)
            item[index] += b'WRONG'
            bad.append(item)
        for body in bad:
            with self.assertRaises(ControlError):
                aim.translate_body(body, REQUEST_ID.replace('-', ''), request)
        for component in range(3):
            items = list(components(request))
            items[component]['nonce'] = '0' * 32
            with self.assertRaises(ControlError):
                aim.translate_body(body_for(request, items), REQUEST_ID.replace('-', ''), request)

    def test_white_fault_uninitialized_overdue_unknown_not_normalized(self):
        request = Request(REQUEST_ID, EPOCH2, 'white-on')
        for key, value in (('flags', 0), ('flags', 13), ('fault', 1), ('owned', 0),
                           ('white_pad', 0), ('direction', 0), ('mux_ok', 0),
                           ('remaining_ms', 0), ('remaining_ms', 5000), ('limit_ms', 5000),
                           ('sequence', True)):
            items = list(components(request))
            items[1]['state'][key] = value
            with self.subTest(key=key), self.assertRaises(ControlError):
                aim.translate_body(body_for(request, items), REQUEST_ID.replace('-', ''), request)
        for key, value in (('result', 'FAILED'), ('ioctl_completed', 1), ('close_errno', 5),
                           ('lease_mode', 'fork'), ('snapshot_errno', True)):
            items = list(components(request))
            items[1][key] = value
            with self.assertRaises(ControlError):
                aim.translate_body(body_for(request, items), REQUEST_ID.replace('-', ''), request)

    def test_motor_hold_other_axis_and_failed_retirement_sticky_contract(self):
        request = Request(REQUEST_ID, EPOCH2, 'motor0-plus')
        for key, value in (('result', 'UNKNOWN'), ('stage', 'pulse-write'), ('requested_steps', 32),
                           ('write_attempted', 1), ('write_completed', False), ('hold', True),
                           ('errno', 5), ('diagnostic_errno', 5)):
            items = list(components(request))
            items[2][key] = value
            with self.subTest(key=key), self.assertRaises(ControlError):
                aim.translate_body(body_for(request, items), REQUEST_ID.replace('-', ''), request)
        for key, value in (('issued_steps', 8), ('phase', 1), ('coils_off_verified', 0)):
            items = list(components(request))
            items[2]['after'][0][key] = value
            with self.assertRaises(ControlError):
                aim.translate_body(body_for(request, items), REQUEST_ID.replace('-', ''), request)
        items = list(components(request))
        items[2]['after'][1] = motor(1, 4)
        with self.assertRaises(ControlError):
            aim.translate_body(body_for(request, items), REQUEST_ID.replace('-', ''), request)

    def test_unreviewed_packet_never_transmits_and_reviewed_packet_is_fixed(self):
        request = Request(REQUEST_ID, EPOCH2, 'motor1-minus')
        settings = dict(expected_pid1='init', expected_shell_pid=165)
        with patch.object(aim, 'NATIVE_REVIEWED', False), self.assertRaises(ControlError):
            uart.packet(request.action, request, settings)
        with patch.object(aim, 'NATIVE_REVIEWED', True):
            wire, nonce = uart.packet(request.action, request, settings)
        self.assertIn(b'. /tmp/qc07f-aim-dispatch.sh motor1-minus ' + nonce.encode(), wire)
        self.assertLessEqual(len(wire), 951)
        self.assertIn(b'motor1-minus ' + nonce.encode() + b' ' + EPOCH2.boot_id.encode(), wire)


if __name__ == '__main__':
    unittest.main()
