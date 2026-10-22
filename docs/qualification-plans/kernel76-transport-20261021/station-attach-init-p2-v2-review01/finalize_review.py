#!/usr/bin/env python3
"""Rehash static public review inputs and exclusively seal new review outputs."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
OUT = ROOT / 'docs/qualification-plans/kernel76-transport-20261021/station-attach-init-p2-v2-review01'
REPORT = OUT.parent / 'STATION76_ATTACH_INIT_P2_V2_REVIEW.md'


def pin(path):
    if path.is_symlink():
        raise ValueError('unexpected symlink')
    raw = path.read_bytes()
    return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def main():
    receipt = json.loads((OUT / 'reader-receipt.json').read_bytes())
    for rel, expected in receipt['inputs'].items():
        if Path(rel).is_absolute() or '..' in Path(rel).parts:
            raise ValueError('unexpected input path')
        if pin(ROOT / rel) != expected:
            raise ValueError('public review input changed: ' + rel)
    doc = OUT.parent
    baseline = json.loads((doc / 'station-attach-init-p2-v2-source01/copy-baseline.json').read_bytes())
    expected_custody = {}
    for directory in ('station-attach-init-p2-source01', 'station-attach-init-p2-review01'):
        name = 'candidate-seal.json' if directory.endswith('source01') else 'review-seal.json'
        path = doc / directory / name
        expected_custody.update(json.loads(path.read_bytes())['files'])
        expected_custody[str(path.relative_to(ROOT))] = pin(path)
    if expected_custody != baseline['frozen_custody']:
        raise ValueError('v1/review custody is not the exact union of both prior seals')
    paths = [REPORT] + sorted(p for p in OUT.iterdir() if p.is_file())
    summary = {
        'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'reader_inputs_revalidated_unchanged': len(receipt['inputs']),
        'frozen_v1_review_custody_exact_prior_seal_union': len(expected_custody),
        'candidate_seal_sha256': '40928114e2de10bc84b0dc26301db2563f1d9a1df4273b6610a7cc7ba13317bb',
        'source_sha256': 'a23ec7ce1e9794762e086853655e286b3567db5ddb4e449c88bab25390b9c75f',
        'disposition': 'Narrow SOURCE sufficiency: R1-R3 corrections accepted at static source/API scope only',
        'scope': 'static public read/diff/hash only; no compilation, runtime/tests, P1 review, target/UART/RPC/network/private IO or Git/index edits',
        'files': {str(path.relative_to(ROOT)): pin(path) for path in paths},
    }
    with (OUT / 'review-seal.json').open('x') as stream:
        json.dump(summary, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'inputs_unchanged': len(receipt['inputs']),
                      'review_files': len(summary['files']),
                      'seal': pin(OUT / 'review-seal.json')}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments or optimized Python')
    main()
