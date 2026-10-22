# Parent disposition — v1 CORRECTION REQUIRED, IPC custody

2026-10-22; RP-08 / CORE-03–04 / STREAM-04/07. Parent read the full independent
review and actual reap/stop/destroy/observer plus start/finish paths. Rehashed all
1,303 input pins and six review outputs unchanged. Review seal:
`d060aaa15deea1526920414f4015505f1024fa73e2f49eff6a502a90049ce323`.

Accept the **R1 correction-required finding**, not v1 source acceptance. The actual
observer can reap an exited verifier without closing its parent IPC FD. Subsequent
retired-fast-path stop returns OK, then destroy wipes/frees the still-owning client.
A non-OK final service result does not recover that lost descriptor record. This
is a static source-reachable branch, not an observed runtime leak/test result.

The device-free start/same-backend serve/stable peers/explicit finish source piece
and six successful HOST/ARM compilations remain preserved unaccepted evidence.
No compiler/test/native reproduction was run by parent. Parent explicitly selects
fresh `apps/rtsp-auth/prestarted-backend-v2/` and corresponding qualification docs
for **R1 only**: consolidate actual terminal IPC disposition for newly or already
reaped clients; clear ownership before exactly one close attempt, preserve errors
and actual wait/wipe/history, never retry the numeric FD or signal a consumed child.
Uncertain startup must remain terminal HOLD without a second stop/finish attempt.
Any deliberate inherited adapter-body change requires fresh review.

V1 source, build, bookkeeping failure and independent review stay byte-exact.
No listener composition, bootstrap/external media retention, backing, provisioning,
native source/glue adoption, executable/main or hardware operation is selected.
Close/error records are not DMA retirement or proof an exited process retains FDs.
Authentication/source checks, strict image pins, budgets and both hardware HOLDs
remain unchanged. Curated Git may preserve safe review/finding metadata; production
v1 remains unaccepted and generated binaries/private captures stay local.
