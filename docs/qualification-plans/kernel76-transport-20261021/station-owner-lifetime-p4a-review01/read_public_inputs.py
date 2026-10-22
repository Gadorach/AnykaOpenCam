#!/usr/bin/env python3
"""P4a static public source/diff/API custody reader; no candidate execution."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
DOC = ROOT / 'docs/qualification-plans/kernel76-transport-20261021'
OUT = DOC / 'station-owner-lifetime-p4a-review01'
AUTHOR = DOC / 'station-owner-lifetime-p4a-source01'
CAND = ROOT / 'apps/qualification/kernel76-station-owner-lifetime'
OLD = ROOT / 'apps/qualification/kernel76-station-attach-init-v2/source'
K = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux'
SEAL = '49e1c0e6209371e4714e4fb1ff5adad4d8cb042014fe4508f2e5257e37f49490'
INPUTS = {}


def read(path):
    rel = str(path.relative_to(ROOT))
    if path.is_symlink() or any(p.is_symlink() for p in path.parents if p != ROOT):
        raise ValueError('unexpected symlink: ' + rel)
    raw = path.read_bytes()
    INPUTS[rel] = {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}
    return raw


def load(path):
    return json.loads(read(path))


def verify(rel, expected):
    if Path(rel).is_absolute() or '..' in Path(rel).parts:
        raise ValueError('unexpected relative input')
    read(ROOT / rel)
    if INPUTS[rel] != expected:
        raise ValueError('input pin mismatch: ' + rel)


def save(name, data):
    path = OUT / name
    raw = (json.dumps(data, indent=2) + '\n').encode()
    if path.exists():
        if path.read_bytes() != raw:
            raise ValueError('preserved partial output differs: ' + name)
        return
    with path.open('xb') as stream:
        stream.write(raw)


def text(path):
    # Match the author's explicitly universal-newline text diff; raw hashes
    # independently retain exact source bytes, including inherited CRLF.
    return read(path).decode().replace('\r\n', '\n').replace('\r', '\n')


def main():
    sealpath = AUTHOR / 'candidate-seal.json'
    seal = load(sealpath)
    if INPUTS[str(sealpath.relative_to(ROOT))]['sha256'] != SEAL:
        raise ValueError('supplied seal mismatch')
    public_report = str((DOC / 'STATION76_OWNER_LIFETIME_P4A_CANDIDATE.md').relative_to(ROOT))
    allowed = (str(CAND.relative_to(ROOT)) + '/', str(AUTHOR.relative_to(ROOT)) + '/')
    for rel, pin in seal['files'].items():
        if not (rel.startswith(allowed) or rel == public_report):
            raise ValueError('unexpected candidate public path')
        verify(rel, pin)
    actual = {str(p.relative_to(ROOT)) for tree in (CAND, AUTHOR) for p in tree.rglob('*')
              if p.is_file() and p.name != 'candidate-seal.json'} | {public_report}
    if actual != set(seal['files']):
        raise ValueError('candidate sealed path set mismatch')
    baseline = load(AUTHOR / 'copy-baseline.json')
    prior_union = {}
    prior_seals = [('station-attach-init-p2-source01', 'candidate-seal.json'),
                   ('station-attach-init-p2-review01', 'review-seal.json'),
                   ('station-attach-init-p2-v2-source01', 'candidate-seal.json'),
                   ('station-attach-init-p2-v2-review01', 'review-seal.json'),
                   ('station-attach-tx-p3-source01', 'candidate-seal.json')]
    for directory, name in prior_seals:
        path = DOC / directory / name
        manifest = load(path)
        prior_union[str(path.relative_to(ROOT))] = INPUTS[str(path.relative_to(ROOT))]
        prior_union.update(manifest['files'])
    if prior_union != baseline['frozen_custody']:
        raise ValueError('prior custody does not equal exact five-seal union')
    for rel, pin in prior_union.items():
        if 'station-attach-rx' in rel or '/p1' in rel.lower():
            raise ValueError('P1 input not in scope')
        if not (rel.startswith('apps/qualification/kernel76-station-attach-') or
                rel.startswith(str(DOC.relative_to(ROOT)) + '/')):
            raise ValueError('unexpected frozen public path')
        verify(rel, pin)
    old_seal = load(DOC / 'station-attach-init-p2-v2-source01/candidate-seal.json')
    prefix = str(OLD.relative_to(ROOT)) + '/'
    old_source = {rel[len(prefix):]: pin for rel, pin in old_seal['files'].items() if rel.startswith(prefix)}
    if old_source != baseline['baseline'] or len(old_source) != 71:
        raise ValueError('baseline differs from exact P2v2 sealed source')
    accepted_count = 0
    accepted_roots = ('apps/qualification/kernel76-station/source/',
                      'apps/qualification/kernel76-station-ingress/source/',
                      'apps/qualification/kernel76-station-events/source/')
    for name, accepted in baseline['accepted'].items():
        verify(str((DOC / name / 'candidate-seal.json').relative_to(ROOT)), accepted['seal'])
        manifest = load(DOC / name / 'candidate-seal.json')
        for rel, pin in accepted['source'].items():
            if not rel.startswith(accepted_roots) or manifest['files'].get(rel) != pin:
                raise ValueError('unexpected accepted source input')
            verify(rel, pin)
            accepted_count += 1
    inventory = load(AUTHOR / 'source-inventory.json')
    source = {}
    for path in sorted((CAND / 'source').rglob('*')):
        if path.is_file():
            read(path)
            source[str(path.relative_to(CAND / 'source'))] = INPUTS[str(path.relative_to(ROOT))]
    if source != inventory['source']:
        raise ValueError('source set/pin mismatch')
    changed = sorted(rel for rel, pin in old_source.items() if source.get(rel) != pin)
    added = sorted(set(source) - set(old_source))
    if changed != ['hgic_def.h', 'hgic_smac/Makefile', 'hgic_smac/attach_init.c',
                   'hgic_smac/attach_status.c', 'utils/fwctrl.c', 'utils/fwctrl.h'] or added != ['utils/owner_usb.c']:
        raise ValueError('unexpected source delta')
    delta = []
    for rel in changed:
        delta.extend(difflib.unified_diff(text(OLD / rel).splitlines(keepends=True),
                     text(CAND / 'source' / rel).splitlines(keepends=True),
                     fromfile='p2-v2/' + rel, tofile='p4a/' + rel))
    for rel in added:
        delta.extend(difflib.unified_diff([], text(CAND / 'source' / rel).splitlines(keepends=True),
                     fromfile='/dev/null', tofile='p4a/' + rel))
    delta = ''.join(delta)
    if delta.encode() != read(AUTHOR / 'P4A_SOURCE_DELTA.patch'):
        raise ValueError('actual normalized diff differs from author patch')
    delta_path = OUT / 'actual-p2v2-p4a-diff.patch'
    if delta_path.exists():
        if delta_path.read_bytes() != delta.encode():
            raise ValueError('preserved partial actual diff differs')
    else:
        with delta_path.open('x') as stream:
            stream.write(delta)
    aggregate = hashlib.sha256(json.dumps(source, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    if aggregate != seal['source_sha256'] or aggregate != inventory['source_sha256']:
        raise ValueError('source aggregate mismatch')
    contracts_count = 0
    for name in ('kernel-owner-contracts.json', 'module-return-contracts.json'):
        for rel, expected in load(AUTHOR / name).items():
            if not (rel.startswith(str(K.relative_to(ROOT)) + '/') or rel.startswith(prefix)):
                raise ValueError('unexpected public API input')
            verify(rel, {k: expected[k] for k in ('bytes', 'sha256')})
            lines = read(ROOT / rel).splitlines(keepends=True)
            for row in expected['excerpts']:
                raw = b''.join(lines[row['start_line'] - 1:row['end_line']])
                if raw.decode() != row['text'] or hashlib.sha256(raw).hexdigest() != row['sha256']:
                    raise ValueError('API excerpt mismatch')
            contracts_count += 1
    syntax = load(AUTHOR / 'syntax02/receipt.json')
    if syntax['source'] != {str((CAND / 'source' / rel).relative_to(ROOT)): pin for rel, pin in source.items()}:
        raise ValueError('final syntax source mismatch')
    if len(syntax['commands']) != 4 or any(c['returncode'] != 0 or '-fsyntax-only' not in c['argv'] for c in syntax['commands']):
        raise ValueError('author final syntax not four successful syntax-only commands')
    ranges = {
        CAND / 'source/utils/owner_usb.c': [(1, 514)],
        CAND / 'source/hgic_smac/attach_init.c': [(268, 335), (355, 501), (503, 591), (645, 706)],
        CAND / 'source/utils/fwctrl.c': [(84, 148), (188, 267), (313, 476)],
        CAND / 'source/utils/fwctrl.h': [(10, 75)],
        CAND / 'source/hgic_def.h': [(171, 201)],
        CAND / 'source/hgic_smac/Makefile': [(1, 28)],
        CAND / 'source/hgic_smac/attach_status.c': [(10, 65)],
        CAND / 'source/hgic_smac/ap.c': [(24, 48), (77, 107)],
        K / 'drivers/usb/core/urb.c': [(677, 779)],
        K / 'drivers/usb/core/hcd.c': [(1115, 1162), (1519, 1569), (1634, 1676)],
        K / 'include/linux/usb.h': [(1734, 1748)],
        K / 'drivers/usb/musb/musb_host.c': [(285, 305), (2136, 2172)],
        K / 'kernel/workqueue.c': [(4463, 4485)],
        K / 'drivers/usb/core/driver.c': [(319, 327), (399, 427), (431, 461), (1104, 1124)],
        K / 'drivers/base/driver.c': [(193, 203), (264, 279)],
        K / 'drivers/base/bus.c': [(258, 277), (448, 465), (651, 655), (780, 797)],
        K / 'drivers/base/dd.c': [(443, 480), (622, 640), (718, 775), (1034, 1042), (1080, 1111), (1170, 1208), (1318, 1379), (1415, 1442)],
        K / 'lib/klist.c': [(234, 259), (304, 318), (368, 407)],
        K / 'kernel/module/main.c': [(810, 850)],
        K / 'fs/sysfs/file.c': [(503, 518)],
        K / 'fs/kernfs/dir.c': [(482, 528), (1472, 1512), (1693, 1738)],
        K / 'fs/proc/generic.c': [(790, 817)],
        K / 'fs/proc/inode.c': [(255, 273)],
        K / 'net/core/dev.c': [(2002, 2037), (2220, 2244)],
        DOC / 'STATION76_ATTACH_REMAINING_SOURCE_PLAN.md': [(225, 266), (281, 355)],
    }
    excerpts = []
    for path, spans in ranges.items():
        lines = read(path).splitlines(keepends=True)
        for start, end in spans:
            raw = b''.join(lines[start - 1:end])
            excerpts.append({'path': str(path.relative_to(ROOT)), 'start_line': start,
                             'end_line': end, 'sha256': hashlib.sha256(raw).hexdigest(), 'text': raw.decode()})
    save('public-excerpts.json', excerpts)
    summary = {
        'candidate_seal_sha256': SEAL, 'candidate_sealed_files_verified': len(seal['files']),
        'source_files': len(source), 'source_sha256': aggregate,
        'changed': {rel: source[rel] for rel in changed}, 'added': {rel: source[rel] for rel in added},
        'unchanged_p2v2': len(old_source) - len(changed),
        'actual_universal_newline_text_diff_matches_author': True,
        'frozen_custody_exact_five_seal_union_verified': len(prior_union),
        'accepted_source_pins_verified': accepted_count, 'author_contract_fullfile_excerpt_entries_verified': contracts_count,
        'legacy_if_usb_contracts': 'hash/provenance only; not selected active transport',
        'license': source['LICENSE'],
        'author_syntax02': {'scope': syntax['scope'], 'commands': syntax['commands'],
                            'compiler': syntax['compiler'], 'host_kernel_inputs': syntax['host_kernel_inputs'],
                            'same_final_source': True, 'reviewer_reran': False,
                            'external_compiler_headers_rehashed': False},
    }
    save('public-pin-verification.json', summary)
    read(Path(__file__))
    save('reader-receipt.json', {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'scope': 'public static source/API/diff/hash only; no compiler, runtime/tests, P1 review, target/UART/RPC/status/network/private IO or Git/index/source/helper edits',
        'inputs': INPUTS})
    print(json.dumps({k: v for k, v in summary.items() if k not in ('changed', 'added', 'author_syntax02')}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments or optimized Python')
    main()
