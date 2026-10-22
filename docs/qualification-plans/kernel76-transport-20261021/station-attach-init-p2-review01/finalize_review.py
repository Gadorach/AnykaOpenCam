#!/usr/bin/env python3
"""Final public-input rehash and exclusive review seal; no build/test/device IO."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
OUT = ROOT / 'docs/qualification-plans/kernel76-transport-20261021/station-attach-init-p2-review01'
REPORT = OUT.parent / 'STATION76_ATTACH_INIT_P2_REVIEW.md'


def pin(path):
    if path.is_symlink():
        raise ValueError('unexpected symlink')
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    original = json.loads((OUT / 'reader-receipt.json').read_bytes())
    for rel, expected in original['inputs'].items():
        if Path(rel).is_absolute() or '..' in Path(rel).parts:
            raise ValueError('unexpected input path')
        if pin(ROOT / rel) != expected:
            raise ValueError('input changed: ' + rel)
    paths = [REPORT] + sorted(p for p in OUT.iterdir() if p.is_file())
    data = {
        'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'public_reader_inputs_revalidated_unchanged': len(original['inputs']),
        'candidate_seal_sha256': 'c54ff57f63bec1180199a97321fbf1ea38e9b448edf0969c9135953c565b1919',
        'disposition': 'CHANGES REQUIRED: P2-R1 missing required chanctx callbacks; P2-R2 surviving registration gap; P2-R3 deferred negative-status first-error capture',
        'scope': 'ordinary SOURCE/build-custody review only; no compilation, runtime/tests, P1 review, Git/index, target/UART/RPC/network/private IO',
        'files': {str(p.relative_to(ROOT)): pin(p) for p in paths},
    }
    with (OUT / 'review-seal.json').open('x') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'inputs_unchanged': len(original['inputs']),
                      'review_files': len(data['files']),
                      'seal': pin(OUT / 'review-seal.json')}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments/optimized Python')
    main()
