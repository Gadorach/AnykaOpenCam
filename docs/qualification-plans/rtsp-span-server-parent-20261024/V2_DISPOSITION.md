# Authenticated span sender v2 — residual R1, NOT ADOPTED

2026-10-24; RP-09/B07, bounded STREAM-01/02/03/07 and CAP-01/05/07.
Parent read the complete v2 review and actual affected call sites. Accept its
**correction-required residual R1 finding**. Local pause/close/finish_au fixes and
R2 detach acknowledgment are source-sufficient in their local scope; that does
not establish source-wide retain-before-cleanup ordering for this revision.

Direct unsubscribe after clock-failing guarded() at v2 C290 and675 bypasses the
corrected cleanup functions. Subscription/playing state can be retired after an
already observed sticky fault, including while an acquired producer packet and
real frame bracket remain live. Later retention cannot reconstruct that state.
These are inherited ordinary static control-flow defects, not executed fault tests.

Parent additionally identified adjacent source-packet cleanup under the same R1
rule: C659–661 calls producer release after observed MAX before retaining; the
precopy gate can also observe a fault before C688 release/C691 retention. A fresh
correction must retain that actual packet/frame/process before release on observed
uncertainty, without treating ordinary auth denial as global uncertainty or
routing the open frame through detach/STOP/fabricated END. No whole-interval
clock-prevention claim follows from these fences.

V2 C `8bebe38b59a2690921074bb1fd348c2504503716433b0d416925a7e2592cc020`, header
`ae1fcbe656a5ded4fba5b06c7d5114ee9652f9287187df66c65c3b2b8333a7b1` and review
`818fb2e69131c44bb6d5bd31b0dfc7849da31407cc4df9852e7bfb077dc6c22d` remain frozen.
Its six HOST/ARM compiles/two rooted REL links/26 commands remain compile facts,
not lifetime acceptance. No runtime/test invocation or historical relabeling.
V1/R1/R2 and original build01/02 failures remain independently preserved.

One fresh rtsp-span-server-v3 author owns this bounded call-site/source-release
correction. Independent ordinary review and parent disposition precede any use.
Accepted hub v2/span packetizer and chunk-memory source are not revoked or
automatically integrated. Disjoint actual native runtime source work continues;
native1MiB output/raw/prefault/2MiB/combined-load/external recovery/main remain
open. Last camera boundary is OFF/finalized23h; RAM75 C1/recovery still blocked.
No negative/adversarial replay or hardware phase. This parent record is outside
all frozen author/review directories.
