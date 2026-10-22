# Owned loopback listener v1 — design before implementation

2026-10-22. RP-09 / STREAM-04 / STREAM-07; SOURCE/compile only.

## Authority and caller

This is a destination-config **source API**, not a binary plugin or permission
receipt. Only a reviewed trusted bootstrap in the same process/source closure may
call it. No such bootstrap is delivered or currently admitted. Bootstrap alone
creates/binds/listens under its separately admitted policy; passing metadata or an
FD does not grant network authority. Only the frozen server's exact validated
127.0.0.1 / options.port (port >=1024) is supported. No public-bind mode/fallback.

Caller must serialize the entire process FD table (including callbacks, signal
handlers and other threads) through return. No dup/SCM_RIGHTS/other-process aliases,
forks, concurrent close/reuse/flag changes, reentry or callbacks performing those
operations are allowed. The one exception is the inspected original rv_start fork:
see below. Neither fd numbers nor fstat identity prove absence of aliases. Caller
must establish this prerequisite by exact source/owner review, not metadata.

The actual writable caller int slot, outcome, options, producer, lifecycle,
config/graph/URI/parameter data and backing are live, disjoint, stable storage for
the full synchronous call and notifications. They must not alias each other, the
hub, callbacks' mutable state, verifier image/material descriptors or each other’s
ownership roles. Callbacks may not mutate listener slot/outcome/options/config,
close/borrow/retain/use the listener, reenter serve or keep the server guard context
after unbind. Original source/provider/retention prerequisites remain mandatory.
No peer/verifier/provider FD custody is changed by this API.

## Borrow, initialization, commit

1. Validate original options/lifecycle/hub/URI constraints and query the borrowed
   caller descriptor: fstat socket identity; F_GETFL O_NONBLOCK; F_GETFD FD_CLOEXEC;
   SOL_SOCKET SO_TYPE == SOCK_STREAM; SO_ACCEPTCONN ==1; getsockname exact AF_INET,
   127.0.0.1 and expected network-order port. Queries fail closed, including missing
   old kernel/libc support. No socket/open/dup/bind/listen/setsockopt/fcntl SET on
   this descriptor, and no internal listener fallback.
2. Initialize the unchanged real backend, including actual rv_start READY flags15,
   profile and engine ACK; preserve native2MiB versus separate HOST4MiB, strict
   image pins/readiness/entropy/credential requirements. No fake callbacks.
3. Requery the caller's still-owned descriptor and compare identity and exact
   endpoint/flags immediately before transfer, after all startup callbacks. With
   serialized FD table there is no effect/callback between this query and commit.
4. Copy its actual fd into the private owned slot, set **actual caller int = -1**,
   set outcome.consumed=1. This is the sole commit point. Bind the synchronous
   source gate only after commit; poll/accept use only that moved descriptor.

All failures before commit leave the listener owned by the caller, including bad
socket/listen-state/options/hub, clock failure, backend initialization failure or
final revalidation failure. No listen operation is attempted. Backend cleanup can
itself return uncertainty and retain a verifier; that does not consume the parent
listener. Caller must obey the unchanged uncertain-verifier retention contract.

## Fork inheritance is not concealed

Frozen verifier_adapter_overlay.c:105–121 and164–225 was read. rv_start duplicates
image/material/socket >=16, forks once, uses a pidfd/parent-release barrier and
then launch overwrites3/4/5, closes >=6 via mandatory close_range, closes0/1/2 and
execveat. The preopened listener can transiently exist in this child **before**
readiness, while still caller-owned; CLOEXEC is not a fork barrier. This source
does not claim sole-description custody during that interval, nor forbid that
mechanical inheritance by a metadata flag. Inspected child launch has no listener
poll/accept/use path and no callback into server/caller code. Actual successful
READY + engine ACK occur after launch's overwrite/close/exec, so only then may
parent commit. Listener cannot equal the occupied image/material descriptions.
If child startup/retirement is UNKNOWN, do not claim the transient child reference
has retired; preserve actual verifier/process ownership via the unchanged sink.
No unsupported close_range/pidfd/exec fallback or verifier-core change is supplied.

## Last use and returns

Every postcommit return reports consumed=1, even on fatal backend/clock/close
error. Last listener poll/accept precedes cleanup. After source/client lifecycle
cleanup, unbind the source gate while its stack context is live; clear the private
owned fd slot **before** one close attempt, then finish verifier cleanup. No retry,
second close or claim that a close error preserves the original description. A
close error has explicit close_attempted/close_errno and makes final status fatal;
numeric FD may already have been released/reused. Outcome is custody/disposition,
not proof of source/DMA/FD retirement. No new source backing retention/cleanup.

Callbacks remain synchronous/nonreentrant and bounded under original APIs; the
entry rejects nested calls while its static clock/gate context is active. The
sticky clock fault is never reset. This is not multithread synchronization, a
restart policy, protection against malicious same-process code or an external
failed-next owner. All native/bootstrap/provisioning/backing/app gates remain open.
