# Attempt24a — HOLD on exact SPI response envelope; OFF/finalized

2026-10-24 · BASE-01/05/07, PLAT-06/08; supports RP-11.
The ONE selected entry24-v2 invocation exited1. Preserve it as **FAILED/HOLD**,
not a boot/driver result. No second invocation or automatic retry selected.

Actual initial GET OFF, one ON acknowledgement/GET ON, one OFF acknowledgement/
GET OFF succeeded. Recorder stopped and actual owned wait0/finalization completed.
Parent independently checked both private final-file hashes, all20 retained input
bytes/metadata including three original control markers, then one independent
GET OFF/PID absence/socket absence/no-visible-UART-owner postcheck. No manual OFF
is currently required; controller OFF is not an electrical-rail measurement.

Final counts:845 RX/163 TX. Three original19B catch packets57 TX and the exact
cold/quiet gate preceded controlled work; first controlled echo starts at596 RX.
Exactly eight completed controlled sends106B match the first four closed commands:
SF probe, RAM scratch zero, scratch CRC, original BOOT-range read into scratch.
No subsequent BOOT-image CRC check, neutral-register read, loader word/GO/QCL,
Linux/image boot, motor/radio/station/MTU/mic/audio/media/storage/flash-write command.

Saved evidence localizes refusal to the SF-read envelope predicate. The expected
full-range success record is present exactly once, with no extra RX after its
held prompt. After the already-selected CR normalization, the response has TWO
leading LF records before the fixed SF success line; v2 requires exactly ONE.
The remaining fixed success record and two LF before prompt match. This is a
specific console-envelope discrepancy, not evidence that hardware read failed,
permission to trim unknown bytes, or a diagnosis of earlier23e/23g RX anomalies.
Raw/identifiers/private hashes remain outside Git. The actual failure is unchanged.

Select only a fresh bounded v3 SOURCE correction/review for this observed envelope:
explicitly enumerate exactly one OR two leading LF for this exact fixed SF-read
reply, with all size/address/status/prompt/other-byte checks unchanged. Do not use
general whitespace stripping, prefix search, arbitrary blank lines or watermark
repair. Freeze v2 and all private24a evidence. Source/AST only; no new target
invocation without separate reviewed method selection and current supervision.
All clock/ownership/no-retry/custody/rollback/driver-first constraints remain.
