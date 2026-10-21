#!/usr/bin/env python3
"""Run actual AST-isolated postcheck with in-memory files/transport, never RPC."""
import ast
import datetime
import hashlib
import io
import json
from pathlib import Path
import re
import sys
import types

if not __debug__:
    raise SystemExit("Assertions required")
HERE = Path(__file__).resolve().parent
SOURCE = HERE / "postcheck.py"
UUID = "01234567-89ab-4cde-8f01-23456789abcd"
SHA = "bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f"
CMDLINE = "earlycon console=ttyAK0,115200n8 rdinit=/init loglevel=8 panic=0 cma=16M@0x82800000-0x83800000"


def baseline():
    return {"pid1": "init", "release": "6.18.55-qc07f-prototype-dirty",
            "identity": "Name:\tinit\nPid:\t1\nUid:\t0\t0\t0\t0\nCapEff:\t0000000000020000\n",
            "cmdline": CMDLINE, "modules": "",
            "mounts": "rootfs / rootfs rw 0 0\ndevtmpfs /dev devtmpfs rw 0 0\nproc /proc proc rw 0 0\nsysfs /sys sysfs rw 0 0\n",
            "memory": "MemTotal: 35000 kB\nMemFree: 12000 kB\nCmaTotal: 16384 kB\nCmaFree: 12000 kB\n",
            "iomem": "81800000-83ffffff : System RAM\n",
            "ISP": "result=-61 readable=0 guard_errors=0 irq_mode=0 irq_frames=0 queue_mode=0 queue_started=0 queue_held=0 wb_owner_active=0\n",
            "watchdog": "inactive", "nowayout": "0",
            "UART": "rx_error_events=0 mem_busy_ticks=0 last_status=00080000 rxpos=12; cached diagnostic, lost bytes cannot be reconstructed",
            "boot_id": UUID, "boot_id_after": UUID}


class MemoryFile(io.StringIO):
    def __exit__(self, *args): return False
    def fileno(self): return 123


def simulate(tree, fault=None, point=0, late_log=False):
    files = {}; instances = []; logs = []; rows = baseline()

    class MemoryPath:
        def __init__(self, value): self.value = str(value)
        def __truediv__(self, value): return MemoryPath(self.value + "/" + str(value))
        @property
        def parent(self): return MemoryPath(self.value.rsplit("/", 1)[0])
        def read_text(self): return files[self.value].getvalue()
        def read_bytes(self): return self.read_text().encode()
        def open(self, mode):
            if mode == "r+":
                files[self.value].seek(0); return files[self.value]
            if mode == "x" and self.value in files: raise FileExistsError(self.value)
            files[self.value] = MemoryFile(); files[self.value].name = self.value
            return files[self.value]

    checkpoint = {"part": "foundation-20261021a", "image_sha256": SHA, "recorder_pid": 123,
                  "start_ticks": 42, "rx_bytes": 50, "tx_bytes": 100,
                  "monitor_observed": True, "shell_started": False}
    files["recipe/RAM76_BOOT21A01.json"] = MemoryFile(json.dumps({"state": "RAM76_BOOT_AND_INERT_MONITOR_OBSERVED", "image_sha256": SHA}))
    files["base/boot-checkpoint21a01.private.json"] = MemoryFile(json.dumps(checkpoint))
    files["control/peripheral-gui-operation.lock"] = MemoryFile("FOREIGN" if fault == "foreign_HOLD" else "")
    files["recipe/postcheck.py"] = MemoryFile("public synthetic source marker")
    if fault == "missing_receipt": del files["recipe/RAM76_BOOT21A01.json"]
    if fault == "missing_checkpoint": del files["base/boot-checkpoint21a01.private.json"]

    class FakeInstance:
        def __init__(self, *args):
            if fault == "constructor": raise OSError("synthetic")
            self.peer = (124 if fault == "owner_PID" else 123, 1000, 1000)
            self.tx = 101 if fault == "entry_TX" else 100
            self.rx = 49 if fault == "RX_regression" else 50
            self.checks = self.sends = self.waits = 0; self.command = ""; instances.append(self)
        def state(self):
            return {"part": "foundation-20261021a", "tx_busy": False, "rx_bytes": self.rx,
                    "tx_bytes": self.tx, "recent_rx": "qc07f> USB late\n" if late_log and not self.sends else "qc07f> "}
        def owner(self, **kwargs):
            if fault == "owner_exception": raise RuntimeError("synthetic")
            return self.state()
        def raw_since(self, *args):
            return b"U-Boot 2013.10.0-AK_V2.0.03" if fault == "reset" else b""
        def check_stream(self):
            self.checks += 1
            if fault == "check_TX" and self.checks == point: self.tx += 5
            return self.state()
        def send(self, data, reason):
            self.sends += 1; self.tx += len(data); self.command = data.rstrip(b"\r").decode()
            assert len(data) <= 64 and (not self.command or self.command in ns["QUERIES"].values())
            if fault == "send_TX" and self.sends == point: self.tx += 5
        def wait(self, *args, **kwargs):
            self.waits += 1
            if self.command:
                key = next(k for k, value in ns["QUERIES"].items() if value == self.command)
                answer = (rows[key] + "\r\nqc07f> ").encode()
            else: answer = b"\r\nqc07f> "
            self.rx += len(answer)
            if fault == "wait_TX" and self.waits == point: self.tx += 5
            return answer
        def log(self, event, **kwargs): logs.append(event)

    ns = {"__file__": "recipe/postcheck.py", "Path": MemoryPath, "BASE": MemoryPath("base"),
          "SHA": SHA, "PROMPT": b"qc07f> ", "json": json, "re": re, "datetime": datetime,
          "hashlib": hashlib, "print": lambda *a, **kw: None,
          "sys": types.SimpleNamespace(argv=["postcheck.py", "--owned-RAM76-boot-completed"]),
          "private": types.SimpleNamespace(private_path=lambda value: value, PrivateBoot=FakeInstance),
          "b": types.SimpleNamespace(CONTROL=MemoryPath("control"), ticks=lambda pid: 43 if fault == "start_ticks" else 42, BANNER=b"U-Boot 2013.10.0-AK_V2.0.03"),
          "fcntl": types.SimpleNamespace(LOCK_EX=1, LOCK_NB=2, flock=lambda *a: None),
          "os": types.SimpleNamespace(umask=lambda *a: None, fsync=lambda *a: None, chmod=lambda *a: None)}
    nodes = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in {"body", "parse", "checked_state", "exchange", "main"}]
    nodes += [n for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id in {"QUERIES", "BOOTARGS"} for t in n.targets)]
    exec(compile(ast.Module(body=nodes, type_ignores=[]), "in-memory-actual-postcheck", "exec"), ns)
    failure = None
    try: ns["main"]()
    except (AssertionError, OSError, KeyError, RuntimeError) as e: failure = type(e).__name__
    return {"failure": failure, "HOLD": files["control/peripheral-gui-operation.lock"].getvalue(),
            "PASS": "recipe/RAM76_POSTCHECK21A01.json" in files,
            "checks": instances[0].checks if instances else 0,
            "sends": instances[0].sends if instances else 0}


def main():
    data = SOURCE.read_bytes(); tree = ast.parse(data); results = []
    for late in (False, True):
        answer = simulate(tree, late_log=late)
        assert answer["failure"] is None and answer["PASS"] and not answer["HOLD"]
        results.append({"name": "healthy_refresh" if late else "healthy", "pass": True})
        for point in range(1, answer["checks"] + 1):
            fail = simulate(tree, "check_TX", point, late)
            assert fail["failure"] and not fail["PASS"] and fail["HOLD"]
            results.append({"name": f"foreign_TX_check_{late}_{point}", "pass": True})
        for stage in ("send_TX", "wait_TX"):
            for point in range(1, answer["sends"] + 1):
                fail = simulate(tree, stage, point, late)
                assert fail["failure"] and not fail["PASS"] and fail["HOLD"]
                results.append({"name": f"foreign_{stage}_{late}_{point}", "pass": True})
    for fault in ("missing_receipt", "missing_checkpoint", "constructor", "owner_exception", "entry_TX", "owner_PID", "start_ticks", "RX_regression", "reset"):
        answer = simulate(tree, fault)
        assert answer["failure"] and not answer["PASS"] and answer["HOLD"] and answer["sends"] == 0
        results.append({"name": fault + "_sticky_pre_TX", "pass": True})
    answer = simulate(tree, "foreign_HOLD")
    assert answer["failure"] and not answer["PASS"] and answer["HOLD"] == "FOREIGN" and answer["sends"] == 0
    results.append({"name": "foreign_HOLD_preserved", "pass": True})
    ns = {"re": re, "PROMPT": b"qc07f> ", "BOOTARGS": CMDLINE}
    functions = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in {"body", "parse"}]
    exec(compile(ast.Module(body=functions, type_ignores=[]), "isolated-actual-parsers", "exec"), ns)
    assert ns["parse"](baseline())["CmaTotal"] == 16384
    results.append({"name": "baseline_schema", "pass": True})
    mutants = [("mounts", ""), ("mounts", "/dev/root / ext4 rw 0 0\n"),
               ("cmdline", "not_rdinit=/init not_cma=16M@0x82800000-0x83800000"),
               ("cmdline", CMDLINE + " cma=1M"), ("ISP", baseline()["ISP"] + "queue_started=1\n"),
               ("ISP", baseline()["ISP"] + "queue_started=0\n"),
               ("memory", baseline()["memory"] + "CmaTotal: 16384 kB\n"),
               ("watchdog", "active"), ("nowayout", "1"), ("modules", "hgics 1 0 - Live 1"),
               ("boot_id_after", "ffffffff-ffff-4fff-8fff-ffffffffffff"),
               ("UART", baseline()["UART"].replace("rx_error_events=0", "rx_error_events=1")),
               ("UART", baseline()["UART"].replace("mem_busy_ticks=0", "mem_busy_ticks=1"))]
    for index, (key, value) in enumerate(mutants):
        rows = baseline(); rows[key] = value
        try: ns["parse"](rows)
        except AssertionError: pass
        else: raise AssertionError("parser mutant accepted: " + key)
        results.append({"name": f"parser_mutant_{index}_{key}", "pass": True})
    for response in (b"echo only", b"qc07f> qc07f> ", b"Open failed.\r\nqc07f> "):
        try: ns["body"](response, "mem")
        except AssertionError: pass
        else: raise AssertionError("framing accepted")
        results.append({"name": "framing_refusal", "pass": True})
    output = Path(sys.argv[1]); output.mkdir(parents=True, exist_ok=False)
    result = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "cases": len(results),
              "all_pass": True, "checks": results, "source_sha256": hashlib.sha256(data).hexdigest(),
              "test_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "AST_isolated_all_capabilities_faked": True, "UART_RPC_device_or_private_capture_access": False}
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"cases": len(results), "all_pass": True, "target_access": False}))


if __name__ == "__main__": main()
