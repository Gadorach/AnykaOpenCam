# Bench-only RAM SSH build recipe

This builds pinned public Dropbear, not a new transport/daemon implementation.
Review `docs/qualification-plans/stock-reference-20261013/ssh-control02/CONTRACT.md`
and `BUILD_ENTRY_REVIEW01.md` before executing. No target actions or keys here.
All output directories must be fresh; failed attempts are kept, never cleaned.

From workspace root:

```sh
python3 apps/qualification/stock-reference-20261013/ssh-control/prepare_source.py \
  --evidence evidence/delegated/20261013-stock-reference/ssh-control02/acquisition01 \
  --work work/lightbulb-camera/ssh-control02-source01
python3 apps/qualification/stock-reference-20261013/ssh-control/build.py \
  --source work/lightbulb-camera/ssh-control02-source01/dropbear-28216cd9af822732a1549b78621c2ea9a76a0fe5 \
  --output evidence/delegated/20261013-stock-reference/ssh-control02/build03
python3 apps/qualification/stock-reference-20261013/ssh-control/build_inspection.py \
  --build evidence/delegated/20261013-stock-reference/ssh-control02/build03 \
  --source work/lightbulb-camera/ssh-control02-source01/dropbear-28216cd9af822732a1549b78621c2ea9a76a0fe5 \
  --acquisition evidence/delegated/20261013-stock-reference/ssh-control02/acquisition01 \
  --output evidence/delegated/20261013-stock-reference/ssh-control02/inspection01
python3 apps/qualification/stock-reference-20261013/ssh-control/collect_licenses.py \
  --output evidence/delegated/20261013-stock-reference/ssh-control02/licenses03
python3 apps/qualification/stock-reference-20261013/ssh-control/musl_abi_notes.py \
  --build evidence/delegated/20261013-stock-reference/ssh-control02/build03 \
  --output evidence/delegated/20261013-stock-reference/ssh-control02/musl-abi01
python3 apps/qualification/stock-reference-20261013/ssh-control/seal.py
```

These historical output names now exist: use a separate new evidence/work scope
for any reproduction, never overwrite/refetch/extract this checkout again.
One complete source tree is shared read-only by3 isolated out-of-tree builds.
`-MD -fstack-usage` plus map/trace flags collect diagnostics in addition to the
control01 flags; localoptions values are unchanged. Cross conftests are not run.
Native hostkey tool is not executed; native-options server executes only inert
help/parser/algorithm queries. No keys, auth sessions, listeners, installs,
package-manager calls, firmware binaries, UART or network-interface operations.

Host `/tmp` is full: `build.py` sets TMPDIR to fresh output/compiler-tmp and HOME
to fresh output/empty-home; it does not delete unrelated files. No private inputs
are part of the recipe, outputs, environment allowlist or hash closure.

`seal.py` uses this unit's fixed paths: it packages only the3 public executables
and7 licensing files, validates unchanged source/allocated ELF sections and writes
a fresh release01 plus receipt/seal. It is not a generic packaging service and
also refuses repeat output. musl's main COPYRIGHT text in licenses01 was acquired
separately from its official v1.2.5 cgit URL; exact URL/hash/UTC is in that receipt.
