# Existing guard review — unadopted source candidate

This is an isolated finite source/forensic increment, **not** a new guardian,
daemon, transport, target retry or native availability claim. Nothing in the
current parent recipe/native entry/callbacks/proofs/service/client/kernel was edited.

`candidate.py` accepts only the exact observed native04 guard grammar. It preserves
exclusive FD3 open/initial ping, child FD3 closure/fork/exec, actual `$!` PID/parent
markers,45-count loop,4s sleep/feed cadence, source-owned wait and nonzero6s recovery.
The existing4s sleep moves **before** the exe check, not after it. No widened5s
timeout, cadence/counter/delay, service180s/client140s/2MiB/35HTTP40CPU or auto respawn.

To avoid feeding a recycled/unrelated same-exe PID, the same guard binds its own
forked child's initial PPID/starttime from `/proc/$auth04_pid/stat`, without requiring
the exec image immediately. Each post-sleep check compares that source tuple and
the exact existing launcher path. POSIX `set --`/parameter expansion parse field22
after the comm-closing parenthesis, no awk/stat/ps/argv/env print. Exact fork/PID/
parent/pinned READY proof before public effects remains the parent's responsibility;
this telemetry is not a new image/readiness protocol or substitute.

Bounded public telemetry records first exec match, ping number, source/exe refusal,
gone PID and observed wait status (numbers1..45 or actual exit). It contains no
credentials/args/env/private content. Magic-close is permitted **only** after the
actual own wait0 **and** observed exec admission with no source fault. Fast zero
before admission/wrong-exe/birth reuse/nonzero do not magically close the watchdog.
No external PID selector or signal, arbitrary permissive feeding or root file daemon.

## HOST-only controls

```sh
python3 -B apps/auth-web/ui-candidate/guard-review/run_fixture.py FRESHrun
python3 -B apps/auth-web/ui-candidate/guard-review/test_source.py
```

Fresh outputs under `evidence/delegated/20261012-native-ui/guard-review/` refuse
overwrites. The HOST shell forks a single public process, which genuinely delays
and execs another public fixture role under the same PID. The proc inspector is
explicitly fake and reads only that owned process's fixture record. FD3 is a fresh
**ordinary file**, never `/dev/watchdog0`. Real sleep4 remains; seven actual owned
process cases complete within90s. No actual host/target proc/fuser/device inspection,
root binary/private input/key/frame/bench/parent PID operation. Old controlled
missing-periodic-ping failure and all successful controls are retained.

These cases establish an executable race counterexample, **not native02 reset cause**.
Native02 recorded pinned strict2MiB READY, client start≈0.48s and reset≈5.68s after
bootstrap TX, but did not record the first guard lookup/ping. It issued no complete
TLS/asset/login campaign PASS or normal SIGTERM retirement. Possible race remains
unproved as cause. Native RAM CRC mismatch/refused boot is a separate current parent
boundary, not an operation or retry permission for this job.

## Hard unresolved deadline

Source does **not** guarantee root CPU/watchdog scheduling. BusyBox sleep4 gives no
upper bound on dispatch/exec-update-lock/cat/ls/write latency; Anyka nominal5s timeout
leaves only≈1s for all overhead/jitter. Forty-five4s sleeps plus inspection overhead
can extend wall time beyond180s; service runtime begins later at actual READY.
Source supports neither latency/headroom nor that delayed exec always finishes by
the first4s check. If not admitted then, the candidate refuses feeding and preserves
existing bounded recovery rather than lengthening timeout or starting another child.

Native first-open/heartbeat/nowayout/close timing, actual2MiB/CPU contention and all
G3/G8/trust/durable/product gates remain open. **One next parent step:** review this
diff/control table, establish the existing protected current image/owner/recovery
boundary, then qualify only the guard in one bounded native run with scalar first-
exec/ping/wait evidence before any new UI3 or producer campaign. No blind same-job retry.
