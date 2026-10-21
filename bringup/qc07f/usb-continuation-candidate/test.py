#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile/run actual candidate76 kick helper with exhaustive host write mocks."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import signal
import subprocess
import sys

if not __debug__:
    raise SystemExit("Candidate safety checks require assertions; do not use Python -O")

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
FOUND = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z"
SRC = FOUND / "artifacts/native-usb76-source01/linux"
BUILD = FOUND / "analysis/native-usb76-build01"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=False)
    audit = json.loads((BUILD / "prepare76-manifest.json").read_text())
    path = SRC / "drivers/usb/musb/musb_host.c"
    code = path.read_text()
    assert sha(path) == audit["only_changed_kernel_file"]["drivers/usb/musb/musb_host.c"]
    helper = (HERE / "pio_kick.c.inc").read_text()
    assert code.count(helper) == 1
    assert code.count("musb_h_tx_pio_kick(musb, epio);") == 2
    tx = code[code.index("void musb_host_tx("):code.index("#ifdef CONFIG_USB_TI_CPPI41_DMA", code.index("void musb_host_tx("))]
    assert tx.count("musb_h_tx_pio_kick(musb, epio);") == 2
    nak = tx[tx.index("} else if (tx_csr & MUSB_TXCSR_H_NAKTIMEOUT)"):tx.index("\ndone:")]
    assert nak.count("musb_h_tx_pio_kick(musb, epio);") == 1
    assert tx.rstrip().endswith("musb_h_tx_pio_kick(musb, epio);\n}")
    # Pull exact macro definitions and their dependencies from selected kernel
    # headers rather than maintaining a second hand-entered register vocabulary.
    definitions = {}
    for name in ("musb_regs.h", "musb_core.h"):
        lines = (SRC / "drivers/usb/musb" / name).read_text().splitlines(True)
        i = 0
        while i < len(lines):
            line = lines[i]; i += 1
            m = re.match(r"#define\s+(MUSB_\w+)\s+", line)
            if not m:
                continue
            while line.rstrip().endswith("\\"):
                line += lines[i]; i += 1
            definitions[m[1]] = line
    required = {"MUSB_ANYKA_PIO_TOGGLE", "MUSB_TXCSR", "MUSB_TXCSR_H_WZC_BITS", "MUSB_TXCSR_TXPKTRDY", "MUSB_TXCSR_MODE"}
    while True:
        expanded = required | {x for key in required for x in re.findall(r"\bMUSB_\w+\b", definitions[key])}
        if expanded == required:
            break
        required = expanded
    (out / "register_constants.h").write_text("".join(definitions[key] for key in sorted(required)))
    (out / "actual_helper.inc").write_text(helper)
    records = []
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:abort_on_error=1", UBSAN_OPTIONS="halt_on_error=1")

    def run(name, argv, expected_assertion=None):
        result = subprocess.run(list(map(str, argv)), capture_output=True, env=env, timeout=60)
        (out / (name + ".stdout")).write_bytes(result.stdout)
        (out / (name + ".stderr")).write_bytes(result.stderr)
        records.append({"name": name, "argv": list(map(str, argv)), "exit": result.returncode,
                        "expected_assertion": expected_assertion, "stdout_sha256": hashlib.sha256(result.stdout).hexdigest(),
                        "stderr_sha256": hashlib.sha256(result.stderr).hexdigest()})
        if expected_assertion is not None:
            assert result.returncode == -signal.SIGABRT, name
            assert b"Assertion" in result.stderr and expected_assertion.encode() in result.stderr, name
        else:
            assert result.returncode == 0, name
        return result.stdout

    run("clang-version", ["clang", "--version"])
    cc = ["clang", "-std=c11", "-Wall", "-Wextra", "-Werror"]
    for name, flags in (("optimized", ["-O2"]), ("sanitized", ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"])):
        run(name + "-compile", cc + flags + ["-I" + str(out), HERE / "test_kick.c", "-o", out / name])
        answer = json.loads(run(name + "-run", [out.resolve() / name]))
        assert answer == {"helper_cases": 262144, "hardware_qualified": False}
    mutations = {
        "original-word-kick": helper.replace("musb_writeb(epio, MUSB_TXCSR, MUSB_TXCSR_TXPKTRDY);", "musb_writew(epio, MUSB_TXCSR, MUSB_TXCSR_H_WZC_BITS | MUSB_TXCSR_TXPKTRDY);"),
        "DMA-guard-removed": helper.replace(" && !is_dma_capable()", ""),
        "quirk-inverted": helper.replace("(musb->ops->quirks & MUSB_ANYKA_PIO_TOGGLE)", "!(musb->ops->quirks & MUSB_ANYKA_PIO_TOGGLE)"),
    }
    for name, mutant in mutations.items():
        assert mutant != helper
        directory = out / name; directory.mkdir()
        (directory / "actual_helper.inc").write_text(mutant)
        run(name + "-compile", cc + ["-O2", "-I" + str(directory), "-I" + str(out), HERE / "test_kick.c", "-o", directory / "test"])
        assertion = "byte_writes == 1 && word_writes == 0" if name == "original-word-kick" else "byte_writes == 0 && word_writes == 1"
        run(name + "-run", [directory.resolve() / "test"], expected_assertion=assertion)
    manifest = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "state": "HOST_HELPER_PASS_NOT_HARDWARE",
                "optimized_cases": 262144, "sanitized_cases": 262144, "negative_mutants_rejected": len(mutations),
                "mutants_require_SIGABRT_and_exact_assertion": True,
                "both_real_call_sites_checked": True, "DMA_generic_unchanged": True,
                "actual_kernel_file_sha256": sha(path), "helper_sha256": sha(HERE / "pio_kick.c.inc"),
                "runner_sha256": sha(Path(__file__)), "harness_sha256": sha(HERE / "test_kick.c"),
                "extracted_register_header_sha256": sha(out / "register_constants.h"), "commands": records,
                "target_operations": False, "MMIO_hardware_semantics_proved": False, "MTU1500_qualified": False}
    (out / "result.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({k: manifest[k] for k in ("state", "optimized_cases", "sanitized_cases", "negative_mutants_rejected")}))


if __name__ == "__main__":
    main()
