#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""ONE offline unit: fresh runNN, external module ONLY, sealed inputs RO in bwrap.

Never configure/prepare/build a kernel, never execute ARM, never contact a target.
Failed commands and all source snapshots remain in the fresh evidence directory.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[2]
LANE = ROOT / 'evidence/delegated/20261020-white'
BASE = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
KERNEL = BASE / 'artifacts/native-security75-source01/linux'
SEALED = BASE / 'analysis/native-security75-build01/build'
TOOLCHAIN = ROOT / 'work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1'
PINS = {
    'bin/toolchain-wrapper': 'f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1',
    'bin/arm-buildroot-linux-musleabi-gcc.br_real': 'e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729',
    'arm-buildroot-linux-musleabi/sysroot/lib/libc.a': '0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5',
    'lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a': 'f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a',
}

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def now():
    return datetime.now(timezone.utc).isoformat()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', help='fresh runNN (refuses existing path)')
    args = parser.parse_args()
    if not re.fullmatch(r'run[0-9]{2,}', args.run):
        parser.error('fresh runNN required')
    LANE.mkdir(parents=True, exist_ok=True)
    if LANE.resolve() != LANE:
        parser.error('symlink evidence lane refused')
    out = LANE / args.run
    out.mkdir(exist_ok=False)
    for name in ('input', 'logs', 'tmp', 'module', 'kernel-metadata'):
        (out / name).mkdir()
    report = {'started_utc': now(), 'invocation': [sys.executable, *sys.argv],
              'scope': 'OFFLINE ONLY; external exact75 module + static CLI; no native access',
              'result': 'FAILED', 'native_executed': False, 'commands': [],
              'inputs_sha256': {}, 'toolchain_pins': PINS}
    own = {p.name: sha(p) for p in sorted(SOURCE.iterdir()) if p.is_file()}
    # Read-only public source artifacts only. No private config/device data.
    optical = ROOT / 'bringup/qc07f/optical-control-candidate'
    preserved = {str(p.relative_to(ROOT)): sha(p) for p in sorted(optical.iterdir()) if p.is_file()}
    for p in (SEALED / '.config', SEALED / 'Module.symvers'):
        preserved[str(p.relative_to(ROOT))] = sha(p)
    env = {'PATH': os.environ.get('PATH', '/usr/bin:/bin'), 'LC_ALL': 'C',
           'TMPDIR': str(out / 'tmp'), 'PYTHONDONTWRITEBYTECODE': '1',
           'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
           'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1',
           'KBUILD_BUILD_TIMESTAMP': '2026-10-20 00:00:00 UTC',
           'KBUILD_BUILD_USER': 'offline', 'KBUILD_BUILD_HOST': 'offline'}

    def run(label, argv, timeout=120):
        cmd = ['bwrap', '--ro-bind', '/', '/', '--dev', '/dev', '--bind', str(out), str(out),
               '--unshare-net', '--die-with-parent', '--', *map(str, argv)]
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': cmd, 'cwd': str(out), 'started_utc': now(),
                 'timeout_seconds': timeout, 'log': str(log.relative_to(out))}
        report['commands'].append(entry)
        with log.open('xb') as stream:
            result = subprocess.run(cmd, cwd=out, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=timeout)
        entry.update(returncode=result.returncode, finished_utc=now(), sha256=sha(log))
        if result.returncode:
            raise RuntimeError(f'{label} failed; preserved {log}')
        return log.read_text()

    try:
        for name, digest in own.items():
            shutil.copy2(SOURCE / name, out / 'input' / name)
            assert sha(out / 'input' / name) == digest
            report['inputs_sha256'][str((SOURCE / name).relative_to(ROOT))] = digest
        prepared = out / 'kernel-metadata'
        for name in ('.config', 'Module.symvers'):
            shutil.copy2(SEALED / name, prepared / name)
        for name in ('include', 'scripts', 'arch/arm/include'):
            shutil.copytree(SEALED / name, prepared / name, symlinks=False)
        for root in (KERNEL / 'arch/arm/mach-ak3918ev200', prepared):
            for p in sorted(root.rglob('*')):
                if p.is_file():
                    report['inputs_sha256'][str(p)] = sha(p)
        for rel in ('Makefile', 'include/linux/gpio/consumer.h', 'include/linux/gpio/driver.h',
                    'include/linux/hrtimer.h', 'drivers/gpio/gpiolib.c'):
            report['inputs_sha256'][str(KERNEL / rel)] = sha(KERNEL / rel)
        for rel, digest in PINS.items():
            if sha(TOOLCHAIN / rel) != digest:
                raise RuntimeError('toolchain pin mismatch: ' + rel)
        armcc = TOOLCHAIN / 'bin/arm-buildroot-linux-musleabi-gcc'
        readelf = TOOLCHAIN / 'bin/arm-buildroot-linux-musleabi-readelf'
        for name, tool in (('clang', 'clang'), ('lld', 'ld.lld'), ('make', 'make'),
                           ('python', 'python3'), ('bwrap', 'bwrap'), ('armcc', armcc),
                           ('arm-readelf', readelf), ('llvm-readelf', 'llvm-readelf')):
            report.setdefault('versions', {})[name] = run(name + '-version', [tool, '--version'])
            resolved = Path(shutil.which(str(tool)) or tool).resolve()
            report.setdefault('build_tool_sha256', {})[str(resolved)] = sha(resolved)
        report['arm_target'] = run('arm-target', [armcc, '-dumpmachine']).strip()
        report['license_metadata'] = {
            'summary_sha256': sha(TOOLCHAIN / 'summary.csv'),
            'selected_lines': [line for line in (TOOLCHAIN / 'summary.csv').read_text().splitlines()
                               if line.startswith(('"gcc-final",', '"musl",', '"binutils",'))],
            'own_sources': 'GPL-2.0-or-later; UAPI Linux-syscall-note exception',
        }
        common = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Wconversion', '-Wshadow',
                  '-Wformat=2', '-Wstrict-prototypes', '-fno-common']
        for label, flags in (('optimized', ['-O2']),
                             ('sanitized', ['-O1', '-g', '-fsanitize=address,undefined',
                                            '-fno-omit-frame-pointer'])):
            for test in ('host_policy_test', 'test_cli'):
                host = out / (test + '-' + label)
                run(test + '-' + label + '-build', ['clang', *common, *flags,
                                                    out / 'input' / (test + '.c'), '-o', host])
                argv = [host] if test == 'host_policy_test' else ['python3', '-B', out / 'input/test_cli.py', host]
                result = run(test + '-' + label, argv)
                report.setdefault('host_tests', {})[test + '-' + label] = result.splitlines()[-1]
        module = out / 'module'
        run('module-build', ['make', '-C', KERNEL, f'O={prepared}', f'M={SOURCE}', f'MO={module}',
                             'ARCH=arm', 'LLVM=1', 'LLVM_IAS=1', 'W=1', 'modules'])
        run('module-readelf', ['llvm-readelf', '-h', '-p', '.modinfo', module / 'qc07f_white.ko'])
        obj, binary = out / 'white_cli.o', out / 'qc07f-white-cli'
        armflags = ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft']
        run('arm-object', [armcc, *common, *armflags, '-c', out / 'input/white_cli.c', '-o', obj])
        run('arm-object-readelf', [readelf, '-h', '-A', obj])
        run('arm-static', [armcc, *armflags, '-static', '-Wl,--build-id=none',
                           '-Wl,-Map=' + str(out / 'arm-static.map'), obj, '-o', binary])
        elf = run('arm-readelf', [readelf, '-h', '-A', '-l', '-d', binary])
        if 'Machine:                           ARM' not in elf or 'INTERP' in elf or 'NEEDED' in elf:
            raise RuntimeError('not static ARM ELF')
        report['artifacts'] = {str(p.relative_to(out)): {'sha256': sha(p), 'bytes': p.stat().st_size,
                                                       'executed': False}
                               for p in (module / 'qc07f_white.ko', obj, binary)}
        report['runtime_attributes_limit'] = ('Own object ARM-mode v5TE; pinned static runtime may merge '
            'v5TEJ/Thumb-2 metadata. Native ISA/ABI acceptance OPEN.')
        report['result'] = 'OFFLINE_BUILD_POLICY_CLI_PASS_NOT_NATIVE'
    except BaseException as error:
        report['failure'] = {'type': type(error).__name__, 'message': str(error)}
        raise
    finally:
        report['source_unchanged'] = own == {p.name: sha(p) for p in SOURCE.iterdir() if p.is_file()}
        report['preserved_inputs_sha256'] = preserved
        report['preserved_inputs_unchanged'] = all(sha(ROOT / name) == digest for name, digest in preserved.items())
        if not report['source_unchanged'] or not report['preserved_inputs_unchanged']:
            report['result'] = 'FAILED_INPUT_CHANGED'
        report['finished_utc'] = now()
        with (out / 'REPORT.json').open('x') as stream:
            json.dump(report, stream, indent=2)
            stream.write('\n')
    if report['result'] != 'OFFLINE_BUILD_POLICY_CLI_PASS_NOT_NATIVE':
        raise RuntimeError(report['result'])
    print(report['result'], str(out.relative_to(ROOT)), report['artifacts'])

if __name__ == '__main__':
    main()
