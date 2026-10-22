#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile the frame-FD custody helper only; never execute any output/test."""
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
DOC = ROOT / 'docs/qualification-plans/qc07f-provider-20261022'
NATIVE = ROOT / 'evidence/delegated/20261012-native-handoff/run34'
PLATFORM = ROOT / 'bringup/qc07f/variants/security75/mach-ak3918ev200'
KERNEL = ROOT / 'work/lightbulb-camera/implementation-20261008T205404Z/sources/linux-lts'
NATIVE_PIN = '8286c85c09d7f34d5bb6af13054803f68ebafad4e2e874da4b1a35aaf9e0d529'


def pin(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run')
    parser.add_argument('--toolchain', type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', args.run):
        parser.error('simple fresh name required')
    if pin(NATIVE / 'manifest.json')['sha256'] != NATIVE_PIN:
        raise RuntimeError('native run34 manifest changed')
    entries = {e['path']: e['sha256'] for e in
               json.loads((NATIVE / 'manifest.json').read_text())['files']}
    native_names = ('native_adapter.c', 'native_adapter.h', 'rtspd-native-import.c',
                    'sensor_baseline.c', 'capture-retirement-uapi.h', 'qc07f_hwenc.h')
    for name in native_names:
        if pin(NATIVE / 'inputs' / name)['sha256'] != entries['inputs/' + name]:
            raise RuntimeError('native input changed: ' + name)
    # Existing UAPI exactly matches the implementation whose close/identity
    # behavior was inspected. No new ioctl numbers or source overlays.
    if (NATIVE / 'inputs/capture-retirement-uapi.h').read_bytes() != (
            PLATFORM / 'capture-retirement-uapi.h').read_bytes():
        raise RuntimeError('retained/implementation receipt UAPI mismatch')
    expected_platform = {
        'isp.c': '7d0ef80561df4ef058be1deb6698001c35066fab5533f492dec97d2b447bbd50',
        'qc07f_hwenc.c': 'e97744838acd469772db41fb1fe0907c95fb6e923577cdbffc84ead0141a026f'}
    for name, expected in expected_platform.items():
        if pin(PLATFORM / name)['sha256'] != expected:
            raise RuntimeError('pinned native75 implementation changed: ' + name)
    pins = next(ast.literal_eval(n.value) for n in ast.parse(
        (ROOT / 'apps/rtsp-auth/build.py').read_text()).body if
        isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and
        n.targets[0].id == 'ARM_PINS')
    toolchain = args.toolchain.resolve()
    for name, expected in pins.items():
        if pin(toolchain / name)['sha256'] != expected:
            raise RuntimeError('toolchain pin mismatch: ' + name)
    parent = DOC / 'runs'
    parent.mkdir(parents=True, exist_ok=True)
    if parent.resolve() != parent:
        raise RuntimeError('output-parent symlink refused')
    out = parent / args.run
    out.mkdir()
    for name in ('inputs', 'logs', 'objects', 'tmp'):
        (out / name).mkdir()
    environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C',
                   'SOURCE_DATE_EPOCH': '1791763200', 'TMPDIR': str(out / 'tmp')}
    report = {'started': now(), 'scope': 'SOURCE_COMPILE_ARCHIVE_RELOCATABLE_ONLY',
              'status': 'BUILDING', 'commands': [], 'inputs': {}, 'artifacts': {},
              'applied_source_overlays': [], 'new_source': 'frame_fd_custody.c/.h',
              'native_manifest_pin': NATIVE_PIN, 'toolchain_pins': pins,
              'environment': environment, 'output_execution': False,
              'test_execution': False, 'target_IO': False, 'private_input': False,
              'full_provider_adoption': False, 'failed_next_retention': 'BLOCKED',
              'native_memory_admission': 'HOLD', 'G3': False, 'G8': False}

    def copy(path, relative):
        if path.is_symlink():
            raise RuntimeError('input symlink refused: ' + str(path))
        before = pin(path)
        target = out / 'inputs' / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as stream:
            stream.write(path.read_bytes())
        if pin(target) != before:
            raise RuntimeError('input changed during copy')
        report['inputs'][str(path.relative_to(ROOT))] = before

    def command(argv, label):
        argv = list(map(str, argv))
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'cwd': str(ROOT), 'started': now(), 'log': str(log)}
        report['commands'].append(entry)
        with log.open('xb') as stream:
            result = subprocess.run(argv, cwd=ROOT, env=environment, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=120)
        entry.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('compile/link failed: ' + label)

    try:
        for path in sorted(APP.glob('*')):
            if path.is_file():
                copy(path, Path('new') / path.name)
        for name in native_names:
            copy(NATIVE / 'inputs' / name, Path('native') / name)
        copy(NATIVE / 'manifest.json', Path('provenance/run34-manifest.json'))
        for name in ('isp.c', 'qc07f_hwenc.c', 'qc07f_hwenc.h', 'watchdog.c',
                     'capture-retirement.c', 'capture-retirement.h', 'capture-retirement-uapi.h'):
            copy(PLATFORM / name, Path('implementation/native75') / name)
        for name in ('drivers/watchdog/watchdog_dev.c', 'drivers/i2c/i2c-dev.c',
                     'fs/file.c', 'fs/file_table.c', 'Makefile'):
            copy(KERNEL / name, Path('implementation/linux-lts') / name)
        for path in sorted((NATIVE / 'inputs').glob('runtime-license-*')):
            copy(path, Path('licenses') / path.name)
        copy(ROOT / 'apps/rtsp-auth/build.py', Path('provenance/toolchain-pin-source.py'))
        prefix = str(toolchain / 'bin/arm-buildroot-linux-musleabi-')
        modes = [('host', '/usr/bin/cc', '/usr/bin/ar', '/usr/bin/nm', '/usr/bin/readelf', ['-O2']),
                 ('arm', prefix + 'gcc', prefix + 'ar', prefix + 'nm', prefix + 'readelf',
                  ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft'])]
        for name, cc, ar, nm, readelf, flags in modes:
            command([cc, '--version'], name + '-compiler-version')
            command([cc, '-dumpmachine'], name + '-compiler-target')
            obj = out / 'objects' / (name + '-frame_fd_custody.o')
            command([cc, *flags, '-std=c11', '-Wall', '-Wextra', '-Werror',
                     '-fno-common', '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2',
                     '-ffunction-sections', '-fdata-sections', '-fstack-usage',
                     '-I' + str(out / 'inputs/new'), '-I' + str(out / 'inputs/native'),
                     '-MD', '-MF', obj.with_suffix('.d'), '-c',
                     out / 'inputs/new/frame_fd_custody.c', '-o', obj], name + '-compile')
            archive = out / ('libqc07f-frame-fd-custody-' + name + '.a')
            command([ar, 'rcsD', archive, obj], name + '-archive')
            reloc = out / (name + '-frame-fd-custody-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', obj, '-Wl,--gc-sections',
                     '-Wl,--undefined=qc_frame_fd_take', '-Wl,--undefined=qc_frame_fd_give_back',
                     '-Wl,-Map=' + str(out / (name + '.map')), '-o', reloc], name + '-relocatable-link')
            command([readelf, '-h', '-A', '-r', '-W', reloc], name + '-ELF-relocations')
            command([nm, '-u', reloc], name + '-undefined-symbols')
            command([nm, '-g', '--defined-only', reloc], name + '-defined-symbols')
            for path in (archive, reloc):
                report['artifacts'][path.name] = {**pin(path), 'executed': False}
            report.setdefault('tool_files', {})[name] = {
                n: pin(Path(p).resolve()) for n, p in
                (('cc', cc), ('ar', ar), ('nm', nm), ('readelf', readelf))}
        closure = {}
        for path in (out / 'objects').glob('*.d'):
            for name in shlex.split(path.read_text().replace('\\\n', '').split(':', 1)[1]):
                dependency = Path(name)
                if dependency.is_file():
                    closure[str(dependency)] = pin(dependency)
        report['compiled_header_closure'] = closure
        report['inputs_unchanged'] = all(pin(ROOT / name) == before
                                         for name, before in report['inputs'].items())
        if not report['inputs_unchanged']:
            raise RuntimeError('input changed')
        report['status'] = 'HOST/ARM COMPILE/ARCHIVE/RELOCATABLE PASS; NOT RUN'
    except BaseException as exc:
        report['status'] = 'BUILD FAILED; preserved'
        report['error'] = type(exc).__name__ + ': ' + str(exc)
        raise
    finally:
        report['ended'] = now()
        with (out / 'REPORT.json').open('x') as stream:
            json.dump(report, stream, indent=2)
            stream.write('\n')
        print(out / 'REPORT.json')


if __name__ == '__main__':
    main()
