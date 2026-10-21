#!/usr/bin/env python3
"""A2b exclusive public synthetic HOST runner. Never imports/runs old collectors."""
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
OLD = ROOT / "apps/qualification/kernel76-station-ingress"
DOC = ROOT / "docs/qualification-plans/kernel76-transport-20261021"
E = DOC / "station-event-a2b-host01"
parser = argparse.ArgumentParser()
parser.add_argument("--run", required=True)
args = parser.parse_args()
assert re.fullmatch(r"run[0-9]{2}", args.run)
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
    if p.is_relative_to(HERE) and (p.parent == HERE or (HERE / "tests") in p.parents):
        saved = OUT / "input-snapshots" / p.relative_to(HERE)
        if not saved.exists():
            saved.parent.mkdir(parents=True,exist_ok=True)
            write(saved,b)
    return b
def write(p, b):
    with p.open("xb") as f:
        f.write(b.encode() if isinstance(b, str) else b)
def extract(p, start, end=None, text=None):
    t = read(p).decode() if text is None else text
    if start not in t:
        start = start.replace("\n", "\r\n")
    if end and end not in t:
        end = end.replace("\n", "\r\n")
    assert t.count(start) == 1 and (not end or t.count(end) == 1), (p, start, end)
    a, b = t.index(start), t.index(end) if end else len(t)
    assert a < b
    body = t[a:b]
    extractions.append({"source": str(p.relative_to(ROOT)), "start_marker": start,
                        "end_marker": end or "EOF", "start_byte": a, "bytes": b-a,
                        "start_line": t.count("\n", 0, a)+1, "sha256": sha(body.encode()),
                        "mutated": text is not None})
    return body
def command(label, argv, expected=0):
    start = utc()
    r = subprocess.run([str(x) for x in argv], cwd=ROOT, capture_output=True, timeout=120,
                       env={"PATH": "/usr/bin:/bin", "LC_ALL": "C",
                            "ASAN_OPTIONS": "detect_leaks=1:abort_on_error=0",
                            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
    write(OUT / (label + ".stdout"), r.stdout)
    write(OUT / (label + ".stderr"), r.stderr)
    commands.append({"label": label, "argv": list(map(str, argv)), "cwd": str(ROOT),
                     "utc_start": start, "utc_end": utc(), "timeout_seconds": 120,
                     "exit": r.returncode})
    if expected is not None and r.returncode != expected:
        raise RuntimeError(f"{label}: exit {r.returncode}, expected {expected}")
    return r
def main():
    baseline = json.loads(read(E / "copy-baseline.json"))
    assert baseline["count"] == 68
    changed = []
    for row in baseline["files"]:
        rel = row["path"]
        a, b = read(OLD / "source" / rel), read(SOURCE / rel)
        assert sha(a) == row["original_sha256"] == row["copied_sha256"] and len(a) == row["bytes"]
        if a != b:
            changed.append(rel)
            write(OUT / (rel.replace("/", "_") + ".diff"), "".join(difflib.unified_diff(
                a.decode().splitlines(keepends=True), b.decode().splitlines(keepends=True),
                fromfile="accepted-A2a/"+rel, tofile="A2b/"+rel)))
    assert changed == ["hgic_smac/event.c", "hgic_smac/rx.c"]
    assert {str(p.relative_to(SOURCE)) for p in SOURCE.rglob("*") if p.is_file()} == {r["path"] for r in baseline["files"]}
    for row in baseline["copied_tests"]:
        b = read(HERE / "tests" / row["path"])
        assert len(b) == row["bytes"] and sha(b) == row["sha256"]
    for p in sorted((HERE / "tests").glob("*")):
        read(p)
    read(Path(__file__))
    read(HERE / "prepare_source.py")
    core = read(SOURCE / "hgic_smac/core.c").decode()
    binding = "hg->ctrl.rx_event = hgics_rx_fw_event;"
    assert core.count(binding) == 1
    write(OUT / "binding.json", json.dumps({"actual_core_callback_binding": binding,
          "line": core[:core.index(binding)].count("\n")+1, "real_consumer_used": True}, indent=2))
    inc = OUT / "include"
    (inc / "linux").mkdir(parents=True)
    for n in ("completion.h", "workqueue.h", "unaligned.h", "etherdevice.h", "build_bug.h"):
        write(inc / "linux" / n, '#include "shim.h"\n')
    write(inc / "linux/errno.h", '#include_next <linux/errno.h>\n')
    defs = read(SOURCE / "hgic_def.h").decode()
    constants = []
    for n in ("HGIC_WDEV_ID_STA", "HGIC_HDR_TX_MAGIC", "HGIC_HDR_RX_MAGIC", "HGIC_WLAN_8400",
              "HGIC_TX_COOKIE_MASK", "HGIC_CMD_TIMEOUT", "HGIC_BLOCK_ACK_CNT"):
        matches = re.findall(r"^#define " + n + r"\s+[^\r\n]+", defs, re.M)
        assert len(matches) == 1
        constants += matches
    definitions = "\n".join(constants)+"\n"
    bus = extract(SOURCE / "hgic_def.h", "struct hgic_bus {", "#ifdef __RTOS__\n#define ALLOC_ORDERED_WORKQUEUE")
    ack = extract(SOURCE / "hgic_def.h", "struct hgic_dack_hdr {", "struct hgic_nlmsg_hdr {")
    state = extract(SOURCE / "hgic_smac/hgics.h", "enum hgics_state {", "enum hgics_vif_state {")
    txstatus = extract(SOURCE / "hgic_smac/hgics.h", "struct hgics_txstatus {", "int hgics_rx_data(")
    def base(events=False, ingress=True):
        return ('#include "'+("event_shim.h" if events else "ingress_shim.h" if ingress else "shim.h")+'"\n'+
                '#include "'+str(SOURCE / "hgic.h")+'"\n'+definitions+(bus if ingress else "")+
                '#include "'+str(SOURCE / "utils/fwctrl.h")+'"\n'+
                '#include "'+str(SOURCE / "utils/control_receipt.h")+'"\n')
    def fwunit():
        p = SOURCE / "utils/fwctrl.c"
        return extract(p, "#define STR_LEN", "void hgic_fwctrl_init(")+extract(
            p, "int hgic_fwctrl_get_fwinfo(", "int hgic_fwctrl_set_countryregion(")
    def unit(events=False, original=False, overrides=None):
        overrides = overrides or {}
        code = base(events) + fwunit() + ack + state + txstatus
        code += '''
struct hgics_wdev {
    struct hgic_bus *bus;
    struct hgic_fwctrl ctrl;
    unsigned long state, last_rx;
    int if_test, test_rx_len, txwnd;
    struct completion txwnd_cp;
    struct sk_buff_head evt_list;
    struct semaphore evt_sema;
};
static void hgics_rx_ack(struct hgics_wdev *h, struct hgics_txstatus *s) { (void)h; (void)s; data_calls++; }
static void hgics_rx_agg_frm(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
static void hgics_rx_sigfrm(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
static void hgics_rx_bt_data(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
'''
        if events:
            p = (OLD if original else HERE) / "source/hgic_smac/event.c"
            start = "#define HGIC_EVENT_MAX" if original else "static_assert(sizeof(struct hgic_hdr)"
            code += extract(p, start, text=overrides.get("event"))
            if original:
                code += "\n#define ORIGINAL_EVENT_OBSERVATION 1\n"
        code += extract(SOURCE / "hgic_smac/rx.c", "void hgics_rx_error(", text=overrides.get("rx"))
        p = SOURCE / "utils/if_usb.c"
        code += extract(p, "#define HGIC_PKT_MAX_LEN", "static int txq_cnt")
        code += '''
static u32 max_pkt_len = HGIC_PKT_MAX_LEN;
static void hgic_usb_receive(struct urb *urb);
static void hgic_usb_cancle(struct hgic_usb *d) { (void)d; cancel_calls++; }
'''
        code += extract(p, "static void hgic_usb_enq(", "static int hgic_usb_submit_rx_urbs(")
        code += extract(p, "static void hgic_usb_receive(struct urb *urb)\n{", "static void hgic_usb_tx_complete(")
        code += '#include "'+str(HERE / ("tests/test_events.c" if events else "tests/test_ingress.c"))+'"\n'
        return code
    command("compiler-version", ["cc", "--version"])
    command("compiler-target", ["cc", "-dumpmachine"])
    common = ["cc", "-std=gnu11", "-g", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
              "-Wno-unused-parameter", "-Wno-sign-compare", "-I"+str(inc), "-I"+str(HERE / "tests")]
    modes = [("optimized", ["-O2"]), ("sanitized", ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"])]
    for name, code in (("events", unit(True)), ("original-event-hooks", unit(True, True)),
                       ("a2a-regression", unit()),
                       ("a1-regression", base(False,False)+fwunit()+'\n#include "'+str(HERE / "tests/test_control.c")+'"\n')):
        c = OUT / (name+".c")
        write(c, code)
        for label, flags in modes:
            binary = OUT / (name+"-"+label+".host")
            baseline_warning = ["-Wno-unused-but-set-variable"] if name == "original-event-hooks" else []
            command(name+"-"+label+"-compile", common+flags+baseline_warning+[c,"-o",binary])
            command(name+"-"+label+"-test", [binary])
    event = read(SOURCE / "hgic_smac/event.c").decode()
    rx = read(SOURCE / "hgic_smac/rx.c").decode()
    mutations = [
        ("restore-payload-format", "event", "    goto dispose;\n\ndeny:", "    printk((char *)skb->data + 12);\n    goto dispose;\n\ndeny:"),
        ("bounded-string-log", "event", "    goto dispose;\n\ndeny:", '    printk("%.*s", (int)skb->len - 12, skb->data + 12);\n    goto dispose;\n\ndeny:'),
        ("missing-event-pretrim-equality", "rx", "declared != actual", "(hdr->hdr.type != HGIC_HDR_TYPE_EVENT && hdr->hdr.type != HGIC_HDR_TYPE_EVENT2 && declared != actual)"),
        ("missing-event-pretrim-minimum", "rx", "len < sizeof(struct hgic_ctrl_hdr)", "(hdr->hdr.type != HGIC_HDR_TYPE_EVENT && hdr->hdr.type != HGIC_HDR_TYPE_EVENT2 && len < sizeof(struct hgic_ctrl_hdr))"),
        ("missing-consumer-type", "event", "if (skb->data[2] != HGIC_HDR_TYPE_EVENT &&\n        skb->data[2] != HGIC_HDR_TYPE_EVENT2)", "if (false)"),
        ("truncate-event2-id", "event", "get_unaligned_le16(skb->data + offsetof(struct hgic_ctrl_hdr, event2.event_id))", "(u8)get_unaligned_le16(skb->data + offsetof(struct hgic_ctrl_hdr, event2.event_id))"),
        ("restore-unknown-queue", "event", "if (evt_id != HGIC_EVENT_FWDBG_INFO)\n        goto deny;", "if (evt_id != HGIC_EVENT_FWDBG_INFO) {\n        struct hgics_wdev *hg = container_of(ctrl, struct hgics_wdev, ctrl);\n        skb_queue_tail(&hg->evt_list, skb); up(&hg->evt_sema); return;\n    }"),
        ("bypass-persistent-denial", "event", "if (READ_ONCE(ctrl->receipt_error) || READ_ONCE(ctrl->bus->rx_failed))\n        goto dispose;", "if (false) goto dispose;\n    ctrl->receipt_error = 0; ctrl->bus->rx_failed = 0;"),
        ("event-success-completion", "event", "    goto dispose;\n\ndeny:", "    { struct hgic_cmd_response *r;\n      list_for_each_entry(r, &ctrl->pd_list, list) { r->done = true; complete(&r->cmpl); } }\n    goto dispose;\n\ndeny:"),
    ]
    mutants = []
    for name, where, old, new in mutations:
        t = event if where == "event" else rx
        assert t.count(old) == 1, name
        code = unit(True, overrides={where: t.replace(old,new)})
        c = OUT / (name+".c")
        write(c,code)
        binary = OUT / (name+".host")
        command(name+"-compile", common+modes[1][1]+[c,"-o",binary])
        r = command(name+"-test",[binary],expected=None)
        assert r.returncode and (b"FAIL line" in r.stderr or b"Sanitizer" in r.stderr), name
        mutants.append({"name":name,"compile_exit":0,"runtime_exit":r.returncode,"caught_at_runtime":True})
    # Every original seal remains immutable, including original tests and tools.
    for name in ("station-control-a1-host01/candidate-seal.json", "station-ingress-a2a-host01/candidate-seal.json"):
        s = json.loads(read(DOC / name))
        for rel, pin in s["files"].items():
            b = read(ROOT / rel)
            assert {"bytes":len(b),"sha256":sha(b)} == pin, rel
    return {"all_pass":True,"changed_files":changed,"mutants":mutants,
            "a1_a2a_seals_rechecked":True,"real_core_event_binding_verified":True}
result = {"utc_start":utc(),"argv":sys.argv,"python":sys.version,
          "scope":"A2B_PUBLIC_SYNTHETIC_HOST_ONLY_NO_ARM_NO_HARDWARE",
          "runner_sha256":sha(Path(__file__).read_bytes())}
try:
    result.update(main())
except Exception as exc:
    result.update({"all_pass":False,"failure":str(exc)})
    raise
finally:
    result.update({"utc_end":utc(),"inputs":inputs,"commands":commands,"extractions":extractions})
    write(OUT / "result.json",json.dumps(result,indent=2)+"\n")
    outputs = {str(p.relative_to(OUT)):{"bytes":p.stat().st_size,"sha256":sha(p.read_bytes())}
               for p in sorted(OUT.rglob("*")) if p.is_file()}
    write(OUT / "output-hashes.json",json.dumps(outputs,indent=2)+"\n")
print("A2b SOURCE/HOST PASS; independent review required; full S4/ARM/native NO-GO")
