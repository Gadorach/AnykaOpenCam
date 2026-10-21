# 05 — Capture, memory ownership, V4L2/vb2 and DMA-BUF

Entry: [index](00-INDEX.md). Prefix: **CAP**. Depends: BASE, CORE-01/02/04,
PLAT resources. Coordinate with ENC; G2 cannot close on copied-queue fixtures.

## Established facts and hard constraints

Measured System RAM is `[0x81800000,0x84000000)` (40MiB). Current explicit CMA
is16MiB at `[0x82800000,0x83800000)`; compiled default0. Four-slot custom capture
has held-token, pre/post-copy CRC, guard, wrong/stale/duplicate return, timeout,
stop/reopen/close-held and recovery evidence. Kernel69 passed48 deliveries and
fresh48 after11102ms held expiry. These are delivered frames; raw IRQ/drop counts
are not individually integrity-qualified deliveries. Encoding still uses gated
capture, not this persistent copied queue.

Current allocation audit: codec2MiB input+2MiB output+two2MiB references+64KiB
aux =8.0625MiB. Four capture slots8MiB+ISP references4MiB+table16KiB+stats48KiB+
codec =20.125MiB before other obligations. Importing instead of copying encoder
input leaves18.125MiB. Proposed two-slot/import14.125MiB is arithmetic only;
another2MiB sink invalidates the16MiB fit. Do not free references just because
temporal filtering is disabled or shrink guard tails to force allocation success.

Relevant files: `isp.c`, `isp-stream.h`, `framework/{include/camera,src}/lease_pool*`,
`memory_budget*`, `docs/QC07F_PERSISTENT_COPIED_QUEUE.md`,
`docs/QC07F_CAPTURE_LEASE_EXPERIMENT.md`, `docs/STREAMING_MEMORY_AND_BACKPRESSURE.md`.

## Tasks

- [ ] **CAP-01 — Peak-memory/lifetime design.** Enumerate live DMA allocations,
  retained exports/imports, statistics, guard extents, atomic pool, alignment,
  user copies, packet buffers, network/recording/UI stacks, logs and filesystem
  cache. Distinguish reservations from physically allocated pages and CMA
  availability from reclaimable pressure. Use the checked memory-budget helper;
  measure headroom under fragmentation/repeated opens and the combined workload.
- [ ] **CAP-02 — Fewer-slot feasibility.** Recover exact enabled-slot mask,
  address/latch, completion selection and no-buffer behavior. Design a bounded
  two-slot experiment with all inactive DMA destinations safely owned, including
  any necessary sink. Prove selected-slot retirement while another captures,
  reject overrun/order errors, and qualify starvation without pointing hardware
  into a held slot. If hardware needs more buffers, revisit the memory design
  openly; do not label two-slot assumptions as observations.
- [ ] **CAP-03 — Source and format model.** Define media topology for the sensor,
  ISP and main/sub outputs; each physical sensor has explicit format negotiation,
  crop/compose, strides, plane extents and timestamps. Main+sub1728000bytes is
  not interchangeable with main-only1382400bytes. Enumerate supported formats
  honestly and preserve frame metadata across capture/encode/services.
- [ ] **CAP-04 — V4L2/vb2 capture implementation.** Implement query/format/buffer
  setup, REQBUFS/QUERYBUF/QBUF/DQBUF, STREAMON/OFF, poll, mmap, sequence,
  timestamps and appropriate events using standard kernel helpers. Define the
  correct DMA memory backend for this noncoherent ARM platform. Done/error
  buffers follow real IRQ retirement, not observed changing pixels. Handle
  partial probe/start failures, simultaneous opens and close during streaming.
- [ ] **CAP-05 — Export/import lifetimes.** Add DMA-BUF export through standard
  mechanisms and a qualified encoder import path. Validate size, plane offsets,
  alignment, DMA aperture, mapping/scatter-gather support and device ownership.
  No arbitrary user physical addresses. Export FDs outliving STREAMOFF/close
  keep storage alive; generation changes prevent stale completion/requeue.
  Wait for actual DMA retirement and synchronization before detach/unmap/free.
- [ ] **CAP-06 — Integrity with shared memory.** Specify CPU/device cache
  transitions and synchronization for each read/write. Keep complete owned-frame
  CRC/guard checks in qualification. Main-only encoder input must not treat valid
  sub-profile bytes as guard corruption. Use shared valid extents and independent
  guards for statistics/output/references. Changed ownership design requires
  new integrity evidence; matching checksums cannot excuse concurrent mutation.
- [ ] **CAP-07 — Generation, cancellation and starvation.** Standardize lease
  handoff across vb2, backend and encoder. Test duplicate releases, old-generation
  IRQ completion, close-held, dequeue cancellation, device loss, requeue after
  restart, queue-empty pause/resume and outstanding consumer references. Failed
  reset keeps memory alive and reports a failed device state.
- [ ] **CAP-08 — Capture/encode overlap.** After separate ownership/import tests,
  connect a small measured pipeline: capture one owned frame while encoder holds
  another, then retire/requeue exactly once. Add timeline evidence of overlapping
  hardware work, source stability and reduced per-frame time. Network/recording
  hold encoded leases, not raw DMA slots. Stop drains both engines in order.
- [ ] **CAP-09 — Remove artificial session renewal only when justified.** The
  current10s/128-frame diagnostic limits are safeguards, not a production timing
  design. Replace them with explicit health/deadline/recovery contracts after
  long-lived queue ownership passes. Do not disable watchdog/CRC/retirement just
  to avoid renewal pauses. Retain the gated path as an explicit rollback mode.
- [ ] **CAP-10 — Standard-interface validation and adapter migration.** Run
  relevant `v4l2-compliance`/v4l2-ctl tests plus meaningful ARM lifecycle/import
  and fault tests. Move the common QC adapter onto this path. Keep lab UAPI
  compatibility documented; never call a renamed private ioctl “V4L2 support.”

## Gate

G2 capture contribution requires actual standard buffer operations, correctly
retained mappings/imports, repeatable allocation under real memory pressure,
no source/guard mismatches and proved overlap on the intended profile. Produce
a buffer state diagram, exact allocation table, per-frame timeline and tests for
every free/reuse edge. If any shared-buffer ownership remains unproved, keep
production queue/zero-copy/overlap flags false.
