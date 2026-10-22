#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile/archive/relocatable-link only. No main, tests or output execution."""
import argparse
import ast
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
OUT = ROOT / 'docs/qualification-plans/rtsp-runtime-20261022/runs'
NATIVE = ROOT / 'evidence/delegated/20261012-native-handoff/run34'
SERVER = ROOT / 'apps/rtsp-auth/isolated-server-host-tail'
VERIFIER = ROOT / 'apps/rtsp-auth/isolated-verifier'
VENDOR = ROOT / 'apps/auth-web/service/vendor/mbedtls'
SERVER_SEAL = ROOT / 'docs/qualification-plans/rtsp-tail-20261022/harness-correction/CANDIDATE_SOURCE_SEAL01.json'
SERVER_PIN = '2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f'
NATIVE_MANIFEST_PIN = '8286c85c09d7f34d5bb6af13054803f68ebafad4e2e874da4b1a35aaf9e0d529'
V6_SNAPSHOT = ROOT / 'evidence/delegated/20261012-native-handoff/v6-final-source-snapshot01/manifest.json'
V6_PIN = 'bdc87e1700b8f0a8e6c191ed0bd8869035487258636b55f4e6c8bce3640267d5'


def pin(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def constants(path):
    return {n.targets[0].id: ast.literal_eval(n.value)
            for n in ast.parse(path.read_text()).body
            if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name)
            and n.targets[0].id in ('ARM_PINS', 'VENDOR_PIN')}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run')
    parser.add_argument('--toolchain', required=True, type=Path)
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', args.run):
        parser.error('simple fresh run name required')
    if pin(NATIVE / 'manifest.json')['sha256'] != NATIVE_MANIFEST_PIN:
        raise RuntimeError('run34 manifest changed')
    if pin(V6_SNAPSHOT)['sha256'] != V6_PIN:
        raise RuntimeError('v6 source snapshot manifest changed')
    native_entries = {e['path'][7:]: e['sha256']
                      for e in json.loads((NATIVE / 'manifest.json').read_text())['files']
                      if e['path'].startswith('inputs/')}
    for e in json.loads(V6_SNAPSHOT.read_text())['files']:
        path = ROOT / e['path']
        if path.is_symlink() or pin(path)['sha256'] != e['sha256']:
            raise RuntimeError('v6 source snapshot changed: ' + e['path'])
        if '/source/' in e['path']:
            name = path.name
            if native_entries.get(name) != e['sha256']:
                raise RuntimeError('v6 snapshot/run34 mismatch: ' + name)
    server_entries = json.loads(SERVER_SEAL.read_text())['candidate_source_sha256']
    digest = hashlib.sha256()
    for name, expected in sorted(server_entries.items()):
        path = ROOT / name
        if path.is_symlink() or pin(path)['sha256'] != expected:
            raise RuntimeError('frozen server changed: ' + name)
        digest.update(name.encode() + b'\0' + bytes.fromhex(expected))
    if digest.hexdigest() != SERVER_PIN:
        raise RuntimeError('server aggregate pin changed')
    base = constants(ROOT / 'apps/rtsp-auth/build.py')
    toolchain = args.toolchain.resolve()
    for name, expected in base['ARM_PINS'].items():
        if pin(toolchain / name)['sha256'] != expected:
            raise RuntimeError('toolchain changed: ' + name)
    vendor_files = sorted(p for p in VENDOR.rglob('*') if p.is_file())
    digest = hashlib.sha256()
    for p in vendor_files:
        if p.is_symlink():
            raise RuntimeError('vendor symlink refused')
        digest.update(p.relative_to(VENDOR).as_posix().encode() + b'\0' +
                      bytes.fromhex(pin(p)['sha256']))
    if len(vendor_files) != 277 or digest.hexdigest() != base['VENDOR_PIN']:
        raise RuntimeError('retained Mbed TLS pin changed')

    OUT.mkdir(parents=True, exist_ok=True)
    if OUT.resolve() != OUT:
        raise RuntimeError('output parent symlink refused')
    out = OUT / args.run
    out.mkdir()
    for name in ('inputs', 'objects', 'logs', 'tmp'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp')}
    report = {'started': now(), 'scope': 'SOURCE_COMPILE_RELOCATABLE_LINK_ONLY',
              'status': 'BUILDING', 'commands': [], 'source_inputs': {}, 'artifacts': {},
              'server_pin': SERVER_PIN, 'run34_manifest_pin': NATIVE_MANIFEST_PIN,
              'v6_source_manifest_pin': V6_PIN, 'toolchain_pins': base['ARM_PINS'],
              'vendor_pin': base['VENDOR_PIN'], 'environment': env,
              'application_or_test_executed': False, 'target_operation': False,
              'private_input_read': False, 'native_adoption': False, 'G3': False, 'G8': False}

    def snapshot(path, destination):
        if path.is_symlink():
            raise RuntimeError('source symlink refused: ' + str(path))
        before = pin(path)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open('xb') as stream:
            stream.write(path.read_bytes())
        if pin(destination) != before:
            raise RuntimeError('source changed during copy')
        report['source_inputs'][str(path.relative_to(ROOT))] = before
        return destination

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
            raise RuntimeError('compile/link command failed: ' + label)
        return log.read_text()

    try:
        for name, expected in sorted(native_entries.items()):
            path = NATIVE / 'inputs' / name
            if path.is_symlink() or pin(path)['sha256'] != expected:
                raise RuntimeError('run34 public input changed: ' + name)
            snapshot(path, out / 'inputs/native' / name)
        for name in sorted(server_entries):
            path = ROOT / name
            snapshot(path, out / 'inputs/server' / path.relative_to(SERVER))
        for path in sorted(APP.glob('*')):
            if path.is_file():
                snapshot(path, out / 'inputs/new' / path.name)
        snapshot(ROOT / 'apps/rtsp-auth/build.py', out / 'inputs/provenance/rtsp-build.py')
        snapshot(NATIVE / 'manifest.json', out / 'inputs/provenance/run34-manifest.json')
        snapshot(V6_SNAPSHOT, out / 'inputs/provenance/v6-source-manifest.json')
        snapshot(SERVER_SEAL, out / 'inputs/provenance/server-seal.json')
        framework_names = ('rtsp_digest_wire.c', 'rtsp_request.c', 'rtsp_input.c',
                           'backend.c', 'packet_hub.c', 'producer.c', 'rtp_h264.c')
        framework = [snapshot(ROOT / 'framework/src' / name,
                             out / 'inputs/framework/src' / name) for name in framework_names]
        for path in sorted((ROOT / 'framework/include/camera').glob('*.h')):
            snapshot(path, out / 'inputs/framework/include/camera' / path.name)
        for name in ('verifier.h', 'ipc.c'):
            snapshot(VERIFIER / name, out / 'inputs/verifier' / name)
        # Retain vendor source, headers and license with its original layout.
        for path in vendor_files:
            snapshot(path, out / 'inputs/vendor' / path.relative_to(VENDOR))
        sources = [out / 'inputs/new/rtsp_native_source.c']
        sources += [out / 'inputs/native' / name for name in
                    ('sensor_baseline.c', 'memory_reserve.c', 'memory_runtime.c', 'memory_reader.c')]
        sources += [out / 'inputs/server' / name for name in
                    ('rtsp_server_candidate.c', 'candidate_auth.c', 'verifier_adapter_overlay.c')]
        sources += [out / 'inputs/verifier/ipc.c', *framework]
        sources += [out / 'inputs/vendor/library' / name for name in
                    ('sha256.c', 'constant_time.c', 'platform_util.c')]
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common',
                  '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
                  '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2', '-fstack-usage',
                  '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']
        for name in ('new', 'native', 'server', 'verifier', 'framework/include',
                     'vendor/include', 'vendor/library'):
            common.append('-I' + str(out / 'inputs' / name))
        prefix = str(toolchain / 'bin/arm-buildroot-linux-musleabi-')
        platforms = [('host', '/usr/bin/cc', '/usr/bin/ar', '/usr/bin/nm', '/usr/bin/readelf', ['-O2']),
                     ('arm', prefix + 'gcc', prefix + 'ar', prefix + 'nm', prefix + 'readelf',
                      ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft'])]
        for label, cc, ar, nm, readelf, flags in platforms:
            command([cc, '--version'], label + '-compiler-version')
            command([cc, '-dumpmachine'], label + '-compiler-target')
            directory = out / 'objects' / label
            directory.mkdir()
            objects = []
            for index, path in enumerate(sources):
                obj = directory / (str(index) + '-' + path.stem + '.o')
                legacy = []
                if path.name in ('rtsp_native_source.c', 'memory_reserve.c', 'memory_runtime.c',
                                 'memory_reader.c', 'rtsp_server_candidate.c', 'rtsp_input.c'):
                    legacy.append('-Wno-misleading-indentation')
                if path.name == 'rtsp_input.c':
                    legacy.append('-Wno-error=nonnull')
                command([cc, *common, *flags, *legacy, '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', path, '-o', obj], label + '-cc-' + str(index))
                objects.append(obj)
            archive = out / ('libqc07f-native-rtsp-wiring-' + label + '.a')
            command([ar, 'rcsD', archive, *objects], label + '-archive')
            reloc = out / ('qc07f-native-rtsp-wiring-' + label + '-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *objects, '-Wl,--gc-sections',
                     '-Wl,--undefined=qc07f_rtsp_native_wire',
                     '-Wl,--undefined=camera_rtsp_isolated_candidate_serve',
                     '-Wl,--undefined=candidate_backend_destroy',
                     '-Wl,-Map=' + str(out / (label + '.map')), '-o', reloc], label + '-relocatable-link')
            undefined_text = command([nm, '-u', reloc], label + '-undefined-symbols')
            command([nm, '-g', '--defined-only', reloc], label + '-defined-symbols')
            command([readelf, '-h', '-A', '-S', reloc], label + '-ELF')
            relocations = command([readelf, '-r', '-W', reloc], label + '-relocations')
            undefined = [line.split()[-1] for line in undefined_text.splitlines() if line.strip()]
            # -r may retain undefined symbol-table entries from GC-discarded
            # direct-Digest sections. Distinguish real relocations from those
            # leftovers; do not link a credential core just to satisfy a name.
            referenced = [name for name in undefined if any(
                name in line.split() for line in relocations.splitlines())]
            report.setdefault('undefined', {})[label] = {
                'symbol_table': undefined, 'referenced_by_relocation': referenced,
                'unreferenced_symbol_table_only': sorted(set(undefined) - set(referenced))}
            for path in (archive, reloc):
                report['artifacts'][path.name] = {**pin(path), 'executed': False,
                    'role': 'compile-only native-source integration; no main/provider adoption'}
            report.setdefault('tools', {})[label] = {
                name: pin(Path(path).resolve()) for name, path in
                (('cc', cc), ('ar', ar), ('nm', nm), ('readelf', readelf))}
        closure = {}
        for path in (out / 'objects').rglob('*.d'):
            for name in shlex.split(path.read_text().replace('\\\n', '').split(':', 1)[1]):
                dependency = Path(name)
                if dependency.is_file():
                    closure[str(dependency)] = pin(dependency)
        report['compiled_source_header_closure'] = closure
        report['sources_unchanged'] = all(pin(ROOT / name) == expected
                                          for name, expected in report['source_inputs'].items())
        if not report['sources_unchanged']:
            raise RuntimeError('source input changed')
        report['status'] = 'COMPILE/ARCHIVE/RELOCATABLE LINK PASS; NOT AN APPLICATION'
    except BaseException as exc:
        report['status'] = 'BUILD FAILED; output retained'
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
