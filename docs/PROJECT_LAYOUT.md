# AnykaOpenCam workspace layout

Canonical root: `/home/gadorach/Desktop/AnykaOpenCam`. This is the existing camera
workspace reorganized for a shared product, not a replacement of its evidence.
QC07F is primary; T34-4G and Fulhan hardware ports wait for foundation **G8**.
Product implementation remains incomplete. Public source and three explicitly
authorized original stock-dump copies are selected for GitHub publication;
generated native images, full acquisitions and private bench data remain local.

## Active code and navigation

| Path | Role |
| --- | --- |
| `README.md`, `START_HERE.md`, `AGENTS.md` | Product overview, current continuation and operating/evidence rules |
| `docs/plan/camera-foundation/00-INDEX.md` | Canonical next-task roadmap, task IDs and acceptance gates |
| `docs/plan/camera-foundation/01-*.md` through `18-*.md` | Imported subsystem plans; original content and relative index links preserved |
| `framework/include/`, `framework/src/`, `framework/tests/` | Shared backend contracts, service primitives and host fixtures; root placement unchanged |
| `bringup/qc07f/` | Platform/kernel overlay, codec and userland bring-up; root placement unchanged |
| `apps/auth-web/` | Promoted authentication/web service and native-launch candidates copied with exact source provenance; not a claim of accepted native auth |
| `scripts/`, `tools/`, `templates/` | Acquisition, analysis/build/test helpers and report templates; unchanged placement |
| `references/` | Original pinned source subsets, license notices and source acquisition metadata, **not a complete kernel/SDK tree** |
| `firmware/original/`, `firmware/tools/` | Three byte-exact public stock `.bin` copies/checksums and self-contained per-device offline extractors; no current native builds |
| `targets/` | Stable identities and historical per-target state; reconcile stale status fields with original evidence |
| `docs/` | Design, qualification and historical analysis documents; read dates/evidence scope, not just filenames |

Linux6.18.55 LTS is the current pinned bring-up baseline. An upstream-based local
platform port is not upstream/mainline acceptance. Exact full-tree/build/config/
symvers/module/image pins and reproducible commands live with the applicable
continuation/source manifests; do not build the original reference subsets as a
standalone kernel or substitute another image with matching vermagic.

## Stock firmware: complete acquisition mirrors

| Mirror | Stable target | Original acquisition location |
| --- | --- | --- |
| `firmware/stock/km01-lte/` | T34-4G / `km01-lte` | `work/session-20261008T001154Z/` |
| `firmware/stock/lightbulb-camera/` | QC07F / `lightbulb-camera` | Acquisition records under `work/lightbulb-camera/` |
| `firmware/stock/camera-03/` | Fulhan / `camera-03` | Acquisition records under `work/fulhan-dual-camera/` |

These are **whole-acquisition mirrors**, not a renamed single “golden ROM”: raw
reads, logs, analysis/extraction and acquisition manifests retain their context.
There are three matching original8-MiB raw reads per target, nine reads totaling
72MiB. Original locations remain intact and usable. Import manifests establish
copy identity; they do not replace original reader/electrical provenance, certify
completeness from hashes alone or authorize installing any image.

The Fulhan authorized environment-only boot-access modification and its verified
readback remain distinct from the original stock acquisitions. No migration is a
new firmware write. Exterior labels, firmware model names and stable IDs remain
separate; development order does not renumber cameras.

## Evidence and preserved local history

- `evidence/photos/`, `evidence/PHOTO_INDEX.md` and `evidence/photos.json` retain
  original user photographs and acquisition metadata. Do not recompress originals
  or rewrite their timestamps to make them appear newly acquired.
- `work/` remains the entire bulky, local historical workspace: original raw
  acquisitions, REA configuration/projects, experiments, logs/UART captures,
  images/decodes, dependencies, toolchains and builds. It is untracked historical
  storage, **not disposable scratch merely because Git ignores it**.
- Durable migration/import/source indexes and manifests record locations, hashes,
  timestamps and selected promoted code without rewriting frozen artifacts.
  Consult those indexes plus the original session manifests for provenance.
- `docs/archive/initial-handoff/` contains byte-exact pre-edit copies of root
  `README.md`, `AGENTS.md`, `START_HERE.md`, `FIRST_PROMPT.txt`, `PROJECT_STATE.json`,
  `VALIDATION.md`, `MANIFEST.json`, `SHA256SUMS` and `NOTICE.md`. They capture the
  pre-migration handoff, including any later edits already present at that time;
  they are not a reconstructed pristine2026-10-07 package.
- `docs/archive/migration-docs-20261012.json` records before/after documentation
  hashes, original paths/timestamps and the byte-exact19-file roadmap import.
  Only the new canonical index's workspace/entrypoint/current-next-task text is
  changed; files01–18 retain historical content. External originals are retained;
  updating an external index pointer is a separate parent-owned action.

Legacy root state, prompt, notice and original package checksum files remain
unchanged. Their off-board/no-runtime assertions describe historical phases;
their checksum set is not a current whole-workspace integrity check. Preserve
original source notices; the handoff notice does not assign a global license.

## Compatibility and future relocation

`/home/gadorach/Desktop/anyka-km01-agent-handoff` is a compatibility symlink to the
canonical root. Keeping `framework/`, `bringup/`, scripts and original `work/`
positions unchanged preserves frozen absolute evidence/build recipes through
that alias. Code using `Path.resolve()` may report the new canonical root.
Future path migration must be audited and regression-tested **offline** before
target use; no guarantee is made for arbitrary cache/toolchain/build-tree
relocation. Do not mass-rewrite captured logs, source pins or acquisition paths.

## State and confidentiality boundaries

Last kernel75 boot/strict READY/public600k proof is historical. The latest native
HTTPS attempt failed required markers; a qualified watchdog reset and held
U-Boot followed. Last observed state is not Linux or WLAN. The recorder stopped
normally with verified final hashes; fresh exact UART ownership, actual runtime
and recovery protection are required before new target operations.

Stock ROMs, photographs, device identifiers and extracts can be sensitive.
**Local Git alone does not authorize publication.** On2026-11-14 the owner explicitly
approved the three original public dump copies, including the disclosed shared
KM01/QC07F factory RSA private key. See `firmware/README.md`; originals are intentionally
not sanitized. Private bench keys/credentials/provisioning and extracts stay local.
Apply each upstream license/copyright independently. No inferred
speaker, unknown GPIO routing or unmeasured capability becomes product truth.
