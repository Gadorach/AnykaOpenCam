# Cold23c — parent EOF-classification FAILED/HOLD; subsequently OFF

2026-10-23. User confirmed current UART RX/TX/GND-only wiring, then actual camera
OFF/readiness. Fresh sole original recorder/source/peer/part and zero RX/TX were
verified, one hold-cli catch positively armed, then user reported ON while ready
to turn OFF. No image, readback command, boot command, motor or flash operation.

The parent caller **FAILED/HOLD** by passing the entire596-byte capture into the
frozen EOF-only first-prompt classifier. It did not reach its planned10s quiet
step. Preserve that failed call/error and all raw/events unchanged. No second arm,
catch/reset retry or rescue TX occurred. User turned the camera OFF on request.

Separate saved-evidence inspection established:

- One new original U-Boot banner, no kernel/read-handoff marker, final held prompt.
- The first360 bytes are exactly the original19-record cold production, including
  its expected challenge text and first prompt. The remaining236 bytes are exactly
  two complete fixed catch-key echo/unknown-command groups, not unexplained bytes.
- Three original19-byte automatic catch packets completed, total57TX bytes; no
  other UART TX was requested. The first packet establishes the first prompt; two
  remaining packets account for the four unknown-command diagnostics/echo groups.
- A group is `D(KEY) + D(CTRL-Q + KEY)`,118bytes, where `KEY` is the already audited
  delay key and `D(C)` is command echo, one LF, the public generic U-Boot unknown-
  command diagnostic for that same fixed command, CRLF/CRLF and held prompt.
  Privacy review found only those fixed software/control tokens; no variable
  device/credential/identity field is needed to describe this trailer.

These offline facts explain the caller's **interval-selection error**. They do
not change the old classifier's valid EOF contract, relabel the failed attempt
PASS, supply the unperformed quiet test or admit a native effect. A future caller
must classify exactly the first-prompt interval AND independently validate EVERY
remaining byte and its actual catch-packet lineage. It must never discard extra
bytes, normalize a mismatch or search a later banner to manufacture a pass.

After the actual OFF report, a separately bounded same-peer stop RPC finalized
only the proven recorder. Private raw/event hashes matched its final manifest;
normal stop/capture_end, no redaction/error, socket absent, recorded process absent
and no visible tty owner. Wait exit status was NOT observed: the original startup
helper does not return its Popen object. Do not claim wait0; the new transaction
must retain its actual child object. Original HOLD/instance bytes/modes/inodes
remained exact. No active capture/transfer remains; new failure stays preserved.

## Newly available actual power control

User identified the ESPHome endpoint referenced by ms42p-switch-control as now
powering **QC07F only**, not the Meraki switch or another load. Parent read-only
GET returned HTTP200/OFF, then one idempotent turn_off POST returned HTTP200 and
GET again read OFF. No ON/power-cycle was requested through it. Exact endpoint/
association and all private evidence stay in the protected outside-Git envelope.
Skill's MS42P target/UART/image instructions do not apply to this camera.

**Current state: user-reported camera OFF plus outlet-controller OFF readback.**
No electrical rail measurement, Linux runtime, armed watchdog or native recovery
lease is claimed. Last positive target text is the new held U-Boot during23c,
before the subsequent OFF, not Oct22 Linux/RAM76. Both original radio/reset HOLDs
and this caller failure remain independently preserved.

Parent selects fresh source-only `qc07f-cold-entry23` for one complete foreground
owned-child/arm/explicit camera-outlet ON/full-context+trailer/quiet/OFF/finalize
transaction. One author owns that new app/docs lane, never hardware. No new image,
clock/receipt framework, classifier edit, native trial/retry or controller execution
is queued. Independent ordinary source review/parent selection precede invocation.
