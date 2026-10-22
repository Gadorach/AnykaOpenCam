# Parent review — additive encoder-open query / distinct object compile

2026-10-22; ENC-02 / RP-07/09 dependency. **Narrow static SOURCE/API sufficiency
for this observational addition; not paired-kernel/native/provider acceptance.**

## Actual source read and independently checked

Parent read the entire46-line UAPI,45-line handler and exact applied patch, plus
the original ioctl/stream-owner/release/open/init implementations at
`qc07f_hwenc.c:361–505` in the retained76 source. Parent independently reconstructed
the candidate bytes from the unchanged original using exactly three insertion
sites: include the handler, dispatch the new command before legacy lifetime QUERY,
and include the new UAPI in the original header. The reconstruction exactly matches
the sealed candidate. Handler/UAPI copies also match their authored source bytes.

- `encoder_open:445–457` initializes a nonzero numeric session under `enc.lock`;
  maximum-u64 denies later opens. No session allocation/identity behavior changes.
- `encoder_release:429–443` clears a matching owner before freeing private data,
  under the same lock. Calling-file lifetime is held by VFS during ioctl; another
  reference owner's private session cannot be freed during the locked snapshot.
- New handler initializes all88 bytes, reads the actual calling-file session and
  actual reference-owner relation/identity, and copies a bounded output snapshot.
  Failed session/trace/partial-copy paths return an error; no fabricated identity,
  hardware effect, allocation, reference clear, owner transfer or counter update.
- The legacy dispatcher/handlers/open/release/init remain byte-identical except
  those three insertion sites. The old exact command set returns ENOTTY for the
  new command; offline compilation does not add support to frozen76.
- Header alignment/static ABI declarations select88 bytes on HOST/ARM. Generation
  and operation diagnostics saturate; frame numbers wrap16 bits. They are history,
  not unique operation/lease identities, task authority or DMA retirement proof.

Query acquires the existing mutex and may wait behind a synchronous job. It is
**not time-bounded or a watchdog substitute**, even though the copy size is bounded.
No globally idle/healthy, alias-free, never-used, final-release or safe-close claim
is accepted. Sole-description ownership and exact boot/image/source association
still need their own bootstrap/admission contract.

## Actual build-method/custody review, no repeat compile

Parent read the fresh builder and result, verified all pins in both frozen source
seals (including the real object), all513 actual consumed source/header files and
15 recorded loaded compiler ELF images against current bytes. Six recorded tool
commands exit0, all old-evidence/input/compiler unchanged flags are true. No source,
compiler, tests, linker or target operation was invoked during this review.

The real whole-driver ARM object is19676 bytes, SHA256
`886c46f2dfe39d644d5aec2fb744461884e43b272d28eb56b35bab3bfaf8e2ee`.
It is retained as an authored compile result whose inputs/method are inspected,
not an independently rerun build. Only source/safe metadata are commit candidates;
generated objects and dependency/header snapshots remain local.

Clang23.1.3 is explicitly a **new SOURCE-object baseline**. Retained generated
compiler/config metadata still names23.1.1; this compile does not qualify that
compiler/config feature pairing or reproduce the original76 ABI. A future paired
full build must deliberately regenerate and pin a coherent compiler/config/header/
module/image closure in a fresh tree, preserving the original. That build and native
query are not selected by this review. The earlier build01 refusal stays valid.

The selected next real app-side increment is observational encoder-FD custody in
a fresh provider-v2 candidate, not a fake main or duplicated stream/temp owner.
Watchdog lifecycle, failed-next retention/backing/provisioning/owned listener,
128KiB/40MiB/CMA and all native/memory/G2/G3/G8 gates remain open. Both original
radio HOLD and reset-controller failure HOLD remain; current target is held U-Boot.
