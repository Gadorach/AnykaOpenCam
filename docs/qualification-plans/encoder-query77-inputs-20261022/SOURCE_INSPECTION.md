# C2a author static source/cleanup/format inspection — unreviewed

2026-10-22. `apps/qualification/encoder-query77-inputs/selected_inputs.py`,376
lines/18643B, SHA `5fbcc656882075c44c1e4f4e5df8a896fe83433a33e770deec1fd50c3792d3f0`.
No candidate import/API/helper/format/path/CRC/cleanup executed. HOST AST/syntax
only; public artifact SHA and a direct52B client ELF-header read were separate
author observations, not running the new parser or testing behavior.

## Closed dependencies and lineage

`SELECTED_INPUTS01.tsv` is the25-entry runtime catalog, all separately hashed
18:18UTC before source creation and re-compared18:26UTC. AST literal extraction
checked each source `_Pin` row without calling candidate initializers/functions.
Full-file and format checks are **proposed code**, not an observed validator run.

- Image/package/config/zImage and DTB/initramfs pins come from actual PACKAGE02
  and build disposition/result. DTB path is selected by `package02.py:28` using
  `build.py:183–187`'s source02 artifact directory. Original initramfs path is exact
  `.config:143` and `build.py:27,296–304`, no compatibility symlink/guessed alias.
- Generated release/compiler paths confirmed by actual BUILT.json outputs221–231;
  `compile.h` path/pin is also in BUILD_RESULT. Exact config compiler/localversion
  lines read from real config5/8/10/13/37. No make/packager/helper invocation.
- Client artifact/C/three UAPI copies from client source contract/report and
  actual source02 kernel header; no C compile/header evaluation. Loader/recorder/
  primitive source paths match native-method public input receipt and Boot lineage.
- C1v2 module/review/seal pins exactly match the conditional independent review.
  That review does not accept this new validator, apply custody or authorize IO.
- `DISCOVERY_INPUTS01.tsv` records five public path-discovery/context sources;
  PREPARED was read as command metadata, not used as authority or runtime input.
  Runtime source never imports them or follows their returned paths/argv.
- `FROZEN_CHECK01.json` compares inherited35 inputs:32 frozen matches, three
  changing central checkpoint documents preserved separately, not rebased. All
  four C1v2 review outputs match the frozen review seal. Not a whole-tree build
  reproduction or transitive acquisition/private evidence audit.

## API and actual call/ownership closure

| Source | Static finding, not exercised behavior |
|---|---|
| Constants/`_Pin`27–61 | Closed tuple of25 frozen literal pins, exact canonical workspace and fixed public relative paths. No path/root/digest/caller selector, source discovery, old controller/Boot/private/C1 import. Declaration dataclasses/enums only, no authored import-time IO. |
| `Reason`, `InputsError`64–86 | Only fixed primary/postcheck/cleanup/overall codes; invalid constructor code types sanitized. No raw path/OS/env/digest/error interpolation. Public rethrow suppresses original exception chain. |
| `InputFact`, `OfflineManifest`99–110 | Frozen fixed schema. Only public labels/sizes/SHA/narrow offline inspection and OFFLINE_NOT_ADMISSION limitations; no native PASS/ready/owner/catch/clock/recovery authority fields. |
| `_Read.open`144–169 | Only manifest member accepted. Fixed canonical absolute components, bounds32/1024, no dot/dotdot/Git-component aliases. Root/ancestors opened DIRECTORY/RDONLY/NOFOLLOW/CLOEXEC/NONBLOCK, safe uid/no022 and path/FD directory associations checked before further descent. Public file precheck REG/single-link/trusted uid/no special or022, exact size, then RDONLY/NOFOLLOW open and actual FD/path metadata comparison. No private0700 policy or chmod. |
| `_dirs_check`, `_file_check`132–143 | Pin/recheck inode/type/mode/uid/gid associations for held directories; regular file full metadata includes size/nlink/mtime/ctime. Directory timestamps/link counts intentionally not frozen; atime excluded. Root path `/` and entries compared from retained parent descriptors, not resolve/follow symlinks. |
| `_stream`294–328 | Fixed-size4096B preads and exact count, SHA256, one EOF byte; bounded selected size from manifest. Large files never retained; only small format/UAPI records<=64KiB may accumulate. No full image/zImage/initramfs buffer, target/source FD handoff or executable mapping. |
| `_observe`331–348 | Entry/read/format errors captured as fixed primary. Finally attempts available directory and file postchecks independently, then all host descriptors close. A failed postcheck/close prevents return; cleanup code does not erase primary. No result escapes while descriptors remain owned. |
| `_Read.postcheck`171–184 | Metadata checks after success or failure; an unavailable file FD is not opened/probed to fill context. Directory failure does not skip available file check. No raw error formatter, repair or write. Primary failure remains non-OK even if no descriptors were acquired. |
| `_Read.close`186–198 | Clear file slot/pop each directory slot BEFORE one close. Remaining closes attempted on error, no retry or claimed numeric retention, no destructor. Future caller serialization/namespace and async limitations remain. |
| `validate_selected_inputs`351–376 | No arguments; explicit platform/features/uid admission only for this offline host read. Iterates fixed pins through `_observe`; actual UAPI bytes compared after each read's descriptors close. Returns frozen OFFLINE_NOT_ADMISSION only after all observations/closures. No future freeze, executing-code or native facts. |

All external authored IO callsites from AST: `os.open`152/157/167 (directories
and fixed public file); `os.stat`135/142/162 and `fstat`134/141/154/159; `pread`
302/318 only bounded selected bytes; `close`189/195; `getuid`357 only explicit
API call. No write/create/mkdir/chmod/remove/rename/fsync, proc/dev/socket/serial/
RPC/network/process/signal/get-clock, importlib, exec/eval, CLI or output artifact.
`_observe` is called only by the public fixed catalog loop; `_Read` only there.
All pure format helpers are called from that same bounded-data closure, never
from returned source/JSON commands or caller callbacks.

## Format/association source reasoning

- `_bzip_update`201–207 uses nonreflected04c11db7 polynomial,32-bit mask per bit,
  whole-image initial/final ffffffff. `_stream` updates it across every image chunk;
  payload IEEE excludes64B header, header IEEE zeroes its checksum field. Exact
  header tuple includes all fields/name and full expected3603307B length is
  enforced before opening. No CRC function was invoked by the author.
- Image spans `[64,3600736)` and `[3600736,3603307)` hash new zImage3600672B and
  DTB2571B, then compare to exact constituent pins independently observed in their
  own file reads. This is hash association, not runtime/boot/DMA proof.
- `_package`222–243 checks selected image/zImage/DTB/initramfs pin dictionaries,
  fixed image path/release/whole CRC/header and fixed40MiB/kernel/staging/CMA/
  decompressor ranges; no PASS/G2/G3 trust or interpreted argv. It does not
  independently decompress XZ/GZIP, recalculate ELF-kernel extents or repeat build.
- `_elf`246–270 parses ELF32LE EXEC/ARM/EABI5 soft-float, exact table sizes/count
  caps and file/address bounds; LOAD filesz<=memsz/alignment, no WX, entry backed
  by executable LOAD, one RW non-exec stack; PT_INTERP/PT_DYNAMIC/SHT_DYNAMIC
  absent. No dynamic section means no dynamic NEEDED semantics to follow.
  ARMv5TE remains a previously reviewed exact-file-pin fact, not parsed here.
- `_small`273–291 checks five explicit config identity lines plus exact initramfs
  SOURCE, generated release/compiler metadata, package and C1 seal-to-REVIEW hash
  association. Three UAPI small byte strings compare equal and share frozen pin;
 88-byte/version/ioctl meaning comes from prior header review, no fresh compiler/
  syscall check. No old76 fallback or station/module support inferred.

## Deferred facts and handback

Conditional serialized namespace/FD lifetime only; precheck/open/postcompare does
not prove atomic REG-only-open against same-UID substitution or privileged mounts.
No signal/thread/async-allocation/durability/runtime or Python/stdlib4MiB/native
128KiB memory-fit proof. Source/class declarations and decoded literal catalog
agreement are lineage evidence, not independent semantic acceptance or a passed
format run. Descriptors closed before return cannot freeze future inputs.

All C1 reviews/failures and originals remain frozen. No actual marker/control/
capture/current-state/native observation, source adoption, physical request or
automatic next call/test/worker/native queue. Independent review must precede any
parent-selected offline invocation; none is supplied as a runnable recipe here.
