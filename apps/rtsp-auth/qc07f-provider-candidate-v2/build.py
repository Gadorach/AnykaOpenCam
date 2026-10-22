#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""HOST/ARM custody helpers compile/archive/relocatable only; never execute."""
import argparse
import ast
import datetime
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/qc07f-provider-v2-20261022'
V1 = ROOT / 'apps/rtsp-auth/qc07f-provider-candidate'
QUERY = ROOT / 'apps/qualification/kernel76-encoder-identity'
V1_DOC = ROOT / 'docs/qualification-plans/qc07f-provider-20261022'
QUERY_DOC = ROOT / 'docs/qualification-plans/encoder-identity-20261022'
SEALS = {
 'qc07f-provider-20261022': 'bcc141c9e57b791d2670b7a45c55c54a33699d2ceb12f4e91d4e0b6fccf49d82',
 'encoder-identity-20261022': 'ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520',
 'encoder-identity-clang2313-20261022': 'a1d9f38074bcbb204460f2289deee8cc61872f64545171b5fbcbca9dbc320ad3',
 'rtsp-runtime-20261022': '35712f25616610e4e6026436d3a239e9d7020c420ae7fe3de4991a3aa7734288'}


def pin(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(65536), b''):
            h.update(block)
    return {'bytes': path.stat().st_size, 'sha256': h.hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run')
    parser.add_argument('--toolchain', type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', args.run):
        parser.error('simple fresh run name required')
    frozen, seal_paths = {}, []
    for name, expected in SEALS.items():
        path = ROOT / 'docs/qualification-plans' / name / 'SOURCE_SEAL01.json'
        seal_paths.append(path)
        seal = json.loads(path.read_text())
        if seal['source_tree_sha256'] != expected:
            raise RuntimeError('unexpected original source seal: ' + name)
        digest = hashlib.sha256()
        for source_name, value in sorted(seal['source_files'].items()):
            if pin(ROOT / source_name) != value:
                raise RuntimeError('original sealed source changed: ' + source_name)
            digest.update(source_name.encode() + b'\0' + bytes.fromhex(value['sha256']))
        if digest.hexdigest() != expected:
            raise RuntimeError('original source aggregate mismatch: ' + name)
        for group in ('source_files', 'reports', 'evidence_pins', 'report_and_build_receipts'):
            for source_name, value in seal.get(group, {}).items():
                if pin(ROOT / source_name) != value:
                    raise RuntimeError('original sealed evidence changed: ' + source_name)
            frozen.update(seal.get(group, {}))
        frozen[str(path.relative_to(ROOT))] = pin(path)
    # Preserve original public source/evidence snapshots, not just implementations.
    for directory in (V1_DOC / 'runs/build01', QUERY_DOC / 'runs/build01',
            ROOT / 'docs/qualification-plans/encoder-identity-clang2313-20261022/runs/build01'):
        frozen.update({str(p.relative_to(ROOT)): pin(p) for p in sorted(directory.rglob('*')) if p.is_file()})
    for name, expected in frozen.items():
        if pin(ROOT / name) != expected:
            raise RuntimeError('original sealed input changed: ' + name)
    for name in ('frame_fd_custody.c', 'frame_fd_custody.h'):
        if (APP / name).read_bytes() != (V1 / name).read_bytes():
            raise RuntimeError('frame import must be byte-identical: ' + name)
    query_run = QUERY_DOC / 'runs/build01'
    query_report = json.loads((query_run / 'REPORT.json').read_text())
    for name, expected in query_report['candidate_files'].items():
        if pin(query_run / 'candidate' / name) != expected:
            raise RuntimeError('query snapshot changed: ' + name)
    pins = next(ast.literal_eval(n.value) for n in ast.parse(
        (ROOT / 'apps/rtsp-auth/build.py').read_text()).body if
        isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and
        n.targets[0].id == 'ARM_PINS')
    toolchain = args.toolchain.resolve()
    for name, expected in pins.items():
        if pin(toolchain / name)['sha256'] != expected:
            raise RuntimeError('retained ARM toolchain pin mismatch: ' + name)
    parent = DOC / 'runs'
    parent.mkdir(parents=True, exist_ok=True)
    if parent.resolve() != parent:
        raise RuntimeError('output parent symlink refused')
    out = parent / args.run
    out.mkdir()
    for name in ('inputs', 'objects', 'logs', 'tmp'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp')}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'artifacts': {}, 'environment': env, 'original_snapshot_pins': frozen,
              'original_source_seals': SEALS, 'ARM_toolchain_pins': pins,
              'scope': 'HOST/ARM SOURCE COMPILE/ARCHIVE/RELOCATABLE ONLY',
              'byte_identical_imports': ['frame_fd_custody.c', 'frame_fd_custody.h'],
              'outputs_or_tests_executed': False, 'target_or_private_IO': False,
              'driver_or_UAPI_suite_rerun': False, 'full_kernel_module_image_build': False,
              'provider_adoption': False, 'native_memory_admission': 'HOLD',
              'failed_next_retention': 'BLOCKED', 'G2': False, 'G3': False, 'G8': False}

    def copy(path, target):
        before = pin(path)
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as f:
            f.write(path.read_bytes())
        if pin(target) != before:
            raise RuntimeError('copy input changed')
        key = str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
        report['inputs'][key] = before

    def command(argv, label):
        argv = list(map(str, argv))
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'cwd': str(ROOT), 'started': now(), 'log': str(log)}
        report['commands'].append(entry)
        with log.open('xb') as f:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=f,
                                    stderr=subprocess.STDOUT, timeout=120)
        entry.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('compile/link failed; no altered-flags retry: ' + label)

    try:
        for path in sorted(APP.glob('*')):
            if path.is_file():
                copy(path, out / 'inputs/source' / path.name)
        for name in ('frame_fd_custody.c', 'frame_fd_custody.h'):
            copy(V1 / name, out / 'inputs/v1-import-original' / name)
        receipt = V1_DOC / 'runs/build01/inputs/native/capture-retirement-uapi.h'
        if pin(receipt)['sha256'] != '887e32a3a9201812be3bd882826d2bc88a518b613a11793c3da862a39458dc39':
            raise RuntimeError('original frame UAPI changed')
        copy(receipt, out / 'inputs/uapi/capture-retirement-uapi.h')
        copy(QUERY / 'encoder-open-query.h', out / 'inputs/uapi/encoder-open-query.h')
        copy(query_run / 'candidate/stage-telemetry-uapi.h', out / 'inputs/uapi/stage-telemetry-uapi.h')
        for path in seal_paths:
            copy(path, out / 'inputs/provenance' / path.parent.name / path.name)
        copy(ROOT / 'apps/rtsp-auth/build.py', out / 'inputs/provenance/ARM-toolchain-pins.py')
        for path in sorted((V1_DOC / 'runs/build01/inputs/licenses').glob('*')):
            if path.is_file():
                copy(path, out / 'inputs/licenses' / path.name)
        prefix = str(toolchain / 'bin/arm-buildroot-linux-musleabi-')
        modes = [('host', '/usr/bin/cc', '/usr/bin/ar', '/usr/bin/nm', '/usr/bin/readelf', ['-O2']),
                 ('arm', prefix + 'gcc', prefix + 'ar', prefix + 'nm', prefix + 'readelf',
                  ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft'])]
        for mode, cc, ar, nm, readelf, flags in modes:
            command([cc, '--version'], mode + '-compiler-version')
            command([cc, '-dumpmachine'], mode + '-compiler-target')
            objects = []
            for name in ('frame_fd_custody', 'encoder_fd_custody'):
                obj = out / 'objects' / (mode + '-' + name + '.o')
                command([cc, *flags, '-std=c11', '-Wall', '-Wextra', '-Werror', '-fno-common',
                         '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2', '-ffunction-sections',
                         '-fdata-sections', '-fstack-usage', '-I' + str(out / 'inputs/source'),
                         '-I' + str(out / 'inputs/uapi'), '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', out / 'inputs/source' / (name + '.c'), '-o', obj], mode + '-cc-' + name)
                objects.append(obj)
            archive = out / ('libqc07f-provider-custody-v2-' + mode + '.a')
            command([ar, 'rcsD', archive, *objects], mode + '-archive')
            reloc = out / (mode + '-provider-custody-v2-NOT-EXECUTABLE.o')
            roots = ['-Wl,--undefined=qc_' + role + '_fd_' + operation
                     for role in ('frame', 'encoder') for operation in ('take', 'give_back')]
            command([cc, *flags, '-nostdlib', '-r', *objects, '-Wl,--gc-sections', *roots,
                     '-Wl,-Map=' + str(out / (mode + '.map')), '-o', reloc], mode + '-relocatable-link')
            command([readelf, '-h', '-A', '-r', '-W', reloc], mode + '-ELF-relocations')
            command([nm, '-u', reloc], mode + '-undefined-symbols')
            command([nm, '-g', '--defined-only', reloc], mode + '-defined-symbols')
            for path in (archive, reloc):
                report['artifacts'][path.name] = {**pin(path), 'executed': False}
            report.setdefault('tools', {})[mode] = {
                name: {'resolved': str(Path(path).resolve()), **pin(Path(path))}
                for name, path in (('cc', cc), ('ar', ar), ('nm', nm), ('readelf', readelf))}
        closure = {}
        for path in (out / 'objects').glob('*.d'):
            for name in shlex.split(path.read_text().replace('\\\n', '').split(':', 1)[1]):
                p = Path(name)
                if p.is_file():
                    closure[str(p)] = pin(p)
        report['compiled_source_header_closure'] = closure
        for name in closure:
            copy(Path(name), out / 'inputs/consumed-header-closure' / Path(name).relative_to('/'))
        report['status'] = 'HOST/ARM COMPILE/ARCHIVE/RELOCATABLE PASS; NOT RUN OR ADOPTED'
    except BaseException as exc:
        report['status'] = 'FAILED; outputs preserved'
        report['error'] = type(exc).__name__ + ': ' + str(exc)
        raise
    finally:
        report['original_snapshots_unchanged'] = all(pin(ROOT / n) == p for n, p in frozen.items())
        report['inputs_unchanged'] = all(pin(ROOT / n) == p for n, p in report['inputs'].items())
        report['frame_import_still_byte_identical'] = all((APP / n).read_bytes() == (V1 / n).read_bytes()
            for n in ('frame_fd_custody.c', 'frame_fd_custody.h'))
        intact = all(report[k] for k in ('original_snapshots_unchanged', 'inputs_unchanged',
                                         'frame_import_still_byte_identical'))
        if not intact:
            report['status'] = 'FAILED INPUT CUSTODY; outputs preserved'
        report['ended'] = now()
        with (out / 'REPORT.json').open('x') as f:
            json.dump(report, f, indent=2)
            f.write('\n')
        print(out / 'REPORT.json')
        if not intact:
            raise RuntimeError('input custody mismatch')


if __name__ == '__main__':
    main()
