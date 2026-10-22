# Preserved public-reader bookkeeping error

First invocation:

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-attach-init-p2-v2-review01/read_public_inputs.py
ValueError: changed receipt header missing/mismatched in author dependency pins
exit status 1
```

Initial reader SHA256:
`a3ce20a9f2ce23d665771262be2c85fe52764457049af224a1f35475184a6b5d`.
Its partial `actual-v1-v2-diff.patch` is preserved, not overwritten. No candidate,
frozen review, source or author evidence was changed. This was a reviewer metadata
path-comparison error, not a policy rejection or a candidate failure.

The author's dependency map names the same header through both literal paths
`source/hgic_smac/../attach_init.h` and `source/utils/../attach_init.h`. The first
reader looked for only `source/attach_init.h`. The corrected reader lexically
normalizes dependency names, requires all matching rows to equal the final header
pin, and revalidates the preserved actual diff before continuing with fresh output
names. It does not rerun any compiler or change author evidence.

The first reader is reconstructible from the final reader by removing
`import posixpath`, restoring the direct `header_dependencies.get(...)` comparison,
and replacing the preserved-diff validation branch with exclusive `open('x')`.
