# RAM-root rescue01 — HOLD

The unchanged retail 3.4.35 kernel cannot load the proposed external legacy
ramdisk. **No rescue SquashFS or type3 uImage was built.** This is the requested
unsupported-path stop, not a candidate for target upload.

`audit.py` rechecks the exact original kernel, recovered raw/analysis ELF byte
identity, bounded static disassembly, source anchors and RAM block capacity.
It never executes an ARM binary, accesses a device/network/socket/private input,
modifies originals, or generates a flash helper/image. Outputs must be fresh.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 apps/qualification/stock-reference-20261013/ram-root-rescue/test_host.py
PYTHONDONTWRITEBYTECODE=1 python3 apps/qualification/stock-reference-20261013/ram-root-rescue/audit.py --output evidence/delegated/20261015-resume/ram-root-rescue01/audit01
```

For a repeat use a new output path; existing evidence is never overwritten.
The input hashes are pinned in the recipe. Retained kernel source is GPL-2.0
(see its `COPYING` and individual notices); no stock userspace or credentials
are copied or redistributed by this unit. No workspace-wide licence is implied.

Detailed support/blocker and parent handoff:
`docs/qualification-plans/stock-reference-20261013/ram-root-rescue01/REPORT.md`.
