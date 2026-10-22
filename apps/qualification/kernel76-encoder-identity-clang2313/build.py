#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Explicit NEW Clang23.1.3 source-object baseline; not retained76 reproduction."""
import datetime
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
OLD = ROOT / 'docs/qualification-plans/encoder-identity-20261022'
FROZEN = OLD / 'runs/build01'
SEAL = OLD / 'SOURCE_SEAL01.json'
BASE = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
META = BASE / 'analysis/native-usb76-package01/build'
BOARD = BASE / 'artifacts/native-usb76-source01/linux/arch/arm/mach-ak3918ev200'
DOC = ROOT / 'docs/qualification-plans/encoder-identity-clang2313-20261022'


def pin(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for data in iter(lambda: stream.read(65536), b''):
            h.update(data)
    return {'bytes': path.stat().st_size, 'sha256': h.hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def inventory(directory):
    return {str(p.relative_to(ROOT)): pin(p) for p in sorted(directory.rglob('*')) if p.is_file()}


def dependencies(path):
    return {Path(p).absolute() for p in shlex.split(
        path.read_text().replace('\\\n', '').split(':', 1)[1])}


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', sys.argv[1]):
        raise SystemExit('usage: build.py <fresh-run-name>')
    seal = json.loads(SEAL.read_text())
    if seal['source_tree_sha256'] != 'ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520':
        raise RuntimeError('unexpected frozen encoder source seal')
    frozen_pins = {**seal['source_files'], **seal['evidence_pins']}
    for name, expected in frozen_pins.items():
        if pin(ROOT / name) != expected:
            raise RuntimeError('frozen source/evidence changed: ' + name)
    old_report = json.loads((FROZEN / 'REPORT.json').read_text())
    for name, expected in old_report['inputs'].items():
        if pin(ROOT / name) != expected:
            raise RuntimeError('retained76 metadata/source input changed: ' + name)
    old_tree = inventory(FROZEN)
    parent = DOC / 'runs'
    parent.mkdir(parents=True, exist_ok=True)
    if parent.resolve() != parent:
        raise RuntimeError('output parent symlink refused')
    out = parent / sys.argv[1]
    out.mkdir()
    for name in ('source', 'inputs', 'headers', 'objects', 'logs', 'tmp', 'home'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp'), 'HOME': str(out / 'home')}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'compiler_baseline': 'NEW EXPLICIT Clang23.1.3, NOT retained76 Clang23.1.1',
              'scope': 'EXACT FROZEN QUERY; REAL ARM TRANSLATION UNIT OBJECT ONLY',
              'frozen_source_pin': seal['source_tree_sha256'], 'frozen_build01_tree_before': old_tree,
              'environment': env, 'loaded_ELF_images': {}, 'artifacts': {},
              'warning_or_configuration_suppression_added': False,
              'generated_metadata_replaced': False, 'linker_invoked': False,
              'UAPI_declarations_recompiled': False, 'kernel_module_image_build': False,
              'outputs_or_tests_executed': False, 'target_or_private_IO': False,
              'retained76_compiler_pairing': False, 'native_adoption': False,
              'native_memory_admission': 'HOLD', 'G2': False, 'G3': False, 'G8': False}

    def copy(path, destination):
        before = pin(path)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open('xb') as target, path.open('rb') as source:
            for block in iter(lambda: source.read(65536), b''):
                target.write(block)
        if pin(destination) != before:
            raise RuntimeError('input changed during copy')
        report['inputs'][str(path)] = before

    def command(argv, label, trace=False):
        argv = list(map(str, argv))
        stdout = out / 'logs' / (label + '.stdout')
        stderr = out / 'logs' / (label + '.stderr')
        run_env = dict(env)
        if trace:
            # Trace THIS host compiler's actual loader, not proc/status or target
            # IO. Original diagnostics stay byte-exact beside loader messages.
            run_env['LD_DEBUG'] = 'files,libs'
        entry = {'argv': argv, 'cwd': str(ROOT), 'environment': run_env,
                 'started': now(), 'stdout': str(stdout), 'stderr': str(stderr)}
        report['commands'].append(entry)
        with stdout.open('xb') as output, stderr.open('xb') as errors:
            result = subprocess.run(argv, cwd=ROOT, env=run_env, stdout=output,
                                    stderr=errors, timeout=120)
        entry.update(exit=result.returncode, ended=now(), stdout_pin=pin(stdout), stderr_pin=pin(stderr))
        trace_text = stderr.read_text(errors='replace')
        if trace:
            images = set(re.findall(r'calling (?:init|fini):\s+(/\S+)', trace_text))
            images.update(re.findall(r'file=(/\S+)\s+\[\d+\];\s+generating link map', trace_text))
            for name in sorted(images):
                p = Path(name)
                if not p.is_file():
                    raise RuntimeError('traced compiler image missing: ' + name)
                report['loaded_ELF_images'][name] = {'resolved': str(p.resolve()), **pin(p)}
        if result.returncode:
            raise RuntimeError('real command failed; stop without altered flags: ' + label)
        return stdout.read_text()

    try:
        copy(SEAL, out / 'inputs/frozen-SOURCE_SEAL01.json')
        copy(FROZEN / 'REPORT.json', out / 'inputs/frozen-failed-build01-REPORT.json')
        copy(FROZEN / 'applied-source.patch', out / 'inputs/frozen-applied-source.patch')
        for name, expected in old_report['candidate_files'].items():
            source = FROZEN / 'candidate' / name
            if pin(source) != expected:
                raise RuntimeError('frozen candidate source/header changed: ' + name)
            copy(source, out / 'source' / name)
        for path in sorted(APP.glob('*')):
            if path.is_file():
                copy(path, out / 'inputs/new-builder' / path.name)
        cmd_file = META / 'arch/arm/mach-ak3918ev200/.qc07f_hwenc.o.cmd'
        for path in (cmd_file, META / '.config', META / 'Module.symvers',
                     META / 'include/generated/compile.h', META / 'include/generated/autoconf.h',
                     META / 'include/config/kernel.release', BOARD.parents[2] / 'COPYING'):
            copy(path, out / 'inputs/retained-metadata' / path.relative_to(ROOT))
        compiler = Path('/usr/bin/clang')
        report['compiler_executable'] = {'path': str(compiler), 'resolved': str(compiler.resolve()), **pin(compiler)}
        version = command([compiler, '--version'], 'compiler-version', trace=True)
        if not version.startswith('clang version 23.1.3\n'):
            raise RuntimeError('explicit NEW baseline is Clang23.1.3 only')
        report['compiler_version'] = version
        resource = Path(command([compiler, '-print-resource-dir'], 'compiler-resource-dir', trace=True).strip())
        if not resource.is_absolute() or not (resource / 'include').is_dir():
            raise RuntimeError('unexpected actual compiler resource directory')
        report['compiler_resource_directory'] = str(resource)
        # Pin resource headers without changing the compiler's resource/sysroot
        # selection. -nostdinc is retained; actual consumed resource files are
        # separately identified in the real dependency closure below.
        report['resource_header_inventory'] = {str(p): pin(p) for p in
            sorted((resource / 'include').rglob('*')) if p.is_file()}
        elf = command(['/usr/bin/readelf', '-l', '-d', compiler], 'compiler-ELF')
        match = re.search(r'Requesting program interpreter:\s+([^\]]+)', elf)
        if not match:
            raise RuntimeError('actual compiler ELF interpreter not found')
        loader = Path(match.group(1).strip())
        report['ELF_interpreter'] = {'path': str(loader), 'resolved': str(loader.resolve()), **pin(loader)}
        original = shlex.split(cmd_file.read_text().splitlines()[0].split(' := ', 1)[1])
        report['retained_Kbuild_argv'] = original
        if original[0] != 'clang' or '--target=arm-linux-gnueabi' not in original or '-nostdinc' not in original:
            raise RuntimeError('unexpected retained real driver command')
        flags, changes = [], []
        index = 1
        while index < len(original):
            value = original[index]
            if value == '-o':
                changes.append({'original_output': original[index + 1], 'replacement': 'fresh owned output'})
                index += 2
                continue
            if value in ('-c', str(BOARD / 'qc07f_hwenc.c')) or value.startswith('-Wp,-MMD,'):
                changes.append({'path_or_output_operand_removed': value})
                index += 1
                continue
            if value.startswith('-I') and not Path(value[2:]).is_absolute():
                rebased = '-I' + str((META / value[2:]).resolve())
                changes.append({'include_before': value, 'include_after': rebased})
                value = rebased
            flags.append(value)
            index += 1
        report['command_path_changes_only'] = changes
        prefix = [compiler, '-I' + str(out / 'source'), *flags]
        source = out / 'source/qc07f_hwenc.c'
        # Obtain the genuine compiler's header set first, so every consumed file
        # has a precompile pin/snapshot. Errors here stop, never trigger stubs.
        predep = out / 'objects/precompile.d'
        command([*prefix, '-M', '-MF', predep, '-MT', 'driver', source], 'driver-dependencies', trace=True)
        before = {str(p): pin(p) for p in sorted(dependencies(predep))}
        for name in before:
            p = Path(name)
            copy(p, out / 'headers' / p.relative_to('/'))
        report['header_source_closure_before_compile'] = before
        obj = out / 'objects/encoder-identity-clang2313-ARM.o'
        dependency = out / 'objects/driver.d'
        command([*prefix, '-MD', '-MF', dependency, '-c', source, '-o', obj],
                'actual-driver-ARM-object', trace=True)
        after = {str(p): pin(p) for p in sorted(dependencies(dependency))}
        if before != after:
            raise RuntimeError('consumed source/header set or pins changed during compile')
        report['consumed_header_source_closure'] = after
        report['consumed_resource_files'] = [n for n in after if Path(n).is_relative_to(resource)]
        report['artifacts'][obj.name] = {**pin(obj), 'executed': False}
        command(['/usr/bin/readelf', '-h', '-A', '-s', obj], 'object-ELF-symbols')
        report['inspection_tool'] = {'path': '/usr/bin/readelf', **pin(Path('/usr/bin/readelf').resolve())}
        report['status'] = 'NEW CLANG23.1.3 REAL ARM SOURCE OBJECT COMPILE PASS; NOT RETAINED76 PAIRING'
    except BaseException as exc:
        report['status'] = 'FAILED; exact logs/partial outputs preserved; no altered-flags retry'
        report['error'] = type(exc).__name__ + ': ' + str(exc)
        raise
    finally:
        report['frozen_build01_tree_unchanged'] = inventory(FROZEN) == old_tree
        report['frozen_sealed_files_unchanged'] = all(pin(ROOT / n) == p for n, p in frozen_pins.items())
        report['inputs_unchanged'] = all(pin(Path(n)) == p for n, p in report['inputs'].items())
        report['loaded_ELF_images_unchanged'] = all(pin(Path(n)) == {'bytes': p['bytes'], 'sha256': p['sha256']}
            for n, p in report['loaded_ELF_images'].items())
        report['compiler_executable_unchanged'] = ('compiler_executable' not in report or
            pin(Path(report['compiler_executable']['path'])) ==
            {k: report['compiler_executable'][k] for k in ('bytes', 'sha256')})
        report['ended'] = now()
        with (out / 'REPORT.json').open('x') as stream:
            json.dump(report, stream, indent=2)
            stream.write('\n')
        print(out / 'REPORT.json')


if __name__ == '__main__':
    main()
