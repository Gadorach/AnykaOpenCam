# RP-04 — unchanged v6 parent offline reproduction

2026-10-21. **BUILD/HOST reproduction PASS; adoption/native memory HOLD.**

The parent inspected the existing scoped builder, then ran exactly the documented
fresh `runParentV6RP04A` command from `PLACEMENT_ADOPTION_REVIEW.md`.
Shell `sh_15cee9094001hhgp6fp8K2kvTZ` completed successfully05:28:43UTC.
Only synthetic host fixtures/local loopback ran. ARM was built/inspected, never
executed; no target/serial/RPC/device or private input was accessed.

Results against preserved run34:

- 55 original source pins,265 normalized consumed source/header closure entries
  and46 external tool inputs match. Controlled import and memory-footprint records
  are unchanged.
- Stripped ARM bytes are **identical**,91236B/SHA256
  `64434b3c3c7dc9bae28c6ff66bddbc01a4f08a9e6972d0082b0f418036ff2319`.
- Handoff42, memory35, imported adapter67 and TCP24 cases pass in both optimized
  and ASan/UBSan builds;16 owned-loopback receiver cases pass.
- CPU forecast remains2844KiB raw-reuse/3872KiB separate; H512/R1024 and unpaid
  codec8256KiB are unchanged. This is mapped capacity accounting, not measured
  native high-water, placement or product acceptance.

The initial offline comparator incorrectly compared the `tool_inputs` dictionaries
without normalizing their run-local object paths. That assertion failure and its
original script are preserved in `parent-reproduction01/`. The corrected comparison
requires equal external tool pins and normalized object path sets while recording
all differing object hashes explicitly:24 HOST object hashes differ; **no claim
that all generated HOST objects are byte-identical**. The target ELF is byte-identical.
No source or build result was changed to satisfy the comparator.

Reproducible comparator and exact argv, timestamps, versions, manifest/source/log
pins and results: `parent-reproduction01/compare.py` and `result.json`.
Original complete manifests/logs/binaries remain in the two evidence run trees;
generated target binaries are not added to Git.

## Remaining gate

This completes an unchanged-v6 offline reproduction, not full76 adoption. Existing
v6 has no6160KiB fixed-ISP exclusion, early continuous source pin or76 policy
entry. Parent source adoption and any narrowly implemented proof still need review,
fresh current placement/provider/owner receipts, recalculated ELF debt and actual
native retirement before media. The historical hypothetical88KiB margin is not
admission; no reserve, guard, CMA, allocator, codec/input or sensor policy changed.
RP-04/05/combined-memory HOLD remains.
