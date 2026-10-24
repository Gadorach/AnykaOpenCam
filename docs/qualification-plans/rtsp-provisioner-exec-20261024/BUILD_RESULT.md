# Build01 — production compile/link/metadata PASS, NOT RUN

2026-10-24. First and ONLY fresh attempt. No preflight/recipe/compiler/link failure,
correction, retry, build02, guard weakening or runtime invocation. This is author
evidence, not independent review or parent source disposition.

## Exact invocation and evidence

From canonical workspace:

```
/usr/bin/bash apps/rtsp-auth/rtsp-provisioner-exec-candidate/compile_recipe.sh
```

Fixed refuse-overwrite output: this directory's `build01/`. Exact127 command argv
records in `build01/COMMANDS.txt`,127 corresponding logs and successful command
names in `SUCCESS.txt`. Recipe copies and diffs16 actual public source inputs,
pins22 overlapping selected inputs, compares17 frozen source/design/prior-parent
inputs before/after, and retains actual compiler `.d` dependency hashes. All16
source-copy diffs, frozen-pin diff and selected-input diff are empty. All32 C
compiler logs and five REL/static-link logs are empty. Nothing in old source was
edited. No Git/index inspection or modification was performed by this lane.

| Operation | Actual evidence |
|---|---|
| C compilation |16 HOST +16 retained musl ARM, production symbols/defines, no fixture/native fallback |
| New C units |public codec, launch security, typed parent owner, actual worker main, ABI metadata |
| Actual unchanged dependency C |private handoff, startup constructor, graph, rate wire, route/input, request parser, Digest, SHA256, MD5, constant-time, zeroizer |
| Partial links |HOST/ARM parent and worker = four real GC-rooted RELs |
| Final link |ONE real static ARM ELF; no HOST executable linked |
| Main roles |parent has three actual typed owner roots and NO main/constructor/handoff; worker has actual main and actual constructor/handoff, NO parent-owner implementation |
| Runtime |ZERO calls/tests/fork/exec/hardening/credentials/material operations, HOST included |

HOST compiler: GCC16.2.1 20260810. Retained ARM compiler: GCC14.3.0, Buildroot
2021.11-18033-g83947c7bb6, `-Os -march=armv5te -marm -mfloat-abi=soft`.
Recipe logs/pins driver, cc1, collect2, as, ld and metadata tools. The unchanged
frozen input parser alone retains its existing warning exceptions; new source
uses `-Wall -Wextra -Werror`. No old combined REL, old fork child, verifier engine,
runtime, provider, media, TLS or generated stub is linked as project code.

`STATIC_LINK_INPUTS.sha256` additionally pins the actual eight CRT/libgcc/musl
inputs named in the static map and actual `.br_real` compiler driver. This was a
public-input hash-only supplement, not recompilation. `BUILD_FILE_METADATA.txt`
records artifact ownership/size/timestamps: final recipe result at
2026-10-24 11:47:47.373592627 -0300. These are filesystem timestamps, not an
independent operation-duration or wall-clock-continuity guarantee.

## Public protocol and local C metadata

The offline `abi_metadata.c` objects contain constants only, never functions and
never enter either linked role. Readelf dumps show:

| Value | HOST | ARM |
|---|---:|---:|
| protocol prefix / maximum / receive scratch / reply |1374 /7934 /8192 /1392 |same |
| device / association / profile / group wire records |144 /128 /204 /72 |same |
| actual rv_material |1536 |1536 |
| candidate_config |416 |368 |
| candidate_prestarted |1032 |1000 |
| pe_public local typed reconstruction |8136 |8056 |
| pe_exec_owner private stable storage |13128 |13032 |

Local C sizes are NOT wire ABI compatibility. No C struct/stat/enum/padding/
pointer or verifier descriptor is serialized. Actual new source makes no heap
allocation request. Individual stack-usage records are retained; these do not
establish transitive stack, libc, exec, memory-lock or process fit.

## Positive symbol-index / relocation chain, not absence-only inference

See complete `*-symbols.txt`, `*-relocations.txt`, `*-nm.txt` and `.map` files.
The actual ARM worker retained symbols and referenced indices include:

* main index328 (0x148), section46 → actual hardener index332 (0x14c), create357
  (0x165), move342 (0x156), zeroizer316 (0x13c).
* create section63 → constructor326 (0x146), clear299 (0x12b), hardener332.
* constructor section70 → HA1 index322 (0x142), defined GLOBAL section97.
* HA1 section97 → LOCAL function `hash` index110 (0x6e), section95.
* hash section95 → actual defined SHA256 init292/start305/update306/finish297,
  MD5 init337/start331/update307/finish348, frees and zeroizer316.

HOST matches the same real chain: main193 → hardener197/create222/move208;
create → constructor191; constructor → HA1 index187 (0xbb), section98;
HA1's relocation targets section-symbol61 (0x3d), section96 containing actual
LOCAL hash134. Hash targets real SHA256/MD5/zeroizer181 (0xb5), not stubs.
This distinguishes section-symbol relocations, LOCAL hash and actual GLOBAL
functions; no assertion demands unused Digest close/issue/verify remain GLOBAL.

ARM parent roots begin301/observe255/bind265 are defined; begin's retained CALL
uses actual pe_child_exec282 (0x11a), not worker main. Its own ELF hash reaches
actual SHA256 functions/zeroizer; no secret constructor is needed in that role.
Public validators are actual graph305, routes271, rate270, not synthetic graphs.

Parent REL symbol tables retain three **LOCAL UND, unreferenced** names from
discarded frozen wire functions: ARM issue223/close225/verify226 (0xdf/0xe1/0xe2),
HOST118/120/121 (0x76/0x78/0x79). Complete retained relocation tables have no
references to those indices. Map's discarded wire verify/challenge/close bodies
explain them. They are not linked definitions, required retained dependencies,
or a reason to invent replacement crypto. `nm -u` is preserved literally,
including those names and all normal libc/compiler/syscall/GOT dependencies.
No claim that RELs have no UND names; no final parent ELF was selected.

The final static worker has an empty `nm -u` result and actually defined main,
constructor, hardener, HA1 and handoff chain. Positive metadata establishes this
bounded project link, not full compiler closure, native ABI or instruction fit.

## Static companion — not an admitted image

`build01/worker-arm-static-NOT-ADMITTED.elf`:68080 bytes,
SHA256 `1c38f3ce45ccdb564c9cdfdc7cc134054b97bac88167a3ed5b49a835da941b0a`.
User-owned `gadorach:gadorach`, linker-created0755; no chmod/chown performed.
This is NOT a root-owned trusted worker inode/policy and MUST NOT be supplied as
an admitted launch image. No path/pin was installed into any config/launcher.

Actual metadata: ELF32 little-endian ARM ET_EXEC, EABI5 softfloat, entry0x10750;
R-E LOAD [0x10000,0x1a4ec), RW LOAD [0x1bfa8,0x20ddc), nonexec GNU_STACK,
no PT_INTERP/PT_DYNAMIC/dynamic section or RWX LOAD. Sum of LOAD memsz62240B is
NOT a process memory/stack/mlock measurement. Attributes include CPU5TEJ AND
Thumb-2 usage; do not treat these aggregate archive attributes or these compile
flags as ARMv5 instruction qualification. Native instruction/OS/libc/startup/
caps/securebits/seccomp/execveat/close_range/memlock/2MiB behavior stays OPEN.

All normal/negative/adversarial/mutation/fuzz execution NOT RUN. No image staged,
installed, admitted, launched, host-policy changed or hardware touched. Stop at
source handback; independent ordinary review and parent disposition precede use.
