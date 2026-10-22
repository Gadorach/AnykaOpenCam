# B08 parent disposition — frozen tail not yet accepted for reproduction

2026-10-22; B08 / RP-08–09 / SEC-05/11 / STREAM-04/07.

Parent read the independent source review, its public-only collector, and actual
`client_test.py`, `integration_test.py`, `public_dummy_material.c` and
`candidate_auth.c` in `apps/rtsp-auth/isolated-server-candidate/`.

## Accepted findings, not test acceptance

- The new BAD-DOMAIN generator mode sets UI domain while keeping generation1,
  matching fixture generation1. The actual server rejects foreign domain in its
  routing-prefix check **before** `rv_start`; this is not an exercised child-engine
  domain rejection. Old integration05 generation-mismatch evidence stays distinct.
- **R1 confirmed:** the fixture is launched in `Server.__init__` before READY/
  refusal checks, without enclosing constructor cleanup. `finish` can itself time
  out after SIGTERM, and the builder's immediate Python timeout does not establish
  descendant retirement. No existing test run was observed leaking here; this is
  a source-supported failure-path gap. Do not run the unchanged builder unattended.
- **R2 confirmed:** expected exit2/no READY/zero protected effects is not a unique
  domain-rejection oracle. Require a fixed nonsecret observation of the actual
  domain predicate, matching remaining inputs, a paired valid-domain control and
  zero child launch on the rejected branch. Do not read/hash generated material.

The old frozen source and HALT evidence remain immutable. The review's pin/diff
results are static provenance, not fresh behavioral proof or product acceptance.

## Selected correction

One disjoint source worker owns a fresh derivative
`apps/rtsp-auth/isolated-server-host-tail/`, its `HARNESS_CANDIDATE.md` and
`harness-correction/` evidence. Implement only HOST harness lifetime/cleanup and
the domain-specific scalar test oracle. Preserve source-copy provenance and all
failures. No shared verifier/framework/old candidate edits, new native authority,
credential handling, budget widening or station work.

Standalone synthetic process-guard checks may run after inspecting that guard.
Full RTSP builder/generator/localhost campaigns and ARM builds require subsequent
parent review of the correction. Forced HOST process cleanup is failure recovery,
not normal verifier wipe/source retirement or a passing campaign. Unknown/unreaped
children must retain FAIL/HOLD. This HOST-only supervision is not a camera recovery
mechanism or permission to terminate uncertain target owners.

## Current boundary and next task

No RTSP builder/server/generator or test was run by parent for this disposition.
`parent-environment01.json` records only the nonroot HOST environment/compiler
availability and unchanged ordinary index. No target/device/private-input I/O,
native listener, native memory fit, source/handoff adoption, G3/G8 or NVR result.

Next: review the exact harness correction and its standalone failure tests before
selecting a new local synthetic RTSP campaign. P1's separately user-directed
original-worker clarification does not depend on or get completed by B08.
