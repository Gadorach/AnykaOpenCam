# Station76 A1 — source/host candidate only

**No attach/build/staging/native authorization.** Parent independent exact-source
review is required. S1/S2/S3/S5/S6 and the USB-status/ingress-extent portion of S4
remain open. See
`docs/qualification-plans/kernel76-transport-20261021/STATION76_CONTROL_A1_CANDIDATE.md`.

`source/` began as exactly 67 public files from the preparer-pinned paired76
source. LICENSE, notices and relative paths are preserved. Only
`utils/fwctrl.c`, `utils/fwctrl.h`, and new `utils/control_receipt.h` differ.
Do **not** run the copied vendor Makefiles or treat this tree as a built module.

The one-time copy command already ran and refuses existing destinations:

```text
python3 -B apps/qualification/kernel76-station/prepare_source.py
```

The final synthetic host evidence is `station-control-a1-host01/run06/` under
the qualification directory above. A parent reproduction needs a **fresh** runNN:

```text
python3 -B apps/qualification/kernel76-station/run_host_tests.py --run run07
```

This command compiles/runs native x86_64 C harnesses only, extracts the real
control/copy functions with recorded byte extents, and refuses an existing output
directory. It imports no project/target helper. It also deliberately reproduces
the original short-payload ASan failure and runs faulty mutants; those expected
failures are retained, not a reason to retry anything on hardware.

The harness models locks, queues and completions deterministically in one thread.
It is not a model of kernel scheduling, USB URBs, timeout cancellation, module
unload, watchdog ownership, or resource retirement. No firmware payload, image,
ARM ELF/module, private profile/checkpoint, RPC, target or network is accessed.
