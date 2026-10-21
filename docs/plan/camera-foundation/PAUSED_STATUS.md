# Paused implementation — user-requested halt

## Current superseding checkpoint — Oct16 SSH-only install complete

The user explicitly resumed Oct15. Persistent SSH-only ROOT is now **flashed and
cold-boot verified**, including full8MiB byte readback against the private expected
composition and all nonROOT bytes unchanged. Earlier NOT-flashed pause statements
below are historical, not current state. See [current resume](RESUME_20261015.md)
and the [installation result](../../qualification-plans/stock-reference-20261013/persistent-ssh01/INSTALL_RESULT_20261016.md).

Camera left powered on, stock3.4.35/BusyBox initPID1, key-only SSH at last observed
DHCP192.168.1.73:22222; normal camera/cloud/recording/homing startup disabled.
Private15c capture sealed/hash-verified/stopped: **no UART owner/socket/arm remains**.
No automatic GUI/peripheral/continuous queue. The bounded SSH installation is done;
further hardware work still needs fresh runtime/owner/recovery, not old PIDs/timers.

## Explicitly resumed — 2026-10-15

User resumed and confirmed camera unplugged/UART connected. See
[current resume](RESUME_20261015.md). Fresh15a sole receiver/future hold was
verified22:25UTC; no boot observed yet. Persistent SSH is still not installed;
write-image/backup/recovery gates remain. Prior pause below is preserved history.

## Current pause — 2026-10-14

**Paused again on the user's latest request. Persistent SSH was NOT flashed.**
Only a NOTFLASHABLE dummy ROOT image was validated; final private image and safe
programming/rollback gates were missing. No SPI/SD-data/saveenv/OTP write occurred.
See [the current halt handoff](HALT_20261014.md), which supersedes the older resume/
pause/runtime paragraphs below. Do not resume from background review completions.

Last target: original stock U-Boot held `anyka$`, no Linux handoff. Recorder13h is
stopped/finalized/hash-verified; **no UART owner, socket or recovery arm exists**.
Camera power-off is requested, not observed: user should **OFF/leaveOFF**. Future
boot still uses original firmware, not installed SSH. Resume only on user request
with fresh power/wiring/owner/runtime/recovery and exact write-image review.

---

**Historical pause; explicitly resumed2026-10-13.** User powered on; fresh13a
capture observed U-Boot held, no Linux handoff. See [RESUME_20261013.md](RESUME_20261013.md)
and [current review/strategy](REVIEW_AND_COMPLETION_STRATEGY_20261013.md). The prior closure,
results and remaining work below remain unchanged evidence, not current runtime.

**Work is paused. Do not resume the old continuous queue automatically.**
The user's latest instruction is to finish active work, provide status, update
documentation and halt. This overrides all earlier “continue after completion”
instructions and historical active-job statements. Resume only on user request.

## Last observed bench and shutdown

- QC07F: exact kernel75 **RAM Linux6.18.55**, public kernel notes/config matching
  the sealed image;40MiB system/16MiB CMA policy unchanged.
- Authorized `VertexHotspot`: WPA2/CCMP, DHCP192.168.1.73/24, WLAN MTU384.
- ISP/capture idle, guard errors0, UART error/busy counters0, watchdog inactive.
  Qualification listeners18443 and17891 absent. Existing guest networking remains
  running; these are dated observations, not durable configuration/trust acceptance.
- Guard-only helper retired normally; its two exact owned RAM copies were removed
  after pin/retirement checks. Original PC builds, failures and private inputs remain.
- UART recorder **i stopped and finalized cleanly**, original hashes verified;
  no serial owner/control socket remains. Its future-boot arm was removed with the
  recorder. **There is no current UART owner or armed recovery protection.**
- UI03 was cancelled before target staging/execution: the finite public server
  closed with **zero transfers**. New private03 inputs exist only on the PC, in
  protected storage; do not log/hash/stage their payloads or reuse them as defaults.
- No new SPI/SD-data/saveenv/OTP writes, commits, pushes or publication.

Closure evidence:
`evidence/delegated/20261012-parent-review/USER_HALT_actual-runtime-and-scopedcleanup01.json`
and `USER_HALT_verified_process_and_capture_closure01.json`. Exact observation times
are in those receipts. A future session must establish fresh ownership, actual
runtime and bounded recovery; **old PIDs, addresses and timers are not authority**.

## Completed increments and evidence class

| Area / task IDs | Latest result | Still not established |
|---|---|---|
| Platform/recovery | Same75 restored:37,237 UART frames/zero retries, full image/independent RAM CRC and directly observed boot | Persistent replacement/other hardware ports |
| Network baseline | Original256-byte radio load passed after measured guarded USB cold reset; APPff and paired75 station/authorized WLAN restored | Native2048 transfer, AP/APSTA, network transactions/durable settings |
| SEC-06/11, WEB-10 guard | **Hardware-tested**7.046s guard-only run: first exec admission/ping, own wait0, inactive5s watchdog/no lineage. Corrected parent recipe passed121 models | Cause of UI02 reset; hard scheduling/combined-load guarantees; full UI03 campaign |
| WEB-03/05 UI | Serialized dummy browser and134 real-TLS HOST checks; matched launcher HOST lifecycle. NativeUI01 strict2MiB READY/natural retirement only | Native assets/login/logout/revocation/browser entry on this UI candidate. UI02 reset failed; UI03 never ran |
| SEC-05/06/11 RTSP | Copied server v3, isolated verifier v2 and finite first-entry pack independently SOURCE/HOST reviewed; ARM links only | Strict native root-origin/READY15/2MiB/retirement, native positive Digest/per-effect media, actual NVR |
| SETUP/SEC storage | Network metadata v2:400 contexts/7510 checks each; private profiles v2:57/1220 each; profile-transition v2:30 scenarios/34 contexts/2036 checks each. SOURCE/HOST only | Physical store, key unseal/cold entropy, rollback-resistant authority and native radio/observer integration |
| CTRL | PTZ v2:11466 checks each; reset input:252584 each. SOURCE/HOST only | Routing/current/displacement/asynchronous physical STOP; native button and deliberate settings reset |
| AUDIO-01–06 | Capture v2:39728 each plus Linux ARM objects; media v2:410499 optimized/410495 sanitized plus independent codec oracles. SOURCE/HOST only | Genuine microphone/8k format/cache/IRQ/retirement, browser/NVR audio and A/V synchronization |
| STORE-08 motion | Motion v2 independently1858785 optimized/1561293 sanitized, C++/ARM link only; final callback authorization and loan timing corrected | Actual cheap complete luma source, native tuning/precision, real-frame masks and recordings |
| Shared regression |30 C +2 Python +76 receipt cases passed before pause | Native/product acceptance |

Parent review receipts live under `evidence/delegated/20261012-parent-review/`.
Do not infer native acceptance from fixture counts, ARM links or source capability.

## Failures and unfinished active checkpoints

- **UI02 watchdog reset preserved.** The controlled HOST pre-exec counterexample
  explains a possible hazard, not the measured reset cause. The guard-only native
  result does not turn the failed campaign into a pass or justify an unchanged retry.
- **Radio mode-selection error preserved.** Parent selected unqualified2048-byte
  `radio native-load`; original source already recorded its first-payload E110/FIFO
  failure. Corrected restoration explicitly used the previously qualified256-byte
  `radio load` after measured source-owned USB cold reset. No automatic fallback.
- **Native handoff v6 remains HOLD.** Raw CPU-to-NAL temporal reuse is a source
  hypothesis; migration-aware memory admission, source/ELF sealing, independent
  review and real13-AU/12-output/END/QUERY/ACK measurements remain required.
  Unknown movable CMA occupants cannot be assumed absent to force admission.
- Media-v6 finalization and isolated-server integration workers sealed their existing
  SOURCE/HOST checkpoints and stopped. Final receipts are listed below; neither
  is parent-reviewed/native-adopted by this pause.
- All original acquisition/source/boot/failure evidence and licences remain local
  and preserved; secret fixture payloads/ciphertext/nonces/keys are excluded from
  public hash inventories. No imported footage/recovery files were touched.

## Remaining work — only after user resumes

1. Re-establish sole UART ownership/actual runtime/fresh bounded recovery. Never
   reuse recorder i or its02:28 timer. Confirm the exact image/source/ABI and guest
   endpoint again before target work.
2. Review final media-v6 and isolated-server checkpoint reports/diffs; preserve
   partial failures. Revalidate memory reserve/migration forecast and storage
   overlay contract before any producer allocation/sensor/codec operation.
3. Run one fresh bounded native UI campaign with the reviewed guard, matched pins,
   private input lifetime/trust checks, immediate client-on-READY and actual owned
   shutdown. UI03 preparation alone is not a campaign result.
4. Qualify strict native RTSP verifier readiness/isolation/retirement, then integrate
   per-effect source authorization and independent credentials. Runtime lab/source
   NOAUTH and media501 labels must remain truthful until actual replacement passes.
5. Qualify native handoff/integrity/timestamps/encoded retirement and protected TLS
   media/browser presentation, then combined-load headroom/FPS. Keep full CRCs/all
   guards,2s/10s budgets, allocation ceilings and frozen reserve; no fit-by-relaxation.
6. MMC read-only provider/routing/shared L2/cache/IRQ retirement and actual owned
   filesystem durability precede recording/config writes. Disabled MMC/refused
   probe does **not** prove card absence/failure. Preserve existing footage/recovery.
7. Complete real recording/motion/retention/playback; durable credentials/settings,
   entropy/trust/UTC/rotation, AP/onboarding/reset; physical PTZ/lights/ambient,
   microphone/audio, discovery/ONVIF and owned Frigate/Blue Iris interoperability.
8. All product gates remain open. T34-4G/Fulhan hardware ports stay deferred until
   genuine QC07F G8. Everything remains local, uncommitted, unpushed and unpublished.

## Final source-worker closure

**Isolated RTSP server stopped at a sealed partial checkpoint.**
`evidence/delegated/20261012-rtsp-isolated-server/HALT-SEAL.json` and `README.md`
separate `integration05` tested source from `frozen-source/`. The tested snapshot
has47 HOST campaigns each/86761 optimized/86425 sanitized checks, C++ and ARM
static links (ARM never executed); these are worker results, **parent review pending**.
Three current fixture edits (`public_dummy_material.c`, `fixture.c`, `client_test.py`)
are **unbuilt/untested**; see `tested05-to-frozen-HALT.diff`. Do not attribute the
tested counts to the current tail or infer native RTSP/authentication acceptance.
Source worker reports no further work; original shared v2/v3 and all failures retained.

**Media-v6 finalization is sealed and stopped.** See
`evidence/delegated/20261012-native-handoff/FINAL-v6-SEAL.json`, `REPORT-v6.json`
and `PAUSED-v6-STATUS.md` (seal time2026-10-13T00:42:33UTC). Worker results:
42 handoff/35 memory/67 adapter/24 TCP cases each optimized/sanitized, plus16
owned-localhost receiver cases. Static ARM ELF91236B was inspected, **never run**.
Raw reuse removes one1028KiB mapping without reducing512KiB overhead/1024KiB reserve.
The dated i forecast still **fails**:5476KiB above W versus11548KiB required for
raw reuse,6072KiB deficit while future CMA migration is unqualified. No known ISP
placement is subtracted without qualified evidence; no allocation to hope for reclaim.
Exact v5/failures and frozen v6 source/build closure preserved. **Parent independent
review/rerun pending only after explicit resume; native status remains HOLD.**

**All already-active units are now finished or frozen at explicit safe checkpoints;
all workers have stopped.** No additional implementation, build/test or target
experiment is authorized during pause. Last target observations remain dated;
there is no current recorder/recovery protection and no automatic continuation.
