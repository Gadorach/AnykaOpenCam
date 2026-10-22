# R1 contract before code — terminal verifier IPC custody only

2026-10-22. RP-08 / CORE-03–04 / STREAM-04/07. Fresh correction candidate only;
v1 is frozen/unaccepted. No runtime failure/test or independent acceptance claimed.

## Actual source defect and selected delta

Read full independent review (FINAL_SEAL SHA256
`d060aaa15deea1526920414f4015505f1024fa73e2f49eff6a502a90049ce323`), actual
v1 reap/observer, startup partial failure, stop/destroy and backend start/finish/
destructor. Ordinary sequence: successful start -> observer reap consumes actual
exit -> START_FAILED -> permitted explicit finish -> retired stop fast path ->
destroy. V1's fast path never closed parent IPC, yet freed its sole client record.
Known CPU exit and non-OK final result do not dispose that still-owned descriptor.

Change only the fresh adapter's terminal IPC handling and additive scalar outcome
declaration. Keep reap/observer/wait and all shutdown signal/poll decisions intact.
No broad pidfd/constructor/ancillary-FD cleanup refactor. Client layout gains only
explicit IPC close-attempt, former-number-NOT-owned and sticky positive errno.

## Actual FD / exactly-once rule

One helper disposes **the actual c->fd**. Under existing sole serialized/nonreentrant
caller/FD-table contract: save the numeric diagnostic, set actual c->fd=-1 BEFORE
the one close attempt, set attempt history, then close exactly once. Even on error,
the former number is NOT owned and cannot be closed/retried/reasserted, duplicated
or used as a description. Save the first errno (EIO if unavailable), mark failure
and return RV_UNCERTAIN. The helper never opens, dups, signals or waits.

Once attempted, later helper/stop/destroy calls return the same sticky non-OK
close outcome without another close, even though c->fd==-1. Success with fd=-1
cannot overwrite an earlier failure. Missing/no-owned IPC without a prior close
error needs no close. No FD0 exclusion or numeric-PID fallback is introduced.

## Terminal routes / startup boundary

- Already-retired rv_stop reaches the helper immediately, without clock/signal/
  poll/wait against a consumed child. This covers observer/earlier adapter reaps.
- A new successful reap in the original rv_stop loop reaches the same helper.
  Normal stop signal/wait budgets and actual wait/wipe receipts remain unchanged.
- The existing pre-release-barrier startup-failure IPC close also uses the helper,
  so an early close error cannot disappear before the actual child exit is proved.
  The original stop loop still handles that real child; no extra stop is added.
- rv_destroy remains unable to wipe/free a client unless rv_stop returns OK.
  Its second already-retired call has no new signal/wait/close after successful
  disposition, and must return sticky failure after a failed close. On such error
  the real allocated client/history remains retained, fd=-1, not falsely owned.
- Original backend destructor keeps its real client/config and mandatory
  retain_verifier sink on non-OK. Original outer explicit finish becomes HOLD if
  client remains; no normal FINISHED/free or retry. If actual child was reaped,
  rv_retired stays true and its exit/wipe evidence remains; RV_UNCERTAIN here can
  describe failed IPC disposition, not an invented unknown child wait.
- Original rv_start RV_UNCERTAIN from an already-attempted stop still enters the
  same terminal startup HOLD/sink. Never route it through new finish/stop/retry.
  A pre-release close failure may now produce that same non-OK status even with
  known child exit, correctly retaining the sticky IPC error and actual receipt.

An additive read-only scalar observer may report attempt, former-number-NOT-owned
and saved errno, and returns non-OK on recorded close failure. It does not clear
history, close, query private payload, grant readiness or prove future liveness.
Its output is stable/disjoint trusted caller storage. Public rv_retired remains
only actual child-exit evidence, independent of the IPC outcome observer.

## Preserved ownership and limits

No backend/source-config/peer-copy/reinitialization changes. Real device-free
rv_start once, self/process one-shot claim, stable config/actual auth peers,
serve without init/fork/hidden finish, explicit one finish and no HOLD retry remain.
Caller retains actual client/backend/config/process namespace on uncertainty;
metadata/pidfd cannot preserve a process that exits or prove native DMA retired.
Native failed-NEXT/backing/retention/bootstrap/recovery, listener composition and
regular native uncertain shutdown remain BLOCKED. No main/media RPC/supervisor,
native END/ACK/reset/feed/kill/resume, allocation/backing or credentials work.

Auth/pins/RNG/child payload/barrier/native2MiB/HOST4MiB/loopback and all original
checks/budgets,128KiB/40MiB/CMA/10s/<2s/5s remain unchanged. Public framework and
all v1/review/retention/listener originals stay byte-exact. Preserve SPDX, licenses,
provenance, actual source delta/tool/header closure and all failed evidence.

Fresh HOST/ARM real component compile/archive/REL only (all three actual header
consumers may be rebuilt; no unproved reuse). No suites, runtime/executables,
listener/network/private/target IO, old guarded builder, kernel/module/image.
Functional verification is deferred, not passed. Hand back this one candidate
for independent review; no automatic next queue or source/native/full-app GO.
