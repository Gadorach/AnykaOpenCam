import datetime as dt
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from backend import BenchError
from ssh_backend import SSHBench, CONSOLE, load_profile, tone_parameters, validate_tone


def status():
    return {"record": "peripheral-status-v7", "state": "STATUS", "calculated_DAC_rate": 8012,
            "frequency_max_hz": 4005, "HP_gain_code": 1, "SYS74": 0x2000186,
            "SYS78": 0, "DIR0": 0x198ece6, "OUT0": 2, "ambient_path_present": True}


def tone(parameters=(1000, 3, 25, 250), original_gain=1):
    frequency, gain, volume, duration = parameters
    row = {"record": "stock-tone-control-v7", "state": "TONE_COMPLETED_AWAIT_USER",
           "frequency_hz": frequency, "gain_test": gain, "volume_percent": volume,
           "duration_ms": duration, "digital_peak": (32767 * volume + 50) // 100,
           "frames_generated": (8012 * duration + 500) // 1000, "calculated_DAC_rate": 8012,
           "frequency_max_hz": 4005, "phase": 10, "effects": 5,
           "channels": 2, "bits": 16, "kernel_ring_bytes": 49152,
           "kernel_ring_backing_bytes": 65536, "scratch_bytes": 512, "stream_error": 0,
           "gain_SET_attempts": 2, "bank_lease_verified_by_helper": False,
           "UART_disconnection_admission": "USER_UART_SIGNALS_DISCONNECTED",
           "physical_heard": "AWAIT_USER", "pause_completed": True, "close_ok": True,
           "close_uncertain": False, "DMA_stop_evidence": "SOURCE_CONDITIONAL_PAUSE_IRQ_COMPLETION",
           "gain_before": original_gain, "gain_during": gain, "gain_after": original_gain,
           "gain_restored": True, "HP_encoded_before": [16, 8, 4, 2, 1, 0][original_gain],
           "HP_encoded_during": [16, 8, 4, 2, 1, 0][gain],
           "HP_encoded_after": [16, 8, 4, 2, 1, 0][original_gain], "HP_direct_stores": 0,
           "SYS74_before": 0x2000186, "SYS74_during": 0x2000184, "SYS74_after": 0x2000186,
           "DIR0_before": 0x198ece6, "DIR0_after": 0x198ece6,
           "OUT0_before": 2, "OUT0_after": 2,
           "mux_restoration_verified": True, "latch_restoration_verified": True,
           "mux_enter_writes": 1, "OUT0_restore_writes": 1, "mux_exit_writes": 1,
           "MMIO_write_count": 3}
    content = (row["frames_generated"] * 4 + 4095) // 4096
    row.update(bytes_submitted=max(4, content + 3) * 4096,
               refill_periods=max(0, content - 1), periods_completed=content)
    for suffix in ("before", "during", "after"):
        row["HP9c_" + suffix] = row["HP_encoded_" + suffix] << 18
    if duration == 0:
        keep = ("record state phase frequency_hz gain_test volume_percent duration_ms digital_peak calculated_DAC_rate "
                "frequency_max_hz channels bits kernel_ring_bytes kernel_ring_backing_bytes scratch_bytes "
                "DMA_stop_evidence physical_heard UART_disconnection_admission").split()
        row = {k: v if k in keep else False if type(v) is bool else 0 for k, v in row.items()}
        row.update(state="TONE_NOOP_ZERO_DURATION", phase=1, DMA_stop_evidence="NONE")
    return row


def capabilities():
    return {"record": "peripheral-capabilities-v7", "state": "CAPABILITIES", "frequency_min_hz": 1,
            "frequency_max_hz": None, "frequency_limit_rule": "floor((calculated_DAC_rate-1)/2)",
            "gain_min": 0, "gain_max": 5, "volume_min": 0, "volume_max": 100,
            "duration_min_ms": 0, "duration_max_ms": 5000,
            "generator": "camera_software_triangle_hardware_DAC", "ambient_one_shot": True,
            "ambient_irq_off_wait_unbounded": True}


class Child:
    def __init__(self, returncode=0):
        self.returncode = returncode

    def poll(self):
        return self.returncode

    def kill(self):
        raise AssertionError("An active SSH client must never be killed")

    terminate = kill

    def wait(self, *args, **kwargs):
        raise AssertionError("No timed subprocess wait")


class SSHTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.base = Path(self.tmp.name)
        self.private = self.base / "private"
        self.private.mkdir(mode=0o700)
        self.config = self.private / "config"
        self.config.write_text("TEST FIXTURE ONLY - contents must never be read")
        self.config.chmod(0o600)
        self.diag = self.private / "diagnostics"
        self.diag.mkdir(mode=0o700)
        self.profile = self.base / "profile.json"
        self.data = {"ssh_config": str(self.config), "diagnostics_dir": str(self.diag),
                     "expires_utc": (dt.datetime.now(dt.timezone.utc) + dt.timedelta(minutes=10)).isoformat(),
                      "uart_signals_disconnected": True, "expected_runtime": "3.4.35", "host_alias": "camera-bench"}
        self.data.update(mode="persistent-ssh-trial", expected_pid1="init", admission="ADMITTED",
                         startup_policy="minimal-no-app-cloud-record-homing", source_epoch_reviewed=True,
                         startup_reviewed=True, recovery_reviewed=True, owners_reviewed=True,
                         generation="11111111-1111-1111-1111-111111111111",
                         boot_id="22222222-2222-2222-2222-222222222222")
        self.save_profile()
        self.manifest = self.base / "manifest.json"
        self.manifest.write_text(json.dumps({"path": CONSOLE, "sha256": "a" * 64, "bytes": 1234, "build": "MOCK_ONLY"}))
        self.calls = []
        self.row = status()
        self.raw = None
        self.child = Child()
        self.now = 0
        self.patch_root = patch("ssh_backend.ROOT", self.base / "mock-repository")
        self.patch_root.start()
        self.b = SSHBench(self.profile, popen=self.spawn, clock=lambda: self.now,
                          sleep=self.advance, operation_lock=self.base / "operation.lock", manifest=self.manifest)

    def save_profile(self):
        self.profile.write_text(json.dumps(self.data))
        self.profile.chmod(0o600)

    def advance(self, amount):
        self.now += amount

    def spawn(self, argv, **kwargs):
        self.calls.append((argv, kwargs))
        for name in ("stdout", "stderr"):
            info = os.fstat(kwargs[name].fileno())
            self.assertTrue(stat.S_ISREG(info.st_mode))
            self.assertEqual(stat.S_IMODE(info.st_mode), 0o600)
            self.assertEqual(info.st_uid, os.getuid())
        marker = re.search(r"GUI_SSH_EXIT:[a-f0-9-]+:[a-f0-9]+:[a-f0-9-]+:", argv[-1]).group()
        text = self.raw if self.raw is not None else json.dumps(self.row) + "\n" + marker + "0\n"
        kwargs["stdout"].write(text.encode())
        kwargs["stdout"].flush()
        kwargs["stderr"].write(b"PRIVATE fingerprint/private key detail, never GUI output")
        kwargs["stderr"].flush()
        return self.child

    def tearDown(self):
        self.release_mock_pending()
        self.patch_root.stop()
        self.tmp.cleanup()

    def release_mock_pending(self):
        # Mock child only: fixture cleanup is not production client recovery.
        if self.b.pending:
            _, streams, lease = self.b.pending
            for stream in streams:
                stream.close()
            lease.close()
            self.b.pending = None
            # Fixture-only simulated independent recovery. Production has no
            # reset/clear-HOLD API and must never infer it from client liveness.
            self.b.operation_lock.write_text("")

    def connect(self):
        self.row = status()
        self.b.status()

    def test_status_is_read_only_and_no_uart_or_private_config_read(self):
        original = Path.read_text
        def read(path, *args, **kwargs):
            if path == self.config:
                self.fail("Private SSH config must not be read")
            return original(path, *args, **kwargs)
        with patch.object(Path, "read_text", read), patch("backend.rpc", side_effect=AssertionError("UART")), \
                patch("subprocess.run", side_effect=AssertionError("Unexpected runner")):
            result = json.loads(self.b.status())
        self.assertEqual(result, status())
        argv, kwargs = self.calls[-1]
        self.assertEqual(argv[0:3], ["/usr/bin/ssh", "-T", "-F"])
        for option in ("StrictHostKeyChecking=yes", "IdentityAgent=none", "ProxyCommand=none", "ProxyJump=none",
                       "ForwardAgent=no", "ClearAllForwardings=yes", "ControlPath=none", "ControlMaster=no", "BatchMode=yes",
                       "RequestTTY=no", "SessionType=default", "RemoteCommand=none", "PermitLocalCommand=no"):
            self.assertIn(option, argv)
        command = argv[-1]
        self.assertIn(CONSOLE + " --status", command)
        self.assertNotIn("exec 3>/dev/watchdog", command)
        for forbidden in ("--ambient", "--tone", "SET", "uart.sock", "insmod", "stty"):
            self.assertNotIn(forbidden, command)
        self.assertEqual(kwargs["stdin"], subprocess.DEVNULL)
        self.assertNotEqual(kwargs["stdout"], subprocess.PIPE)
        self.assertNotIn("SSH_AUTH_SOCK", kwargs["env"])

    def test_all_valid_boundaries_and_camera_only_arguments(self):
        self.connect()
        for parameters in ((1, 0, 0, 1), (4005, 5, 100, 5000), (1000, 3, 25, 250)):
            self.row = tone(parameters)
            f, g, v, ms = parameters
            result = json.loads(self.b.tone(str(f), g, str(v), str(ms / 1000), True))
            self.assertEqual(result["digital_peak"], (32767 * v + 50) // 100)
            command = self.calls[-1][0][-1]
            self.assertIn(f"{CONSOLE} --tone {f} {g} {v} {ms}", command)
            self.assertIn("trap", command)
            self.assertIn("1 2 13 15", command)
            self.assertIn("exec 3>&-", command)
            self.assertIn("0|1|2)", command)
            self.assertIn("kill -STOP $$", command)
            for prerequisite in ("3.4.35", "id -u", "fuser", "/proc/modules", "ak39_top_wdt", "otg_hs", "hgics", "sha256sum", "wc -c"):
                self.assertIn(prerequisite, command)
            self.assertNotIn(".wav", command)
            self.assertNotIn("base64", command)
            parsed = subprocess.run(["/bin/sh", "-n", "-c", command], capture_output=True)
            self.assertEqual(parsed.returncode, 0, parsed.stderr)
        self.assertEqual(tone_parameters("1000", 3, "25", "0.25", 8012), (1000, 3, 25, 250))

    def test_invalid_inputs_refused_before_ssh(self):
        self.connect()
        count = len(self.calls)
        for args in (("0", 3, "25", "0.25"), ("4006", 3, "25", "0.25"),
                     ("1000;id", 3, "25", "0.25"), (True, 3, "25", "0.25"),
                     ("1000", 6, "25", "0.25"), ("1000", True, "25", "0.25"),
                     ("1000", 3, "101", "0.25"), ("1000", 3, "-1", "0.25")):
            with self.subTest(args=args), self.assertRaises(BenchError):
                self.b.tone(*args, acknowledge=True)
        for seconds in ("NaN", "Infinity", "1e-3", "-0.1", "5.001", "0.0001", "1.2345", "1.0000", " 1", "", "1;id"):
            with self.subTest(seconds=seconds), self.assertRaises(BenchError):
                self.b.tone("1000", 3, "25", seconds, True)
        with self.assertRaises(BenchError):
            self.b.tone("1000", 3, "25", "0.25", False)
        with self.assertRaises(BenchError):
            self.b.request("tone", (1, 0, 0, 5001), acknowledge=True)
        with self.assertRaises(BenchError):
            self.b.request("ambient", ("anything",))
        self.assertEqual(len(self.calls), count)
        self.assertFalse(self.b.fault)

    def test_zero_noop_only_zero_effects_no_played_claim(self):
        self.connect()
        self.row = tone((4005, 5, 100, 0))
        result = json.loads(self.b.tone("4005", 5, "100", "0", True))
        self.assertEqual(result["effects"], 0)
        self.assertNotIn("physical_heard", result)
        for change in ({"effects": 1}, {"effects": False}, {"frames_generated": 1}, {"state": "TONE_COMPLETED_AWAIT_USER"}):
            with self.assertRaises(BenchError):
                validate_tone(dict(self.row, **change), (4005, 5, 100, 0), 8012)

    def test_restore_original_gain_not_assumed_code1_and_full_banks(self):
        for gain in range(6):
            validate_tone(tone(original_gain=gain), (1000, 3, 25, 250), 8012)
        row = tone()
        for field, value in (("gain_after", 3), ("HP_encoded_before", 2), ("HP9c_before", 2 << 18),
                             ("SYS74_after", 0x2000106), ("SYS74_during", 0x2000180),
                             ("DIR0_after", 0x198ece4), ("OUT0_after", 6), ("OUT0_during", 4),
                             ("MMIO_write_count", True), ("MMIO_write_count", 1),
                             ("HP_direct_stores", 1), ("pause_completed", False), ("close_ok", False),
                             ("close_uncertain", True), ("gain_restored", False),
                             ("mux_restoration_verified", False), ("latch_restoration_verified", False),
                             ("physical_heard", "CONFIRMED"), ("digital_peak", 768), ("frames_generated", 1792)):
            with self.subTest(field=field), self.assertRaises(BenchError):
                validate_tone(dict(row, **{field: value}), (1000, 3, 25, 250), 8012)

    def test_ambient_manual_single_command_scaled_and_no_status_poll(self):
        self.row = {"record": "stock-ambient-control-v1", "state": "READ_COMPLETED", "value": 2999,
                    "units": "stock_adc_scaled_uncalibrated", "higher_is_darker": True, "close_ok": True}
        for value in (0, 2999):
            self.row["value"] = value
            self.assertEqual(json.loads(self.b.ambient())["value"], value)
        self.assertEqual(len(self.calls), 2)
        for argv, _ in self.calls:
            self.assertIn(CONSOLE + " --ambient", argv[-1])
            self.assertNotIn("--tone", argv[-1])
            self.assertNotIn("--status", argv[-1])
        for value in (-1, 3000, True):
            with self.assertRaises(BenchError):
                self.b.validate("ambient", dict(self.row, value=value), ())
        with self.assertRaises(BenchError):
            self.b.validate("ambient", dict(self.row, close_ok=False), ())

    def test_profile_expiry_modes_owner_and_no_contents_disclosure(self):
        fixed = dt.datetime.now(dt.timezone.utc)
        self.data["expires_utc"] = (fixed + dt.timedelta(seconds=75)).isoformat()
        self.save_profile()
        self.assertEqual(load_profile(self.profile, fixed)["host_alias"], "camera-bench")
        for delta in (74.9, 0, -1):
            self.data["expires_utc"] = (fixed + dt.timedelta(seconds=delta)).isoformat()
            self.save_profile()
            with self.assertRaises(BenchError):
                load_profile(self.profile, fixed)
        self.data["expires_utc"] = (fixed + dt.timedelta(minutes=5)).isoformat()
        self.save_profile()
        for path, wrong, right in ((self.profile, 0o644, 0o600), (self.config, 0o644, 0o600),
                                   (self.private, 0o755, 0o700), (self.diag, 0o755, 0o700)):
            path.chmod(wrong)
            with self.assertRaises(BenchError):
                load_profile(self.profile, fixed)
            path.chmod(right)
        with patch("ssh_backend.os.getuid", return_value=os.getuid() + 1), self.assertRaises(BenchError):
            load_profile(self.profile, fixed)
        link = self.private / "config-link"
        link.symlink_to(self.config)
        self.data["ssh_config"] = str(link)
        self.save_profile()
        with self.assertRaises(BenchError):
            load_profile(self.profile, fixed)

    def test_preparation_missing_manifest_and_profile_is_fail_closed(self):
        self.manifest.unlink()
        with self.assertRaises(BenchError):
            self.b.status()
        self.assertEqual(self.calls, [])
        self.assertTrue(self.b.fault)
        with self.assertRaises(BenchError):
            self.b.status()

    def test_epoch_expiry_rechecked_before_each_effect_no_ssh_after_expiry(self):
        self.connect()
        self.data["expires_utc"] = (dt.datetime.now(dt.timezone.utc) + dt.timedelta(seconds=74)).isoformat()
        self.save_profile()
        with self.assertRaises(BenchError):
            self.b.ambient()
        self.assertEqual(len(self.calls), 1)
        self.assertTrue(self.b.fault)

    def test_safe_error_exit_and_missing_duplicate_markers_never_claim_success(self):
        for body, code in ((json.dumps(status()) + "\n", 0),
                           (json.dumps(status()) + "\nGUI_SSH_EXIT:0\nGUI_SSH_EXIT:0\n", 0),
                           (json.dumps(status()) + "\nGUI_SSH_EXIT:1\n", 1),
                           (json.dumps(status()) + "\nGUI_SSH_EXIT:2\n", 2),
                           (json.dumps(status()) + "\nGUI_SSH_EXIT:3\n", 3)):
            self.release_mock_pending()
            self.b.fault = False  # independent mock cases, never a production reset API
            self.child = Child(code)
            self.raw = body
            with self.subTest(code=code), self.assertRaises(BenchError):
                self.b.status()
            self.assertTrue(self.b.fault)

    def test_capabilities_is_inert_and_never_opens_watchdog_or_pcm(self):
        self.row = capabilities()
        self.assertEqual(json.loads(self.b.capabilities()), capabilities())
        command = self.calls[-1][0][-1]
        self.assertIn(CONSOLE + " --capabilities", command)
        self.assertNotIn("exec 3>/dev/watchdog", command)
        self.assertNotIn("--ambient", command)

    def test_timeout_retains_client_streams_and_lock_beyond75s_sticky(self):
        self.b.rate = 8012
        self.child = Child(None)
        with self.assertRaisesRegex(BenchError, "client and operation lock retained"):
            self.b.tone("4005", 5, "100", "5", True)
        self.assertGreaterEqual(self.now, 13)
        self.assertLess(self.now, 13.1)
        child, streams, lease = self.b.pending
        self.advance(75)
        self.assertIsNone(child.poll())
        self.assertTrue(all(not stream.closed for stream in streams))
        self.assertFalse(lease.closed)
        with (self.base / "operation.lock").open("a") as other:
            import fcntl
            with self.assertRaises(BlockingIOError):
                fcntl.flock(other, fcntl.LOCK_EX | fcntl.LOCK_NB)
        for operation in (self.b.status, self.b.ambient, lambda: self.b.audio_test("mic")):
            with self.assertRaisesRegex(BenchError, "Sticky fault"):
                operation()
        self.assertEqual(len(self.calls), 1)

    def test_fault_sticky_no_refresh_clears_and_stderr_private(self):
        self.raw = "PRIVATE fingerprint error, not JSON\nGUI_SSH_EXIT:0\n"
        with self.assertRaises(BenchError) as error:
            self.b.status()
        self.assertNotIn("fingerprint", str(error.exception))
        with self.assertRaisesRegex(BenchError, "Sticky fault"):
            self.b.status()
        self.assertEqual(len(self.calls), 1)
        self.assertEqual(len(list(self.diag.iterdir())), 2)
        self.assertTrue(all(p.stat().st_mode & 0o777 == 0o600 for p in self.diag.iterdir()))

    def test_waiting_request_rechecks_sticky_fault_after_serialization(self):
        bench = self.b
        class FaultingLock:
            def __enter__(self):
                bench.fault = True  # previous serialized operation just failed

            def __exit__(self, *args):
                pass
        self.b.lock = FaultingLock()
        with self.assertRaisesRegex(BenchError, "Sticky fault"):
            self.b.status()
        self.assertEqual(self.calls, [])

    def test_scalar_only_mic_manifest_reuse_and_projection(self):
        self.row = {"record": "mic-level-v3", "samples": 8000, "raw_audio_output": False,
                    "audio_hash_output": False, "close_ok": True, "rms": 32, "half_ac_rms": [25, 26],
                    "clipped": 0, "unexpected_private_audio": "never surface",
                    "state": "CAPTURE_COMPLETED_NEEDS_USER_SOUND_CORRELATION", "stop_return_zero": True,
                    "DMA_state_zero": True, "playback_opened": False}
        result = json.loads(self.b.audio_test("mic"))
        self.assertNotIn("unexpected_private_audio", result)
        self.assertIn("/tmp/stock-mic-level3-armv5 --native-stock-mic-level1s", self.calls[-1][0][-1])
        self.assertNotIn("--ambient", self.calls[-1][0][-1])
        for changed in ({"raw_audio_output": True}, {"audio_hash_output": True}, {"close_ok": False},
                        {"stop_return_zero": False}, {"DMA_state_zero": False}, {"playback_opened": True},
                        {"half_ac_rms": ["voice", 0]}):
            with self.assertRaises(BenchError):
                self.b.validate("mic", dict(self.row, **changed), ())
        for kind in ("speaker", "other"):
            with self.assertRaises(BenchError):
                self.b.audio_test(kind)


if __name__ == "__main__":
    unittest.main()
