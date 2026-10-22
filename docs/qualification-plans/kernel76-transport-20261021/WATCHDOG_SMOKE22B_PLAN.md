# RAM76 normal watchdog smoke — one bounded functional operation

2026-10-22; PLAT-06 / RP-00–01. Parent selection after actual22b boot and fixed
read-only idle PASS. This is ordinary operation confirmation, not penetration,
fault-injection or mutation testing. No deliberate reboot on the normal path.

## Source and current admission

Exact76 image/package/full RAM CRC and actual monitor/startup watchdog closure
passed. Fixed14-query idle/sameboot/PID1/UID/CMA/mount/module/ISP/watchdog/UART
postcheck passed11:49:33UTC. These are fresh native receipts, not a current memory
load budget, radio/association or product acceptance.

`apps/qualification/kernel76-current-state/watchdog22b.py` is a single fixed parent
controller. Parent inspected its source, the existing monitor's `watchdog_test(0)`
and dispatcher, and the source-bound driver/core review. `--check-inputs` passed
without UART/RPC/device access, verifying the earlier source/build binding and
fresh postcheck pins. Existing boot/postcheck/transport/image bytes are unchanged.

Before any target effect, it requires the exact same private boot checkpoint/
UUID, source/sole peer/start/cumulative TX, fresh future catch and >=600s initial
coverage. Original failed radio HOLD is locked and compared, never cleared; the
new22b subordinate marker is sticky on error. Every nested low-level send/status
RPC observes the same peer/time/HOLD fence and10s send reserve. Total phase240s;
capture reserve stays >300s, >=360s checked again before watchdog smoke and >=90s
normal-postflight time remaining. No target timeout/guard duration increase.

## Fixed operation

1. Fresh sameboot14-query idle inventory; cached PID1 FD inspection through one
   owned shell (only console descriptors0–3), then actual shell return. Confirm
   `AK3918EV200 TOP watchdog`, current logical timeout1..60, inactive/nowayout0.
2. Exactly one `wdt smoke` plus CR. Actual monitor opens/arms watchdog, sets5s,
   sleeps2s, sends one ping, sleeps2s, gets exact5 and magic-closes. Require its four
   exact positive messages and fresh returned monitor prompt; no generic-error pass.
3. Ten seconds passive no-reset observation, no feeding, then sameboot14-query
   idle inventory, PID1 console-only FD inspection/owned shell return and timeout5.
   All raw/UUID/FD detail stays in private evidence; public outcome is scalar only.

No GPIO, radio, module, endpoint reset, network, capture/media, actuator, storage,
persistent write or arbitrary monitor command is permitted. The cached shell query
does not open watchdog/USB. This does not invoke the old station attachment guard.

## Failure/recovery and claim boundary

If the monitor hangs during smoke, its already-armed5s watchdog and future capture
catch are the recovery path. No second sender, `wdt reset`, kill, unload, FD stealing,
timeout growth, repair query or automatic repeat. Any failure leaves subordinate
HOLD and private evidence; observe actual reset/catch separately before treating
ownership as invalidated. Manual OFF remains fallback if watchdog/control/catch
is unavailable or fails. The source still cannot guarantee electrical reset domain.

Normal success proves the actual timeout/ping/get/close receipts, no reset in the
passive interval, stable boot and observed logical inactive/FD retirement. It does
**not** prove electrical stop, calibrated timing, deliberate reset, every-hang
recovery, live DMA retirement, radio cold cycle or full PLAT/G8 acceptance.

One source-reviewed explicit invocation, only after current gates pass:

```text
python3 -B apps/qualification/kernel76-current-state/watchdog22b.py --one-owned-idle-smoke-only
```

No result is claimed by this plan. Source/controller and its imported helpers stay
frozen during execution; no concurrent UART/RPC/status/progress poll.
