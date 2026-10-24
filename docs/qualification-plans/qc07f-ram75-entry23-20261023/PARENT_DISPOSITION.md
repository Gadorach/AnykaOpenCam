# Parent disposition — retained-image facts accepted; native entry BLOCKED

2026-10-23; BASE-01/02/05/07, PLAT-01/06/08. Parent read the complete findings,
actual QCL loader, retained watchdog driver and PID1 startup/idle paths. Freshly
matched51/51 public pins and independently checked exact image header/data CRCs,
size/load/entry/staging endpoint. Accept the **bounded offline finding**, not a
controller, replay, new image, native boot or qualified complete recovery path.

The exact3,574,675B RAM75 image and static staging extents are intact. This image
exceeds the existing1.5MiB KERNEL partition: NOT flash/install material. Static
fit is not combined-load/CMA/128KiB-cap qualification or old-toolchain reproduction.
Built-in sensor reset/MCLK, radio-power/USB enumeration and ISP allocation occur
on boot even though PID1 does not autostart station/firmware/RUN/RTSP/capture/motors.
Do not call the proposed boot write-free or allocation-free.

Recovery is not covered at future ON/U-Boot/word staging/READY, after QCL commit
or after PID1 magic-close. QCL refuses prearmed TOP at entry and arms only on its
first non-cancel byte; it disables before COMMIT_OK. The Linux core may service an
inherited watchdog indefinitely before userspace open; PID1 then attempts stop
without checking write/close results. Neither a prior boot nor a success message
establishes protection for a fresh phase. Independent recovery and separate strict
C1 whole-interval clock admission remain unresolved. No old guard/class/global
bypass, guessed deadline, fresh receipt or generic shell route is admitted.

Parent additionally performed bounded read-only GETs of the exact confirmed outlet
controller root and state-event stream, storing responses privately outside Git.
Both returned HTTP200; four entity records were observed and no timer/countdown/
duration/auto-off entity was advertised in that sample. No POST/configuration/
firmware/power change occurred. This is NOT proof of all firmware capabilities
or absence of hidden automation, and no autonomous cutoff was found/admitted.
Same-host HTTP OFF still does not execute through host loss/suspend. Do not add
an outlet firmware/configuration change, cloud/account action or clock framework.

**No controller/source implementation or native trial selected.** A real reviewed
independent fallback (or actual manual readiness for the uncovered phases) and
the separate clock admission would still be required. No such readiness is
asserted/requested here; broad authorization alone does not supply these facts.
23h cold PASS stays scoped to its finalized epoch, with camera ending OFF and all
earlier failures independently preserved. No transfer/capture is active.

Parent instead selects the proposed eligible source FEATURE: fresh derivative
bootstrap registration of the accepted real32-byte OS-random callback BEFORE
provisioner snapshots, never replacing a live/custom callback. One isolated author
owns qc07f-bootstrap-os-random app/docs; original APIs/source/seals frozen. Actual
bind→prepare→start/HOLD/finish custody remains, no test/child/native execution or
entropy/adoption claim. Independent review precedes use. Original findings and
public pins may be committed locally; no image/private evidence or push.
