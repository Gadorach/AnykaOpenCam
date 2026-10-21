"""Public synthetic epochs and receipts only; no profile/transport is created."""
from dataclasses import replace
import json
import threading
import unittest

from optical_control import (ACTIONS, ADAPTER_PENDING, ControlError, Controls, Epoch,
                             NotAdmitted, Request, UnavailableDriver, receipt)
from optical_video import Preview
from test_optical_video import Clock

EPOCH = Epoch("10.1.2.3", "11111111-1111-4111-8111-111111111111",
              "22222222-2222-4222-8222-222222222222", 2000000000, "a" * 64, "b" * 64)
REQUEST_ID = "33333333-3333-4333-8333-333333333333"


def row(request, *, on=None):
    epoch = request.epoch
    on = request.action == "ir-on" if on is None else on
    return {"schema": "optical-control-v1", "request_id": request.request_id,
            "generation": epoch.generation, "boot_id": epoch.boot_id, "host": epoch.host,
            "action": request.action, "completion": "COMPLETE", "admission": "ADMITTED",
            "kernel": "kernel75", "kernel_sha256": epoch.kernel_sha256,
            "helper_sha256": epoch.helper_sha256, "expires_at": epoch.expires_at,
            "release": "00", "pulse_ms": 10 if request.action in ("pulse-a", "pulse-b") else 0,
            "ir_state": "ON" if on else "OFF", "timer_owner": "target" if on else "none",
            "timer_remaining_ms": 4900 if on else 0, "timer_bound_ms": 5000 if on else 0,
            "proof": "TARGET_TIMER_ARMED" if on else "OFF_CONFIRMED"}


def encode(value):
    return json.dumps(value, separators=(",", ":")).encode()


class MockDriver:
    def __init__(self):
        self.calls = []
        self.threads = []
        self.result = None

    def status(self, request):
        self.calls.append(request)
        self.threads.append(threading.get_ident())
        if callable(self.result):
            return self.result(request)
        if self.result is not None:
            return self.result
        return encode(row(request))

    def act(self, action, request):
        assert action == request.action
        return self.status(request)


class ReceiptTests(unittest.TestCase):
    def test_all_actions_and_status_on_off(self):
        for action in ("status",) + ACTIONS:
            request = Request(REQUEST_ID, EPOCH, action)
            receipt(encode(row(request)), request, EPOCH.expires_at - 100)
        request = Request(REQUEST_ID, EPOCH, "status")
        receipt(encode(row(request, on=True)), request, EPOCH.expires_at - 100)

    def test_every_missing_and_extra_field(self):
        request = Request(REQUEST_ID, EPOCH, "status")
        value = row(request)
        for key in value:
            bad = dict(value)
            del bad[key]
            with self.subTest(key=key), self.assertRaises(ControlError):
                receipt(encode(bad), request, 100)
        with self.assertRaises(ControlError):
            receipt(encode(dict(value, unexpected=1)), request, 100)

    def test_all_bindings_and_types(self):
        request = Request(REQUEST_ID, EPOCH, "pulse-a")
        for key in row(request):
            for wrong in (None, True, [], {}, "UNKNOWN", float("inf")):
                bad = dict(row(request), **{key: wrong})
                with self.subTest(key=key, wrong=wrong), self.assertRaises(ControlError):
                    receipt(encode(bad), request, 100)
        for key in ("pulse_ms", "expires_at", "timer_remaining_ms", "timer_bound_ms"):
            bad = dict(row(request))
            bad[key] = float(bad[key])
            with self.assertRaises(ControlError):
                receipt(encode(bad), request, 100)

    def test_json_negative_paths(self):
        request = Request(REQUEST_ID, EPOCH, "status")
        valid = encode(row(request))
        bad = [b"", b"x" * 4097, valid.decode(), b"[]", b"null", b"true", b"\xff",
               b'{"x":NaN}', b'{"x":Infinity}', b'{"x":-Infinity}',
               valid[:-1] + b',"action":"status"}', b"[" * 1100 + b"]" * 1100]
        for raw in bad:
            with self.subTest(type=type(raw)), self.assertRaises(ControlError):
                receipt(raw, request, 100)

    def test_on_requires_target_timer_and_off_no_contradiction(self):
        request = Request(REQUEST_ID, EPOCH, "ir-on")
        for key, value in (("timer_owner", "desktop"), ("timer_remaining_ms", 0),
                           ("timer_remaining_ms", 5001), ("timer_bound_ms", 5001),
                           ("proof", "OFF_CONFIRMED"), ("release", "10"),
                           ("release", "11"), ("pulse_ms", 10)):
            with self.subTest(key=key, value=value), self.assertRaises(ControlError):
                receipt(encode(dict(row(request), **{key: value})), request, 100)
        for action, on in (("ir-on", False), ("ir-off", True)):
            request = Request(REQUEST_ID, EPOCH, action)
            with self.assertRaises(ControlError):
                receipt(encode(row(request, on=on)), request, 100)

    def test_expired_wrong_epoch_and_loopback_control(self):
        for epoch in (replace(EPOCH, expires_at=101), replace(EPOCH, expires_at=True),
                      replace(EPOCH, generation="wrong"), replace(EPOCH, boot_id="wrong"),
                      replace(EPOCH, host="127.0.0.1"), replace(EPOCH, host="8.8.8.8"),
                      replace(EPOCH, kernel_sha256="z" * 64), replace(EPOCH, helper_sha256="")):
            with self.assertRaises(ControlError):
                epoch.check(100)
        with self.assertRaises(ControlError):
            EPOCH.check(float("nan"))
        request = Request(REQUEST_ID, EPOCH, "status")
        with self.assertRaises(ControlError):
            receipt(encode(row(request)), replace(request, epoch=replace(EPOCH, boot_id=REQUEST_ID)), 100)


class ControlsTests(unittest.TestCase):
    def setUp(self):
        self.clock = Clock()
        self.driver = MockDriver()
        self.controls = Controls(self.driver, EPOCH, clock=self.clock, wall=lambda: 100)
        self.live = Preview("one", True, b"frame", 1, "Live")

    def complete(self):
        self.controls.thread.join(1)
        self.assertFalse(self.controls.thread.is_alive())
        self.controls.poll()

    def admit(self):
        self.controls.observe(self.live, EPOCH.host)
        self.assertTrue(self.controls.dispatch("status"))
        self.complete()
        self.assertTrue(self.controls.admitted)

    def test_startup_and_default_adapter_no_effect(self):
        default = Controls()
        self.assertFalse(default.dispatch("status"))
        self.assertFalse(default.dispatch("ir-on"))
        self.assertEqual(default.message, ADAPTER_PENDING)
        self.assertFalse(default.fault)
        self.assertEqual(self.driver.calls, [])
        self.assertFalse(self.controls.dispatch("status"))

    def test_fresh_frame_not_admission_and_status_worker_thread(self):
        self.controls.observe(self.live, EPOCH.host)
        for action in ACTIONS:
            self.assertFalse(self.controls.allowed(action))
        self.admit()
        self.assertTrue(all(self.controls.allowed(a) for a in ACTIONS))
        self.assertNotEqual(self.driver.threads[0], threading.get_ident())

    def test_no_frame_host_mismatch_or_epoch_expiry(self):
        self.controls.observe(self.live, "10.1.2.4")
        self.assertFalse(self.controls.allowed("status"))
        self.controls.observe(replace(self.live, ready=False), EPOCH.host)
        self.assertFalse(self.controls.allowed("status"))
        self.controls.wall = lambda: EPOCH.expires_at - 1
        self.controls.observe(self.live, EPOCH.host)
        self.assertFalse(self.controls.allowed("status"))

    def test_stop_stale_end_revoke_manual_admission(self):
        for reason in ("Stopped", "Stale", "Ended", "Error"):
            self.admit()
            self.controls.observe(replace(self.live, ready=False, reason=reason), EPOCH.host)
            self.controls.observe(self.live, EPOCH.host)
            self.assertFalse(self.controls.allowed("pulse-a"))
            self.assertFalse(self.controls.admitted)

    def test_old_status_cannot_revalidate_new_preview(self):
        self.controls.observe(self.live, EPOCH.host)
        self.assertTrue(self.controls.dispatch("status"))
        self.controls.observe(replace(self.live, ready=False), EPOCH.host)
        self.controls.observe(self.live, EPOCH.host)
        self.complete()
        self.assertFalse(self.controls.admitted)
        self.assertFalse(self.controls.allowed("ir-on"))
        self.controls.dispatch("status")
        self.controls.observe(replace(self.live, session="two"), EPOCH.host)
        self.complete()
        self.assertFalse(self.controls.admitted)

    def test_serialization_and_two_second_cooldown_no_repeat(self):
        self.admit()
        self.assertTrue(self.controls.dispatch("pulse-a"))
        self.assertFalse(self.controls.dispatch("pulse-b"))
        self.complete()
        self.assertFalse(self.controls.allowed("pulse-b"))
        self.clock.now += 1.999
        self.assertFalse(self.controls.allowed("pulse-b"))
        self.clock.now += .002
        self.assertTrue(self.controls.allowed("pulse-b"))
        self.assertEqual(len(self.driver.calls), 2)

    def test_independent_off_and_no_desktop_auto_off(self):
        self.admit()
        self.controls.dispatch("ir-on")
        self.complete()
        self.assertFalse(self.controls.allowed("pulse-a"))
        self.assertFalse(self.controls.allowed("ir-on"))
        self.assertTrue(self.controls.allowed("ir-off"))
        count = len(self.driver.calls)
        self.clock.now += 6
        self.controls.poll()
        self.assertEqual(len(self.driver.calls), count)
        self.assertTrue(self.controls.allowed("pulse-a"))  # Pair control is independent.
        self.assertFalse(self.controls.allowed("ir-on"))  # No unproved rearm/extension.
        self.controls.dispatch("ir-off")
        self.complete()
        self.assertEqual(self.controls.ir_until, 0)

    def test_filter_pulse_while_independently_timed_ir_is_on(self):
        self.admit()
        self.driver.result = lambda request: encode(row(request, on=True))
        self.assertTrue(self.controls.dispatch("ir-on"))
        self.complete()
        self.clock.now += 2.001
        self.assertTrue(self.controls.allowed("pulse-a"))
        self.assertTrue(self.controls.allowed("pulse-b"))
        self.assertFalse(self.controls.allowed("ir-on"))
        self.assertTrue(self.controls.dispatch("pulse-a"))
        self.complete()
        self.assertFalse(self.controls.fault)
        self.assertGreater(self.controls.ir_until, self.clock.now)
        self.assertTrue(self.controls.allowed("ir-off"))
        self.assertEqual([r.action for r in self.driver.calls], ["status", "ir-on", "pulse-a"])

    def test_malformed_unknown_exception_duplicate_receipt_sticky(self):
        for result in (b"not-json", lambda r: encode(dict(row(r), release="11")),
                       lambda r: encode(dict(row(r), completion="UNKNOWN")),
                       lambda r: (_ for _ in ()).throw(RuntimeError("private rtsp://secret"))):
            self.controls = Controls(self.driver, EPOCH, clock=self.clock, wall=lambda: 100)
            self.driver.result = None
            self.admit()
            previous = encode(row(self.driver.calls[-1]))
            self.driver.result = result
            self.controls.dispatch("pulse-a")
            self.complete()
            self.assertTrue(self.controls.fault)
            self.assertTrue(self.controls.busy)
            self.assertFalse(self.controls.close_allowed())
            self.controls.observe(self.live, EPOCH.host)
            self.assertFalse(self.controls.dispatch("status"))
            self.assertNotIn("secret", self.controls.message)
        self.controls = Controls(self.driver, EPOCH, clock=self.clock, wall=lambda: 100)
        self.driver.result = previous
        self.controls.observe(self.live, EPOCH.host)
        self.controls.dispatch("status")
        self.complete()
        self.assertTrue(self.controls.fault)  # Replayed request nonce denied.

    def test_timeout_sticky_even_after_late_valid_receipt(self):
        self.admit()
        release = threading.Event()
        self.driver.result = lambda r: (release.wait(1), encode(row(r)))[1]
        try:
            self.controls.dispatch("pulse-b")
            self.clock.now += 8
            self.controls.poll()
            self.assertTrue(self.controls.fault)
            self.assertTrue(self.controls.busy)
        finally:
            release.set()
            self.controls.thread.join(1)
        self.controls.poll()
        self.controls.observe(self.live, EPOCH.host)
        self.assertFalse(self.controls.allowed("ir-off"))
        self.assertFalse(self.controls.close_allowed())
        self.assertEqual(len(self.driver.calls), 2)

    def test_known_no_send_denial_does_not_trap_close(self):
        self.admit()
        self.driver.result = lambda request: (_ for _ in ()).throw(NotAdmitted('No target TX'))
        self.assertTrue(self.controls.dispatch('pulse-a'))
        self.complete()
        self.assertFalse(self.controls.busy)
        self.assertFalse(self.controls.fault)
        self.assertFalse(self.controls.admitted)
        self.assertTrue(self.controls.close_allowed())
        self.assertIn('no camera command sent', self.controls.message)

    def test_delayed_UI_accepts_already_queued_timely_completion_for_close(self):
        self.admit()
        self.controls.dispatch('pulse-a')
        self.controls.thread.join(1)
        self.clock.now += 20  # E.g. a blocked Tk/window-manager event loop.
        self.controls.observe(replace(self.live, ready=False), EPOCH.host)
        self.controls.poll()
        self.assertFalse(self.controls.busy)
        self.assertFalse(self.controls.fault)
        self.assertFalse(self.controls.admitted)
        self.assertTrue(self.controls.close_allowed())

    def test_expiry_after_timely_completion_does_not_reclassify_as_unknown(self):
        self.admit()
        self.controls.dispatch('pulse-b')
        self.controls.thread.join(1)
        self.controls.wall = lambda: EPOCH.expires_at + 1
        self.controls.observe(self.live, EPOCH.host)
        self.controls.poll()
        self.assertTrue(self.controls.close_allowed())
        self.assertFalse(self.controls.fault)
        self.assertFalse(self.controls.allowed('pulse-a'))

    def test_no_send_proof_can_retire_a_delayed_alert_not_a_real_effect(self):
        self.admit()
        release = threading.Event()
        def denied(request):
            release.wait(1)
            raise NotAdmitted('Admission only, no send RPC')
        self.driver.result = denied
        try:
            self.controls.dispatch('pulse-a')
            self.clock.now += 8
            self.controls.poll()
            self.assertTrue(self.controls.fault)
        finally:
            release.set()
            self.controls.thread.join(1)
        self.controls.poll()
        self.assertTrue(self.controls.close_allowed())
        self.assertFalse(self.controls.fault)

    def test_UI_disables_before_backend_75_second_admission_floor(self):
        epoch = replace(EPOCH, expires_at=200)
        controls = Controls(self.driver, epoch, clock=self.clock, wall=lambda: 100)
        controls.observe(self.live, epoch.host)
        self.assertTrue(controls.allowed('status'))
        controls.wall = lambda: 111  # 89s left: no request may be started.
        controls.observe(self.live, epoch.host)
        self.assertFalse(controls.allowed('status'))
        self.assertTrue(controls.close_allowed())

    def test_ui_state_methods_reject_worker_thread(self):
        errors = []
        def work():
            for call in (lambda: self.controls.observe(self.live, EPOCH.host),
                         lambda: self.controls.allowed("status"), self.controls.poll,
                         lambda: self.controls.dispatch("status"), self.controls.close_allowed):
                try:
                    call()
                except RuntimeError:
                    errors.append(True)
        thread = threading.Thread(target=work)
        thread.start()
        thread.join(1)
        self.assertEqual(len(errors), 5)


if __name__ == "__main__":
    unittest.main()
