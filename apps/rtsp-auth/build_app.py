#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile/link the RTSP candidate without running applications or test suites.

Builds a HOST bench executable and integration archives/strict verifier children.
Optional ARM outputs use the retained pinned toolchain. Nothing is installed,
provisioned, launched, staged to a camera or declared operational by this build.
"""
import argparse
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / 'apps/rtsp-auth/isolated-server-host-tail'
VERIFIER = ROOT / 'apps/rtsp-auth/isolated-verifier'
VENDOR = ROOT / 'apps/auth-web/service/vendor/mbedtls'
SEAL = ROOT / 'docs/qualification-plans/rtsp-tail-20261022/harness-correction/CANDIDATE_SOURCE_SEAL01.json'
PIN = '2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f'
FRAME = [ROOT / 'framework/src' / name for name in (
    'rtsp_digest_wire.c', 'rtsp_request.c', 'rtsp_input.c', 'backend.c',
    'packet_hub.c', 'producer.c', 'rtp_h264.c')]
SERVER = [APP / name for name in (
    'rtsp_server_candidate.c', 'candidate_auth.c', 'verifier_adapter_overlay.c')]
SERVER += [VERIFIER / 'ipc.c', *FRAME]
SERVER += [VENDOR / 'library' / name for name in (
    'sha256.c', 'constant_time.c', 'platform_util.c')]
CHILD = [VERIFIER / name for name in ('ipc.c', 'engine.c', 'isolation.c', 'child.c')]
CHILD += [ROOT / 'framework/src' / name for name in (
    'rtsp_digest.c', 'rtsp_digest_wire.c', 'rtsp_request.c', 'rtsp_input.c', 'access_policy.c')]
CHILD += [VENDOR / 'library' / name for name in (
    'sha256.c', 'md5.c', 'constant_time.c', 'platform_util.c')]
FIXTURE = [APP / name for name in (
    'fixture.c', 'fault_clock.c', 'wait_receipt.c', 'pread_receipt.c', 'prefix_receipt.c')]


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def pin(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def check_candidate():
    seal = json.loads(SEAL.read_bytes())
    entries = seal['candidate_source_sha256']
    digest = hashlib.sha256()
    for name, expected in sorted(entries.items()):
        source = ROOT / name
        if source.is_symlink() or pin(source)['sha256'] != expected:
            raise RuntimeError('frozen candidate changed: ' + name)
        digest.update(name.encode() + b'\0' + bytes.fromhex(expected))
    actual_paths = {str(p.relative_to(ROOT)) for p in APP.rglob('*') if p.is_file()}
    if actual_paths != set(entries) or digest.hexdigest() != PIN:
        raise RuntimeError('frozen candidate inventory mismatch')
    return {'seal': pin(SEAL), 'candidate_pin': PIN, 'source_files': len(entries)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', help='fresh output name under local/build/rtsp-app')
    parser.add_argument('--toolchain', type=Path, help='existing pinned ARMv5 musl toolchain')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]{0,63}', args.run):
        parser.error('use a simple fresh run name')
    candidate = check_candidate()
    spec = importlib.util.spec_from_file_location('rtsp_build_constants', ROOT / 'apps/rtsp-auth/build.py')
    base = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(base)  # definitions only; main/test pipeline is not called
    vendor = base.vendor_pin()
    out = ROOT / 'local/build/rtsp-app' / args.run
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.parent.resolve() != out.parent:
        raise RuntimeError('output-parent symlink refused')
    out.mkdir()
    for name in ('objects', 'logs', 'inputs', 'tmp'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'SOURCE_DATE_EPOCH': '1791763200',
           'TMPDIR': str(out / 'tmp')}
    report = {'started': now(), 'argv': list(os.sys.argv), 'cwd': str(ROOT),
              'status': 'BUILDING', 'scope': 'COMPILE_LINK_ONLY', 'candidate': candidate,
              'vendor': vendor, 'environment': env, 'commands': [], 'artifacts': {},
              'application_executed': False, 'tests_executed': False,
              'private_input_or_provisioning': False, 'target_operation': False,
              'deployment_ready': False, 'product_complete': False,
              'P1_independent_review_completed': False, 'G3': False, 'G8': False}

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
            raise RuntimeError('build command failed: ' + label + '; see ' + str(log))
        return log.read_text()

    common = ['-std=c99', '-Wall', '-Wextra', '-Werror', '-fno-common',
              '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
              '-fstack-usage', '-I' + str(APP), '-I' + str(VERIFIER),
              '-I' + str(ROOT / 'framework/include'),
              '-I' + str(VENDOR / 'include'), '-I' + str(VENDOR / 'library')]

    def objects(cc, label, flags, sources, defines=(), server=True):
        directory = out / 'objects' / label
        directory.mkdir()
        result = []
        config = 'server_crypto_config.h' if server else 'rtsp_crypto_config.h'
        for index, source in enumerate(sources):
            obj = directory / (str(index) + '-' + source.stem + '.o')
            # Retain the existing builder's narrowly documented legacy policies.
            legacy = ['-Wno-misleading-indentation'] if source.name in (
                'rtsp_server_candidate.c', 'rtsp_input.c') else []
            if source.name == 'rtsp_input.c':
                legacy += ['-Wno-error=nonnull']
            command([cc, *common, *flags, *defines, *legacy,
                     '-DMBEDTLS_CONFIG_FILE="' + config + '"', '-MD', '-MF',
                     obj.with_suffix('.d'), '-c', source, '-o', obj], label + '-cc-' + str(index))
            result.append(obj)
        return result

    def artifact(path, role):
        report['artifacts'][path.name] = {**pin(path), 'path': str(path),
                                           'role': role, 'executed': False}

    try:
        sources = sorted(set(SERVER + CHILD + FIXTURE + [APP / 'candidate_auth.h',
            VERIFIER / 'verifier.h', Path(__file__), ROOT / 'apps/rtsp-auth/build.py']))
        before = {str(p.relative_to(ROOT)): pin(p) for p in sources}
        report['source_pins'] = before
        for source in sources:
            destination = out / 'inputs' / source.relative_to(ROOT)
            destination.parent.mkdir(parents=True, exist_ok=True)
            with destination.open('xb') as stream:
                stream.write(source.read_bytes())
        platforms = [('host', Path('/usr/bin/cc'), Path('/usr/bin/ar'),
                      Path('/usr/bin/readelf'), ['-O2'])]
        if args.toolchain:
            toolchain = args.toolchain.expanduser().resolve()
            for relative, expected in base.ARM_PINS.items():
                if pin(toolchain / relative)['sha256'] != expected:
                    raise RuntimeError('ARM toolchain pin mismatch: ' + relative)
            report['ARM_toolchain_pins'] = base.ARM_PINS
            prefix = toolchain / 'bin/arm-buildroot-linux-musleabi-'
            platforms.append(('arm', Path(str(prefix) + 'gcc'), Path(str(prefix) + 'ar'),
                              Path(str(prefix) + 'readelf'),
                              ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft', '-static']))
        for name, cc, ar, readelf, flags in platforms:
            report.setdefault('compilers', {})[name] = {
                'version': command([cc, '--version'], name + '-compiler-version'),
                'target': command([cc, '-dumpmachine'], name + '-compiler-target').strip(),
                'compiler_file': pin(cc.resolve())}
            compiled = objects(cc, name + '-integration', flags, SERVER)
            library = out / ('libcamera-rtsp-integration-' + name + '.a')
            command([ar, 'rcsD', library, *compiled], name + '-archive')
            artifact(library, 'strict integration archive; real camera entry/provider wiring missing')
            # Root the real server API while retaining the existing section-GC
            # boundary: unused direct-Digest wire helpers do not belong in this
            # isolated server and must not drag credential code into its process.
            entry = out / (name + '-link-only.c')
            entry.write_text('/* Link check only: not an application entry point. */\nint main(void) { return 0; }\n')
            linkcheck = out / (name + '-integration-linkcheck-NOT-RUN')
            command([cc, *flags, entry, '-Wl,--whole-archive', library,
                     '-Wl,--no-whole-archive', '-Wl,--gc-sections',
                     '-Wl,--undefined=camera_rtsp_isolated_candidate_serve',
                     '-Wl,--undefined=candidate_backend_destroy', '-Wl,-t',
                     '-Wl,-Map=' + str(out / (name + '-integration.map')),
                     '-o', linkcheck], name + '-integration-link')
            artifact(linkcheck, 'link check only; never run or deploy')
            child_objects = objects(cc, name + '-verifier', flags, CHILD, server=False)
            child = out / ('camera-rtsp-verifier.' + name)
            command([cc, *flags, *child_objects, '-Wl,--gc-sections', '-Wl,-t',
                     '-Wl,-Map=' + str(out / (name + '-verifier.map')), '-o', child], name + '-verifier-link')
            artifact(child, 'strict verifier companion; no new runtime image pin installed')
            command([readelf, '-h', '-A', '-l', child], name + '-verifier-ELF')
            command([readelf, '-h', '-A', '-l', linkcheck], name + '-integration-ELF')
            if name == 'host':
                bench_objects = objects(cc, 'host-bench', flags, SERVER + FIXTURE,
                                        ['-DIS_HOST_PUBLIC_ONLY', '-DIS_SYNTHETIC_SOURCE_ONLY'])
                bench = out / 'camera-rtsp-bench.host'
                command([cc, *flags, *bench_objects, '-Wl,--wrap=clock_gettime',
                         '-Wl,--wrap=waitpid', '-Wl,--wrap=pread', '-Wl,--wrap=fork',
                         '-Wl,--gc-sections', '-o', bench], 'host-bench-link')
                artifact(bench, 'synthetic localhost bench app, not a camera runtime')
        closure = {}
        for dependency in (out / 'objects').rglob('*.d'):
            for name in shlex.split(dependency.read_text().replace('\\\n', '').split(':', 1)[1]):
                path = Path(name)
                if path.is_file():
                    closure[str(path.resolve())] = pin(path)
        report['source_header_closure'] = closure
        report['source_unchanged_after_build'] = before == {
            str(p.relative_to(ROOT)): pin(p) for p in sources}
        if not report['source_unchanged_after_build'] or check_candidate() != candidate or base.vendor_pin() != vendor:
            raise RuntimeError('build input changed')
        report['status'] = 'BUILD PASS; NOT RUN OR DEPLOYED'
        report['remaining'] = [
            'No real camera main/provider/provisioning integration or deployable camera package',
            'Native verifier image pin remains unchanged; new binary not automatically adopted',
            'Functional operation checklist pending; extended security/robustness tests deferred',
            'P1 independent review blocked; no station build or attachment approval']
    except BaseException as exc:
        report['status'] = 'BUILD FAILED; prior outputs preserved'
        report['error'] = type(exc).__name__ + ': ' + str(exc)
        raise
    finally:
        report['ended'] = now()
        with (out / 'REPORT.json').open('x') as stream:
            json.dump(report, stream, indent=2)
            stream.write('\n')
        print('Build report: ' + str(out / 'REPORT.json'))


if __name__ == '__main__':
    main()
