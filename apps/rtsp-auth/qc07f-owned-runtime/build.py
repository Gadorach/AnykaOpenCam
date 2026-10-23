#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile actual owned-runtime sources HOST/ARM; archive/ELF REL only, no execution."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/rtsp-owned-runtime-20261023'
VENDOR = ROOT / 'apps/auth-web/service/vendor/mbedtls'
ARM_PINS = {
 'bin/toolchain-wrapper': 'f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1',
 'bin/arm-buildroot-linux-musleabi-gcc.br_real': 'e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729',
 'arm-buildroot-linux-musleabi/sysroot/lib/libc.a': '0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5',
 'lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a': 'f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a',
}
VENDOR_PIN = 'bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5'


def pin(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run')
    parser.add_argument('--toolchain', required=True, type=Path)
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', args.run):
        parser.error('fresh simple run name required')
    tc = args.toolchain.resolve()
    parent = DOC / 'runs'
    parent.mkdir(exist_ok=True)
    if parent.resolve() != parent:
        raise RuntimeError('output parent symlink refused')
    out = parent / args.run
    out.mkdir()  # exclusive, failures retained
    for name in ('source', 'objects', 'logs', 'tmp'):
        (out / name).mkdir()
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'dependencies': {}, 'artifacts': {}, 'frozen_inputs_unchanged': False,
              'scope': 'ACTUAL_FEATURE_SOURCE_HOST_ARM_COMPILE_ARCHIVE_REL_ONLY',
              'program_test_generator_listener_target_executed': False}
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp')}
    report['environment'] = env

    def copy(source, relative):
        if source.is_symlink():
            raise RuntimeError('source symlink refused: ' + str(source))
        expected = pin(source)
        dest = out / 'source' / relative
        dest.parent.mkdir(parents=True, exist_ok=True)
        with dest.open('xb') as stream:
            stream.write(source.read_bytes())
        if pin(dest) != expected:
            raise RuntimeError('source changed during copy')
        report['inputs'][str(source)] = expected
        return dest

    def command(argv, label):
        argv = list(map(str, argv))
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'started': now(), 'cwd': str(ROOT), 'log': str(log)}
        report['commands'].append(entry)
        with log.open('xb') as stream:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=120)
        entry.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('compile/tool failure: ' + label)
        return log.read_text()

    try:
        frozen = json.loads((DOC / 'INPUTS.json').read_text())['pins']
        for name, expected in frozen.items():
            if pin(ROOT / name) != expected:
                raise RuntimeError('frozen public input changed: ' + name)
        for name, expected in ARM_PINS.items():
            if pin(tc / name)['sha256'] != expected:
                raise RuntimeError('retained toolchain pin changed: ' + name)
        report['toolchain_pins'] = ARM_PINS
        for p in sorted(APP.rglob('*')):
            if p.is_file():
                copy(p, Path('app') / p.relative_to(APP))
        for p in sorted((ROOT / 'framework/include/camera').glob('*.h')):
            copy(p, Path('framework/include/camera') / p.name)
        framework = ('rtsp_digest_wire', 'rtsp_request', 'rtsp_input', 'backend',
                     'packet_hub', 'producer', 'rtp_h264')
        for name in framework:
            copy(ROOT / 'framework/src' / (name + '.c'), Path('framework/src') / (name + '.c'))
        for name in ('ipc.c', 'verifier.h'):
            copy(ROOT / 'apps/rtsp-auth/isolated-verifier' / name, Path('verifier') / name)
        digest = hashlib.sha256()
        vendor_files = sorted(p for p in VENDOR.rglob('*') if p.is_file())
        for p in vendor_files:
            relative = p.relative_to(VENDOR)
            digest.update(relative.as_posix().encode() + b'\0' + bytes.fromhex(pin(p)['sha256']))
            copy(p, Path('vendor') / relative)
        if len(vendor_files) != 277 or digest.hexdigest() != VENDOR_PIN:
            raise RuntimeError('retained vendor pin changed')
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common',
                  '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
                  '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2', '-fstack-usage',
                  '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']
        for name in ('app', 'app/native', 'app/server', 'framework/include',
                     'verifier', 'vendor/include', 'vendor/library'):
            common.append('-I' + str(out / 'source' / name))
        sources = ['app/owned_runtime.c']
        sources += ['app/native/' + name + '.c' for name in ('sensor_baseline',
                    'memory_reserve', 'memory_runtime', 'memory_reader',
                    'frame_fd_custody', 'encoder_fd_custody', 'watchdog_owner')]
        sources += ['app/server/' + name + '.c' for name in ('candidate_auth',
                    'rtsp_server_candidate', 'verifier_adapter_overlay', 'listener_custody')]
        sources += ['verifier/ipc.c'] + ['framework/src/' + name + '.c' for name in framework]
        sources += ['vendor/library/' + name + '.c' for name in ('sha256', 'constant_time', 'platform_util')]
        prefix = str(tc / 'bin/arm-buildroot-linux-musleabi-')
        platforms = [('host', '/usr/bin/cc', '/usr/bin/ar', '/usr/bin/nm', '/usr/bin/readelf', ['-O2']),
                     ('arm', prefix + 'gcc', prefix + 'ar', prefix + 'nm', prefix + 'readelf',
                      ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft'])]
        roots = ('qc07f_owned_runtime_prepare_v1', 'qc07f_owned_runtime_bind_postfork_v1',
                 'camera_rtsp_prestarted_owned_listener_serve_v1', 'candidate_prestarted_start',
                 'candidate_prestarted_finish', 'nh_native_watchdog_take',
                 'nh_native_provider_take', 'nh_native_storage_select', 'nh_native_optin_exact139',
                 'nh_native_memory_start', 'nh_native_memory_fixed_buffers',
                 'nh_native_return_pre_memory_unknown_bootstrap', 'nh_native_watchdog_give_back',
                 'nh_native_next', 'candidate_backend_revoke', 'candidate_backend_cancel')
        for label, cc, ar, nm, readelf, flags in platforms:
            command([cc, '--version'], label + '-compiler-version')
            command([cc, '-dumpmachine'], label + '-compiler-target')
            report.setdefault('tools', {})[label] = {name: {'path': path, **pin(Path(path).resolve())}
                for name, path in (('cc', cc), ('ar', ar), ('nm', nm), ('readelf', readelf))}
            objects = []
            for i, name in enumerate(sources):
                source = out / 'source' / name
                obj = out / 'objects' / (label + '-' + str(i) + '-' + source.stem + '.o')
                inherited = ['-Wno-misleading-indentation'] if source.stem in (
                    'owned_runtime', 'memory_reserve', 'memory_runtime', 'memory_reader',
                    'rtsp_server_candidate', 'rtsp_input') else []
                if source.stem == 'rtsp_input':
                    inherited.append('-Wno-error=nonnull')
                command([cc, *common, *flags, *inherited, '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', source, '-o', obj], label + '-cc-' + str(i))
                objects.append(obj)
            archive = out / ('libqc07f-owned-runtime-' + label + '.a')
            command([ar, 'rcsD', archive, *objects], label + '-archive')
            reloc = out / ('qc07f-owned-runtime-' + label + '-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *objects, '-Wl,--gc-sections',
                     *('-Wl,--undefined=' + name for name in roots),
                     '-Wl,-Map=' + str(out / (label + '.map')), '-o', reloc], label + '-REL')
            defined = command([nm, '-g', '--defined-only', reloc], label + '-defined')
            undefined = command([nm, '-u', reloc], label + '-undefined')
            command([readelf, '-h', '-A', '-S', reloc], label + '-ELF')
            relocations = command([readelf, '-r', '-W', reloc], label + '-relocations')
            names = [line.split()[-1] for line in undefined.splitlines() if line.strip()]
            referenced = [name for name in names if any(name in line.split() for line in relocations.splitlines())]
            report.setdefault('undefined', {})[label] = referenced
            if any(name.startswith(('camera_', 'candidate_', 'nh_', 'qc_', 'qw_', 'rv_', 'is_rv_', 'mbedtls_')) for name in referenced):
                raise RuntimeError('unresolved actual project dependency')
            if any(defined.split().count(name) != 1 for name in roots):
                raise RuntimeError('missing/duplicate actual feature API')
            if 'main' in defined.split():
                raise RuntimeError('unexpected main')
            for path in (*objects, archive, reloc):
                report['artifacts'][str(path)] = {**pin(path), 'executed': False}
        for dep in sorted((out / 'objects').glob('*.d')):
            for name in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][name] = pin(Path(name))
        for name, expected in report['inputs'].items():
            if pin(Path(name)) != expected:
                raise RuntimeError('build source changed: ' + name)
        report['frozen_inputs_unchanged'] = all(pin(ROOT / name) == expected for name, expected in frozen.items())
        if not report['frozen_inputs_unchanged']:
            raise RuntimeError('frozen original changed')
        report['status'] = 'HOST_ARM_COMPILE_ARCHIVE_REL_PASS_NOT_RUN'
    except BaseException as exc:
        report['status'] = 'FAILED_PRESERVED'
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
