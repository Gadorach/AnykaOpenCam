# Span hub v2 — independent ordinary R1 SOURCE/API/custody review

2026-10-24; RP-09/B07, STREAM-01/02/03, CAP-01/05/07.

**Disposition: narrow conditional SOURCE/API/ownership sufficiency for R1.
No correction-required ordinary defect found in this focused replacement.**
Both view validation and scalar release now bind to the actual consumer's
canonical acquisition token. This is not source adoption, runtime/native/cap
GO or acceptance of frozen V1. Parent disposition remains separate; no next
implementation, test or hardware queue is selected.

## Exact scope and pins

Read full V2 and frozen V1 production C/header, full original R1 review, actual
accepted RTP-span C/header and parent disposition; V2 CONTRACT, HANDOFF,
AUTHOR_AUDIT, recipe (text only), notices, forward/inverse diffs, SOURCE_DELTA,
build/pin/command/log/dependency metadata and relevant CAP/STREAM plans.

SHA-256 (workspace-relative paths):

| File | SHA-256 |
| --- | --- |
| `apps/rtsp-auth/span-hub-v2/span_hub.c` | `bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1` |
| `apps/rtsp-auth/span-hub-v2/span_hub.h` | `310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454` |
| `apps/rtsp-auth/span-hub-candidate/span_hub.c` | `40c6d84c1e78c29a78384bee4dc1f435a53032f31655e122949921275a5c2289` |
| `apps/rtsp-auth/span-hub-candidate/span_hub.h` | `8c45f279a09dce01b53d928e2fa024663093f6955c469d0b71bb18ba72947c47` |
| `apps/rtsp-auth/rtp-span-candidate/rtp_h264_spans.c` | `a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e` |
| `apps/rtsp-auth/rtp-span-candidate/rtp_h264_spans.h` | `41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91` |
| `docs/qualification-plans/span-hub-review-20261024/REVIEW.md` | `f89efc3252cc11c30037576b327bece77269f4953efbf80a2e1ebbb11710dccc` |

Evidence anchors under `docs/qualification-plans/span-hub-v2-20261024/`:

- `DELIVERY_PINS.json`: `2373e030c786ccec7e2d907fa9f11e6f8f9bc010bac036757e8522fb72af61e3`.
- `FROZEN_INPUTS.json`: `c97692a75864faf1b02e748cee92f46c1be5aeb6a0f0f8e085170e9d23916f7c`.
- `SOURCE_DELTA.json`: `9d86cb7e43feeddd2aa509bb55520c5ec6343a42b034e831a62a6849b278050b`.
- `V1_TO_V2.diff`: `780ca1f4a4098647674a17e5ae1a9d09fd76d6f65dd2dac1b8f74a9db5197d10`.
- `V2_TO_V1.diff`: `e756392117e70fc40772ad36af338e36e0f5b62388b67c9c0f85065af435aadf`.
- `runs/build01/REPORT.json`: `609b7588a343f7814ef75e3587150afc787d7a7a401d594a887a7d34a968d169`.

## R1 source finding

- **Acquire (`span_hub.c:143–165`):** all prior owner/chunk/consumer/subscription,
  output, ACTIVE-phase, busy and pending checks precede the added UINT64_MAX
  exhaustion check; every refusal precedes output/token/mask mutation. A local
  zero-initialized view receives `issued_token+1`, separate publication generation
  and actual consumer tag. Bounded view construction has no callback/refusal.
  Only success commits issued counter, canonical `active_token[c]`, acquired bit
  and output. Starting true-fresh at zero yields distinct nonzero 1..UINT64_MAX
  tokens without wrapping. No other counter assignment/reset exists, including
  release, unsubscribe/resubscribe, publication, STOP and disposal.
- **Validation (`179–196`) and release (`167–177`):** validator requires the
  actual consumer tag and its canonical active token, retaining subscription,
  acquired/ACTIVE, owner, nonzero token/generation, current generation, codec,
  keyframe, timestamp, byte/count and all eight pointer/size checks (unused spans
  included). Scalar release independently checks nonzero token/current generation
  against `active_token[c]` and requires its held bit; it does not rely on a view
  check. Success clears that canonical token BEFORE acquired/pending bits.
  Distinct issued tokens mean another consumer's unchanged view/token cannot
  validate or release this lease, even after its own proper release. A cleared
  zero slot cannot admit a second release with an old nonzero token. These are
  static deductions, not executed wrong-consumer or refusal cases.
- **Retention/drain invariant:** initially every token slot and acquired bit is
  zero. Acquire alone sets a slot/bit; exact release alone clears its slot before
  its bit under serialization. Publication assigns acquired=0 only after refusing
  any held bit; unsubscribe refuses held consumers. Backlog/resync preserve held
  pending bits, tokens and metadata. Thus acquired bits imply valid canonical
  tokens; no acquired bits implies all slots zero. STOP preserves held tokens,
  generation and backing, denies new acquire/packet effects, but STOPPED remains
  admissible to exact old releases. INIT_FAILED never acquired; STOPPED/drained
  disposal cannot free held backing under this invariant. Issued history survives.

## Preserved paths and evidence custody

Independently regenerated both saved diffs and applied each to text in memory:
exact opposite originals reproduced. C is **+7/-4**, only acquire/release/validation;
header **+16/-4**, fields/comments. Complete C prefix before acquire and suffix
from packet wrapper are byte-identical to V1: real allocation/input/copy/publish,
accepted-packetizer wrapper, STOP/drain/disposal are unchanged. No cap/policy
increase: eight separate 128KiB requests, one shared CPU copy, four consumers.

Fresh read-only hashes/resolved-path/size checks matched 282 overlapping records:
17 delivery, 19 selected inputs, 42 consumed dependencies, 12 tools, 59 artifacts,
20 command logs and **113 frozen records**. No mismatches. Exact frozen file
membership matched V1 source (6), V1 evidence (103), old review (1); the three
other frozen pins bind accepted packetizer C/header/disposition. Dependency-file
membership matched all 42 recorded lexical paths; tools matched retained V1 pins.
V2 source membership is exactly C/header/recipe/notices, no copied harness.

All 20 saved command records match REPORT, exit0, declared cwd/environment and
hashed logs. Four warning-as-error production compiles (hub + accepted packetizer
per HOST/ARM) have empty logs; two direct REL links/traces use exactly those real
objects. Actual ELF header bytes confirm HOST ELF64/x86-64 REL (7640B) and ARM
ELF32/ARM REL (5300B), no program headers. Saved ELF metadata reports EABI5/v5TE;
symbol logs resolve the real span builder, no main/old packetizer, with ordinary
malloc/free/libc/compiler-support references unresolved as expected. REL hashes:
HOST `3760bf130a175a215e0e9223920d529e88b39578d0dc7789d48bb18c227e6dff`;
ARM `2637896af43fc03bb6b9cf8328d5959376099135a6382aa70b5381bc4d87288a`.

## Conditions and open gates

True-fresh one-use stable-address trusted C owner, valid stable disjoint objects,
no mutable chunk aliases/moves/reentry/signals/fork/concurrency and serialized
operations remain essential. Tokens/self checks are local identity, NOT security
authorization, cross-owner/process/camera namespaces or hostile-pointer defense.
Retained V1 exports/guard do not preserve binary layouts: exclusive replacement,
all consumers rebuilt against V2; never both headers/hubs or old combined RELs.
Caller still checks leases before deferred effects and owns partial-send/commit
discipline. CPU copy/free is not producer/DMA release, END/ACK or recovery authority.

**V2 has zero candidate function/program/test calls or runtime observations.**
Compile-only author evidence was inspected, not rerun. V1's sole normal HOST PASS
used correct cursors only and remains frozen/unaccepted for R1. No normal rerun,
negative/adversarial/mutation/fuzz matrix or candidate/helper import occurred.
Only this fresh REVIEW.md was written; no source, old evidence/reviews, Git/index,
central plans, private/current-state/device/network/hardware or settings changes.

Native ABI/loadability, process2MiB, prefault/residency, transitive stack and
whole-load40MiB/CMA/TLS/headroom/cap acceptance remain unproved. Actual runtime
hub/native contiguous1MiB output/raw ownership/prefault ledger/main/provider and
external retention/recovery remain unadopted/open. Last OFF/finalized23h and
RAM75 recovery/C1 block remain inherited parent context, not fresh observations.
Separate parent disposition only; no automatic adoption or next queue.
