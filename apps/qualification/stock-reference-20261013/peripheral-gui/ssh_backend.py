"""Fixed SSH requests for the parent-reviewed, short-lived stock audio7 epoch.

No UART fallback, config/key reads, waveform generation or automatic recovery.
Only validated scalar receipts leave the private SSH diagnostics directory.
"""
import datetime as dt
from decimal import Decimal, InvalidOperation
import fcntl
import json
import os
from pathlib import Path
import re
import shlex
import stat
import subprocess
import threading
import time
import uuid

from backend import BenchError, ROOT, AUDIO_PATHS

CONSOLE = "/tmp/qc-peripheral-audio7-armv5"
DEFAULT_PROFILE = ROOT / "work/qc-uart-live/ssh-session.json"
OPERATION_LOCK = ROOT / "work/qc-uart-live/peripheral-gui-operation.lock"
FAULT = ("Unknown physical state/audibility. Switch bulb power OFF and leave OFF; "
         "ask parent for fresh recovery. No automatic retry/reset. "
         "ADC can hang forever with IRQs disabled; this is not a hard safety timeout.")
MODES = {"controlled-ram-shell": "sh", "persistent-ssh-trial": "init"}
STARTUP_POLICY = "minimal-no-app-cloud-record-homing"


def strict_json(text):
    def pairs(items):
        row = {}
        for key, value in items:
            if key in row:
                raise ValueError("Duplicate JSON field")
            row[key] = value
        return row

    def nonfinite(value):
        raise ValueError("Nonfinite JSON")

    return json.loads(text, object_pairs_hook=pairs, parse_constant=nonfinite)


def integer(value, low, high, name):
    if type(value) is int:
        number = value
    elif isinstance(value, str) and re.fullmatch(r"[0-9]{1,10}", value):
        number = int(value)
    else:
        raise BenchError(f"{name}: enter a decimal integer {low}..{high}; no command sent")
    if not low <= number <= high:
        raise BenchError(f"{name}: valid range {low}..{high}; no command sent")
    return number


def tone_parameters(frequency, gain, volume, seconds, rate):
    rate = integer(rate, 2, 1000000, "Current DAC rate (Read status first)")
    frequency = integer(frequency, 1, (rate - 1) // 2, "Frequency Hz")
    gain = integer(gain, 0, 5, "Analog gain code")
    volume = integer(volume, 0, 100, "Digital volume %")
    # Decimal avoids binary rounding, NaN, exponent forms and silent truncation.
    if not isinstance(seconds, str) or not re.fullmatch(r"[0-9]{1,2}(?:\.[0-9]{1,3})?", seconds):
        raise BenchError("Duration: enter 0..5 seconds, at most 3 decimal places; no command sent")
    try:
        duration = Decimal(seconds) * 1000
    except InvalidOperation:
        raise BenchError("Invalid duration; no command sent") from None
    if not duration.is_finite() or not 0 <= duration <= 5000 or duration != duration.to_integral_value():
        raise BenchError("Duration: enter 0..5 seconds to millisecond precision; no command sent")
    return frequency, gain, volume, int(duration)


def private_node(path, mode, directory=False):
    path = Path(path)
    info = path.lstat()
    kind = stat.S_ISDIR if directory else stat.S_ISREG
    if not kind(info.st_mode) or info.st_uid != os.getuid() or stat.S_IMODE(info.st_mode) != mode:
        raise BenchError("SSH profile/private path has unsafe owner, mode or file type")
    # Reject symlink ancestors and writable-by-others traversal paths as well.
    for parent in path.absolute().parents:
        p = parent.lstat()
        if not stat.S_ISDIR(p.st_mode) or p.st_mode & 0o022:
            # System /tmp is never accepted as a private diagnostics/config root.
            raise BenchError("SSH private path has an unsafe parent")
    return path


def load_profile(path, now=None):
    try:
        profile = strict_json(private_node(path, 0o600).read_text())
        if not isinstance(profile, dict):
            raise BenchError("Invalid SSH profile object")
        if profile.get("expected_runtime") != "3.4.35" or profile.get("host_alias") != "camera-bench":
            raise BenchError("SSH profile is not the controlled stock camera epoch")
        if (profile.get("mode") not in MODES or profile.get("expected_pid1") != MODES[profile["mode"]]
                or profile.get("startup_policy") != STARTUP_POLICY or profile.get("admission") != "ADMITTED"
                or any(profile.get(k) is not True for k in
                       ("source_epoch_reviewed", "startup_reviewed", "recovery_reviewed", "owners_reviewed"))):
            raise BenchError("Unknown/HOLD startup or source epoch; parent review required")
        for key in ("generation", "boot_id"):
            if not isinstance(profile.get(key), str) or not re.fullmatch(
                    r"[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}", profile[key]):
                raise BenchError("Missing controlled SSH generation/boot identity")
        if type(profile.get("uart_signals_disconnected")) is not bool:
            raise BenchError("SSH profile lacks the parent's UART isolation statement")
        end = dt.datetime.fromisoformat(profile["expires_utc"].replace("Z", "+00:00"))
        if end.tzinfo is None or (end - (now or dt.datetime.now(dt.timezone.utc))).total_seconds() < 75:
            raise BenchError("SSH epoch expired/less than 75 seconds left; parent must revalidate")
        for key, mode, directory in (("ssh_config", 0o600, False), ("diagnostics_dir", 0o700, True)):
            p = Path(profile[key])
            if not p.is_absolute() or p.is_relative_to(ROOT):
                raise BenchError("SSH config/diagnostics must be private outside the repository")
            private_node(p, mode, directory)
            private_node(p.parent, 0o700, True)
        # Config is stat'ed only. Its identity/known_hosts paths and contents stay private.
        return profile
    except BenchError:
        raise
    except (OSError, ValueError, KeyError, TypeError):
        raise BenchError("Missing/invalid private SSH profile; ask parent to prepare it") from None


def manifest_entry(path, expected):
    try:
        entry = strict_json(Path(path).read_text())
        if (not isinstance(entry, dict) or entry.get("path") != expected
                or not re.fullmatch(r"[0-9a-f]{64}", entry.get("sha256", ""))
                or type(entry.get("bytes")) is not int or entry["bytes"] <= 0
                or not isinstance(entry.get("build"), str) or not entry["build"]):
            raise ValueError()
        return entry
    except (OSError, ValueError, TypeError):
        raise BenchError("Reviewed/staged helper manifest missing; parent must prepare controls") from None


def expected(row, fields, exact=False):
    if ((exact and set(row) != set(fields))
            or any(type(row.get(k)) is not type(v) or row[k] != v for k, v in fields.items())):
        raise BenchError("Invalid native completion/restoration receipt. " + FAULT)


def uint(row, key, maximum=0xffffffff):
    value = row.get(key)
    if type(value) is not int or not 0 <= value <= maximum:
        raise BenchError("Invalid native scalar receipt. " + FAULT)
    return value


def validate_tone(row, parameters, rate):
    frequency, gain, volume, duration = parameters
    expected(row, {"record": "stock-tone-control-v7", "frequency_hz": frequency,
                   "gain_test": gain, "volume_percent": volume, "duration_ms": duration,
                   "digital_peak": (32767 * volume + 50) // 100})
    # Actual console.c record() schema: no invented during/order witnesses.
    keys = ("record state phase effects frequency_hz gain_test volume_percent duration_ms digital_peak "
            "frames_generated calculated_DAC_rate frequency_max_hz channels bits bytes_submitted kernel_ring_bytes "
            "kernel_ring_backing_bytes scratch_bytes periods_completed refill_periods stream_error pause_completed "
            "close_ok close_uncertain DMA_stop_evidence gain_before gain_during gain_after gain_restored gain_SET_attempts "
            "HP_encoded_before HP_encoded_during HP_encoded_after HP9c_before HP9c_during HP9c_after HP_direct_stores "
            "SYS74_before SYS74_during SYS74_after DIR0_before DIR0_after OUT0_before OUT0_after "
            "mux_restoration_verified latch_restoration_verified MMIO_write_count mux_enter_writes OUT0_restore_writes "
            "mux_exit_writes physical_heard bank_lease_verified_by_helper UART_disconnection_admission").split()
    if set(row) != set(keys):
        raise BenchError("Unexpected/missing console07 receipt fields. " + FAULT)
    expected(row, {"calculated_DAC_rate": rate, "frequency_max_hz": (rate - 1) // 2,
                   "channels": 2, "bits": 16, "kernel_ring_bytes": 49152,
                   "kernel_ring_backing_bytes": 65536, "scratch_bytes": 512,
                   "stream_error": 0, "HP_direct_stores": 0, "close_uncertain": False,
                   "physical_heard": "AWAIT_USER", "bank_lease_verified_by_helper": False,
                   "UART_disconnection_admission": "USER_UART_SIGNALS_DISCONNECTED"})
    if duration == 0:
        expected(row, {"state": "TONE_NOOP_ZERO_DURATION", "phase": 1, "DMA_stop_evidence": "NONE"})
        zero = ("effects frames_generated bytes_submitted periods_completed refill_periods gain_before gain_during gain_after "
                "gain_SET_attempts HP_encoded_before HP_encoded_during HP_encoded_after HP9c_before HP9c_during HP9c_after "
                "SYS74_before SYS74_during SYS74_after DIR0_before DIR0_after OUT0_before OUT0_after MMIO_write_count "
                "mux_enter_writes OUT0_restore_writes mux_exit_writes").split()
        expected(row, dict.fromkeys(zero, 0))
        expected(row, dict.fromkeys(("pause_completed", "close_ok", "gain_restored",
                                    "mux_restoration_verified", "latch_restoration_verified"), False))
        return
    frames = (rate * duration + 500) // 1000
    expected(row, {"state": "TONE_COMPLETED_AWAIT_USER", "physical_heard": "AWAIT_USER",
                   "phase": 10, "frames_generated": frames, "gain_SET_attempts": 2,
                   "pause_completed": True, "close_ok": True, "close_uncertain": False,
                   "DMA_stop_evidence": "SOURCE_CONDITIONAL_PAUSE_IRQ_COMPLETION",
                   "gain_during": gain, "gain_restored": True,
                   "HP_direct_stores": 0, "mux_restoration_verified": True,
                   "latch_restoration_verified": True})
    before = uint(row, "gain_before", 5)
    expected(row, {"gain_after": before, "HP_encoded_before": [16, 8, 4, 2, 1, 0][before],
                   "HP_encoded_during": [16, 8, 4, 2, 1, 0][gain],
                   "HP_encoded_after": [16, 8, 4, 2, 1, 0][before]})
    for suffix in ("before", "during", "after"):
        if (uint(row, "HP9c_" + suffix) & 0x007c0000) >> 18 != row["HP_encoded_" + suffix]:
            raise BenchError("HP register/cache field mapping mismatch. " + FAULT)
    for name in ("SYS74", "DIR0", "OUT0"):
        original = uint(row, name + "_before")
        expected(row, {name + "_after": original})
        if name == "SYS74":
            if original & 6 != 6:
                raise BenchError("Invalid UART mux entry predicate. " + FAULT)
            expected(row, {name + "_during": original & ~2})
        elif name == "DIR0":
            if original & 2 != 2:
                raise BenchError("Invalid GPIO1 direction entry predicate. " + FAULT)
        elif original != 2:
            raise BenchError("Invalid GPIO bank entry predicate. " + FAULT)
    enter, latch, leave = [uint(row, k, 1) for k in ("mux_enter_writes", "OUT0_restore_writes", "mux_exit_writes")]
    if enter != 1 or leave != 1:
        raise BenchError("Invalid mux restoration count. " + FAULT)
    expected(row, {"MMIO_write_count": enter + latch + leave, "effects": enter + latch + leave + 2})
    content = (frames * 4 + 4095) // 4096
    submitted = uint(row, "bytes_submitted")
    refills = uint(row, "refill_periods")
    completed = uint(row, "periods_completed")
    if (submitted != (4 + refills) * 4096 or submitted > max(4, content + 3) * 4096
            or submitted < frames * 4 or not content <= completed <= content + 1
            or completed + 1 >= submitted // 4096):
        raise BenchError("Invalid bounded producer/retirement accounting. " + FAULT)


class SSHBench:
    """One existing operation lock, one fixed SSH child per request; no broker."""
    def __init__(self, profile_path, *, popen=subprocess.Popen, clock=time.monotonic,
                 sleep=time.sleep, operation_lock=OPERATION_LOCK, manifest=None):
        self.profile_path = Path(profile_path)
        self.popen, self.clock, self.sleep = popen, clock, sleep
        self.operation_lock = Path(operation_lock)
        self.manifest = manifest or Path(__file__).with_name("console-helper.json")
        self.lock = threading.Lock()
        self.fault = False
        self.profile = None
        self.rate = None
        self.pulse_until = 0
        self.audio = {"mic": True}
        self.pending = None

    def uncertain_running(self):
        # Client liveness is not remote retirement. Even an exited/disconnected
        # client retains effect ownership until parent recovery/revalidation.
        return self.pending is not None

    @staticmethod
    def command(entry, arguments, effect, profile, tag):
        # Caller provides only fixed selectors and already-validated integers.
        binary = entry["path"]
        script = ("trap '' 1 2 13 15; "
                   'test "$(/bin/busybox id -u)" = 0 && '
                   'test "$(/bin/busybox cat /proc/sys/kernel/osrelease)" = 3.4.35 || exit 90; '
                   f'test "$(/bin/busybox cat /proc/sys/kernel/random/boot_id)" = {profile["boot_id"]} '
                   f'&& test "$(/bin/busybox cat /proc/1/comm)" = {profile["expected_pid1"]} '
                   '&& test "$(/bin/busybox readlink /proc/1/exe)" = /bin/busybox || exit 90; '
                   "/bin/busybox awk '($2==\"/\" || $2==\"/usr\") {"
                   "if ($3!=\"squashfs\" || $4 !~ /(^|,)ro(,|$)/) bad=1; seen[$2]=1} "
                   "$3==\"jffs2\" || $3==\"vfat\" || $3==\"exfat\" || $1 ~ /mmcblk/ {bad=1} "
                   "END {exit (bad || !seen[\"/\"] || !seen[\"/usr\"])}' /proc/mounts || exit 90; "
                  "test \"$(/bin/busybox awk '{print $1}' /proc/modules | /bin/busybox sort)\" = "
                  "\"$(printf 'ak39_top_wdt\\nhgics\\notg_hs')\" || exit 90; "
                  "/bin/busybox fuser /dev/watchdog /dev/akpcm_cdev0 /dev/akpcm_cdev1 "
                   "/dev/motor0 /dev/motor1 /dev/video0 /dev/isp_char /dev/ion /dev/uio0 /dev/mem /dev/akgpio >/dev/null 2>&1; "
                  'test "$?" = 1 || exit 90; '
                  f'test "$(/bin/busybox sha256sum {binary} | /bin/busybox cut -d \' \' -f 1)" = {entry["sha256"]} '
                  f'&& test "$(/bin/busybox wc -c < {binary})" -eq {entry["bytes"]} || exit 90; ')
        if effect:
            script += "exec 3>/dev/watchdog || exit 90; "
        script += f"(exec 3>&-; {binary} {' '.join(map(str, arguments))}); x=$?; "
        if effect:
            script += ('case "$x" in 0|1|2) echo . >&3; exec 3>&-;; '
                       '*) echo GUI_RETAINED; kill -STOP $$; while :; do /bin/busybox sleep 3600; done;; esac; ')
        script += (f'test "$(/bin/busybox cat /proc/sys/kernel/random/boot_id)" = {profile["boot_id"]} || exit 90; '
                   f'echo GUI_SSH_EXIT:{profile["generation"]}:{tag}:{profile["boot_id"]}:$x; exit "$x"')
        return "/bin/busybox sh -c " + shlex.quote(script)

    def request(self, action, parameters=(), duration=0, acknowledge=False):
        if self.fault:
            raise BenchError("Sticky fault: controls remain locked. " + FAULT)
        if action not in ("status", "capabilities", "tone", "ambient", "mic"):
            raise BenchError("SSH epoch has no prepared motor/LED controls")
        if action == "tone" and (acknowledge is not True or len(parameters) != 4):
            raise BenchError("Explicitly confirm UART signal wires are physically disconnected")
        if action == "tone":
            if self.rate is None:
                raise BenchError("Read status first")
            limits = ((1, (self.rate - 1) // 2), (0, 5), (0, 100), (0, 5000))
            if any(type(v) is not int or not low <= v <= high for v, (low, high) in zip(parameters, limits)):
                raise BenchError("Invalid tone parameters; no command sent")
            duration = parameters[3]
        elif parameters:
            raise BenchError("This fixed request accepts no arguments")
        with self.lock:
            # A waiting caller must not inherit admission from before another
            # serialized request faulted. There is deliberately no fault reset.
            if self.fault:
                raise BenchError("Sticky fault: controls remain locked. " + FAULT)
            lease = self.operation_lock.open("a+")
            streams = []
            child = None
            committed = False
            try:
                fcntl.flock(lease, fcntl.LOCK_EX | fcntl.LOCK_NB)
                lease.seek(0)
                if lease.read(1):
                    raise BenchError("Unretired operation HOLD in existing lock; parent recovery/revalidation required")
                profile = load_profile(self.profile_path)
                if self.profile is not None and profile != self.profile:
                    raise BenchError("SSH epoch/profile changed; restart only after parent review")
                self.profile = profile
                if action == "tone" and not profile["uart_signals_disconnected"]:
                    raise BenchError("Parent profile does not admit UART-disconnected tone")
                entry = manifest_entry(self.manifest, CONSOLE)
                arguments = ["--" + action, *parameters]
                if action == "mic":
                    audio = json.loads(Path(__file__).with_name("audio-helpers.json").read_text())
                    # Reuse the parent's old mic3 manifest, not a new audio transport.
                    entry = audio.get("mic", {})
                    if (entry.get("path") != AUDIO_PATHS["mic"] or
                            not re.fullmatch(r"[a-f0-9]{64}", entry.get("sha256", "")) or
                            type(entry.get("bytes")) is not int or entry["bytes"] <= 0):
                        raise BenchError("Reviewed/staged microphone helper missing")
                    arguments = ["--native-stock-mic-level1s"]
                tag = uuid.uuid4().hex
                for suffix in ("stdout", "stderr"):
                    fd = os.open(Path(profile["diagnostics_dir"]) / f"{tag}.{suffix}",
                                 os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
                    streams.append(os.fdopen(fd, "wb"))
                options = ["BatchMode=yes", "IdentitiesOnly=yes", "IdentityAgent=none",
                           "PreferredAuthentications=publickey", "PasswordAuthentication=no",
                           "KbdInteractiveAuthentication=no", "StrictHostKeyChecking=yes",
                           "GlobalKnownHostsFile=/dev/null", "UpdateHostKeys=no", "AddKeysToAgent=no",
                           "ForwardAgent=no", "ForwardX11=no", "ClearAllForwardings=yes",
                           "ProxyCommand=none", "ProxyJump=none", "ControlMaster=no",
                           "ControlPath=none", "ControlPersist=no", "PermitLocalCommand=no",
                            "RequestTTY=no", "SessionType=default", "RemoteCommand=none",
                            "Tunnel=no", "ServerAliveInterval=0", "TCPKeepAlive=no",
                           "ConnectionAttempts=1", "ConnectTimeout=5"]
                argv = ["/usr/bin/ssh", "-T", "-F", profile["ssh_config"]]
                for option in options:
                    argv += ["-o", option]
                argv += ["camera-bench", self.command(entry, arguments, action in ("tone", "ambient", "mic"), profile, tag)]
                # A small HOLD record in the EXISTING lock, not a new broker.
                # Crash/restart must not mistake an abandoned unknown epoch for
                # retirement. Only valid same-request completion clears it here;
                # parent owns independent recovery/revalidation of stale HOLDs.
                committed = True
                lease.write(json.dumps({"state": "HOLD", "generation": profile["generation"],
                                        "boot_id": profile["boot_id"], "request": tag, "action": action}) + "\n")
                lease.flush()
                os.fsync(lease.fileno())
                child = self.popen(argv, stdin=subprocess.DEVNULL, stdout=streams[0], stderr=streams[1],
                                   close_fds=True, env={"PATH": "/usr/bin:/bin", "HOME": str(Path.home()), "LC_ALL": "C"})
                deadline = self.clock() + max(8, duration / 1000 + 8)
                while child.poll() is None and self.clock() < deadline:
                    self.sleep(.05)
                if child.poll() is None:
                    # Never terminate/kill/wait(timeout), close pipes, drop lease, or retry.
                    self.pending = (child, streams, lease)
                    streams = []
                    lease = None
                    raise BenchError("Completion alert; SSH client and operation lock retained. " + FAULT)
                with (Path(profile["diagnostics_dir"]) / f"{tag}.stdout").open() as received:
                    output = received.read(65537)
                lines = output.splitlines()
                marker = f'GUI_SSH_EXIT:{profile["generation"]}:{tag}:{profile["boot_id"]}:{child.returncode}'
                if len(output) > 65536 or child.returncode not in (0, 1, 2) or lines[-1:] != [marker]:
                    raise BenchError("Missing valid SSH completion marker. " + FAULT)
                rows = [strict_json(line) for line in lines[:-1]]
                if len(rows) != 1 or not isinstance(rows[0], dict) or child.returncode != 0:
                    raise BenchError("Helper refused request/no verified completion. " + FAULT)
                row = rows[0]
                self.validate(action, row, parameters)
                # Only fields actually checked above reach routine logs/UI. An
                # unexpected stdout field must not become an audio/privacy leak.
                fields = {
                    "status": "record state calculated_DAC_rate frequency_max_hz HP_gain_code SYS74 SYS78 DIR0 OUT0 ambient_path_present",
                    "capabilities": "record state frequency_min_hz frequency_max_hz frequency_limit_rule gain_min gain_max volume_min volume_max duration_min_ms duration_max_ms generator ambient_one_shot ambient_irq_off_wait_unbounded",
                    "ambient": "record state value units higher_is_darker close_ok",
                    "mic": "record samples rms clipped half_ac_rms raw_audio_output audio_hash_output close_ok",
                    "tone": "record state frequency_hz gain_test volume_percent duration_ms digital_peak frames_generated effects physical_heard calculated_DAC_rate pause_completed close_ok close_uncertain DMA_stop_evidence gain_before gain_during gain_after gain_restored HP_encoded_before HP_encoded_during HP_encoded_after HP9c_before HP9c_during HP9c_after HP_direct_stores SYS74_before SYS74_during SYS74_after DIR0_before DIR0_after OUT0_before OUT0_after mux_restoration_verified latch_restoration_verified mux_enter_writes OUT0_restore_writes mux_exit_writes MMIO_write_count stream_error gain_SET_attempts bytes_submitted periods_completed refill_periods",
                }[action].split()
                if action == "tone" and parameters[3] == 0:
                    fields = "record state frequency_hz gain_test volume_percent duration_ms digital_peak frames_generated effects".split()
                result = json.dumps({k: row[k] for k in fields if k in row}, separators=(",", ":"))
                lease.seek(0)
                lease.truncate()
                lease.flush()
                os.fsync(lease.fileno())
                return result
            except Exception as exc:
                self.fault = True
                if committed and self.pending is None:
                    # Exit255, malformed receipts and disconnects are uncertain
                    # even when poll() says dead. Do not close streams/drop lease.
                    self.pending = (child, streams, lease)
                    streams = []
                    lease = None
                if isinstance(exc, BenchError):
                    raise
                # Never surface stderr/config paths, key fingerprints or raw SSH errors.
                raise BenchError("SSH preparation/receipt failure; private diagnostics retained. " + FAULT) from None
            finally:
                for stream in streams:
                    stream.close()
                if lease is not None:
                    lease.close()

    def validate(self, action, row, parameters):
        if action == "status":
            expected(row, {"record": "peripheral-status-v7", "state": "STATUS"})
            if set(row) != set("record state calculated_DAC_rate frequency_max_hz HP_gain_code SYS74 SYS78 DIR0 OUT0 ambient_path_present".split()):
                raise BenchError("Unexpected/missing console07 status fields")
            rate = uint(row, "calculated_DAC_rate", 1000000)
            if rate < 2:
                raise BenchError("Invalid calculated DAC rate")
            expected(row, {"frequency_max_hz": (rate - 1) // 2})
            uint(row, "HP_gain_code", 5)
            for key in ("SYS74", "SYS78", "DIR0", "OUT0"):
                uint(row, key)
            if type(row.get("ambient_path_present")) is not bool:
                raise BenchError("Invalid ambient presence status")
            self.rate = rate
        elif action == "capabilities":
            expected(row, {"record": "peripheral-capabilities-v7", "state": "CAPABILITIES",
                           "frequency_min_hz": 1, "frequency_max_hz": None,
                           "frequency_limit_rule": "floor((calculated_DAC_rate-1)/2)",
                           "gain_min": 0, "gain_max": 5, "volume_min": 0, "volume_max": 100,
                           "duration_min_ms": 0, "duration_max_ms": 5000,
                           "generator": "camera_software_triangle_hardware_DAC",
                           "ambient_one_shot": True, "ambient_irq_off_wait_unbounded": True}, exact=True)
        elif action == "ambient":
            expected(row, {"record": "stock-ambient-control-v1", "state": "READ_COMPLETED",
                           "units": "stock_adc_scaled_uncalibrated", "higher_is_darker": True, "close_ok": True})
            if set(row) != set("record state value units higher_is_darker close_ok".split()):
                raise BenchError("Unexpected/missing console07 ambient fields")
            uint(row, "value", 2999)
        elif action == "tone":
            validate_tone(row, parameters, self.rate)
        elif action == "mic":
            expected(row, {"record": "mic-level-v3", "samples": 8000, "raw_audio_output": False,
                           "audio_hash_output": False, "close_ok": True,
                           "state": "CAPTURE_COMPLETED_NEEDS_USER_SOUND_CORRELATION",
                           "stop_return_zero": True, "DMA_state_zero": True, "playback_opened": False})
            for key in ("rms", "clipped"):
                uint(row, key, 32768 if key == "rms" else 8000)
            half = row.get("half_ac_rms")
            if not isinstance(half, list) or len(half) != 2 or any(type(v) is not int or not 0 <= v <= 32768 for v in half):
                raise BenchError("Invalid scalar microphone activity receipt")

    def status(self):
        return self.request("status")

    def capabilities(self):
        return self.request("capabilities")

    def tone(self, frequency, gain, volume, seconds, acknowledge=False):
        if self.rate is None:
            raise BenchError("Read status first to establish the actual DAC Nyquist limit")
        parameters = tone_parameters(frequency, gain, volume, seconds, self.rate)
        return self.request("tone", parameters, parameters[3], acknowledge)

    def ambient(self):
        return self.request("ambient")

    def audio_test(self, kind):
        if kind != "mic":
            raise BenchError("Legacy speaker helper is unavailable in this SSH epoch")
        return self.request("mic", duration=1000)
