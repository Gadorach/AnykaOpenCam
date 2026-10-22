# ARM linker-generated LOAD label — second collector error preserved

Command `python3 -B docs/qualification-plans/encoder-query77-client-review-20261022/collect_static.py before02`
exited1 at reviewer line261 with `AssertionError: linker stubs`. The ARM BFD map
has exactly one nonabsolute `LOAD linker stubs` generated label, not a real file.
HOST has no nonabsolute LOAD entries. The collector had incorrectly required the
synthetic label to be a path in INPUTS.json. `STATIC_before02.json` preserves its
error/traceback and completed input hashes. No compiler, candidate, runtime or
policy failure occurred.

Collector before this correction SHA256:
`21b22115bb809ff0102dd8d5ff0352f655fcede350301132042543afdba23efb`.

Only the exact ARM synthetic label is excluded from **filesystem** pin lookup; it
remains recorded in receipt link-map labels. Real input paths still require their
original pins. Fresh before03/after03 output names preserve both earlier errors.
No source/build/evidence input was changed and no compiler/client/helper was run.

Exact added condition, plus admission of before03/after03 in the argument tuple:

```python
if arch == 'arm' and name == 'linker stubs':
    continue  # BFD-generated LOAD label, not a filesystem input.
```
