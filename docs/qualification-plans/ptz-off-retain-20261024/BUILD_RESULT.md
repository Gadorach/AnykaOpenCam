# ONE real ARM object compile — PASS, NOT RUN

2026-10-24. Candidate235 lines /7551 bytes, SHA256
`a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`.
Object8580 bytes, SHA256
`6d06084ca2908a580764787df6b977b85315343b08a074f90d6f0fda1c0b1ae4`.

Three actual successful external commands, in order:

1. `/usr/bin/clang --version` — NEW23.1.3, identical to prior counter version log.
2. ONE `/usr/bin/clang ... -c -o <fresh>/ptz.o <fresh-source>/ptz.c` — exit0,
   compile.txt empty, no warnings or repairs.
3. `/usr/bin/readelf -h -A <fresh>/ptz.o` — exit0; ELF32 little-endian ARM REL,
   EABI5, ARMv5TE, ARM ISA / Thumb-1 attributes; no program headers.

Full actual argv/cwd/timestamps/tools/log and artifact pins and all394 emitted
source/header dependency pins are in `runs/compile01/REPORT.json`. All three
logs and actual object/dependency file are retained there. No second attempt.
The object is not executed and is not a module, kernel, image or native result.

## Fixed argv and genuine inputs

The saved prior recipe TEXT is the commands[1] argv in
`../ptz-counter-20261023/runs/compile01/REPORT.json`. The actual earlier top-level
Kbuild `.ptz.o.cmd` was also read/pinned, not invoked. A one-shot inline Python
metadata/orchestration command read the prior JSON and copied its argv. EXACTLY
three path operands changed: -MF dependency output, -o object output, final actual
source input. Each before/after/index is saved in the fresh report. No include,
target, warning/Werror, codegen, define, optimization or ABI flag changed. In
particular the previous -MD/-MF dependency emission is preserved. No builder,
test/helper script, Kbuild, make, stub, fabricated header or sysroot was invoked.
The new source was compiled directly, not an included facade over the old driver.

Cwd remains the real prepared query77 build tree at
`work/lightbulb-camera/foundation-20261011T041148Z/analysis/encoder-query77-build02/build`.
All kernel/generated headers are the genuine retained query77 inputs. Their
.config identifies Clang23.1.3, PTZ=y, MODULES=y, MODULE_UNLOAD=y, FORCE_UNLOAD=n.
No MODULE define was added: this is the same built-in object scope as counter,
not modular branch compile/ABI/runtime qualification. No old75 ABI reproduction,
paired module link or release eligibility is asserted.

Before compiling,421 unique original/candidate/reference/tool inputs matched
their recorded hashes (including all prior counter consumed inputs/dependencies);
the same421 matched after compilation. Metadata saved the exact literal forward
and inverse patches and applied the literal inverse in memory to the one real
candidate: reconstruction equals the immutable accepted counter baseline.
The four-line counter refusal and probe/tail were checked byte-for-byte. This
was actual-artifact hashing/comparison, not a normal/negative/adversarial test,
candidate function invocation or simulated GPIO/provider observation.

Compiler resolved path `/usr/bin/clang-23` remains SHA256
`583017067c40b0ff0a9b35805d8ac89d5a66fb014221c78ae13a7c21997dd464`.
The actual tool-file pins are in the fresh report; no tool installation occurred.
Original GPL-2.0-only source SPDX and MODULE_LICENSE("GPL") remain unchanged;
kernel COPYING and preferred GPL-2.0 texts are pinned, not relicensed.

Scope is NEW coherent6.18.55/query77/Clang23.1.3 changed-driver object evidence
ONLY. No tests/fixtures/mutation/fuzz, function/helper/native execution, hardware,
GPIO/motor/UART/network/outlet/RPC, image/source baseline, Git/index or private
evidence operation occurred. Independent review and parent disposition are still
required; STOP/displacement/homing/provider-unbind/recovery gates stay OPEN.
