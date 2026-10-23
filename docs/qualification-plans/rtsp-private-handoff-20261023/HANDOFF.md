# Private RTSP descriptor pipeline — author handback

2026-10-23 · SEC-01/03/05/11, STREAM-07.
**Actual C candidate delivered; HOST/ARM compile PASS; ONE normal PUBLICDUMMY
functional call PASS. No independent/parent source acceptance or native GO.**

## Delivered feature

`apps/rtsp-auth/rtsp-private-handoff/private_handoff.{c,h}` calls the frozen
startup constructor and real Digest/Mbed TLS implementation, creates the actual
1536-byte record in an exclusive fixed-name private regular file, retires its
writer with exactly one close, then owns/checks a real readonly CLOEXEC FD.
An explicit final move transfers that FD to the same unused actual
`candidate_config.material_fd` immediately before caller prepare/start. No backend,
runtime, listener, child, image loader, main or hardware action is called here.

The native provisioner-only hardening function performs real root UID/GID/saved-ID
and empty supplementary-group checks, sets/verifies zero core limits,
nondumpability/no-new-privileges, and requires successful current/future memory
locking. It does not drop IDs/capabilities inside media. Caller must call it before
private authenticated ingress; create repeats its actual operations/checks, not
a caller-provided permit. Native function has NOT been executed in this lane.
Hardening failure can have partial irreversible process-wide effects; it is
terminal, with no automatic rollback/retry. This is not proof of a separate
process, protected ingress, capability/FD/environment hygiene or authorization.

No real/operator material was read/generated/hashed/used. No material file,
username/realm/password/HA1 or payload digest was dumped/evidenced. Only the fixed
explicit public fixture was used in the one separately compiled HOST call.

## Custody and failure semantics (ordinary author inspection, not a failure suite)

| Phase/result | Actual ownership and required disposition |
| --- | --- |
| REJECTED | One attempt consumed, no returned FD; never reopen/overwrite/retry. No cleanup/namespace absence claim follows from an open failure. |
| CREATED_PARTIAL | File was created; written count, identity-valid, fsync and known writer-close flags are separately retained. Name means incomplete **admission**, not necessarily a short file. No unlink. |
| WRITER_CLOSE_UNKNOWN | Writer slot was cleared before one close; error is sticky, including EINTR. No numeric close retry, readonly open or transfer. Preserve uncertainty/file/object. |
| CLOSED_FILE_HOLD | Complete writer checked/fsynced/known closed, but readonly acquisition has not succeeded. File retained; no reopen retry. |
| READONLY_FILLED / READONLY_HOLD | Opened FD stored BEFORE validation. Failed validation/move leaves it in its owned slot; no implicit close, overwrite or second move. |
| READY | Known writer close + rechecked linked regular inode, mode0600, owner, exact1536 size, readonly access/CLOEXEC/offset0; still not backend-ready. |
| MOVED | Custody slot cleared, actual config now sole FD owner. Later prepare/start failure does not return or automatically close that ownership. |

Writer open uses O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC and mode0600; no chmod/repair.
At most64 write syscalls, each bounded by the remaining1536 bytes; EINTR consumes
the same budget. File fsync is mandatory. Pre/post checks retain dev/inode from
the actual writer and recheck the borrowed directory's identity, owner/mode0700,
named inode and readonly opened inode. Readonly O_NONBLOCK limits replacement-
FIFO blocking; it does NOT establish trusted namespace quiescence. Final move
rechecks and requires the same config address/empty material slot and actual
fresh prestarted owner fields. Caller still guarantees no earlier prepare/start
or other config user. There is no hostile-memory/concurrency defense.

All material scratch is wiped by the original crypto clear API at entry and on
every nonnull-scratch return, including refusal; borrowed password is unmodified.
No target heap allocation or128KiB cap increase. New ARM create's recorded
per-function stack is216B, move24B; this is not total-stack/native-memory proof.

## Build and one authorized functional observation

Read/audited new recipe and harness before invoking either. Old builders,
generators and qualification tool were not invoked. Fixed compact recipe:

```sh
env -i PATH=/usr/bin:/bin LANG=C LC_ALL=C TZ=UTC /bin/bash apps/rtsp-auth/rtsp-private-handoff/compile_recipe.sh
```

It refuses existing build01 output; no overwrite or retry occurred. Result:

- **24 C compilations**, diagnostics empty:11 HOST native-shaped +11 retained
  ARM native-shaped, plus separately compiled HOST PUBLICDUMMY handoff/harness.
- **37 successful tool commands total**:24 compilations, two REL combinations,
  one HOST-only harness link, ten version/machine/symbol/ELF inspections. No ARM
  executable was linked or run. Native-shaped combined outputs are ELF REL only.
- HOST GCC16.2.1; retained musl ARM GCC14.3.0, `-march=armv5te -marm
  -mfloat-abi=soft`, observed ELF32 little-endian ARM EABI5/v5TE REL. These are
  source/compile observations, not native ABI/launch or paired-image acceptance.
- Real original constructor, Digest/wire, graph, registry/parser and four Mbed TLS
  units were compiled. Original `rtsp_input.c` alone retains build01's isolated
  warning exceptions; no new handoff/test warnings waived. REL undefined symbols
  are libc/compiler support, not missing camera/crypto APIs.
- Build01 report records exact argv/environment, source/header consumption pins,
  tool executable pins, **1363 dependency records /243 unique paths**,75 artifacts
  including dependency/stack files, and all37 logs. These are retained metadata,
  not a full toolchain-closure or release packaging qualification.
- Original constructor's16 selected public source/evidence pins still matched
  byte-for-byte after build. Owned-runtime/constructor/old sources were not edited.

The PUBLICDUMMY build exports distinct `qc07f_rtsp_publicdummy_handoff_*` names,
requires a HOST fixture config/nonroot matched IDs and cannot move into native
config. A local synthetic native-shaped copy only exercises the unchanged
constructor; resulting numeric account is then explicitly PUBLICDUMMY, NOT native1.
The harness links no adapter/child/runtime/media/image launch code. Symbol
inspection preceded the single invocation.

**ONE invocation**, 2026-10-23 04:21:27.215727–04:21:27.217580 UTC, exit0:

- Fresh private directory under `/tmp/opencode`; original constructor/crypto call.
- Actual exclusive write1536 → fsync → known writer close → readonly open.
- Inode/owner/mode/nlink1/size1536/access/CLOEXEC/offset0 assertions before AND after
  explicit move into unused actual config; original owner remains unstarted.
- Entire caller material scratch observed zero; fixture password explicitly wiped.
- Explicit test-only single close of moved FD after clearing its slot, then
  inode-matched fixed-file unlink, directory close and empty-directory rmdir PASS.
- No payload readback/hash/dump, listener, child, media, device, proc, capture,
  process controller or network IO. URI strings only pass the existing parser.

The failure label makes no unconditional artifact-preservation assertion: its
handler wipes scratch/password and performs NO cleanup/retry. All functional
assertions precede success-only test cleanup; partial cleanup errors, if any, would
be failures with only the remaining filesystem artifacts preserved. None occurred.
No further positive, negative, fault-injection, fuzz, mutation or adversarial test
was run; deferred coverage is NOT passed. Native hardening/account1/writer-close-
error/namespace-race behavior is source-only, not runtime proof.

## Pins / artifacts / licenses

- C: `cd3e08600841502f1373c30ea80b43c736aa07e705bed7ec83f20f19d5f5903c`.
- Header: `4ad3136dfcf4e8fff5b3a26ab794279a96ee908466bfa547819dd87546fefa44`.
- `SOURCE_PINS.json`:32 selected new/frozen public source/context inputs.
- `runs/build01/REPORT.json`, `COMMANDS.txt`, `logs/`, `objects/` and both
  `private-handoff-*-NOT-EXECUTABLE.o`: compile evidence, generated objects local.
- `runs/positive01/OBSERVATION.json`, stdout/stderr: exact sole call and scalar
  observations only; no material file/content is retained in the report.
- `apps/rtsp-auth/rtsp-private-handoff/LICENSE_NOTICES.md`: new GPL-2.0-only source,
  unchanged camera notices and original Mbed TLS dual-license provenance. No
  upstream source/license was copied over or relicensed; no publication.

## Remaining runtime/adoption gaps and next boundary

Parent ordinary review is the next selected action; no native queue. In particular:

1. Real separate authenticated/authorized secret ingress, process/launch separation,
   capability and inherited-FD/environment policy, independent RTSP-domain choice,
   caller password memory/wiping and namespace/no-other-writer exclusion remain
   caller work. This C-slot FD transfer does not create IPC or prove separation;
   an actual launch/ownership integration must not copy/inherit competing owners.
2. Hardening function, root native checks, real image admission and backend adoption
   have not run. No fake callbacks, ready flags, default credential, image selector
   or test fallback supplies those prerequisites.
3. Storage provisioning/durability, durable generation authority, rotation/reset,
   linked-file lifetime/secure retirement and explicit startup-failure FD cleanup/
   retention need separately reviewed real integration. File fsync alone is not
   crash-safe storage; directory fsync and journal/replace policy are absent.
   Writer exclusion and linked-inode stability must continue BEYOND move through
   verifier startup/material consumption and separately reviewed retirement;
   moving custody does not authorize namespace mutation or a new writer.
4. Actual bootstrap/external media owner/recovery/backing,128KiB-compliant media
   buffers,40MiB/CMA combined-load admission, main/packaging and native authenticated
   PLAY/decode/finite STOP remain blocked. No G3/product/adversarial acceptance.

Only the two authorized fresh directories were written. Git/index/central plans/
AGENTS/old source were not changed. No device/hardware/current-state/HOLD query or
mutation occurred; both native HOLDs remain untouched. Historical held U-Boot is
not a new runtime observation. Parent review must precede product adoption and any
native effect; all effects in this lane are exhausted.
