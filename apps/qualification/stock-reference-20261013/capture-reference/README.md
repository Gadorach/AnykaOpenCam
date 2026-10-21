# Fixed finite stock capture reference — CANDIDATE_HOLD

**A real fixed syscall adapter is linked, but native entry is not admitted.** The
static ARM executable accepts only the absolute-path invocation shape with
`--native-stock-capture-reference`, checks inherited FD3 is closed, prints
`NO_NATIVE_LAUNCH` and returns2 before opening files/devices, allocating, issuing
ioctls or writing GPIO. There is no argv/env/build-flag bypass. Do not stage/run
it as a capture test. Parent owns all eventual native admission and recovery.

This is one bounded source unit, not a daemon/service/controller. It packages:

- Explicit old ARM32 wire definitions and compile-only original3.4 header checks.
- Exact pinned-file validation and24-block/34-real-request plan; sdpc is a NOOP.
- The finite capture/IR/processing/publication/retention state machine.
- Public synthetic MOCK operations exercising that state machine and its failures.
- Fixed native syscall operations in `native_adapter.c`, interposed only for host
  tests through `adapter_sys.h`, plus a hard-HOLD native main for static inspection.

`cr_adapter_bind` supplies real syscall callbacks; `host_test.c` supplies the
portable algorithm fixture and `adapter_test.c` interposes actual adapter syscalls.
The native main still never calls bind/run, and native adapter ENTRY independently
rejects before file/device access. Host-only explicit instance/clock/order seals
are absent from native builds. There is no enable environment variable or native
launch build flag. The old `CR_UNBOUND_*` operations have been removed: reference
installation and pre-sensor gate checks now have concrete implementations.

## Closed source contracts incorporated

1. Exact full tuning file69716bytes, SHA256
   `719e84d0093887d10c81139babdc9410602c384d1874c64e2338c1a69c529332`, fixed target
   path `/usr/local/isp_sc1346.conf`; first mode0/style0/sensor1346 record begins
   0x200 and ends0x882a. Parser checks24 exact IDs/sizes, sensor ID0x1c/length560,
   original reset/off entries and original140th stream-on entry. No tail append,
   modified count on old139 arrays or reading of those private/runtime files here.
2. Parent copy-extent receipt binds34 actual requests and offsets. The build script
   independently compares the code table against that receipt. Every source span
   is inside its serialized block; request encoded size4 is never used as its copy
   size. sdpc at block16+88 has no ioctl. AE enum/data are preserved, not guessed.
3. Exact flags3/mask2 ION direct mapper is uncached/unbuffered; owned RW guard
   prefill is source-eligible. Legacy iolock checks338 main-prefix PFNs but takes
   no persistent allocation reference. Full2048-page ownership is independent.
   No ION_SYNC is added. This is source closure, not measured captured integrity.
4. Four2MiB slots in one8MiB backing; complete main/sub/trailer extent1729024bytes;
   only921600 main-first-plane bytes used as the luma candidate. Advertised YUYV
   size1843200 is not the layout authority. DQ main-size length/bytesused1382400 is
   distinct from retained2MiB backing. No trailer/UV/private old-memory export.
5. A separate **0x384000-byte/900-page** reference owner, not4MiB and not output
   guard reuse. Verify all900 PFNs/nonoverlap before zero-prefill. Set/get the exact
   three address/length pairs `[P,0x258000,P+0x258000,0x96000,P+0x2ee000,0x96000]`
   using0x40044967/0x80044968. P is measured from this owner, never assumed80800000.

Source authorities (workspace-relative):

- `evidence/qualification/qc07f/stock-reference-20261013/parent24-tuning-copyextent-review01.json`
- `docs/qualification-plans/stock-reference-20261013/feed-entry/retail-mapping-rea01/CONTRACT.md`
- `docs/qualification-plans/stock-reference-20261013/feed-entry/rea-start04/RESULTS.md`
- `docs/qualification-plans/stock-reference-20261013/feed-entry/PARENT_ADMISSION_REVIEW02.md`
- `docs/qualification-plans/stock-reference-20261013/feed-entry/buffer-contract01/CONTRACT.md`
- `docs/qualification-plans/stock-reference-20261013/feed-entry/retail-reference-rea01/CONTRACT.md`
- `docs/qualification-plans/stock-reference-20261013/feed-entry/PARENT_NATIVE_ENTRY_PROPOSAL01.md`
- `docs/qualification-plans/stock-reference-20261013/feed-entry/retail-clock-rea01/CONTRACT.md`

## Finite model and output contract

The fixture starts with root-child/instance-pin/FD3/source-clock/file-identity admission,
validated blob and observed GPIO23 baseline0. No caller-supplied paths/pins/request
numbers/sizes enter the CLI. The adapter checks fixed node identities, file
identity, capabilities, module inventory and syscall results. That inventory is
not a loaded-image hash proof; parent hardware-instance admission remains required.

The model retains from the **first ISP-open attempt**, even before activation.
It uses ISP then video, flags3/mask2 allocation/export/RW mapping of8MiB plus the
separate900-page reference block, all2048+900 PFNs within[0x80000,0x81800), verified
nonoverlap, owned-output prefill and explicit cold reference zeroing. Parent's
source proposal orders24 blocks/34 requests, reference SET/GET, full crop/main/sub
setup and final readiness before original full140 sensor load. This is a declared
controlled-client deviation, not normal-app equivalence. G_FMT/G_CROP are queried;
sub640x360 is source-bound successful private SET, explicitly **not** a made-up GET.
Reference GET is checked again after geometry. Only physical **0x20000000** word0
is read for bit20 while ISP/video clocks are owned; an unexpected gate fails,
never gets cleared. Full140 is marked active before attempting it. Four USERPTRs,
first-Q-attempt activation and one STREAMON follow. No APP FPS policy is imported.

Same stream:2s settle, then three3s target phases with GPIO23 levels0→1→0. Every
transition must pass SET then VERIFY of the expected latch level; its timestamp
is the bounded monotonic clock observation **after verification**, not the request
time or the nominal phase boundary. Poll
requests are100ms; capture elapsed budget25s from entry; up to512 drain iterations;
at most8 analyzed samples per phase/24 total. The source completion timestamp must
be at least verified-transition+500ms+admitted-clock-error, and no later than the
observed current clock+that error. Stale queued completions drain/requeue without
sensor sampling or thumbnail attribution; impossible future timestamps stop.
The300ms schedule and nominal phase windows only limit sampling frequency;
they do not supply freshness proof. Actual phase timing and throughput still require
native measurement; CPU/syscall time cannot be assumed from a host clock fixture.

Only completed/error-free dequeued slots are read. Validate type/memory/index/
USERPTR/main length, nonzero valid and globally increasing raw timestamp, per-slot
timestamp/sequence (equal sequence with new timestamp is valid), local ordinal and
generation. Sequence wrap/backwards and duplicate timestamps reject; observed
sequence/time gaps are counted. DONE is not required because legacy DQ clears it.
Reject unchanged0xa5-prefill luma, verify every spare byte while dequeued,
calculate full-luma CRC/mean/16-bin histogram and8×8 box-downsample160×90, then
requeue a fresh zeroed68-byte descriptor. No luma pointer survives requeue.
Completed uniform/zero luma is allowed as a **flat, unconfirmed image observation**,
with `flat`, `zero` and `image_confirmed:false` fields. This does not weaken CRC,
guard, completion/freshness or ownership checks, and is not sensor/IR-absence proof.

Allowlisted12 sensor register reads bracket sample processing:
`0100,320c,320d,320e,320f,3e00,3e01,3e02,3e06,3e07,3e08,3e09`.
They are raw register values, **not calibrated exposure/gain units or exact
exposure-time association**. Original AE remains unchanged. All output is labelled
`QUALITATIVE_AE_CONFOUNDED`; it cannot establish absent IR or a controlled manual-AE
comparison. ISP colour/IR-cut/white/speaker/motor/reset/ambient/FPS controls are absent.

The last analyzed sample supplies each phase's one thumbnail/statistic/readback
receipt. Up to8 sample CRCs fold into a phase checksum; other raw frames and
thumbnails are not retained. Three JSON records each contain length, dimensions,
SHA256 and complete Base64. Version2 records publish the **verified** `level`,
`verified_transition_ms` and `source_clock_error_us` alongside the raw completion
timestamp; these are not optical acknowledgements. Chunks are at most512 bytes (last may be
shorter). The terminal JSON is separate. Total cap61440bytes; output budget10s and
global bound40s. This is direct byte output, not a server/file/media endpoint.
Parent may later decode *actual* frames into a fresh private directory; this unit
produces only synthetic host data and does not render camera imagery.

### Completion-clock correspondence is a pre-effect admission

`cr_entry.source_clock_admitted` must explicitly equal1 before tuning-file or
device/IR operations. `source_clock_error_us` is a backend-proved symmetric bound
between the raw completion timestamp and the monotonic clock used for verified
transitions/deadlines. Zero is valid only for an explicitly proved exact domain,
as in the public host fixture; an omitted admission flag fails closed. A large
uncertainty does not relax the lower bound: if no phase gets a qualified sample,
the run fails without publishing images. Future checks use the DQ-return clock,
not the later time after processing. Clock arithmetic is overflow-checked.

Exact retained `ak_camera_dma_irq` writes timeval from
`jiffies_to_msecs(jiffies+29999/30000)` at lines44–56 and131–145, and can emit zero
on an invalid-buffer branch. This is **not do_gettimeofday/CLOCK_REALTIME**. The
stock header's INITIAL_JIFFIES is unsigned(-300*HZ); at HZ100 its32-bit start is
0xffff8ad0, the branch threshold in the retail IRQ. Tick quantization, the one-tick
branch difference, boot epoch/monotonic correspondence and both jiffies/millisecond
wraps must be bound by a later adapter. This source arithmetic alone is not that
admission, and the core never guesses a REALTIME epoch or adds a timestamp offset.
Native entry additionally remains HOLD for **unbound source-clock correspondence**.
No native clock was read or calibrated by this unit. The parent's1500ms backlog
counterexample is a permanent host regression case; dequeue delay does not prove
freshness of queued completions.

The subsequent exact retail clock report proves uint32 multiply-by10 and
non-atomic timer-list sampling; it does not prove a session-wide symmetric bound.
**The real adapter does not yet implement the native pointwise A/B/J witness and
phase-token filter and keeps native clock admission false.** No100ms policy is
silently used as calibration. The remaining code binding is at before-POLL/after-DQ
and phase verification: preserve every completed DQ ordinal/generation/guard, drain
old/tied tokens explicitly, and permit optical sampling only with the report's
per-event and phase fences. No hidden backend requeue/EAGAIN substitution is added.

## End/error policy

After any retained boundary, no close/FREE/munmap/exit capability is provided by
the core interface. Attempt GPIO23 restoration0 once after any output attempt,
verify the returned level, optionally attempt STREAMOFF, and retain **all** owners
regardless of STREAMOFF success. Failed restoration records reason91 and requires
parent/user full-reset recovery; no physical-off claim. Failed output leaves an
incomplete record, never an invented complete image.

Cancellation is an injected sticky flag in host fixtures; recovery restoration
and one bounded terminal receipt still occur. MOCK stop returns125 for inspection.
In an accidentally returning native stop callback, the core repeatedly stops/
waits instead of returning/releasing owners. Real POSIX signal/ALRM/nonblocking
write/poll callbacks are now implemented and syscall-interposed in host tests;
they have not run natively. Signal handlers only mark cancellation. UART output
uses a fresh fixed `/dev/ttySAK0` open-file description, checked against stdout's
character-device identity, with O_NONBLOCK and no parent-FD F_SETFL/termios edits.
Blocking stock ioctls cannot be made preemptible
by these elapsed-time checks. Parent's60s stock watchdog/full-reset ownership is
external and is neither opened nor renewed here. No module unload/reset/controller.

## Remaining bindings before any native implementation/admission

1. Implement/review the exact pointwise completion-clock/phase witness binding
   from `retail-clock-rea01/CONTRACT.md`. Current source-clock admission remains
   false in native; host exact dummy domains are not a native clock theorem.
2. Parent review/seal of this exact source/binary, fresh loaded-image/module/provider
   identity, root/USR/console/sole-owner conditions and actual reset boundary.
   Existing source proposal and host tests are not permission to stage/launch.
   The native main and ENTRY remain unconditional HOLD; no admission toggle exists.

Normal post-capture retirement is deliberately not claimed; retained-until-reset
is the qualification boundary. No private config read is needed to select the
explicit pinned reference pack. Current recorder PIDs/timers are never authority
in this candidate.

## Reproducible host/build unit

`python3 -B apps/qualification/stock-reference-20261013/capture-reference/build.py buildNN`
requires a fresh output name under the matching delegated evidence directory.
It compiles/runs only CR_HOST synthetic binaries in optimized and ASan/UBSan
modes, then statically cross-links ARM and inspects it without execution.
Original3.4 header comparison uses a clearly isolated **compile-only time32 shim**
because musl1.2.5 uses a modern time ABI. Runtime packets never use libc timeval.

Mbed TLS3.6.7 SHA256/Base64/platform-zeroize/constant-time code is read from the
existing pinned vendor tree; original license remains there. No homemade crypto,
vendor modification or workspace-wide license grant. Build receipts pin every
dependency/header/runtime library/compiler/link input and source snapshot. Earlier
failed builds are preserved; source mock success is not hardware success.
