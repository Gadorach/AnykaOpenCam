"""Synthetic console/epoch only. No recorder, device, process or network calls."""
import fcntl
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from dataclasses import replace

from optical_control import ControlError, NotAdmitted, Request
from test_optical_control import EPOCH, REQUEST_ID
import optical_uart as bridge


def state(active=0):
    active = int(active)
    return dict(abi=1, setup=1, fault=0, owned_mask=7, pair_latch=0, pair_pad=0,
                ir_latch=active, ir_pad=active, direction_mask=7, mux_ok=1,
                release_verified=1, ir_active=active, ir_remaining_ms=4800 if active else 0,
                cooldown_remaining_ms=0, pulse_us=10000, ir_limit_ms=4900,
                sequence=1, monotonic_ns=1000000000)


class FakeBoot:
    instances = []
    response = b''
    prompt = '~ #'

    def __init__(self, *args):
        self.sent = []
        self.logs = []
        self.instances.append(self)

    def owner(self, margin):
        return dict(part='foundation-20261016b', abort_armed=True, tx_busy=False,
                    recent_rx=self.prompt, rx_bytes=0)

    def send(self, data, reason):
        self.sent.append(data)

    def received(self, start):
        return self.response

    def log(self, *args, **kwargs):
        self.logs.append((args, kwargs))


class UARTTests(unittest.TestCase):
    def test_async_printk_after_completed_frame_is_not_unknown_input(self):
        nonce = REQUEST_ID.replace('-', '')
        for prefix, ending in (('P20_BEGIN_', 'P20_END_'), ('OB_BEGIN_', 'OB_DONE_')):
            frame = prefix + nonce + '\n{}\n' + ending + nonce + ':0\n'
            self.assertTrue(bridge.completed_shell_context(frame + 'wlan0: connected\n'))
            for transition in ('U-Boot', 'Starting kernel', 'Linux version ', 'qc07f>',
                               'RAM-only shell returned', 'Kernel panic', 'Oops:', 'BUG:'):
                self.assertFalse(bridge.completed_shell_context(frame + transition))
            self.assertFalse(bridge.completed_shell_context(frame.replace(':0', ':1')))
            self.assertFalse(bridge.completed_shell_context(frame.split('\n', 1)[1]))
            self.assertFalse(bridge.completed_shell_context('echo ' + frame.replace('\n', ' ')))
        self.assertTrue(bridge.completed_shell_context('/ # '))
        self.assertFalse(bridge.completed_shell_context('qc07f> '))

    def test_strict_cli_envelope_and_no_orphan_fork_mode(self):
        nonce = REQUEST_ID.replace('-', '')
        for action in ('status', 'pulse-a', 'pulse-b', 'ir-on', 'ir-off'):
            request = Request(REQUEST_ID, EPOCH, action)
            row = dict(action=action, nonce=nonce, result='OK', ioctl_completed=True,
                       command_errno=0, snapshot_errno=0, transport_errno=0,
                       keeper_errno=0, verification_errno=0, close_errno=0,
                       keeper_pid=0, lease_mode='self' if action == 'ir-on' else 'none',
                       state=state(action == 'ir-on'))
            self.assertEqual(json.loads(bridge.UARTDriver._native(row, nonce, request))['action'], action)
            for key, wrong in (('nonce', '0' * 32), ('action', 'setup'), ('result', 'FAILED'),
                               ('ioctl_completed', 1), ('command_errno', True),
                               ('snapshot_errno', 1), ('transport_errno', 1),
                               ('keeper_errno', 1), ('verification_errno', 1),
                               ('close_errno', 1), ('keeper_pid', 123), ('lease_mode', 'fork')):
                with self.subTest(action=action, key=key), self.assertRaises(ControlError):
                    bridge.UARTDriver._native(dict(row, **{key: wrong}), nonce, request)
            for key in row:
                wrong = dict(row)
                del wrong[key]
                with self.subTest(action=action, missing=key), self.assertRaises(ControlError):
                    bridge.UARTDriver._native(wrong, nonce, request)

    def test_fixed_wire_atoms_and_line_bound(self):
        request = Request(REQUEST_ID, EPOCH, 'pulse-a')
        settings = dict(expected_pid1='sh', expected_shell_pid=42)
        wire, nonce = bridge.packet('pulse-a', request, settings)
        self.assertLessEqual(len(wire), 951)
        self.assertEqual(wire.count(b'\r'), 1)
        self.assertEqual(nonce, REQUEST_ID.replace('-', ''))
        self.assertIn(b'. /tmp/qc07f-optical-dispatch.sh pulse-a ' + nonce.encode(), wire)
        self.assertNotIn(b'setup', wire)
        for action in ('setup', 'close', 'pulse-a; reboot', 'ir-on\r'):
            with self.assertRaises(ControlError):
                bridge.packet(action, request, settings)
        with self.assertRaises(ControlError):
            bridge.packet('pulse-a', request, dict(settings, expected_pid1='sh; reboot'))
        with self.assertRaises(ControlError):
            bridge.packet('pulse-a', replace(request, request_id='a' * 32), settings)
        with self.assertRaises(ControlError):
            bridge.packet('pulse-a', request, dict(settings, expected_shell_pid=True))

    def test_native_translation_positive_and_every_field_type(self):
        for action in ('status', 'pulse-a', 'pulse-b', 'ir-on', 'ir-off'):
            request = Request(REQUEST_ID, EPOCH, action)
            row = json.loads(bridge.translate(state(action == 'ir-on'), request))
            self.assertEqual(row['action'], action)
            self.assertEqual(row['release'], '00')
        request = Request(REQUEST_ID, EPOCH, 'status')
        for field in state():
            bad = state()
            bad[field] = True
            with self.subTest(field=field), self.assertRaises(ControlError):
                bridge.translate(bad, request)
        for field, value in (('fault', 1), ('setup', 0), ('owned_mask', 3),
                             ('release_verified', 0), ('pair_pad', 1), ('pair_latch', 2),
                             ('ir_active', 1), ('ir_remaining_ms', 1)):
            with self.subTest(field=field), self.assertRaises(ControlError):
                bridge.translate(dict(state(), **{field: value}), request)
        with self.assertRaises(ControlError):
            bridge.translate(dict(state(1), ir_remaining_ms=0), request)

    def make_driver(self, root):
        settings = dict(expected_pid1='sh', expected_shell_pid=42, recorder_part='foundation-20261016b',
                        capture_dir='/PUBLIC-MOCK-NOT-OPENED', diagnostics_dir='/PUBLIC-MOCK-NOT-OPENED')
        factory = patch.object(bridge, 'profile', return_value=(settings, EPOCH))
        factory.start()
        self.addCleanup(factory.stop)
        return bridge.UARTDriver(root / 'not-opened-profile')

    def exercise(self, root, mode):
        driver = self.make_driver(root)
        nonce = REQUEST_ID.replace('-', '')
        request = Request(REQUEST_ID, EPOCH, 'status')
        begin = ('OB_BEGIN_' + nonce + '\n').encode()
        end = ('OB_DONE_' + nonce + ':0\n').encode()
        FakeBoot.response = begin + b'{}\n' + end
        FakeBoot.prompt = '~ #'
        FakeBoot.instances = []
        if mode == 'wrong-prompt':
            FakeBoot.prompt = 'anyka$'
        elif mode == 'duplicate':
            FakeBoot.response += end
        elif mode == 'contradiction':
            FakeBoot.response += ('OB_DONE_' + nonce + ':125\n').encode()
        elif mode == 'failure':
            FakeBoot.response = begin + ('OB_DONE_' + nonce + ':125\n').encode()
        # Test transport state machinery independently of pending CLI schema.
        if mode != 'pending-envelope':
            driver._native = lambda native, nonce, request: bridge.translate(state(), request)
        if mode == 'late-completion':
            clock = [0.0]
            driver._clock = lambda: clock[0]
            def late(native, nonce, request):
                clock[0] = 7.01
                return bridge.translate(state(), request)
            driver._native = late
        lock = root / 'operation.lock'
        with patch.object(bridge, 'LOCK', lock), patch.object(bridge.private, 'PrivateBoot', FakeBoot), \
                patch.object(bridge.time, 'sleep'), patch.object(bridge, 'NATIVE_SCHEMA_REVIEWED', True):
            try:
                if mode == 'positive':
                    self.assertEqual(json.loads(driver.status(request))['release'], '00')
                    self.assertEqual(lock.read_text(), '')
                    self.assertIsNone(driver._held)
                    self.assertEqual(b''.join(FakeBoot.instances[0].sent),
                                     bridge.packet('status', request, driver.settings)[0])
                else:
                    with self.assertRaises(ControlError):
                        driver.status(request)
                    if mode == 'wrong-prompt':
                        self.assertFalse(driver._fault)
                        self.assertIsNone(driver._held)
                        self.assertFalse(FakeBoot.instances[0].sent)
                        self.assertEqual(lock.read_text(), '')
                        return
                    self.assertTrue(driver._fault)
                    self.assertEqual(json.loads(lock.read_text())['state'], 'HOLD')
                    before = len(FakeBoot.instances)
                    with self.assertRaises(ControlError):
                        driver.status(request)
                    self.assertEqual(len(FakeBoot.instances), before)
                    with lock.open('a+') as other, self.assertRaises(BlockingIOError):
                        fcntl.flock(other, fcntl.LOCK_EX | fcntl.LOCK_NB)
            finally:
                if driver._held is not None:
                    driver._held.close()  # Test teardown only, never production recovery.

    def test_mock_positive_and_durable_sticky_refusals(self):
        for mode in ('positive', 'wrong-prompt', 'duplicate', 'contradiction', 'failure',
                     'pending-envelope', 'late-completion'):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(
                    dir=os.environ.get('TMPDIR', str(bridge.ROOT / 'work')),
                    prefix='optical-uart-test-') as temporary:
                self.exercise(Path(temporary), mode)

    def test_pending_transport_never_opens_recorder_or_sends(self):
        driver = self.make_driver(Path('/PUBLIC-MOCK-NOT-OPENED'))
        with patch.object(bridge, 'NATIVE_SCHEMA_REVIEWED', False), \
                patch.object(bridge.private, 'PrivateBoot', side_effect=AssertionError('No recorder')):
            with self.assertRaises(ControlError):
                driver.status(Request(REQUEST_ID, EPOCH, 'status'))

    def test_duplicate_and_nonfinite_json_denied(self):
        for raw in (b'{"x":1,"x":2}', b'{"x":NaN}', b'{"x":Infinity}'):
            with self.assertRaises(ControlError):
                bridge.strict_json(raw)

    def test_profile_requires_fresh_qualified_native_epoch(self):
        value = dict(schema='optical-uart-epoch-v1', admission='ADMITTED',
                     startup_reviewed=True, source_reviewed=True, recovery_reviewed=True,
                     owners_reviewed=True, feed_retirement_qualified=True,
                     controls_native_inert_qualified=True, bounded_optical_trial_admitted=True,
                     uart_connected=True,
                     host=EPOCH.host, generation=EPOCH.generation, boot_id=EPOCH.boot_id,
                     expires_at=500, kernel_sha256=bridge.KERNEL_PIN,
                     module_sha256=bridge.MODULE_PIN, helper_sha256=bridge.HELPER_PIN,
                     dispatcher_sha256=bridge.DISPATCH_PIN, expected_shell_pid=42,
                     expected_pid1='sh', recorder_part='foundation-20261016b',
                     capture_dir='/PUBLIC-MOCK-NOT-OPENED', diagnostics_dir='/PUBLIC-MOCK-NOT-OPENED')
        def load(row, now=100):
            with patch.object(bridge.private, 'read_private', return_value=json.dumps(row).encode()), \
                    patch.object(bridge.private, 'private_path'):
                return bridge.profile('/PUBLIC-MOCK-NOT-OPENED', now=now)
        self.assertEqual(load(value)[1].boot_id, EPOCH.boot_id)
        for key, wrong in (('feed_retirement_qualified', False), ('controls_native_inert_qualified', False),
                           ('bounded_optical_trial_admitted', False),
                           ('kernel_sha256', 'c' * 64), ('module_sha256', 'c' * 64),
                           ('helper_sha256', 'c' * 64), ('dispatcher_sha256', 'c' * 64),
                           ('expected_shell_pid', True), ('expected_shell_pid', 0),
                           ('admission', 'HOLD'), ('expires_at', 170), ('expires_at', 701),
                           ('uart_connected', False), ('expected_pid1', 'anyka$')):
            with self.subTest(key=key, wrong=wrong), self.assertRaises(ControlError):
                load(dict(value, **{key: wrong}))


if __name__ == '__main__':
    unittest.main()
