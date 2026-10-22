# Initial independent collector bookkeeping error — preserved

Command:

```text
python3 -B docs/qualification-plans/encoder-query77-client-review-20261022/collect_static.py before01
```

Exit1, `KeyError` at original collector line189: it incorrectly expected imported
`encoder-open-query.h` and `.inc` under the frozen seal's `evidence_pins/.../candidate/`
map. That map holds only real provider `qc07f_hwenc.c/.h`; header/inc are the
original five-file `source_files` entries. All originals are present and unchanged.
`STATIC_before01.json` retains the error/traceback and every completed input hash.
This is a collector schema error, not a policy rejection or candidate/runtime failure.

Initial collector SHA256:
`fb2dfa7ef91a59dbbe518524713059f2f6169e72aa619e8b6a59c26623a8e184`.

Only reviewer bookkeeping was corrected: the two imported files now use their exact
frozen `source_files` entries; real provider C/H still require exact evidence pins.
No file was skipped, substituted, compiled or executed. Fresh before02/after02
receipt names were admitted; before01 remains immutable. Candidate/author source,
builds, seals and all inputs were not changed.

Exact reversible collector change (in addition to admitting before02/after02 in
the existing argument-name tuple):

```diff
- check(PLATFORM/name,frozen['evidence_pins']['docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate/'+name])
+ if name in ('encoder-open-query.h','encoder-open-query.inc'):
+     expected = frozen['source_files']['apps/qualification/kernel76-encoder-identity/'+name]
+ else:
+     expected = frozen['evidence_pins']['docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate/'+name]
+ check(PLATFORM/name,expected)
```
