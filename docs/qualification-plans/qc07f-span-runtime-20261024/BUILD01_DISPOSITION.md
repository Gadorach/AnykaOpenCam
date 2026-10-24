# Build01 — FAILED PRESERVED before compilation

The fixed reuse audit refused the retained sensor-baseline object because its
actual dependency list includes old handoff/memory definitions. Six successful
public metadata commands (HOST version/target/cc1/as/ld/archive listing), zero C
compiles, zero REL links, zero candidate calls. All logs/report and original
build.py/source inputs remain unchanged. This is not a compiler/runtime PASS.

Bounded correction: fresh build02.py recompiles the actual unchanged sensor C
body with the COMPLETE accepted new handoff selected explicitly by compiler
`-include`, feature macros before system headers. It excludes the old sensor
object. No reuse guard is weakened or retried against the same artifact. This
is the additional real consumer required by the user's recompile-all rule, not
a synthetic wrapper, missing-definition guard, native owner or source change.

All reuse dependency/role/define/pin checks stay unchanged. Old runtime/memory/
server/flat-hub objects remain excluded. Both recipes refuse output overwrite.
If this single corrected compile/link cannot close, hand back honest compile/
partial-REL facts; do not extend link work, change tools or reroute safeguards.
