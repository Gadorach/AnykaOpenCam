#!/usr/bin/env python3
"""Independent host-only closure probes; no real trial/private/transport entry."""
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


def pins(paths):
    return {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in paths}


def main():
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    prior_paths = [OUT.parent / n for n in (
        "METADATA_RECEIPT_REVIEW.md", "TRIAL_CORRECTION_REVIEW.md",
        "TRIAL_FINAL_REVIEW.md", "RADIO_ENTRY_REVIEW.md")]
    prior_paths += [p for folder in ("metadata-review", "trial-review", "final-review", "source-review")
                    for p in sorted((OUT.parent / folder).iterdir()) if p.is_file()]
    previous = json.loads((OUT.parent / "final-review/result.json").read_text())
    sources = [ROOT / n for n in previous["source_sha256"]]
    source_pins, prior_pins = pins(sources), pins(prior_paths)
    changed = [n for n, digest in source_pins.items() if digest != previous["source_sha256"][n]]
    assert set(changed) == {"apps/qualification/kernel76-transport/trial21a.py",
                            "apps/qualification/kernel76-transport/test_trial21a.py"}

    argv = [sys.executable, "-B", "-m", "unittest", "discover", "-s",
            "apps/qualification/kernel76-transport", "-p", "test_*.py", "-v"]
    begin = datetime.datetime.now(datetime.timezone.utc).isoformat()
    run = subprocess.run(argv, cwd=ROOT, env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"),
                         capture_output=True, text=True, timeout=60)
    log = OUT / "unittest-discover.log"
    with log.open("x") as f:
        f.write(json.dumps({"argv": argv, "cwd": str(ROOT), "start_utc": begin}) + "\n")
        f.write("stdout:\n" + run.stdout + "\nstderr:\n" + run.stderr + "\nexit: " + str(run.returncode) + "\n")
    assert run.returncode == 0 and "Ran 17 tests" in run.stderr

    sys.path.insert(0, str(APP))
    import test_trial21a as host
    import test_radio_receipt as samples
    import radio_receipt as receipt

    diagnostics = {
        "host_resume": b"[ 1.000000] musb_handle_intr_resume 692: bogus host RESUME (a_host)\r\n",
        "peripheral_resume": b"[ 1.000000] musb_handle_intr_resume 721: bogus peripheral RESUME (a_host)\r\n",
        "disconnect": b"[ 1.000000] musb_handle_intr_disconnect 980: unhandled DISCONNECT transition (a_wait_bcon)\r\n",
        "babble": b"[ 1.000000] musb-hdrc musb-hdrc.0.auto: Babble\r\n",
    }
    for diagnostic in diagnostics.values():
        for value in (diagnostic, diagnostic.replace(b"\r\n", b"\n"), diagnostic.rstrip(b"\r\n")):
            assert host.t.HCD_DIAGNOSTIC.search(value)
    for benign in (b"musb-hdrc musb-hdrc.0.auto: MUSB HDRC host driver\r\n",
                   b"usb 1-1: new high-speed USB device number 2 using musb-hdrc\r\n",
                   b"musb-hdrc musb-hdrc.0.auto: Babble statistics disabled\r\n"):
        assert not host.t.HCD_DIAGNOSTIC.search(benign)

    placements = ("before", "after_load", "after_state", "after_info", "post_metadata", "post_inventory")
    cases = {name + "_" + place: (diagnostic, place)
             for name, diagnostic in diagnostics.items() for place in placements}
    source = inspect.getsource(host.simulate)
    owner_line = "            return value\n        def check_stream"
    deliver_line = '            self.raw.extend(answer); self.recent = "# " if marker == b"# " else "qc07f> "'
    assert source.count(owner_line) == source.count(deliver_line) == 1
    source = source.replace(owner_line, '''            if self.owner_calls == 2:
                if fault == 'peer_uid_refresh': self.peer = (123, 1001, 1000)
                if fault == 'peer_gid_refresh': self.peer = (123, 1000, 1001)
                if fault == 'busy_refresh': value['tx_busy'] = True
                if fault == 'part_refresh': value['part'] = 'foreign'
                if fault == 'TX_refresh': self.tx += 1
            return value
        def check_stream''')
    source = source.replace('patch.object(t.b, "ticks", return_value=42)',
        'patch.object(t.b, "ticks", side_effect=lambda pid: 43 if fault == "start_refresh" and instances and instances[0].owner_calls == 2 else 42)')
    source = source.replace(deliver_line, '''            trailing = b''
            if fault in REVIEW_CASES:
                diagnostic, place = REVIEW_CASES[fault]
                inject = ((place == 'before' and command == 'exit' and not self.load_count)
                       or (place == 'after_load' and command == 'radio native-load')
                       or (place == 'after_state' and command == 'radio state')
                       or (place == 'after_info' and command == 'radio info')
                       or (place == 'post_metadata' and command == 'exit' and self.load_count))
                if inject: answer = diagnostic + answer
                # Arrival just after the returned last query body: parse sees the
                # original body; the whole-interval final fence must see the log.
                if place == 'post_inventory' and self.load_count and command == POST.QUERIES['boot_id']:
                    trailing = diagnostic
            self.raw.extend(answer); self.raw.extend(trailing)
            self.recent = "# " if marker == b"# " else "qc07f> "''')
    namespace = dict(vars(host), REVIEW_CASES=cases)
    # Only the supplied Fake/harness changes in memory. Actual main and helpers
    # remain the imported production definitions and retain their real predicate.
    exec(compile(source, "<core-closure-Fake-only-injections>", "exec"), namespace)
    simulate = namespace["simulate"]
    owner_faults = ("changed_owner", "lost_catch", "peer_uid_refresh", "peer_gid_refresh",
                    "start_refresh", "busy_refresh", "part_refresh", "TX_refresh")
    original_warnings = ("source_warning_before", "recognized_warning_before", "broken_before",
                         "warning_after_load", "warning_after_state")
    probes = {}
    for name in (None, *owner_faults, *original_warnings, *cases):
        result = simulate(name)
        result["radio_commands"] = [c for c in result.pop("calls") if c.startswith("radio ")]
        probes[name or "healthy"] = result
    assert probes["healthy"]["PASS"] and not probes["healthy"]["HOLD"]
    assert probes["healthy"]["radio_commands"] == ["radio native-load", "radio state", "radio info"]
    expected_commands = {"before": [], "after_load": ["radio native-load"],
        "after_state": ["radio native-load", "radio state"],
        "after_info": ["radio native-load", "radio state", "radio info"],
        "post_metadata": ["radio native-load", "radio state", "radio info"],
        "post_inventory": ["radio native-load", "radio state", "radio info"]}
    for name, result in probes.items():
        if name == "healthy": continue
        assert result["error"] and result["HOLD"] and not result["PASS"]
        assert result["load_count"] <= 1
        if name in cases:
            assert result["radio_commands"] == expected_commands[cases[name][1]]
        elif name in owner_faults or name.endswith("before"):
            assert result["radio_commands"] == []
    assert probes["warning_after_load"]["radio_commands"] == expected_commands["after_load"]
    assert probes["warning_after_state"]["radio_commands"] == expected_commands["after_state"]

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
        else: raise AssertionError("ACK mutant accepted")

    manifest_path = NEW / "analysis/native-usb76-package01/package76-manifest.json"
    manifest = json.loads(manifest_path.read_text())
    linked_path = NEW / "analysis/native-usb76-package01/build/vmlinux"
    linked = linked_path.read_bytes()
    assert sha(linked) == manifest["output_sha256"][str(linked_path.relative_to(ROOT))]
    formats = (b"bogus %s RESUME (%s)\n\0", b"unhandled DISCONNECT transition (%s)\n\0", b"Babble\n\0")
    assert all(fmt in linked for fmt in formats)
    config_path = NEW / "analysis/native-usb76-package01/build/.config"
    config = config_path.read_text().splitlines()
    assert "CONFIG_USB_MUSB_HOST=y" in config and "# CONFIG_PM is not set" in config
    # Finite source-change check: removing only the three new alternatives
    # reconstructs the previous final-review trial's exact pinned file.
    trial = (APP / "trial21a.py").read_bytes()
    added = (b'    rb"bogus (?:host|peripheral) RESUME \\([^\\r\\n]*\\)|unhandled DISCONNECT transition \\([^\\r\\n]*\\)|"\n'
             b'    rb"musb[^\\r\\n]*: Babble(?:\\r?\\n|$)|"\n')
    assert trial.count(added) == 1
    assert sha(trial.replace(added, b"")) == previous["source_sha256"]["apps/qualification/kernel76-transport/trial21a.py"]
    assert source_pins == pins(sources) and prior_pins == pins(prior_paths)
    record = {"start_utc": started, "end_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "python": sys.version, "collector_sha256": sha(Path(__file__).read_bytes()),
        "source_sha256": source_pins, "only_source_changes_since_final_review": changed,
        "trial_minus_three_new_patterns_equals_previous_exact_pin": True,
        "prior_reports_and_failures_sha256": prior_pins, "prior_reports_and_failures_unchanged": True,
        "tests": {"argv": argv, "start_utc": begin, "exit": run.returncode, "methods": 17,
                  "log": log.name, "log_sha256": sha(log.read_bytes())},
        "full_main_in_memory_probes": probes, "strict_ACK_probes": strict,
        "compiled_core_formats_present": True, "linked_kernel_sha256": sha(linked),
        "kernel_HOST_only_PM_disabled": True,
        "public_artifact_sha256": pins([manifest_path, config_path]),
        "verdict": "CONDITIONAL_SOURCE_GO_one_whole_functional_trial_only",
        "target_operation": False, "private_input_access": False,
        "fresh_parent_hardware_admission_required": True}
    with (OUT / "result.json").open("x") as f:
        json.dump(record, f, indent=2); f.write("\n")
    print("17 methods PASS; 24 core-diagnostic placements and 13 C1/C3 probes refuse; healthy flow loads once; conditional source GO only.")


if __name__ == "__main__":
    main()
