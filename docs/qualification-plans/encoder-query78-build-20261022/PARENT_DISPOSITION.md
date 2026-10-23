# Parent disposition — recipe v1 R1/R2 corrections required

Selection2026-10-22; actual review host date2026-10-23 retained. ENC-02 /
PLAT-01/02/05/08. Parent read the complete independent review and actual command,
receipt and finalization paths. Verified all100 reader inputs and seven review
outputs unchanged. Independent FINAL_SEAL SHA256:
`3b293d00fa1ea7bdb41cfce9ff0b12e1850c92b02c83ee7b795e97f49a0a5685`.

Accept the **two ordinary correction-required findings**, not SOURCE/build-method
acceptance or invocation. No recipe/helper, archive transformer, compiler or
package code was executed; no archive/kernel/image exists from this selection.

- **R1:** final inventories record newly observed output/closure hashes without
  comparing image, built artifacts, copied DTB and captured closure back to the
  qualification pins. FINAL also precedes complete final inventories/seal. Preserve
  and revalidate those exact associations before packaging/final completion; create
  the completed seal before an unambiguous successful terminal receipt that binds
  it. Freshly observed bytes cannot replace the facts used to qualify the package.
- **R2:** unguarded command/outer failure receipt writes can replace the primary
  operation exception. Preserve/re-raise the original exception and traceback,
  record secondary bookkeeping errors separately where possible, and still fail
  a previously successful path when mandatory recording fails. Storage failure
  may prevent durable failure evidence; no write/recompile retry or ignored error.

V1 six-file aggregate
`d3ac858dafa835587474bf7708ef0c5f5a17820cccbe42f5cb4619a52e29a373`
and its syntax01/02/derivation/callsite/seal/review history remain frozen and
unaccepted. Syntax4/4 is not behavioral or filesystem qualification. Parent's
mistyped review-input lookup under apps/ returned missing-file; the actual docs/
manifest was then read. No guessed path/file or candidate effect was created.

Parent selects fresh `apps/qualification/encoder-query78-build-v2/` and matching
qualification docs for **R1/R2 only plus necessary fresh provenance/path binding**.
No broader recipe, compiler/config/source/client/archive/CRC/layout/memory/monitor
or native scope change. Source authoring and AST/syntax only, no imports/calls,
preflight/help/CLI, archive generation/extraction, kernel/image build or tests.
Independent re-review and separate parent build selection remain required.

Actual client review/ARM input identities, full exact query77 kernel baseline,
Clang/LLD23.1.3, two permitted config changes,50-record archive one-remove/one-add,
old PID1/other-record preservation, source/closure checks,4MiB/40MiB/CMA/128KiB
limits and no original-write/no-resume policy remain. No overwritten/normalized
failure, new admission flag, protection relaxation or native recipe is allowed.

Both original FAILED/HOLDs remain untouched. No current target/owner/catch/clock
or manual readiness is inherited, and no physical action is requested. No target
image/module change, boot/query/reset, retention/backing/provisioning/main or
native/product acceptance follows. Source/review metadata is local Git eligible
only after privacy/license inspection; generated artifacts/private evidence remain
excluded and the ordinary index must stay intact.
