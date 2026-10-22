#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline compiler/linker/ELF inspection ONLY; never executes client outputs."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[3]
APP = Path(__file__).resolve().parent
DOC = ROOT / 'docs/qualification-plans/encoder-query77-client-20261022'
F = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
K = F / 'artifacts/encoder-query77-source02/linux'
PLATFORM = K / 'arch/arm/mach-ak3918ev200'
PIN = 'ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520'
ARM_PINS = {
    'bin/toolchain-wrapper': 'f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1',
    'bin/arm-buildroot-linux-musleabi-gcc.br_real': 'e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729',
    'arm-buildroot-linux-musleabi/sysroot/lib/libc.a': '0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5',
    'lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a': 'f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a',
}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def pin(path):
    data = path.read_bytes()
    result = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    if path.is_symlink():
        result['symlink'] = os.readlink(path)
        result['resolved'] = str(path.resolve())
    return result


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def dependencies(path):
    text = path.read_text().replace('\\\n', ' ')
    return [Path(name) for name in shlex.split(text.split(':', 1)[1])]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run')
    parser.add_argument('--toolchain', type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'build[0-9]{2}', args.run):
        parser.error('fresh buildNN required')
    toolchain = args.toolchain.resolve(strict=True)
    out = APP / args.run
    if APP.is_symlink() or out.parent.resolve() != APP:
        raise RuntimeError('source/output symlink refused')
    out.mkdir()  # Never overwrite/reuse a previous failed or successful build.
    for name in ('logs', 'inputs', 'tmp', 'host', 'arm'):
        (out / name).mkdir()
    report = {'started': now(), 'status': 'BUILDING', 'scope': 'COMPILE_STATIC_LINK_ELF_ONLY',
              'commands': [], 'artifacts': {}, 'outputs_executed': False,
              'tests_executed': False, 'target_private_IO': False,
              'Git_index_central_plan_change': False, 'native_admitted': False}
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1792627200',
           'TMPDIR': str(out / 'tmp')}
    report['environment'] = env
    before = {}

    def remember(path):
        path = path.absolute()
        item = pin(path)
        if str(path) in before and before[str(path)] != item:
            raise RuntimeError('input changed: ' + str(path))
        before[str(path)] = item

    def command(argv, label):
        argv = list(map(str, argv))
        executable = Path(argv[0]).resolve(strict=True)
        # Only compiler / ELF inspection tools discovered below, never artifacts.
        if executable not in allowed:
            raise RuntimeError('not an admitted build/ELF tool: ' + str(executable))
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'cwd': str(ROOT), 'started': now(), 'log': str(log),
                 'timeout_seconds': 120}
        report['commands'].append(entry)
        run_env = dict(env, LD_DEBUG='files', LD_DEBUG_OUTPUT=str(out / 'logs' / (label + '-loader')))
        with log.open('xb') as stream:
            result = subprocess.run(argv, cwd=ROOT, env=run_env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=120, check=False)
        entry.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('build/inspection failed: ' + label)
        return log.read_text()

    allowed = set()
    try:
        old_seal = ROOT / 'docs/qualification-plans/encoder-identity-20261022/SOURCE_SEAL01.json'
        frozen = json.loads(old_seal.read_bytes())
        aggregate = hashlib.sha256()
        for name, expected in sorted(frozen['source_files'].items()):
            path = ROOT / name
            if pin(path)['sha256'] != expected['sha256']:
                raise RuntimeError('frozen query input changed: ' + name)
            aggregate.update(name.encode() + b'\0' + bytes.fromhex(expected['sha256']))
            remember(path)
        if aggregate.hexdigest() != PIN or frozen['source_tree_sha256'] != PIN:
            raise RuntimeError('query source aggregate mismatch')
        original = ROOT / 'apps/qualification/kernel76-encoder-identity/encoder-open-query.h'
        if (APP / 'encoder-open-query.h').read_bytes() != original.read_bytes() or \
           (PLATFORM / 'encoder-open-query.h').read_bytes() != original.read_bytes():
            raise RuntimeError('UAPI byte import mismatch')
        report['query_source_pin'] = PIN
        report['UAPI_import'] = {'original': str(original), **pin(original)}
        for relative, expected in ARM_PINS.items():
            if pin(toolchain / relative)['sha256'] != expected:
                raise RuntimeError('retained ARM pin mismatch: ' + relative)
        report['retained_ARM_pins'] = ARM_PINS
        # Full retained toolchain bytes/relative symlinks pinned before tool use,
        # including compiler support, assembler/linker, crt, archives and headers.
        for path in sorted(toolchain.rglob('*')):
            if path.is_file():
                remember(path)
        report['toolchain_root'] = str(toolchain)
        custody = [old_seal,
                   ROOT / 'docs/qualification-plans/encoder-query77-build-20261022/BUILD_RESULT.md',
                   ROOT / 'docs/qualification-plans/encoder-query77-build-20261022/PARENT_DISPOSITION.md',
                   ROOT / 'docs/qualification-plans/encoder-identity-clang2313-20261022/PARENT_REVIEW.md',
                   ROOT / 'bringup/qc07f/initramfs/init.c',
                   PLATFORM / 'qc07f_hwenc.c', PLATFORM / 'qc07f_hwenc.h',
                   PLATFORM / 'encoder-open-query.h', PLATFORM / 'encoder-open-query.inc',
                   PLATFORM / 'watchdog.c', PLATFORM / 'stage-telemetry.h',
                   K / 'drivers/watchdog/watchdog_dev.c', K / 'drivers/watchdog/watchdog_core.c',
                   K / 'include/uapi/linux/watchdog.h', K / 'LICENSES/preferred/GPL-2.0',
                   F / 'analysis/encoder-query77-build02/build/.config',
                   F / 'artifacts/encoder-query77-images02/QC6EQ77.UIM']
        custody += list(APP.glob('*')) + list(DOC.glob('*.md'))
        for name in ('WATCHDOG_RECOVERY_REVIEW.md', 'WATCHDOG_SMOKE22B_PLAN.md',
                     'WATCHDOG_RESET22B_RESULT.md'):
            custody.append(ROOT / 'docs/qualification-plans/kernel76-transport-20261021' / name)
        for path in custody:
            if path.is_file():
                remember(path)
        if pin(F / 'artifacts/encoder-query77-images02/QC6EQ77.UIM')['sha256'] != \
           'f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e':
            raise RuntimeError('query77 image mismatch')
        for name in ('qc07f_hwenc.c', 'qc07f_hwenc.h', 'encoder-open-query.h', 'encoder-open-query.inc'):
            expected = frozen['evidence_pins'].get(
                'docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate/' + name)
            if expected and pin(PLATFORM / name)['sha256'] != expected['sha256']:
                raise RuntimeError('real kernel query candidate mismatch: ' + name)
        old_binding = ROOT / 'docs/qualification-plans/kernel76-transport-20261021/recovery-source-review/binding.json'
        for name, expected in json.loads(old_binding.read_bytes())['input_sha256'].items():
            for suffix in ('arch/arm/mach-ak3918ev200/watchdog.c',
                           'drivers/watchdog/watchdog_dev.c', 'drivers/watchdog/watchdog_core.c'):
                if name.endswith('/' + suffix) and pin(K / suffix)['sha256'] != expected:
                    raise RuntimeError('watchdog differs from source-bound historical review')
        remember(old_binding)
        prefix = toolchain / 'bin/arm-buildroot-linux-musleabi-'
        platforms = [('host', Path('/usr/bin/cc'), Path('/usr/bin/readelf'), ['-O2']),
                     ('arm', Path(str(prefix) + 'gcc'), Path(str(prefix) + 'readelf'),
                      ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft'])]
        for _, cc, elf, _ in platforms:
            allowed.update((cc.resolve(), elf.resolve()))
            remember(cc)
            remember(elf)
        save(out / 'PRE_INPUTS.json', before)
        common = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-Wpedantic',
                  '-fno-common', '-fno-strict-aliasing', '-ffunction-sections',
                  '-fdata-sections', '-fstack-protector-strong', '-fstack-usage',
                  '-Wframe-larger-than=8192', '-Wstack-usage=8192',
                  '-I' + str(APP), '-ffile-prefix-map=' + str(ROOT) + '=WORKSPACE']
        source = APP / 'encoder-query77-qualification.c'
        for name, cc, elf, flags in platforms:
            directory = out / name
            command([cc, '--version'], name + '-compiler-version')
            command([elf, '--version'], name + '-readelf-version')
            for program in ('cc1', 'collect2', 'as', 'ld'):
                value = command([cc, '-print-prog-name=' + program], name + '-locate-' + program).strip()
                path = Path(value) if '/' in value else Path('/usr/bin') / value
                remember(path)
            for library in ('crt1.o', 'crti.o', 'crtbeginT.o', 'crtend.o', 'crtn.o', 'libgcc.a', 'libgcc_eh.a', 'libc.a'):
                path = Path(command([cc, '-print-file-name=' + library],
                                    name + '-locate-' + library.replace('.', '-')).strip())
                if not path.is_absolute():
                    raise RuntimeError('unresolved link input: ' + library)
                remember(path)
            dep = directory / 'pre.d'
            command([cc, *common, *flags, '-M', '-MF', dep, source], name + '-dependencies')
            consumed = dependencies(dep)
            for path in consumed:
                remember(path)
                destination = out / 'inputs' / path.absolute().relative_to('/')
                destination.parent.mkdir(parents=True, exist_ok=True)
                if not destination.exists():
                    with destination.open('xb') as stream:
                        stream.write(path.read_bytes())
            obj = directory / 'encoder-query77-qualification.o'
            command([cc, *common, *flags, '-MD', '-MF', directory / 'actual.d',
                     '-c', source, '-o', obj], name + '-compile')
            if set(map(str, dependencies(directory / 'actual.d'))) != set(map(str, consumed)):
                raise RuntimeError('pre/actual header closure differs')
            executable = directory / 'encoder-query77-qualification'
            command([cc, *flags, '-static', '-no-pie', obj, '-Wl,--gc-sections',
                     '-Wl,-Map=' + str(directory / 'link.map'), '-Wl,-t', '-o', executable],
                    name + '-static-link')
            # Read actual link trace/map inputs, including linker scripts/support.
            for line in (out / 'logs' / (name + '-static-link.txt')).read_text().splitlines():
                path = Path(line.strip())
                if path.is_absolute() and path.is_file() and not path.is_relative_to(out):
                    remember(path)
            for match in re.finditer(r'^LOAD (.+)$', (directory / 'link.map').read_text(), re.M):
                path = Path(match.group(1))
                if path.is_file() and not path.is_relative_to(out):
                    remember(path)
            inspection = command([elf, '-h', '-l', '-d', '-A', '-sW', executable], name + '-elf')
            if 'INTERP' in inspection or '(NEEDED)' in inspection:
                raise RuntimeError('static ELF has interpreter/dependency')
            if name == 'arm' and ('ARM' not in inspection or 'Version5 EABI' not in inspection):
                raise RuntimeError('not ARM EABI5')
            report['artifacts'][name] = {'path': str(executable), **pin(executable),
                                         'executed': False, 'headers': len(consumed)}
        # Loader metadata is compiler closure, not client/runtime tracing. These
        # paths are final-observed pins, not misrepresented as all pre-use hashes.
        loaded = {}
        for trace in sorted((out / 'logs').glob('*-loader.*')):
            for match in re.finditer(r'file=(/\S+) \[\d+\];\s+generating link map', trace.read_text()):
                path = Path(match.group(1))
                if path.is_file():
                    loaded[str(path)] = pin(path)
        save(out / 'LOADED_TOOL_ELF_FINAL.json', loaded)
        save(out / 'INPUTS.json', before)
        for name, expected in before.items():
            if pin(Path(name)) != expected:
                raise RuntimeError('build input changed: ' + name)
        report.update(status='PASS', all_captured_inputs_unchanged=True,
                      captured_input_paths=len(before), loaded_tool_elf_paths=len(loaded),
                      ended=now())
    except Exception as error:
        report.update(status='FAILED', error=str(error), ended=now())
        raise
    finally:
        save(out / 'REPORT.json', report)
        files = {str(path.relative_to(out)): pin(path) for path in sorted(out.rglob('*')) if path.is_file()}
        save(out / 'OUTPUT_SEAL.json', files)


if __name__ == '__main__':
    main()
