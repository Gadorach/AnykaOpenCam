# PIVOT01 continuation

The original `README.md`/`audit.py` concern the correctly held external-initrd
route and remain unchanged to preserve its seal. The separately authorized
tmpfs-pivot candidate uses `build_pivot.py`, `test_pivot_host.py`, inert
`pivot-stage1.sh` definitions and `pivot-stage2.sh`.

Full parent-operated protocol and safety gates:
`docs/qualification-plans/stock-reference-20261013/ram-root-rescue01/PIVOT.md`.

Use only `pivot01-build-final-a` or its byte-identical `pivot01-build-final-b`
under `evidence/delegated/20261015-resume/ram-root-rescue01/`.
Earlier `pivot01-build-a` is preserved exploratory output, not the final bundle.

The tar is 2058240 bytes, SHA256:
`ed535173959c9c1ed8b5e654932559e60031f96a323a4c1528d1b92b8df6095a`.

The XZ transport copy is 804284 bytes, SHA256:
`052385e53989ae64c78e455e5ad0bb1a89f13353ebb8dc7e30f3f0f75494e2c4`.

No target/native qualification, host pivot/mount, private inputs, programmer
implementation or write operations. It is RAM-shell preparation, NOT independent
power-loss recovery and NOT publication/source-licence closure. Stop after unit.
