# Reviewer receipt-schema bookkeeping error

The first independent inline static collector stopped before writing any receipt:

```text
Traceback (most recent call last):
  File "<stdin>", line 42, in <module>
AssertionError: apps/qualification/encoder-query78-build-v2/build.py
```

The reviewer incorrectly reused v1's `functions` callsite-receipt schema. Actual
v2 CALLSITES01 uses `definitions` and includes class definitions as well as
functions. Read-only inspection confirmed the383 build.py call records and13
imports already agreed; the definition-list schema was the mistaken assumption.
Corrected collection uses the actual published schema including ClassDef spans.

No candidate/helper/code object or tool was imported/called/executed; no source,
old artifact or existing receipt was changed. This was not a candidate defect,
runtime result or policy refusal. The R1-A alias finding is independent of it.
