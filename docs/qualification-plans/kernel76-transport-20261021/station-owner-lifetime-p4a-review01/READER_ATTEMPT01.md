# Initial static-reader metadata error — preserved

Command (same command later completed after reader-only correction):

```text
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-owner-lifetime-p4a-review01/read_public_inputs.py
KeyError: 'kernel_inputs'
exit status 1
```

Initial reader SHA256:
`b7c8e372f583496c61260e1e67601823d81844ba3aca106109eb706094a03e1b`.
The partial actual diff and public-excerpts JSON remain preserved byte-exact.

This was a reviewer metadata-key assumption, not a candidate/compiler failure or
policy rejection. The author's actual runner/receipt uses `host_kernel_inputs`,
not the earlier P2 runner's `kernel_inputs`. Only the new review reader was fixed:
use that actual key, and validate rather than overwrite already-produced partial
outputs when resuming the static read. No author source/receipt or frozen input
was changed, no compilation or tests run, and no safeguard/settings changed.
