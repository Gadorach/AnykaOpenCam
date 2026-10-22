# Run01 configuration stop — preserved; no full build

2026-10-22. `olddefconfig` exited0, but the stricter author config gate refused
before full compilation because `CONFIG_TOOLS_SUPPORT_RELR=y` disappeared.
The only other differences were the selected five compiler/localversion identities.
No hardware/feature symbol was manually flipped, ignored or force-enabled.

The exact source explains the instrumentation artifact:

- `init/Kconfig:112–113` obtains this internal capability through
  `scripts/tools-support-relr.sh`.
- That script:19 requires **empty NM stderr**, using
  `test -z "$( $NM ... 2>&1 >/dev/null )"` (spacing paraphrased).
- Run01 exported `LD_DEBUG=files,libs` without `LD_DEBUG_OUTPUT`. The dynamic
  loader therefore wrote diagnostic lines to that captured stderr even for an
  otherwise successful LLVM NM. This is not a compiler capability conclusion.
- `arch/Kconfig:1638–1645` uses the capability only for RELR; selected ARM32
  does not select ARCH_HAS_RELR. Nevertheless the gate correctly stopped rather
  than silently classifying/removing the unexpected difference.

Reviewed method correction for **fresh run02**: keep complete loader evidence but
add `LD_DEBUG_OUTPUT=<owned build02>/loader/trace`. Glibc writes per-process trace
files there, leaving the Kconfig probe's real stderr unchanged. Source/config
guards, feature symbols, warnings and installed tool versions remain unchanged.
No rerun overwrites run01. Configuration must again pass the same narrow gate
before full compilation. No unit/runtime/security/mutation tests are selected.

Original run01 source, metadata inventories, logs, refused config/diff and failure
receipt remain under the exact fresh source01/build01/images01 owned work paths.
No image was created in images01. The next run uses fresh02 paths; these are
separate immutable evidence, not corrections written over the original failure.
