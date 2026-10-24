# Authenticated span server v2 — independent ordinary SOURCE/API/order review

2026-10-24; RP-09/B07, bounded STREAM-01/02/03/07 and CAP-01/05/07.

**Disposition: CORRECTION REQUIRED — residual R1 bypass below.** The three
revised cleanup functions correct their original local R1/R2 defects, including
the R2 exactly-once acknowledgment ordering. However, an unchanged real caller
still retires subscription/playing state after `guarded()` has observed a sticky
clock fault, before reaching those functions or retention. Therefore this exact
revision does not receive the requested source-wide retain-before-cleanup
order/lifetime sufficiency. This is an ordinary static finding, not an executed
fault case, native/cap/runtime acceptance or parent adoption decision.

## Blocking ordinary minimum finding — R1 bypasses the corrected functions

Locations refer to `apps/rtsp-auth/rtsp-span-server-v2/rtsp_span_server.c`.

At **672–675**, after successful producer acquisition and while the actual frame
bracket remains open, each playing consumer is checked by `guarded()`:

1. Its first actual `camera_rtsp_clock_ns(&server_clock)` can return MAX and set
   `server_clock.failed` (94–103; `framework/include/camera/rtsp_clock.h:11–19`).
   This is an **already observed** fault, not an unobserved interval hypothesis.
2. `guarded()` returns false. The branch at675 immediately calls the real
   `camera_span_hub_unsubscribe_v1`, then sets `playing=0` and `deferred_close=1`.
   No sticky-error check or `retain_span` precedes these effects. The hub can
   ordinarily accept this unsubscribe: consumer CPU acquisitions occur later in
   `send_access_unit`; unsubscribe only refuses a held acquired bit
   (`span-hub-v2/span_hub.c:73–79`). Actual demand ownership remains live.
3. Only later, after the original producer release at688, the sticky MAX at691
   reaches `retain_frame`/whole-process retention. Eventual retention cannot
   restore the subscription/playing state already retired at675. Neither the new
   `pause_client` nor `close_client` entry fence is on that earlier branch.

A second instance of the same bypass is **290**: after subscription at287, a
clock-failing `guarded()` directly unsubscribes before returning to the newly
fenced `close_client`. This branch likewise does not pass through `pause_client`
before its unsubscribe. These paths are byte-identical to frozen v1; they are
not newly introduced by v2. Byte preservation is not proof that they meet R1.

**Minimum correction needed for the requested R1 claim:** distinguish observed
sticky clock/ownership uncertainty from ordinary guard/auth refusal at these
direct unsubscribe branches, and enter the existing whole-process nonreturning
retention path before unsubscribe/playing retirement when uncertain. Preserve
ordinary known-safe denial handling. Do **not** route the open-frame branch
through detach/STOP, invent an END receipt, retain all ordinary timeouts, or
claim whole-interval clock prevention. This review makes no source correction
and selects no expanded implementation scope or next queue; parent disposition
of this inherited bypass is separate.

## Corrected local R1/R2 paths

- `finish_au:356–385`: preexisting `server_clock.failed`, `lifecycle_error` or
  `lease_uncertain`, and its initial fresh MAX, retain before diagnostics, exact
  CPU release, active flags or partial-packet wipe. The fresh check at373 follows
  potentially blocking TCP/AU diagnostics and precedes release. Failed exact
  consumer-token/generation release still retains before active flags clear.
  Checks at380–382 catch newly discovered pending-control/auth/clock faults before
  packet wipe/failure logging/close. Guard/control failures from the AU sender
  reach these fences. Ordinary send/auth/deadline failure without ownership or
  clock uncertainty still releases the exact safe CPU lease and closes the TCP
  connection. A new fault after a previously safe release retains current state;
  it does not require reconstructing the earlier acknowledged lease.
- `pause_client:175–198`: sticky/fresh-clock entry checks precede unsubscribe and
  playing clear. The hub unsubscribe has no callback/reentry under the trusted
  contract. A subsequent MAX before detach retains current state without falsely
  acknowledging demand retirement.
- **R2 corrected, 184–195:** `demand_attached` remains true through ONE actual
  detach and its post-clock. Only detach OK plus valid post-clock clears it.
  Non-OK detach remains the actual error even if the post-clock also fails;
  OK+MAX becomes IO error. Error is latched before original `failure_client`,
  restored afterward, and immediately enters nonreturning retention before flag
  clear/return/another cleanup attempt. Notification is not acknowledgment.
  Actual demand scalars remain in the live record. Earlier unsubscribe/playing
  changes are not asserted to acknowledge detach. Common detach checks bridge
  time then calls the actual producer detach, whose last-demand drain may notify
  (`rtsp_server.h:123–128`, `producer.c:108–115`). Stable synchronous metadata-only
  notifications/no reentry are essential; no asynchronous completion is inferred.
- `close_client:199–228`: entry uncertainty retains before native cleanup, fd<0
  return, pause or socket/input/peer cleanup. Fresh reads gate entry, post-pause
  close and post-peer local clear. Clean memset preserves the **actual** span
  binding, so a later fault on an fd<0 slot has a valid retention destination.
  Actual runtime cleanup at `owned_runtime.c:54–60` either retains or returns
  without reading/mutating the server clock; its return is not a retirement ACK.
- Unchanged `retain_span:54–61` latches actual source HOLD, calls the typed sink,
  invokes actual runtime cleanup if it returns, then repeatedly SIGSTOPs. It
  cannot return/unwind/free/close/END-retry. The contract retains the whole actual
  process/stack/cursor array, not copied tokens. No sink implementation, survival
  guarantee or independent external-owner/recovery mechanism is supplied.

## Exact source and retained evidence

Read full v2 C/header/recipe/notices, contract/handback/proofs/final metadata,
full frozen v1 independent review and actual old/new three functions; inspected
accepted hub v2 and span builder C/headers, real common clock/detach/producer,
private owned-runtime cleanup/start/finish/auth and listener custody as relevant.

SHA-256. App prefix: `apps/rtsp-auth/rtsp-span-server-v2/`; evidence prefix:
`docs/qualification-plans/rtsp-span-server-v2-20261024/`.

| File | SHA-256 |
| --- | --- |
| app `rtsp_span_server.c` | `8bebe38b59a2690921074bb1fd348c2504503716433b0d416925a7e2592cc020` |
| app `rtsp_span_server_v1.h` | `ae1fcbe656a5ded4fba5b06c7d5114ee9652f9287187df66c65c3b2b8333a7b1` |
| app `build.py` | `52f010b9ce4a67d634ed13e7877923458af5c0ee37ff50965ec6f19664e78e18` |
| app `LICENSE_NOTICES.md` | `2ff6da57865eea15d4dd52825094f30ab51d9174f084e9ff221574dec414bfba` |
| `CONTRACT.md` | `17b43a0ca8201a113fc158e30e88e31caa21a6ed5eb54d48e38c91c8ae3eddb1` |
| `HANDBACK.md` | `fc75baeb4dd3c549c90263e41662e6f9f375ecb0e017f4996780b8bc48197d0f` |
| `FROZEN_INPUTS.json` | `6354023c9dac58cbdd35195318ef57466dfe5573e251a22424b5a3e6ca5e4c21` |
| `DELIVERY_PINS.json` | `c9139edc4740aa33f551d1e547222cea7509d59983093472c9e6c8753833e44f` |
| `AUTHOR_SOURCE_PROOF.json` | `a24b36d68cb60310209f84bcfcc9584fa362edac3cc52b0ae4aa1e14eccd21a7` |
| `SOURCE_DIFF_PROOF.json` | `1e909be15edc69daf68b2f716ff639a6688ffca3625c9a73a3a983b39b82dcaf` |
| `AUTHOR_FINAL_VERIFICATION.json` | `5cafcafd2435e95bc2307eeaff668100bf77541f5c90b5476bd559dadcd3890c` |
| `build01/REPORT.json` | `a4fb08f2c98e0c24eefe50361079fd90f1ad415aaed8b6f00c69edc0dd188d1a` |
| frozen v1 independent `REVIEW.md` | `5ae6708f00f3e5e6465730b9b47c6726278964a4bc3e294cba25bae8cf4b9871` |

Independent read-only stdlib inspection matched217 exact frozen members/bits,
203 build inputs,251 new dependencies,363 reused dependencies,98 artifacts,
26 exit0 hashed command logs and113 delivery pins;899 distinct public paths
were hashed in that pass. Counts overlap. Both dependency memberships matched.
Consumed C/header/recipe/notices equal their delivered originals. All four saved
forward/inverse diff pairs regenerate and apply exactly in memory. **ALL C bytes
outside the three functions match**, not only15 selected old function bodies;
header noncomment bytes match. Same exports/types/guard: exclusive replacement
only, never old server/hub units or combined RELs alongside it.

Author evidence remains six real HOST/ARM C compiles, two rooted REL links and26
successful commands, NOT RUN. Six compile/two link logs are empty. Actual archive
bytes contain23 bodies per target; all46 match retained objects, and the SAME21
selected bodies per target match v1 build03 and fresh extracted copies. Old server
member9 and old packet_hub member17 are excluded. All46 source/role/production
define records matched; no fixtures/stubs/old builders/new crypto or whole archive.

Actual ELF-byte/symbol/relocation parsing independently found HOST ELF64/x86-64
REL259064B and ARM ELF32/ARM REL172324B, entry0/no program headers. Same18 API
roots as v1 build03, nonempty real GLOBAL FUNC roots and required live APIs,
no main/old server/old hub export. Retained project relocation closure holds
(2778 HOST/2082 ARM relocations inspected); libc/compiler/linker references remain.
All three raw Digest UND names remain explicitly present, LOCAL/NOTYPE/size0,
with zero retained references, matching saved readelf/report proof. Recipe adds
this proof without weakening the actual-relocation check.

REL hashes: HOST `f6863373166954b320af723c21f400c7e4cb49c790c42a62973d9546476484a0`;
ARM `1ca1a251e327180bf8e44340f18d07c2e6296d085452852c8a45167a0f2ff30d`.
Not final executable/native ABI/full tool-execution or stack closure evidence.
V1 remains frozen NOT ADOPTED; all earlier build failures remain unchanged.

## Conditions and stop boundary

Auth/listener custody, validated producer copy→release→END, pristine borrowed-hub
checks, distinct per-consumer hub tokens, packetizer/partial-send/whole-AU budget
and STOP/drain paths are unchanged source, not new runtime observations. Mandatory
valid stable disjoint storage/no aliases/concurrency/reentry/signals/FD mutation,
true-fresh hub after the last fork, admitted backing and whole-process retention
remain caller prerequisites. The blocking bypass above prevents blanket order
sufficiency despite the corrected local functions.

**Native use remains BLOCKED:** actual flat kind1/paid[1] hub contract, output1MiB,
raw backing,128KiB allocation cap,2MiB process/combined-load/native accounting
integration, external ownership/recovery/provisioning/main remain unresolved.
The independent memory lane is not this review's dependency or an adopted runtime.
No whole-clock-interval GO. Camera23h OFF/finalized and RAM75 C1/recovery block are
inherited context, not fresh observations; old failures/HOLDs remain intact.

Only this fresh review file was written. No candidate import/helper/build/program/
ELF/test/callback/auth/backend/verifier/provider/network/hardware invocation,
negative/adversarial/mutation/fuzz case, subworker, source/Git/index/AGENTS/central
plan/private/settings/safeguard change. Parent adoption is separate. Stop here;
no automatic correction, runtime operation or next work queue is selected.
