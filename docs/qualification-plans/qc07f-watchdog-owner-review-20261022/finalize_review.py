#!/usr/bin/env python3
"""Read-only public input revalidation and exclusive new review seal."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
OUT = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-review-20261022'


def pin(path):
    if path.is_symlink():
        raise ValueError('unexpected final-component symlink')
    raw = path.read_bytes()
    return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def main():
    receipt = json.loads((OUT / 'reader-receipt.json').read_bytes())
    for rel, expected in receipt['inputs'].items():
        if Path(rel).is_absolute() or '..' in Path(rel).parts:
            raise ValueError('unexpected relative input')
        if pin(ROOT / rel) != expected:
            raise ValueError('public input changed: ' + rel)
    paths = sorted(path for path in OUT.iterdir() if path.is_file())
    data = {
        'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'reader_inputs_revalidated_unchanged': len(receipt['inputs']),
        'candidate_source_tree_sha256': '2f9e50840a58bbcf8025df1b4e6850278eee0ff5d9a48e0354c531f218b7872a',
        'disposition': 'CORRECTIONS REQUIRED: W1 mutable session/accounting initialization; W2 early NEXT nonzero outputs; W3 guard/codec effects before known held-loan BUSY',
        'scope': 'focused ordinary public static SOURCE/API/diff/hash only; no generator/build/test execution, target/UART/RPC/status/network/private IO, P1 inspection or old-source/Git edits',
        'files': {str(path.relative_to(ROOT)): pin(path) for path in paths},
    }
    with (OUT / 'REVIEW_SEAL01.json').open('x') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'inputs_unchanged': len(receipt['inputs']), 'review_files': len(data['files']),
                      'seal': pin(OUT / 'REVIEW_SEAL01.json')}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments or optimized Python')
    main()
