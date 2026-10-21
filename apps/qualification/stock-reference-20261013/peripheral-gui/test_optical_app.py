"""Mock desktop + optional fail-closed evidence runner. Never launches a GUI."""
from dataclasses import replace
import hashlib
import io
import json
import os
from pathlib import Path
import sys
import threading
import unittest
import tempfile
import fcntl
from unittest.mock import patch

import optical_app
from optical_control import Controls, NotAdmitted
from optical_video import Preview
from test_optical_control import EPOCH, MockDriver
from test_optical_video import Clock


class Element:
    def __init__(self, gui, key):
        self.gui, self.key = gui, key

    def update(self, *args, **kwargs):
        self.gui.check_thread()
        self.gui.updates.setdefault(self.key, []).append((args, kwargs))


class Window:
    def __init__(self, gui):
        self.gui = gui

    def read(self, timeout):
        self.gui.check_thread()
        if not self.gui.events:
            raise AssertionError("Fixture failed to close its mock GUI")
        event = self.gui.events.pop(0)
        if callable(event):
            event = event()
        return event, dict(self.gui.values)

    def __getitem__(self, key):
        return Element(self.gui, key)

    def close(self):
        self.gui.check_thread()
        self.gui.closed = True


class GUI:
    WIN_CLOSED = "WINDOW_CLOSED"
    WINDOW_CLOSE_ATTEMPTED_EVENT = "CLOSE_ATTEMPT"

    def __init__(self, events, values=None):
        self.owner = threading.get_ident()
        self.events = list(events)
        self.values = values or {"HOST": EPOCH.host, "SECONDS": "60"}
        self.updates = {}
        self.closed = False
        self.layout = None

    def check_thread(self):
        assert threading.get_ident() == self.owner, "GUI used by worker thread"

    def theme(self, _):
        self.check_thread()

    def Window(self, title, layout, **kwargs):
        self.check_thread()
        self.layout = layout
        return Window(self)

    def _element(self, *args, **kwargs):
        self.check_thread()
        return args, kwargs

    Text = Input = Button = Image = _element


class MockDecoder:
    instances = []

    def __init__(self):
        self.instances.append(self)
        self.preview = Preview("one", False, None, 0, "Waiting")
        self.starts = []
        self.stops = 0
        self.closes = 0

    def start(self, *args):
        self.starts.append(args)

    def snapshot(self):
        return self.preview

    def stop(self):
        self.stops += 1
        self.preview = replace(self.preview, ready=False, frame=None, reason="Stopped")

    def close(self):
        self.stop()
        self.closes += 1


class AppTests(unittest.TestCase):
    def setUp(self):
        MockDecoder.instances = []

    def test_startup_close_no_decoder_or_driver_calls(self):
        driver = MockDriver()
        gui = GUI(["Close"])
        optical_app.run(gui, driver=driver, epoch=EPOCH, decoder_factory=MockDecoder)
        self.assertEqual(MockDecoder.instances, [])
        self.assertEqual(driver.calls, [])
        self.assertTrue(gui.closed)
        keys = {kwargs.get("key") for line in gui.layout for _, kwargs in line}
        for forbidden in ("speaker-tone", "mic-start", "ambient"):
            self.assertNotIn(forbidden, keys)
        for expected in ('white-on', 'white-off', 'motor0-minus', 'motor1-plus'):
            self.assertIn(expected, keys)

    def test_default_status_has_explanation_and_no_effect(self):
        gui = GUI(["status", "pulse-a", "ir-on", "Close"])
        optical_app.run(gui, decoder_factory=MockDecoder)
        self.assertEqual(MockDecoder.instances, [])
        self.assertIn("NOT YET ADMITTED", str(gui.updates["CONTROL"]))

    def test_connect_explicit_stop_reaps_only_decoder(self):
        gui = GUI(["connect", "stop", "Close"])
        driver = MockDriver()
        optical_app.run(gui, driver=driver, epoch=EPOCH, decoder_factory=MockDecoder)
        decoder, = MockDecoder.instances
        self.assertEqual(decoder.starts, [(EPOCH.host, 60, False)])
        self.assertEqual(decoder.closes, 1)
        self.assertEqual(driver.calls, [])
        self.assertEqual(gui.updates["FRAME"][-1], ((), {}))

    def test_close_reaps_local_decoder_without_remote_off(self):
        gui = GUI(["connect", "Close"])
        driver = MockDriver()
        optical_app.run(gui, driver=driver, epoch=EPOCH, decoder_factory=MockDecoder)
        self.assertEqual(MockDecoder.instances[0].closes, 1)
        self.assertEqual(driver.calls, [])

    def test_invalid_endpoints_and_duration_do_not_construct_decoder(self):
        for values in ({"HOST": "rtsp://10.1.2.3/live", "SECONDS": "60"},
                       {"HOST": EPOCH.host, "SECONDS": "1e2"},
                       {"HOST": EPOCH.host, "SECONDS": "NaN"},
                       {"HOST": EPOCH.host, "SECONDS": "0"},
                       {"HOST": EPOCH.host, "SECONDS": "301"},
                       {"HOST": EPOCH.host, "SECONDS": " 60"},
                       {"HOST": EPOCH.host, "SECONDS": "９"}):
            gui = GUI(["connect", "Close"], values)
            optical_app.run(gui, decoder_factory=MockDecoder)
        self.assertEqual(MockDecoder.instances, [])

    def test_injected_control_path_live_status_action_stale_blocks(self):
        driver = MockDriver()
        controls = Controls(driver, EPOCH, wall=lambda: 100)
        def live():
            decoder = MockDecoder.instances[0]
            decoder.preview = Preview("one", True, b"fixture-png", 1, "Live")
            return "pulse-a"  # A frame alone does NOT admit this click.
        def complete():
            controls.thread.join(1)
            return "timeout"
        def stale():
            decoder = MockDecoder.instances[0]
            decoder.preview = replace(decoder.preview, ready=False, frame=None, reason="Stale")
            return "pulse-b"
        gui = GUI(["connect", live, "status", complete, "pulse-a", complete, stale, "Close"])
        with patch.object(optical_app, "Controls", return_value=controls):
            optical_app.run(gui, driver=driver, epoch=EPOCH, decoder_factory=MockDecoder)
        self.assertEqual([r.action for r in driver.calls], ["status", "pulse-a"])
        self.assertTrue(all(t != gui.owner for t in driver.threads))
        self.assertFalse(controls.admitted)
        self.assertTrue(gui.closed)
        self.assertEqual(gui.updates["FRAME"][-1], ((), {}))

    def test_main_uses_shared_instance_lock_and_default_has_no_driver(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get(
                'TMPDIR', str(optical_app.INSTANCE_LOCK.parents[1]))) as tmp:
            lock = Path(tmp) / 'instance.lock'
            gui = GUI([])
            with patch.object(optical_app, 'INSTANCE_LOCK', lock), \
                    patch.dict(sys.modules, {'FreeSimpleGUI': gui}), \
                    patch.object(optical_app, 'run') as run:
                optical_app.main([])
                self.assertIsNone(run.call_args.kwargs['driver'])
                self.assertIsNone(run.call_args.kwargs['epoch'])
                run.reset_mock()
                with lock.open('a') as held:
                    fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
                    with self.assertRaises(SystemExit):
                        optical_app.main([])
                    run.assert_not_called()

    def test_stale_status_completion_cannot_reenable_gui(self):
        driver = MockDriver()
        controls = Controls(driver, EPOCH, wall=lambda: 100)
        def live():
            MockDecoder.instances[0].preview = Preview("one", True, b"fixture", 1, "Live")
            return "status"
        def stale():
            controls.thread.join(1)
            MockDecoder.instances[0].preview = Preview("one", False, None, 1, "Error")
            return "pulse-a"
        gui = GUI(["connect", live, stale, "Close"])
        with patch.object(optical_app, "Controls", return_value=controls):
            optical_app.run(gui, driver=driver, epoch=EPOCH, decoder_factory=MockDecoder)
        self.assertEqual([r.action for r in driver.calls], ["status"])
        self.assertFalse(controls.admitted)

    def test_close_during_pending_control_stops_only_decoder_and_waits(self):
        driver = MockDriver()
        controls = Controls(driver, EPOCH, wall=lambda: 100)
        release = threading.Event()
        def live():
            MockDecoder.instances[0].preview = Preview("one", True, b"fixture", 1, "Live")
            return "status"
        def complete():
            controls.thread.join(1)
            return "timeout"
        def pending_action():
            from test_optical_control import encode, row
            driver.result = lambda request: (release.wait(1), encode(row(request)))[1]
            return "pulse-a"
        def release_action():
            self.assertFalse(gui.closed)
            self.assertGreater(MockDecoder.instances[0].stops, 0)
            release.set()
            controls.thread.join(1)
            return "timeout"
        gui = GUI(["connect", live, complete, pending_action, "Close", release_action, "Close"])
        try:
            with patch.object(optical_app, "Controls", return_value=controls):
                optical_app.run(gui, driver=driver, epoch=EPOCH, decoder_factory=MockDecoder)
        finally:
            release.set()
        self.assertEqual([r.action for r in driver.calls], ["status", "pulse-a"])
        self.assertTrue(gui.closed)

    def test_GUI_can_close_after_proven_preTX_admission_denial(self):
        driver = MockDriver()
        controls = Controls(driver, EPOCH, wall=lambda: 100)
        def live():
            MockDecoder.instances[0].preview = Preview('one', True, b'fixture', 1, 'Live')
            return 'status'
        def complete():
            controls.thread.join(1)
            return 'timeout'
        def denied_action():
            def denied(request):
                raise NotAdmitted('Expired before target TX')
            driver.result = denied
            return 'pulse-a'
        gui = GUI(['connect', live, complete, denied_action, complete, 'Close'])
        with patch.object(optical_app, 'Controls', return_value=controls):
            optical_app.run(gui, driver=driver, epoch=EPOCH, decoder_factory=MockDecoder)
        self.assertTrue(gui.closed)
        self.assertFalse(controls.fault)
        self.assertFalse(controls.busy)
        self.assertEqual([x.action for x in driver.calls], ['status', 'pulse-a'])

    def test_GUI_delayed_event_loop_retires_already_timely_status_and_closes(self):
        clock = Clock()
        driver = MockDriver()
        controls = Controls(driver, EPOCH, clock=clock, wall=lambda: 100)
        def live():
            MockDecoder.instances[0].preview = Preview('one', True, b'fixture', 1, 'Live')
            return 'status'
        def delayed_close():
            controls.thread.join(1)
            clock.now += 20
            return 'Close'
        gui = GUI(['connect', live, delayed_close])
        with patch.object(optical_app, 'Controls', return_value=controls):
            optical_app.run(gui, driver=driver, epoch=EPOCH, decoder_factory=MockDecoder)
        self.assertTrue(gui.closed)
        self.assertFalse(controls.fault)
        self.assertEqual([x.action for x in driver.calls], ['status'])


def offline_evidence(directory):
    """Fresh output only, deny all external processes/network/device/private IO."""
    import datetime
    import importlib.metadata
    import shlex
    import sysconfig
    import test_optical_control
    import test_optical_video
    import test_optical_uart
    import test_optical_aim
    import test_optical_aim_backend

    source = Path(__file__).resolve().parent
    output = Path(directory).resolve()
    expected = source.parents[3] / "evidence/delegated/20261016-optical-viewer"
    import re
    if output.parent != expected or not re.fullmatch(r"run[0-9]{2}", output.name):
        raise SystemExit("Evidence output must be a fresh runNN in the optical-viewer evidence directory")
    output.mkdir(parents=True, exist_ok=False)
    (output / 'tmp').mkdir()
    import os
    os.environ['TMPDIR'] = str(output / 'tmp')
    files = (sorted(source.glob("optical_*.py")) + sorted(source.glob("test_optical*.py")) +
             [source / "optical_README.md", source / "optical_dispatch.sh", source / "aim_backend.py",
              source / 'aim_dispatch.sh'])
    hashes = {str(path.relative_to(source)): hashlib.sha256(path.read_bytes()).hexdigest()
              for path in files}
    metadata = {"started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                "command": shlex.join([sys.executable, "-B", *sys.argv]),
                "python": sys.version, "platform": sys.platform,
                "FreeSimpleGUI": importlib.metadata.version("FreeSimpleGUI"),
                "input_sha256": hashes, "validation": "OFFLINE_MOCK_ONLY",
                "FFmpeg": "not executed; command/process mocked", "gui_launched": False,
                "network_or_hardware": False}
    allowed = (source, output, Path(sysconfig.get_path("stdlib")),
               Path(sys.prefix).resolve(), Path(sys.base_prefix).resolve() / "lib")
    def audit(event, args):
        if event.startswith("socket.") or event in ("subprocess.Popen", "os.system", "os.exec", "os.posix_spawn", "pty.spawn"):
            raise AssertionError("Offline unit denies process/network access")
        if event == "open":
            path = args[0]
            if isinstance(path, int):
                raise AssertionError("Offline unit denies descriptor opens")
            candidate = Path(path).resolve()
            if not any(candidate.is_relative_to(root) for root in allowed):
                raise AssertionError("Offline unit denies private/device/unrelated files")
    sys.addaudithook(audit)
    suite = unittest.TestSuite(unittest.defaultTestLoader.loadTestsFromModule(module)
                               for module in (test_optical_video, test_optical_control,
                                              test_optical_uart, test_optical_aim,
                                              test_optical_aim_backend, sys.modules[__name__]))
    log = io.StringIO()
    result = unittest.TextTestRunner(stream=log, verbosity=2).run(suite)
    (output / "tests.txt").write_text(log.getvalue())
    metadata.update(tests=result.testsRun, failures=len(result.failures), errors=len(result.errors),
                    finished_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
    (output / "result.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(log.getvalue(), end="")
    print("Evidence:", output)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--evidence":
        raise SystemExit(offline_evidence(sys.argv[2]))
    unittest.main()
