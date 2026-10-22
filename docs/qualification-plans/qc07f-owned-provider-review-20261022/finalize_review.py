#!/usr/bin/env python3
"""Revalidate public pins and exclusively seal this independent static review."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
OUT = ROOT / 'docs/qualification-plans/qc07f-owned-provider-review-20261022'


def pin(path):
    current = path
    while current != ROOT:
        if current.is_symlink():
            raise ValueError('unexpected symlink')
        current = current.parent
    raw = path.read_bytes()
    return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def main():
    reader = json.loads((OUT / 'reader-receipt.json').read_bytes())
    for rel, expected in reader['inputs'].items():
        if Path(rel).is_absolute() or '..' in Path(rel).parts:
            raise ValueError('unexpected input path')
        if pin(ROOT / rel) != expected:
            raise ValueError('input changed: ' + rel)
    files = sorted(p for p in OUT.iterdir() if p.is_file())
    expected_names = {'REVIEW.md', 'read_public_inputs.py', 'finalize_review.py',
        'actual-watchdog-v2-to-provider-app.patch', 'actual-watchdog-v2-to-provider-generated.patch',
        'actual-applied-source.patch', 'public-excerpts.json', 'object-custody.json',
        'captured-public-input-map.json', 'verification.json', 'reader-receipt.json'}
    if {p.name for p in files} != expected_names:
        raise ValueError('review output set differs or already sealed')
    seal = {'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'reader_inputs_revalidated_unchanged': len(reader['inputs']),
        'source_tree_sha256': 'fb9cc61500beaad9c6a2cb518ebf06ce73cd1d7a3d9595e54b8153d8db58dedb',
        'disposition': 'Narrow static SOURCE/API/ownership source-piece and changed/reused-object custody sufficiency; not complete-provider/native/full-app acceptance',
        'open_blockers': ['healthy never-activated COMMITTED-role handback', 'external failed-next/partial-transfer process retention',
            'admitted backing and allocation design', 'provisioning and owned listener', 'ordinary functional/native timing/query-driver/watchdog/combined-load qualification', 'full app and G2/G3/G8'],
        'scope': 'public byte/diff/hash/receipt/dependency/archive inspection only; no compiler/generator/candidate/helper/tests/runtime/target/network/private/P1/Git work',
        'automatic_next_queue': False,
        'files': {str(p.relative_to(ROOT)): pin(p) for p in files}}
    target = OUT / 'REVIEW_SEAL01.json'
    with target.open('x') as stream:
        json.dump(seal, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'inputs_unchanged': len(reader['inputs']), 'review_files': len(files), 'seal': pin(target)}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments/optimized Python')
    main()
