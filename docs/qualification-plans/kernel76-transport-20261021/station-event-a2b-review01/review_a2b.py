#!/usr/bin/env python3
"""Independent exact-source HOST review. Never invokes/imports an author runner."""
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
ROOT = HERE.parents[3]
DOC = HERE.parent
APP = ROOT / "apps/qualification/kernel76-station-events"
SRC = APP / "source"
OLD = ROOT / "apps/qualification/kernel76-station-ingress"
AUTHOR = DOC / "station-event-a2b-host01"
parser = argparse.ArgumentParser()
parser.add_argument("--run", required=True)
args = parser.parse_args()
assert re.fullmatch(r"run[0-9]{2}", args.run)
OUT = HERE / args.run
OUT.mkdir(exist_ok=False)
inputs, commands, extents = {}, [], []


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def pin(b):
    return {"bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}


def read(p):
    p = Path(p)
    assert p.is_relative_to(ROOT) and not p.is_symlink(), p
    rel = str(p.relative_to(ROOT))
    assert rel.startswith(("apps/qualification/kernel76-station/",
                           "apps/qualification/kernel76-station-ingress/",
                           "apps/qualification/kernel76-station-events/",
                           "docs/qualification-plans/kernel76-transport-20261021/"))
    b = p.read_bytes()
    inputs[rel] = pin(b)
    return b


def write(p, b):
    with p.open("xb") as stream:
        stream.write(b.encode() if isinstance(b, str) else b)


def save(name, value):
    write(OUT / name, json.dumps(value, indent=2) + "\n")


def extract(rel, start, end=None, replacement=None, base=SRC):
    path = base / rel
    data = read(path) if replacement is None else replacement.encode()
    a, z = start.encode(), end.encode() if end else None
    if a not in data:
        a = a.replace(b"\n", b"\r\n")
    if z and z not in data:
        z = z.replace(b"\n", b"\r\n")
    assert data.count(a) == 1 and (not z or data.count(z) == 1), (path, start, end)
    lo = data.index(a); hi = data.index(z) if z else len(data)
    assert lo < hi
    segment = data[lo:hi]
    extents.append({"source": str(path.relative_to(ROOT)), "start_byte": lo,
                    "bytes": len(segment), "start_line": data[:lo].count(b"\n") + 1,
                    "sha256": pin(segment)["sha256"], "mutated": replacement is not None})
    return segment.decode()


def command(label, argv, expected=0):
    start = utc()
    completed = subprocess.run(list(map(str, argv)), cwd=ROOT, capture_output=True,
        timeout=120, env={"PATH": "/usr/bin:/bin", "LC_ALL": "C",
                         "ASAN_OPTIONS": "detect_leaks=1:abort_on_error=0",
                         "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
    write(OUT / (label + ".stdout"), completed.stdout)
    write(OUT / (label + ".stderr"), completed.stderr)
    commands.append({"label": label, "argv": list(map(str, argv)), "cwd": str(ROOT),
                     "utc_start": start, "utc_end": utc(), "timeout_seconds": 120,
                     "exit": completed.returncode})
    if expected is not None:
        assert completed.returncode == expected, (label, completed.returncode)
        assert not completed.stderr, (label, "nonempty stderr")
    return completed


def verify_seal(path):
    data = json.loads(read(path))
    for rel, expected in data["files"].items():
        assert pin(read(ROOT / rel)) == expected, rel
    return data


def main():
    sealpath = AUTHOR / "candidate-seal.json"
    assert pin(read(sealpath))["sha256"] == "63aa6d567f8218b9dcf7c413b9a9bbe56fca742a5ca532729c8a4325ed8bf3cc"
    seal = verify_seal(sealpath)
    for name in ("station-control-a1-host01/candidate-seal.json",
                 "station-ingress-a2a-host01/candidate-seal.json",
                 "station-ingress-a2a-review01/review-seal.json"):
        verify_seal(DOC / name)
    for name in ("STATION76_CONTROL_A1_REVIEW.md", "STATION76_INGRESS_A2A_REVIEW.md",
                 "STATION76_EVENT_ENTRY_REVIEW.md", "STATION76_EVENT_A2B_CANDIDATE.md"):
        read(DOC / name)
    author = json.loads(read(AUTHOR / "run08/result.json"))
    assert author["all_pass"] and len(author["extractions"]) == 96
    for rel, expected in author["inputs"].items():
        assert pin(read(ROOT / rel)) == expected, rel
    for rel, expected in json.loads(read(AUTHOR / "run08/output-hashes.json")).items():
        assert pin(read(AUTHOR / "run08" / rel)) == expected, rel
    for x in author["extractions"]:
        if not x["mutated"]:
            b = read(ROOT / x["source"])
            segment = b[x["start_byte"]:x["start_byte"] + x["bytes"]]
            assert pin(segment)["sha256"] == x["sha256"]
            assert b[:x["start_byte"]].count(b"\n") + 1 == x["start_line"]
            assert segment.startswith(x["start_marker"].encode())
    baseline = json.loads(read(AUTHOR / "copy-baseline.json"))
    rows, changed = [], []
    expected_paths = {row["path"] for row in baseline["files"]}
    assert len(expected_paths) == 68
    for base in (SRC, OLD / "source"):
        assert {str(p.relative_to(base)) for p in base.rglob("*") if p.is_file()} == expected_paths
    for row in baseline["files"]:
        rel = row["path"]
        before, after = read(OLD / "source" / rel), read(SRC / rel)
        assert pin(before) == {"bytes": row["bytes"], "sha256": row["original_sha256"]}
        assert row["original_sha256"] == row["copied_sha256"]
        if before != after:
            changed.append(rel)
            write(OUT / (rel.replace("/", "_") + ".diff"), "".join(difflib.unified_diff(
                before.decode().splitlines(keepends=True), after.decode().splitlines(keepends=True),
                fromfile="accepted-A2a/" + rel, tofile="reviewed-A2b/" + rel)))
        rows.append({"path": rel, "A2a": pin(before), "A2b": pin(after)})
    assert changed == ["hgic_smac/event.c", "hgic_smac/rx.c"]
    assert pin(read(SRC / "hgic_smac/event.c"))["sha256"] == "b00e6ee84c10f676c2ad7d3afe8abb0800d37224b98445d86acdddde774a6345"
    assert pin(read(SRC / "hgic_smac/rx.c"))["sha256"] == "600c2bccc48fae9ee5642fa320d83541b261bf24ff8d2d492c7871f6b97e2cf0"
    for row in seal["source_extents"]:
        assert row["offset"] == 0
        assert pin(read(SRC / row["path"])) == {k: row[k] for k in ("bytes", "sha256")}
    for name in ("shim.h", "ingress_shim.h", "test_ingress.c", "test_control.c"):
        assert read(APP / "tests" / name) == read(OLD / "tests" / name)
    for p in sorted((APP / "tests").glob("*")):
        read(p)
    core = read(SRC / "hgic_smac/core.c").decode()
    assert core.count("hg->ctrl.rx_event = hgics_rx_fw_event;") == 1
    calls = set(re.findall(r"\b([A-Za-z_]\w*)\s*\(", re.sub(r"/\*.*?\*/", "",
        read(SRC / "hgic_smac/event.c").decode().split("void hgics_rx_fw_event", 1)[1], flags=re.S)))
    assert calls == {"if", "READ_ONCE", "skb_is_nonlinear", "hgic_ctrl_frame_valid",
                     "get_unaligned_le16", "offsetof", "hgic_fwctrl_rx_error", "dev_kfree_skb"}
    history = []
    for i in range(1, 9):
        value = json.loads(read(AUTHOR / f"run{i:02}/result.json"))
        history.append({"run": f"run{i:02}", "all_pass": value["all_pass"],
                        "failure": value.get("failure"), "commands": value["commands"]})
    save("provenance.json", {"source_pairs": rows, "changed": changed,
        "seal_entries_checked": len(seal["files"]), "author_run08_inputs": len(author["inputs"]),
        "author_extractions": len(author["extractions"]), "consumer_closed_calls": sorted(calls),
        "author_history": history, "A1_A2a_tests_byte_exact": True})
    write(OUT / "SOURCE_LICENSE_GPL-2.0.txt", read(SRC / "LICENSE"))
    inc = OUT / "include"
    (inc / "linux").mkdir(parents=True)
    for name in ("completion.h", "workqueue.h", "unaligned.h", "etherdevice.h", "build_bug.h"):
        write(inc / "linux" / name, '#include "shim.h"\n')
    write(inc / "linux/errno.h", '#include_next <linux/errno.h>\n')
    definitions = read(SRC / "hgic_def.h").decode()
    constants = []
    for name in ("HGIC_WDEV_ID_STA", "HGIC_HDR_TX_MAGIC", "HGIC_HDR_RX_MAGIC", "HGIC_WLAN_8400",
                 "HGIC_TX_COOKIE_MASK", "HGIC_CMD_TIMEOUT", "HGIC_BLOCK_ACK_CNT"):
        found = re.findall(r"^#define " + name + r"\s+[^\r\n]+", definitions, re.M)
        assert len(found) == 1
        constants.extend(found)
    defines = "\n".join(constants) + "\n"
    bus = extract("hgic_def.h", "struct hgic_bus {", "#ifdef __RTOS__\n#define ALLOC_ORDERED_WORKQUEUE")
    ack = extract("hgic_def.h", "struct hgic_dack_hdr {", "struct hgic_nlmsg_hdr {")
    state = extract("hgic_smac/hgics.h", "enum hgics_state {", "enum hgics_vif_state {")
    status = extract("hgic_smac/hgics.h", "struct hgics_txstatus {", "int hgics_rx_data(")

    def prefix(event=False, ingress=True):
        shim = "event_shim.h" if event else "ingress_shim.h" if ingress else "shim.h"
        return ('#include "' + shim + '"\n#include "' + str(SRC / "hgic.h") + '"\n' +
                defines + (bus if ingress else "") + '#include "' + str(SRC / "utils/fwctrl.h") +
                '"\n#include "' + str(SRC / "utils/control_receipt.h") + '"\n')

    def fw():
        return extract("utils/fwctrl.c", "#define STR_LEN", "void hgic_fwctrl_init(") + extract(
            "utils/fwctrl.c", "int hgic_fwctrl_get_fwinfo(", "int hgic_fwctrl_set_countryregion(")

    context = '''
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
    observer = '''
static struct hgic_fwctrl *review_free_ctrl;
static bool review_expect_deny;
static unsigned int review_free_latched;
static void review_ordered_free(struct sk_buff *s) {
    if (review_expect_deny) {
        assert(review_free_ctrl->receipt_error == -EPROTO);
        assert(review_free_ctrl->bus->rx_failed == 1);
        review_free_latched++;
    }
    tracked_free(s);
}
#undef dev_kfree_skb
#define dev_kfree_skb review_ordered_free
'''

    def unit(event=False, original=False, override=None, supplemental=False):
        override = override or {}
        code = prefix(event) + fw() + ack + state + status + context
        if supplemental:
            code += observer
        if event:
            marker = "#define HGIC_EVENT_MAX" if original else "static_assert(sizeof(struct hgic_hdr)"
            code += extract("hgic_smac/event.c", marker, replacement=override.get("event"),
                            base=OLD / "source" if original else SRC)
            if original:
                code += "\n#define ORIGINAL_EVENT_OBSERVATION 1\n"
        code += extract("hgic_smac/rx.c", "void hgics_rx_error(", replacement=override.get("rx"))
        code += extract("utils/if_usb.c", "#define HGIC_PKT_MAX_LEN", "static int txq_cnt")
        code += '''
static u32 max_pkt_len = HGIC_PKT_MAX_LEN;
static void hgic_usb_receive(struct urb *urb);
static void hgic_usb_cancle(struct hgic_usb *d) { (void)d; cancel_calls++; }
'''
        code += extract("utils/if_usb.c", "static void hgic_usb_enq(", "static int hgic_usb_submit_rx_urbs(")
        code += extract("utils/if_usb.c", "static void hgic_usb_receive(struct urb *urb)\n{", "static void hgic_usb_tx_complete(")
        if supplemental:
            code += "#define main author_event_main\n"
        code += '#include "' + str(APP / ("tests/test_events.c" if event else "tests/test_ingress.c")) + '"\n'
        if supplemental:
            code += '#undef main\n#include "' + str(HERE / "supplement.c") + '"\n'
        return code

    command("compiler-version", ["cc", "--version"])
    command("compiler-target", ["cc", "-dumpmachine"])
    common = ["cc", "-std=gnu11", "-g", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
              "-Wno-unused-parameter", "-Wno-sign-compare", "-I" + str(inc), "-I" + str(APP / "tests")]
    modes = [("optimized", ["-O2"]), ("sanitized", ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"])]
    units = [("events", unit(True)), ("original-event-hooks", unit(True, True)),
             ("a2a-regression", unit()), ("a1-regression", prefix(False, False) + fw() +
              '\n#include "' + str(APP / "tests/test_control.c") + '"\n')]
    for name, code in units + [("supplement", unit(True, supplemental=True))]:
        if name != "supplement":
            assert code.encode() == read(AUTHOR / "run08" / (name + ".c")), name
        path = OUT / (name + ".c"); write(path, code)
        for mode, flags in modes:
            binary = OUT / (name + "-" + mode + ".host")
            warning = ["-Wno-unused-but-set-variable"] if name == "original-event-hooks" else []
            command(name + "-" + mode + "-compile", common + flags + warning + [path, "-o", binary])
            command(name + "-" + mode + "-test", [binary])
    # Independently regenerate every advertised mutant, then compare its full unit
    # before compiling a new executable. Never execute any inventoried binary.
    event_text = read(SRC / "hgic_smac/event.c").decode()
    rx_text = read(SRC / "hgic_smac/rx.c").decode()
    changes = [
        ("restore-payload-format", "event", "    goto dispose;\n\ndeny:", "    printk((char *)skb->data + 12);\n    goto dispose;\n\ndeny:"),
        ("bounded-string-log", "event", "    goto dispose;\n\ndeny:", '    printk("%.*s", (int)skb->len - 12, skb->data + 12);\n    goto dispose;\n\ndeny:'),
        ("missing-event-pretrim-equality", "rx", "declared != actual", "(hdr->hdr.type != HGIC_HDR_TYPE_EVENT && hdr->hdr.type != HGIC_HDR_TYPE_EVENT2 && declared != actual)"),
        ("missing-event-pretrim-minimum", "rx", "len < sizeof(struct hgic_ctrl_hdr)", "(hdr->hdr.type != HGIC_HDR_TYPE_EVENT && hdr->hdr.type != HGIC_HDR_TYPE_EVENT2 && len < sizeof(struct hgic_ctrl_hdr))"),
        ("missing-event-pretrim-type", "rx", "hdr->hdr.type == HGIC_HDR_TYPE_EVENT || hdr->hdr.type == HGIC_HDR_TYPE_EVENT2", "false"),
        ("missing-consumer-type", "event", "if (skb->data[2] != HGIC_HDR_TYPE_EVENT &&\n        skb->data[2] != HGIC_HDR_TYPE_EVENT2)", "if (false)"),
        ("truncate-event2-id", "event", "get_unaligned_le16(skb->data + offsetof(struct hgic_ctrl_hdr, event2.event_id))", "(u8)get_unaligned_le16(skb->data + offsetof(struct hgic_ctrl_hdr, event2.event_id))"),
        ("restore-unknown-queue", "event", "if (evt_id != HGIC_EVENT_FWDBG_INFO)\n        goto deny;", "if (evt_id != HGIC_EVENT_FWDBG_INFO) {\n        struct hgics_wdev *hg = container_of(ctrl, struct hgics_wdev, ctrl);\n        skb_queue_tail(&hg->evt_list, skb); up(&hg->evt_sema); return;\n    }"),
        ("bypass-persistent-denial", "event", "if (READ_ONCE(ctrl->receipt_error) || READ_ONCE(ctrl->bus->rx_failed))\n        goto dispose;", "if (false) goto dispose;\n    ctrl->receipt_error = 0; ctrl->bus->rx_failed = 0;"),
        ("event-success-completion", "event", "    goto dispose;\n\ndeny:", "    { struct hgic_cmd_response *r;\n      list_for_each_entry(r, &ctrl->pd_list, list) { r->done = true; complete(&r->cmpl); } }\n    goto dispose;\n\ndeny:"),
    ]
    mutants = []
    for name, where, before, after in changes + [
        ("review-payload-byte-read", "event", "    goto dispose;\n\ndeny:",
         "    if (skb->len > 12) { volatile u8 byte = skb->data[12]; (void)byte; }\n    goto dispose;\n\ndeny:")
    ]:
        text = event_text if where == "event" else rx_text
        assert text.count(before) == 1, name
        is_review = name.startswith("review-")
        code = unit(True, override={where: text.replace(before, after)}, supplemental=is_review)
        if not is_review:
            assert code.encode() == read(AUTHOR / "run08" / (name + ".c")), name
        path = OUT / (name + ".c"); write(path, code)
        binary = OUT / (name + ".host")
        command(name + "-compile", common + modes[1][1] + [path, "-o", binary])
        run = command(name + "-test", [binary], expected=None)
        assert run.returncode != 0 and (b"FAIL line" in run.stderr or b"Sanitizer" in run.stderr), name
        mutants.append({"name": name, "compile_exit": 0, "runtime_exit": run.returncode})
    for path in (Path(__file__), HERE / "supplement.c"):
        read(path)
    for rel, expected in inputs.items():
        assert pin((ROOT / rel).read_bytes()) == expected, rel
    return {"all_pass": True, "source_changed_paths": changed, "mutants": mutants,
            "all_recorded_inputs_rechecked": True, "four_author_units_and_ten_mutants_byte_exact": True}


result = {"utc_start": utc(), "argv": sys.argv, "cwd": str(ROOT), "python": sys.version,
          "scope": "INDEPENDENT_EVENT_SOURCE_HOST_REVIEW_ONLY_NO_ARM_NO_TARGET"}
try:
    result.update(main())
except Exception as error:
    result.update({"all_pass": False, "failure": repr(error)})
    raise
finally:
    result.update({"utc_end": utc(), "inputs": inputs, "commands": commands, "extractions": extents})
    save("result.json", result)
    save("output-hashes.json", {str(p.relative_to(OUT)): pin(p.read_bytes())
                               for p in sorted(OUT.rglob("*")) if p.is_file()})
print("A2b independent SOURCE/HOST review passed; full S4/build/native remain NO-GO")
