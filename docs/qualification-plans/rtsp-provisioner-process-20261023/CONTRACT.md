# Separate RTSP provisioner process — design before implementation

2026-10-23 · SEC-01/03/05/11, STREAM-07 · new isolated source candidate.
Original constructor, handoff, owned-runtime and all historical evidence stay frozen.

## Actual missing bridge and feasible API relationship

The reviewed handoff owns a readonly file FD but its final move is process-local.
It cannot be fabricated/reconstituted in a receiver. Its actual API does permit
the ONE new child to construct material and move into the **fork-private copy of
the same original config address**, with the actual fresh prestarted owner.
Child sends that FD over a new private Unix seqpacket socketpair (SCM_RIGHTS
duplicates a kernel reference, NOT an atomic move), clears its config slot and
closes its sender reference once. Child wipes its bounded ingress/material buffers
and exits0 only after all required closes succeed. Parent receives into its own
stable typed owner, never a forged handoff object. Parent binds its own original
unused config slot only after exact public envelope/inode checks AND owned wait0.
No backend/verifier/listener/provider/native/device callbacks are invoked.

## Mandatory external launch preconditions, not checked authorization flags

Caller is trusted, serialized, single-threaded and nonreentrant, with default
SIGCHLD/no external reaper and no signal-handler FD mutation, extra fork or FD
aliases. BEFORE the provisioner fork, parent contains NO input credentials,
password/HA1/material mapping or native/provider/media/guardian/backing/listener
ownership; no backend start or runtime prepare has occurred anywhere. Config,
graph, routes, callback addresses/context address and actual prestarted owner are
stable/immutable for this entire attempt. Only device-free public context exists.
Child never invokes config callbacks. Native process is the existing root/no-
groups class. Caller establishes inherited-FD, environment and capability hygiene;
the module does not pretend metadata proves these conditions or authorization.

Ingress is a preopened trusted exclusive O_RDONLY pipe, CLOEXEC and nonblocking,
from independent authorized RTSP-domain provisioning. No path/PID/network selector,
default credential, UI/HTTPS reuse or RNG password. Trusted external writer and
authorization/launch are NOT implemented. Parent moves the canonical ingress slot
into the stable owner before fork, reads zero bytes, and closes its alias exactly
once after successful fork. Fork/preflight failure leaves it represented in owner;
never numeric close retry. No alternate aliases, competing readers or writers.

Caller holds a preopened trusted exclusive directory FD, CLOEXEC, mode0700,
native root-owned (HOST fixture current nonroot UID). Its ancestors/namespace,
fixed name and linked inode remain exclusive, stable, and writer-free through
future verifier consumption/retirement, not merely this transfer. No chmod/chown,
unlink, directory discovery, repair, overwrite or automatic cleanup. Directory FD
is borrowed and MUST outlive the owner/HOLD/config consumer. Metadata alone
cannot prove no writer alias. Durable generations/storage/rotation are absent.

## Bounded one-attempt protocol and owner

New stable process owner is initialized once, cannot be copied/reset/destroyed,
and consumes one process-local attempt. It retains the borrowed directory,
canonical ingress/socket/received-FD slots, possible partial fixed-name file,
own unreaped child identity and sticky public error. All acquired received FDs
are stored BEFORE validation; ancillary storage is bounded and all delivered
rights represented even on protocol failure. Close uncertainty clears the slot
before one close and is sticky, never converted into a number to retry.

Child calls the actual native hardening helper BEFORE ingress; existing create
repeats hardening. HOST PUBLICDUMMY is a different compile class and exported
symbols and cannot call native hardening or bind native config. Input is one
versioned little-endian header, bounded username/realm/password payload and exact
EOF, finite syscall/poll count and nominal CLOCK_MONOTONIC budget. EINTR consumes
budget. Fixed scratch is far below128KiB; no heap/full-AU allocation. No credential
readback/hash/log/digest/paths. Child always wipes owned secret scratch then _exit;
it never returns into parent code. Failure can preserve partial linked file.

New socketpair is AF_UNIX SOCK_SEQPACKET|CLOEXEC|NONBLOCK created before fork,
not an externally selectable channel. One exact fixed public metadata packet
(bindings/generations/profile/device/limits/routes + inode identity) and exactly
one rights FD. Receive uses MSG_CMSG_CLOEXEC and rejects truncation/extra control.
Send success is only duplicate-reference delivery, not wipe/exit evidence.
Successful child sender close and wipe precede exit0. Parent waitpid only targets
its actual unreaped fork child, WNOHANG, bounded per observation; unknown/nonzero
exit, receive/send/close uncertainty or nominal timeout permanently forbids bind.
An observer may later collect the still-owned child on HOLD but never clear HOLD.
No automatic kill, reset, destruction, close, unlink, wait retry on ECHILD, or
unrepresented background process. Nominal polling is NOT a runtime hang guarantee
or a clock/recovery/watchdog framework.

Parent checks received regular inode mode0600, native root/current fixture owner,
nlink1, size1536, O_RDONLY, CLOEXEC, offset0, directory/name association and exact
child public envelope against immutable original logical bindings. It reads NO
material contents, including no secret prefix; original backend subsequently
checks its own public prefix and actual verifier checks its material. Trusted
constructor/child and unchanged fork snapshot bind contents; FD metadata alone
does not. Explicit parent bind rechecks freshness/metadata and moves only after
known wait0 and known parent closes; no auto prepare/start. Config becomes FD
owner on bind even on subsequent startup failure. File remains linked (backend
requires nlink1). SOURCE relationship is not full app launch/main/SEC acceptance.

## Evidence allowed

Compile real units + frozen constructor/handoff/Digest/Mbed TLS/interfaces HOST
native-shaped and retained musl ARMv5, REL only; separate PUBLICDUMMY harness.
Audit recipe/harness first. At most ONE normal fixed PUBLICDUMMY child case under
fresh /tmp/opencode0700 namespace, scalar-only observations, explicit test-only
cleanup after known complete success. Dummy bytes prepared in parent are NOT
product secret-free-fork proof. Native hardening/real credentials/verifier/media/
hardware never run. No negative/adversarial/mutation/fuzz cases. Parent independent
review required before adoption. Only this app/doc lane may be written; no Git.
