#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Actual one-backend/owned-listener composition, HOST/ARM archive/ELF REL; NOT RUN."""
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
DOC = ROOT / 'docs/qualification-plans/rtsp-prestarted-owned-listener-20261022'
BASE_DOC = ROOT / 'docs/qualification-plans/rtsp-prestarted-backend-v2-20261022'
BASE_APP = ROOT / 'apps/rtsp-auth/prestarted-backend-v2'
BASE_REVIEW = ROOT / 'docs/qualification-plans/rtsp-prestarted-backend-v2-review-20261022'
LISTENER_DOC = ROOT / 'docs/qualification-plans/rtsp-owned-listener-20261022'
LISTENER_APP = ROOT / 'apps/rtsp-auth/owned-listener-candidate'
LISTENER_REVIEW = ROOT / 'docs/qualification-plans/rtsp-owned-listener-review-20261022'
VERIFIER = ROOT / 'apps/rtsp-auth/isolated-verifier'
VENDOR = ROOT / 'apps/auth-web/service/vendor/mbedtls'
BASE_PIN = '5a42cdc9d76628031c5380edd2e3b2c4dd06aefc6675138c8e10e1b098a28868'
BASE_SEAL_PIN = '2bfa06a6b239c0c62b1003df6ee0f0ef9dd17ef0088efad00cccaf1ada7e7a8a'
BASE_REVIEW_PIN = 'b3310e3a85cc7295137fa46e3aca4aae8fb12653b7377a55ad9d433a42532292'
LISTENER_PIN = 'febab8640027f71dfc8a1ec4aa53c26cba219048ca4bf74578050377101ce557'
LISTENER_SEAL_PIN = '835aeb64c0967dfe8cdef1d13d3e376a984bf3485bc09b882812b2a9be814ed0'
LISTENER_REVIEW_PIN = '2f39d960ba6cff2b236d215d65dea206f0005fad0fc430a621e24de25bd3db61'
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
              'scope': 'ONE_ACTUAL_PRESTARTED_BACKEND_OWNED_LISTENER_SOURCE_COMPOSITION_COMPILE_REL_ONLY',
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
        seal_specs = ((BASE_DOC / 'SOURCE_SEAL01.json', BASE_SEAL_PIN),
                      (BASE_REVIEW / 'FINAL_SEAL.json', BASE_REVIEW_PIN),
                      (LISTENER_DOC / 'FINAL_SEAL.json', LISTENER_SEAL_PIN),
                      (LISTENER_REVIEW / 'FINAL_SEAL.json', LISTENER_REVIEW_PIN))
        records = []
        for seal, expected in seal_specs:
            if pin(seal)['sha256'] != expected:
                raise RuntimeError('frozen source/review seal changed: ' + str(seal))
            report['frozen'][str(seal)] = pin(seal)
            records.append(json.loads(seal.read_text()))
        base, base_review, listener, listener_review = records
        tables = (base['source_files'], base['evidence_pins'],
                  base_review['input_pins'], base_review['output_pins'],
                  listener['source_files'], listener['files'], listener['compile_input_pins_reverified'],
                  listener_review['input_pins'], listener_review['output_pins'])
        for table in tables:
            for name, expected in table.items():
                path = ROOT / name
                # The frozen review deliberately records these two existing
                # compiler aliases; hash their unchanged target bytes too.
                tool_alias = path in (Path('/usr/bin/cc'),
                                      toolchain / 'bin/arm-buildroot-linux-musleabi-gcc')
                if (path.is_symlink() and not tool_alias) or pin(path) != expected:
                    raise RuntimeError('frozen source/review input changed: ' + name)
                if path.is_symlink():
                    report.setdefault('frozen_tool_aliases', {})[str(path)] = str(path.resolve())
                report['frozen'][str(path)] = expected
        for entries, original, copy, expected_pin, count in (
            (base['source_files'], BASE_APP, DOC / 'baseline-backend-v2', BASE_PIN, 11),
            (listener['source_files'], LISTENER_APP, DOC / 'baseline-listener', LISTENER_PIN, 47),
        ):
            digest = hashlib.sha256()
            for name, expected in sorted(entries.items()):
                path = ROOT / name
                baseline = copy / path.relative_to(original)
                if pin(baseline) != pin(path):
                    raise RuntimeError('baseline copy changed: ' + str(baseline))
                report['frozen'][str(baseline)] = pin(baseline)
                digest.update(name.encode() + b'\0' + bytes.fromhex(expected['sha256']))
            if len(entries) != count or digest.hexdigest() != expected_pin:
                raise RuntimeError('frozen source aggregate changed')
        origins = {'backend': BASE_DOC, 'backend-review': BASE_REVIEW,
                   'listener': LISTENER_DOC, 'listener-review': LISTENER_REVIEW}
        for path in sorted((DOC / 'baseline-provenance').rglob('*')):
            if not path.is_file():
                continue
            relative = path.relative_to(DOC / 'baseline-provenance')
            if relative.parts[0] == 'history':
                history = ROOT / ('docs/qualification-plans/rtsp-prestarted-backend-20261022' if
                                 path.name == 'SEAL_FAILURE01.json' else
                                 'docs/qualification-plans/rtsp-prestarted-backend-review-20261022')
                origin = history / path.name
            else:
                origin = origins[relative.parts[0]] / path.name
            if pin(path) != pin(origin):
                raise RuntimeError('provenance copy changed: ' + str(path))
            report['frozen'][str(origin)] = pin(origin)
            report['frozen'][str(path)] = pin(path)
            snapshot(path, out / 'provenance' / relative)
        snapshot(DOC / 'COMPOSITION_CONTRACT.md', out / 'provenance/COMPOSITION_CONTRACT.md')
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
            snapshot(path, out / 'source/vendor' / path.relative_to(VENDOR))
        if len(vendor_files) != 277 or digest.hexdigest() != VENDOR_PIN:
            raise RuntimeError('retained vendor changed')
        report.update(base_source_pin=BASE_PIN, listener_source_pin=LISTENER_PIN,
                      base_review_seal_pin=BASE_REVIEW_PIN,
                      listener_review_seal_pin=LISTENER_REVIEW_PIN, vendor_pin=VENDOR_PIN)
        for path in sorted(APP.iterdir()):
            if path.is_file():
                snapshot(path, out / 'source/app' / path.name)
        for path in sorted((ROOT / 'framework/include/camera').glob('*.h')):
            snapshot(path, out / 'source/framework/include/camera' / path.name)
        snapshot(VERIFIER / 'verifier.h', out / 'source/verifier/verifier.h')
        snapshot(VERIFIER / 'ipc.c', out / 'source/verifier/ipc.c')
        framework_names = ('rtsp_digest_wire', 'rtsp_request', 'rtsp_input', 'backend',
                           'packet_hub', 'producer', 'rtp_h264')
        for name in framework_names:
            snapshot(ROOT / 'framework/src' / (name + '.c'), out / 'source/framework/src' / (name + '.c'))
        shutil.copytree(LISTENER_APP / 'frozen/runtime-licenses', out / 'source/runtime-licenses')
        for name in ('candidate_auth.h', 'rtsp_server_candidate.c', 'README.md', 'build.py'):
            command(['/usr/bin/diff', '-u', DOC / 'baseline-backend-v2' / name, APP / name],
                    'delta-' + name, diff=True)
        command(['/usr/bin/diff', '-u', DOC / 'baseline-listener/listener_custody.h', APP / 'listener_custody.h'],
                'delta-listener_custody.h', diff=True)
        command(['/usr/bin/diff', '-u', '/dev/null', APP / 'prestarted_owned_listener_v1.h'],
                'delta-new-source-api', diff=True)
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common',
                  '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
                  '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2', '-fstack-usage',
                  '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']
        for name in ('app', 'framework/include', 'verifier', 'vendor/include', 'vendor/library'):
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
            sources = [out / 'source/app' / (name + '.c') for name in
                       ('candidate_auth', 'rtsp_server_candidate', 'verifier_adapter_overlay', 'listener_custody')]
            sources += [out / 'source/verifier/ipc.c']
            sources += [out / 'source/framework/src' / (name + '.c') for name in framework_names]
            sources += [out / 'source/vendor/library' / (name + '.c') for name in
                        ('sha256', 'constant_time', 'platform_util')]
            for index, source in enumerate(sources):
                name = source.stem
                obj = out / 'objects' / (label + '-' + str(index) + '-' + name + '.o')
                legacy = ['-Wno-misleading-indentation'] if name in ('rtsp_server_candidate', 'rtsp_input') else []
                if name == 'rtsp_input':
                    legacy.append('-Wno-error=nonnull')
                command([cc, *common, *flags, *legacy, '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', source, '-o', obj], label + '-compile-' + str(index) + '-' + name)
                objects.append(obj)
            archive = out / ('libprestarted-owned-listener-' + label + '.a')
            command([ar, 'rcsD', archive, *objects], label + '-archive')
            reloc = out / ('prestarted-owned-listener-' + label + '-NOT-EXECUTABLE.o')
            roots = ('camera_rtsp_prestarted_owned_listener_serve_v1', 'candidate_prestarted_start',
                     'candidate_prestarted_finish', 'candidate_backend_revoke', 'candidate_backend_cancel',
                     'is_rv_ipc_close_outcome', 'is_rv_context_bytes')
            command([cc, *flags, '-nostdlib', '-r', *objects, '-Wl,--gc-sections',
                     *('-Wl,--undefined=' + name for name in roots),
                     '-Wl,-Map=' + str(out / (label + '.map')), '-o', reloc], label + '-relocatable-link')
            defined = command([nm, '-g', '--defined-only', reloc], label + '-defined')
            undefined = command([nm, '-u', reloc], label + '-undefined')
            command([readelf, '-h', '-A', '-S', reloc], label + '-ELF')
            relocations = command([readelf, '-r', '-W', reloc], label + '-relocations')
            names = [line.split()[-1] for line in undefined.splitlines() if line.strip()]
            referenced = [name for name in names if any(name in line.split() for line in relocations.splitlines())]
            report.setdefault('undefined', {})[label] = {
                'symbol_table': names, 'referenced_by_relocation': referenced,
                'unreferenced_symbol_table_only': sorted(set(names) - set(referenced))}
            if any(name.startswith(('camera_', 'candidate_', 'owned_', 'rv_', 'is_rv_', 'mbedtls_')) for name in referenced):
                raise RuntimeError('unresolved real project call graph')
            if any(name not in defined.split() for name in roots):
                raise RuntimeError('missing actual composite API definition')
            if any(name in defined.split() for name in ('main', 'camera_rtsp_isolated_candidate_serve',
                    'camera_rtsp_prestarted_candidate_serve', 'camera_rtsp_owned_listener_serve_v1',
                    'candidate_backend_init', 'candidate_backend_destroy', 'candidate_backend_finish')):
                raise RuntimeError('duplicate/legacy API present')
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
