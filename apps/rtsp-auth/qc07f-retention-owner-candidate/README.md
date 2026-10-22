# RP-08 retention prerequisite — BLOCKED before production code

2026-10-22. This directory contains **no implementation or builder**. Source-only
investigation found that the actual native SIGSTOP quarantine retains its own
FDs/backing only while that process lives; existing supervisors own verifier,
HOST-fixture or web processes and cannot safely substitute as the media owner.

The frozen RTSP entry internally starts/forks the verifier. Calling it after native
provider setup would duplicate native descriptions before exec. Error-returning
NEXT also cannot satisfy its unchanged partial-retirement contract just by copying
metadata or retaining a pidfd. No unsafe partial wrapper, ready flag or fake main
is provided.

Contract and exact blocker/source pins:
`docs/qualification-plans/qc07f-retention-owner-20261022/RETENTION_CONTRACT.md`
and `CANDIDATE_REPORT.md` in that directory. The latest provider-return source is
left at aggregate
`9ac7f363d7395fe944f2d068a4c964a5b20c77e9410e84633c07ce7f8b8a169f`;
no frozen source is copied/rebased/changed here. Existing licenses stay intact;
this documentation makes no new license grant.

No compile, candidate/helper/script/test/runtime/listener/hardware execution,
memory grant, full-app/native acceptance or automatic next operation. Parent
review of one bounded technical-block report is the handback.
