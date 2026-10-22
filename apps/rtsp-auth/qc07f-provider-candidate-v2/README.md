# Frame + persistent-encoder FD custody — compile-only v2 candidate

`frame_fd_custody.c/.h` are **byte-identical v1 imports**, not corrected/adopted as
native merely by copying. `encoder_fd_custody.c/.h` add move-only custody for ONE
actual preopened persistent encoder description using the frozen new OPEN_QUERY.
No temporary-job role collapse, watchdog alias, provider-ready flag or app main.

Encoder take/give-back use GET-only fcntl/fstat plus only actual OPEN_QUERY:
O_RDWR/CLOEXEC/matching char-device and independently observed nonzero session ID,
successful syscall, version1/88/reserved0 and a consistent NONE/THIS/OTHER snapshot.
Transfer requires NONE/owner0/frame0, no observed fault/active ledger or telemetry
error/unknown-flag contradiction. Saturation/history/generation are retained as
actual diagnostics, not promoted to unique leases. No guessed global sequence.

All prerequisites/observations precede commit; failures leave bootstrap's owning
FD slot unchanged. Give-back observation failure latches sticky HOLD, retaining
the actual FD/identity/last valid complete snapshot; no retry/destructor/close or
HOLD clear. Recipient and initial owner must be explicit-1. Syscall failures keep
errno, including natural ENOTTY on the **unsupported frozen76 kernel**. No fallback.
Nothing invokes this helper on a target; that requires a future real paired kernel.

Trusted exact boot/image/source association and sole serialized custody (no
dup/fork/SCM_RIGHTS alias, concurrent I/O/fd-table mutation/close/reentry) are explicit
preconditions; metadata/query cannot prove them. No DMA health/retirement, normal
close, never-used, backing, authority, readiness, alias exclusivity or kernel pairing
is certified. This record is not ownership of a failed-next process/DMA/backing.

## Build only

```sh
python3 -B apps/rtsp-auth/qc07f-provider-candidate-v2/build.py fresh-name --toolchain /path/to/retained/armv5-eabi--musl--stable-2025.08-1
```

Writes fresh `docs/qualification-plans/qc07f-provider-v2-20261022/runs/<name>/` only.
Pins original provider/query/new-compiler/runtime seals and original source/evidence
snapshots before/after; verifies exact v1 imports; snapshots actual UAPI, own source,
license/tool/header closure; compiles HOST/ARM, archives and links ELF REL only.
No driver/UAPI/unrelated suite rerun, executable/listener/test/synthetic/runtime,
full kernel/module/image build, target/private/network or Git/index operation.

New code GPL-2.0-only; imported SPDX/source/runtime license/provenance remain intact.
128KiB/40MiB/CMA, native2MiB/HOST4MiB child split/strict image pin, loopback policy,
radio/reset/failed-next/memory HOLD stay unchanged. Complete sensor/temp-encoder/
watchdog/backing/failed-next/provisioning/listener contracts and G2/G3/G8 remain open.
