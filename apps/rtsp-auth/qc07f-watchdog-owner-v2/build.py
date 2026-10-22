#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Derive actual v6 source; HOST/ARM compile/archive/REL only. No execution."""
import argparse
import ast
import datetime
import difflib
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-v2-20261022'
OLD_RUN = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-20261022/runs/build04'
NATIVE = ROOT / 'evidence/delegated/20261012-native-handoff/run34'
KERNEL = ROOT / 'work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts'
PLATFORM = ROOT / 'bringup/qc07f/variants/security75/mach-ak3918ev200'
RUN_PIN = '8286c85c09d7f34d5bb6af13054803f68ebafad4e2e874da4b1a35aaf9e0d529'


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
        parser.error('simple fresh name required')
    manifest = NATIVE / 'manifest.json'
    if pin(manifest)['sha256'] != RUN_PIN:
        raise RuntimeError('original v6 run34 manifest changed')
    entries = {e['path'][7:]: e['sha256'] for e in json.loads(manifest.read_text())['files']
               if e['path'].startswith('inputs/')}
    frozen = {}
    for directory in ('rtsp-runtime-20261022', 'qc07f-provider-20261022',
                      'qc07f-provider-v2-20261022', 'encoder-identity-20261022',
                      'encoder-identity-clang2313-20261022'):
        path = ROOT / 'docs/qualification-plans' / directory / 'SOURCE_SEAL01.json'
        seal = json.loads(path.read_text())
        for group in ('source_files', 'reports', 'report_pins', 'evidence_pins', 'report_and_build_receipts'):
            for name, expected in seal.get(group, {}).items():
                if pin(ROOT / name) != expected:
                    raise RuntimeError('frozen source/evidence changed: ' + name)
                frozen[name] = expected
        frozen[str(path.relative_to(ROOT))] = pin(path)
    custody = DOC / 'BASELINE_CUSTODY01.json'
    for name, expected in json.loads(custody.read_text())['prior_custody_pins'].items():
        if pin(ROOT / name) != expected:
            raise RuntimeError('v1/review/prior custody changed: ' + name)
        frozen[name] = expected
    frozen[str(custody.relative_to(ROOT))] = pin(custody)
    previous = json.loads((OLD_RUN / 'REPORT.json').read_text())
    pins = next(ast.literal_eval(n.value) for n in ast.parse(
        (ROOT / 'apps/rtsp-auth/build.py').read_text()).body if
        isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id == 'ARM_PINS')
    toolchain = args.toolchain.resolve()
    for name, expected in pins.items():
        if pin(toolchain / name)['sha256'] != expected:
            raise RuntimeError('retained ARM toolchain changed: ' + name)
    parent = DOC / 'runs'
    parent.mkdir(parents=True, exist_ok=True)
    if parent.resolve() != parent:
        raise RuntimeError('output parent symlink refused')
    out = parent / args.run
    out.mkdir()
    for name in ('original', 'source', 'inputs', 'objects', 'logs', 'tmp'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp'), 'PYTHONDONTWRITEBYTECODE': '1'}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'frozen_prior_pins': frozen, 'source_files': {}, 'artifacts': {},
              'environment': env, 'native_run34_manifest_pin': RUN_PIN,
              'ARM_toolchain_pins': pins, 'scope': 'REAL V6 SOURCE OVERLAY/HOST/ARM COMPILE/ARCHIVE/REL ONLY',
              'outputs_or_tests_executed': False, 'target_or_private_IO': False,
              'kernel_module_image_or_app_build': False, 'provider_native_adoption': False,
              'native_memory_admission': 'HOLD', 'failed_next_process_retention': 'OPEN',
              'G2': False, 'G3': False, 'G8': False,
              'revision': 'W1-W3 ONLY; compile changed native_adapter/watchdog_owner, reuse four pinned v1 units per platform',
              'reused_objects': {}, 'compiled_units_per_platform': ['native_adapter', 'watchdog_owner']}

    def copy(path, target):
        before = pin(path)
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as f:
            f.write(path.read_bytes())
        if pin(target) != before:
            raise RuntimeError('input changed during copy')
        report['inputs'][str(path)] = before

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
            raise RuntimeError('real source compile/link failed; stop: ' + label)

    try:
        names = ('native_adapter.c', 'native_adapter.h', 'rtspd-native-import.c',
                 'sensor_baseline.c', 'sensor_baseline.h', 'memory_reserve.c', 'memory_reserve.h',
                 'memory_runtime.c', 'memory_runtime.h', 'memory_reader.c', 'handoff.h',
                 'capture-receipt.h', 'capture-retirement-uapi.h', 'crc-bzip2.h',
                 'h264-baseline.h', 'isp-stream.h', 'qc07f_hwenc.h', 'sc1346-init.h')
        for name in names:
            original = NATIVE / 'inputs' / name
            if pin(original)['sha256'] != entries[name]:
                raise RuntimeError('native source input changed: ' + name)
            copy(original, out / 'original' / name)
        for path in sorted((NATIVE / 'inputs/framework/include/camera').glob('*.h')):
            name = str(path.relative_to(NATIVE / 'inputs'))
            if pin(path)['sha256'] != entries[name]:
                raise RuntimeError('v6 common header changed')
            copy(path, out / 'source/framework/include/camera' / path.name)
        for path in sorted(APP.glob('*')):
            if path.is_file():
                copy(path, out / 'inputs/new' / path.name)
        copy(custody, out / 'inputs/provenance/BASELINE_CUSTODY01.json')
        for path in sorted((DOC / 'baseline-v1-app').iterdir()):
            copy(path, out / 'inputs/v1-app' / path.name)
        delta = ''.join(''.join(difflib.unified_diff(
            (DOC / 'baseline-v1-app' / p.name).read_text().splitlines(True),
            p.read_text().splitlines(True), fromfile='v1-app/' + p.name, tofile='v2-app/' + p.name))
            for p in sorted(APP.iterdir()) if p.is_file())
        (out / 'v1-to-v2-app.patch').write_text(delta)
        report['v1_to_v2_app_patch'] = pin(out / 'v1-to-v2-app.patch')
        copy(manifest, out / 'inputs/provenance/run34-manifest.json')
        for path in (KERNEL / 'drivers/watchdog/watchdog_dev.c', PLATFORM / 'watchdog.c',
                     KERNEL / 'include/uapi/linux/watchdog.h', KERNEL / 'fs/file_table.c',
                     ROOT / 'apps/rtsp-auth/build.py'):
            copy(path, out / 'inputs/provenance' / path.relative_to(ROOT))
        for path in sorted((NATIVE / 'inputs').glob('runtime-license-*')):
            copy(path, out / 'inputs/licenses' / path.name)
        spec = importlib.util.spec_from_file_location('watchdog_source_overlay', APP / 'overlay.py')
        overlay = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(overlay)  # pure string transformations, no main/runtime
        changed = overlay.derive((out / 'original/native_adapter.c').read_text(),
                                 (out / 'original/rtspd-native-import.c').read_text())
        patch = ''
        for name in names:
            original = (out / 'original' / name).read_text()
            text = changed.get(name, original)
            (out / 'source' / name).write_text(text)
            if text != original:
                patch += ''.join(difflib.unified_diff(original.splitlines(True), text.splitlines(True),
                    fromfile='a/' + name, tofile='b/' + name))
        for name in ('watchdog_owner.c', 'watchdog_owner.h', 'native_watchdog.h',
                     'source-close-once.inc', 'native-watchdog.inc'):
            text = (APP / name).read_text()
            (out / 'source' / name).write_text(text)
            patch += ''.join(difflib.unified_diff([], text.splitlines(True), fromfile='/dev/null', tofile='b/' + name))
        (out / 'applied-source.patch').write_text(patch)
        report['applied_source_patch'] = pin(out / 'applied-source.patch')
        report['source_files'] = {str(p.relative_to(out / 'source')): pin(p)
                                  for p in sorted((out / 'source').rglob('*')) if p.is_file()}
        delta = ''.join(''.join(difflib.unified_diff(
            (OLD_RUN / 'source' / n).read_text().splitlines(True),
            (out / 'source' / n).read_text().splitlines(True),
            fromfile='v1-source/' + n, tofile='v2-source/' + n)) for n in sorted(report['source_files']))
        (out / 'v1-to-v2-generated.patch').write_text(delta)
        report['v1_to_v2_generated_patch'] = pin(out / 'v1-to-v2-generated.patch')
        report['changed_v1_generated_files'] = [n for n in report['source_files']
            if pin(out / 'source' / n) != previous['source_files'][n]]
        report['unchanged_sensor139'] = pin(out / 'source/sensor_baseline.c') == pin(out / 'original/sensor_baseline.c')
        report['unchanged_sensor_table'] = pin(out / 'source/sc1346-init.h') == pin(out / 'original/sc1346-init.h')
        report['unchanged_memory_policy'] = all(pin(out / 'source' / n) == pin(out / 'original' / n)
            for n in ('memory_reserve.c', 'memory_runtime.c', 'memory_reader.c'))
        prefix = str(toolchain / 'bin/arm-buildroot-linux-musleabi-')
        modes = [('host', '/usr/bin/cc', '/usr/bin/ar', '/usr/bin/nm', '/usr/bin/readelf', ['-O2']),
                 ('arm', prefix + 'gcc', prefix + 'ar', prefix + 'nm', prefix + 'readelf',
                  ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft'])]
        units = ('native_adapter', 'sensor_baseline', 'memory_reserve', 'memory_runtime',
                 'memory_reader', 'watchdog_owner')
        for mode, cc, ar, nm, readelf, flags in modes:
            command([cc, '--version'], mode + '-compiler-version')
            command([cc, '-dumpmachine'], mode + '-compiler-target')
            objects = []
            for unit in units:
                obj = out / 'objects' / (mode + '-' + unit + '.o')
                if unit not in report['compiled_units_per_platform']:
                    if pin(out / 'source' / (unit + '.c')) != previous['source_files'][unit + '.c']:
                        raise RuntimeError('reused unit source differs from v1')
                    old = OLD_RUN / 'objects' / obj.name
                    deps = shlex.split(old.with_suffix('.d').read_text().replace('\\\n', '').split(':', 1)[1])
                    for name in deps:
                        if pin(Path(name)) != previous['compiled_source_header_closure'][name]:
                            raise RuntimeError('reused object dependency drift: ' + name)
                    for suffix in ('.o', '.d', '.su'):
                        copy(old.with_suffix(suffix), obj.with_suffix(suffix))
                    report['reused_objects'][str(old)] = pin(old)
                    objects.append(obj)
                    continue
                # Exact legacy compact-source policy from retained v6 builder;
                # no suppression on the new standalone owner implementation.
                legacy = ['-Wno-misleading-indentation'] if unit != 'watchdog_owner' else []
                command([cc, *flags, '-std=gnu11', '-Wall', '-Wextra', '-Werror', *legacy,
                         '-fno-common', '-ffunction-sections', '-fdata-sections', '-fstack-usage',
                         '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2',
                         '-I' + str(out / 'source'), '-I' + str(out / 'source/framework/include'),
                         '-MD', '-MF', obj.with_suffix('.d'), '-c', out / 'source' / (unit + '.c'), '-o', obj],
                        mode + '-cc-' + unit)
                objects.append(obj)
            archive = out / ('libqc07f-native-watchdog-owner-v2-' + mode + '.a')
            command([ar, 'rcsD', archive, *objects], mode + '-archive')
            reloc = out / (mode + '-native-watchdog-owner-v2-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *objects, '-Wl,-Map=' + str(out / (mode + '.map')),
                     '-o', reloc], mode + '-relocatable-link')
            command([readelf, '-h', '-A', '-r', '-W', reloc], mode + '-ELF-relocations')
            command([nm, '-u', reloc], mode + '-undefined-symbols')
            command([nm, '-g', '--defined-only', reloc], mode + '-defined-symbols')
            report.setdefault('tools', {})[mode] = {name: {'resolved': str(Path(path).resolve()), **pin(Path(path))}
                for name, path in (('cc', cc), ('ar', ar), ('nm', nm), ('readelf', readelf))}
            for p in (archive, reloc):
                report['artifacts'][p.name] = {**pin(p), 'executed': False}
        closure = {}
        for path in (out / 'objects').glob('*.d'):
            for name in shlex.split(path.read_text().replace('\\\n', '').split(':', 1)[1]):
                p = Path(name)
                if p.is_file():
                    closure[str(p)] = pin(p)
        report['compiled_source_header_closure'] = closure
        for name in closure:
            p = Path(name)
            copy(p, out / 'inputs/consumed-header-closure' / p.relative_to('/'))
        report['status'] = 'REAL V6 DERIVATIVE/OWNER HOST+ARM COMPILE/ARCHIVE/REL PASS; NOT RUN'
    except BaseException as exc:
        report['status'] = 'FAILED; exact partial evidence retained'
        report['error'] = type(exc).__name__ + ': ' + str(exc)
        raise
    finally:
        report['original_inputs_unchanged'] = all(pin(Path(n)) == v for n, v in report['inputs'].items())
        report['prior_frozen_pins_unchanged'] = all(pin(ROOT / n) == v for n, v in frozen.items())
        report['ended'] = now()
        if not report['original_inputs_unchanged'] or not report['prior_frozen_pins_unchanged']:
            report['status'] = 'FAILED INPUT CUSTODY; partial evidence retained'
        with (out / 'REPORT.json').open('x') as f:
            json.dump(report, f, indent=2)
            f.write('\n')
        print(out / 'REPORT.json')


if __name__ == '__main__':
    main()
