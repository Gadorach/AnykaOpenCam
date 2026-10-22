# Author method — ordinary static composition and compile custody only

2026-10-22; RP-08/09, CORE-03/04, STREAM-04/07. This directory and
`apps/rtsp-auth/prestarted-owned-listener/` are the only authored paths. No Git,
ordinary index, central plan, AGENTS, hardware, private or other active lane work.

## Design and source

Read the full selected sibling reports/contracts/reviews/disposition and actual
backend/adapter/server/listener handlers and deltas before writing
`COMPOSITION_CONTRACT.md`. Its pre-code SHA256 is
`07d271826d37aedaf0422878537764f78edc61a3ecfabba2b81ad6debb7ad3b9`.
Copied the actual11-file v2 and47-file listener baselines, license notices and
selected provenance into fresh paths here; frozen originals were never edited.
The v2 R1 review accepts only the terminal IPC correction; listener review accepts
only selected SOURCE/API custody. Neither constitutes composition acceptance.

Derivation uses actual v2 implementation once. Backend C, R1 adapter C/diagnostic
header, crypto configuration and three policy pins are byte-identical. Listener
custody C is byte-identical; its header changes only the public API include.
New API header includes the actual current `candidate_auth.h`, not the old sibling
frozen header. Four v2 basenames changed and three files were added; their exact
classification is sealed. No compiled app source changed after build01.

## Fresh offline compile

Actual command (one build, no retry):

```sh
python3 -B apps/rtsp-auth/prestarted-owned-listener/build.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

The new builder was derived as source from the frozen recipes. No old guarded
builder was executed/imported. It snapshots new app, real framework/verifier and
277 retained vendor files, preserving licenses, before compiling. The same15
real units are freshly compiled for each platform: four app, verifier IPC, seven
framework and three SHA/constant-time/platform units. No sibling archive linking,
old-object reuse, synthetic/harness/main/fixture build or executable link.

HOST GCC16.2.1, ARM retained Buildroot GCC14.3.0; actual tool binaries/cc1/as/ld
and compiler aliases, target/version logs, arguments/environment, dependencies,
source copies, stack-usage output, archives, RELs and maps are retained in
`runs/build01/`. ARM flags retain ARMv5TE/soft-float. Both use original crypto
configuration, hardening flags, and real platform pin predicates. Existing
misleading-indentation exceptions on server/input and nonnull warning exception
on legacy input are explicit in recorded commands, not new semantic changes.

Build ran16:19:19.320948–16:19:29.126347UTC. There are58 recorded commands:52
successful compiler/tool/archive/REL commands and six expected diff exit1 records.
Thirty actual C compiles produced empty diagnostic logs. Report status retains
the inherited spelling `HOST_ARM_CHANGED_UNITS_COMPILE_ARCHIVE_REL_PASS_NOT_RUN`,
but actual command list compiles ALL15 selected real units per platform. `-r`
with section GC selects the real start/serve/finish and cancel/revoke/diagnostic
roots. No output was executed. REL libc/libgcc undefined references are expected;
these artifacts are not self-contained executables or release packages.

## Static final custody

Fresh `seal03.py` is the corrected data-only finalizer. It does not import candidate code, run
subprocesses, compile, execute ELF, invoke tests or probe endpoints. Command:

```sh
python3 -B docs/qualification-plans/rtsp-prestarted-owned-listener-20261022/seal03.py
```

It exclusively creates `BUILD_SEAL01.json` then `SOURCE_SEAL01.json`, refusing
existing seal outputs and pinning both preserved static failures. It rehashes all recorded337 inputs,2669 frozen
paths,308 consumed dependency paths and their copies,34 artifacts, toolchain/tool
bytes/resolutions and58 command logs. It compares generated source/provenance/
license snapshots with origins, actual per-unit dependency lists and link/archive
arguments/maps, and source aggregate with the reported14-file candidate pin.

It reads archive headers/member bytes and ELF headers/symbols/relocations directly
as data. Archive members must be the15 actual freshly compiled objects exactly
once; project definitions cannot be duplicated across them. Both final ELF
objects must be ET_REL with the expected machine, required actual APIs once and
no old serve/main/fixture exports. It independently compares actual undefined
relocation names with retained tool inventories and rejects project references.
Three unused `camera_rtsp_digest_*` undefined symbol-table names survive GC but
have NO relocations: they are disclosed, not mislabeled resolved definitions.
Maps and log inventories are retained evidence, not runtime call tracing.

It also compares preserved auth/header/layout, R1/helper bodies, complete original
server helper prefix and original serve loop/client/source cleanup span. These
are static byte-custody assertions, not functional tests or independent review.
The build seal contains observations/pins; source seal contains all authored
source pins and every evidence file except itself. Aggregates use sorted
workspace-relative UTF-8 path + NUL + raw SHA256 bytes; absolute recorded input
maps retain their original path spellings. Seals are not self-hashing.

## Failure/history and limits

Current build01 succeeded first attempt; no current build/policy rejection or
output/runtime execution. Two read-only lookups returned File not found:
the assumed v2 `PARENT_DISPOSITION.md` (parent acceptance was explicit) and an
assumed `08-STREAMING-AND-BROWSER-VIDEO.md` name (inventory identified actual
`08-STREAMING.md`). No guessed files were created and no policy retry/reroute.

`baseline-provenance/history/` preserves v1 correction-required R1 review and its
unrelated `SEAL_FAILURE01.json` basename-classification bookkeeping failure.
Listener review's original filename lookup errors remain copied in its
`COLLECTION_ERRORS.md`. Old failures are not erased by current compilation.

Current initial `seal.py` stopped before any final seals on a static bookkeeping
error: its direct ELF parser included only global/weak undefined names, but
retained `nm -u` also includes the three GC-localized undefined Digest names.
Data-only inspection confirmed exact name/relocation agreement when local names
are included; none of those three names has a relocation on either platform.
`SEAL_FAILURE01.json` records the command/exit/error and unchanged original
finalizer hash. Fresh `seal02.py` corrects only undefined-name classification and
pins that history. It then stopped before seals on ARM map bookkeeping: GNU ld
appends `LOAD linker stubs` after its OUTPUT line in addition to the fifteen real
objects. `SEAL_FAILURE02.json` and unchanged `seal02.py` preserve this second
failure. Fresh `seal03.py` requires exactly that one ARM-only terminal pseudo-entry
and records it separately from fifteen actual input paths. No app/build source
change or compilation/output/test rerun.
An initial documentation patch was also rejected atomically for a missing context
line; no partial files were created. This was tool bookkeeping, not policy denial.

No target/UART/RPC/status/network/private IO or query77-client lane inspection;
no runtime/functional/penetration/adversarial/fuzz/mutation campaigns. No output,
listener, helper, generator, test or candidate was run. Deferred functional checks
and native/full-app gates remain open. Historical12:12UTC U-Boot observation is
not current admission; both original HOLDs are unchanged. No next queue is created.
