#!/usr/bin/env python3
"""Focused P2 v2 public-source/diff/hash reader. No candidate execution or IO."""
import datetime
import difflib
import hashlib
import json
import posixpath
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
DOC = ROOT / 'docs/qualification-plans/kernel76-transport-20261021'
OUT = DOC / 'station-attach-init-p2-v2-review01'
AUTHOR = DOC / 'station-attach-init-p2-v2-source01'
CAND = ROOT / 'apps/qualification/kernel76-station-attach-init-v2'
OLD = ROOT / 'apps/qualification/kernel76-station-attach-init/source'
KERNEL = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux'
INPUTS = {}
SEAL = '40928114e2de10bc84b0dc26301db2563f1d9a1df4273b6610a7cc7ba13317bb'


def read(path):
    rel = str(path.relative_to(ROOT))
    if path.is_symlink() or any(p.is_symlink() for p in path.parents if p != ROOT):
        raise ValueError('symlink input: ' + rel)
    raw = path.read_bytes()
    INPUTS[rel] = {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}
    return raw


def load(path):
    return json.loads(read(path))


def verify(rel, expected):
    path = Path(rel)
    if path.is_absolute() or '..' in path.parts:
        raise ValueError('unexpected relative path')
    read(ROOT / path)
    if INPUTS[str(path)] != expected:
        raise ValueError('pin mismatch: ' + str(path))


def save(name, value):
    with (OUT / name).open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def main():
    manifest = load(AUTHOR / 'candidate-seal.json')
    if INPUTS[str((AUTHOR / 'candidate-seal.json').relative_to(ROOT))]['sha256'] != SEAL:
        raise ValueError('supplied v2 seal mismatch')
    allowed = (str(CAND.relative_to(ROOT)) + '/', str(AUTHOR.relative_to(ROOT)) + '/')
    candidate_report = str((DOC / 'STATION76_ATTACH_INIT_P2_V2_CANDIDATE.md').relative_to(ROOT))
    for rel, expected in manifest['files'].items():
        if not (rel.startswith(allowed) or rel == candidate_report):
            raise ValueError('unexpected v2 public path')
        verify(rel, expected)
    actual_sealed_set = {str(p.relative_to(ROOT)) for tree in (CAND, AUTHOR)
                         for p in tree.rglob('*') if p.is_file() and p.name != 'candidate-seal.json'}
    actual_sealed_set.add(candidate_report)
    if actual_sealed_set != set(manifest['files']):
        raise ValueError('v2 sealed path set differs')
    baseline = load(AUTHOR / 'copy-baseline.json')
    frozen_allowed = ('apps/qualification/kernel76-station-attach-init/',
                      str(DOC.relative_to(ROOT)) + '/station-attach-init-p2-source01/',
                      str(DOC.relative_to(ROOT)) + '/station-attach-init-p2-review01/')
    frozen_reports = {str((DOC / name).relative_to(ROOT)) for name in (
        'STATION76_ATTACH_INIT_P2_CANDIDATE.md', 'STATION76_ATTACH_INIT_P2_REVIEW.md')}
    for rel, expected in baseline['frozen_custody'].items():
        if not (rel.startswith(frozen_allowed) or rel in frozen_reports):
            raise ValueError('unexpected frozen public path')
        verify(rel, expected)
    v1_seal = load(DOC / 'station-attach-init-p2-source01/candidate-seal.json')
    if INPUTS[str((DOC / 'station-attach-init-p2-source01/candidate-seal.json').relative_to(ROOT))]['sha256'] != 'c54ff57f63bec1180199a97321fbf1ea38e9b448edf0969c9135953c565b1919':
        raise ValueError('v1 seal changed')
    prefix = str(OLD.relative_to(ROOT)) + '/'
    old_sources = {k[len(prefix):]: v for k, v in v1_seal['files'].items() if k.startswith(prefix)}
    if old_sources != baseline['copied_source']:
        raise ValueError('byte-copy baseline differs from v1 source seal')
    accepted_roots = ('apps/qualification/kernel76-station/source/',
                      'apps/qualification/kernel76-station-ingress/source/',
                      'apps/qualification/kernel76-station-events/source/')
    accepted_count = 0
    for name, accepted in baseline['accepted'].items():
        verify(str((DOC / name / 'candidate-seal.json').relative_to(ROOT)), accepted['seal'])
        accepted_seal = load(DOC / name / 'candidate-seal.json')
        for rel, expected in accepted['source'].items():
            if not rel.startswith(accepted_roots) or accepted_seal['files'].get(rel) != expected:
                raise ValueError('unexpected accepted public source')
            verify(rel, expected)
            accepted_count += 1
    inventory = load(AUTHOR / 'source-inventory.json')
    source_map = {str(p.relative_to(CAND / 'source')): None for p in (CAND / 'source').rglob('*') if p.is_file()}
    for rel in source_map:
        path = CAND / 'source' / rel
        read(path)
        source_map[rel] = INPUTS[str(path.relative_to(ROOT))]
    if source_map != inventory['source'] or set(source_map) != set(old_sources):
        raise ValueError('source-set/pin mismatch')
    aggregate = hashlib.sha256(json.dumps(source_map, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if aggregate != inventory['source_sha256'] or aggregate != manifest['source_sha256']:
        raise ValueError('aggregate mismatch')
    changed = sorted(rel for rel in source_map if source_map[rel] != old_sources[rel])
    if changed != ['attach_init.h', 'hgic_smac/attach_init.c', 'hgic_smac/attach_status.c', 'utils/fwctrl.c']:
        raise ValueError('outside R1-R3 source diff')
    delta = ''.join(line for rel in changed for line in difflib.unified_diff(
        read(OLD / rel).decode().splitlines(keepends=True),
        read(CAND / 'source' / rel).decode().splitlines(keepends=True),
        fromfile='p2-v1/' + rel, tofile='p2-v2/' + rel))
    if delta.encode() != read(AUTHOR / 'R1_R2_R3_DELTA.patch'):
        raise ValueError('author delta differs from independently derived actual diff')
    delta_path = OUT / 'actual-v1-v2-diff.patch'
    if delta_path.exists():
        if delta_path.read_bytes() != delta.encode():
            raise ValueError('preserved partial diff differs; refuse overwrite')
    else:
        with delta_path.open('x') as stream:
            stream.write(delta)
    anchors = load(AUTHOR / 'kernel-anchors.json')
    for rel, expected in anchors.items():
        if not rel.startswith(str(KERNEL.relative_to(ROOT)) + '/'):
            raise ValueError('unexpected kernel public input')
        verify(rel, {k: expected[k] for k in ('bytes', 'sha256')})
        lines = read(ROOT / rel).splitlines(keepends=True)
        for excerpt in expected['excerpts']:
            raw = b''.join(lines[excerpt['start_line'] - 1:excerpt['end_line']])
            if raw.decode() != excerpt['text'] or hashlib.sha256(raw).hexdigest() != excerpt['sha256']:
                raise ValueError('kernel excerpt mismatch')
    syntax = load(AUTHOR / 'syntax01/receipt.json')
    if syntax['source'] != {str((CAND / 'source' / k).relative_to(ROOT)): v for k, v in source_map.items()}:
        raise ValueError('syntax is not final source')
    expected_units = [str(CAND / 'source' / rel) for rel in (
        'hgic_smac/attach_init.c', 'hgic_smac/attach_status.c', 'utils/fwctrl.c')]
    if [c['argv'][-1] for c in syntax['commands']] != expected_units:
        raise ValueError('unexpected author syntax units')
    for c in syntax['commands']:
        if c['returncode'] != 0 or '-fsyntax-only' not in c['argv']:
            raise ValueError('author syntax is not recorded successful syntax-only')
    receipt_header_rows = [v for k, v in syntax['header_dependencies'].items()
                           if posixpath.normpath(k) == str(CAND / 'source/attach_init.h')]
    if not receipt_header_rows or any(v != source_map['attach_init.h'] for v in receipt_header_rows):
        raise ValueError('changed receipt header missing/mismatched in author dependency pins')
    summary = {
        'candidate_seal_sha256': SEAL, 'v2_sealed_files_verified': len(manifest['files']),
        'v2_source_files_verified': len(source_map), 'source_sha256': aggregate,
        'changed': {rel: {'v1': old_sources[rel], 'v2': source_map[rel]} for rel in changed},
        'unchanged_v1': len(source_map) - len(changed),
        'actual_diff_byte_matches_author_patch': True,
        'frozen_v1_review_custody_verified': len(baseline['frozen_custody']),
        'accepted_source_pins_verified': accepted_count,
        'kernel_fullfile_and_excerpt_pins_verified': len(anchors),
        'license': source_map['LICENSE'],
        'author_syntax': {'scope': syntax['scope'], 'commands': syntax['commands'],
                          'compiler': syntax['compiler'], 'kernel_inputs': syntax['kernel_inputs'],
                          'same_final_source': True, 'changed_header_dependency_matches': True,
                          'reviewer_reran_compilation': False,
                          'current_external_compiler_headers_rehashed': False},
    }
    save('public-pin-verification.json', summary)
    ranges = {
        CAND / 'source/attach_init.h': [(14, 65)],
        CAND / 'source/hgic_smac/attach_init.c': [(31, 71), (207, 266), (328, 468), (559, 608), (610, 659)],
        CAND / 'source/hgic_smac/attach_status.c': [(10, 38)],
        CAND / 'source/utils/fwctrl.c': [(84, 148), (188, 267), (313, 371), (455, 541)],
        CAND / 'source/utils/control_receipt.h': [(49, 96)],
        CAND / 'source/hgic_smac/Makefile': [(1, 27)],
        KERNEL / 'net/mac80211/main.c': [(792, 833), (1595, 1636)],
        KERNEL / 'include/net/mac80211.h': [(4763, 4777)],
        KERNEL / 'net/core/dev.c': [(11401, 11430), (12280, 12376)],
        KERNEL / 'net/mac80211/iface.c': [(2376, 2407)],
        DOC / 'STATION76_ATTACH_REMAINING_SOURCE_PLAN.md': [(150, 223), (225, 266)],
    }
    excerpts = []
    for path, spans in ranges.items():
        lines = read(path).splitlines(keepends=True)
        for start, end in spans:
            raw = b''.join(lines[start - 1:end])
            excerpts.append({'path': str(path.relative_to(ROOT)), 'start_line': start,
                             'end_line': end, 'text': raw.decode(), 'sha256': hashlib.sha256(raw).hexdigest()})
    save('public-excerpts.json', excerpts)
    read(Path(__file__))
    save('reader-receipt.json', {
        'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'argv': sys.argv,
        'python': sys.version, 'cwd': str(ROOT),
        'scope': 'public static SOURCE/diff/provenance/hash only; no compiler, runtime/tests, P1 review, Git/index, target/UART/RPC/network/private IO',
        'inputs': INPUTS,
    })
    print(json.dumps({k: v for k, v in summary.items() if k not in ('changed', 'author_syntax')}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments or optimized Python')
    main()
