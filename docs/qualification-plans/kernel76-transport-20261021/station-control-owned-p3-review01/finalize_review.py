"""Rehash only public reader inputs; exclusively seal this review's outputs."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[4]
D = Path('docs/qualification-plans/kernel76-transport-20261021')
OUT = D / 'station-control-owned-p3-review01'
REPORT = D / 'STATION76_CONTROL_OWNED_P3_REVIEW.md'
K = Path('work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux')


def pin(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def read(path):
    p = ROOT / path
    assert p.is_file() and not p.is_symlink()
    assert not any(x in str(path) for x in ('station-attach-rx', 'P1_REVIEW', 'P1_TOOL'))
    return p.read_bytes()


def emit(name, value):
    with (ROOT / OUT / name).open('x') as f:
        f.write(json.dumps(value, indent=2) + '\n')


receipt = json.loads(read(OUT / 'reader-receipt.json'))
inputs = dict(receipt['inputs'])
for path, expected in inputs.items():
    assert pin(read(path)) == expected, path

additional = {}
for path, ranges in [
    (K / 'net/core/dev.c', [(3429, 3458)]),
    (Path('work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-package01/build/.config'),
     [(334, 342), (499, 506)]),
]:
    data = read(path)
    assert str(path) not in inputs or inputs[str(path)] == pin(data)
    inputs[str(path)] = pin(data)
    rows = []
    for start, end in ranges:
        value = b''.join(data.splitlines(keepends=True)[start - 1:end])
        rows.append({'start_line': start, 'end_line': end,
                     'text': value.decode(), 'sha256': pin(value)['sha256']})
    additional[str(path)] = {**pin(data), 'excerpts': rows}
emit('additional-api-excerpts.json', additional)
for path, expected in inputs.items():
    assert pin(read(path)) == expected, path
emit('input-seal.json', {
    'utc': datetime.now(timezone.utc).isoformat(),
    'reader_inputs_revalidated_unchanged': len(receipt['inputs']),
    'additional_excerpt_files': len(additional),
    'new_public_inputs': len(inputs) - len(receipt['inputs']), 'inputs': inputs,
    'scope': 'Static public byte custody only; not runtime/build verification.'})
outputs = {str(p.relative_to(ROOT)): pin(p.read_bytes())
           for p in sorted((ROOT / OUT).rglob('*')) if p.is_file()}
outputs[str(REPORT)] = pin(read(REPORT))
emit('review-seal.json', {
    'utc': datetime.now(timezone.utc).isoformat(), 'argv': sys.argv,
    'cwd': str(ROOT), 'python': sys.version,
    'candidate_seal_sha256': '5d8a96a78e1b6d99f7a16f259d36f0e077644f7e7378c8ecd88b58109add66eb',
    'p4a_review_seal_sha256': 'c677fe73b1e28691365e31383780f12b89b797bafe6baa075f6d7ae366bdc4ff',
    'source_sha256': 'f9865e63c55b09bdd36d6b53ae61acf8dfd4f3b340de5d3409e1a7c31204722e',
    'public_inputs_unchanged': len(inputs),
    'disposition': 'Narrow SOURCE sufficiency for selected P3 on exact P4a; no concrete correction-required defect identified.',
    'scope': 'No P1 inspection/adoption, candidate/test/compiler/generator/runtime/ARM/module/native/network/private IO or Git/index/source/helper/central-plan changes. All deferred gates remain open; no next queue.',
    'files': outputs})
print(json.dumps({'inputs_revalidated': len(inputs), 'outputs_sealed': len(outputs),
                  'review_seal': pin(read(OUT / 'review-seal.json'))}))
