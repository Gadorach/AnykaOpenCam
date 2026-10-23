# Cold-entry23 v2 — ordinary R2 tty-role finding review

2026-10-23; BASE-01/05/07 / PLAT-06/08. **R2 CONFIRMED: correction required before
invocation.** The proposed separate tty association is sufficient as a minimal
source correction plan, subject to review of its actual fresh implementation.
No hardware reproduction is needed or selected. V1/v2 and both earlier reviews
remain frozen/uninvoked; the earlier R1 correction remains narrowly sufficient,
but is not whole-v2 execution admission in light of this additional finding.

## Public basis and exact pins

Read PARENT_R2_FINDING and actual v2 `identity`/Retained, `child_identity`/status,
pre-open port metadata capture, post-ON status and finally retirement bodies.
Read retained Linux `tty_io.c:798–822,901–929,952–1033,1062–1103` for the actual
time-update/write path. This same session previously read the full candidates and
original recorder. Public text/hash inspection only; no current tty/private state.

| Path | SHA256 |
|---|---|
| `docs/qualification-plans/qc07f-cold-entry23-v2-20261023/PARENT_R2_FINDING.md` | `935066f962f3e95f407f08bb7430ee0d1aea65b2096f2895a742cee9c9727176` |
| `apps/qualification/qc07f-cold-entry23-v2/cold_entry.py` | `0d46fda61ced57bbbaa768d84f552b3d2d3f1bbc107f4d6a7f6f0cacf59e6bcf` |
| `apps/qualification/qc07f-cold-entry23/cold_entry.py` | `03cc62101a4da20d2c6896e1f990ef13d85b4a12b427c3d1162d62e3e78b3bad` |
| `docs/qualification-plans/qc07f-cold-entry23-review-20261023/REVIEW.md` | `d899403829951a060e721cf927d293212acb81be60b8ef723efa1cdd3ea0ff78` |
| `docs/qualification-plans/qc07f-cold-entry23-v2-review-20261023/REVIEW.md` | `c207c784ccc1c231d5c1a4f8d67a6bcaf70f55b54b0f986626d107c84df43317` |
| `work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux/drivers/tty/tty_io.c` | `c52e640e24d08ce55205516f7612802bf7bd9abfbca11fe7bd5db6a3cbaf32d1` |

## Concrete role mismatch

V2 `identity:55–57` includes size/mtime/ctime, appropriately for retained immutable
regular files. The caller also stores that tuple as `port_meta` before serial open
(367–369). Every `child_identity` compares the live tty's entire tuple against it
(277), including through the post-catch `status()` (436).

In the inspected Linux source, `tty_write` calls `file_tty_write`, which calls
`iterate_tty_write`; when bytes were written, it calls `tty_update_time(tty,true)`
(1026–1028). That helper walks open tty files and directly updates inode mtime
(805–817), without updating ctime in that body. Successful reads instead request
atime updates (925–926). Thus ordinary recorder catch TX can change mtime without
changing the device association that the caller means to protect.

The helper coarsens updates using an eight-second bucket comparison (809–815).
Do not claim every write necessarily changes mtime, or that this retained tree is
the running host kernel. Parent's supplied read-only observation—selected tty mtime
inside finalized cold23c, ctime outside it—independently supports the practical
role mismatch within its limits, not precise write attribution or new runtime proof.
Actual selector/timestamps/provenance were not inspected or exported by this reviewer.

Whenever normal catch IO changes that mtime, unchanged v2 rejects its own legitimate
child at the next identity/status check: a deterministic false HOLD under that
ordinary condition, not evidence of ownership loss. Cleanup still attempts one OFF
and GET OFF, but its later `child_identity()` (467) rejects the same mismatch before
same-peer stop can be sent. Actual owned wait is still attempted; it may time out,
leaving the finite recorder/evidence preserved. This is a substantive observation
and retirement-path defect, not merely an overstrict final cosmetic check.

## Minimum fresh correction and preserved limits

Use a separate explicitly character-device-role association helper, based on a
stat snapshot and retaining exactly `st_dev, st_ino, st_uid, st_gid, st_mode,
st_nlink, st_rdev, st_ctime_ns`. Keep character-device validation and exact selected
port resolution. Use that role consistently at initial `port_meta` capture and
every live port comparison in `child_identity`; do not simply remove the comparison
or refresh the baseline after writes. The typed device association needs no content
size/mtime identity; rdev is essential in addition to filesystem device/inode.

Leave `identity()` and ALL regular/source/private/marker byte/hash/size/mtime/ctime,
path/FD/ancestry guards exactly unchanged. Retain all peer/Popen/argv/start/source,
visible fuser/cooperative locks, strict R1 POST-ack/separate GET state observations,
full raw/trailer/quiet lineage, sticky failure/OFF and known-child stop/wait/final
postchecks. Do not catch/ignore arbitrary identity mismatches, waive ctime changes,
change tty timestamps or loosen existing source pins. No new framework is needed.

This plan fixes an IO-versus-ownership field distinction, not actual native custody,
privileged/global exclusion, trusted namespace/client serialization, physical wiring,
rail state, host timing/recovery or C1/native-clock admission. Original23c EOF error,
v1 R1 and both historical HOLDs remain preserved. Parent may select a fresh R2-only
source copy and focused review; no ON/cold retry/native operation follows this report.

Only this new REVIEW.md was written. No candidate compile/import/call/run/test/help/
default, hardware/tty/proc/dev/private/capture/HOLD-state/socket/RPC/HTTP access or
adversarial/mutation campaign occurred. No old source/review/AGENTS/central plan,
Git/index/model/tool/provider/safeguard change or subworker occurred.
