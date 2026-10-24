# Explicit bootstrap staging-material retirement — contract before code

Assigned20261023 · SEC-03/08/11, CORE-03/04, STREAM-07. SOURCE/compile-only
replacement candidate; independent ordinary review and parent adoption required.
Own only this directory and `apps/rtsp-auth/qc07f-bootstrap-material-retire/`.

## Actual authority and narrow extension

Derive accepted OS-random bootstrap C/header (C `abd2ad799c635f011c726dee493b417bfebd672ff2fb6a0bba0528ca55894c94`,
header `f965a8075d6079f7d1d02a9e966ba17c2802b3e8e7221ae423e4fbbb1310c076`).
Keep old exports and one process attempt, adding
`qc07f_bootstrap_retire_material_v1`. Select ONLY this unit and its new header,
never either old bootstrap unit/object/combined REL. Struct/enums grow: source
layout extension, NOT binary/native ABI compatibility. No mutable getter, main,
native activation or serve export. No earlier native/media ownership ANYWHERE is
a mandatory trusted closed-API prerequisite, not proved by fields.

Preserve actual pre-snapshot OS callback selection, provisioner begin/observe,
known-child wait/bind -> device-free prepare -> same-owner start, and one explicit
verifier-only finish including START_FAILED/HOLD handling. Add metadata capture
immediately AFTER successful real bind and BEFORE prepare/start. A capture failure
is sticky HOLD retaining original cfg FD/file, borrowed directory and stable owners;
never start or clean up. Never reconstruct a child handoff or read/hash payload.

Capture actual cfg.material_fd number (identity observation, NOT another owned
slot), borrowed explicit directory FD and fstat snapshots. Require real bound
provisioner/config/unused-owner/directory association, actual observed wait0, no
unreaped/unknown wait/unknown close, received slots moved, and exact fixed name
`RTSP_PRIVATE_HANDOFF_FILE` from reviewed header. Directory: root-owned0700 live
directory/CLOEXEC, same dev/inode as actual provisioner. File: root:root0600 regular,
nlink1, size1536, readonly/CLOEXEC, same actual received envelope dev/inode and
same no-follow named inode. No arbitrary paths, reopen, repairs or payload inspection.

## One explicit request, only after real normal finish

Only actual finish return0 + PS_FINISHED sets FINISHED_DEVICE_FREE. Retirement
additionally checks same stable owner/config/runtime/provisioner identity, recorded
actual start attempt/success and finish attempt/success, no sticky errors, normal
owner start/finish/serve results, no source hold, no remaining backend child/peers/
pending state and the actual provisioner known-retirement/slot facts. These are
corroboration of this API's actual calls, not standalone permission booleans.
READY/PROVISIONING/START_FAILED/HOLD/unknown/early-prepare failure NEVER close/unlink.
One identity-valid explicit request is consumed even on an ineligible phase or
metadata failure; no retry/idempotent success. Reentry/foreign identity refuses
without touching another owner. Repeated requests refuse without altering history.

Preflight rechecks cfg slot number, readonly/CLOEXEC, directory and fixed-name/file
association, comparing dev/inode/type/mode/UID/GID/nlink/size/mtime/ctime to the
post-bind snapshots. Do NOT compare atime: real consumer reads can change it.
Do NOT check/reset offset: rv_start F_DUPFD_CLOEXEC -> fork/dup2 -> child read1536
shares the original open-file description; parent prefix pread does not reset it.
No writes/chmod/chown/link/rename or directory operations are permitted between
capture and retirement, so mtime/ctime (including nanoseconds) and directory size/
nlink are expected stable BEFORE unlink. This is a trusted quiet-namespace policy,
not a universal filesystem guarantee; mismatch refuses without repair. Unlink can
legitimately change directory mtime/ctime/size and file ctime/nlink afterward; do not
apply pre-unlink immutable comparisons after it.

Order: consume request -> gate -> metadata preflight -> clear canonical
cfg.material_fd=-1 BEFORE ONE close -> record actual result/errno -> if close0,
recheck fixed named inode AND quiet directory association -> ONE unlinkat(dirfd,
fixedname,0) -> record result/errno -> only if unlink0, ONE fsync(borrowed dirfd).
No numeric close retry, reopen, unlink/fsync retry, restoration or automatic exit.
Close error/EINTR: sticky HOLD/close uncertainty, linked file retained. Post-close
metadata failure: HOLD/known closed, file untouched. Unlink error: HOLD/namespace
outcome uncertain, no fsync. Fsync error after unlink0: HOLD/known unlink acknowledged,
directory acknowledgement uncertain, never recreate. Separate attempted/result/
errno records distinguish unattempted from successful zero. Terminal
MATERIAL_RETIRED requires close0+unlink0+fsync0; it means ONLY original cfg FD closed
and this staging name logically unlinked with directory syscall acknowledgement.
It is NOT secure erasure, power-loss/flash durability, rotation, whole-process/native
release or exit permission. Borrowed directory, image FD, root context and native
memory are not retired. All states retain stable storage, no stack-unwind cleanup.

## Essential caller conditions, not metadata-proved

Same stable private object/config/graph/routes/context and once initializer; no
copy/reset/destructor, aliases or handle borrowing. Borrowed directory remains live
and exclusively owned by caller through ALL states, including HOLD and terminal
material retirement. Exclusive namespace/ancestors/fixed name and no writers,
FD-number reuse/aliases, extra fork, signal effects, competing reapers, threads or
reentry for the whole lifetime are mandatory. Flags do not prove writer exclusion;
inode/name checks do not resist hostile root or prove TOCTOU freedom. This is only
the exclusive generation actually created by this bootstrap's trusted child, not
private config/credentials/history/acquisitions or other entries. Never rmdir.
Underlying ingress/secret-free launch/global FD-env-cap hygiene/real retain_verifier/
hardening/native image prerequisites remain. No cleanup on early failure/unknown,
atexit, kill, stop or watchdog effects are added. No C1/clock/recovery waiver.

## Selected evidence only

HOST/retained musl ARMv5 changed-unit compile + real integration REL with audited
runtime build02 archives, provisioner production objects0,1,2,3,9 and OS-random
production object. No old bootstrap/combined REL, duplicate zeroizer, fixture/stub
or generated program. Fixed fresh refuse-overwrite recipe; retain actual argv,
tools/inputs/dependencies/logs/artifact hashes and ELF/symbol inspection. Public
source/artifact metadata only. No function/program/test/helper/fixture/provisioner/
verifier/OS-random/material-retirement/native/credential invocation, even HOST.
No private/proc/dev/state/HOLD/capture/network/UART/RPC/HTTP/hardware/Git/index or
central-plan edits, subworkers, adversarial/fault/mutation/fuzz campaign.
23h finalized OFF/old failures and RAM75 recovery/C1 block remain inherited facts,
not new observations. Durable backend/rotation/real launch/backing/main/native app,
entropy/ABI/128KiB/combined-load and filesystem semantics qualification stay open.
