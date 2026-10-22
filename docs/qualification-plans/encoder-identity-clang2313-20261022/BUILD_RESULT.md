# NEW Clang23.1.3 real encoder-query ARM object compile — PASS, not run

2026-10-22; RP-07/09 source dependency only. Parent explicitly selected this
**distinct SOURCE-ONLY compiler baseline**, not reproduction/adoption of the
retained76 Clang23.1.1 compiler association. The old builder's guard and honest
compiler-prerequisite failure remain byte-exact.

## Actual result

```text
python3 -B apps/qualification/kernel76-encoder-identity-clang2313/build.py build01
```

**PASS12:16:36–12:16:42UTC**,6 host tool commands: compiler version, resource-dir,
compiler ELF inspection, genuine dependency preprocessing, actual whole-driver ARM
object compilation, and resulting object ELF/symbol inspection. No linker, UAPI
declaration rerun, full kernel/module/image build, executable/main, runtime/test/
synthetic campaign, install/download, target/UART/RPC/status/network/private IO,
Git/index/central-plan/P1 or hardware/helper operation occurred.

Output: `runs/build01/objects/encoder-identity-clang2313-ARM.o`,**19676 bytes**,
SHA256 `886c46f2dfe39d644d5aec2fb744461884e43b272d28eb56b35bab3bfaf8e2ee`.
Static ELF inspection: ELF32 little-endian ARM EABI5, **REL**, entry0, no program
headers. Real `encoder_ioctl`, `encoder_open`, `encoder_release` and built-in init
are present. The actual included query body was compiled into that real provider
translation unit; no extracted/shadow helper, guessed symbol or mock implementation.
No output was executed. ELF metadata is not module ABI or native behavior proof.

No warning/error/fatal diagnostic was found in the exact compiler stderr logs.
Loader tracing remains beside unchanged original diagnostics, not a warning filter.
No error class, mismatch check or configuration guard was disabled.

## Exact source and old evidence custody

- Frozen query source pin:
  `ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`, from
  `docs/qualification-plans/encoder-identity-20261022/SOURCE_SEAL01.json`.
- The7-file old `candidate/` was copied byte-exact into this run's `source/`.
  No implementation/header/UAPI is independently maintained in the new app path.
  Existing patch SHA256 remains
  `9d096489e3ba8cc44556b81feb8192b6c5f08e698c18b2718cdfedbab5aea232`.
- The real frozen driver source compiled has SHA256
  `9bf16b1175befb091d6c61c3ac573121378e1d8c403eed21806a3e8fc80fc946`;
  its query implementation/UAPI match the original seal and old run pins.
- All**37 files** in the old `encoder-identity-20261022/runs/build01/` tree
  were inventoried before and verified unchanged afterward, including its failed
  report/logs/partial UAPI artifacts and original/candidate/input snapshots.
  All separately sealed old app/evidence files also remained unchanged.
- New app-side footprint is only `build.py` and `README.md`,258 lines/14371 bytes.
  All new files/artifacts remain within the two assigned fresh directories.

Original source GPL/SPDX, kernel COPYING and source/header notices are retained;
the new builder is GPL-2.0-only. No compiler/library binary is imported into an
application or given a new license; this is local toolchain metadata and source
compilation, not a redistributed release/package.

## NEW compiler closure and exact method boundary

Actual `/usr/bin/clang` resolves to `/usr/bin/clang-23`, **Clang23.1.3**:
124752 bytes, SHA256
`583017067c40b0ff0a9b35805d8ac89d5a66fb014221c78ae13a7c21997dd464`.
Resource directory `/usr/lib/clang/23`: **334 resource header files inventoried
and hashed**. The real driver retains `-nostdinc`; **zero resource files were
consumed** in its actual513-file dependency closure. No resource/sysroot override.

Actual ELF interpreter `/lib64/ld-linux-x86-64.so.2` resolves to
`/usr/lib/ld-linux-x86-64.so.2`, SHA256
`2a950d46575efa0210d22440988f5a6b4507ebddc6c86d9a42e6e7ef6f01463e`.
`LD_DEBUG=files,libs` on the **host compiler** recorded15 actual loaded ELF images
including loader, libclang-cpp23.1, libLLVM23.1, libc/libstdc++/libgcc and their
dependencies. Every observed loaded image's path/resolved path/hash is retained
and rechecked unchanged, not inferred from the small compiler wrapper alone.
No `/proc`, target status or process-poll operation was used for this closure.

Important compiler-library pins:

| Actual loaded library | SHA256 |
|---|---|
| `/usr/lib/libclang-cpp.so.23.1` | `9005ad5821598cd8fda30bc26fdf98e2294088c0eeb69eb4d70e97aa74c402cd` |
| `/usr/lib/libLLVM.so.23.1` | `29a1219f15b0abedf329210dab4a9d43a043516906340f851b7bc341fd319b2b` |

Complete executable/version/resource/loaded-image/header/command/object closure:
`runs/build01/REPORT.json`. All513 actual source/header dependencies were first
identified by the genuine compiler, pinned and copied before object compilation;
the actual compile's dependency set and hashes then matched exactly. All531
captured input paths and compiler/loaded-image pins remained unchanged afterward.

The retained actual driver `.cmd` supplied all ARM target, ABI, optimization,
warning/error, config and required kernel inputs. Changes were only explicit new
compiler association, fresh exact source/header location, absolute rebasing of
previously output-relative include paths, and fresh dependency/object destinations.
`command_path_changes_only` and full original/actual argv retain every difference.
No linker, make or configuration/header generator was invoked. No original file
or recorded `.cmd` was edited.

Retained generated metadata **still says23.1.1**, including CONFIG_CLANG_VERSION
230101 and CONFIG_CC_VERSION_TEXT. `.config`, symvers and header inputs match the
old selected76 data; their old compiler association was not rewritten to pretend
23.1.3 is the original compiler. This successful new-baseline source object compile
does **not** qualify compiler/config feature pairing, module ABI, final image or
runtime readiness. The old23.1.1 reproduction prerequisite remains unresolved and
its failure remains valid independently of this result.

## One next source dependency; no automatic build/native queue

**Next: parent ordinary exact-source review/disposition of the original query
overlay plus this compiled object/method closure**, especially per-open lifetime
under enc.lock, interpretation of saturating reference/history fields and unchanged
legacy ioctl/final-release semantics. No further adversarial/fuzz/mutation test is
selected. Compiler acceptance alone is not source acceptance or provider adoption.

Only a later separately selected paired full build may choose/pin its complete
compiler/config/header baseline and adopt reviewed source into a fresh kernel tree,
preserving original76 and rollback. Later native query still requires parent-only
hardware/recovery/source/image/owned-FD admission. No full build/query is queued;
the frozen76 image does **not** gain support from an offline object.

No DMA/normal-close proof, exclusive-alias/task authority, memory-fit claim or
failed-next retention follows. Watchdog/backing/provisioning/owned listener/radio
HOLD,128KiB/40MiB/CMA and native2MiB/HOST4MiB child limits/old image pin stay intact.
No camera/main/provider deployment or G2/G3/G8 acceptance is claimed.
