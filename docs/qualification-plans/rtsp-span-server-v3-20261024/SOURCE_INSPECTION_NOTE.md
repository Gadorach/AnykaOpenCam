# Preserved author metadata-inspection failure

Before compilation, the first one-off stdlib text-proof inspection stopped with
`AssertionError: pause_client`: its multiline function-extraction regular
expression did not match the actual function. No candidate code was imported or
called, no compiler ran, and it had not written any proof/diff output. This is
an author evidence-extraction failure, not a source/compiler/runtime failure.
The subsequent inspection uses actual top-level definition/body delimiters and
still requires exact equality. No source edit or equality-check relaxation was
made to obtain the proof; this failed inspection is not relabelled PASS.

After the successful compile/REL recipe, the first final stdlib metadata pass
also stopped before writing outputs: its link selector counted `readelf -r`
alongside compiler `-r`. The corrected selector requires the actual compiler
`-nostdlib -r` link argv. All six compiles/two links had already succeeded and
were NOT repeated. This is a separate author metadata-counting failure, not a
changed recipe, weakened closure check or source/runtime observation.
