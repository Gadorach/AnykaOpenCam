# Start here — AnykaOpenCam

## Public repository — 2026-11-14

See [README.md](README.md) for the current public source/firmware archive,
[original dump provenance](firmware/README.md) and [per-device extraction tools](firmware/tools/README.md).
The owner explicitly requested the three unmodified stock dumps, including known
embedded shared factory-key material. Generated current native images and private
bench material are excluded. Old runtime/publication statements below are dated
history, not present hardware state or the latest publication authorization.

## Explicitly resumed2026-10-13

**Latest:** user authorized stock reference tests after review. Actual controlled
stock3.4.35 root rescue now running; see [module results](docs/qualification-plans/stock-reference-20261013/ACTUAL_RESULTS.md).
Original stockMTU1500/2048firmwareload and finite network integrity observed, card/
config read-only; native GETs and stored settings recorded. No app/capture/motor
movement; lightGPIO readback alone produced no user-visiblechange. Next is a
controlled feed comparison for IR, after safe source/entry audit. Historical
held-U-Boot and prior75 runtime statements below are superseded.

User powered on; **U-Boot caught and held**, no kernel handoff. The requested
[project review and completion strategy](docs/plan/camera-foundation/REVIEW_AND_COMPLETION_STRATEGY_20261013.md)
is the current planning entry. See [resume status](docs/plan/camera-foundation/RESUME_20261013.md)
for evidence. The one-shot catch is consumed; fresh recovery is required before
later target work. Old pause/runtime below are history, not inherited authority.

## Work paused at the user's request

See [current paused status and remaining work](docs/plan/camera-foundation/PAUSED_STATUS.md).
Do not automatically continue the old queue. Last observed QC07F is restored75 RAM
Linux/authorized WLAN384, ISP idle/UART clean/watchdog inactive. Recorder and public
stage are stopped; **no current UART owner or armed recovery**. UI03 never staged/
launched on target. All product gates remain open; older checkpoints below are history.

## Current checkpoint — 2026-10-12

Build the common product on **QC07F first**. The T34-4G and Fulhan hardware ports
remain deferred until the shared foundation passes **G8**. All three stock ROM
acquisitions already exist; the initial off-board-flash task is historical.

Kernel75 RAM boot, strict credential-child READY flags15 and the public-dummy
full600k backend match/mismatch/issuance/revocation/retirement passed on QC07F.
Native HTTPS **functional and owned lifecycle campaign04 passed** after narrow
musl stdio/writev and ARM ordinary signal-return filter fixes: full600k login,
protected status/capabilities, CSRF, logout/replay, discard/drain, owner revocation
and clean SIGTERM/child/service/bootstrap retirement. The original failed startup,
orchestration and campaign03 shutdown remain preserved. This bounded increment is
not complete product acceptance or combined-media memory qualification.

**Last observed runtime: kernel75 RAM Linux, associated authorized guest WLAN,
MTU384, capture idle, watchdog inactive, UART clean; no running auth/RTSP/viewer.**
Fresh recordere ownership/recovery was established before campaign04 and remains
a dated observation, not an inherited PID/timer. Revalidate before target use.

## Navigation

1. Read [AGENTS.md](AGENTS.md) and the canonical
   [plan index](docs/plan/camera-foundation/00-INDEX.md).
2. Read [baseline/execution](docs/plan/camera-foundation/01-BASELINE-AND-EXECUTION.md),
   [security](docs/plan/camera-foundation/09-SECURITY-AND-CONFIGURATION.md) and
   [web](docs/plan/camera-foundation/10-WEB-UI-AND-BROWSER-MEDIA.md).
3. Review the latest sections of
   `work/lightbulb-camera/foundation-20261011T041148Z/CONTINUATION_AUTH_KERNEL75_ACTIVE.md`
   and its linked exact source, build/config/module/image and failed-attempt evidence.
   Filenames and older “active” PIDs are not current ownership proof.
4. Use [project layout](docs/PROJECT_LAYOUT.md), [shared framework](framework/README.md),
   `bringup/qc07f/` and promoted [apps/auth-web/](apps/auth-web/README.md) source/provenance for implementation.
   [Pinned reference subsets](references/SOURCE_GUIDE.md) are not a full kernel tree.
5. Consult [product acceptance](docs/CAMERA_PRODUCT_ACCEPTANCE.md); retain
   source/build, fixture-passed, target-passed and product-accepted distinctions.

## One next executable task

The user's independent-lane/delegation request is active; see
[parallel work and parent review](docs/plan/camera-foundation/PARALLEL_WORK_20261012.md).
Five isolated assignments returned; parent corrected/reran the three code lanes
and reviewed the two evidence/dependency lanes. Shared28 C +2 Python +76 receipt
cases pass; Digest566 optimized+566 sanitized checks pass separately. Native
recording/reconnect/RTSP-server integration is not enabled or qualified.

Shared C init/SPS/profile/cadence and borrowed fragment/chunk paths now pass
**26 C +2 Python +76 receipt** tests; C-repacked retained footage decodes in MSE
and pixel-identically with FFmpeg. The browser reported one presentation drop,
not zero-drop proof. A model reuses existing producer/hub storage without waiting
on a producer-held output slot. Continue **bounded native handoff integration/
measurement**, then protected TLS/browser integration; model tests do not prove
actual DMA/driver/TLS/headroom. No guessed FPS/full-AU TLS copy/raised limits.
Media stays501; independent RTSP auth remains required.

13:01UTC metadata found the adapter/socket present but no serial owner; olde
protection expired and final_e manifest was absent. Preserve/investigate the
unexplained socket/logs before fresh ownership/runtime/recovery. The dated05:09
Linux/WLAN observation above is not a current connection assertion.

Later parent review preserved the stale socket and established fresh sole f
receiver/recovery hold18:03:42UTC; read-only actual6.18.55 RAM shell/idle ISP/clean
UART/inactive watchdog and WLAN192.168.1.73 MTU384 were revalidated. This supersedes
the no-owner boundary only for that dated observation, not future admission or
exact image/source proof. Original e logs remain untouched/unfinalized.
Preserve failures and exact input hashes. Do not weaken isolation/workfactor,
or add another transport/guardian/publisher infrastructure phase.

Before any later target operation, independently establish the exact sole UART
owner/adapter, no competing upload, actual runtime and fresh bounded recovery.
Use the current user's authorization without repetitive approval or invented
permissions. Persistent writes still need an exact image, affected ranges, boot
policy and recovery method; RAM/build success is not flashing authorization.

## Historical material and privacy

The previous workspace name is a compatibility symlink preserving frozen absolute
paths. Original handoff documents are byte-exact in `docs/archive/initial-handoff/`.
Legacy root `FIRST_PROMPT.txt`, `PROJECT_STATE.json`, `VALIDATION.md`, `MANIFEST.json`,
`SHA256SUMS` and `NOTICE.md` are preserved history, not today's task/status or a
whole-workspace checksum set. The roadmap import and document hashes are recorded
in `docs/archive/migration-docs-20261012.json`.

Keep complete acquisition mirrors, photos and private bench provisioning local.
The explicit2026-11-14 exception covers only the three original dump copies under
`firmware/original/`, with their embedded factory material and warnings. Other
private credentials/keys/digests remain excluded. Preserve source notices and originals.
