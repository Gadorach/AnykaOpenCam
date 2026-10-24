# FIRST packaging-only handback — actual RAM artifact, review required

2026-10-24. **ONE package PASS; zero build invocations/retries; NOT BOOTED.**

Actual image:
`work/lightbulb-camera/driver-foundation-package-20261024/images/QC6DF01.UIM`

- 3,403,371B; SHA256
  `d37615e86699ef5d80b159eb690ec130049c4ee023785e9d6d80fc169f543cee`.
- Whole CRC32/BZIP2 `f8371c93`; header/data IEEE `c61aedd5` / `c09e7c76`.
- Load0x82408000/entry0x82408040; actual held new zImage + unchanged appended DTB;
  release6.18.55-qc07f-driver-foundation01, reviewed built-in OFF-retention PTZ.
- Staged range[0x82408000,0x82746e6b); conservative decompressor end0x8275f878,
  657288B below existing0x82800000 split. Static fit ONLY.

Read CONTRACT, PACKAGE_RESULT and PACKAGE_MANIFEST in this directory. Full fresh
indexed byte/range proof is `work/lightbulb-camera/driver-foundation-package-20261024/PREPACKAGE_PROOF.json`;
76 before/after held-file pins and bounded original graphs are under its inputs/.
Exact consumed recipe/contract/notices copies are retained there too. DELIVERY
records public source/report and local evidence/artifact role pins.

Fresh checks: indexed PTZ identities/actual initcall-driver-sysfs pointers/eight
split-HOLD ranges; all23 allocated kernel sections via PT_LOAD LMA (including
vectors/stubs)→Image, all six compressed sections→zImage, XZ+LE32→Image, embedded
GZIP→exact archive, all48 retained newc records/exact2 removals, exact2 config
differences, global anchors/actual stack/DTB growth/malloc/layout and final CRCs.

REUSED, not fully recomputed: exact-pinned independent ALL95 relocation-target/
seven-section PTZ proof, full source delta and DTB decoding. Review SHA
`592d780b157c9426293f7295d321f5a2bacb728886b4c8600f273d448ee5aeef`.
N2 three FINAL-ONLY Brotli-library deltas remain a provenance limit. No complete
unchanged toolchain/exact reproduction claim; no host/library/settings changes.

Original source/build/recipe/FAILURE/logs/docs/manifest remain unchanged and
original images directory remains EMPTY. Original failed predicate is NOT repaired,
rerun or relabelled. Only the three newly assigned package roots were written.

**Parent next boundary:** independent ordinary package/layout review and parent
disposition/image selection before any use. No author/reviewer/subworker remains
active here; no further lane or executable native queue is selected by this handback.
No entry selection file was created. Image native_eligible=false/boot_admission=false.

RAM-only, exceeds1.5MiB NOR KERNEL: no flash/install/repartition. No target ELF/helper,
tests/compiler/linker/tools/oldrecipe/network/proc/device/UART/RPC/outlet/motor/mic/
radio/audio/hardware/Git/index/AGENTS/central-plan action. Speaker deferred/backend
parked, P1/ADC/provider/recovery/physical STOP/thermal/zeroing/cap/combined-load gates
unchanged. Supervised user policy is not native GO, a current lease or strict C1 proof.
