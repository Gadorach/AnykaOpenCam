"""GUI event-loop fixtures: real layout code, no desktop/camera/transport calls."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import app


class Element:
    def __init__(self, value=None, **kwargs):
        self.value = value
        self.options = kwargs

    def update(self, value=None, **kwargs):
        if value is not None:
            self.value = value
        self.options.update(kwargs)

    def get(self):
        return self.value


class GUI:
    WIN_CLOSED = "-CLOSED-"
    WINDOW_CLOSE_ATTEMPTED_EVENT = "-CLOSE-"

    def __init__(self, events):
        self.events = list(events)
        self.elements = {}
        self.results = []

    def theme(self, *args):
        pass

    def element(self, value=None, *args, **kwargs):
        if "default_value" in kwargs:
            value = kwargs["default_value"]
        if "default" in kwargs:
            value = kwargs["default"]
        item = Element(value, **kwargs)
        if "key" in kwargs:
            self.elements[kwargs["key"]] = item
        return item

    Text = Button = Input = Combo = Checkbox = ProgressBar = Multiline = Frame = element

    def Window(self, *args, **kwargs):
        return self

    def __getitem__(self, key):
        return self.elements[key]

    def read(self, timeout):
        if not self.events:
            raise AssertionError("Unexpected operation/poll or missing Close")
        event = self.events.pop(0)
        values = {k: element.value for k, element in self.elements.items()}
        if event == "-RESULT-":
            values[event] = self.results.pop(0)
        elif event == "UART_ACK":
            self.elements[event].value = True
            values[event] = True
        return event, values

    def write_event_value(self, event, value):
        self.results.append(value)

    def close(self):
        pass


class Thread:
    def __init__(self, target, **kwargs):
        self.target = target

    def start(self):
        self.target()


class Bench:
    def __init__(self, *args):
        self.rate = None
        self.audio = {"mic": True}
        self.pulse_until = 0
        self.calls = []

    def uncertain_running(self):
        return False

    def status(self):
        self.calls.append("status")
        self.rate = 8012
        return json.dumps({"record": "peripheral-status-v7", "calculated_DAC_rate": 8012, "frequency_max_hz": 4005})

    def tone(self, *args):
        self.calls.append(("tone", args))
        return json.dumps({"record": "stock-tone-control-v7", "duration_ms": 250})

    def ambient(self):
        self.calls.append("ambient")
        return json.dumps({"record": "stock-ambient-control-v1", "value": 2999})

    def audio_test(self, kind):
        self.calls.append(kind)
        return json.dumps({"record": "mic-level-v3", "rms": 25, "half_ac_rms": [24, 25], "clipped": 0})


class UITests(unittest.TestCase):
    def run_gui(self, gui, smoke=False, bench=None):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "work/qc-uart-live").mkdir(parents=True)
            bench = bench or Bench()
            argv = ["app.py", "--ssh-profile", str(root / "unused-mock-profile.json")]
            (root / "unused-mock-profile.json").write_text("MOCK_ONLY")
            if smoke:
                argv += ["--smoke-ui"]
            with patch.object(app, "ROOT", root), patch.object(app, "sg", gui), \
                    patch.object(app, "SSHBench", return_value=bench) as factory, \
                    patch.object(app, "Bench", side_effect=AssertionError("UART backend")), \
                    patch.object(app.threading, "Thread", Thread), patch("sys.argv", argv):
                app.main()
                if smoke:
                    factory.assert_not_called()
            return bench

    def test_startup_no_operations_all_controls_preserved_ack_false(self):
        gui = GUI(["Close"])
        bench = self.run_gui(gui)
        self.assertEqual(bench.calls, [])
        self.assertFalse(gui["UART_ACK"].get())
        self.assertEqual(gui["TONE_HZ"].get(), "1000")
        self.assertEqual(gui["TONE_GAIN"].get(), 3)
        self.assertEqual(gui["TONE_VOLUME"].get(), "25")
        self.assertEqual(gui["TONE_SECONDS"].get(), "0.25")
        for key in ("motor0-minus", "motor0-plus", "motor1-minus", "motor1-plus", "white-on", "white-off", "ir-on", "ir-off", "lights-off", "speaker-tone", "tone", "ambient"):
            self.assertTrue(gui[key].options["disabled"])

    def test_status_does_not_sample_or_play_and_ack_gates_tone(self):
        gui = GUI(["refresh", "-RESULT-", "Close"])
        bench = self.run_gui(gui)
        self.assertEqual(bench.calls, ["status"])
        self.assertIn("4005", gui["TONE_LIMIT"].get())
        self.assertTrue(gui["tone"].options["disabled"])
        self.assertFalse(gui["ambient"].options["disabled"])
        self.assertTrue(gui["motor0-plus"].options["disabled"])

    def test_explicit_tone_ambient_and_mic_stop_after_current(self):
        gui = GUI(["refresh", "-RESULT-", "UART_ACK", "tone", "-RESULT-", "ambient", "-RESULT-",
                   "mic-start", "mic-stop", "-RESULT-", "Close"])
        bench = self.run_gui(gui)
        self.assertEqual(bench.calls, ["status", ("tone", ("1000", 3, "25", "0.25", True)), "ambient", "mic"])
        self.assertEqual(gui["AMBIENT_BAR"].get(), 2999)
        self.assertIn("higher = darker", gui["AMBIENT_VALUE"].get())

    def test_smoke_constructs_neither_backend(self):
        bench = self.run_gui(GUI(["Close"]), smoke=True)
        self.assertEqual(bench.calls, [])

    def test_fault_disables_refresh_and_cannot_be_cleared_by_event(self):
        bench = Bench()
        def failed_status():
            bench.calls.append("status")
            raise app.BenchError("Mock failure; power OFF")
        bench.status = failed_status
        gui = GUI(["refresh", "-RESULT-", "refresh", "ambient", "Close"])
        self.run_gui(gui, bench=bench)
        self.assertEqual(bench.calls, ["status"])
        self.assertTrue(gui["refresh"].options["disabled"])
        self.assertTrue(gui["ambient"].options["disabled"])


if __name__ == "__main__":
    unittest.main()
