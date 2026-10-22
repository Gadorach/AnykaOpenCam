#!/usr/bin/env python3
"""Static W1-W3 source/diff/dependency/object custody only; no child execution."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import posixpath
import shlex
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
APP = ROOT / 'apps/rtsp-auth/qc07f-watchdog-owner-v2'
DOC = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-v2-20261022'
OUT = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-v2-review-20261022'
RUN = DOC / 'runs/build01'
OLD_DOC = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-20261022'
OLD = OLD_DOC / 'runs/build04'
NATIVE_NAME = str(ROOT / 'evidence/delegated/20261012-native-handoff/run34')
SOURCE_PIN = 'e511920a5c4cc53eac99d4d56e1b50e547c726764b790dbc881aa16ef94c7d8b'
INPUTS = {}


def read(path):
    path = Path(path)
    rel = str(path.relative_to(ROOT))
    if path.is_symlink():
        raise ValueError('unexpected symlink: ' + rel)
    raw = path.read_bytes()
    INPUTS[rel] = {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}
    return raw


def load(path):
    return json.loads(read(path))


def verify(path, expected):
    read(path)
    if INPUTS[str(path.relative_to(ROOT))] != {k: expected[k] for k in ('bytes', 'sha256')}:
        raise ValueError('pin mismatch: ' + str(path.relative_to(ROOT)))


def save(name, obj):
    with (OUT / name).open('x') as stream:
        json.dump(obj, stream, indent=2)
        stream.write('\n')


def text(path):
    return read(path).decode().replace('\r\n', '\n').replace('\r', '\n')


def diff(old, new, left, right):
    return ''.join(difflib.unified_diff(text(old).splitlines(True), text(new).splitlines(True),
                                      fromfile=left, tofile=right))


def archive_members(path):
    raw = read(path)
    if raw[:8] != b'!<arch>\n':
        raise ValueError('not normal archive')
    offset, names, members = 8, b'', {}
    while offset < len(raw):
        header = raw[offset:offset + 60]
        if len(header) != 60 or header[58:60] != b'`\n':
            raise ValueError('invalid archive header')
        size = int(header[48:58].decode().strip())
        body = raw[offset + 60:offset + 60 + size]
        if len(body) != size:
            raise ValueError('short archive member')
        label = header[:16].decode().strip()
        if label == '//':
            names = body
        elif label not in ('/', '/SYM64/'):
            if label.startswith('/') and label[1:].isdigit():
                name = names[int(label[1:]):].split(b'\n', 1)[0].decode().rstrip('/')
            else:
                name = label.rstrip('/')
            if name in members:
                raise ValueError('duplicate archive member')
            members[name] = body
        offset += 60 + size + size % 2
    if offset != len(raw):
        raise ValueError('archive extent mismatch')
    return members


def main():
    seal = load(DOC / 'SOURCE_SEAL01.json')
    buildseal = load(DOC / 'BUILD_SEAL01.json')
    for group in ('source_files', 'report_pins', 'evidence_pins'):
        for rel, pin in seal[group].items():
            if Path(rel).is_absolute() or '..' in Path(rel).parts or not (
                rel.startswith(str(APP.relative_to(ROOT)) + '/') or rel.startswith(str(DOC.relative_to(ROOT)) + '/')):
                raise ValueError('unexpected new candidate public path')
            verify(ROOT / rel, pin)
    actual_app = {str(p.relative_to(ROOT)) for p in APP.iterdir() if p.is_file()}
    if actual_app != set(seal['source_files']) or len(actual_app) != 8:
        raise ValueError('source path set mismatch')
    aggregate = hashlib.sha256(b''.join(k.encode() + b'\0' + bytes.fromhex(v['sha256'])
                        for k, v in sorted(seal['source_files'].items()))).hexdigest()
    if aggregate != SOURCE_PIN or aggregate != seal['source_tree_sha256']:
        raise ValueError('user source pin mismatch')
    baseline = load(DOC / 'BASELINE_CUSTODY01.json')
    if baseline['prior_custody_pins'] != seal['prior_custody_pins']:
        raise ValueError('baseline/seal prior custody differs')
    for rel, pin in baseline['prior_custody_pins'].items():
        if Path(rel).is_absolute() or '..' in Path(rel).parts or 'station-attach-rx' in rel or '/p1' in rel.lower():
            raise ValueError('unexpected/P1 prior input')
        verify(ROOT / rel, pin)
    verify(OLD_DOC / 'SOURCE_SEAL01.json', baseline['old_source_seal'])
    old_review = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-review-20261022/REVIEW_SEAL01.json'
    verify(old_review, baseline['review_seal'])
    if baseline['old_source_seal']['sha256'] != '96bc8a72e628c1f55daa75e49fcbe92622f8686d04faf06f13a2bccb29d87e75' or baseline['review_seal']['sha256'] != '1edf97af9a4d337e55ec5527d2e4ff7bdccc3989f136512fc8c16b301b54c727':
        raise ValueError('known v1/review seal changed')
    report = load(RUN / 'REPORT.json')
    old = load(OLD / 'REPORT.json')
    if report['compiled_units_per_platform'] != ['native_adapter', 'watchdog_owner']:
        raise ValueError('unexpected changed-unit compilation selection')
    if len(report['commands']) != 18 or any(c['exit'] != 0 for c in report['commands']):
        raise ValueError('author metadata not eighteen successful commands')
    if report['tools'] != old['tools'] or report['ARM_toolchain_pins'] != old['ARM_toolchain_pins'] or (
        {k: v for k, v in report['environment'].items() if k != 'TMPDIR'} !=
        {k: v for k, v in old['environment'].items() if k != 'TMPDIR'}) or (
        report['environment']['TMPDIR'] != str(RUN / 'tmp') or old['environment']['TMPDIR'] != str(OLD / 'tmp')):
        raise ValueError('recorded toolchain/environment drift from v1')
    for rel, pin in report['frozen_prior_pins'].items():
        verify(ROOT / rel, pin)
    for rel, pin in buildseal['run_files'].items():
        verify(ROOT / rel, pin)
    actual_run = {str(p.relative_to(ROOT)) for p in RUN.rglob('*') if p.is_file()}
    if actual_run != set(buildseal['run_files']):
        raise ValueError('build-seal run path set differs')
    if buildseal['reused_original_objects'] != report['reused_objects']:
        raise ValueError('reused object receipts differ')
    for rel, pin in report['source_files'].items():
        verify(RUN / 'source' / rel, pin)
    generated_changed = [rel for rel in sorted(report['source_files']) if report['source_files'][rel] != old['source_files'][rel]]
    if generated_changed != ['native-watchdog.inc', 'native_adapter.c', 'native_watchdog.h', 'watchdog_owner.c', 'watchdog_owner.h']:
        raise ValueError('unexpected generated delta')
    app_delta = ''
    for path in sorted(APP.iterdir()):
        if path.is_file():
            oldpath = DOC / 'baseline-v1-app' / path.name
            oldrel = str(Path(baseline['original_app']) / path.name)
            verify(oldpath, baseline['baseline_app'][oldrel])
            verify(ROOT / oldrel, baseline['baseline_app'][oldrel])
            verify(RUN / 'inputs/new' / path.name, seal['source_files'][str(path.relative_to(ROOT))])
            app_delta += diff(oldpath, path, 'v1-app/' + path.name, 'v2-app/' + path.name)
    gen_delta = ''.join(diff(OLD / 'source' / rel, RUN / 'source' / rel,
                            'v1-source/' + rel, 'v2-source/' + rel) for rel in sorted(report['source_files']))
    for name, delta in (('v1-to-v2-app.patch', app_delta), ('v1-to-v2-generated.patch', gen_delta)):
        if delta.encode() != read(RUN / name):
            raise ValueError('actual delta does not match: ' + name)
        with (OUT / ('actual-' + name)).open('x') as stream:
            stream.write(delta)
    captured = []
    for origin, pin in report['inputs'].items():
        if origin in report['compiled_source_header_closure']:
            path = RUN / 'inputs/consumed-header-closure' / origin.lstrip('/')
        elif origin.startswith(str(OLD / 'objects') + '/'):
            path = RUN / 'objects' / Path(origin).name
        elif origin.startswith(str(DOC / 'baseline-v1-app') + '/'):
            path = RUN / 'inputs/v1-app' / Path(origin).name
        elif origin == str(DOC / 'BASELINE_CUSTODY01.json'):
            path = RUN / 'inputs/provenance/BASELINE_CUSTODY01.json'
        elif origin.startswith(NATIVE_NAME + '/inputs/framework/'):
            path = RUN / 'source' / origin[len(NATIVE_NAME + '/inputs/'):]
        elif origin.startswith(NATIVE_NAME + '/inputs/runtime-license-'):
            path = RUN / 'inputs/licenses' / Path(origin).name
        elif origin.startswith(NATIVE_NAME + '/inputs/'):
            path = RUN / 'original' / origin[len(NATIVE_NAME + '/inputs/'):]
        elif origin == NATIVE_NAME + '/manifest.json':
            path = RUN / 'inputs/provenance/run34-manifest.json'
        elif origin.startswith(str(APP) + '/'):
            path = RUN / 'inputs/new' / Path(origin).name
        else:
            path = RUN / 'inputs/provenance' / Path(origin).relative_to(ROOT)
        verify(path, pin)
        captured.append({'recorded_origin': origin, 'captured_public_copy': str(path.relative_to(ROOT)), **pin})
    reused_rows = []
    reused_units = ('sensor_baseline', 'memory_reserve', 'memory_runtime', 'memory_reader')
    for mode in ('host', 'arm'):
        for unit in reused_units:
            name = mode + '-' + unit
            oldobj, newobj = OLD / 'objects' / (name + '.o'), RUN / 'objects' / (name + '.o')
            expected = report['reused_objects'][str(oldobj)]
            verify(oldobj, expected)
            verify(newobj, expected)
            for suffix in ('.d', '.su'):
                if read(oldobj.with_suffix(suffix)) != read(newobj.with_suffix(suffix)):
                    raise ValueError('reused dependency/stack file changed')
            deps = shlex.split(text(newobj.with_suffix('.d')).replace('\\\n', '').split(':', 1)[1])
            projects = []
            for dep in deps:
                pin = old['compiled_source_header_closure'][dep]
                if report['compiled_source_header_closure'][dep] != pin:
                    raise ValueError('reused dependency pin differs from old compile closure')
                verify(RUN / 'inputs/consumed-header-closure' / dep.lstrip('/'), pin)
                verify(OLD / 'inputs/consumed-header-closure' / dep.lstrip('/'), pin)
                normalized = posixpath.normpath(dep)
                oldprefix = str(OLD / 'source') + '/'
                if normalized.startswith(oldprefix):
                    relative = normalized[len(oldprefix):]
                    if relative in generated_changed:
                        raise ValueError('reused object consumed changed owner source/header')
                    verify(RUN / 'source' / relative, pin)
                    projects.append(relative)
            oldcompile = [c for c in old['commands'] if '-c' in c['argv'] and str(OLD / 'source' / (unit + '.c')) in c['argv'] and Path(c['log']).name.startswith(mode + '-')]
            if len(oldcompile) != 1:
                raise ValueError('missing exact original reused-unit compile command')
            reused_rows.append({'mode': mode, 'unit': unit, 'object': expected, 'dependency_count': len(deps),
                'project_dependencies': projects, 'no_changed_owner_dependency': True,
                'original_compile_command': oldcompile[0], 'dependency_file': str(newobj.with_suffix('.d').relative_to(ROOT))})
        members = archive_members(RUN / ('libqc07f-native-watchdog-owner-v2-' + mode + '.a'))
        expected_names = {mode + '-' + unit + '.o' for unit in ('native_adapter', *reused_units, 'watchdog_owner')}
        if set(members) != expected_names:
            raise ValueError('archive member composition differs')
        for name, body in members.items():
            if body != read(RUN / 'objects' / name):
                raise ValueError('archive member does not equal reviewed changed/reused object')
    if len(reused_rows) != 8:
        raise ValueError('unexpected reused object count')
    for c in report['commands']:
        verify(Path(c['log']), c['log_pin'])
        if '-c' in c['argv'] and read(Path(c['log'])):
            raise ValueError('changed-unit compile log is not empty')
    for name, pin in report['artifacts'].items():
        verify(RUN / name, pin)
    for name, pin in report['compiled_source_header_closure'].items():
        verify(RUN / 'inputs/consumed-header-closure' / name.lstrip('/'), pin)
    # Check the complete original-v6 patch from retained existing source bytes.
    applied = ''
    for name in ('native_adapter.c', 'native_adapter.h', 'rtspd-native-import.c', 'sensor_baseline.c',
                 'sensor_baseline.h', 'memory_reserve.c', 'memory_reserve.h', 'memory_runtime.c',
                 'memory_runtime.h', 'memory_reader.c', 'handoff.h', 'capture-receipt.h',
                 'capture-retirement-uapi.h', 'crc-bzip2.h', 'h264-baseline.h', 'isp-stream.h', 'qc07f_hwenc.h', 'sc1346-init.h'):
        applied += diff(RUN / 'original' / name, RUN / 'source' / name, 'a/' + name, 'b/' + name)
    for name in ('watchdog_owner.c', 'watchdog_owner.h', 'native_watchdog.h', 'source-close-once.inc', 'native-watchdog.inc'):
        applied += ''.join(difflib.unified_diff([], text(RUN / 'source' / name).splitlines(True), fromfile='/dev/null', tofile='b/' + name))
    if applied.encode() != read(RUN / 'applied-source.patch'):
        raise ValueError('actual complete v6 patch differs')
    with (OUT / 'actual-applied-source.patch').open('x') as stream:
        stream.write(applied)
    ranges = {
        APP / 'native-watchdog.inc': [(1, 128)], APP / 'watchdog_owner.c': [(1, 127)],
        APP / 'watchdog_owner.h': [(1, 47)], APP / 'native_watchdog.h': [(1, 27)],
        APP / 'overlay.py': [(62, 103)], APP / 'build.py': [(193, 243)],
        RUN / 'source/native_adapter.c': [(17, 38), (56, 78), (108, 168)],
        RUN / 'source/memory_runtime.c': [(16, 29)],
        RUN / 'source/framework/include/camera/backend.h': [(9, 12)],
        RUN / 'source/rtspd-native-import.c': [(61, 96), (128, 163), (221, 250)],
        RUN / 'source/source-close-once.inc': [(1, 15)],
    }
    excerpts = []
    for path, spans in ranges.items():
        lines = read(path).splitlines(keepends=True)
        for start, end in spans:
            raw = b''.join(lines[start - 1:end])
            excerpts.append({'path': str(path.relative_to(ROOT)), 'start_line': start, 'end_line': end,
                             'text': raw.decode(), 'sha256': hashlib.sha256(raw).hexdigest()})
    save('public-excerpts.json', excerpts)
    save('reused-object-custody.json', reused_rows)
    save('captured-public-input-map.json', captured)
    save('verification.json', {
        'source_tree_sha256': aggregate, 'source_files': len(seal['source_files']),
        'source_seal_report_pins': len(seal['report_pins']), 'source_seal_evidence_pins': len(seal['evidence_pins']),
        'prior_custody_verified': len(baseline['prior_custody_pins']), 'frozen_build_custody_verified': len(report['frozen_prior_pins']),
        'build_sealed_run_files': len(buildseal['run_files']), 'captured_public_inputs_verified': len(captured),
        'consumed_source_header_copies_verified': len(report['compiled_source_header_closure']),
        'generated_files': len(report['source_files']), 'changed_generated': generated_changed,
        'reused_objects_verified': len(reused_rows), 'no_reused_object_consumed_changed_owner_header': True,
        'both_archives_members_byte_match_six_reviewed_objects': True,
        'recorded_tools_ARM_pins_and_environment_match_v1_except_fresh_TMPDIR': True,
        'author_commands': report['commands'], 'author_artifacts': report['artifacts'],
        'actual_app_generated_and_full_v6_diffs_match': True,
        'reviewer_reran_compiler_generator_or_tests': False,
        'current_external_tools_headers_rehashed': False, 'private_original_acquisition_paths_opened': False,
    })
    read(Path(__file__))
    save('reader-receipt.json', {'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'scope': 'public static source/diff/dependency/archive-byte metadata and hash only; no child/compiler/generator/tests, target/UART/RPC/status/network/private IO, P1 or old-source/Git work',
        'inputs': INPUTS})
    print(json.dumps({'source_tree_sha256': aggregate, 'generated_files': len(report['source_files']),
        'sealed_evidence_files': len(seal['evidence_pins']), 'prior_custody': len(baseline['prior_custody_pins']),
        'captured_inputs': len(captured), 'consumed_paths': len(report['compiled_source_header_closure']),
        'reused_objects': len(reused_rows), 'archive_members_byte_verified': 12, 'diffs_match': True}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments or optimized Python')
    main()
