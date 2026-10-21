# AnykaOpenCam

## Current execution — 2026-10-21

The [detailed gated plan](docs/plan/camera-foundation/REMAINING_FEATURES_AND_BUGS_GATED_PLAN_20261021.md)
is approved for execution; follow the [active ledger](docs/plan/camera-foundation/EXECUTION_TASKS_20261021.md).
Candidate76 has a historical narrow RAM boot/idle PASS, not full-size USB, protected
media or product acceptance. Current hardware admission must be freshly established.
QC07F remains first; other ports wait for G8.

Reviewed source increments are committed to main. No remote is configured; the user
selected local commits until a destination is supplied. See [Git workflow](GIT_WORKFLOW.md).
Private acquisitions, images/footage, provisioning and hardware-specific local
selectors are deliberately not part of the source baseline. Existing local staged
work is preserved; this snapshot is not a complete release or permission to publish
historical evidence. Original notices/licenses remain with their source components.

The older checkpoints below are retained history, not current runtime authority.

A shared, local camera software product for three owned camera systems: upstream-based Linux, networking, real hardware video from every fitted sensor, authenticated RTSP/browser viewing, PTZ and fitted peripherals, recording/playback, configuration, setup/reset and NVR integration.

**Implementation is incomplete. QC07F is the primary development target.** Complete and qualify its shared foundation before the T34-4G and Fulhan hardware ports (**G8** prerequisite). Linux 6.x is permitted; the pinned bring-up baseline is **Linux 6.18.55 LTS**, not a mainline-merged platform port. Binary reverse engineering uses **REA**.

Start with [START_HERE.md](START_HERE.md), then the canonical [implementation plan](docs/plan/camera-foundation/00-INDEX.md). See [project layout](docs/PROJECT_LAYOUT.md) for source, stock ROMs and preserved evidence.

## Targets

| Exterior label | Stable inventory ID | Fitted system / development order |
| --- | --- | --- |
| T34-4G | `km01-lte` | Anyka KM01, two reported sensors, LTE/PTZ; deferred until G8 |
| QC07F | `lightbulb-camera` | AK3918EN080-class bulb camera, one sensor, Wi-Fi/PTZ; **first** |
| CFMTA-635727-BJYVX | `camera-03` | Fulhan FH8626V200, two reported sensors, Wi-Fi/PoE/PTZ; deferred port |

Inventory numbers and IDs do not change with development order. Firmware names are not exterior labels. [Target records](targets/README.md) retain identity/history; original acquisition evidence is authoritative where an older status field lags.

All three stock acquisitions have three matching 8-MiB main-array reads. Whole-acquisition mirrors live under `firmware/stock/{km01-lte,lightbulb-camera,camera-03}/`; the original acquisitions remain in `work/`. Nine raw reads total **72 MiB**. Repeatable hashes do not alone establish electrical completeness. ROMs and photos may contain device identifiers, credentials or private configuration: **local Git is not permission to publish them**.

## Latest checkpoint — 2026-10-12

- Kernel75 was directly RAM-booted on QC07F. Strict credential-child READY flags15 and the public-dummy full600k match/mismatch/issuance/revocation/retirement backend campaign passed. These are narrow target proofs, **not native HTTPS authentication acceptance**.
- Native HTTPS authentication **and owned lifecycle campaign04 passed**: full600k login, protected API, CSRF/logout/replay, discard/drain, owner revocation and clean SIGTERM/service/child/bootstrap retirement. Narrow musl stdio and ARM signal-return ABI repairs are supported by old-fail/new-pass regressions; all original failures are preserved.
- **Last observed target: kernel75 RAM Linux/WLAN, MTU384, capture idle, watchdog inactive and UART clean.** No authservice/RTSP/viewer remains running. Revalidate exact ownership, runtime and recovery before any target operation; historical PID/timer claims are not authority.
- Earlier kernel74 raw-receipt expiry/resume passed 16 full-CRC frames. Kernel73 finite network H.264 evidence includes 24 frames at approximately2.2fps. Neither is the current runtime or sustained15fps/product acceptance.
- Protected media, independent RTSP authentication, durable onboarding/secrets, cold entropy/trust, hard-fault supervision, combined-load qualification and all product gates remain open. No other-board port or overall completion is claimed.

Retained hardware H.264 now decodes in desktop browser MSE at720p with actual RTP cadence. The shared112B fragment-header/borrowed-span primitive matches those bytes;22 C +2 Python +76 receipt fixtures pass. This is not live protected camera video.

Shared C init/profile/cadence,8192B spans and CPU lease/admission models now pass26 C +2 Python +76 receipt cases. C-generated retained footage decodes correctly; one browser-reported presentation drop prevents zero-drop qualification.

**Next executable work:** bounded native producer-handoff integration/measurement, then protected media/independent RTSP auth. Fresh bench ownership/runtime/recovery is required; the old recorder protection expired. Keep existing ceilings and actual timestamps, with no new transport/guardian phase.

## Code, evidence and reuse

`framework/` contains shared contracts/services and fixtures; `bringup/qc07f/` contains platform and userland bring-up. [apps/auth-web/](apps/auth-web/README.md) holds the promoted authentication/web candidates with their source provenance. [The source guide](references/SOURCE_GUIDE.md) describes the original pinned **subsets**, not a complete buildable kernel. Exact configs, modules, image/source hashes and RAM-boot evidence remain in the continuation records under `work/lightbulb-camera/`.

[Product acceptance](docs/CAMERA_PRODUCT_ACCEPTANCE.md) and the canonical plan distinguish source/build, host-fixture, target-tested and accepted states. No guessed GPIO, speaker, ambient sensor, IR-cut mechanism or dual-sensor concurrency is promoted to a capability.

The pre-migration root handoff is preserved byte-exact in [docs/archive/initial-handoff/](docs/archive/initial-handoff/). Legacy root `FIRST_PROMPT.txt`, `PROJECT_STATE.json`, `MANIFEST.json`, `SHA256SUMS`, `VALIDATION.md` and `NOTICE.md` remain historical material, not today's entry point or whole-workspace verification. Separate migration/session manifests cover new artifacts.

This workspace is local, uncommitted and unpublished at the migration checkpoint. Keys, credentials and private provisioning are excluded from source/public manifests. Upstream files retain their own copyright and licenses; [NOTICE.md](NOTICE.md) is the original handoff notice, **not a new global license or redistribution grant**. No automatic ROM/photo publication, third-party access or persistent firmware installation is authorized by this README.
