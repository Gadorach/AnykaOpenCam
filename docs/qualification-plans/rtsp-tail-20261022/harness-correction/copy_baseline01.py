#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exclusive byte copy of the 35 sealed PUBLIC source/license files; no runs."""
from pathlib import Path
import datetime
import hashlib
import json

root = Path(__file__).resolve().parents[4]
source = root / 'apps/rtsp-auth/isolated-server-candidate'
dest = root / 'apps/rtsp-auth/isolated-server-host-tail'
seal = root / 'evidence/delegated/20261012-rtsp-isolated-server/HALT-SEAL.json'
pins = json.loads(seal.read_text())['public_source_sha256']
dest.mkdir()  # refuse any previous derivative
rows = []
for name, expected in pins.items():
    relative = Path(name).relative_to('frozen-source')
    data = (source / relative).read_bytes()
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise RuntimeError('sealed public baseline changed')
    target = dest / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open('xb') as f:
        f.write(data)
    rows.append({'source': str((source / relative).relative_to(root)),
                 'copy': str(target.relative_to(root)), 'sha256_before_edits': actual})
with (dest / 'SOURCE_TAIL_PROVENANCE.json').open('x') as f:
    json.dump({'copied_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
               'scope': 'PUBLIC source/license bytes only; no generated startup records/images',
               'originals_immutable': True, 'files': rows}, f, indent=2)
    f.write('\n')
print('SOURCE_COPY files=' + str(len(rows)) + ' byte_equal=1 tests=0')
