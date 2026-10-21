#!/usr/bin/env python3
"""Host-only final re-review; never uses target/private input capabilities."""
import datetime
import hashlib
import inspect
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
APP = ROOT / "apps/qualification/kernel76-transport"
NEW = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    sources = list(sorted(APP.glob("*.py"))) + [ROOT / p for p in (
        "scripts/qc07f_boot.py", "scripts/qc07f_private_root_transfer.py",
        "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/postcheck.py")]
    sources += [NEW / "artifacts/native-usb76-source01/linux/drivers/usb/musb" / p
                for p in ("musb_core.c", "musb_debug.h", "musb_host.c")]
    pins = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in sources}
    prior_paths = [OUT.parent / "METADATA_RECEIPT_REVIEW.md", OUT.parent / "TRIAL_CORRECTION_REVIEW.md"]
    prior_paths += [p for folder in ("metadata-review", "trial-review")
                    for p in sorted((OUT.parent / folder).iterdir()) if p.is_file()]
    prior = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in prior_paths}
    tests = []
    for name in ("test_metadata21a.py", "test_radio_receipt.py", "test_trial21a.py"):
        argv = [sys.executable, "-B", str(APP / name), "-v"]
        begin = datetime.datetime.now(datetime.timezone.utc).isoformat()
        p = subprocess.run(argv, cwd=ROOT, env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"),
                           capture_output=True, text=True, timeout=60)
        log = OUT / (name + ".log")
        with log.open("x") as f:
            f.write("argv: " + json.dumps(argv) + "\ncwd: " + str(ROOT) + "\nstart_utc: " + begin)
            f.write("\nstdout:\n" + p.stdout + "\nstderr:\n" + p.stderr + "\nexit: " + str(p.returncode) + "\n")
        tests.append({"argv": argv, "start_utc": begin, "exit": p.returncode,
                      "log": log.name, "log_sha256": sha(log.read_bytes())})
    assert all(t["exit"] == 0 for t in tests)

    sys.path.insert(0, str(APP))
    import test_trial21a as host
    import radio_receipt as receipt
    import test_radio_receipt as samples
    warning_cases = {
        "core_host_resume": b"[ 1.000000] musb_handle_intr_resume 692: bogus host RESUME (a_host)\r\n",
        "core_disconnect": b"[ 1.000000] musb_handle_intr_disconnect 980: unhandled DISCONNECT transition (a_wait_bcon)\r\n",
        "core_babble": b"[ 1.000000] musb-hdrc musb-hdrc.0.auto: Babble\r\n",
    }
    # Inject only into the reviewed Fake's owned-shell-exit reply. Do not modify
    # actual main/helpers/diagnostic predicate or execute any private handoff.
    source = inspect.getsource(host.simulate)
    exit_line = '                answer = b"\\r\\nRAM-only shell returned to monitor\\r\\nqc07f> "'
    owner_line = "            return value\n        def check_stream"
    assert source.count(exit_line) == source.count(owner_line) == 1
    source = source.replace(exit_line, exit_line + "\n                if fault in REVIEW_WARNINGS: answer = REVIEW_WARNINGS[fault] + answer")
    source = source.replace(owner_line, '''            if self.owner_calls == 2:
                if fault == 'peer_uid_refresh': self.peer = (123, 1001, 1000)
                if fault == 'busy_refresh': value['tx_busy'] = True
                if fault == 'part_refresh': value['part'] = 'foreign'
            return value
        def check_stream''')
    namespace = dict(vars(host), REVIEW_WARNINGS=warning_cases)
    exec(compile(source, "<final-review-Fake-only-injections>", "exec"), namespace)
    simulate = namespace["simulate"]
    names = [None, "changed_owner", "lost_catch", "peer_uid_refresh", "busy_refresh", "part_refresh",
             "source_warning_before", "recognized_warning_before", "broken_before",
             "warning_after_load", "warning_after_state", *warning_cases]
    probes = {}
    for name in names:
        result = simulate(name)
        result["radio_commands"] = [c for c in result.pop("calls") if c.startswith("radio ")]
        probes[name or "healthy"] = result
    assert probes["healthy"]["PASS"] and probes["healthy"]["load_count"] == 1
    for name in ("changed_owner", "lost_catch", "peer_uid_refresh", "busy_refresh", "part_refresh",
                 "source_warning_before", "recognized_warning_before", "broken_before"):
        assert not probes[name]["PASS"] and probes[name]["HOLD"] and not probes[name]["radio_commands"]
    assert probes["warning_after_load"]["radio_commands"] == ["radio native-load"]
    assert probes["warning_after_state"]["radio_commands"] == ["radio native-load", "radio state"]
    for name in warning_cases:
        assert host.t.HCD_DIAGNOSTIC.search(warning_cases[name]) is None
        assert probes[name]["PASS"] and probes[name]["load_count"] == 1 and not probes[name]["HOLD"]

    manifest = json.loads((NEW / "analysis/native-usb76-package01/package76-manifest.json").read_text())
    vmlinux_path = NEW / "analysis/native-usb76-package01/build/vmlinux"
    vmlinux = vmlinux_path.read_bytes()
    assert sha(vmlinux) == manifest["output_sha256"][str(vmlinux_path.relative_to(ROOT))]
    compiled = {"bogus_RESUME": b"bogus %s RESUME (%s)\n\0" in vmlinux,
                "unhandled_DISCONNECT": b"unhandled DISCONNECT transition (%s)\n\0" in vmlinux,
                "Babble": b"Babble\n\0" in vmlinux}
    assert all(compiled.values())
    config_path = NEW / "analysis/native-usb76-package01/build/.config"
    config = config_path.read_text().splitlines()
    assert "CONFIG_USB_MUSB_HOST=y" in config and "# CONFIG_PM is not set" in config

    fixture = samples.loader(); ack = samples.ack(2, 102)
    mutants = {"historical_28hex_truncation": fixture.replace(ack, ack[:-4], 1),
               "missing_ACK": fixture.replace(ack + "\n", "", 1),
               "negative_length": fixture.replace("len0x00000010", "len0xffffffff", 1),
               "short_length": fixture.replace("len0x00000010", "len0x0000000f", 1),
               "status_nonzero": fixture.replace("1a2b0700100064000000", "1a2b0700100064000001", 1)}
    strict = {}
    for name, text in mutants.items():
        assert text != fixture
        try: receipt.parse_loader(text)
        except AssertionError: strict[name] = "REJECTED"
        else: raise AssertionError("unexpected ACK acceptance")
    assert pins == {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in sources}
    assert prior == {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in prior_paths}
    record = {
        "start_utc": started, "end_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "python": sys.version, "collector_sha256": sha(Path(__file__).read_bytes()),
        "source_sha256": pins, "prior_reports_and_failures_sha256": prior,
        "prior_reports_and_failures_unchanged": True, "tests": tests,
        "full_main_in_memory_probes": probes,
        "diagnostics_present_in_manifest_pinned_linked_kernel": compiled,
        "linked_kernel_sha256": sha(vmlinux), "kernel_HOST_only_PM_disabled": True,
        "strict_ACK_probes": strict,
        "verdict": "NO_GO_residual_C2_compiled_core_diagnostics_missed",
        "target_operation": False, "private_input_access": False,
    }
    with (OUT / "result.json").open("x") as f:
        json.dump(record, f, indent=2); f.write("\n")
    print("16 supplied host methods PASS; C1/C3 and named C2 forms refused; three compiled core diagnostics still accepted in memory.")


if __name__ == "__main__":
    main()
