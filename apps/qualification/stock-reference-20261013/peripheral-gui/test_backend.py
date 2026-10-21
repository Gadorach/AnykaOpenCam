import tempfile
import time
import unittest
import subprocess
import json
from pathlib import Path

from backend import Bench, BenchError, MOVES, response_lines, AUDIO_PATHS, command_packet


class FakeBench(Bench):
    def __init__(self, directory):
        super().__init__("a" * 64, directory)
        self.commands = []
        self.response = ""

    def run(self, command, label, timeout=12):
        self.commands.append((command, label))
        return self.response


class Tests(unittest.TestCase):
    def test_stock_line_limit_refuses_before_transmission(self):
        self.assertLessEqual(len(command_packet("x" * 900, "a" * 32)), 1000)
        for command in ("x" * 1000, "echo one\necho two", "echo one\recho two"):
            with self.assertRaises(BenchError):
                command_packet(command, "a" * 32)

    def test_retail_nul_does_not_hide_marker(self):
        self.assertEqual(response_lines("0\r\n\x00GUI_END_abc\r\n~ # "), ["0", "GUI_END_abc", "~ # "])

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.b = FakeBench(Path(self.tmp.name) / "session")

    def tearDown(self):
        self.tmp.cleanup()

    def test_fixed_motors(self):
        self.b.response = '{"record":"motor-map-v2","motor":0}\nGUI_MOTOR_EXIT:0'
        for key in MOVES:
            self.b.motor(key)
            cmd, _ = self.b.commands[-1]
            self.assertIn(MOVES[key], cmd)
            self.assertIn("exec 3>&-", cmd)
            self.assertNotIn("motor-reset", cmd)
        with self.assertRaises(BenchError):
            self.b.motor("motor0-plus; rm anything")

    def test_no_false_motor_success(self):
        for text in ("GUI_MOTOR_EXIT:2", "GUI_BLOCKED_BUSY", "GUI_MOTOR_EXIT:0"):
            self.b.response = text
            with self.assertRaises(BenchError):
                self.b.motor("motor0-plus")

    def test_status_requires_both_receipts(self):
        self.b.response = ('{"record":"motor-gui-v1","state":"STATUS","motor":0}\n'
                           '{"record":"motor-gui-v1","state":"STATUS","motor":1}')
        self.b.status()
        self.b.response = "cat: missing helper"
        with self.assertRaises(BenchError):
            self.b.status()

    def test_led_timer_off_survives_gui(self):
        self.b.response = "GUI_LED_TIMER_STARTED:123"
        for kind in ("white", "ir"):
            self.b.pulse_until = 0
            self.b.led(kind, True)
            cmd, _ = self.b.commands[-1]
            self.assertIn("sleep 5", cmd)
            self.assertIn("trap 'printf 0", cmd)
            self.assertIn(" >| /sys/user-gpio/", cmd)
            self.assertIn("IFS= read -r gui_led_level", cmd)
            self.assertNotIn("/bin/busybox tr", cmd)
            parsed = subprocess.run(["/bin/sh", "-n", "-c", cmd], capture_output=True, timeout=3)
            self.assertEqual(parsed.returncode, 0, parsed.stderr)
            self.assertGreater(self.b.pulse_until, time.monotonic())
            self.b.led(kind, False)  # Explicit off remains possible during timer.
            with self.assertRaises(BenchError):
                self.b.motor("motor0-plus")
            with self.assertRaises(BenchError):
                self.b.led(kind, True)

    def test_reject_other_outputs(self):
        for pin in ("gpio2", "indicator", "/sys/user-gpio/gpio-led"):
            with self.assertRaises(ValueError):
                self.b.led(pin, True)
        self.assertEqual(self.b.commands, [])

    def test_builtin_baseline_with_retail_nul(self):
        script = ('if IFS= read -r gui_led_level && test "$gui_led_level" = 0; '
                  'then printf READY; else printf BLOCKED; fi')
        for data, expected in [(b"0\n\0", b"READY"), (b"0\n", b"READY"),
                               (b"1\n\0", b"BLOCKED"), (b"unexpected\n", b"BLOCKED"),
                               (b"\n", b"BLOCKED"), (b"", b"BLOCKED")]:
            with self.subTest(data=data):
                result = subprocess.run(["/bin/sh", "-c", script], input=data,
                                        capture_output=True, timeout=3)
                self.assertEqual(result.stdout, expected)

    def test_no_pulse_for_bad_baseline(self):
        self.b.response = "GUI_BLOCKED_LED_BASELINE"
        self.b.led("white", True)
        self.assertEqual(self.b.pulse_until, 0)

    def test_off_only_known_led_paths(self):
        self.b.lights_off()
        cmd, _ = self.b.commands[-1]
        self.assertIn("gpio-light", cmd)
        self.assertIn("gpio-ir", cmd)
        self.assertNotIn("gpio-led", cmd)
        self.assertNotIn("gpio2", cmd)

    def test_audio_fixed_helpers_and_privacy_receipt(self):
        self.b.audio = {"mic": {"path": AUDIO_PATHS["mic"], "sha256": "b" * 64},
                        "speaker": {"path": AUDIO_PATHS["speaker"], "sha256": "c" * 64}}
        self.b.response = ('{"record":"mic-level-v3","samples":8000,"raw_audio_output":false,'
                           '"audio_hash_output":false,"close_ok":true}\nGUI_AUDIO_EXIT:0')
        self.b.audio_test("mic")
        cmd, _ = self.b.commands[-1]
        self.assertIn("--native-stock-mic-level1s", cmd)
        self.assertIn("kill -STOP $$", cmd)
        self.assertIn("/bin/busybox sh -c", cmd)
        self.assertNotIn("/dev/akpcm_cdev0 >", cmd)
        self.b.response = "GUI_AUDIO_EXIT:2"
        with self.assertRaises(BenchError):
            self.b.audio_test("mic")
        self.b.response = ('{"record":"mic-level-v3","samples":8000,"raw_audio_output":true,'
                           '"audio_hash_output":false,"close_ok":true}\nGUI_AUDIO_EXIT:0')
        with self.assertRaises(BenchError):
            self.b.audio_test("mic")

    def test_audio_rejects_unknown_kind_path_and_overlap(self):
        with self.assertRaises(BenchError):
            self.b.audio_test("anything")
        self.b.audio = {"mic": {"path": "/dev/akpcm_cdev0", "sha256": "a" * 64}}
        with self.assertRaises(BenchError):
            self.b.audio_test("mic")
        self.b.pulse_until = time.monotonic() + 5
        with self.assertRaises(BenchError):
            self.b.audio_test("speaker")

    def test_speaker_exact_native_receipt_and_retirement(self):
        self.b.audio = {"speaker": {"path": AUDIO_PATHS["speaker"], "sha256": "c" * 64}}
        row = {"record": "stock-speaker-tone-v3", "state": "TONE_COMPLETED_AWAIT_USER",
               "phase": 10, "requested_rate": 8000, "channels": 2, "bits": 16,
               "tone_frames": 1792, "digital_peak": 768, "prefill_bytes": 8192,
               "kernel_ring_bytes": 49152, "pause_completed": True,
               "DMA_stop_evidence": "SOURCE_CONDITIONAL_PAUSE_IRQ_COMPLETION",
               "close_ok": True, "close_uncertain": False, "gain_code": 1,
               "gain_modified": False, "gain_preserved": True, "MMIO_writes": True,
               "MMIO_write_count": 1, "MMIO_write_address": "0x2017000c",
               "MMIO_write_mask": 2, "MMIO_after_close": True,
               "restore_before": 0, "restore_after": 2, "latch_restoration_verified": True,
               "OUT0_before": 2, "OUT0_after": 2,
               "DIR0_before": 0x0198ece6, "DIR0_after": 0x0198ece6,
               "bank_lease": "SPEAKER_TONE03_PARENT_GPIO0_BANK_LEASE_V1",
               "bank_lease_verified_by_helper": False,
               "SYS74_before": 0x02000186, "SYS74_after": 0x02000186,
               "physical_heard": "AWAIT_USER"}
        def response(value):
            return json.dumps(value, separators=(",", ":")) + "\nGUI_AUDIO_EXIT:0"
        self.b.response = response(row)
        self.b.audio_test("speaker")
        cmd, _ = self.b.commands[-1]
        self.assertIn("--native-stock-speaker-tone", cmd)
        self.assertIn("motor0-status &&", cmd)
        self.assertIn("motor1-status &&", cmd)
        self.assertNotIn("--native-stock-mic", cmd)
        self.assertNotIn("devmem", cmd)
        parsed = subprocess.run(["/bin/sh", "-n", "-c", cmd], capture_output=True, timeout=3)
        self.assertEqual(parsed.returncode, 0, parsed.stderr)
        for field, value in [("record", "stock-speaker-tone-v2"), ("close_ok", False),
                             ("pause_completed", False), ("close_uncertain", True),
                             ("MMIO_writes", False), ("gain_modified", True),
                             ("SYS74_after", 0x02000184), ("SYS74_after", 0x02000106),
                             ("OUT0_after", 0), ("digital_peak", 32767),
                             ("MMIO_write_address", "0x08000074"), ("MMIO_write_count", 2),
                             ("MMIO_write_count", True), ("MMIO_after_close", False),
                             ("MMIO_write_mask", 4), ("DIR0_after", 0x0198ece4),
                             ("latch_restoration_verified", False), ("gain_preserved", False),
                             ("bank_lease_verified_by_helper", True),
                             ("physical_heard", "CONFIRMED"), ("phase", True)]:
            with self.subTest(field=field, value=value):
                self.b.response = response(dict(row, **{field: value}))
                with self.assertRaises(BenchError):
                    self.b.audio_test("speaker")
        self.b.response = response(dict(row, MMIO_writes=False, MMIO_write_count=0,
                                        MMIO_after_close=False, MMIO_write_address=None,
                                        restore_before=2))
        self.b.audio_test("speaker")
        for suffix in ["\nGUI_AUDIO_EXIT:1", "\n" + response(row)]:
            self.b.response = response(row) + suffix
            with self.assertRaises(BenchError):
                self.b.audio_test("speaker")


if __name__ == "__main__":
    unittest.main()
