#!/usr/bin/env python3
"""Copy only inspected public synthetic A1 tests; never run its old collector."""
import hashlib
import json
from pathlib import Path
import sys
import datetime

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
source = ROOT / "apps/qualification/kernel76-station/tests"
seal = json.loads((ROOT / "docs/qualification-plans/kernel76-transport-20261021/station-control-a1-host01/candidate-seal.json").read_bytes())
(HERE / "tests").mkdir(exist_ok=False)
pins = {}
for name in ("shim.h", "test_control.c"):
    original = (source / name).read_bytes()
    pin = seal["files"][str((source / name).relative_to(ROOT))]
    assert hashlib.sha256(original).hexdigest() == pin["sha256"]
    data = original
    if name == "shim.h":
        old = b"struct hgic_bus { unsigned int drv_tx_headroom; };"
        assert data.count(old) == 1
        data = data.replace(old, b"#ifndef A2A_BUS_DECL\nstruct hgic_bus { unsigned int drv_tx_headroom; int rx_failed; };\n#endif")
    with (HERE / "tests" / name).open("xb") as f:
        f.write(data)
    pins[name] = {"a1_bytes": len(original), "a1_sha256": hashlib.sha256(original).hexdigest(),
                  "copied_bytes": len(data), "copied_sha256": hashlib.sha256(data).hexdigest()}
with (ROOT / "docs/qualification-plans/kernel76-transport-20261021/station-ingress-a2a-host01/test-copy.json").open("x") as f:
    json.dump({"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "argv": sys.argv,
               "python": sys.version, "files": pins}, f, indent=2)
    f.write("\n")
