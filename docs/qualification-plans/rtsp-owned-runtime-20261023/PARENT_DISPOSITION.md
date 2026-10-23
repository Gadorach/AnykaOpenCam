# Owned QC07F → authenticated RTSP — parent source review

2026-10-23; CORE-03/04, CAP-05/07, STREAM-01/04/07, RP-08/09.
**Narrow conditional SOURCE/API integration sufficiency.** No correction-required
ordinary defect identified in the selected new glue and four copied-file changes.
This is not full provider/server acceptance, execution, deployment or a completed
camera feature/product gate. Parent reviewed separately from the source author.

## Exact candidate and compile evidence

- Source aggregate: `410d42008de738bd9b67cdf51ca0333ab72d96159f4fd52da20175128dff0cf0`.
- Source seal: `43238c695bde0c9c045740b47939e4cd0fc0caedd0352aa4f55617fcfa8e821a`.
- Final build02 report: `89981b98739f12931c1d2a4ff06c0f6284708f304c521b10b830e5b2fa68fa50`.

Parent freshly checked all57 source-file pins and reconstructed their aggregate,
all87 selected original input pins, all document/build-report pins, and final
build02's364 input,378 consumed dependency and50 artifact records plus62 command
logs. Counts overlap; they are not independent coverage measures. Every recorded
command exited0;46 are C compilations. Current app sources match final build02's
compiled snapshots. No compiler, candidate, generator, test, archive or ELF was
executed during this parent review; author HOST/ARM compilation is cited, not rerun.

Reconstructed `SOURCE_DELTA.patch` byte-for-byte from its direct-copy origins.
Exactly four copied files differ: native adapter C/header, native watchdog wrapper
and server C. Other copied auth/backend/listener/provider/sensor/memory/license
bytes remain unchanged. Build01 is preserved but predates the final corrections.
Archives/REL objects and local generated snapshots are not source adoption.

## Focused ordinary findings

1. **Real owned NEXT/keyframe path.** Force0/1 reaches the existing hardware job
   inside the original owned guard → lifetime/capture/receipt/cancellation/time
   checks → disable boundary. Compatibility NEXT remains force0. There is no
   direct unguarded producer call in the new glue or second native/watchdog stack.
2. **Stable startup order.** Prepare installs device-free config/context/callback
   addresses only. Explicit postfork bind requires the same actual READY owner/
   backend, calls actual one-time native initialization and then binds common
   producer/real SPS/PPS generation. No initializer, listener callback or readiness
   predicate secretly adopts devices, creates backing or supplies fake entropy.
   Last-fork ordering itself remains a trusted bootstrap prerequisite, not proved
   by the READY fields. No real app bootstrap is delivered.
3. **Packet and lifecycle custody.** Successful NEXT stores the actual packet/key
   until exact release. The server validates and copies to its hub before release;
   common END uses independent scalar facts afterward. PRE_NEXT/first POST_FIRST
   gates are connected. Void release errors latch and cannot become silent frame
   success. END/retire wrappers propagate failures through the real common bridge.
4. **Uncertainty before unwind.** Actual private ownership/history and native/
   bridge clock faults select nonreturning quarantine when native work remains.
   Failed NEXT does not return into the public partial-work-retired contract with
   retained uncertain work. Release/END/retire failures, denied admission after
   work, client cleanup clock faults and final unretired native state preserve
   actual process-local owners before unsafe return/unbind/FD cleanup. Ordinary
   no-work configuration refusals need not stop an otherwise device-free process.
   Successful retirement is corroborated from actual native retirement/closed
   owner shape, not a caller permission flag or cleared packet metadata.

The added cleanup boundaries do not fix arbitrary caller misuse or supply a
destructor: stable, disjoint, serialized, nonreentrant trusted C storage/FD/callback
use is mandatory. Precommit serve refusals leave listener/native custody with the
caller; failed setup/serve is not permission to exit, finish native owners or free
backing. The caller must already implement the documented retention policy.

## Remaining real feature dependencies

Quarantine retains FDs, stack and backing **only while the media process survives**.
SIGSTOP is not DMA retirement, an armed reset, a continuing external owner or a
survival guarantee. A real bootstrap/external media-process owner, independent
all-domain recovery, admitted backing, genuine private provisioning/RNG, app main
and packaging are still absent. No substitute metadata/PID/pidfd/Boolean is accepted.
Normal STOP is finite; no restart, reopen, automatic retry or post-memory
unactivated-abort implementation is supplied. The native10s/128-frame limits remain.

Inherited1MiB resident/hub sites remain unqualified against the128KiB allocation
cap; no allocation exception,40MiB/CMA fit or native ABI/timing/load claim follows.
Authentication remains loopback-only under the original policy. This is single-VCL
wiring, not general AU support, on-device browser video or completed G3/G8.
The candidate's functional checklist remains entirely NOT RUN.

Next concrete feature dependency is real application startup/provisioning/backing
and external-owner/recovery integration, not another diagnostic-image, clock,
receipt or build-semantics framework. Source acceptance alone does not admit
calling this candidate; resolve those prerequisites before native use.

No current target/recorder/owner state was queried. Both original native
FAILED/HOLDs remain untouched; the last observed target was historical held
original U-Boot12:12UTC Oct22. No hardware operation, transfer, image build,
installation, executable recipe or native queue is selected. Originals and author
seals remain frozen. Only reviewed source/notices/safe metadata may be committed
locally through an isolated index; no binaries/private material or push.
Git whitespace check reports only the unchanged final blank line in the two
byte-identical GCC runtime-exception notices. Preserve those upstream notices
and sealed source bytes; all other selected paths pass the whitespace check.
