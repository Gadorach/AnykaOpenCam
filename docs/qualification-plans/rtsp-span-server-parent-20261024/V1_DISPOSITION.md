# Authenticated span sender v1 — R1/R2 required, NOT ADOPTED

2026-10-24; RP-09/B07, bounded STREAM-01/02/03/07 and CAP-01/05/07.
Parent read the independent review and actual affected source/header paths.
Accept its **correction-required R1/R2 findings**; no source lifetime/order
sufficiency or adoption of this exact sender revision.

- R1: observed/sticky clock failure can enter finish_au's scalar release, active
  flag clearing and partial-packet erasure before the later retention path.
  Eventual nonreturning retention does not restore original live cursor state.
  Direct pause/close also need a pre-cleanup clock/ownership-error fence.
- R2: pause clears demand_attached before the actual detach and accepted post-
  callback clock check. Failure can return into a second cleanup pass with a
  false detached flag. Keep ownership until acknowledgment; uncertain detach
  must immediately retain without return/retry/flag retirement.

These are ordinary static control-flow findings, not executed failure cases.
They do not revoke the accepted v2 hub or span packetizer. The real data path,
typed sink, final build03's six HOST/ARM compiles/two REL links/26 commands and
zero runtime calls remain their bounded facts, not a source ownership pass.
Earlier build01/02 link-check failures remain FAILED_PRESERVED, not relabelled.

Freeze source C `529fd4902931467dc896b6faa12d502a20846cc0ba4cb76060044b453a018f97`,
header `8f6488595576c4205b9e9548f18f5e63109b41398cfab6319ce702261b980a07` and
review `5ae6708f00f3e5e6465730b9b47c6726278964a4bc3e294cba25bae8cf4b9871`, with all
original evidence/consumed recipes. Parent selected a fresh minimal v2 correction
at rtsp-span-server-v2; compile and independent review precede any source adoption.
No test/negative/adversarial/mutation replay, runtime invocation or hardware action.

A disjoint qc07f-span-memory author continues actual chunk prefault/accounting
source. That lane is not a correction to this sender or native admission. Native
adapter/ready integration, producer1MiB/raw buffers,2MiB process/combined load,
external retention/recovery/main and RAM75 C1/recovery remain open. Last camera
boundary is controller OFF/finalized23h, not a new observation or live lease.
This parent record is outside all frozen author/review directories.
