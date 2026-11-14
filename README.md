# AnykaOpenCam

## Public source and original firmware archive

Repository: <https://github.com/Gadorach/AnykaOpenCam>

**Experimental and incomplete. No working built image of the project's current
native firmware is distributed here.** Source/build reviews are not proof that
hardware drivers or the complete product are ready.

The three `.bin` files below are **unmodified original manufacturer-firmware
acquisitions**, not images built from this project's source. Each is 8,388,608 bytes
and matches three independent reads retained locally. They predate later bench
modifications; do not assume they match the firmware currently installed on a unit.

| Camera | Original dump | Extraction tool |
| --- | --- | --- |
| T34-4G / Anyka KM01, AliOS Things | [`t34-4g-km01.bin`](firmware/original/t34-4g-km01.bin) | [`extract_t34_4g_km01.py`](firmware/tools/extract_t34_4g_km01.py) |
| QC07F / Anyka V200, Linux 3.4.35 | [`qc07f.bin`](firmware/original/qc07f.bin) | [`extract_qc07f.py`](firmware/tools/extract_qc07f.py) |
| Fulhan FH8626V200, Linux 4.9.129 | [`fulhan-fh8626v200.bin`](firmware/original/fulhan-fh8626v200.bin) | [`extract_fulhan_fh8626v200.py`](firmware/tools/extract_fulhan_fh8626v200.py) |

**Security warning:** these dumps are intentionally not sanitized. The owner
explicitly approved public originals after being informed of a shared factory RSA
private key in the KM01 and QC07F firmware. Original firmware may also contain
factory credentials, device identifiers and cloud configuration. Do not reuse its
keys/credentials or treat it as a secure firmware baseline. No extracted key files,
private bench provisioning, newer credential-bearing modified dumps, photographs
or footage are separately included.

From the repository root:

```sh
sha256sum -c firmware/original/SHA256SUMS
python3 -B firmware/tools/extract_t34_4g_km01.py
python3 -B firmware/tools/extract_qc07f.py
python3 -B firmware/tools/extract_fulhan_fh8626v200.py
```

See [firmware provenance and warnings](firmware/README.md) and the
[extraction guide](firmware/tools/README.md) for host dependencies, partition maps
and limits. Outputs go to ignored `firmware/extracted/`; existing destinations are
refused. JFFS2 is carved only, not reconstructed. No firmware executable is run and
these are not installation/flashing commands.

## Development status

QC07F is first. Current priorities are stable Wi-Fi/USB and MTU1500, bounded motor
movement/STOP and defensible zeroing, then modern microphone capture. Speaker tests
and further backend expansion are deferred. Modern MTU1500, mechanical/homing and
modern audio readiness are **not accepted**. The last recorded supervised boot
attempt stopped in U-Boot on a console-format guard and ended OFF; it did not boot
the new driver image. No current hardware state is inferred from old records.

Follow the [active ledger](docs/plan/camera-foundation/EXECUTION_TASKS_20261021.md)
and [gated plan](docs/plan/camera-foundation/00-INDEX.md). Full kernel/SDK/toolchain
and private build/evidence trees are not bundled; historical recipes may reference
those local paths. The stock extraction tools are self-contained subject to their
listed host dependencies. Upstream copyright/license notices remain applicable;
publishing a dump does not relicense vendor software. See [NOTICE.md](NOTICE.md).

## Project scope

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

All three stock acquisitions have three matching 8-MiB main-array reads. Public
copies and provenance are under `firmware/original/`. Whole-acquisition mirrors,
all nine reads (72 MiB), private configuration and photographs remain local under
`firmware/stock/`, `work/` and private evidence storage. Matching hashes establish
repeatability, not electrical completeness or permission to publish other data.

## Historical checkpoint — 2026-10-12

The observations and next-step wording in this section are dated history, not
current runtime authority or the latest work selection.

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

The original migration checkpoint was local and unpublished. The owner's explicit
2026-11-14 publication approval now covers the selected source and three original
dumps, including their embedded factory material, not unrelated private evidence
or any firmware installation. [NOTICE.md](NOTICE.md) remains the original handoff
notice, not a new global license or redistribution grant.
