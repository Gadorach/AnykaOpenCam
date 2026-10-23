# Cold23g — power/readback exercised; full cold context FAILED/HOLD

2026-10-23; BASE-01/05/07 / PLAT-06/08. After separate passive23f and focused
post23e method review, parent explicitly selected ONE new23g with UNCHANGED v4.
No retry-until-quiet loop, filtering or source/guard change was used.

Actual private power evidence:

- Initial strict GET OFF accepted; ON POST acknowledged, single strict GET ON
  accepted after the existing nominal3s settle. This exercises the v4 wait order,
  but does not prove the specific cause of23d's generic early failure.
- One original automatic catch trigger, three completed19-byte packets (57TX).
- Cold-observation phase rejected the complete597-byte RX context. One unexplained
  leading byte precedes the otherwise exact360-byte19-record profile and two118-
  byte fixed catch echo groups. One original U-Boot banner/held prompt; no kernel
  handoff text. Raw is retained byte-exactly, not sliced/relabelled as a pass.
- The original recorder's internal boot-window event reports one prior byte
  discarded from its trigger buffer. That does NOT discard it from raw capture
  or authorize the parent's first-byte-zero classifier to ignore it. The strict
  caller correctly remains HOLD. Planned separate10s quiet was not reached.
- OFF POST acknowledged, one strict GET OFF accepted after nominal250ms settle;
  same-owner stop and actual owned wait0/finalization succeeded. Private final
  hashes checked. Arm-to-stop approximately3.817s; no timing/recovery guarantee.

Independent GET after the controller again reported OFF; child task and active
socket absent, no visible tty owner. Source/original-input postchecks completed.
No electrical rail measurement or current Linux/watchdog admission. No loader,
image, readback command, RAM boot, GPIO/motor/NOR/storage/radio/media effect.

**No further power-cycle/catch attempt selected.** Neither23e's two pre-ON bytes
nor23g's one power-transition-prefix byte has an established provenance. Passive
23f does not explain them. Do not trim a byte, search a later banner, relax the
classifier, declare the prefix harmless, or retry to obtain a clean sample.
Hardware qualification is blocked on understanding the unexplained serial data;
electrical/signal-integrity evidence may be required. Do not re-ask the already
confirmed load association, supply path, visible power-up or UART-only wiring.

Original radio/reset/23c/23d/23e/23g failures remain independent and unchanged.
Camera stays controller OFF, no active capture/transfer. Parent continues eligible
feature development: one isolated source author owns a fresh real separate RTSP
provisioner-child/private credential ingress/readonly FD IPC candidate, not a
hardware/diagnostic-image/clock/receipt lane or main/native adoption. Parent review
is required; actual credentials/native hardening/media execution remain unselected.

Curated Git note: preserved v2/v3/v4 unified-diff files contain the standard
single-space blank-context prefix, which Git flags as trailing whitespace when
the patch itself is added. Keep those sealed diff bytes; the other selected
source/public metadata passes the whitespace check. Ordinary index unchanged;
private captures/configuration/response digests and generated binaries excluded.
