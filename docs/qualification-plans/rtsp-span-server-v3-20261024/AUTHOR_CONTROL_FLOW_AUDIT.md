# V3 ordinary author control-flow audit — SOURCE ONLY

2026-10-24. Not an independent review, executed fault case, functional proof or
source adoption. References below are actual delivered v3 C line numbers.
Full frozen v2 C/header, full independent v1/v2 reviews, both contracts and parent
V2_DISPOSITION were read before correction; actual sticky clock, hub v2,
common producer/bridge and owned-runtime cleanup/frame retention were inspected.

## Every actual direct unsubscribe

| Site | Observed-uncertainty order / ordinary path |
| --- | --- |
| pause_client 177–195, unsubscribe181 | V2 unchanged: server_clock.failed / lifecycle_error / lease_uncertain and fresh MAX retain_span before unsubscribe/playing clear. No callback/reentry in actual hub unsubscribe. Detach flag clears only after one OK and valid post-clock; error notification is not acknowledgment. |
| PLAY rollback 290–295, unsubscribe294 | NEW: after guard false, sticky/flags/fresh MAX retain_span with actual c->span, demand and index before unsubscribe. All those checks false -> same actual unsubscribe and return -1. Failed unsubscribe still nonreturns. |
| attach rollback 296–302, unsubscribe301 | NEW: identical fence after actual attach failure, not merely before the callback. Thus a fault observed within that call is fenced before unsubscribe. Ordinary failed attach still follows the original rollback contract; no successful-demand claim, detach or retry added. |
| open-frame guard rollback 685–691, unsubscribe690 | NEW: guard false -> sticky/flags/fresh MAX retain_frame(actual source) BEFORE unsubscribe, playing=0 or deferred_close=1. Whole actual stack includes packet, frame bracket and all demands. Known-safe denial alone takes the old unsubscribe/deferred-close policy. No pause/detach/STOP/release/END on the uncertain path. |

There are exactly four actual unsubscribe call sites; none other is hidden in a
new helper. Hub unsubscribe 73–79 can ordinarily clear a subscription without an
acquired bit: lack of CPU acquisition was NOT permission to bypass these fences.

## Actual producer packet and frame bracket

* The only actual successful producer next call is666. Source packet validation
  remains original; it stores independent scalar facts, not retained input views.
* 669–674 checks ALL client lifecycle_error/lease_uncertain and sticky/fresh clock.
  On uncertainty, actual retain_frame is reached with the native packet and source
  frame still live. The old observed-MAX producer release at v2:660 is GONE.
  Failed next likewise cannot reach an END on one of these observed conditions.
* 685–691 performs no producer release on guard denial. Observed uncertainty is
  handled as above, while known-safe denial preserves subscription policy.
* 698–706 retains original source revalidation and actual capture_admitted call.
  AFTER that possibly blocking gate, all flags and sticky/fresh MAX are checked
  BEFORE actual publish. This catches sticky fault even if a later gate result
  would otherwise look admissible. No gate/auth/readiness implementation changes.
* 708–712 repeats the flags + sticky/fresh clock fence immediately BEFORE the
  **sole remaining producer release**. It applies also when packet validation,
  eligible count or ordinary admission denial skipped the copy. Hub publish has
  no callback/reentry under its unchanged trusted contract, but the fresh read
  still detects a fault first observed after the copy and before release.
* Safe success remains shared copy706 -> producer release712 -> fresh post-clock
  715 -> exact END718. Safe ordinary denials keep the existing published/result /
  keyframe mapping and release/END path, not unconditional process retention.
  Source packet/payload is never read after release. The producer release is void;
  this does not invent an acknowledgment, independent DMA-stop proof or END time.
* A fault first discovered by the post-release read or inside END retains the
  CURRENT process/frame. It cannot reconstruct a safely released earlier packet;
  no such claim is made. Callback-internal native/bridge ownership guarantees stay
  the actual callback's responsibility, not supplied by these server fences.

Other END sites656/676 remain through end_checked495–500: sticky/fresh MAX
returns BEFORE actual lifecycle END; non-OK immediately retains actual frame.
Post-BEGIN ordinary admission denial still ends its exact frame with failure;
no next/producer release was acquired there. Begin's existing source normalization
and validation are untouched; no native ownership or malformed-pointer guarantee
is newly asserted. No retry or receipt framework was added.

## Other retirement / local invalidation callers

* finish_au366–395 is EXACT v2: entry and post-diagnostic uncertainty fences precede
  exact consumer-token release384; failed release nonreturns before active flags.
  Pending control/auth discovery is checked before partial packet erasure393.
  Every sender guard/control/deadline/send failure continues through finish_au or
  fenced close_client; no direct CPU-release bypass exists in send_access_unit.
* close_client199–228 is EXACT v2: sticky/flags/fresh MAX precede native cleanup,
  fd<0 return, pause, socket/input cleanup and local clear. Post-pause/post-peer
  clock boundaries stay intact. The actual span binding survives clean memset.
  The existing pending-backend refusal branch is not a source/detach ACK.
* demand_attached has ONE zero assignment195, after real detach OK + valid
  post-clock. Its sole true assignment303 still follows actual attach success.
  The new fences do not touch either assignment. Only pause/close or the fenced
  open-frame ordinary-denial branch clears playing; deferred_close is unchanged.
* retire_checked477–482 and idle_checked483–488 gate real retirement on fresh
  time, detect post-call faults, and their callers either mark retirement_failed
  then reach typed retention, or directly retain. No retry/STOP repair is added.
* clients_done737 onward still invokes real native cleanup (which either retains
  or returns without mutating server clock), then retains observed clock/global
  retirement faults BEFORE client cleanup. Hub STOP/drain/zero-token and actual
  source STOPPED/no-demand/no-active checks still precede guard unbind/listener
  close-once. Per-client close entry checks cover client flags/fresh clock.
* Input/header/receive zeroization and parser retirement are unchanged secret /
  control handling, not source-packet or CPU-lease acknowledgment. This audit does
  not claim that absolutely no diagnostic/statistic/parser scalar ever changes
  after a fault. It covers the selected subscription/frame/packet/cursor/demand
  retirement contract, not every state field in imported code.

No additional concrete bypass of that bounded observed-fault retirement contract
was identified in this ordinary source audit. This is not blanket correctness or
negative/adversarial coverage; unaccepted v1/v2 and original failures stay frozen.

## Actual retention destinations and trusted limits

retain_span54–61 and retain_frame62–68 are byte-identical to v2. Actual owner HOLD
is latched; actual frame notification and typed whole-process sink are used. If
the sink returns, existing runtime cleanup precedes repeated SIGSTOP. No exit,
abort, close, free, cleanup unwind, release or END retry is added to these paths.
Source-index4/NULL demand means whole server ownership, NOT no ownership; the live
stack retains actual packet, native context, frame bracket, demands and cursors.

The existing sink must actually retain the whole process/stack/external media
owner, not copy tokens. It remains an external implementation/lifetime obligation,
not implemented by this derivative. Stable disjoint addresses, no mutable aliases,
serialized synchronous metadata callbacks, no reentry/signals/concurrency/FD
mutation and fresh hub after last fork are mandatory trusted preconditions.
Fences observe real faults only: no whole-interval C1 prevention, native ABI,
memory-cap/backing/process2MiB/combined-load or recovery/survival acceptance.
