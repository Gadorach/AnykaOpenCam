# Static callsite closure — bounded candidate only

Inspected module pin is in SYNTAX_STATUS01.json. AST metadata was inspected without
importing the module or executing decorators/class definitions/functions. This is
not a whole-repository callgraph, dependency resolver or runtime audit. No existing
caller was added/edited; no controller/main/C1/C2a integration is delivered.

| Site | Calls / closure |
|---|---|
| imports, lines7–8 | stdlib `dataclasses.dataclass`, `enum.Enum` only; no Boot/recorder/helper/candidate imports |
| class bases, lines39/45/50 | Enum, Enum, built-in Exception respectively; class construction would use normal Python machinery upon a future import, not executed here |
| decorator, line58 | `dataclass(frozen=True, slots=True)` only; it would construct the record class upon future import, no authored I/O |
| error init, lines53/55 | built-in `type`, `super`, and base Exception initializer; fixed enum/message only, no input formatting |
| classifier, lines74–77 | built-in `type`/`len`, fixed-code `ColdContextError` on explicit guards |
| short record branch, lines81/85/87 | built-in `len`, exact built-in bytes literal `startswith` on short suffix, `TranscriptFacts` constructor |
| full record branch, lines88–91 | exact built-in bytes argument `startswith` at fixed offset, `TranscriptFacts`, built-in `len` |
| EOF, lines96–98 | built-in `len`, enum selection and `TranscriptFacts` |

The constructor's `super().__init__` and its inner `super()` are distinct AST Call
nodes; all17 Call nodes are covered above. Dataclass-generated methods and Enum/
Exception internals were not imported, run or recursively audited. HOST AST and
compile establish syntax only, not stdlib/runtime/deployment closure.

At module level authored operations are imports, immutable constants/per-record
table, class definitions and one decorator; there is no classifier call, I/O,
CLI/main, example/default input, callback registration or application bootstrap.
The sole operational function is `classify_cold_context`; classes supply its
enum/error/frozen data vocabulary, not effectful services. No code object from
compilation was invoked or saved. Review must independently inspect this closure
before any later import/adoption; no call/test/native task is queued here.
