#!/usr/bin/env python3
"""SOURCE ONLY: explicit public reads, AST-selected pure parsers, synthetic bytes.

No transport imports, private inputs, RPC, subprocess, device or ARM execution.
Creates only a fresh result.json beside this script; refuses overwriting evidence.
"""
import ast
import datetime
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import types

if not __debug__:
    raise SystemExit("Assertions required")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
OUT = HERE / "result.json"
if OUT.exists():
    raise SystemExit("Preserve existing result; no rerun")
P = "apps/qualification/kernel76-transport/"
D = "docs/qualification-plans/kernel76-transport-20261021/"
E = "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/"
K = "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/"
FILES = [P + n for n in ("retire21a.py", "trial21a.py", "metadata21a.py", "radio_receipt.py")]
FILES += [E + n for n in ("postcheck.py", "RAM76_BOOT21A01.json", "RAM76_POSTCHECK21A01.json",
                          "RADIO_NATIVE76_21A01_HOLD.json", "RADIO_READONLY_RETIREMENT21A01.json")]
FILES += [D + n for n in ("source-review/entry-bind.json", "source-review/transport-source-pins.json",
                          "recovery-source-review/binding.json", "FAILED_TRIAL_FOLLOWUP_REVIEW.md",
                          "READONLY_RETIREMENT21A.md", "RETIREMENT_GUARD_CLOSURE_REVIEW.md")]
FILES += ["bringup/qc07f/initramfs/" + n for n in ("init.c", "radio-ram-loader.h", "wifi-monitor.h")]
FILES += ["scripts/qc07f_boot.py", "scripts/qc07f_private_root_transfer.py",
          "work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py"]
FILES += [K + n for n in ("drivers/usb/core/devio.c", "drivers/usb/core/message.c", "include/linux/usb.h",
                          "drivers/watchdog/watchdog_dev.c", "arch/arm/mach-ak3918ev200/watchdog.c")]
data = {name: (ROOT / name).read_bytes() for name in FILES}
pins = {name: hashlib.sha256(value).hexdigest() for name, value in data.items()}
checks = []


def check(name, value):
    assert value, name
    checks.append(name)


def refuses(name, fn, *args):
    try:
        fn(*args)
    except (AssertionError, UnicodeError):
        checks.append(name)
    else:
        raise AssertionError("Invalid synthetic fixture accepted: " + name)


def selected(name, names, env):
    tree = ast.parse(data[name].decode(), filename=name)
    nodes = [n for n in tree.body if
             isinstance(n, ast.FunctionDef) and n.name in names or
             isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id in names for t in n.targets)]
    assert len(nodes) == len(names)
    exec(compile(ast.Module(body=nodes, type_ignores=[]), name, "exec"), env)
    return types.SimpleNamespace(**env)


binding = json.loads(data[D + "source-review/entry-bind.json"])
for name in ("init.c", "radio-ram-loader.h", "wifi-monitor.h"):
    key = "bringup/qc07f/initramfs/" + name
    check("unchanged_build50_" + name, pins[key] == binding["source_pins"][key]["sha256"])
closure = data[D + "RETIREMENT_GUARD_CLOSURE_REVIEW.md"].decode()
check("retire_exact_reviewed_revision", pins[P + "retire21a.py"] ==
      "aeda5136b55d3cec920e73adcd41209f52c978b526e4ba1709908a6398caa9d1")
for name in ("retire21a.py", "trial21a.py", "metadata21a.py", "radio_receipt.py"):
    check("closure_pin_" + name, pins[P + name] in closure)
transport = json.loads(data[D + "source-review/transport-source-pins.json"])
check("devio_retained_pin", pins[K + "drivers/usb/core/devio.c"] ==
      transport["source_sha256"][K + "drivers/usb/core/devio.c"])
recovery = json.loads(data[D + "recovery-source-review/binding.json"])
for name in ("drivers/watchdog/watchdog_dev.c", "arch/arm/mach-ak3918ev200/watchdog.c"):
    check("watchdog_retained_pin_" + name, pins[K + name] == recovery["input_sha256"][K + name])
boot = selected("scripts/qc07f_boot.py", {"RECORDER_SHA"}, {})
check("recorder_source_pin", pins["work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py"] == boot.RECORDER_SHA)
native = json.loads(data[E + "RADIO_NATIVE76_21A01_HOLD.json"])
retired = json.loads(data[E + "RADIO_READONLY_RETIREMENT21A01.json"])
check("original_strict_trial_still_failed", native["whole_strict_loader_ACK_receipt_passed"] is False and native["HOLD_retained"] is True)
check("separate_retirement_public_receipt", retired["state"] == "READONLY_POSTFAILURE_RETIREMENT_OBSERVED_ORIGINAL_TRIAL_FAILED_HOLD")
check("retirement_recipe_binding", retired["recipe_sha256"] == pins[P + "retire21a.py"])
check("retirement_did_not_query_radio", retired["no_radio_USB_query_reset_module_network_media_or_persistent_write"] is True)
check("retirement_refresh_is_exact_boolean_true", type(retired["one_empty_monitor_refresh"]) is bool and retired["one_empty_monitor_refresh"])

r = selected(P + "radio_receipt.py", {"parse_application_state", "parse_firmware_info", "parse_parent_fds"}, {"re": re, "struct": struct})
post = selected(E + "postcheck.py", {"PROMPT", "QUERIES", "body"}, {"re": re})
meta = selected(P + "metadata21a.py", {"script"}, {"re": re})
diag = selected(P + "trial21a.py", {"HCD_DIAGNOSTIC"}, {"re": re}).HCD_DIAGNOSTIC
ret = selected(P + "retire21a.py", {"TRANSITION", "NEW_KERNEL"}, {"re": re})
state_reply = bytes.fromhex("1a2b07001000020000ff0000000000fd")
state = "\n".join(("RADIO BOOT STATE QUERY TX RESULT 0x00000014", "RADIO RX RESULT 0x00000010",
                   "RADIO RX BYTES " + state_reply.hex(),
                   "RADIO BOOT QUERY CHECK/CMD/COOKIE MATCH, RESULT 0x000000ff", "RADIO STATE APPLICATION"))
check("synthetic_APPLICATION_control", r.parse_application_state(state))
for offset in (0, 2, 3, 4, 6, 7, 8, 9, 15):
    value = bytearray(state_reply); value[offset] ^= 1
    refuses("state_header_mutation_" + str(offset), r.parse_application_state, state.replace(state_reply.hex(), value.hex()))
for label, bad in (("BOOTLOADER", state.replace("APPLICATION", "BOOTLOADER")),
                   ("wrong_TX_length", state.replace("0x00000014", "0x00000013")),
                   ("extra_RX", state + "\nRADIO RX RESULT 0x00000010"),
                   ("missing_line", "\n".join(state.splitlines()[:-1])),
                   ("interleaved_printk", state.replace("RADIO RX BYTES", "usb 1-1: USB disconnect\nRADIO RX BYTES"))):
    refuses("state_reject_" + label, r.parse_application_state, bad)
reply = bytearray.fromhex("1a2b0301240001002b001800") + bytearray(24)
struct.pack_into("<IIHH", reply, 12, 0x02010001, 22118, 0x8400, 1)
reply[24:] = b"DUMMY_PUBLIC"  # synthetic, never acquired identifying data
info = "\n".join(("RADIO READONLY FWINFO TX RESULT 0x0000000c", "RADIO RX RESULT 0x00000024",
                  "RADIO RX BYTES " + reply.hex(), "RADIO FWINFO COOKIE/CMD MATCH, RETURN/PAYLOAD LENGTH 0x00000018"))
check("synthetic_fwinfo_returns_only_booleans", r.parse_firmware_info(info) == {
    "retained_chip_version_SVN_CPU_identity_matches": True, "device_identifying_fields_kept_private": True})
for offset in (0, 2, 3, 4, 6, 7, 8, 10, 11, 12, 16, 20, 22):
    value = bytearray(reply); value[offset] ^= 1
    refuses("fwinfo_header_identity_mutation_" + str(offset), r.parse_firmware_info, info.replace(reply.hex(), value.hex()))
for label, bad in (("short", info.replace(reply.hex(), reply[:-1].hex())),
                   ("extra_line", info + "\nUNEXPECTED"),
                   ("interleaved", info.replace("RADIO RX BYTES", "musb: warning\nRADIO RX BYTES"))):
    refuses("fwinfo_reject_" + label, r.parse_firmware_info, bad)
for command, body in (("radio state", state), ("radio info", info)):
    response = command.encode() + b"\r\n" + body.encode() + b"\r\nqc07f> "
    check("fresh_body_" + command, post.body(response, command) == body)
    refuses("duplicate_prompt_" + command, post.body, response + b"qc07f> ", command)
    refuses("nonterminal_prompt_" + command, post.body, response + b"late", command)

inventory = [len(c.encode()) + 1 for c in list(post.QUERIES.values()) + [post.QUERIES["boot_id"]]]
line = meta.script("1" * 32).encode()
metadata = [6] + [len(line[i:i + 64]) for i in range(0, len(line), 64)] + [1, 5]
check("fourteen_inventory_320", len(inventory) == 14 and sum(inventory) == 320)
check("cached_metadata_527_539_12_sends", len(line) == 527 and sum(metadata) == 539 and len(metadata) == 12)
check("round_859_26_sends", sum(metadata + inventory) == 859 and len(metadata + inventory) == 26)
check("two_identity_queries_23_UART_bytes", len(b"radio state\r") == 12 and len(b"radio info\r") == 11)
check("minimal_success_1741_or_1742", 2 * 859 + 23 == 1741 and 1 + 2 * 859 + 23 == 1742)
check("broader_two_refresh_accounting_1743", 2 * 860 + 23 == 1743)
for rp in (False, True):
    for rt in (False, True):
        for rr in (False, True):
            cboot = 100000  # synthetic; not a target checkpoint
            cfailed = cboot + 320 + int(rp) + 320 + int(rt) + 539 + 18
            cret = cfailed + 859 + int(rr)
            check(f"counter_derivation_{int(rp)}{int(rt)}{int(rr)}", cret == cboot + 2056 + int(rp) + int(rt) + int(rr))

warning = b"usb 1-1: Process 1 (init) called USBDEVFS_CLEAR_HALT for active endpoint 0x81\n"
check("CONCRETE_GAP_existing_HCD_regex_misses_active_endpoint_warning", not diag.search(warning))
check("CONCRETE_GAP_existing_topology_regex_also_misses_warning", not ret.TRANSITION.search(warning))
active = re.compile(rb"Process [^\r\n]*called USBDEVFS_(?:CLEAR_HALT|RESETEP) for active endpoint", re.I)
check("proposed_narrow_active_endpoint_gate_matches", bool(active.search(warning)))
for split in range(len(warning) + 1):
    prefix, suffix = warning[:split], warning[split:]
    carry = prefix.rsplit(b"\n", 1)[-1]
    # A complete warning before a cursor must already be checked in the prefix.
    assert active.search(prefix) or active.search(carry + suffix)
check("proposed_warning_gate_all_record_splits", True)
for message in (b"usb 1-1: USB disconnect, device number 3\n",
                b"usb 1-1: new high-speed USB device number 4\n",
                b"usb 1-1: reset high-speed USB device number 3\n"):
    for split in range(len(message) + 1):
        assert ret.TRANSITION.search(message[:split]) or ret.TRANSITION.search(message[:split].rsplit(b"\n", 1)[-1] + message[split:])
check("topology_all_record_splits", True)
check("CLEAR_HALT_has_5000ms_control_timeout", b"#define USB_CTRL_SET_TIMEOUT\t5000" in data[K + "include/linux/usb.h"])
check("CLEAR_HALT_forces_DATA0_on_success_source", b"usb_reset_endpoint(dev, endp);" in data[K + "drivers/usb/core/message.c"])
check("minimal_pre_active_floor_140_exceeds_131point2_nominal_plus_reserve", 140 > 2 * 6 + (6 + 3 + 10 + 6 + .2 + 14 * 6) + 10)
check("coverage_600_minus_budget300_retains300", 600 - 300 >= 300)
for name, pin in pins.items():
    assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == pin, "Input changed: " + name
record = {
    "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "scope": "SOURCE ONLY; public allowlist, AST-selected pure parsers, synthetic fixtures",
    "argv": ["python3", "-B", str(Path(__file__).relative_to(ROOT))],
    "python": sys.version, "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    "input_sha256": pins, "checks_passed": len(checks), "checks": checks,
    "inputs_rehash_unchanged": True, "private_inputs_accessed": False, "transport_imported": False,
    "target_RPC_status_network_build_or_ARM_execution": False,
    "concrete_existing_guard_gap": "USBDEVFS active-endpoint dev_warn has no warning token and is missed by HCD_DIAGNOSTIC and TRANSITION",
    "wrapper_exists_or_is_admitted_by_this_audit": False,
    "original_trial_still_FAILED_HOLD": True,
}
with OUT.open("x") as stream:
    json.dump(record, stream, indent=2); stream.write("\n")
print(f"{len(checks)} source-audit checks PASS; concrete guard gap recorded; no controller/target admission")
