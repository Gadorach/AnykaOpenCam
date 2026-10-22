# Prestarted backend + actual owned listener — contract before code

2026-10-22. RP-08/09, CORE-03/04, STREAM-04/07. Bounded ordinary SOURCE composition
candidate only, not installed bootstrap/media retention/native/full-app acceptance.

## Exact provenance / selected composition

- Base: actual prestarted-backend-v2 11-file source
  `5a42cdc9d76628031c5380edd2e3b2c4dd06aefc6675138c8e10e1b098a28868`, source-seal
  `2bfa06a6b239c0c62b1003df6ee0f0ef9dd17ef0088efad00cccaf1ada7e7a8a`.
  Focused R1 review seal:
  `b3310e3a85cc7295137fa46e3aca4aae8fb12653b7377a55ad9d433a42532292`.
- Import actual listener helper bodies from reviewed owned-listener source
  `febab8640027f71dfc8a1ec4aa53c26cba219048ca4bf74578050377101ce557`; independent
  review seal `2f39d960ba6cff2b236d215d65dea206f0005fad0fc430a621e24de25bd3db61`.
  Its full report/contract/parent disposition/review, actual helper and full server/
  diff were read. Both siblings and all prior sources/reviews/failures stay frozen.

Use ONE copy of actual v2 backend/auth/verifier/server, not two sibling archives.
R1 adapter C/diagnostic header and backend C remain byte-identical. Replace only
the old internal-bind prestarted serve entry/header surface with a versioned
mandatory owned-listener serve entry. Listener custody C stays byte-identical;
its internal header includes the new source API header rather than an old frozen
candidate_auth.h through the sibling API. Preserve the v1 outcome layout/name
used by that unchanged helper, with a new explicit composition API/macro.
No old internal-bind or monolithic sibling serve export/fallback is delivered.

## Real caller/storage/bootstrap order

Original macro-initialized stable candidate_prestarted embeds the same actual
backend/child/config/auth peers. Real one-shot device-free start occurs first,
with genuine unchanged RNG/material/routing/graph/source callback addresses.
All fork/exec must finish BEFORE any native/provider/watchdog/guardian description,
media backing/lease/session/provider userdata exists. Additionally, this composite
requires its selected listener be created AFTER the last fork: no inherited-listener
exception is used. Bootstrap create/bind/listen authority is separate and absent.
No source code, Boolean or fstat can prove these trusted caller prerequisites.

Config identity/object, graph/routes, callback addresses and context ADDRESS stay
stable/immutable. After the last fork, independently admitted userdata preparation
may occur inside that same context only; no fake readiness/RNG/provisioning, new
native preparation or source callback is supplied. Caller retains image/material
roles without listener aliases. FD table/calls/threads/signals/callbacks are fully
serialized, nonreentrant, stable and disjoint; no dup/SCM/fork/concurrent close/
reuse/flag changes or callback mutation/use of listener slot/outcome is permitted.
This is trusted public C source lifetime, not thread/attacker-memory protection.

## Entry refusal versus a consumed serve attempt

The versioned serve API takes the real owner, options/producer, actual writable
listener int slot and required disjoint writable outcome. A process-local active
entry guard is checked FIRST: nested calls return BUSY before touching any output,
FD slot or owner, including NULL/different/active arguments. It remains set through
every callback, unbind, owned close and final result publication.

Outside nested entry, NULL owner/slot/outcome or invalid self/config identity and
non-READY phase refuse without touching outcome/slot/owner: no attempt occurred,
and stale outcome contents are NOT a result for that refusal. Once a valid READY
owner and mandatory storage pointers are admitted, mark SERVING and initialize
the outcome. That consumes the one serve attempt even if backend/options/listener/
clock/source preflight later fails. No second serve or hidden backend destruction;
all such errors leave the actual backend/config/client retained on HOLD.

## Borrow, actual backend checks and commit

Keep exact loopback127.0.0.1/options.port>=1024, original auth/source/URI/lifecycle/
hub/options/rate/budget checks. Require real same backend/config association,
initialized/nonfailed/nonrevoked/unoccupied peer/pending state, actual READY15/
profile/current owned-child observation, then actual backend poll. No reinit,
rv_start/fork/exec, copied backend, permission flag or hidden finish in serve.

Initial listener observation is queries only: actual fstat/socket identity,
F_GETFL nonblocking/F_GETFD CLOEXEC, exact SO_TYPE/ACCEPTCONN/PROTOCOL lengths and
values, getsockname AF_INET/exact loopback/port. Original option/URI/lifecycle/hub
metadata checks precede effects. Run actual backend poll/callback and recheck its
real association/READY/profile/current child/failure/revoke/pending/peer state
before final listener move. A clock refusal precedes move too.

Move uses the unchanged helper: requery ALL actual listener metadata, compare
caller numeric slot with initial observation and fstat dev/inode, then private
owned slot takes the FD, actual caller slot=-1 and outcome.consumed=1. No callback/
effect intervenes between final observation/comparison and those stores. Numeric
identity/metadata are refusal predicates, not no-alias or endpoint authority.

All precommit errors leave the actual caller slot unchanged, outcome.consumed=0.
Backend ownership is independent: it remains in the actual stable owner, including
when readiness observation reaps a child. Only after commit bind the source gate.
Existing poll/accept use ONLY the moved private listener, without socket/bind/
listen/F_SETFL/setsockopt on it or an internal fallback. Accepted-peer socket
options are distinct unchanged paths, not selected-listener reconfiguration.

## Cleanup, outcome and actual remaining custody

All postcommit returns go through unchanged client/hub/source retirement order,
then synchronous guard unbind while gate/clients/hub stack is still live, then
unchanged helper's clear-private-slot-before-one-close. No callback may retain
that gate after unbind. Every postcommit returned outcome retains consumed=1 and
close_attempted/error, including failure; never close/retry the caller's former
number or claim it is still owned on close error. Final result is non-OK and owner
HOLD on close failure even if the source loop otherwise returned OK. Error outcome
is retained, never reset by another one-shot call. No listener close retry API.

Outer final result includes unresolved backend/pending/revoked/peer/source/clock
facts. Publish actual owner.serve_result/phase and outcome.result BEFORE releasing
entry guard. Only a clean serve with successful listener disposition and no open
records is SERVED. Every other attempted result is HOLD, not retryable READY.

Explicit original verifier finish remains caller-invoked once, only READY,
START_FAILED or clean SERVED; never HOLD/SERVING/FINISHED. R1 sticky IPC history/
helper/observer, actual stop/wait/destroy/start uncertainty handling are unchanged;
no second stop on RV_UNCERTAIN startup, no forgotten close error or fake wait/wipe.
It is verifier-only cleanup, never permission to close/free/exit native owners.

Stable backend/peer pointers do NOT retain returned gate/client/hub stack or make
native failed-NEXT partial work internally retired. Inherited source cleanup and
the unchanged public RTSP non-OK NEXT contract remain incompatible with uncertain
native work. Genuine nonreturning media retention/process/bootstrap/recovery and
admitted backing/provisioning remain BLOCKED; no main/provider/query/native glue,
allocator, media RPC/supervisor, END/ACK/reset/feed/kill/resume or memory credit.

## Compile-only evidence and stop boundary

Fresh HOST/ARM actual combined units and normal real dependencies, one definition
each, archives/ELF REL only. All changed-header consumers rebuilt, no old layouts/
object reuse, no old guarded builder execution/import. Retain exact before/after
source/imports/licenses/pins, actual commands/tools/headers/dependency closure,
deltas, logs/artifacts and known inherited failure history. Preserve strict native
2MiB/HOST4MiB/pins,2s/one hub,128KiB/40MiB/CMA/native10s/<2s/5s constraints.
No test/runtime/listener/app/executable/payload generator/target/private/network
IO. Query-client lane is untouched/unqueried. Hand back ONE source/compile
composition candidate for independent review; no whole backend/product/native
GO, automatic reviewer/build/test/implementation/native queue.
