#!/usr/bin/env python3
"""Read/hash public P2 review inputs only; no compiler, tests, device or Git IO."""
import datetime
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
DOC = ROOT / 'docs/qualification-plans/kernel76-transport-20261021'
OUT = DOC / 'station-attach-init-p2-review01'
CAND = ROOT / 'apps/qualification/kernel76-station-attach-init'
AUTHOR = DOC / 'station-attach-init-p2-source01'
KERNEL = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux'
EXPECTED_SEAL = 'c54ff57f63bec1180199a97321fbf1ea38e9b448edf0969c9135953c565b1919'
INPUTS = {}


def read(path):
    path = Path(path)
    rel = str(path.relative_to(ROOT))
    if path.is_symlink() or any(p.is_symlink() for p in path.parents if p != ROOT):
        raise ValueError('symlink input: ' + rel)
    data = path.read_bytes()
    INPUTS[rel] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    return data


def load(path):
    return json.loads(read(path))


def verify(path, expected):
    read(path)
    if INPUTS[str(path.relative_to(ROOT))] != expected:
        raise ValueError('pin mismatch: ' + str(path.relative_to(ROOT)))


def save(name, obj):
    with (OUT / name).open('x') as stream:
        json.dump(obj, stream, indent=2)
        stream.write('\n')


def main():
    seal = load(AUTHOR / 'candidate-seal.json')
    if INPUTS[str((AUTHOR / 'candidate-seal.json').relative_to(ROOT))]['sha256'] != EXPECTED_SEAL:
        raise ValueError('candidate seal differs from user pin')
    allowed = (str(CAND.relative_to(ROOT)) + '/', str(AUTHOR.relative_to(ROOT)) + '/')
    report = str((DOC / 'STATION76_ATTACH_INIT_P2_CANDIDATE.md').relative_to(ROOT))
    for rel, expected in seal['files'].items():
        if '..' in Path(rel).parts or not (rel.startswith(allowed) or rel == report):
            raise ValueError('non-public/non-owned seal path: ' + rel)
        verify(ROOT / rel, expected)
    inventory = load(AUTHOR / 'source-inventory.json')
    actual = {str(p.relative_to(CAND / 'source')) for p in (CAND / 'source').rglob('*') if p.is_file()}
    if actual != set(inventory['source']):
        raise ValueError('source path-set mismatch')
    for rel, expected in inventory['source'].items():
        verify(CAND / 'source' / rel, expected)
    aggregate = hashlib.sha256(json.dumps(inventory['source'], sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if aggregate != inventory['source_sha256'] or aggregate != seal['source_sha256']:
        raise ValueError('source aggregate mismatch')
    baseline = load(AUTHOR / 'copy-baseline.json')
    accepted_count = 0
    accepted_roots = ('apps/qualification/kernel76-station/source/',
                      'apps/qualification/kernel76-station-ingress/source/',
                      'apps/qualification/kernel76-station-events/source/')
    for name, accepted in baseline['accepted'].items():
        verify(DOC / name / 'candidate-seal.json', accepted['seal'])
        old_seal = load(DOC / name / 'candidate-seal.json')
        for rel, expected in accepted['source'].items():
            if not rel.startswith(accepted_roots) or '..' in Path(rel).parts:
                raise ValueError('unexpected accepted source path')
            if old_seal['files'].get(rel) != expected:
                raise ValueError('accepted source/seal mismatch')
            verify(ROOT / rel, expected)
            accepted_count += 1
    for rel, expected in inventory['provenance_chain'].items():
        if not (rel.endswith('/copy-baseline.json') or rel.endswith('/prepare_source.py')):
            raise ValueError('unexpected provenance path')
        verify(ROOT / rel, expected)
    for rel, expected in baseline['inputs'].items():
        verify(DOC / rel, expected)
    changed = sorted(rel for rel, expected in baseline['baseline'].items()
                     if inventory['source'][rel] != expected)
    added = sorted(actual - set(baseline['baseline']))
    unchanged = len(baseline['baseline']) - len(changed)
    syntax = load(AUTHOR / 'syntax03/receipt.json')
    syntax_source = {str((CAND / 'source' / rel).relative_to(ROOT)): pin
                     for rel, pin in inventory['source'].items()}
    if syntax['source'] != syntax_source:
        raise ValueError('final syntax receipt not same source')
    summary = {
        'candidate_sealed_files_verified': len(seal['files']),
        'accepted_source_pins_verified': accepted_count,
        'candidate_source_files_verified': len(actual), 'source_sha256': aggregate,
        'unchanged_imports': unchanged, 'changed_imports': changed, 'added': added,
        'license': inventory['source']['LICENSE'],
        'author_syntax03': {
            'scope': syntax['scope'], 'pass_recorded': syntax['pass'],
            'commands': [{'argv': c['argv'], 'returncode': c['returncode'], 'log': c['log']}
                         for c in syntax['commands']],
            'all_fsyntax_only': all('-fsyntax-only' in c['argv'] for c in syntax['commands']),
            'compiler': syntax['compiler'], 'kernel_inputs': syntax['kernel_inputs'],
            'same_final_source': True,
            'current_compiler_or_external_headers_rehashed': False,
            'reviewer_reran_compilation_or_behavior': False,
        },
    }
    save('public-pin-verification.json', summary)
    ranges = {
        CAND / 'source/hgic_smac/attach_init.c': [(31, 89), (97, 248), (250, 325), (328, 432), (434, 553), (555, 616)],
        CAND / 'source/hgic_smac/attach_status.c': [(1, 61)],
        CAND / 'source/attach_init.h': [(1, 58)],
        CAND / 'source/hgic_smac/Makefile': [(1, 27)],
        CAND / 'source/Makefile': [(30, 50), (66, 82)],
        CAND / 'source/hgic_smac/hgics.h': [(61, 96), (159, 162)],
        CAND / 'source/hgic_def.h': [(170, 195)],
        CAND / 'source/utils/fwctrl.c': [(52, 148), (188, 258), (305, 421), (447, 533), (1283, 1292)],
        CAND / 'source/utils/fwctrl.h': [(10, 47), (60, 75)],
        CAND / 'source/utils/if_usb.c': [(74, 155), (184, 279), (281, 382), (384, 555)],
        CAND / 'source/hgic_smac/11bgn.c': [(111, 195)],
        CAND / 'source/utils/control_receipt.h': [(27, 96)],
        KERNEL / 'net/mac80211/main.c': [(792, 833), (1595, 1636)],
        KERNEL / 'include/net/mac80211.h': [(4763, 4781)],
        KERNEL / 'net/core/dev.c': [(1682, 1708), (11401, 11420), (12335, 12370)],
        KERNEL / 'net/mac80211/iface.c': [(2376, 2390)],
        DOC / 'STATION76_ATTACH_REMAINING_SOURCE_PLAN.md': [(150, 223)],
    }
    excerpts = []
    for path, spans in ranges.items():
        lines = read(path).splitlines(keepends=True)
        for start, end in spans:
            chunk = b''.join(lines[start - 1:end])
            excerpts.append({'path': str(path.relative_to(ROOT)), 'start_line': start,
                             'end_line': end, 'sha256': hashlib.sha256(chunk).hexdigest(),
                             'text': chunk.decode()})
    save('public-excerpts.json', excerpts)
    read(Path(__file__))
    save('reader-receipt.json', {
        'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'scope': 'public SOURCE/document/seal reads and SHA256 only; no candidate execution, compilation, tests, Git, device or network IO',
        'inputs': INPUTS,
    })
    print(json.dumps({k: v for k, v in summary.items() if k != 'author_syntax03'}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments/optimized Python')
    main()
