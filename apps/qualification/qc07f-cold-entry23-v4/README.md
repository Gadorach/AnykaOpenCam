# Cold-entry23 v4 — one fresh cold observation candidate

Read [CONTRACT.md](../../../docs/qualification-plans/qc07f-cold-entry23-v4-20261023/CONTRACT.md)
and the inherited v3/v2/v1 contracts. V3's one foundation-20261023d attempt remains
FAILED/HOLD: initial OFF, ON attempted, OFF read back, owned recorder exit0 and
finalization, but0RX/0TX. ON readback was NOT accepted; no3s wait was reached.

V4 moves—not extends—the existing3s ON settle BEFORE the single GET ON. It adds
a nominal250ms settle before the single OFF GET, including after an uncertain
OFF POST. POST acknowledgements and actual strict GET values/phase are recorded
separately in the private result. Null never means an observed state. No HTTP
retry/polling, inferred state or classifier/ownership guard change.

All other v3 guards and limitations are inherited unchanged, including one arm,
ON and OFF; pinned original recorder/classifier;360-byte cold profile plus every
catch echo byte; separate10s quiet; known-child stop/wait and private final hashes;
read-only original HOLD/input postchecks. No native effect/recovery/product GO.

Use v3's explicit argument shape with THIS script and a fresh part. Parent review
and fresh current prerequisite checks must precede any invocation. Nothing has
been imported/called/run by creating this candidate; no boot/image/RAM/flash,
motor/GPIO/media operation or automatic retry queue follows.
