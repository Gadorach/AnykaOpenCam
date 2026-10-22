#!/usr/bin/env python3
"""Static source/metadata/hash review only. No builder/generator/candidate import."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
APP = ROOT / 'apps/rtsp-auth/qc07f-watchdog-owner-candidate'
DOC = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-20261022'
OUT = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-review-20261022'
RUN = DOC / 'runs/build04'
NATIVE_NAME = str(ROOT / 'evidence/delegated/20261012-native-handoff/run34')
K = ROOT / 'work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts'
INPUTS = {}
EXPECTED_SOURCE = '2f9e50840a58bbcf8025df1b4e6850278eee0ff5d9a48e0354c531f218b7872a'


def read(path):
    path = Path(path)
    rel = str(path.relative_to(ROOT))
    if path.is_symlink():
        raise ValueError('unexpected final-component symlink: ' + rel)
    raw = path.read_bytes()
    INPUTS[rel] = {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}
    return raw


def load(path):
    return json.loads(read(path))


def verify(path, pin):
    read(path)
    if INPUTS[str(path.relative_to(ROOT))] != {k: pin[k] for k in ('bytes', 'sha256')}:
        raise ValueError('pin mismatch: ' + str(path.relative_to(ROOT)))


def save(name, obj):
    with (OUT / name).open('x') as stream:
        json.dump(obj, stream, indent=2)
        stream.write('\n')


def main():
    seal = load(DOC / 'SOURCE_SEAL01.json')
    for group in ('source_files', 'report_pins', 'evidence_pins'):
        for rel, pin in seal[group].items():
            if Path(rel).is_absolute() or '..' in Path(rel).parts:
                raise ValueError('unexpected relative seal path')
            if not (rel.startswith(str(APP.relative_to(ROOT)) + '/') or
                    rel.startswith(str(DOC.relative_to(ROOT)) + '/')):
                raise ValueError('unexpected sealed public path')
            verify(ROOT / rel, pin)
    actual_source_paths = {str(p.relative_to(ROOT)) for p in APP.iterdir() if p.is_file()}
    if actual_source_paths != set(seal['source_files']) or len(actual_source_paths) != 8:
        raise ValueError('candidate source set mismatch')
    aggregate = hashlib.sha256(b''.join(rel.encode() + b'\0' + bytes.fromhex(pin['sha256'])
                               for rel, pin in sorted(seal['source_files'].items()))).hexdigest()
    if aggregate != EXPECTED_SOURCE or aggregate != seal['source_tree_sha256']:
        raise ValueError('user-supplied source pin mismatch')
    report = load(RUN / 'REPORT.json')
    for rel, pin in report['frozen_prior_pins'].items():
        if 'station-attach-rx' in rel or '/p1' in rel.lower():
            raise ValueError('P1 input outside scope')
        if not (rel.startswith('apps/') or rel.startswith('docs/qualification-plans/')):
            raise ValueError('unexpected prior public path')
        verify(ROOT / rel, pin)
    for rel, pin in seal['retained76_watchdog_source_association'].items():
        verify(ROOT / rel, pin)
        current = (K / 'drivers/watchdog/watchdog_dev.c' if rel.endswith('/drivers/watchdog/watchdog_dev.c')
                   else ROOT / 'bringup/qc07f/variants/security75/mach-ak3918ev200/watchdog.c')
        verify(current, pin)
    for rel, pin in report['source_files'].items():
        verify(RUN / 'source' / rel, pin)
    actual_generated = {str(p.relative_to(RUN / 'source')) for p in (RUN / 'source').rglob('*') if p.is_file()}
    if actual_generated != set(report['source_files']):
        raise ValueError('actual generated-source set mismatch')
    for path in APP.iterdir():
        if path.is_file():
            verify(RUN / 'inputs/new' / path.name, seal['source_files'][str(path.relative_to(ROOT))])
    # Check captured inputs, never original acquisition/private runtime paths.
    # The build retains a public source/header mirror for every consumed path.
    captured = []
    for original, pin in report['inputs'].items():
        if original in report['compiled_source_header_closure']:
            path = RUN / 'inputs/consumed-header-closure' / original.lstrip('/')
        elif original.startswith(NATIVE_NAME + '/inputs/framework/'):
            path = RUN / 'source' / original[len(NATIVE_NAME + '/inputs/'):]
        elif original.startswith(NATIVE_NAME + '/inputs/runtime-license-'):
            path = RUN / 'inputs/licenses' / Path(original).name
        elif original.startswith(NATIVE_NAME + '/inputs/'):
            path = RUN / 'original' / original[len(NATIVE_NAME + '/inputs/'):]
        elif original == NATIVE_NAME + '/manifest.json':
            path = RUN / 'inputs/provenance/run34-manifest.json'
        elif original.startswith(str(APP) + '/'):
            path = RUN / 'inputs/new' / Path(original).name
        else:
            path = RUN / 'inputs/provenance' / Path(original).relative_to(ROOT)
        verify(path, pin)
        captured.append({'recorded_origin': original, 'captured_public_copy': str(path.relative_to(ROOT)), **pin})
    for original, pin in report['compiled_source_header_closure'].items():
        verify(RUN / 'inputs/consumed-header-closure' / original.lstrip('/'), pin)
    manifest = load(RUN / 'inputs/provenance/run34-manifest.json')
    manifest_path = RUN / 'inputs/provenance/run34-manifest.json'
    if INPUTS[str(manifest_path.relative_to(ROOT))]['sha256'] != report['native_run34_manifest_pin']:
        raise ValueError('captured run34 manifest mismatch')
    origin_pins = {row['path'][7:]: row['sha256'] for row in manifest['files'] if row['path'].startswith('inputs/')}
    original_names = sorted(p.name for p in (RUN / 'original').iterdir() if p.is_file())
    changed = []
    patch = []
    for name in original_names:
        before = read(RUN / 'original' / name)
        if hashlib.sha256(before).hexdigest() != origin_pins[name]:
            raise ValueError('public original source differs from captured source manifest')
        after = read(RUN / 'source' / name)
        if before != after:
            changed.append(name)
            patch.extend(difflib.unified_diff(before.decode().splitlines(True), after.decode().splitlines(True),
                         fromfile='a/' + name, tofile='b/' + name))
    if changed != ['native_adapter.c', 'rtspd-native-import.c']:
        raise ValueError('unexpected original source delta')
    added = ('watchdog_owner.c', 'watchdog_owner.h', 'native_watchdog.h',
             'source-close-once.inc', 'native-watchdog.inc')
    for name in added:
        after = read(RUN / 'source' / name)
        if after != read(APP / name):
            raise ValueError('generated include differs from actual candidate')
        patch.extend(difflib.unified_diff([], after.decode().splitlines(True), fromfile='/dev/null', tofile='b/' + name))
    patch = ''.join(patch)
    if patch.encode() != read(RUN / 'applied-source.patch'):
        raise ValueError('actual original/source diff differs from sealed applied patch')
    with (OUT / 'actual-build04-applied-source.patch').open('x') as stream:
        stream.write(patch)
    if len(report['commands']) != 26 or any(c['exit'] != 0 for c in report['commands']):
        raise ValueError('final build metadata not 26 successful commands')
    for row in report['commands']:
        verify(Path(row['log']), row['log_pin'])
    for name, pin in report['artifacts'].items():
        verify(RUN / name, pin)
    for name in ('sensor_baseline.c', 'sc1346-init.h', 'memory_reserve.c', 'memory_runtime.c', 'memory_reader.c'):
        if read(RUN / 'source' / name) != read(RUN / 'original' / name):
            raise ValueError('immutable sensor/memory source changed')
    ranges = {
        APP / 'watchdog_owner.c': [(1, 114)], APP / 'watchdog_owner.h': [(1, 41)],
        APP / 'native-watchdog.inc': [(1, 102)], APP / 'native_watchdog.h': [(1, 18)],
        APP / 'source-close-once.inc': [(1, 15)], APP / 'overlay.py': [(1, 102)],
        RUN / 'source/native_adapter.c': [(1, 193)],
        RUN / 'source/rtspd-native-import.c': [(1, 253)],
        RUN / 'source/memory_runtime.c': [(16, 62)],
        RUN / 'source/sensor_baseline.c': [(24, 55), (66, 104)],
        RUN / 'source/capture-receipt.h': [(12, 30)],
        RUN / 'source/handoff.h': [(7, 25)],
        K / 'drivers/watchdog/watchdog_dev.c': [(248, 319), (368, 391), (784, 825), (854, 919), (932, 988)],
        ROOT / 'bringup/qc07f/variants/security75/mach-ak3918ev200/watchdog.c': [(39, 96)],
        K / 'fs/file.c': [(682, 708), (842, 860)], K / 'fs/open.c': [(1558, 1602)],
        K / 'fs/file_table.c': [(479, 514), (602, 624)],
        ROOT / 'docs/plan/camera-foundation/08-STREAMING.md': [(16, 44), (66, 73)],
    }
    excerpts = []
    for path, spans in ranges.items():
        lines = read(path).splitlines(keepends=True)
        for start, end in spans:
            raw = b''.join(lines[start - 1:end])
            excerpts.append({'path': str(path.relative_to(ROOT)), 'start_line': start, 'end_line': end,
                             'sha256': hashlib.sha256(raw).hexdigest(), 'text': raw.decode()})
    save('public-excerpts.json', excerpts)
    save('captured-public-input-map.json', captured)
    save('verification.json', {
        'source_tree_sha256': aggregate, 'source_files_verified': len(seal['source_files']),
        'sealed_reports_verified': len(seal['report_pins']), 'sealed_evidence_files_verified': len(seal['evidence_pins']),
        'frozen_prior_pins_verified': len(report['frozen_prior_pins']),
        'captured_public_input_paths_verified': len(captured),
        'consumed_public_source_header_copies_verified': len(report['compiled_source_header_closure']),
        'generated_source_files_verified': len(report['source_files']),
        'actual_applied_diff_matches': True, 'changed_originals': changed, 'added_includes': added,
        'original_acquisition_runtime_inputs_opened': False,
        'author_compile_commands': report['commands'], 'author_artifacts': report['artifacts'],
        'author_status': report['status'], 'reviewer_reran_build_generator_or_tests': False,
        'current_external_toolchain_or_headers_rehashed': False,
        'source_pin_method': seal['hash_method'],
    })
    read(Path(__file__))
    save('reader-receipt.json', {'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'scope': 'public static source/diff/API/hash only; no candidate/generator/build/test execution, target/UART/RPC/status/network/private IO, P1 inspection or old-source/Git edits',
        'inputs': INPUTS})
    print(json.dumps({'source_files': len(seal['source_files']), 'sealed_evidence': len(seal['evidence_pins']),
                      'captured_inputs': len(captured), 'consumed_copies': len(report['compiled_source_header_closure']),
                      'generated': len(report['source_files']), 'prior_pins': len(report['frozen_prior_pins']),
                      'source_tree_sha256': aggregate, 'actual_diff_matches': True}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments or optimized Python')
    main()
