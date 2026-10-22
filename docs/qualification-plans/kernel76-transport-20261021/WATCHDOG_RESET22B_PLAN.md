# One idle watchdog reset22b — functional recovery qualification

2026-10-22; PLAT-06 / RP-00–01. Parent explicitly selects **one** controlled
known-idle CPU watchdog reset, ending at held original U-Boot. This is ordinary
recovery confirmation under the user's watchdog-reboot preference, not an active-
DMA, radio, peripheral, adversarial or mutation experiment. No reload is queued.

## Established prerequisite and scope

Actual RAM76 full upload/independent CRC/monitor/idle passed11:49UTC. One normal
5s timeout/ping/get/magic-close smoke and fresh sameboot idle/console-only PID1 FD
postflight passed12:05:31UTC. No reset occurred in that smoke; original radio HOLD
remains. Current receipts/checkpoints are still revalidated, not inherited authority.

Existing `WATCHDOG_RECOVERY_REVIEW.md` sections4–5 define this entry and limits.
Parent reread that review and actual pinned monitor/driver/core/catch source.
`reset22b.py` imports the frozen smoke's input/parser/preflight functions but has
its own exclusive operation and fresh private outputs. No helper/image edits or
timeout increases; no test campaign. It checks the smoke source pin against its
actual native receipt and revalidates the previous source/build binding.

## One exact sequence

1. Lock original HOLD without writing it; set durable sticky subordinate reset
   HOLD. Fresh sole recorder/source/peer/start and >=600s coverage, exact smoke
   checkpoint TX and unconsumed future catch; no competing transfer/operation.
2. Fresh sameboot PID1/root/mount/module/ISP/watchdog/UART/memory inventory, owned
   cached PID1 FD inspection and actual shell return. Require exact TOP identity,
   inactive watchdog/nowayout0, configured timeout5 and only console FDs0–3.
3. Revalidate sole owner and >=360s coverage; phase<=240s with >=90s reserve before
   effects. Durably record one-command intent and old epoch/cursors privately.
4. Exactly `wdt reset` plus CR. No later manual TX, arm, helper command or repair.
   Existing recorder's automatic future hold-cli catch is the only later sender.
5. Require arm receipt or actual main-CPU reset by5s; genuine original U-Boot and
   caught board5/DRAM64/held prompt by20s. Require the complete arm receipt for
   command PASS, although actual reset can precede full message delivery.
6. Observe at least10s stably held, no second banner, stock boot/SF/kernel handoff,
   inspection or extra sender. Reconcile private event sequence: one10B reset TX
   and1–3 actual19B audited catch packets only; intents/completions/counts agree.
   Revalidate same sole recorder and >=300s remaining. **Stop at U-Boot.**

Controller status reads during this bounded operation are its sole owned receive
observation, not concurrent parent polling. No other UART/RPC/progress operation
or controller/helper edit is allowed during execution.

## Result and failure meaning

Positive reset invalidates the old Linux boot/FD/control epochs; it does not prove
normal close, active-DMA retirement or radio rail power cycling. Consumed catch
must be rearmed and revalidated before any separately selected later effect.
One good idle reset does not clear original radio failure or full PLAT/G8 gates.

Any error, missing/late receipt/catch, repeat reset, boot escape or unknown owner
leaves subordinate HOLD. No reset/reload/RUN retry, second sender, kill, unload,
FD stealing or cleanup of uncertain backing. If watchdog/catch fails, manual
OFF/leave OFF is fallback; no user interaction is needed on a positive result.
No result is claimed by this plan.

```text
python3 -B apps/qualification/kernel76-current-state/reset22b.py --one-idle-reset-stop-at-uboot
```
