# QC07F writer runtime v2 — R1 correction, NOT RUN

Exclusive SOURCE_API4 replacement for qc07f-writer-runtime-candidate, never link
both. Only checked_source() changes: after the existing final blocking auth gate,
observe the actual applicable active/idle watchdog, then read the actual source
clock and refuse against SAME original operation/session/active-phase budgets.
Only bounded private metadata readiness follows that last sample; no new callback,
clock policy, rearm, feed, timer, loop or budget renewal. Observed failure retains
CURRENT canonical armed writer before first-error/native fence/quarantine.

Header/native_adapter.inc/native-watchdog.inc are byte-identical. Actual raw
capture/owned encoder spans ioctl/full CRC/typed scalar loan/release/exact common
END -> hub commit, one private native/memory/runtime, paid/history/source/return
guards and normal STOP remain unchanged. No serve/main/borrow-end; original CPU
hub borrow remains retained. Old contiguous exports still refuse before effects.

Read `docs/qualification-plans/qc07f-writer-runtime-v2-20261024/CONTRACT.md` and
HANDOFF.md. Stable final-address private disjoint objects/config/callbacks/FDs,
trusted serialization/no aliases/edits/reentry/signals/threads/later fork/FD mutation
remain mandatory. This source does not supply launch/backing/recovery/provisioning.

ONE fixed offline compile01.py attempt only, HOST/retained musl ARM production
objects/rooted REL metadata, no program/test/candidate/function execution. Stop on
any failure without correction/retry. V1 remains frozen NOT ADOPTED, with build01
failure and build02 compile facts preserved. Independent ordinary review and parent
disposition required BEFORE use; no self-adoption, next queue or hardware action.
Overall cap/raw/process2MiB/kernel2MiB DMA/CMA/TLS/combined load, native ABI/entropy/
external recovery/backing/provisioning/main and strict C1 remain OPEN.
