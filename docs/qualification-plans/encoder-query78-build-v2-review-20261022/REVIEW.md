# Query78 build v2 — focused independent R1/R2 SOURCE re-review

Selection2026-10-22; actual review host2026-10-23. ENC-02 / PLAT-01/02/05/08,
QC07F first. **CORRECTION REQUIRED: remaining R1-A final-inventory namespace
association defect.** R2's changed primary-error handling is narrowly source-
sufficient, but the complete v2 method is not accepted or admitted for invocation.
No candidate/helper/preflight/compiler/test/code object/program was run; no new
archive/kernel/module/image was generated, extracted or transformed.

## Exact packet and review scope

| Identity | SHA256 |
| --- | --- |
| V2 six-file source aggregate | `e8708daaba932d1b633230917780a5c1dec58bb0a3ba7d953fa2c8d90a3bdd7d` |
| V2 HANDOFF_SEAL01 | `55edc62980d272fbe4deaf914eca2c62ed82e0767b9f9d14936001ed3ed0ec06` |
| V2 68-input manifest | `858d7089aacc4221df100bd1d764c9bb4b08116285d495d3973cc14bd91828c0` |
| Frozen v1 aggregate | `d3ac858dafa835587474bf7708ef0c5f5a17820cccbe42f5cb4619a52e29a373` |
| Frozen negative v1 review seal | `3b293d00fa1ea7bdb41cfce9ff0b12e1850c92b02c83ee7b795e97f49a0a5685` |

Read all actual v2 units (build408/common342/newc164/package308 lines), README,
contract/report, changed caller paths, all direct diffs and receipt schemas. Read
the v1 findings/disposition and fixed parent contract; compare with actual frozen
v1 source, not only author assertions. This is ordinary source/API/error/resource/
publication reasoning, not a new archive/math/framework audit or runtime campaign.

## R1-A — admitted dependency spellings are not inventory coordinates

**Correction-required anchors, all under
`apps/qualification/encoder-query78-build-v2/`:**

- `build.py:276–283,336–339`: closure validates `dependency.resolve()` against the
  permitted roots but stores/captures the original `str(dependency)` spelling.
- `common.py:124–150`: the bound row retains both that original map key and its
  separately resolved endpoint identity. Keeping the original spelling is valid.
- `build.py:352–356`: finalization uses the original lexical path relative to ART/
  BUILD/IMAGES directly as a key in a directory-walk inventory.
- `common.py:237–260`: those inventories use walked file names; they do not contain
  equivalent `..`-bearing aliases as additional keys.

This is not a hypothetical malicious input. The retained ordinary query77 build
already records the following dependency, with `K77` denoting its pinned source
root:

```text
K77/arch/arm/boot/compressed/../../../../lib/../scripts/dtc/libfdt/fdt.c
```

Evidence is the existing public
`work/lightbulb-camera/foundation-20261011T041148Z/analysis/encoder-query77-build02/build/arch/arm/boot/compressed/.fdt.o.cmd:100`
and its saved consumed-source map. The recorded resolved path is
`K77/scripts/dtc/libfdt/fdt.c`; its saved size/SHA match the canonical source-
manifest entry. The raw `arch/arm/.../../...` spelling is **not** a manifest key.
Existing source inclusion text explains that spelling; it is an admitted normal
dependency, not an escaped-root or unsupported input. The saved consumed map has
86 parent-component alias paths among3903 entries; only metadata association and
the concrete example were checked, not all underlying bodies.

For the same ordinary spelling under the future SRC, closure accepts/captures it.
At finish, the source inventory key is `linux/scripts/dtc/libfdt/fdt.c`, whereas
the lookup uses `linux/arch/arm/boot/compressed/../../../../lib/../scripts/dtc/libfdt/fdt.c`.
That lookup is absent and raises KeyError. The new final comparison can therefore
reject a legitimate unchanged-source build after packaging rather than complete.
No query78 build/failure was observed or reproduced; this conclusion comes from
actual retained namespace evidence and the changed source control flow.

`ALIAS_FINDING01.json` pins the example, saved resolution/body association, source
manifest coordinates and code anchors. No fake filesystem, synthetic archive,
candidate inventory/closure call or mutation test was used.

### Minimum fresh correction

Give each captured lexical endpoint a separately verified inventory coordinate/
root association, then compare the actual final inventory value at that coordinate
while continuing to recheck the original lexical endpoint's full frozen identity.
Account correctly for regular aliases versus direct symlink rows: resolving a
target must not silently turn a claimed symlink observation into a regular-file
observation. Confined resolution, body/mode/link identity and conflicting overlaps
must still be checked. Keep every real consumed alias represented in the seal.

Do not drop those inputs, adopt new hashes, treat ordinary aliases as unsupported,
blindly normalize escaping paths, or waive the final comparison. This is a bounded
R1 namespace-association repair, not a new kernel/archive/tool/config policy or
framework rewrite. Preserve v2/review unchanged; any revision needs fresh selection
and focused review, not an automatic retry or build.

## R1 corrections that are otherwise present

| Boundary | Actual source finding |
| --- | --- |
| Immutable capture, build:58–81; common:124–165 | Earlier checked bodies/roles are compared at capture, full endpoint rows retained in immutable JSON strings, repeat labels/conflicting overlaps refused. Rows bind bytes/SHA/resolved/lstat mode/dev/inode/direct link/target mode, not mtime or full-tree inode custody. |
| Prepared/precompile/built, build:162–170,212–218,251–262 | Prepared archive/inventory receipt/copied DTB/client frozen; configure-generated executable-mode files honestly final-observed; BUILT body independently serialized, output and receipt pins bound. |
| Closure, build:264–340 | Actual .cmd/consumed/loaded/generated executable/log/JSON/trace/receipt pins and sets plus optional absence/resolution captured. Shared fields cannot silently conflict. Recheck only reads; it adds no new command/loader/executable files itself. Original lexical aliases remain valid until the erroneous final lookup. |
| Package, package:206–214,277–308 | Original BUILT body equality, already checked in-memory ELF/Image/zImage/DTB/piggy hashes and compression/archive observations tied to frozen body pins. Full callback recheck at entry/immediately before write. Expected image size/SHA computed before exclusive write and compared to actual file; PACKAGE body/image returned as full checked rows and rebound by main. |
| Final inventories and seal, build:342–368 | Inventories precede terminal; known values must compare with frozen rows, extras are observations. Rechecks bracket inventory and exclusive completed-seal write/body check. Namespace mismatch above remains a blocker, not evidence that this path executed successfully. |
| Terminal, build:369–378; common:95–117,168–176 | Completed seal and checked BINDING receipt identities enter complete terminal JSON; exclusive unbuffered pending write, flush/fsync/one close/body comparison precede no-overwrite hardlink publication. Pending stays. Normal success has no later authored I/O/print; main simply returns after finish. |

At a successful fresh invocation, final inventories are evaluated before the seal,
pending and terminal exist; those future names are not recursively included. The
binding strings do not include the final seal/terminal, so no hash cycle is added.
The finite snapshot/set loops add no new members while iterating. Other than R1-A,
no additional ordinary self-inventory/set-growth contradiction was identified.

These are endpoint checks on a trusted serialized host/filesystem, not root/same-UID
concurrent-mutation immunity, atomic all-child-read prevention or power-loss/
directory-fsync durability. No guarantee about arbitrary asynchronous exceptions
between a namespace syscall's effect and Python's return is inferred. Consumers
must require complete terminal JSON and its exact seal/binding associations, never
PACKAGE/BUILT/BINDING/pending alone. No terminal has been produced by this review.

## R2 focused source disposition

**Narrow source sufficiency for the selected changed error paths; no further R2
correction identified here.**

- `build.py:108–125`: log open/subprocess/timeout/nonzero BaseException object and
  traceback are captured before bookkeeping. Nonzero becomes primary before close
  or exit metadata. Each close/exit/timestamp/pin/receipt operation uses recording;
  with no earlier error its failure prevents success. With an earlier error it
  does not replace that primary. No second subprocess/write-retry loop exists.
- `common.py:71–92`: recording retains the first object/saved traceback. Secondary
  note label/type formatting is bounded and its own failure guarded. Reraise uses
  the same object with the saved traceback; normal rethrow frames can be added,
  not an assertion that no Python frames exist around the original traceback tail.
- `common.py:48–64,95–121`: explicit pin-read and exclusive-write helpers retain
  read/write/flush/fsync errors through a one-shot close. Serialization precedes
  creation; one unbuffered write, short-write refusal, no unknown buffered resend.
- `build.py:396–404`: outer timestamp/notes/property/serialization/write work is
  inside the recording action. At most16 string notes/256 characters each are
  selected; original error repr/args are not formatted into the receipt. Secondary
  failure leaves the primary for propagation; durable evidence may be impossible.

The claim covers these authored changed paths and ordinary host-library exception
semantics, not every inherited pathlib/context-manager/library operation, arbitrary
exception overrides or all Python asynchronous/resource intrinsics. No error path
was executed. V1's negative review remains negative and unchanged.

## Protected logic, custody and limits

Independently regenerated all six v1→v2 diff files, including empty newc/.gitignore
diffs. newc is fully byte-identical, SHA
`06e796f5e31b845988ce980d54bc601544fbd1e069cf4035eb52d4a3344da18a`.
Independent AST equality confirms all15 protected common functions, both stable/
configure methods, env/make expressions,20 fixed path/limit expressions, eight
package helpers and every occurrence of the selected layout/header/payload/math
assignments. CALLSITES01's calls/imports/class/function spans match actual source.
No candidate function or code object was evaluated by these comparisons.

Consequently the necessary DOC/input-list hash changes do not rebase source01/
build01/images01, closed run01/client-review argument semantics, Clang/LLD23.1.3,
two allowed config changes, strict baseline verification, archive one-remove/one-
add/PID1 preservation, compression/CRC/layout math or4MiB/40MiB/CMA/128KiB limits.
No first recipe-owned write/tool was moved ahead of closed input/source/client
review admission. Normal interpreter/stdlib import startup I/O is not claimed zero.

All68 selected pins match BEFORE/AFTER/current; old42 pins remain an exact subset.
Six v2 source/17 author document pins and both source/handoff seals match. Fixed
client review83f1b9ec… and parent contractf29ea0e4… were freshly hashed. Those facts
are not native readiness or invocation permission. Full91270-source/50k-tool body
rehash, current tool preflight and any future output-root state query were omitted,
not called passed. Strict future preflight may still refuse host drift/ordinary
output differences; do not bypass a refusal.

INPUTS01 has102 path-associated reader inputs. POSTCHECK01 freshly matches all102,
with no context drift/rebase. The added historical dependency-map/.cmd/source reads
are explicit. Own COLLECTION_ERROR01.md retains
one receipt-schema assumption error: v2 uses definitions including ClassDef, not
v1's functions field. Corrected independent collection succeeded; it was not a
candidate/runtime failure or policy refusal. Author EDIT_ERROR01/COLLECTION_ERROR02
and previous reviews remain frozen. Final review output pins are in FINAL_SEAL.

GPL notices/provenance remain; kernel/tools/libc/retained payload licensing is
separate, no global publication/redistribution grant. Safe metadata only here;
generated binaries stay excluded. No Git/index/source/old-review/central-plan/
AGENTS/provider/model/safeguard modification, private capture/analysis/HOLD/CONTROL/
current-state/proc/dev/clock/process/network/device or hardware action occurred.

## Handback

Parent should read/reverify this review and retain the sealed v2 packet. A fresh
bounded R1-A namespace repair, if separately selected, is the minimum required
revision; preserve the other R1/R2 changes and guards. No source/build-method GO,
invocation/archive/kernel/image integration, native/physical/manual-readiness
request or automatic next queue. Historical U-Boot/both FAILED/HOLDs remain only
history. Bootstrap/FD/global-idle/observer/clock/catch/recovery/backing/provisioning/
main/combined-memory/full PLAT-02 and G2/G3/G8 remain unproved.
