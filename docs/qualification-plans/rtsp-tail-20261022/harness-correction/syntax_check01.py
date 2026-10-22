#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Parse/compile Python SOURCE in memory only; never import project modules."""
from pathlib import Path
import datetime
import hashlib
import json
import sys

root = Path(__file__).resolve().parents[4]
app = root/'apps/rtsp-auth/isolated-server-host-tail'
name, = sys.argv[1:]
if not name or not all(c.isalnum() or c in '-_' for c in name):
    raise RuntimeError('fresh_name')
rows = []
for path in sorted(app.rglob('*.py')):
    data = path.read_bytes()
    row = {'path': str(path.relative_to(root)), 'sha256': hashlib.sha256(data).hexdigest()}
    try:
        compile(data, str(path), 'exec')  # no execution, no .pyc writes
        row['syntax'] = 'PASS'
    except SyntaxError as exc:
        row.update(syntax='FAIL', error_type=type(exc).__name__, line=exc.lineno)
    rows.append(row)
report = {'scope': 'Python syntax ONLY; no builder/test imports or execution',
          'command': 'python3 -B ' + str(Path(__file__).relative_to(root)) + ' ' + name,
          'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'python': sys.version,
          'files': rows, 'all_pass': all(row['syntax']=='PASS' for row in rows)}
with (Path(__file__).parent/(name+'.json')).open('x') as f:
    json.dump(report,f,indent=2);f.write('\n')
print('PYTHON_SYNTAX files='+str(len(rows))+' all_pass='+str(int(report['all_pass'])))
raise SystemExit(0 if report['all_pass'] else 1)
