#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""R1 adapter/header consumers, HOST/ARM compile/archive/ELF REL only; never run output."""
import argparse
import datetime
import hashlib
import json
import re
import shlex
import shutil
import subprocess
from pathlib import Path

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/rtsp-prestarted-backend-v2-20261022'
V1DOC = ROOT / 'docs/qualification-plans/rtsp-prestarted-backend-20261022'
V1APP = ROOT / 'apps/rtsp-auth/prestarted-backend-candidate'
REVIEW = ROOT / 'docs/qualification-plans/rtsp-prestarted-backend-review-20261022'
VERIFIER = ROOT / 'apps/rtsp-auth/isolated-verifier'
VENDOR = ROOT / 'apps/auth-web/service/vendor/mbedtls'
V1_PIN = '867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90'
V1_SEAL_PIN = '9efc3a84ab6342e9ad789ac1b6448358ea8798cde0339726b78fb47dec3a23c9'
REVIEW_PIN = 'd060aaa15deea1526920414f4015505f1024fa73e2f49eff6a502a90049ce323'
VENDOR_PIN = 'bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5'
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
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run')
    parser.add_argument('--toolchain', required=True, type=Path)
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', args.run):
        parser.error('fresh simple run name required')
    toolchain = args.toolchain.resolve()
    out = DOC / 'runs' / args.run
    out.parent.mkdir(exist_ok=True)
    if out.parent.resolve() != out.parent:
        raise RuntimeError('output parent symlink refused')
    out.mkdir()  # exclusive run, including failure evidence
    for name in ('source', 'objects', 'logs', 'tmp', 'dependencies', 'provenance'):
        (out / name).mkdir()
    report = {'started': now(), 'status': 'PREPARING', 'commands': [],
              'inputs': {}, 'artifacts': {}, 'closure': {}, 'frozen': {},
              'scope': 'R1_ADAPTER_AND_REAL_HEADER_CONSUMERS_HOST_ARM_COMPILE_ARCHIVE_REL_ONLY',
              'programs_tests_generators_listeners_executed': False,
              'native_or_composite_acceptance': False}
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp')}
    report['environment'] = env

    def snapshot(path, destination):
        if path.is_symlink():
            raise RuntimeError('source symlink refused: ' + str(path))
        expected = pin(path)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open('xb') as stream:
            stream.write(path.read_bytes())
        if pin(destination) != expected:
            raise RuntimeError('copy changed: ' + str(path))
        report['inputs'][str(path)] = expected
        return destination

    def command(argv, label, diff=False):
        argv = list(map(str, argv))
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'cwd': str(ROOT), 'started': now(), 'log': str(log)}
        report['commands'].append(entry)
        with log.open('xb') as stream:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=120)
        entry.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode not in ((0, 1) if diff else (0,)):
            raise RuntimeError('command failed: ' + label)
        return log.read_text()

    try:
        v1_seal = V1DOC / 'SOURCE_SEAL01.json'
        review_seal = REVIEW / 'FINAL_SEAL.json'
        if pin(v1_seal)['sha256'] != V1_SEAL_PIN or pin(review_seal)['sha256'] != REVIEW_PIN:
            raise RuntimeError('frozen v1/review seal changed')
        v1 = json.loads(v1_seal.read_text())
        review = json.loads(review_seal.read_text())
        for table in (v1['source_files'], v1['evidence_pins'],
                      review['input_pins'], review['output_pins']):
            for name, expected in table.items():
                path = ROOT / name
                # The frozen review deliberately records these two existing
                # compiler aliases; hash their unchanged target bytes too.
                tool_alias = path in (Path('/usr/bin/cc'),
                                      toolchain / 'bin/arm-buildroot-linux-musleabi-gcc')
                if (path.is_symlink() and not tool_alias) or pin(path) != expected:
                    raise RuntimeError('frozen v1/review input changed: ' + name)
                if path.is_symlink():
                    report.setdefault('frozen_tool_aliases', {})[str(path)] = str(path.resolve())
                report['frozen'][str(path)] = expected
        report['frozen'][str(v1_seal)] = pin(v1_seal)
        report['frozen'][str(review_seal)] = pin(review_seal)
        digest = hashlib.sha256()
        for name, expected in sorted(v1['source_files'].items()):
            path = ROOT / name
            baseline = DOC / 'baseline-v1-app' / path.relative_to(V1APP)
            if pin(baseline) != pin(path):
                raise RuntimeError('baseline changed: ' + str(baseline))
            report['frozen'][str(baseline)] = pin(baseline)
            digest.update(name.encode() + b'\0' + bytes.fromhex(expected['sha256']))
        if len(v1['source_files']) != 11 or digest.hexdigest() != V1_PIN:
            raise RuntimeError('frozen v1 aggregate changed')
        for path in sorted((DOC / 'baseline-provenance').iterdir()):
            origin = (REVIEW if path.name in ('FINAL_SEAL.json', 'REVIEW.md') else V1DOC) / path.name
            if pin(path) != pin(origin):
                raise RuntimeError('provenance copy changed: ' + str(path))
            report['frozen'][str(path)] = pin(path)
            snapshot(path, out / 'provenance' / path.name)
        snapshot(DOC / 'R1_CONTRACT.md', out / 'provenance/R1_CONTRACT.md')
        for relative, expected in ARM_PINS.items():
            path = toolchain / relative
            if pin(path)['sha256'] != expected:
                raise RuntimeError('retained toolchain changed: ' + relative)
            report.setdefault('toolchain_pins', {})[str(path)] = pin(path)
        vendor_files = sorted(p for p in VENDOR.rglob('*') if p.is_file())
        digest = hashlib.sha256()
        for path in vendor_files:
            if path.is_symlink():
                raise RuntimeError('vendor symlink refused')
            digest.update(path.relative_to(VENDOR).as_posix().encode() + b'\0' +
                          bytes.fromhex(pin(path)['sha256']))
            if path.suffix == '.h' or path.name.startswith(('LICENSE', 'COPYING')):
                snapshot(path, out / 'source/vendor' / path.relative_to(VENDOR))
        if len(vendor_files) != 277 or digest.hexdigest() != VENDOR_PIN:
            raise RuntimeError('retained vendor changed')
        report.update(v1_source_pin=V1_PIN, v1_seal_pin=V1_SEAL_PIN,
                      review_seal_pin=REVIEW_PIN, vendor_pin=VENDOR_PIN)
        for path in sorted(APP.iterdir()):
            if path.is_file():
                snapshot(path, out / 'source/app' / path.name)
        for path in sorted((ROOT / 'framework/include/camera').glob('*.h')):
            snapshot(path, out / 'source/framework/include/camera' / path.name)
        snapshot(VERIFIER / 'verifier.h', out / 'source/verifier/verifier.h')
        shutil.copytree(V1DOC / 'baseline/runtime-licenses', out / 'source/runtime-licenses')
        for name in ('verifier_adapter_overlay.c', 'adapter_state.h', 'README.md', 'build.py'):
            command(['/usr/bin/diff', '-u', DOC / 'baseline-v1-app' / name, APP / name],
                    'delta-' + name, diff=True)
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common',
                  '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
                  '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2', '-fstack-usage',
                  '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']
        for name in ('app', 'framework/include', 'verifier', 'vendor/include'):
            common.append('-I' + str(out / 'source' / name))
        prefix = str(toolchain / 'bin/arm-buildroot-linux-musleabi-')
        platforms = [('host', '/usr/bin/cc', '/usr/bin/ar', '/usr/bin/nm', '/usr/bin/readelf', ['-O2']),
                     ('arm', prefix + 'gcc', prefix + 'ar', prefix + 'nm', prefix + 'readelf',
                      ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft'])]
        for label, cc, ar, nm, readelf, flags in platforms:
            command([cc, '--version'], label + '-compiler-version')
            command([cc, '-dumpmachine'], label + '-compiler-target')
            tools = {name: {'path': path, **pin(Path(path).resolve())}
                     for name, path in (('cc', cc), ('ar', ar), ('nm', nm), ('readelf', readelf))}
            for program in ('cc1', 'as', 'ld'):
                program_path = command([cc, '-print-prog-name=' + program], label + '-locate-' + program).strip()
                resolved = Path(program_path) if '/' in program_path else Path(shutil.which(program_path, path=env['PATH']))
                tools[program] = {'path': str(resolved.resolve()), **pin(resolved.resolve())}
            report.setdefault('tools', {})[label] = tools
            objects = []
            for name in ('candidate_auth', 'rtsp_server_candidate', 'verifier_adapter_overlay'):
                source = out / 'source/app' / (name + '.c')
                obj = out / 'objects' / (label + '-' + name + '.o')
                legacy = ['-Wno-misleading-indentation'] if name == 'rtsp_server_candidate' else []
                command([cc, *common, *flags, *legacy, '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', source, '-o', obj], label + '-compile-' + name)
                objects.append(obj)
            archive = out / ('libprestarted-backend-v2-' + label + '.a')
            command([ar, 'rcsD', archive, *objects], label + '-archive')
            reloc = out / ('prestarted-backend-v2-' + label + '-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *objects,
                     '-Wl,-Map=' + str(out / (label + '.map')), '-o', reloc], label + '-relocatable-link')
            command([nm, '-g', '--defined-only', reloc], label + '-defined')
            command([nm, '-u', reloc], label + '-undefined')
            command([readelf, '-h', '-A', '-S', reloc], label + '-ELF')
            for path in (*objects, archive, reloc):
                report['artifacts'][str(path)] = {**pin(path), 'executed': False}
        for path in sorted((out / 'objects').glob('*.d')):
            for name in shlex.split(path.read_text().replace('\\\n', '').split(':', 1)[1]):
                source = Path(name)
                if not source.is_file():
                    raise RuntimeError('missing consumed dependency: ' + name)
                expected = pin(source)
                report['closure'][name] = expected
                destination = out / 'dependencies' / source.relative_to('/')
                if not destination.exists():
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    with destination.open('xb') as stream:
                        stream.write(source.read_bytes())
                if pin(destination) != expected:
                    raise RuntimeError('dependency copy changed: ' + name)
        for table in ('inputs', 'frozen', 'closure'):
            for name, expected in report[table].items():
                if pin(Path(name)) != expected:
                    raise RuntimeError('consumed/frozen input changed: ' + name)
        report['status'] = 'HOST_ARM_CHANGED_UNITS_COMPILE_ARCHIVE_REL_PASS_NOT_RUN'
    except BaseException as exc:
        report['status'] = 'FAILED_PRESERVED_NOT_RUNTIME'
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
