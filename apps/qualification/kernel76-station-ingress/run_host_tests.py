#!/usr/bin/env python3
"""Exclusive A2a public-source synthetic native HOST runner; no project imports."""
import argparse
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

if sys.flags.optimize:
    raise SystemExit("refuse optimized Python")
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCE = HERE / "source"
A1 = ROOT / "apps/qualification/kernel76-station/source"
E = ROOT / "docs/qualification-plans/kernel76-transport-20261021/station-ingress-a2a-host01"
parser = argparse.ArgumentParser()
parser.add_argument("--run", required=True)
args = parser.parse_args()
if not re.fullmatch(r"run[0-9]{2}", args.run):
    raise SystemExit("runNN required")
OUT = E / args.run
OUT.mkdir(exist_ok=False)
inputs, commands, extractions = {}, [], []

def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()

def sha(b):
    return hashlib.sha256(b).hexdigest()

def read(p):
    b = p.read_bytes()
    inputs[str(p.relative_to(ROOT))] = {"bytes": len(b), "sha256": sha(b)}
    return b

def write(p, b):
    with p.open("xb") as f:
        f.write(b.encode() if isinstance(b, str) else b)

def extract(p, start, end, text=None):
    t = read(p).decode() if text is None else text
    if start not in t:
        start = start.replace("\n", "\r\n")
    if end not in t:
        end = end.replace("\n", "\r\n")
    assert t.count(start) == 1 and t.count(end) == 1, (p, start, end)
    a, b = t.index(start), t.index(end)
    assert a < b
    body = t[a:b]
    extractions.append({"source": str(p.relative_to(ROOT)), "start_marker": start,
                        "end_marker": end, "start_byte": a, "bytes": b-a,
                        "start_line": t.count("\n", 0, a)+1, "sha256": sha(body.encode()),
                        "mutated": text is not None})
    return body

def command(label, argv, expected=0):
    start = utc()
    r = subprocess.run([str(x) for x in argv], cwd=ROOT, capture_output=True, timeout=90,
                       env={"PATH": "/usr/bin:/bin", "LC_ALL": "C",
                            "ASAN_OPTIONS": "detect_leaks=1:abort_on_error=0",
                            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
    write(OUT / (label + ".stdout"), r.stdout); write(OUT / (label + ".stderr"), r.stderr)
    commands.append({"label": label, "argv": [str(x) for x in argv], "cwd": str(ROOT),
                     "utc_start": start, "utc_end": utc(), "exit": r.returncode,
                     "stdout_sha256": sha(r.stdout), "stderr_sha256": sha(r.stderr)})
    if expected is not None and r.returncode != expected:
        raise RuntimeError(f"{label}: exit {r.returncode}, expected {expected}")
    return r

def main():
    baseline = json.loads(read(E / "copy-baseline.json"))
    assert baseline["count"] == 68
    changed = []
    for row in baseline["files"]:
        rel = row["path"]
        a, b = read(A1 / rel), read(SOURCE / rel)
        assert sha(a) == row["original_sha256"] == row["copied_sha256"] and len(a) == row["bytes"]
        if a != b:
            changed.append(rel)
            write(OUT / (rel.replace("/", "_") + ".diff"), "".join(difflib.unified_diff(
                a.decode().splitlines(keepends=True), b.decode().splitlines(keepends=True),
                fromfile="sealed-A1/" + rel, tofile="A2a/" + rel)))
    assert changed == ["hgic_def.h", "hgic_smac/core.c", "hgic_smac/hgics.h", "hgic_smac/rx.c",
                       "utils/fwctrl.c", "utils/fwctrl.h", "utils/if_usb.c"], changed
    assert {str(p.relative_to(SOURCE)) for p in SOURCE.rglob("*") if p.is_file()} == {x["path"] for x in baseline["files"]}
    for p in sorted((HERE / "tests").glob("*")):
        read(p)
    for p in (Path(__file__), HERE / "prepare_source.py", HERE / "prepare_tests.py"):
        read(p)
    inc = OUT / "include"
    (inc / "linux").mkdir(parents=True)
    for name in ("completion.h", "workqueue.h", "unaligned.h", "etherdevice.h", "build_bug.h"):
        write(inc / "linux" / name, '#include "shim.h"\n')
    write(inc / "linux/errno.h", '#include_next <linux/errno.h>\n')
    defs = read(SOURCE / "hgic_def.h").decode()
    constants = []
    for n in ("HGIC_WDEV_ID_STA", "HGIC_HDR_TX_MAGIC", "HGIC_HDR_RX_MAGIC", "HGIC_WLAN_8400",
              "HGIC_TX_COOKIE_MASK", "HGIC_CMD_TIMEOUT", "HGIC_BLOCK_ACK_CNT"):
        matches = re.findall(r"^#define " + n + r"\s+[^\r\n]+", defs, re.M)
        assert len(matches) == 1
        constants += matches
    definitions = "\n".join(constants) + "\n"
    bus = extract(SOURCE / "hgic_def.h", "struct hgic_bus {", "#ifdef __RTOS__\n#define ALLOC_ORDERED_WORKQUEUE")
    ack = extract(SOURCE / "hgic_def.h", "struct hgic_dack_hdr {", "struct hgic_nlmsg_hdr {")
    state = extract(SOURCE / "hgic_smac/hgics.h", "enum hgics_state {", "enum hgics_vif_state {")
    txstatus = extract(SOURCE / "hgic_smac/hgics.h", "struct hgics_txstatus {", "int hgics_rx_data(")
    # txstatus extraction also contains the added rx-error declaration only after
    # the end marker, so no callback declaration is silently synthesized here.
    def fwunit(source):
        p = source / "utils/fwctrl.c"
        return (extract(p, "#define STR_LEN", "void hgic_fwctrl_init(") +
                extract(p, "int hgic_fwctrl_get_fwinfo(", "int hgic_fwctrl_set_countryregion("))
    def base(source, ingress):
        return ('#include "' + ("ingress_shim.h" if ingress else "shim.h") + '"\n' +
                '#include "' + str(source / "hgic.h") + '"\n' + definitions +
                (bus if ingress else "") + '#include "' + str(source / "utils/fwctrl.h") + '"\n' +
                '#include "' + str(source / "utils/control_receipt.h") + '"\n')
    def unit(source, overrides=None):
        overrides = overrides or {}
        p = source / "utils/if_usb.c"
        usbtext = overrides.get("usb")
        code = base(source, True) + fwunit(source)
        code += ack + state + txstatus
        code += '''
struct hgics_wdev {
    struct hgic_bus *bus;
    struct hgic_fwctrl ctrl;
    unsigned long state, last_rx;
    int if_test, test_rx_len, txwnd;
    struct completion txwnd_cp;
};
static void hgics_rx_ack(struct hgics_wdev *h, struct hgics_txstatus *s) { (void)h; (void)s; data_calls++; }
static void hgics_rx_agg_frm(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
static void hgics_rx_sigfrm(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
static void hgics_rx_bt_data(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
'''
        if source == A1:
            code += '#define A2A_ORIGINAL_REPRO 1\nstatic void hgics_rx_error(void *p) { (void)p; abort(); }\n'
            rx = read(source / "hgic_smac/rx.c").decode()
            # Last function extends to EOF. Explicit byte extent is still sealed.
            start = "int hgics_rx_data("
        else:
            rx = overrides.get("rx", read(source / "hgic_smac/rx.c").decode())
            start = "void hgics_rx_error("
        assert rx.count(start) == 1
        a = rx.index(start); body = rx[a:]
        extractions.append({"source": str((source / "hgic_smac/rx.c").relative_to(ROOT)),
                            "start_byte": a, "bytes": len(body.encode()), "start_line": rx.count("\n", 0, a)+1,
                            "end_marker": "EOF", "sha256": sha(body.encode()), "mutated": "rx" in overrides})
        code += body
        code += extract(p, "#define HGIC_PKT_MAX_LEN", "static int txq_cnt", text=usbtext)
        code += '''
static u32 max_pkt_len = HGIC_PKT_MAX_LEN;
static void hgic_usb_receive(struct urb *urb);
static void hgic_usb_cancle(struct hgic_usb *d) { (void)d; cancel_calls++; }
'''
        code += extract(p, "static void hgic_usb_enq(", "static int hgic_usb_submit_rx_urbs(", text=usbtext)
        receive_marker = "static void hgic_usb_receive(struct urb *urb)\n{"
        code += extract(p, receive_marker, "static void hgic_usb_tx_complete(", text=usbtext)
        code += '#include "' + str(HERE / "tests/test_ingress.c") + '"\n'
        return code
    command("compiler-version", ["cc", "--version"])
    assert command("compiler-target", ["cc", "-dumpmachine"]).stdout.strip() == b"x86_64-pc-linux-gnu"
    common = ["cc", "-std=gnu11", "-g", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
              "-Wno-unused-parameter", "-Wno-sign-compare", "-I"+str(inc), "-I"+str(HERE / "tests")]
    modes = [("optimized", ["-O2"]), ("sanitized", ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"])]
    for name, code in (("ingress", unit(SOURCE)),
                       ("a1-regression", base(SOURCE, False)+fwunit(SOURCE)+'\n#include "'+str(HERE / "tests/test_control.c")+'"\n'),
                       ("original-a1-surplus", unit(A1))):
        c = OUT / (name + ".c"); write(c, code)
        for label, flags in modes:
            binary = OUT / (name + "-" + label + ".host")
            command(name+"-"+label+"-compile", common+flags+[c,"-o",binary])
            command(name+"-"+label+"-test", [binary])
    mutations = [
        ("missing-original-extent", "rx", "declared != actual", "false"),
        ("missing-control-minimum", "rx", "len < sizeof(struct hgic_ctrl_hdr)", "false"),
        ("missing-status", "usb", "urb->status || !buf->data", "!buf->data"),
        ("ignore-known-ERR", "usb", "(usbdev->status & HGIC_USB_STATUS_ERR) ||", ""),
        ("ignore-known-STOP", "usb", "if (buf->usbdev->status & HGIC_USB_STATUS_STOP)", "if (false)"),
        ("missing-sticky-resubmit-gate", "usb", "READ_ONCE(buf->usbdev->bus.rx_failed)", "false"),
        ("missing-transfer-range", "usb", "urb->actual_length > urb->transfer_buffer_length", "false"),
        ("missing-transfer-backing", "usb", "urb->transfer_buffer != buf->data", "false"),
    ]
    mutants = []
    for name, where, old, new in mutations:
        p = SOURCE / ("hgic_smac/rx.c" if where == "rx" else "utils/if_usb.c")
        text = read(p).decode(); assert text.count(old) == 1
        code = unit(SOURCE, {where: text.replace(old, new)})
        c = OUT / (name + ".c"); write(c, code)
        binary = OUT / (name + ".host")
        command(name+"-compile", common+["-Wno-unused-variable", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", c, "-o", binary])
        r = command(name+"-test", [binary], expected=None)
        assert r.returncode != 0, "surviving mutant: " + name
        assert b"FAIL line" in r.stderr or b"Sanitizer" in r.stderr, "nonbehavioral failure"
        mutants.append({"name": name, "target": where, "old": old, "new": new, "compile_exit": 0,
                        "runtime_exit": r.returncode, "caught_at_runtime": True})
    for row in baseline["files"]:
        assert sha(read(A1 / row["path"])) == row["original_sha256"]
    return {"all_pass": True, "changed_files": changed, "mutants": mutants,
            "a1_all68_rechecked": True, "limitations": "single-thread fake USB/kernel model; no actual module/USB/DMA/lifecycle/event qualification"}

result = {"utc_start": utc(), "argv": sys.argv, "python": sys.version,
          "scope": "A2A_PUBLIC_SOURCE_SYNTHETIC_HOST_ONLY_NO_ARM_OR_HARDWARE",
          "runner_sha256": sha(Path(__file__).read_bytes())}
try:
    result.update(main())
except Exception as exc:
    result.update({"all_pass": False, "failure": str(exc)})
    raise
finally:
    result.update({"utc_end": utc(), "inputs": inputs, "commands": commands, "extractions": extractions})
    write(OUT / "result.json", json.dumps(result, indent=2)+"\n")
    outputs = {str(p.relative_to(OUT)): {"bytes": p.stat().st_size, "sha256": sha(p.read_bytes())}
               for p in sorted(OUT.rglob("*")) if p.is_file()}
    write(OUT / "output-hashes.json", json.dumps(outputs, indent=2)+"\n")
print("A2a SOURCE/HOST PASS; full S4/ARM build/native attach remain NO-GO")
