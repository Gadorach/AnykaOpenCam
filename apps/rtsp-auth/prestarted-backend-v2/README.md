# Prestarted backend v2 — R1 IPC disposition correction only

Fresh derivative of frozen v1 source aggregate
`867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90`.
V1/review/build evidence remain unchanged and unaccepted. Contract before code:
`docs/qualification-plans/rtsp-prestarted-backend-v2-20261022/R1_CONTRACT.md`.

Already-retired and newly-reaped rv_stop paths now reach the same actual IPC
clear-before-one-close helper. Pre-release startup-failure IPC disposition uses
it too. The first close error is sticky: no later fd=-1 fast path may turn it
into OK, destroy/wipe/free or retry the former number. A close error retains the
actual client/history with fd=-1 and former number explicitly NOT-owned. Existing
backend/sink/outer HOLD behavior preserves that record, even with known child exit.
`is_rv_ipc_close_outcome` exposes only scalar attempt/former-number/error and
returns non-OK for the recorded error, independently of rv_retired's exit evidence.

Original reap/wait, stop signal/poll budgets, rv_destroy body, start/serve/finish,
stable peers/config, process/self one-shot claims and unknown-start HOLD/no retry
remain. rv_start gets only the pre-release IPC-close callsite substitution; no
child payload/barrier/pidfd/auth/pin/policy change. No broad constructor/pidfd/
ancillary cleanup redesign. No source composition with the owned-listener sibling.

Device-free real start once -> actual serve on same backend/child without init/
fork/hidden finish -> explicit one finish remains a source prerequisite API, not
installed bootstrap/external media retention/recovery/backing or a product main.
Native failed NEXT/uncertain regular shutdown, provisioning/native gates remain
BLOCKED. Preserve strict native2MiB/HOST4MiB/loopback/checks/budgets and memory limits.

Compile-only (real adapter/header consumers freshly compiled, nothing executed):

```sh
python3 -B apps/rtsp-auth/prestarted-backend-v2/build.py build01 --toolchain /path/to/retained/armv5-eabi--musl--stable-2025.08-1
```

HOST/ARM component archives/ELF REL only, not a final executable/full app link.
No tests/runtime/listener/network/private/target/Git operations or automatic queue.
Existing GPL SPDX/runtime notices and provenance preserved; no global relicensing.
