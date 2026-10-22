#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile/archive/ELF-REL only. Never run a server, child, guard, test or fixture."""
import argparse
import ast
import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[3]
APP = Path(__file__).resolve().parent
DOC = ROOT / 'docs/qualification-plans/rtsp-owned-listener-20261022'
FROZEN = ROOT / 'apps/rtsp-auth/isolated-server-host-tail'
SEAL = ROOT / 'docs/qualification-plans/rtsp-tail-20261022/harness-correction/CANDIDATE_SOURCE_SEAL01.json'
PRIOR = ROOT / 'local/build/rtsp-app/build02/REPORT.json'
PIN = '2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f'


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def pin(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def aggregate(entries):
    digest = hashlib.sha256()
    for name, item in sorted(entries.items()):
        digest.update(name.encode() + b'\0' + bytes.fromhex(item['sha256']))
    return digest.hexdigest()


def frozen_check():
    entries = json.loads(SEAL.read_bytes())['candidate_source_sha256']
    inventory = {str(p.relative_to(ROOT)) for p in FROZEN.rglob('*') if p.is_file()}
    if inventory != set(entries):
        raise RuntimeError('frozen inventory changed')
    actual = {}
    for name, expected in sorted(entries.items()):
        path = ROOT / name
        if path.is_symlink() or pin(path)['sha256'] != expected:
            raise RuntimeError('frozen input changed: ' + name)
        imported = APP / 'frozen' / path.relative_to(FROZEN)
        if imported.is_symlink() or imported.read_bytes() != path.read_bytes():
            raise RuntimeError('frozen import differs: ' + name)
        actual[name] = pin(path)
    if aggregate(actual) != PIN:
        raise RuntimeError('frozen aggregate mismatch')
    return actual


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', help='fresh output name inside assigned docs/runs')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', args.run):
        parser.error('simple fresh output name required')
    out = DOC / 'runs' / args.run
    out.parent.mkdir(exist_ok=True)
    if out.parent.resolve() != out.parent:
        raise RuntimeError('output symlink refused')
    out.mkdir()
    for name in ('inputs', 'objects', 'logs', 'tmp'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp')}
    report = {'started': now(), 'argv': list(os.sys.argv), 'cwd': str(ROOT),
              'status': 'BUILDING', 'scope': 'COMPILE_ARCHIVE_RELOCATABLE_ONLY',
              'commands': [], 'inputs': {}, 'artifacts': {}, 'environment': env,
              'application_or_tests_executed': False, 'target_operation': False,
              'private_input_read': False, 'native_adoption': False, 'G3': False, 'G8': False}

    def snapshot(path):
        destination = out / 'inputs' / path.relative_to(ROOT)
        if destination.exists():
            return destination
        if path.is_symlink():
            raise RuntimeError('project input symlink refused: ' + str(path))
        expected = pin(path)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open('xb') as stream:
            stream.write(path.read_bytes())
        if pin(destination) != expected:
            raise RuntimeError('input changed while copying')
        report['inputs'][str(path)] = expected
        return destination

    def command(argv, label):
        argv = list(map(str, argv))
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'cwd': str(ROOT), 'started': now(),
                 'timeout_seconds': 120, 'log': str(log)}
        report['commands'].append(entry)
        with log.open('xb') as stream:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=120)
        entry.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('compile/tool command failed: ' + label)
        return log.read_text()

    try:
        report['frozen_before'] = frozen_check()
        prior = json.loads(PRIOR.read_bytes())
        if prior['status'] != 'BUILD PASS; NOT RUN OR DEPLOYED' or prior['candidate']['candidate_pin'] != PIN:
            raise RuntimeError('prior build receipt mismatch')
        # No old builder is imported/executed. Read only its literal tool pins.
        constants = {}
        for node in ast.parse((ROOT / 'apps/rtsp-auth/build.py').read_text()).body:
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name) and target.id in ('ARM_PINS', 'VENDOR_PIN'):
                        constants[target.id] = ast.literal_eval(node.value)
        toolchain = Path(prior['argv'][prior['argv'].index('--toolchain') + 1])
        report['toolchain_inputs'] = {}
        for name, expected in constants['ARM_PINS'].items():
            path = toolchain / name
            if pin(path)['sha256'] != expected:
                raise RuntimeError('retained ARM toolchain changed: ' + name)
            report['toolchain_inputs'][str(path)] = pin(path)
        # Recorded actual HOST/ARM compiler command closure, not a new harness.
        for platform, entry in prior['compilers'].items():
            cc = Path('/usr/bin/cc') if platform == 'host' else toolchain / 'bin/arm-buildroot-linux-musleabi-gcc'
            if pin(cc.resolve()) != entry['compiler_file']:
                raise RuntimeError('prior compiler changed: ' + platform)
        report['prior_build_receipt'] = pin(PRIOR)
        report['prior_source_header_closure'] = prior['source_header_closure']
        for name, expected in prior['source_header_closure'].items():
            path = Path(name)
            if pin(path) != expected:
                raise RuntimeError('prior compile dependency changed: ' + name)
            if path.is_relative_to(ROOT):
                snapshot(path)
        vendor = ROOT / 'apps/auth-web/service/vendor/mbedtls'
        vendor_entries = {}
        for path in sorted(vendor.rglob('*')):
            if path.is_file():
                vendor_entries[str(path.relative_to(vendor))] = pin(path)
                snapshot(path)
        if len(vendor_entries) != 277 or aggregate(vendor_entries) != constants['VENDOR_PIN']:
            raise RuntimeError('vendor source/license pin mismatch')
        report['vendor_pin'] = constants['VENDOR_PIN']
        for path in sorted(APP.rglob('*')):
            if path.is_file():
                snapshot(path)
        for path in (PRIOR, SEAL, DOC / 'CONTRACT.md', ROOT / 'apps/rtsp-auth/build_app.py',
                     ROOT / 'apps/rtsp-auth/build.py', DOC / 'FROZEN_BEFORE.json'):
            snapshot(path)
        original = (FROZEN / 'rtsp_server_candidate.c').read_text().splitlines(True)
        changed = (APP / 'rtsp_server_owned.c').read_text().splitlines(True)
        (out / 'SERVER_DELTA.patch').write_text(''.join(difflib.unified_diff(
            original, changed, fromfile='frozen/rtsp_server_candidate.c', tofile='rtsp_server_owned.c')))
        for platform in ('host', 'arm'):
            directory = out / 'objects' / platform
            directory.mkdir()
            recorded = [entry['argv'] for entry in prior['commands'] if '-c' in entry['argv']
                        and Path(entry['argv'][-1]).parent.name == platform + '-integration']
            if len(recorded) != 14:
                raise RuntimeError('unexpected real integration translation units')
            template = recorded[0]
            cc = template[0]
            prefix = str(toolchain / 'bin/arm-buildroot-linux-musleabi-')
            ar, nm, readelf = (('/usr/bin/ar', '/usr/bin/nm', '/usr/bin/readelf') if platform == 'host'
                               else (prefix + 'ar', prefix + 'nm', prefix + 'readelf'))
            command([cc, '--version'], platform + '-compiler-version')
            command([cc, '-dumpmachine'], platform + '-compiler-target')
            report.setdefault('tools', {})[platform] = {
                name: pin(Path(path).resolve()) for name, path in
                (('cc', cc), ('ar', ar), ('nm', nm), ('readelf', readelf))}
            for name in ('cc1', 'as', 'ld'):
                text = command([cc, '-print-prog-name=' + name], platform + '-find-' + name).strip()
                path = Path(text) if '/' in text else Path('/usr/bin') / text
                report['tools'][platform][name] = {'path': str(path.resolve()), **pin(path.resolve())}
            objects = []
            for index, old in enumerate(recorded + [template]):
                source = Path(old[old.index('-c') + 1])
                if index == 0:
                    source = APP / 'rtsp_server_owned.c'
                elif index == len(recorded):
                    source = APP / 'listener_custody.c'
                obj = directory / (str(index) + '-' + source.stem + '.o')
                argv = []
                for token in old[:old.index('-MD')]:
                    if token.startswith('-I' + str(ROOT) + '/'):
                        token = '-I' + str(out / 'inputs' / Path(token[2:]).relative_to(ROOT))
                    if index == len(recorded) and token == '-Wno-misleading-indentation':
                        continue
                    argv.append(token)
                argv += ['-I' + str(out / 'inputs' / APP.relative_to(ROOT))]
                argv += ['-MD', '-MF', str(obj.with_suffix('.d')), '-c', str(snapshot(source)), '-o', str(obj)]
                command(argv, platform + '-cc-' + str(index))
                objects.append(obj)
            flags = ['-O2'] if platform == 'host' else ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft', '-static']
            archive = out / ('libowned-loopback-listener-' + platform + '.a')
            command([ar, 'rcsD', archive, *objects], platform + '-archive')
            reloc = out / ('owned-loopback-listener-' + platform + '-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *objects, '-Wl,--gc-sections',
                     '-Wl,--undefined=camera_rtsp_owned_listener_serve_v1',
                     '-Wl,--undefined=candidate_backend_destroy',
                     '-Wl,-Map=' + str(out / (platform + '.map')), '-o', reloc], platform + '-relocatable')
            undefined = command([nm, '-u', reloc], platform + '-undefined')
            defined = command([nm, '-g', '--defined-only', reloc], platform + '-defined')
            command([readelf, '-h', '-A', '-S', reloc], platform + '-ELF')
            relocations = command([readelf, '-r', '-W', reloc], platform + '-relocations')
            names = [line.split()[-1] for line in undefined.splitlines() if line.strip()]
            referenced = [name for name in names if any(name in line.split() for line in relocations.splitlines())]
            report.setdefault('undefined', {})[platform] = {
                'symbol_table': names, 'referenced_by_relocation': referenced,
                'unreferenced_symbol_table_only': sorted(set(names) - set(referenced))}
            if any(name.startswith(('camera_', 'candidate_', 'owned_', 'rv_', 'is_rv_', 'mbedtls_')) for name in referenced):
                raise RuntimeError('unresolved real project call graph')
            if 'camera_rtsp_owned_listener_serve_v1' not in defined or 'camera_rtsp_isolated_candidate_serve' in defined:
                raise RuntimeError('wrong server entry linkage')
            for path in (archive, reloc):
                report['artifacts'][path.name] = {**pin(path), 'executed': False,
                    'role': 'compile-only loopback owned listener; not app/native acceptance'}
        report['compiled_source_header_closure'] = {}
        for dependency in (out / 'objects').rglob('*.d'):
            for name in shlex.split(dependency.read_text().replace('\\\n', '').split(':', 1)[1]):
                path = Path(name)
                report['compiled_source_header_closure'][str(path)] = pin(path)
        report['frozen_after'] = frozen_check()
        report['inputs_unchanged'] = all(pin(Path(name)) == expected for name, expected in report['inputs'].items())
        report['prior_dependencies_unchanged'] = all(pin(Path(name)) == expected for name, expected in prior['source_header_closure'].items())
        if not report['inputs_unchanged'] or not report['prior_dependencies_unchanged']:
            raise RuntimeError('build input changed')
        report['status'] = 'COMPILE/ARCHIVE/RELOCATABLE PASS; NOT RUN OR DEPLOYED'
    except BaseException as exc:
        report['status'] = 'BUILD FAILED; outputs preserved'
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
