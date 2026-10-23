# Passive23a preflight refusal — before UART open

2026-10-23. The first inline parent preflight refused on historical radio-marker
mode0644 versus its selected exact0600 expectation. No serial open, capture/output
directory, RPC, TX, catch, power/reset, motion or target write occurred. No marker
byte, mode, name or inode was changed. The original radio and reset HOLDs remain.

Read-only investigation found the canonical control directory owned0700; original
radio marker owned regular/nlink1/0644, reset marker owned regular/nlink1/0600,
GUI-instance lock owned regular/nlink1/0644 and empty. Both exact logical HOLD
records match. Unprivileged fuser reports no selected-tty owner; privileged global
FD inspection is unavailable. Historical recorder/socket absent. These are host
observations, not a fresh target boot/runtime/quiet or recovery assertion.

Do not chmod/replace/migrate original markers. C1v2's exact0600 check remains
unchanged and **unadmitted**; this result does not call or qualify C1. No fresh
native operation is admitted by a failed preflight.

Parent seeks a bounded independent ordinary review of the following **passive-only
caller-profile correction**, not a generic permission/framework change:

- Exactly named historical radio/GUI-instance files may be read-only/NOFOLLOW/
  CLOEXEC opened and exclusively flocked at their observed0644 mode, and the
  exact reset marker at0600, only below the exact owned0700 canonical control
  root and trusted nonwritable ancestry. Require regular owned nlink1 files,
  unchanged path/FD inode association, exact logical HOLDs/empty instance bytes,
  byte/mode/inode comparisons through failure and normal close. No writes/chmod.
- This read-only/public-logical-marker role is distinct from **all new private
  capture files**, which remain0600 inside fresh0700 outside-Git directories.
  Group/world writable historical markers, alternate names/roots or unknown
  contents still refuse. No copied marker or C1/private-output exception.
- Run only the already reviewed unchanged explicit-port recorder in full private
  capture mode, explicit exact historical adapter,120seconds, new foundation part.
  No RPC client, TX/catch arm or other hardware primitive is selected. Original
  capture code requests DTR/RTS false before exclusive open and discloses possible
  electrical glitches/initial input flush; no software connection proof is claimed.
  Preserve known UART-only wiring; any evidence of reset/boot/power connections
  or a competing GUI/serial owner blocks even passive opening.
- Own the cooperative locks through completion; await the recorder with its
  ordinary finite duration, no concurrent hardware/status polling/helper edits.
  Keep console/raw/events/final private. Require normal exit, matching final file
  pins, no TX/abort/control events and unchanged original markers. Silence is
  UNKNOWN, never OFF/held boot/healthy Linux or permission to send a newline.
- No native timer/recovery/whole-interval guarantee is borrowed. C1/clock/native
  effect gates and both historical failures remain exactly blocked/unchanged.
  Root visibility and current electrical-state limits must remain explicit.

Only after the focused review and parent disposition could this corrected
passive-only profile be selected. No capture retry or current physical request
is automatically queued by writing this document.
