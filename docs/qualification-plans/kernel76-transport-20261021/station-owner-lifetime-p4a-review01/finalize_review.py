#!/usr/bin/env python3
"""Rehash static public review inputs, exclusively seal new report/evidence."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
OUT = ROOT / 'docs/qualification-plans/kernel76-transport-20261021/station-owner-lifetime-p4a-review01'
REPORT = OUT.parent / 'STATION76_OWNER_LIFETIME_P4A_REVIEW.md'


def pin(path):
    if path.is_symlink():
        raise ValueError('unexpected symlink')
    raw = path.read_bytes()
    return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def main():
    receipt = json.loads((OUT / 'reader-receipt.json').read_bytes())
    for rel, expected in receipt['inputs'].items():
        if Path(rel).is_absolute() or '..' in Path(rel).parts:
            raise ValueError('unexpected relative input')
        if pin(ROOT / rel) != expected:
            raise ValueError('public input changed: ' + rel)
    paths = [REPORT] + sorted(p for p in OUT.iterdir() if p.is_file())
    data = {
        'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'reader_inputs_revalidated_unchanged': len(receipt['inputs']),
        'candidate_seal_sha256': '49e1c0e6209371e4714e4fb1ff5adad4d8cb042014fe4508f2e5257e37f49490',
        'source_sha256': '6fc087502ac6d0fe2a1b5988b85404cc81a0d95b0908d0964cfc9fd1686c4dd7',
        'disposition': 'Narrow static SOURCE sufficiency for selected shared-owner prerequisite; P3 design may resume only with parent selection and preserved producer/join contract',
        'scope': 'static public read/diff/API/hash only; no compilation/tests, P1 inspection, target/UART/RPC/status/network/private IO or Git/index/source/helper/central-plan edits',
        'files': {str(path.relative_to(ROOT)): pin(path) for path in paths},
    }
    with (OUT / 'review-seal.json').open('x') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'inputs_unchanged': len(receipt['inputs']), 'review_files': len(data['files']),
                      'seal': pin(OUT / 'review-seal.json')}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments or optimized Python')
    main()
