# Fixed seven-second guard-only qualification pack

Isolated offline pack; current recipe/native entry/callbacks/kernel/client/launcher
and active parent target jobs remain untouched. This is **not** a new guardian,
auth service, listener, bridge, device/radio operation or native qualification.

`helper.c` accepts no arguments or environment configuration. Fixed monotonic7s
lifetime, bounded READY/lifetime/exit/cancel/error scalars only. FD3 must already
be closed by the reviewed guard child-exec; helper never opens watchdog/files/
devices/sockets or reads stdin/private input. Own core0/dump0/caps-cleared/
no-new-privs calls are actual, but HOST nonroot execution does not establish native
root isolation. Clock failure/backward time fail nonzero; SIGTERM/INT are observed
in the loop and exit20 conservatively, not normal wait0. No automatic restart.

The build pins the original native04 public source and reviewed guard builder/hash.
It produces one fixed standalone public command/script with only helper path
`/run/guardqual-native01.elf`, fresh GUARDQUAL01 markers and **empty helper argv**
changed. Existing FD3-owner open/fork/child-close/source birth+PPID/exact-exe proof,
45 iterations/sleep4-before-check/5s watchdog/native180/2MiB/35HTTP40CPU bounds and
wait0+exec_seen+no_fault-only V-close remain. Nothing modifies current production
watchdog orchestration before parent observation. No arbitrary PID/path/mode/seconds.

```sh
python3 -B apps/auth-web/ui-candidate/guard-qualification/build.py FRESH --toolchain /path/to/pinned/existing/ARMv5-musl-toolchain
python3 -B apps/auth-web/ui-candidate/guard-qualification/test_pack.py FRESH
```

Fresh outputs only under the assigned guard-qualification evidence prefix; refused
overwrites. Source/header/compiler/runtime/license/public script/image hashes and
commands are recorded. Actual HOST optimized + ASAN/UBSAN runs each last7s; owned
pidfd SIGTERM/args/FD3/clock fault/parser/source negatives are finite. No ARM execution,
sudo, target/status/proc/fuser/socket/UART/watchdog/PHY/GPIO/keys/card/footage/Git.
Fault injectors link only HOST variants. C++/header API N/A (no API/header exported).

Expected parent guard-only chronology: helper READY near exec; source first match
and ping1 around4s; helper normal exit≥7s; guard second check near8s sees own child
gone, waits actual0, V-close only after its positive admission/no fault. Record
actual RX scalar timestamps/order plus target helper monotonic stamps; ping/write
time cannot be inferred as an exact kernel reload timestamp from RX alone. This
pack preserves the reviewed script; it does not add unreviewed clock/device reads.
Parent must retain observed timing uncertainty and cannot claim a hard scheduling
upper bound from source/sleep or logs. No CPU-load/fps/auth/UI/trust/G3/G8 acceptance.

**One next parent step:** current i ownership/pinned75 image/config/prompt/idle/
watchdog/UART + fresh existing recovery admission, parent stages/verifies this
single root-owned public helper under the fixed path, then executes only the
standalone guard-qualification command once and records first match/ping/helper
7s/wait0/magic-close/actual poststate. No UI3/new producer campaign or target mode/
permissions change by this worker. Active radio restoration is exclusive parent work.
