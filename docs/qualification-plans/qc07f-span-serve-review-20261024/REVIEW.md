# QC07F span serve — independent ordinary source review

2026-10-24. CAP-01/05/06/07, CORE-03/04, RP-04/09, B07; bounded
STREAM-01/02/03/07.

**Disposition: narrow conditional SOURCE/API/order/ownership/lifetime sufficiency.
No blocking ordinary defect found within the declared trusted-caller contract.**
Parent adoption is separate. NOT RUN; not runtime/native/allocation-cap/full-RTSP
or product GO. This review selects no execution, further work or hardware queue.

## Scope and exact pins

Read all seven delivered app files, full CONTRACT/HANDBACK/AUTHOR_AUDIT,
SOURCE_PROOF, all eight forward/inverse diffs, DELIVERY/FROZEN records and saved
build report. Compared the actual accepted runtime (parent closeout `5375584`),
sender v3 (`7f4c120`), unchanged included native adapter/import/provider/watchdog/
close-once paths, common producer, accepted memory, v2 hub and span packetizer.
These prior dispositions bound the comparison; author assertions are not proof.

SHA256, relative to `apps/rtsp-auth/qc07f-span-serve-candidate/`:

| File | SHA256 |
| --- | --- |
| `owned_runtime.c` | `384bac52a5142bd3f6888715a3e4d9b6e061a85433d4267e11f046b1307bba10` |
| `owned_runtime_v1.h` | `a0c620b2e4aba7cbc19c4f1f6c53b6c8a3ecc46e6bff862c0a3f6abceca06815` |
| `rtsp_span_server.c` | `1b3d9c67341832acb76f25ea84052bd1321cbaf85ae6b6aeca7f6a9574669a9b` |
| `rtsp_span_server_v1.h` | `2ecee18cbe6d53d5ef950d9aa78045dbaba9b1317e5743b9a928576699742e53` |
| `build01.py` | `9f9c4aaf5548240c0f505f9a0df4cd4ff7bda69cd09542f1f0dba0027d367af7` |
| `README.md` | `ce1d4fab5d0ef5736b32c2595de66190ff4cf4d7160255a34438b7a2815ec7f5` |
| `LICENSE_NOTICES.md` | `9c4980851f266bee5825ace261121c2a8717000a91ab55a7e3a4f4913626c16b` |

Evidence root: `docs/qualification-plans/qc07f-span-serve-20261024/`.
`DELIVERY_PINS.json`: `c816f550963c9f74315c409f756b438a6e48a121013e4ebf95c5fd2b64d90eeb`.
`FROZEN_INPUTS.json`: `6e4ba6931def8403dbed7693e825026638ec62465a5ea3a26598f3ec0546a07a`.
Build01 report: `3b3200efcb5d298e1f490856a6d790893180cbca23aebc7e12cd7e74fb7a4a21`.
Actual baseline runtime C: `4557ccab74d8f4a499da91acc3626ddb5fdd28befc744817dcd3221e61d05a74`;
included native C: `b0b97dc3946e001023bb0c00d22e0aa1d5104ec61b93b4d5770c8d7245a18f5f`;
baseline sender C: `73c0f5886a54ca8fc064b27c72b7c877180730502673e8a6ee106f085a7304d0`.

## Ordinary findings

1. **Actual exclusive composition, not alternate callbacks or a shadow owner.**
   `owned_runtime.c:7–24,353–404,450–475` selects complete accepted native/handoff/
   memory definitions and directly includes the unchanged actual native adapter
   in ONE TU. Prepare installs a persistent real nonreturning typed sink. Entry
   verifies the same config/context, owner, source functions, lifecycle/bridge,
   gate state and live paid v2 hub. `rtsp_span_server.c:810–861` exposes only the
   typed serve route; the former generic source/hub/sink serve is static. Actual
   `native`, `memory_state` and `runtime` remain sole private owners. SOURCE_API3
   requires exclusive replacement and rebuilding every changed-layout consumer;
   no old runtime/server/memory/hub objects or combined RELs may accompany it.

2. **Pre-effect distinction and retained early refusals.**
   BUSY is first and touches neither caller outcome nor active state, under the
   mandatory no-reentry contract. Known invalid runtime identity may return before
   effects; this is trusted C identity checking, not arbitrary-pointer or mutable-
   identity safety. Valid required pointers/READY consume the original one-use
   owner attempt before option/backend refusal; listener ownership changes only
   at the original move. The typed wrapper routes every non-BUSY result through
   actual final cleanup. With an active borrow, even ordinary early options,
   required-pointer or phase refusal cannot unwind to a destructor. Repeated
   terminal serve/setup refuse before reading the retired hub; they do not renew
   an attempt or touch the old listener/outcome. Invalid identity is never a grant
   to destroy a different actual owner.

3. **Real native retirement, not common STOPPED or an acknowledgment Boolean.**
   `owned_runtime.c:313–329` requires actual DRAINING/active STOP, closed frame/no
   loan and the live canonical paid association. It snapshots only numeric output
   begin/end BEFORE native output free, latches stage1 BEFORE `nh_native_retire`,
   and sets stage2/real `retired` only after actual OK. The unchanged common bridge
   dispatches that STOP, takes its post-clock and completes the exact work; timeout
   or failure cannot become normal relinquishment. The unchanged native path is
   guard/lifetime → sensor-off → END → matching retired/END query → receipt ACK →
   each source close-once → encoder observation/close → QP_CLOSED → logical owned
   watchdog disable ACK → native CPU release. `native_retired_actual` (49–65)
   strengthens the original closed-FD/no receipt/no CPU loan/resident/parked/close-
   uncertainty predicates with staged FDs, real owned watchdog FD>=3/QW_DISABLED/
   QW_RETIRE and matching retired END evidence. COMMITTED/never-started still
   refuses; fresh common STOPPED alone cannot pass. No END/disable/close/retry,
   electrical guarantee or new clock policy is fabricated.

4. **Normal boundary is source-reachable, with both retirement timings covered.**
   Following actual successful native NEXT/release and exact common END, clean
   client detach can cause native retirement while the hub is still ACTIVE;
   readiness then rejects `retired` BEFORE output association reads. Alternatively
   final closeclients detaches demand, hub STOP/drain precedes real common STOP,
   and the explicit pre-retire association admits STOPPED. Last detach cancels
   pending keyframe/control work; successful exact STOP clears needs_stop/active
   and reaches STOPPED. Client close clears actual frontend/auth/backend slots.
   Thus the final predicates are not mutually exclusive on the known-normal finite
   path, provided original budgets and every actual acknowledgment succeed. This
   is a source trace, NOT an observed successful session; zero-start and restart
   paths are not admitted normal cases.

5. **Actual tail and last-pointer-use boundary.**
   `rtsp_span_server.c:749–789` retains uncertainty first, closes clients, STOPs/
   drains the SAME hub, checks all canonical tokens, successfully retires common
   work, and checks real client FD/setup/playing/demand/lease/pending/deferred/
   auth/backend fields plus the still-live exact gate. The private end function
   (`owned_runtime.c:477–505`) additionally checks same serving owner/config,
   backend no failed/revoked/pending/peers/source HOLD, full common drain/no queue/
   attached/frame/clock fault, actual native retirement and the STOPPED canonical
   eight ranges/count/interior-page credit in BOTH paid associations. Raw and
   optional output integer extents contribute to the exact aggregate. It performs
   no query, IO, clock callback, END, disable, close or giveback. Only after ALL
   checks does the no-fail/no-callback transition record numeric owner identity,
   NULL runtime span_hub AND ledger.hub.owner AND span_retained.owner, then latch
   terminal. Caller hub/chunks are never disposed here.

6. **Terminal history is not live credit, a fresh ledger or flag-only cleanup.**
   `terminal_span_history` (154–186) uses retained integers and known NULL owner
   slots, not dangling pointer values or integer-to-pointer reconstruction. It
   preserves/validates attempts, payments, count/ranges/credit/failure/native stage
   and raw/optional-output aggregate. Cleanup also requires the actual native
   closed/disabled facts; terminal alone is insufficient. Ready/setup/next/release
   reject or short-circuit before old hub/output reads. Unchanged native provider
   shape rejects QP_CLOSED before memory-gate/next work; no former CPU loan remains
   to validate against freed output. The included strict pre-memory return uses
   the expanded runtime-history predicate plus original memory/native history;
   all new borrow/attempt/stage/terminal/extent/owner history prevents reuse. The
   persistent sink is prepare-time callback configuration, not a cleared payment
   or custody receipt. Original UNKNOWN is still not DISABLED. Gate unbind,
   listener close-once and separate explicit verifier finish do not revisit the
   retired hub/output; config/runtime/native/watchdog lifetimes remain separate.

7. **V3 ordering, real auth and bounded packet semantics preserved.**
   Independently matched all twelve recorded complete bodies, but also reviewed
   changed callers. Existing retain-before-unsubscribe/release/cursor retirement
   and detach0 + valid post-clock ordering remain. New guard_denied/close fences
   retain actual backend failed/pending uncertainty, not all ordinary auth denial;
   they add no grant or retry. Real shared copy → source CPU release → exact END
   precedes consumer packetization, with no subsequent source payload access.
   Per-consumer canonical tokens, all partial-send continuation checks, complete-
   packet sequence/offset commit, absolute deadlines and SPS/PPS<=256 remain.
   The typed sink reaches actual first-error latch/fence/quarantine; fallback
   cleanup/SIGSTOP remains nonreturning. No newhub cast/NULL fakehub is sent to
   the old retain_uncertain callback. A fault after an earlier safe acknowledgment
   retains CURRENT ownership, not reconstructed earlier cursor/loan state.

## Independently checked evidence — not rerun

- Matched22 delivery/101 frozen records; exact seven-file delivery and all frozen
  source directory memberships (original57, runtime9, sender4, memory8, hub4,
  packetizer6). Regenerated all eight literal and two saved aggregate diffs exactly;
  matched the twelve preserved body lengths/hashes against actual v3 source.
- Matched229 inputs,323 new +320 reused dependency records,132 artifacts,16 tool
  records and46 logs:1066 overlapping checks, not distinct-file/coverage claims.
  Exact artifact membership and all16 fresh `.d` files reproduce new dependencies;
  actual reused `.d` membership/production defines/layout exclusions match.
  Sensor feature macros precede COMPLETE accepted handoff `-include`.
- Independently parsed both GNU archives: all46 bodies match original objects,
  all32 selected bodies match delivered extraction;16/23 roles per target exclude
  old layout consumers. Eight actual production units per target are new runtime,
  sender, accepted reserve/runtime/reader, v2 hub, spans and unchanged real sensor.
  No fixture/stub/HOST-public role, old builder or second native TU is selected.
- Actual nm/readelf metadata reproduced saved outputs in LC_ALL=C. An initial
  ambient-locale comparison differed in nm ordering; using the recorded locale
  resolved that read-only comparison, without source/build/candidate execution.
  All50 delivered ELF objects have correct HOST/ARM ET_REL roles. Both rooted RELs
  have30 explicit live roots, exactly one native/memory_state/runtime, actual
  typed entry/end/native-retire/prefault/hub/span/disable relocations and no old
  serve/flat-hub/main export or referenced unresolved project API. Three raw Digest
  UND names remain LOCAL/NOTYPE/size0 with zero retained references, not masked.
  Libc/compiler/syscall and ARM __ehdr_start dependencies remain unresolved REL
  dependencies, not final-link/native ABI proof. Both entries/program-header counts
  are zero. HOST279280B hash `1f919a48d6cf0099e1bb755156a1c38ec57aaa5d4f235ecb4baa56838489168a`;
  ARM186536B hash `90fc57d5fc8d0daceb2d2930f7af3194c1c953dc5144dcd35ab84e8543a67bce`.
- Author16 HOST/ARM production C compiles/2 rooted REL links/46 exit0 commands,
  05:26:47–54Z are cited, NOT repeated. Zero candidate calls. Older build failures,
  rejected sender versions and HOLDs remain frozen, not upgraded by this review.

## Conditions and remaining gates

Trusted true-fresh, valid stable disjoint private storage; serialization, immutable
callbacks/config and no aliases/copies/reentry/signals/concurrency/later fork or
FD mutation remain mandatory. Actual last-fork/provider/watchdog ordering and
independently admitted external ownership/backing/recovery are not proved by C
shape. Exclusive SOURCE_API3 replacement and rebuilding all consumers are required.
Borrowed backing survives every uncertain outcome; successful terminal payments
are historical only, not surviving live-memory credit or restart authority.

Still open: native1MiB output/raw allocation cap, allocator/library/stack/socket/
process2MiB and40MiB/CMA/TLS/combined-load fit; external surviving owner/all-domain
recovery/backing; actual provisioning/main/entropy/syscalls/image ABI; strict C1
and RAM75 recovery. REL/stack-usage records are not final image, full compiler/
transitive-stack closure or native memory acceptance. Original2s/10s/128-frame
limits remain; there is no whole-interval clock/prevention or recovery claim.

Review effects were only public reads/hashes/diffs/log/ELF/archive metadata and
this fresh document. No build/link/import/program/helper/test/candidate/reader/
prefault/credit/callback/native/socket/verifier/child execution, source/Git/index/
AGENTS/central-plan/private/device/network/hardware changes or subworker. Further
adversarial/negative/mutation/fuzz testing remains deferred, NOT passed. Camera
OFF/finalized23h is inherited history, not fresh state or a live lease. **Parent
disposition remains separate; no adoption or execution/hardware queue follows.**
