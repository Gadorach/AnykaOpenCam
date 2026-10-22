#!/usr/bin/env python3
"""Independent public byte/diff/dependency/archive reader. Executes no inputs."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import posixpath
import shlex
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
OUT = ROOT / 'docs/qualification-plans/qc07f-owned-provider-review-20261022'
DOC = ROOT / 'docs/qualification-plans/qc07f-owned-provider-20261022'
APP = ROOT / 'apps/rtsp-auth/qc07f-owned-provider-candidate'
RUN = DOC / 'runs/build02'
OLD_DOC = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-v2-20261022'
OLD = OLD_DOC / 'runs/build01'
V1 = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-20261022/runs/build04'
INPUTS = {}
SOURCE_PIN = 'fb9cc61500beaad9c6a2cb518ebf06ce73cd1d7a3d9595e54b8153d8db58dedb'
UNITS = ('native_adapter', 'sensor_baseline', 'memory_reserve', 'memory_runtime',
         'memory_reader', 'watchdog_owner', 'frame_fd_custody', 'encoder_fd_custody')


def allowed(path):
    rel = str(path.relative_to(ROOT))
    if '..' in path.parts or not rel.startswith(('docs/qualification-plans/', 'apps/rtsp-auth/', 'apps/qualification/')):
        raise ValueError('not a public review input: ' + rel)
    if any(word in rel.lower() for word in ('station-attach-rx', '/p1', 'kernel76-transport')):
        raise ValueError('out of scope input: ' + rel)
    current = path
    while current != ROOT:
        if current.is_symlink():
            raise ValueError('unexpected symlink: ' + rel)
        current = current.parent
    return rel


def read(path):
    path = Path(path)
    rel = allowed(path)
    raw = path.read_bytes()
    pin = {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}
    if rel in INPUTS and INPUTS[rel] != pin:
        raise ValueError('changed during review: ' + rel)
    INPUTS[rel] = pin
    return raw


def load(path):
    return json.loads(read(path))


def verify(path, expected):
    read(path)
    if INPUTS[str(path.relative_to(ROOT))] != {k: expected[k] for k in ('bytes', 'sha256')}:
        raise ValueError('pin differs: ' + str(path.relative_to(ROOT)))


def save(name, data):
    with (OUT / name).open('x') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')


def delta(before, after, left, right):
    a = read(before).decode().splitlines(True) if before.exists() else []
    return ''.join(difflib.unified_diff(a, read(after).decode().splitlines(True), fromfile=left, tofile=right))


def members(path):
    raw = read(path)
    if raw[:8] != b'!<arch>\n':
        raise ValueError('not normal archive')
    offset, names, result = 8, b'', {}
    while offset < len(raw):
        header = raw[offset:offset + 60]
        if len(header) != 60 or header[58:60] != b'`\n':
            raise ValueError('archive header')
        size = int(header[48:58].decode().strip())
        body = raw[offset + 60:offset + 60 + size]
        if len(body) != size:
            raise ValueError('archive extent')
        label = header[:16].decode().strip()
        if label == '//':
            names = body
        elif label not in ('/', '/SYM64/'):
            name = names[int(label[1:]):].split(b'\n', 1)[0].decode().rstrip('/') if label.startswith('/') and label[1:].isdigit() else label.rstrip('/')
            if name in result:
                raise ValueError('duplicate member')
            result[name] = body
        offset += 60 + size + size % 2
    if offset != len(raw):
        raise ValueError('archive final extent')
    return result


def main():
    seal = load(DOC / 'SOURCE_SEAL01.json')
    build = load(DOC / 'BUILD_SEAL01.json')
    baseline = load(DOC / 'BASELINE_CUSTODY01.json')
    report = load(RUN / 'REPORT.json')
    old = load(OLD / 'REPORT.json')
    v1 = load(V1 / 'REPORT.json')
    for group in ('source_files', 'report_pins', 'evidence_pins', 'prior_custody_pins'):
        for rel, pin in seal[group].items():
            verify(ROOT / rel, pin)
    if seal['prior_custody_pins'] != baseline['prior_custody_pins'] or len(baseline['prior_custody_pins']) != 3649:
        raise ValueError('prior custody set differs')
    appset = {str(p.relative_to(ROOT)) for p in APP.iterdir() if p.is_file()}
    if appset != set(seal['source_files']) or len(appset) != 18:
        raise ValueError('app file set differs')
    aggregate = hashlib.sha256(b''.join(n.encode() + b'\0' + bytes.fromhex(p['sha256'])
                  for n, p in sorted(seal['source_files'].items()))).hexdigest()
    if aggregate != SOURCE_PIN or seal['source_tree_sha256'] != aggregate:
        raise ValueError('source aggregate differs')
    oldseal = load(OLD_DOC / 'SOURCE_SEAL01.json')
    if oldseal['source_tree_sha256'] != 'e511920a5c4cc53eac99d4d56e1b50e547c726764b790dbc881aa16ef94c7d8b':
        raise ValueError('baseline source aggregate differs')
    previous_review = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-v2-review-20261022/REVIEW_SEAL01.json'
    read(previous_review)
    if INPUTS[str(previous_review.relative_to(ROOT))]['sha256'] != 'aa73289cac397d740326527fd2be8b0b495f8056c377d477355ffa7f726a2bc8':
        raise ValueError('baseline review seal differs')
    for group, target in (('baseline_app', ROOT / 'apps/rtsp-auth/qc07f-watchdog-owner-v2'), ('baseline_source', OLD / 'source')):
        base = DOC / ('baseline-app' if group == 'baseline_app' else 'baseline-source')
        for rel, pin in baseline[group].items():
            verify(ROOT / rel, pin)
            verify(target / Path(rel).relative_to(base.relative_to(ROOT)), pin)
    for name, entry in baseline['byte_imports'].items():
        for path in (APP / name, DOC / 'baseline-helper' / name, RUN / 'source' / name, ROOT / entry['original']):
            verify(path, entry)
    if len(baseline['byte_imports']) != 6:
        raise ValueError('import count')
    for name in ('watchdog_owner.c', 'watchdog_owner.h', 'source-close-once.inc'):
        if read(APP / name) != read(DOC / 'baseline-app' / name):
            raise ValueError('immutable owner/close source differs')
    for name in ('sensor_baseline.c', 'sensor_baseline.h', 'sc1346-init.h', 'memory_reserve.c',
                 'memory_reserve.h', 'memory_runtime.c', 'memory_runtime.h', 'memory_reader.c'):
        if read(RUN / 'source' / name) != read(DOC / 'baseline-source' / name):
            raise ValueError('immutable sensor/memory source differs')
    helper_uapi = ROOT / 'docs/qualification-plans/qc07f-provider-v2-20261022/runs/build02/inputs/uapi/capture-retirement-uapi.h'
    if read(helper_uapi) != read(RUN / 'source/capture-retirement-uapi.h'):
        raise ValueError('helper/native frame UAPI differs')
    driver = ROOT / 'docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate'
    if read(driver / 'encoder-open-query.h') != read(APP / 'encoder-open-query.h'):
        raise ValueError('driver/native query88 header differs')
    for rel, pin in report['frozen_prior_pins'].items():
        verify(ROOT / rel, pin)
    for rel, pin in build['run_files'].items():
        verify(ROOT / rel, pin)
    if {str(p.relative_to(ROOT)) for p in (DOC / 'runs').rglob('*') if p.is_file()} != set(build['run_files']):
        raise ValueError('build01/02 run file set differs')
    if len(report['frozen_prior_pins']) != 3650 or len(report['commands']) != 20 or any(c['exit'] for c in report['commands']):
        raise ValueError('author command/custody metadata')
    if report['tools'] != old['tools'] or report['ARM_toolchain_pins'] != old['ARM_toolchain_pins']:
        raise ValueError('recorded tools drift')
    if {k: v for k, v in report['environment'].items() if k != 'TMPDIR'} != {k: v for k, v in old['environment'].items() if k != 'TMPDIR'} or report['environment']['TMPDIR'] != str(RUN / 'tmp'):
        raise ValueError('recorded environment drift')
    for name, pin in report['source_files'].items():
        verify(RUN / 'source' / name, pin)
    changed = [n for n, p in report['source_files'].items() if p != old['source_files'].get(n)]
    if changed != report['changed_or_added_generated_files'] or len(changed) != 14 or len(report['source_files']) != 38:
        raise ValueError('generated source set')
    for path in APP.iterdir():
        verify(RUN / 'inputs/new' / path.name, seal['source_files'][str(path.relative_to(ROOT))])
    appdiff = ''.join(delta(DOC / 'baseline-app' / p.name, p, 'watchdog-v2-app/' + p.name,
                         'owned-provider-app/' + p.name) for p in sorted(APP.iterdir()))
    gendiff = ''.join(delta(OLD / 'source' / n, RUN / 'source' / n, 'watchdog-v2-source/' + n,
                         'owned-provider-source/' + n) for n in sorted(report['source_files']))
    names = ('native_adapter.c', 'native_adapter.h', 'rtspd-native-import.c', 'sensor_baseline.c',
             'sensor_baseline.h', 'memory_reserve.c', 'memory_reserve.h', 'memory_runtime.c',
             'memory_runtime.h', 'memory_reader.c', 'handoff.h', 'capture-receipt.h',
             'capture-retirement-uapi.h', 'crc-bzip2.h', 'h264-baseline.h', 'isp-stream.h',
             'qc07f_hwenc.h', 'sc1346-init.h')
    applied = ''.join(delta(RUN / 'original' / n, RUN / 'source' / n, 'a/' + n, 'b/' + n) for n in names)
    for n in ('watchdog_owner.c', 'watchdog_owner.h', 'native_watchdog.h', 'source-close-once.inc',
              'native-watchdog.inc', 'owned-provider.h', 'owned-provider-types.h', 'provider-observe.inc',
              'provider-take.inc', 'frame_fd_custody.c', 'frame_fd_custody.h', 'encoder_fd_custody.c',
              'encoder_fd_custody.h', 'encoder-open-query.h', 'stage-telemetry-uapi.h'):
        before = DOC / 'baseline-source' / n
        applied += delta(before, RUN / 'source' / n, 'a/' + n if before.exists() else '/dev/null', 'b/' + n)
    for n, value in (('watchdog-v2-to-provider-app.patch', appdiff), ('watchdog-v2-to-provider-generated.patch', gendiff), ('applied-source.patch', applied)):
        if value.encode() != read(RUN / n):
            raise ValueError('actual diff differs: ' + n)
        with (OUT / ('actual-' + n)).open('x') as stream:
            stream.write(value)
    captured = []
    for origin, pin in report['inputs'].items():
        if origin in report['compiled_source_header_closure']:
            path = RUN / 'inputs/consumed-header-closure' / origin.lstrip('/')
        elif origin.startswith(str(OLD / 'objects') + '/'):
            path = RUN / 'objects' / Path(origin).name
        elif origin.startswith(str(DOC / 'baseline-source') + '/'):
            rel = Path(origin).relative_to(DOC / 'baseline-source')
            path = RUN / ('source' if str(rel).startswith('framework/') else 'original') / rel
        elif origin.startswith(str(APP) + '/'):
            path = RUN / 'inputs/new' / Path(origin).name
        elif origin.startswith(str(DOC / 'baseline-app') + '/'):
            path = RUN / 'inputs/watchdog-v2-app' / Path(origin).name
        elif origin == str(DOC / 'BASELINE_CUSTODY01.json'):
            path = RUN / 'inputs/provenance/BASELINE_CUSTODY01.json'
        elif origin == str(OLD / 'REPORT.json'):
            path = RUN / 'inputs/provenance/watchdog-v2-build.json'
        elif origin.startswith(str(OLD / 'inputs/licenses') + '/'):
            path = RUN / 'inputs/licenses' / Path(origin).name
        else:
            path = RUN / 'inputs/provenance' / Path(origin).relative_to(ROOT)
        verify(path, pin)
        captured.append({'recorded_origin': origin, 'captured_public_copy': str(path.relative_to(ROOT)), **pin})
    closures = report['compiled_source_header_closure']
    for origin, pin in closures.items():
        verify(RUN / 'inputs/consumed-header-closure' / origin.lstrip('/'), pin)
    rows = []
    for mode in ('host', 'arm'):
        for unit in UNITS:
            name = mode + '-' + unit
            obj = RUN / 'objects' / (name + '.o')
            reused = unit not in report['compiled_units_per_platform']
            original_report = old if unit == 'watchdog_owner' else v1
            original_run = OLD if unit == 'watchdog_owner' else V1
            if reused:
                oldobj = OLD / 'objects' / obj.name
                expected = report['reused_objects'][str(oldobj)]
                verify(oldobj, expected)
                verify(obj, expected)
                for ext in ('.d', '.su'):
                    if read(oldobj.with_suffix(ext)) != read(obj.with_suffix(ext)):
                        raise ValueError('reused dep/stack file differs')
            deps = shlex.split(read(obj.with_suffix('.d')).decode().replace('\\\n', '').split(':', 1)[1])
            project = []
            for dep in deps:
                pin = closures[dep]
                if reused:
                    if original_report['compiled_source_header_closure'][dep] != pin:
                        raise ValueError('reused dependency pin drift')
                    verify(original_run / 'inputs/consumed-header-closure' / dep.lstrip('/'), pin)
                normalized = posixpath.normpath(dep)
                for src in (RUN / 'source', OLD / 'source', V1 / 'source'):
                    if normalized.startswith(str(src) + '/'):
                        rel = normalized[len(str(src)) + 1:]
                        verify(RUN / 'source' / rel, pin)
                        if reused and rel in changed:
                            raise ValueError('reused unit consumed changed project source')
                        project.append(rel)
                        break
            r = original_report if reused else report
            src = original_run if reused else RUN
            commands = [c for c in r['commands'] if '-c' in c['argv'] and str(src / 'source' / (unit + '.c')) in c['argv'] and Path(c['log']).name.startswith(mode + '-')]
            if len(commands) != 1:
                raise ValueError('no unique actual compile receipt')
            rows.append({'mode': mode, 'unit': unit, 'reused': reused,
                         'dependency_count': len(deps), 'project_dependencies': project,
                         'actual_compile_command': commands[0], 'object': INPUTS.get(str(obj.relative_to(ROOT)))})
        archive = members(RUN / ('libqc07f-owned-provider-' + mode + '.a'))
        if set(archive) != {mode + '-' + unit + '.o' for unit in UNITS}:
            raise ValueError('archive member set')
        for name, body in archive.items():
            if body != read(RUN / 'objects' / name):
                raise ValueError('archive member body differs')
        expected_objects = [str(RUN / 'objects' / (mode + '-' + u + '.o')) for u in UNITS]
        for label in ('archive', 'relocatable-link'):
            command = next(c for c in report['commands'] if Path(c['log']).name == mode + '-' + label + '.txt')
            if [a for a in command['argv'] if a.endswith('.o') and '/objects/' in a] != expected_objects:
                raise ValueError('composition argv differs')
    for command in report['commands']:
        verify(Path(command['log']), command['log_pin'])
        if '-c' in command['argv'] and read(Path(command['log'])):
            raise ValueError('nonempty compiler diagnostics')
    for name, pin in report['artifacts'].items():
        verify(RUN / name, pin)
    for n in ('native_adapter.c', 'rtspd-native-import.c'):
        if 'open(' in read(RUN / 'source' / n).decode():
            raise ValueError('unexpected device open fallback')
    # Retain the actual public code anchors read during ordinary source inspection.
    spans = {APP / 'provider-take.inc': [(1, 55)], APP / 'provider-observe.inc': [(1, 88)],
             APP / 'owned-provider-types.h': [(1, 24)], APP / 'owned-provider.h': [(1, 31)],
             APP / 'native-watchdog.inc': [(1, 139)], APP / 'watchdog_owner.c': [(1, 127)],
             RUN / 'source/native_adapter.c': [(1, 204)], RUN / 'source/rtspd-native-import.c': [(1, 262)],
             driver / 'qc07f_hwenc.c': [(361, 459)], driver / 'encoder-open-query.inc': [(1, 45)],
             ROOT / 'docs/qualification-plans/qc07f-provider-20261022/runs/build01/inputs/implementation/native75/capture-retirement.c': [(29, 91), (93, 138)]}
    excerpts = []
    for path, ranges in spans.items():
        lines = read(path).splitlines(True)
        for start, end in ranges:
            raw = b''.join(lines[start - 1:end])
            excerpts.append({'path': str(path.relative_to(ROOT)), 'start_line': start, 'end_line': end,
                             'sha256': hashlib.sha256(raw).hexdigest(), 'text': raw.decode()})
    save('public-excerpts.json', excerpts)
    save('object-custody.json', rows)
    save('captured-public-input-map.json', captured)
    save('verification.json', {'source_tree_sha256': aggregate, 'prior_custody_verified': 3649,
        'build_frozen_pins_verified': len(report['frozen_prior_pins']), 'build01_and_build02_run_files_verified': len(build['run_files']),
        'seal_groups_verified': {k: len(seal[k]) for k in ('source_files', 'report_pins', 'evidence_pins')},
        'byte_imports': baseline['byte_imports'], 'captured_public_inputs_verified': len(captured),
        'consumed_source_header_paths': len(closures), 'generated_files': len(report['source_files']),
        'changed_or_added_generated_files': changed, 'actual_diffs_match': True,
        'changed_objects': sum(not x['reused'] for x in rows), 'reused_objects': sum(x['reused'] for x in rows),
        'archive_member_bodies_byte_verified': 16, 'recorded_tools_and_ARM_pins_match_baseline': True,
        'environment_matches_except_fresh_TMPDIR': True, 'author_commands': report['commands'], 'author_artifacts': report['artifacts'],
        'reviewer_compiler_generator_candidate_helper_tests_execution': False,
        'current_external_tool_or_header_installations_rehashed': False, 'private_original_acquisitions_opened': False})
    read(Path(__file__))
    save('reader-receipt.json', {'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'scope': 'public static bytes/diff/hash/receipt/dependency/archive inspection only; no input execution, target/network/private/P1/Git work',
        'inputs': INPUTS})
    print(json.dumps({'source_pin': aggregate, 'inputs_recorded': len(INPUTS), 'prior_pins': 3649,
                      'captured_inputs': len(captured), 'consumed_paths': len(closures), 'reused_objects': 10,
                      'new_objects': 6, 'archive_member_bodies': 16, 'actual_diffs_match': True}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments/optimized Python')
    main()
