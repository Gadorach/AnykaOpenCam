#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fixed offline UAPI/real-driver object recipe; never execute any output."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/qc07f-encoder-output-spans-20261024'
OLD = ROOT / 'docs/qualification-plans/encoder-identity-20261022'
FROZEN = OLD / 'runs/build01/candidate'
BASE = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
KERNEL = BASE / 'artifacts/native-usb76-source01/linux'
BOARD = KERNEL / 'arch/arm/mach-ak3918ev200'
META = BASE / 'analysis/native-usb76-package01/build'
TOOLCHAIN = ROOT / ('work/lightbulb-camera/implementation-20261008T205404Z/sources/'
                   'toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1')
SUPPORT = ('dma_integrity.h', 'stage-telemetry.h', 'stage-telemetry-uapi.h',
           'encoder-open-query.h', 'encoder-open-query.inc')
SOURCE = ('qc07f_hwenc.c', 'qc07f_hwenc.h', 'encoder-output-spans.h', 'encoder-output-spans.inc')


def pin(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(65536), b''):
            h.update(block)
    return {'bytes': path.stat().st_size, 'sha256': h.hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def deps(path):
    return sorted({str(Path(p).absolute()) for p in shlex.split(
        path.read_text().replace('\\\n', '').split(':', 1)[1])})


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,47}', sys.argv[1]):
        raise SystemExit('usage: build.py <explicit-fresh-run>')
    parent = DOC / 'runs'
    parent.mkdir(exist_ok=True)
    if parent.resolve() != parent:
        raise RuntimeError('output symlink refused')
    out = parent / sys.argv[1]
    out.mkdir()  # Exclusive: preserve every existing success/failure.
    for name in ('source', 'inputs', 'objects', 'logs', 'tmp', 'home'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp'), 'HOME': str(out / 'home')}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'tools': {}, 'dependencies': {}, 'artifacts': {}, 'environment': env,
              'compiler_baseline': 'NEW Clang23.1.3 SOURCE object; generated metadata remains23.1.1',
              'retained76_paired_ABI': False, 'full_toolchain_closure_claim': False,
              'linker_make_generator_image_module_invoked': False,
              'outputs_tests_candidate_functions_executed': False,
              'target_private_network_IO': False, 'application_adoption': False,
              'overall_allocation_cap_or_native_fit_accepted': False}

    def record(path):
        key = str(path)
        value = pin(path)
        if key in report['inputs'] and report['inputs'][key] != value:
            raise RuntimeError('input changed: ' + key)
        report['inputs'][key] = value

    def copy(path, dest):
        record(path)
        with dest.open('xb') as target:
            target.write(path.read_bytes())
        if pin(dest) != report['inputs'][str(path)]:
            raise RuntimeError('input changed during snapshot')

    def command(argv, label):
        argv = list(map(str, argv))
        tool = Path(argv[0])
        record(tool)
        report['tools'][str(tool)] = {'resolved': str(tool.resolve()), **pin(tool)}
        stdout, stderr = (out / 'logs' / (label + '.' + suffix) for suffix in ('stdout', 'stderr'))
        entry = {'argv': argv, 'cwd': str(ROOT), 'started': now(),
                 'stdout': str(stdout), 'stderr': str(stderr)}
        report['commands'].append(entry)
        with stdout.open('xb') as output, stderr.open('xb') as errors:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=output,
                                    stderr=errors, timeout=120)
        entry.update(exit=result.returncode, ended=now(), stdout_pin=pin(stdout), stderr_pin=pin(stderr))
        if result.returncode:
            raise RuntimeError('command failed; preserve and stop: ' + label)
        return stdout.read_text()

    try:
        seal = json.loads((OLD / 'SOURCE_SEAL01.json').read_text())
        if seal['source_tree_sha256'] != 'ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520':
            raise RuntimeError('unexpected prior source seal')
        for name, expected in {**seal['source_files'], **seal['evidence_pins']}.items():
            path = ROOT / name
            if pin(path) != expected:
                raise RuntimeError('old sealed input changed: ' + name)
            record(path)
        old = json.loads((OLD / 'runs/build01/REPORT.json').read_text())
        for name, expected in old['inputs'].items():
            path = ROOT / name
            if pin(path) != expected:
                raise RuntimeError('retained original/metadata changed: ' + name)
            record(path)
        for name, expected in old['candidate_files'].items():
            if pin(FROZEN / name) != expected:
                raise RuntimeError('prior seven-file baseline changed: ' + name)
            record(FROZEN / name)
        for name in SUPPORT:
            copy(FROZEN / name, out / 'source' / name)
        for name in SOURCE:
            copy(APP / name, out / 'source' / name)
        for name in ('abi_compile.c', 'build.py', 'README.md', 'LICENSE_NOTICES.md'):
            copy(APP / name, out / 'inputs' / name)
        copy(DOC / 'CONTRACT.md', out / 'inputs/CONTRACT.md')
        copy(DOC / 'SOURCE_DELTA.json', out / 'inputs/SOURCE_DELTA.json')
        copy(DOC / 'INPUTS.json', out / 'inputs/INPUTS.json')
        for name, expected in json.loads((DOC / 'INPUTS.json').read_text())['files'].items():
            if pin(ROOT / name) != expected:
                raise RuntimeError('read-only reference changed: ' + name)
            record(ROOT / name)
        # Exact literal inverse, not a synthetic behavior test or build generator.
        delta = json.loads((DOC / 'SOURCE_DELTA.json').read_text())
        forward, inverse = '', ''
        for name in ('qc07f_hwenc.c', 'qc07f_hwenc.h'):
            original = (FROZEN / name).read_text()
            current = (APP / name).read_text()
            restored = current
            for change in reversed(delta[name]):
                if restored.count(change['after']) != 1:
                    raise RuntimeError('nonunique literal reverse delta: ' + name)
                restored = restored.replace(change['after'], change['before'], 1)
            if restored != original:
                raise RuntimeError('inverse not exactly prior source: ' + name)
            forward += ''.join(difflib.unified_diff(original.splitlines(True), current.splitlines(True),
                                                   fromfile='a/' + name, tofile='b/' + name))
            inverse += ''.join(difflib.unified_diff(current.splitlines(True), original.splitlines(True),
                                                   fromfile='b/' + name, tofile='a/' + name))
        for name in ('encoder-output-spans.h', 'encoder-output-spans.inc'):
            text = (APP / name).read_text()
            forward += ''.join(difflib.unified_diff([], text.splitlines(True), fromfile='/dev/null', tofile='b/' + name))
            inverse += ''.join(difflib.unified_diff(text.splitlines(True), [], fromfile='b/' + name, tofile='/dev/null'))
        for name, text in (('source.patch', forward), ('inverse.patch', inverse)):
            with (out / name).open('x') as stream:
                stream.write(text)
        report['exact_literal_inverse_to_prior_baseline'] = True
        for name in ('source.patch', 'inverse.patch'):
            report['artifacts'][name] = pin(out / name)
        cmd_file = META / 'arch/arm/mach-ak3918ev200/.qc07f_hwenc.o.cmd'
        for path in (OLD / 'SOURCE_SEAL01.json', cmd_file, META / '.config', META / 'Module.symvers',
                     META / 'include/generated/compile.h', META / 'include/generated/autoconf.h',
                     META / 'include/config/kernel.release', KERNEL / 'COPYING',
                     ROOT / 'apps/qualification/kernel76-encoder-identity-clang2313/build.py',
                     ROOT / 'docs/qualification-plans/encoder-identity-clang2313-20261022/PARENT_REVIEW.md',
                     ROOT / 'docs/qualification-plans/encoder-identity-clang2313-20261022/BUILD_RESULT.md'):
            record(path)
        for name, expected in old['ARM_toolchain_pins'].items():
            path = TOOLCHAIN / name
            if pin(path)['sha256'] != expected:
                raise RuntimeError('retained ARM toolchain pin changed: ' + name)
            record(path)
        host, arm, clang = '/usr/bin/cc', str(TOOLCHAIN / 'bin/arm-buildroot-linux-musleabi-gcc'), '/usr/bin/clang'
        command([host, '--version'], 'host-version')
        command([arm, '--version'], 'arm-version')
        version = command([clang, '--version'], 'driver-version')
        if not version.startswith('clang version 23.1.3\n'):
            raise RuntimeError('explicit NEW Clang23.1.3 baseline required')
        original = shlex.split(cmd_file.read_text().splitlines()[0].split(' := ', 1)[1])
        if original[0] != 'clang' or '--target=arm-linux-gnueabi' not in original or '-nostdinc' not in original:
            raise RuntimeError('unexpected actual Kbuild command')
        flags, index = [], 1
        while index < len(original):
            value = original[index]
            if value == '-o':
                index += 2
                continue
            if value in ('-c', str(BOARD / 'qc07f_hwenc.c')) or value.startswith('-Wp,-MMD,'):
                index += 1
                continue
            if value.startswith('-I') and not Path(value[2:]).is_absolute():
                value = '-I' + str((META / value[2:]).resolve())
            flags.append(value)
            index += 1
        report['retained_Kbuild_argv'] = original
        modes = (
            ('host-abi', [host, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror'], out / 'inputs/abi_compile.c'),
            ('arm-abi', [arm, '-std=c11', '-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft',
                         '-Wall', '-Wextra', '-Werror'], out / 'inputs/abi_compile.c'),
            ('driver-ARM', [clang, *flags], out / 'source/qc07f_hwenc.c'))
        for label, prefix, source in modes:
            prefix = [*prefix, '-I' + str(out / 'source')]
            predep, dep = (out / 'objects' / (label + suffix) for suffix in ('.pre.d', '.d'))
            command([*prefix, '-M', '-MF', predep, '-MT', label, source], label + '-dependencies')
            before = {name: pin(Path(name)) for name in deps(predep)}
            for name in before:
                record(Path(name))
            obj = out / 'objects' / (label + '.o')
            command([*prefix, '-MD', '-MF', dep, '-c', source, '-o', obj], label + '-compile')
            after = {name: pin(Path(name)) for name in deps(dep)}
            if before != after:
                raise RuntimeError('dependency set/pin changed during compile: ' + label)
            report['dependencies'][label] = after
            report['artifacts'][str(obj.relative_to(out))] = {**pin(obj), 'executed': False}
            command(['/usr/bin/readelf', '-h', '-A', '-s', obj], label + '-ELF')
        report['status'] = 'PASS: HOST/ARM ABI + NEW Clang23.1.3 whole ARM driver object; NOT RUN'
    except BaseException as exc:
        report['status'] = 'FAILED_PRESERVED; no retry/altered flags'
        report['error'] = type(exc).__name__ + ': ' + str(exc)
        raise
    finally:
        report['inputs_unchanged'] = all(pin(Path(n)) == p for n, p in report['inputs'].items())
        if not report['inputs_unchanged']:
            report['status'] = 'FAILED_PRESERVED: an input changed'
        report['source_files'] = {p.name: pin(p) for p in sorted((out / 'source').iterdir())}
        report['ended'] = now()
        with (out / 'REPORT.json').open('x') as stream:
            json.dump(report, stream, indent=2)
            stream.write('\n')
        print(out / 'REPORT.json')


if __name__ == '__main__':
    main()
