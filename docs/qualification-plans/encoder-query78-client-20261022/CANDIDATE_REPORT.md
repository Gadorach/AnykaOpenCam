# ENC-02 / PLAT-01/02/05/08 — query78 client SOURCE/build candidate

Selection dated 2026-10-22; actual build-host UTC below is 2026-10-23.
**UNREVIEWED candidate delivered; HOST/ARM compile/static-link PASS. Neither
executable run. Parent independent review required before curated commit or any
image integration.** No native recipe or automatic next queue.

Own writes are confined to new `apps/qualification/encoder-query78-client/` and
this documentation directory. No old source/helper/client/kernel/recorder/
controller/review/HOLD, Git/index, AGENTS, central plan, model/provider/safeguard
or hardware operation was changed by this lane. Parent-only separate central
updates are not claimed unchanged by these receipts. No other lane was inspected.

## Exact provenance and source delta

Before code, [CONTRACT.md](CONTRACT.md) fixed the scope. Read current AGENTS,
00-INDEX, execution ledger, relevant03 platform/06 encoder plans, original full
267-line main/2455-byte UAPI/255-line builder text, actual old contract/report,
independent client review and parent disposition. No old builder/helper import,
main call or execution occurred.

Original five-file client aggregate was freshly revalidated:
`101afbb482c4562d02e58ab7c2aafed0d3cbb76a7d7b4cc36cf08e667fc80600`.
Original independent FINAL_SEAL SHA256 was freshly revalidated:
`e05a777864ff87de8d8773caa1bc9c1d6da7581e24a4968f65dcd809818333ab`.
Those are historical conditional SOURCE/API dispositions, not acceptance of78.

| Main source | Lines / bytes | SHA256 |
|---|---:|---|
| Original query77 |267 /10258|`d132a2eeaae170586aa437a2e7a3102027d9ff656077c5102f57dd79d95268b7`|
| New query78 |267 /10258|`d3f85cd01f722ce0148531b0f6430c18795733e9dbf4336bff8df426a1946985`|

[CLIENT_DELTA.diff](CLIENT_DELTA.diff) contains exactly two runtime-literal
replacements, each checked to occur once in the appropriate before/after source:
the exact trial argument77→78 and exact uname release77→78. Restoring these two
strings recovers **all original source bytes**. No other source/body/docstring/GPL
change, including the deliberately retained historical query77 comment.

New main requires reserved release `6.18.55-qc07f-encoder-query78`; it does not
accept77 as fallback. [BUILDER_DELTA.diff](BUILDER_DELTA.diff) contains only the new
DOC directory and three source/object/executable name edits. Full-byte transformed
old/new builder equality was checked before and after build. No old builder was
called. Header is byte-identical to original and retained kernel include:
2455 bytes, SHA256
`6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37`.
88-byte command0x80585166, all schema/layout/static assertions unchanged.

New five-file source aggregate:
**`66f6e86fa51036593eae5d2bc77df299ce513e0549f181fec68562c5a099401e`**.
Recipe: SHA256 of concatenated sorted workspace-relative UTF-8 path + NUL + raw
file SHA256 bytes. Individual pins in SOURCE_BEFORE/OFFLINE_CLOSURE/HANDOFF_SEAL.
GPL-2.0-only authored/imported notices retained. Baseline Linux license input and
retained toolchain source/license provenance remain associated; compiler/binutils/
musl/glibc licenses are separate, not a global license grant or redistribution.

## Fresh ordinary offline build

Only the new builder was executed, once, into exclusive refused-if-existing
`apps/qualification/encoder-query78-client/build01/`. Actual selected build argv:

```text
python3 -B apps/qualification/encoder-query78-client/compile_only.py build01 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

This is an offline compiler recipe only, not a native client-use recipe.
Build-host UTC: started **2026-10-23T01:11:54.063677+00:00**, ended
**2026-10-23T01:11:58.843511+00:00**. These real host timestamps do not establish
target time/current state or any admission/continuity guarantee. Directory date
retains the parent selection identity, not an invented earlier build time.

**36 tool commands all exit0:** two compiler-version, two readelf-version,
24 compiler-support-location queries, two dependency-only preprocesses, two C
compiles, two static links and two ELF inspections. No failed build/collection
or policy refusal occurred in this lane; no retry/reroute was used. Original
earlier failures, including prior reviewer bookkeeping failures, are unchanged.

HOST GCC16.2.1/host readelf2.47; ARM GCC14.3.0/readelf2.43.1. Retained exact
wrapper/real GCC/libc.a/libgcc.a four expected SHA pins matched, and whole retained
ARM file/symlink/tool/header/archive map was captured before tool use. Old baseline
input revalidation also confirmed retained toolchain bytes against old receipts.
Clang23.1.3 kernel drift is not used as a userspace pin bypass.

Unchanged common flags: `-std=c11 -Wall -Wextra -Werror -Wpedantic -fno-common
-fno-strict-aliasing -ffunction-sections -fdata-sections -fstack-protector-strong
-fstack-usage -Wframe-larger-than=8192 -Wstack-usage=8192`, local include and exact
workspace prefix map. HOST `-O2`; ARM `-Os -march=armv5te -marm -mfloat-abi=soft`.
Both links `-static -no-pie`, section GC, actual map and link trace. No object
reuse, stubs, warning suppression, budget increase or missing-feature bypass.

REPORT.json records every actual tool argv/cwd/build-host start/end/log pin and
base environment: PATH `/usr/bin:/bin`, LC_ALL `C`, SOURCE_DATE_EPOCH1792627200,
TMPDIR local build01/tmp. Each tool additionally receives LD_DEBUG=`files` and
its label-specific LD_DEBUG_OUTPUT under build01/logs as fixed by builder source.
This traces build-tool loader closure only, never client/target runtime. Explicit
subprocess allowlist contains only resolved HOST/ARM compiler/readelf paths;
compiler-internal cc1/collect2/as/ld are recorded dependencies, not client outputs.

| New local static ELF (never run) | Bytes | SHA256 |
|---|---:|---|
| `build01/arm/encoder-query78-qualification` |30140|`5947009a526bbecacf1d0843f26aabd10fb2b13d6db42203585a52d48d95b4fa`|
| `build01/host/encoder-query78-qualification` |906640|`0b220037d53f7209e3ef618da7c51da9ae3bd237b900c1ae8fb18ae89a3760a0`|

Both hashes differ from77; no old binary hash is adopted as78. Actual ELF parsing
and readelf show ARM ELF32 EXEC/EABI5/soft-float/ARMv5TE and HOST ELF64 EXEC/x86-64.
No PT_INTERP/PT_DYNAMIC/dynamic section/NEEDED, no WX LOAD, GNU stack RW/nonexecuting,
nonempty real main and entry inside executable LOAD, no strong undefined symbols.
Compiler `.su` records are static: largest authored frame ARM1496B/HOST1504B;
PT_LOAD memory sums ARM14468B/HOST833165B. Entry/flags/segments/main and all frame
records are in OFFLINE_CLOSURE01. These exclude page rounding, live call depth,
libc startup, VFS/kernel/slab/other allocations: **not RSS, total stack or native
combined-memory fit**. HOST static glibc is not a verifier-budget proof.

## Custody/seals and limitations

- `collect_offline.py before/after/seal` performs public read/hash/text/direct-ELF
  metadata collection only, no subprocess or candidate import/execution.
- SOURCE_BEFORE pins4024 path-associated immutable baseline inputs/source/build/
  old documentation/review/tool-ELF paths, checked unchanged after build and again
  at final seal. This includes3710 original build input paths,252 original sealed
  outputs and58 supplementary original tool-ELF alias paths; counts overlap, not
  additive unique physical files. No original artifact is rewritten/resealed.
- Fresh PRE_INPUTS3573 paths; INPUTS3711 captured paths checked unchanged. Exact
  consumed HOST119/ARM37 dependency maps equal actual `-MD` maps, and every copied
  source/header body matches the captured pin. Toolchain, cc1/collect2/as/ld,
  crt/libc/libgcc/support and actual link trace/map inputs remain associated.
- Fresh OUTPUT_SEAL verifies252 original build outputs/logs/copied dependencies.
  Supplemental loader closure resolves58 final-observed tool/shared-ELF alias
  paths and13 transfer spellings with no unresolved observed name. The inherited
  narrow LOADED_TOOL_ELF_FINAL receipt contains only2 plugin paths; it is retained
  intact, not mislabeled complete. Final-discovered HOST support/loader/library
  pins are explicitly **not all pre-use hashes**. Full retained ARM inventory and
  consumed compilation headers have their distinct pre-use capture boundaries.
- OFFLINE_CLOSURE01 binds source/tool/header/link/ELF facts. HANDOFF_SEAL01 binds
  source/docs/local report/input/output-seal pins after one more fresh unchanged
  check. These are authored custody facts, not independent review or reproduction.
- `.gitignore` excludes local `/build*/`; executables/toolchains/images stay local.
  Public-safe path/hash/size metadata contains no credentials/private capture/
  device labels. No Git command, index edit, commit or publication was performed.

## Unchanged source/native boundary

Default/refused argv remains effect-free before authored I/O; never launched.
Only trial/release literals changed. All device/type/sizeof/ioctl/clock/budget/
timeout/signal/close/retain-uncertain/output/accounting/callback/source policy is
otherwise byte-identical. One watchdog owner/nominal5s initial configuration,
original1.5s whole-phase budget, one encoder open/two identical virgin query88
snapshots, no query retry or global identity fallback remain intact.

No global-idle/prior-DMA/sole-owner proof follows from NONE, argv, uid, sysfs or
release. Clear-before-one-close never retries/claims a former FD. Uncertain
post-open effects retain only actual still-owned descriptions while the process
survives; SIGKILL/OOM/exit remain outside that promise. Failed/late disable may
already have stopped the timer: this is not armed recovery/external retention.
Independent observer/lifetime/capture/fresh unconsumed catch and manual OFF
fallback remain mandatory future parent gates, along with exact boot/image/FD
association, global idle/no competing/inherited descriptions and bounded output.

Query77 kernel/UAPI/driver/image pins are baseline interface association ONLY;
query77 was never booted. Reserved query78 release is **not a built/running kernel,
new ABI pairing or embedded client**. No image/kernel/config/module/cpio/monitor/
launcher/controller/provider work, RAM image generation, embedding, staging,
upload/boot/query/reset, target/serial/UART/RPC/network/private/current-state/
proc/dev/HOLD/control/clock collection, process inventory/control, executable/
default/help/test/payload-generator/listener run or physical request occurred.
Future embedding may fail static RAM limits;128KiB,40MiB/CMA,4MiB and inherited
child/HOST caps are unchanged. No deployable app, PLAT-02/native watchdog/memory/
product/G3/G8 acceptance. Both actual HOLDs and all frozen originals untouched.

**Handback:** exact unreviewed SOURCE/build candidate for parent's independent
ordinary SOURCE/compile-custody review. No source adoption, executable native
recipe, image integration, automatic retry or next hardware operation selected.
