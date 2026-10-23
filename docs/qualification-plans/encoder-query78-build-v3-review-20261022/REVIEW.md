# Query78 build v3 — focused independent R1-A SOURCE/build-method review

Selection2026-10-22; actual review host2026-10-23. ENC-02 / PLAT-01/02/05/08,
QC07F first. **Narrow conditional v3 R1-A plus preserved R1/R2 SOURCE/build-method
sufficiency. No additional correction-required ordinary defect identified in
this focused scope; no minimum corrective source revision is requested.**

This is not source adoption, invocation/build admission, a successful namespace
operation, kernel/image result, runtime/native qualification or an automatic queue.
No candidate/identity/coordinate/inventory/helper/recipe/preflight/tool/fixture/
test/program/ELF/code object was imported/called/executed. No synthetic filesystem,
archive case, extraction/transformation/generation or kernel/module/image build.

## Exact reviewed source and evidence

| Item | SHA256 |
| --- | --- |
| V3 six-file source aggregate | `7a6207e669ca8480f0fa69020c3b91a585152dc14895c90e6adeafaa4b1ec838` |
| V3 HANDOFF_SEAL01.json | `8b4613f29432ec33cb35a14c07fce27ba368308efba17ccfeb224a7254e31aaa` |
| V3 104-file input manifest | `57cad55929d9360391f5f36c281155984be4f24634975918122b03b80ec39bac` |
| Frozen v2 source aggregate | `e8708daaba932d1b633230917780a5c1dec58bb0a3ba7d953fa2c8d90a3bdd7d` |
| Frozen negative v2 review seal | `88a1edc2c5eb6ef3f92b5633d84a73c0e685732227bb1164e69b349355746c56` |

Aggregate recipe: sorted workspace-relative UTF-8 path + NUL + raw file SHA256
digest bytes. All original/negative packets and reviews remain frozen/unaccepted
at their historical scopes; this fresh source disposition does not rewrite them.

Read the complete changed common.py394/build.py406, contract/report/README and
direct diffs, the preserved package caller and source/callsite/inspection receipts.
Compare actual v2/v3 text/AST, not only author claims. Unchanged newc/package bodies
were read/hash-compared as data; no broad unchanged archive/math/library re-audit
or adversarial/fuzz/mutation/runtime campaign is claimed.

## R1-A source findings

References are under `apps/qualification/encoder-query78-build-v3/`.

| Boundary | Finding and anchors |
| --- | --- |
| Original lexical endpoint retained | `common.py:124–135`: original map key remains outside the row. All previous bytes/SHA/resolved target/lstat full mode/dev/inode/direct-link spelling/target-mode expressions remain identical; only the `inventory` member is added. Initial lstat is a point observation, not a race-free type proof before open. |
| Separate object coordinate | `common.py:138–154`: resolve the parent strictly and append the original last component. This follows actual parent-symlink semantics without blindly normpathing across them, and does not whole-resolve away a direct final symlink. Coordinate lstat object identity, direct link, followed body, resolved target and target mode must match the lexical endpoint. |
| Root and row shape | `common.py:155–174`: all four fixed roots must be canonical directories. BUILD precedes OUT, whose evidence inventory excludes BUILD. ART/source and IMAGES are distinct. Internal coordinates require a canonical nonempty relative key. Regular rows use bytes/SHA/permission mode, direct symlinks use the exact link row; evidence uses bytes/SHA, matching its followed-body inventory. |
| Explicit external role | `common.py:171–174,214–217`; `build.py:354–358`: outside-inventory endpoints retain an actual object coordinate but null root/relative/row. Every such lexical endpoint and coordinate is still subject to complete identity comparison; external is not a dropped dependency or failed relativetest bypass. |
| Multiple aliases / conflicts | `common.py:177–183,186–201,214–217`: aliases reaching the same direct object are accepted only if complete endpoint/coordinate/row claims agree. The original lexical name is not an equality field inside the row, so ordinary equivalent spellings do not falsely conflict. The collision map is temporary; the returned/captured map still contains every lexical key. Conflicting full rows fail closed, within batches and across all frozen captures. |
| Freeze and full rebind | `build.py:58–81`; `common.py:186–217`: the new field is serialized into BINDING receipt/snapshot strings. Existing rich-row equality and all earlier checked body/resolved/mode comparisons remain. Current identity/coordinate is recomputed only for equality with frozen data, never adopted as a new association. Package capture→main bind still compares all fields. |
| Final lookup | `build.py:342–366`: actual inventories are now indexed using the **frozen** role/relative key and compared to the frozen row. Raw `..` spellings are no longer directory-walk keys. Complete endpoint/root/coordinate rechecks bracket the inventory/seal work. Frozen bindings are included in the output seal and terminal-bound BINDING receipt identities. |

Resolving a symlinked parent can lead to the same canonical direct object while
preserving the lexical endpoint key. A direct final symlink instead retains its
own object coordinate/link inventory row; its resolved target is still a separate
full-identity field. The target's regular-file coordinate is not confused with that
link object's coordinate. Collision comparison groups by object spelling, not by
followed target or inode alone. These are source/API semantic findings, not
executed example cases or a general filesystem/security qualification.

The known retained query77 dependency remains supported as ordinary source evidence:

```text
K77/arch/arm/boot/compressed/../../../../lib/../scripts/dtc/libfdt/fdt.c
```

Existing .fdt.o.cmd:100, consumed-source map and canonical manifest/body associate
it with `K77/scripts/dtc/libfdt/fdt.c` (7882B,
SHA `0574facf26b0e9d1fdbce3747ceba66b4bdc16bd161601e2bed6a2791b1774c6`).
The raw parent-component key is absent from the canonical manifest, as v2 review
reported. V3's new source coordinate logically selects
`linux/scripts/dtc/libfdt/fdt.c` under ART while keeping that lexical alias bound.
This review checked saved receipt/map/manifest/body association only: it did not
call coordinate/identity, simulate a filesystem, or query/materialize a future tree.
Author's saved parent-resolution/lstat observation is cited, not rerun as v3 proof.

## Changed-callers / ordinary error and resource closure

- **First capture after roots exist:** all initial closed input/client review/
  baseline source/tool/archive/config checks still precede recipe-owned mkdir/copy/
  tool effects (`build.py:130–149`). The four canonical directories are created
  before prepared bind (`:162–170`). No identity/coordinate capture is inserted
  into preflight before those roots exist. Local module imports only define the
  future method; ordinary interpreter/stdlib startup/read I/O is not claimed zero.
- **Captured-data flow:** prepared archive/inventory receipt/copied DTB/client,
  configure-generated executable files, actual BUILT output/body receipt, full
  closure .cmd/consumed/generated/loaded/log/trace/JSON/receipt data, optional absence
  resolution and captured sets all flow through unchanged bind/recheck methods.
  Added coordinates do not mutate those earlier body pins or collapse aliases.
- **Package call/rebind:** unchanged `package.py:206–214,277–308` checks original
  BUILT body, already checked in-memory ELF/Image/zImage/DTB/piggy and compression/
  archive pins, calls full recheck before image write, fixes expected constructed
  image size/SHA before publication, compares written bytes and returns full rows.
  Main's unchanged `build.py:391–393` rebind requires complete matching rich rows,
  now including coordinates, before finish.
- **Final completion ordering:** finish's lookup block alone changes. Final known
  inventory values must agree with frozen coordinate rows; unknown extras remain
  observations. Completed exclusive FINAL_OUTPUTS write/body check and rechecks
  precede terminal JSON. `common.py:220–228` still serializes complete JSON to an
  exclusive pending file, performs one unbuffered write/flush/fsync/one close/body
  check, then no-overwrite no-follow hardlink publication as last authored effect.
  Pending remains; normal return has no later authored I/O/print. No new hash cycle
  or finite snapshot/set-growth contradiction was identified.
- **Preserved R2:** the eight explicit read/write/recording/reraise/publication
  helpers and command/outer failure paths remain AST-identical. Open/subprocess/
  timeout/nonzero becomes primary before bookkeeping; each secondary operation is
  guarded; mandatory recording failure on an otherwise successful path fails the
  run. Same exception object/saved traceback tail is propagated, bounded label/type
  notes are best effort, outer FAILURE recording cannot replace an active primary.
  Single unbuffered writes/one-shot closes do not retry unknown buffered writes.
  Storage failure can prevent all durable evidence. This retains v2's narrow
  authored-path disposition, not every inherited library/context-manager or Python
  asynchronous/resource-intrinsic failure guarantee.

The method remains conditional on a trusted serialized host/namespace and ordinary
local filesystem semantics. It establishes endpoint comparisons, not directory-
inode custody, atomic whole-interval immutability, root/same-UID concurrent-mutation
immunity or immediate prevention at every Kbuild child/header read. No power-loss/
directory-fsync durability or arbitrary asynchronous syscall-return guarantee is
claimed. A future consumer must verify complete FINAL JSON and actual seal/BINDING
associations; intermediate BUILT/PACKAGE/CLOSURE/BINDING/pending is not completion.

## Independent static comparisons and custody

`STATIC01.json` records independently checked:

- All104 input pins match BEFORE/AFTER/current; immutable old68 are an exact subset.
- Six actual source/17 document pins, source/handoff seals and source aggregate.
- All four final call/import/class/function-span lists equal actual AST. Author
  AST/in-memory syntax4/4 is cited, not rerun; reviewer parses AST only and creates
  no executable code object.
- All six direct diffs regenerate exactly. newc/package/.gitignore are full-byte
  identical to v2. newc SHA `06e796f5e31b845988ce980d54bc601544fbd1e069cf4035eb52d4a3344da18a`;
  package SHA `fc6b90c43c98ac45bf07553c352d73c3f203e19603ca46e6f2ab0f636deb73c2`.
- All24 protected common function ASTs, eight R2 helpers,11 unchanged Build methods,
  main and20 non-DOC fixed path/limit expressions agree. Original identity field
  expressions, capture checks/return and compare_rows loop are preserved. Fixed
  input validation changes only its own hard-bound manifest hash. Only two new
  helpers are introduced, neither called here.

No kernel/client/config/compiler/tool/target/CRC/archive/memory-policy change or
budget increase. Source01/build01/images01 remain fixed/exclusive/nonresumable;
no actual future output-root state was queried. Clang/LLD23.1.3, ALL-symbol two-
change config check, strict full baseline source verification,50-record archive
one-remove/one-add with old PID1, actual compression/ELF/layout predicates and
4MiB/40MiB/CMA/128KiB caps remain. No monitor/new query command/autostart or native
recipe is added. Unsupported advertised wifi attach remains forbidden, not full
PLAT-02/station support.

Known fixed client REVIEW.md SHA
`83f1b9ec9b1a125c9485038b0571ec5c7c5c241c0fb2b53bbbffddf5646c0f60`
and parent contract SHA
`f29ea0e41d947c44bb4ae69a3244bcac3b543a3b2db08474ec4b4e5e9db676a7`
were freshly matched. The explicit64-lowercase-hex fixed-path argument remains
required, not a ready Boolean or prose-PASS/native authority. Client's recorded36
tool commands remain historical compile custody, not ELF/runtime execution here.

INPUTS01 contains134 path-associated reader inputs. POSTCHECK01 rehashes all134
unchanged, no parent-context drift/rebase. Saved dependency metadata has3903 paths/
86 parent-component aliases, but this review does not claim to rehash every body,
all91270 source entries,50k tool inputs or a250219-path kernel closure. No preflight,
new memory/CRC/ABI-fit result, namespace/function/failure-path execution or runtime
qualification follows. Strict future checks may refuse host drift or ordinary
actual-build differences; no refusal may be waived by this source disposition.

No reviewer collection error or policy refusal occurred. Author READ_ERROR01 and
all previous author/reviewer bookkeeping errors/negative dispositions remain
unchanged as history. GPL notices/derivation remain; third-party toolchain/libc/
kernel/retained payload licensing is separate, no global redistribution/publication
grant. Safe source metadata only; binaries/images remain excluded. No Git/index,
source/old-review/AGENTS/central-plan/provider/model/safeguard write or private
capture/analysis/HOLD/CONTROL/current-state/proc/dev/clock/process/network/device/
serial/UART/RPC/hardware action occurred. Only this fresh review directory was written.

## Parent boundary / handback

Parent must read/reverify the review's final input/output seal and separately
disposition this exact source; any actual offline invocation still needs explicit
selection and fresh build-host/input admission. This review selects none and does
not adopt source, integrate an image, queue a build, or supply a native recipe.

No physical/manual-readiness/current owner/catch/clock/global-idle/FD/recovery proof.
Historical U-Boot12:12UTC and both FAILED/HOLDs remain history only. Bootstrap,
observer/lifetime/backing/retention/provisioning/main, native watchdog/combined
memory/full PLAT-02 and G2/G3/G8 remain open. **No automatic next operation.**
