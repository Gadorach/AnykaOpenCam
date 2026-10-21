#!/usr/bin/env python3
"""HOST ONLY: explicit public source reads and pure AST-selected parser fixtures.

Never imports the transport/trial modules, opens private data, or constructs RPC.
Exclusively creates result.json; do not rerun over an existing result.
"""
import ast
import copy
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
PKG = "apps/qualification/kernel76-transport/"
EV = "evidence/qualification/qc07f/execution-20261021/ram76-resume21a/"
DOC = "docs/qualification-plans/kernel76-transport-20261021/"
NATIVE = "work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/"
FILES = [
    PKG + name for name in ("trial21a.py", "metadata21a.py", "radio_receipt.py", "test_radio_receipt.py")
] + [EV + "postcheck.py", EV + "RADIO_NATIVE76_21A01_HOLD.json",
     "bringup/qc07f/initramfs/init.c", "bringup/qc07f/initramfs/radio-ram-loader.h",
     "bringup/qc07f/initramfs/wifi-monitor.h", "scripts/qc07f_boot.py",
     "scripts/qc07f_private_root_transfer.py", DOC + "source-review/entry-bind.json",
     DOC + "source-review/transport-source-pins.json", DOC + "recovery-source-review/binding.json",
     DOC + "CORE_DIAGNOSTIC_CLOSURE_REVIEW.md", DOC + "WATCHDOG_RECOVERY_REVIEW.md",
     NATIVE + "station76-final/utils/fwdl.c",
     NATIVE + "linux/drivers/usb/core/hub.c", NATIVE + "linux/drivers/usb/core/devio.c",
     NATIVE + "linux/drivers/usb/core/sysfs.c", NATIVE + "linux/drivers/usb/core/endpoint.c",
     NATIVE + "linux/drivers/usb/musb/musb_core.c",
     "bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp.c",
     "bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/uart.c"]
data = {name: (ROOT / name).read_bytes() for name in FILES}
pins = {name: hashlib.sha256(value).hexdigest() for name, value in data.items()}
assert pins[PKG + "trial21a.py"] == "6d0ce652d3355baef2e5505e9d2a34813aa1f780289b17268ccb160c4748da9c"
binding = json.loads(data[DOC + "source-review/entry-bind.json"])
for name in ("init.c", "radio-ram-loader.h", "wifi-monitor.h"):
    key = "bringup/qc07f/initramfs/" + name
    assert pins[key] == binding["source_pins"][key]["sha256"]
transport = json.loads(data[DOC + "source-review/transport-source-pins.json"])
for name in (NATIVE + "linux/drivers/usb/core/devio.c", NATIVE + "station76-final/utils/fwdl.c"):
    assert pins[name] == transport["source_sha256"][name]


def selected(name, names, env):
    """Only named pure functions/constants; imports and module main are omitted."""
    tree = ast.parse(data[name].decode(), filename=name)
    nodes = [node for node in tree.body if
             isinstance(node, ast.FunctionDef) and node.name in names or
             isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id in names for t in node.targets)]
    assert len(nodes) == len(names)
    exec(compile(ast.Module(body=nodes, type_ignores=[]), name, "exec"), env)
    return types.SimpleNamespace(**env)


checks = []


def check(name, value):
    assert value, name
    checks.append(name)


def refuses(name, fn, *args):
    try:
        fn(*args)
    except AssertionError:
        checks.append(name)
    else:
        raise AssertionError("Accepted invalid synthetic fixture: " + name)


r = selected(PKG + "radio_receipt.py", {"ACK", "BEGIN", "DONE", "ack", "parse_loader", "parse_parent_fds"}, {"re": re})
sample = selected(PKG + "test_radio_receipt.py", {"ack", "loader"}, {"r": r})
text = sample.loader()
check("synthetic_control_151_ACKs", r.parse_loader(text)["matched_ACKs"] == 151)
final = sample.ack(4, 176)
interrupted = final.replace("RADIO BOOT RX cmd0x", "RADIO BOOT RX cmd0xusb 1-1: USB disconnect, device number 2\n", 1)
refuses("synthetic_interrupted_RUN_ACK_stays_failed_despite_DONE", r.parse_loader, text.replace(final, interrupted))
refuses("synthetic_missing_RUN_ACK_stays_failed_despite_DONE", r.parse_loader, text.replace(final + "\n", ""))
diag = selected(PKG + "trial21a.py", {"HCD_DIAGNOSTIC"}, {"re": re}).HCD_DIAGNOSTIC
check("ordinary_disconnect_is_not_HCD_error_pattern", not diag.search(b"usb 1-1: USB disconnect, device number 2"))
for message in (b"musb_core 980: unhandled DISCONNECT transition (a_wait_bcon)",
                b"musb-hdrc musb-hdrc.0.auto: Babble\n", b"musb_core 692: bogus host RESUME (a_host)"):
    check("HCD_error_refused_" + str(len(checks)), bool(diag.search(message)))
fds = "\n".join(f"lrwx------ 1 64 {i} -> /dev/console" for i in range(4))
check("four_console_FDs_control", r.parse_parent_fds(fds)["PID1_USB_watchdog_fds_absent"])
for suffix in ("4 -> /dev/watchdog0", "4 -> /dev/bus/usb/001/002"):
    refuses("leaked_" + suffix, r.parse_parent_fds, fds + "\nlrwx------ 1 64 " + suffix)
post = selected(EV + "postcheck.py", {"PROMPT", "BOOTARGS", "QUERIES", "body", "parse"}, {"re": re})
nonce = "0123456789abcdef0123456789abcdef"
meta = selected(PKG + "metadata21a.py", {"ATTRS", "EPATTRS", "script", "parse"}, {"re": re, "radio_receipt": r})
script = meta.script(nonce)
lines = [f"K76MD_BEGIN_{nonce}", "FDS", *fds.splitlines(), "FD_RC:0"]
for key, value in zip(meta.ATTRS, ("a012", "8400", "1", "480")):
    lines += ["ATTR:" + key, value]
for ep, address in (("ep_01", "01"), ("ep_81", "81")):
    for key, value in zip(meta.EPATTRS, (address, "02", "0200")):
        lines += [f"EP:{ep}:{key}", value]
lines += [f"K76MD_DONE_{nonce}:0", "# "]
md = "\n".join(lines)
check("cached_metadata_control", meta.parse(md, nonce)["no_bound_USB_interface_driver"])
refuses("metadata_missing_endpoint_refuses", meta.parse, md.replace("EP:ep_81:wMaxPacketSize\n0200\n", ""), nonce)
refuses("metadata_wrong_endpoint_refuses", meta.parse, md.replace("0200", "0040", 1), nonce)
refuses("metadata_late_log_not_filtered", meta.parse, md.replace("# ", "usb 1-1: new device\n# "), nonce)
refuses("metadata_failed_read_refuses", meta.parse, md.replace(f"DONE_{nonce}:0", f"DONE_{nonce}:1"), nonce)
UUID = "01234567-89ab-4cde-8f01-23456789abcd"
rows = {"pid1": "init", "release": "6.18.55-qc07f-prototype-dirty",
        "identity": "Pid:\t1\nUid:\t0\t0\t0\t0\nCapEff:\t0000000000020000\n",
        "cmdline": post.BOOTARGS, "modules": "",
        "mounts": "rootfs / rootfs rw 0 0\ndevtmpfs /dev devtmpfs rw 0 0\nproc /proc proc rw 0 0\nsysfs /sys sysfs rw 0 0\n",
        "memory": "MemTotal: 35000 kB\nMemFree: 12000 kB\nCmaTotal: 16384 kB\nCmaFree: 10000 kB",
        "iomem": "81800000-83ffffff : System RAM",
        "ISP": "result=-61 readable=0 guard_errors=0 irq_mode=0 irq_frames=0 queue_mode=0 queue_started=0 queue_held=0 wb_owner_active=0",
        "watchdog": "inactive", "nowayout": "0", "boot_id": UUID, "boot_id_after": UUID,
        "UART": "rx_error_events=0 mem_busy_ticks=0 last_status=00080000 rxpos=12; cached diagnostic, lost bytes cannot be reconstructed"}
check("inventory_control", post.parse(rows)["CmaTotal"] == 16384)
for key, bad in (("watchdog", "active"), ("nowayout", "1"), ("boot_id_after", "ffffffff-ffff-ffff-ffff-ffffffffffff"),
                 ("UART", rows["UART"].replace("rx_error_events=0", "rx_error_events=1")),
                 ("ISP", rows["ISP"].replace("queue_started=0", "queue_started=1")),
                 ("modules", "unexpected_module")):
    changed = copy.deepcopy(rows); changed[key] = bad
    refuses("inventory_refuses_" + key, post.parse, changed)
refuses("fresh_prompt_required", post.body, b"init\n", "cat /proc/1/comm")
refuses("multiple_prompts_refused", post.body, b"qc07f> init\nqc07f> ", "cat /proc/1/comm")
inventory_bytes = sum(len(c.encode()) + 1 for c in list(post.QUERIES.values()) + [post.QUERIES["boot_id"]])
check("fixed_inventory_14_queries_320_bytes", len(post.QUERIES) == 13 and inventory_bytes == 320)
check("metadata_script_no_usbfs_or_guard_open", "/dev/bus" not in script and "/dev/watchdog" not in script and "radio " not in script)
loader = data["bringup/qc07f/initramfs/radio-ram-loader.h"].decode()
check("success_text_before_unchecked_cleanup", loader.index("RAM LOAD/CRC/RUN ACKNOWLEDGED") < loader.index("cleanup:"))
hold = json.loads(data[EV + "RADIO_NATIVE76_21A01_HOLD.json"])
check("public_failure_preserved_not_accepted", hold["HOLD_retained"] and not hold["whole_strict_loader_ACK_receipt_passed"] and not hold["post_load_descriptor_FD_idle_retirement_performed"])
check("all_inspected_inputs_unchanged", all((ROOT / name).read_bytes() == value for name, value in data.items()))
result = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "python": sys.version,
          "argv": ["python3", "-B", str(Path(__file__).relative_to(ROOT))],
          "scope": "HOST-only public-source AST-selected pure parser fixtures; no transport imports/private reads/build/ARM execution",
          "input_sha256": pins, "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
          "checks": checks, "count": len(checks), "all_passed": True,
          "wire_derivation": {"metadata_script_ascii_bytes": len(script.encode()), "metadata_shell_total_bytes": len(script.encode()) + len(b"shell\r") + 1 + len(b"exit\r"),
                              "inventory_14_query_bytes": inventory_bytes, "native_load_wire_bytes": len(b"radio native-load\r"),
                              "optional_empty_refresh_bytes": 1},
          "target_IO": False, "native_retirement_proved": False, "original_trial_still_failed": True}
with OUT.open("x") as f:
    json.dump(result, f, indent=2); f.write("\n")
print(json.dumps({"checks": len(checks), "all_passed": True, "wire_derivation": result["wire_derivation"]}))
