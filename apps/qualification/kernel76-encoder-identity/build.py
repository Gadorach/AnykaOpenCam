#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh source overlay + UAPI HOST/ARM and real ARM driver compile only."""
import argparse
import ast
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
BASE = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
KERNEL = BASE / 'artifacts/native-usb76-source01/linux'
BOARD = KERNEL / 'arch/arm/mach-ak3918ev200'
META = BASE / 'analysis/native-usb76-package01/build'
DOC = ROOT / 'docs/qualification-plans/encoder-identity-20261022'
PIN_C = 'e97744838acd469772db41fb1fe0907c95fb6e923577cdbffc84ead0141a026f'
PIN_H = '6723c30c76a3fbd1feaf1ed9d1676a2b9b6289b352ffe7682372bf8e95123731'


def pin(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('overlay context is not unique')
    return text.replace(old, new)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run')
    parser.add_argument('--toolchain', type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', args.run):
        parser.error('simple fresh name required')
    for name, expected in (('qc07f_hwenc.c', PIN_C), ('qc07f_hwenc.h', PIN_H)):
        if pin(BOARD / name)['sha256'] != expected:
            raise RuntimeError('retained76 encoder input changed: ' + name)
    package = BASE / 'analysis/native-usb76-package01/package76-manifest.json'
    package_data = json.loads(package.read_text())
    for name in ('.config', 'Module.symvers'):
        path = META / name
        expected = package_data['output_sha256'][str(path.relative_to(ROOT))]
        if pin(path)['sha256'] != expected:
            raise RuntimeError('retained final76 metadata changed: ' + name)
    tool_pins = next(ast.literal_eval(n.value) for n in ast.parse(
        (ROOT / 'apps/rtsp-auth/build.py').read_text()).body if
        isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and
        n.targets[0].id == 'ARM_PINS')
    toolchain = args.toolchain.resolve()
    for name, expected in tool_pins.items():
        if pin(toolchain / name)['sha256'] != expected:
            raise RuntimeError('retained ARM toolchain pin mismatch: ' + name)
    parent = DOC / 'runs'
    parent.mkdir(parents=True, exist_ok=True)
    if parent.resolve() != parent:
        raise RuntimeError('output-parent symlink refused')
    out = parent / args.run
    out.mkdir()
    for name in ('original', 'candidate', 'inputs', 'objects', 'logs', 'tmp'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C',
           'SOURCE_DATE_EPOCH': '1791763200', 'TMPDIR': str(out / 'tmp')}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'candidate_files': {}, 'artifacts': {}, 'environment': env,
              'scope': 'FRESH SOURCE OVERLAY; UAPI HOST/ARM; REAL DRIVER ARM OBJECT ONLY',
              'full_kernel_or_module_build': False, 'outputs_executed': False,
              'tests_executed': False, 'target_or_private_IO': False,
              'provider_adoption': False, 'native_memory_admission': 'HOLD',
              'G2': False, 'G3': False, 'G8': False, 'ARM_toolchain_pins': tool_pins}

    def copy(path, destination):
        if path.is_symlink():
            raise RuntimeError('input symlink refused: ' + str(path))
        before = pin(path)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open('xb') as stream:
            stream.write(path.read_bytes())
        if pin(destination) != before:
            raise RuntimeError('input changed while copying')
        report['inputs'][str(path.relative_to(ROOT))] = before

    def command(argv, label):
        argv = list(map(str, argv))
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'cwd': str(ROOT), 'started': now(), 'log': str(log)}
        report['commands'].append(entry)
        with log.open('xb') as stream:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=120)
        entry.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('compile/link failed: ' + label)
        return log.read_text()

    try:
        names = ('qc07f_hwenc.c', 'qc07f_hwenc.h', 'dma_integrity.h',
                 'stage-telemetry.h', 'stage-telemetry-uapi.h')
        for name in names:
            copy(BOARD / name, out / 'original' / name)
        for path in sorted(APP.glob('*')):
            if path.is_file():
                copy(path, out / 'inputs/new' / path.name)
        cmd_file = META / 'arch/arm/mach-ak3918ev200/.qc07f_hwenc.o.cmd'
        for path in (META / '.config', META / 'Module.symvers', cmd_file,
                     META / 'include/generated/compile.h', META / 'include/generated/autoconf.h',
                     META / 'include/config/kernel.release', package,
                     ROOT / 'apps/rtsp-auth/build.py', KERNEL / 'COPYING'):
            copy(path, out / 'inputs/provenance' / path.relative_to(ROOT))
        c = (out / 'original/qc07f_hwenc.c').read_text()
        h = (out / 'original/qc07f_hwenc.h').read_text()
        changed_c = replace_once(c, 'static long encoder_ioctl(',
                                 '#include "encoder-open-query.inc"\nstatic long encoder_ioctl(')
        changed_c = replace_once(changed_c,
            ' int result;\n if(command==QC_HWENC_LIFETIME_QUERY){',
            ' int result;\n if(command==QC_HWENC_OPEN_QUERY)return encoder_open_query(file,argument);\n if(command==QC_HWENC_LIFETIME_QUERY){')
        changed_h = replace_once(h, '#endif\n', '#include "encoder-open-query.h"\n#endif\n')
        patch = ''
        for name, old, new in (('qc07f_hwenc.c', c, changed_c), ('qc07f_hwenc.h', h, changed_h)):
            (out / 'candidate' / name).write_text(new)
            patch += ''.join(difflib.unified_diff(old.splitlines(True), new.splitlines(True),
                fromfile='a/' + name, tofile='b/' + name))
        for name in names[2:]:
            (out / 'candidate' / name).write_bytes((out / 'original' / name).read_bytes())
        for name in ('encoder-open-query.h', 'encoder-open-query.inc'):
            text = (out / 'inputs/new' / name).read_text()
            (out / 'candidate' / name).write_text(text)
            patch += ''.join(difflib.unified_diff([], text.splitlines(True),
                fromfile='/dev/null', tofile='b/' + name))
        (out / 'applied-source.patch').write_text(patch)
        report['applied_source_patch'] = pin(out / 'applied-source.patch')
        report['candidate_files'] = {p.name: pin(p) for p in sorted((out / 'candidate').iterdir())}
        # UAPI declarations compile on HOST and retained ARM GCC. No main/test.
        prefix = str(toolchain / 'bin/arm-buildroot-linux-musleabi-')
        for mode, cc, flags in (('host', '/usr/bin/cc', ['-O2']),
                ('arm', prefix + 'gcc', ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft'])):
            command([cc, '--version'], mode + '-compiler-version')
            command([cc, '-std=c11', *flags, '-Wall', '-Wextra', '-Werror',
                     '-I' + str(out / 'candidate'), '-MD', '-MF', out / 'objects' / (mode + '-abi.d'),
                     '-c', out / 'inputs/new/abi_compile.c', '-o',
                     out / 'objects' / (mode + '-abi.o')], mode + '-abi-compile')
        # Reuse actual final76 Kbuild flags without invoking make or changing its
        # cwd/source/output. Only source, dependency/output paths and relative
        # include path rebasing differ; all original config guards stay enabled.
        original = shlex.split(cmd_file.read_text().splitlines()[0].split(' := ', 1)[1])
        original_source = str(BOARD / 'qc07f_hwenc.c')
        flags = []
        index = 1
        while index < len(original):
            value = original[index]
            if value == '-o':
                index += 2
                continue
            if value in ('-c', original_source) or value.startswith('-Wp,-MMD,'):
                index += 1
                continue
            if value.startswith('-I') and not Path(value[2:]).is_absolute():
                value = '-I' + str((META / value[2:]).resolve())
            flags.append(value)
            index += 1
        report['retained_Kbuild_argv'] = original
        version = command(['/usr/bin/clang', '--version'], 'driver-compiler-version')
        if 'clang version 23.1.1' not in version:
            raise RuntimeError('clang does not match retained76 compiler metadata')
        driver = out / 'objects/encoder-identity-arm.o'
        command(['/usr/bin/clang', '-I' + str(out / 'candidate'), *flags,
                 '-MD', '-MF', out / 'objects/driver.d', '-fstack-usage', '-c',
                 out / 'candidate/qc07f_hwenc.c', '-o', driver], 'actual-driver-ARM-compile')
        reloc = out / 'encoder-identity-ARM-NOT-EXECUTABLE.o'
        command(['/usr/bin/ld.lld', '-r', driver, '-o', reloc], 'driver-relocatable-link')
        command(['/usr/bin/llvm-readelf', '-h', '-A', '-s', reloc], 'driver-ELF-symbols')
        command(['/usr/bin/llvm-objdump', '-dr', driver], 'driver-disassembly')
        for mode, tool in (('host', '/usr/bin/readelf'), ('arm', prefix + 'readelf')):
            command([tool, '-h', '-A', out / 'objects' / (mode + '-abi.o')], mode + '-abi-ELF')
        for path in (out / 'objects/host-abi.o', out / 'objects/arm-abi.o', driver, reloc):
            report['artifacts'][str(path.relative_to(out))] = {**pin(path), 'executed': False}
        closure = {}
        for path in (out / 'objects').glob('*.d'):
            for name in shlex.split(path.read_text().replace('\\\n', '').split(':', 1)[1]):
                dependency = Path(name)
                if dependency.is_file():
                    closure[str(dependency)] = pin(dependency)
        report['consumed_header_source_closure'] = closure
        report['tools'] = {str(p): pin(p.resolve()) for p in map(Path,
            ('/usr/bin/cc', '/usr/bin/clang', '/usr/bin/ld.lld', '/usr/bin/llvm-readelf',
             '/usr/bin/llvm-objdump', prefix + 'gcc', prefix + 'readelf'))}
        report['inputs_unchanged'] = all(pin(ROOT / name) == value
                                         for name, value in report['inputs'].items())
        if not report['inputs_unchanged']:
            raise RuntimeError('retained input changed')
        report['status'] = 'SOURCE OVERLAY / HOST+ARM UAPI / REAL ARM DRIVER COMPILE PASS; NOT RUN'
    except BaseException as exc:
        report['status'] = 'FAILED; outputs preserved'
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
