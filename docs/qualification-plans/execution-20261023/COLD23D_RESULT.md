# Cold23d — automatic transaction FAILED/HOLD, outlet returned OFF

2026-10-23; BASE-01/05/07 / PLAT-06/08. V3 was invoked ONCE as
foundation-20261023d after ordinary R1/R2 source reviews and parent selection.
V1/v2 were never invoked. Preserve their source/reviews and all23c/older failures.

The actual private result records initial GET OFF, ON attempted, OFF attempted,
GET OFF proved, recorder finalized and actual owned-child wait return0. RX/TX
were both0; no catch packet, banner, prompt or kernel text was observed. No
loader/image/boot/readback/motor/NOR/media operation occurred. Independent GET
after the transaction again reported outlet OFF. No active capture/transfer.

**ON readback was not accepted.** The earlier statement that ON then OFF were
verified was wrong; POST intent is not GET state evidence. The original generic
ValueError does not locate the rejected predicate. Arm-to-stop was0.19894351s;
capture open-intent-to-final was0.261695036s. Its planned3s wait was never reached.
The user confirmed the supply path is correct and the camera visibly powered up
during that brief ON. Do not re-ask those physical questions or infer failed
wiring/no camera power from0RX. No electrical rail measurement is claimed.

The immediate strict ON GET may have raced asynchronous ESPHome dispatch. This
is a hypothesis, NOT an established diagnosis or permission to retry frozen v3.
Fresh v4 moves the existing3s wait before the single ON GET, adds a nominal250ms
OFF-dispatch settle and private separate acknowledgement/actual-state/failure-
phase fields. Strict state checks, one arm/ON/OFF, original classifier/recorder,
all custody/finalization guards and sticky errors remain. No retry/polling policy
or C1/native-clock/independent recovery claim is introduced. Independent review
and fresh parent selection precede one new23e transaction.

Private intent/capture/results and endpoint association stay outside Git. Both
original radio/reset HOLDs,23c EOF-caller failure and23d failure remain unchanged.
