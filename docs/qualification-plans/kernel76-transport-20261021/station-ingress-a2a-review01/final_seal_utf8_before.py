#!/usr/bin/env python3
"""Exclusive review seal; public frozen inputs and review-owned outputs only."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

assert not sys.flags.optimize
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
def pin(p):
    b = p.read_bytes()
    return {'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()}
def load(p):
    return json.loads(p.read_bytes())
result = load(HERE / 'run02/result.json')
assert result['all_pass'] and len(result['runtime_mutants']) == 8
for rel, expected in result['inputs'].items():
    assert not Path(rel).is_absolute() and '..' not in Path(rel).parts
    assert rel.startswith(('apps/qualification/kernel76-station',
                           'docs/qualification-plans/kernel76-transport-20261021/'))
    assert pin(ROOT / rel) == expected, rel
for run in ('run01', 'run02'):
    for rel, expected in load(HERE / run / 'output-hashes.json').items():
        assert not Path(rel).is_absolute() and '..' not in Path(rel).parts
        assert pin(HERE / run / rel) == expected
old = load(HERE / 'run01/result.json')
for name in ('review_a2a.py', 'supplement.c'):
    assert pin(HERE / 'run01-fixture-before' / name) == old['inputs'][str((HERE / name).relative_to(ROOT))]
refs = []
source = ROOT / 'apps/qualification/kernel76-station-ingress/source'
for p in sorted(source.rglob('*')):
    if p.is_file():
        for n, line in enumerate(p.read_bytes().decode().splitlines(), 1):
            if any(s in line for s in ('rx_packet', 'rx_error', 'rx_failed', 'hgic_fwctrl_init(', 'hgic_fwctrl_rx(')):
                refs.append({'path': str(p.relative_to(source)), 'line': n, 'text': line})
with (HERE / 'context-audit.json').open('x') as f:
    json.dump({'scope': 'independent selected-source reference inventory, not behavior assertions', 'references': refs}, f, indent=2)
    f.write('\n')
files = {str(p.relative_to(ROOT)): pin(p) for p in sorted(HERE.rglob('*')) if p.is_file()}
report = HERE.parent / 'STATION76_INGRESS_A2A_REVIEW.md'
files[str(report.relative_to(ROOT))] = pin(report)
with (HERE / 'review-seal.json').open('x') as f:
    json.dump({'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'argv': sys.argv,
               'python': sys.version, 'scope': 'A2A_BOUNDED_SOURCE_HOST_GO_ONLY_FULL_S4_BUILD_NATIVE_NO_GO',
               'all_successful_inputs_rechecked': True, 'all68_A1_originals_rechecked': True,
               'failed_review_fixture_preserved': True, 'final_run': 'run02', 'files': files}, f, indent=2)
    f.write('\n')
print('Independent A2a review sealed; bounded SOURCE/HOST GO only')
