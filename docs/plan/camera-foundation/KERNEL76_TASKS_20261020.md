# Active modern-kernel increment — candidate76

**Latest actual05:33UTC:** candidate76 **RAM boot + fixed read-only idle postcheck
PASS**, one upload/boot, zero frame retries. Target ON at idle RAM76 monitor;
UID0/PID1init, exact bootargs,16MiB CMA, no modules/flash/SD mounts, ISP idle,
watchdog inactive and UART errors/busy0. No radio/network/media/controls started.
05:34UTC same20c ownership/source/catch/HOLD revalidated, bounded through08:02UTC
DATED/revalidate before future ops. No additional hardware queue or workers.
See [native result](../../qualification-plans/kernel76-integration-20261020/RAM_BOOT20C.md).

**New user request:** continue RAM boot testing. Fixed RAM76 native entry is active
under fresh20c, parent-only, scoped to boot and read-only idle/runtime checks.
See [RAM_BOOT20C](../../qualification-plans/kernel76-integration-20261020/RAM_BOOT20C.md).
User freshly OFF/UART connected, then ON after capture readiness. Cold original
U-Boot/BOOT-pin/neutral readback passed; native76 boot result is pending. No
radio firmware/association/traffic, media, controls, flash/card write or stock retry.

**05:13UTC:** fixed parent RAM76 upload/boot recipe launched in the background
after dry-run,24 mock checks and independent source review. **No concurrent UART
commands**, including status probes, while this operation owns the lease. Await
its completion before read-only postcheck. Do not claim native76 boot yet; an
active transfer or staged image is not a boot result.

Postcheck preparation: initial checker independently NO-GO (HOLD/TX-accounting/
parser defects), **never natively run**. Old source/reproducers preserved; only
host postcheck corrected,151 in-memory cases PASS and correction review pending.
Do not execute postcheck until both boot completion and renewed review pass.

**05:29UTC source update:** correction review GO independently reproduced151 cases
and19 scope/boundary checks. Corrected checker035c1069… pinned; active boot recipe
d0bbb91a… remains byte-exact. Still wait actual transfer/boot completion before any
query; no UART/RPC/status/progress probe was issued by this source review.

User requested continuation of remaining investigations or transition to new kernel
work. **Modern work ACTIVE, source/build only.** Stock's remaining native/physical
rows stay blocked in STOCK_REFERENCE_TASKS_20261020.md; no more stock boot retries.

**Earlier build checkpoint:** source/build/package increment COMPLETE offline; candidate76
has not booted. Final independent verifier20 checks PASS. Camera remains OFF, no
UART owner/socket/recovery arm or workers. Next is finite native-loader/test-entry
review, not automatic execution. [Results](../../qualification-plans/kernel76-integration-20261020/RESULTS.md).

Keep Linux6.18.55 pin725bd2f3… and kernel75 rollback byte-exact. Camera is user-
confirmed OFF/UART connected, no recorder/socket/recovery arm. No target operation,
flash/card write, AP broadcast, speaker effect, homing or GUI launch is queued.
All changes remain local/uncommitted/unpublished.

| Task | State | Output / boundary |
|---|---|---|
| K76-01 / NET-02 | SOURCE-REVIEW-PASS | Independent review found no blocking source defect. Anyka PIO-only byte helper at continuation/NAK; generic/DMA retained; one kernel source file changes |
| K76-02 / NET-02 | HOST+ARM-BUILD-PASS | Actual helper262144 cases each optimized+san;3 runtime mutants rejected with SIGABRT/exact assertion. Final usb-host05 passes current hardened recipes; final ARM object equals independently reviewed preliminary object; no native |
| K76-03 / PLAT-01 | BUILT | Isolated preliminary kernel76+station76 build passed; config/release/symvers checks and rollback preservation passed; first outputs retained, not overwritten by packaging |
| K76-04 / PLAT-02 | REVIEWED+ARCHIVE-PASS | New cpio76 replaces stale radio/RTSP, adds ELF DHCP bridge; all47 other records byte-exact.17 archive tests/7 optimized-Python refusals pass; no autostart/GUI. Specific stale payloads fixed, not whole PLAT-02/product closure |
| K76-05 / PLAT-01/02 | PACKAGED+VERIFIED | Final fresh ARM kernel/radio builds pass; final module byte-equal embedded. QC6U76.UIM3603555B/bd8a2a08…;20 independent final checks pass;75 and preliminary76 preserved |
| K76-06 / NET-02/04 | GATED-NOT-EXECUTED | RAM76 boot/idle admission passed; radio RAM-loader/full-size USB/network trial still needs its own reviewed entry/scope and current owner/recovery revalidation. Keep384 rollback; no automatic queue |
| K76-07 / BASE/PLAT | NATIVE-PASS-NARROW | Fresh20c cold catch/pins, full exact image upload+CRC/monitor/startup-close and approved fixed RO idle inventory passed05:33UTC; native transport/media/peripherals/product remain open |

Only the USB defect hypothesis is changed first. No incidental CMA/timeout/FIFO/
PLL/radio-credits/firmware/MTU-default change, kernel rebase or disabling guard gates.
MMC/audio need shared L2/clock ownership before enabling; protected AP and stock FPS
remain unresolved. Preserve all failed builds/tests and name one next increment.

## Checkpoint

Source/tools: `bringup/qc07f/usb-continuation-candidate/`.
Generated source76 and build logs are new `native-usb76-source01` / `native-usb76-build01`
directories under the retained foundation work tree. Original75 is untouched.
Host result: `evidence/qualification/qc07f/kernel76-20261020/usb-host05/result.json`.
Runs01/02 fixture-mutant compile failures are preserved, not relabelled passes.
Independent USB/package source reviews and final artifact verification completed.
Exact recipes used for the completed build are preserved in the package output's
`recipe-snapshot/`; subsequent host-only safety hardening does not change the image.
Replay of the hardened archive transformer reproduces selected cpio76 byte-exact.
No target operation queued.
Future native proposal (not execution):
`docs/qualification-plans/kernel76-integration-20261020/NATIVE_TRANSPORT_PLAN.md`.

## Final artifact / evidence

- Image: `work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/images/QC6U76.UIM`.
- SHA256: `bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f`.
- Staged RAM range82408000–82777c63; decompressed81808000–82059bc0. Both bounds
  independently checked. RAM-only candidate; does not fit existing flash KERNEL.
- Actual final-build gzip archive matches selected cpio76; rebuilt final radio
  bytes equal embedded radio, not just a vermagic match. Source pin6.18.55 retained.
- Final manifest: `analysis/native-usb76-package01/package76-manifest.json` under
  the foundation work tree. Source-only USB patch and explicit cpio/config-path
  deltas are recorded. No private credential/config/firmware payload was added.
- Final20 checks: `evidence/qualification/qc07f/kernel76-20261020/final-verify01/result.json`.
- Final parent checks04:53UTC: `FINAL_PARENT_REVIEW01.json` in that evidence root;
  adapter unowned, socket absent/HOLD empty, completed recipes preserved.
- Native76 boot/USB/MTU1500, media cadence/load, GUI76/peripherals, shared-L2 MMC/
  audio, protected AP, security/product and persistent-install gates remain OPEN.
