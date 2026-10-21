#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh-directory host fixtures with provenance; never accesses camera hardware."""
import argparse
import datetime
import hashlib
import json
import os
import shutil
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CASES = {
    'backend': ['src/backend.c', 'tests/backend_contract_test.c'],
    'dma_integrity': ['tests/dma_integrity_test.c'],
    'lease_pool': ['src/lease_pool.c', 'tests/lease_pool_test.c'],
    'packet_hub': ['src/packet_hub.c', 'tests/packet_hub_test.c'],
    'rtp_h264': ['src/rtp_h264.c', 'tests/rtp_h264_test.c'],
    'fmp4': ['src/fmp4.c', 'tests/fmp4_test.c'],
    'fmp4_init': ['src/fmp4_init.c', 'tests/fmp4_init_test.c'],
    'fmp4_timeline': ['src/fmp4.c', 'src/fmp4_timeline.c', 'tests/fmp4_timeline_test.c'],
    'fmp4_handoff': ['src/fmp4.c', 'src/fmp4_timeline.c', 'src/packet_hub.c', 'tests/fmp4_handoff_test.c'],
    'fmp4_media_gate': ['src/fmp4.c', 'src/packet_hub.c', 'src/access_policy.c',
                        '../apps/auth-web/service/media_admission.c', 'tests/fmp4_media_gate_test.c'],
    'qc07f_whitebalance': ['tests/qc07f_whitebalance_test.c'],
    'qc07f_whitebalance_manual': ['tests/qc07f_whitebalance_manual_test.c'],
    'producer': ['src/producer.c', 'src/lease_pool.c', 'tests/producer_test.c'],
    'rtsp_producer_lifecycle': ['src/producer.c', 'src/packet_hub.c', 'src/rtp_h264.c',
                               'src/rtsp_request.c', 'src/rtsp_input.c', 'tests/rtsp_producer_lifecycle_test.c'],
    'rtsp_parser_integration': ['src/producer.c', 'src/packet_hub.c', 'src/rtp_h264.c',
                               'src/rtsp_request.c', 'src/rtsp_input.c', 'tests/rtsp_parser_integration_test.c'],
    'rtsp_fix': ['src/producer.c', 'src/packet_hub.c', 'src/rtp_h264.c',
                'src/rtsp_request.c', 'src/rtsp_input.c', 'tests/rtsp_fix_test.c'],
    'rtsp_request': ['src/rtsp_request.c', 'tests/rtsp_request_test.c'],
    'rate_model': ['src/rate_model.c', 'tests/rate_model_test.c'],
    'access_policy': ['src/access_policy.c', 'tests/access_policy_test.c'],
    'settings_store': ['src/settings_store.c', 'tests/settings_store_test.c'],
    'recording_catalog': ['src/recording_catalog.c', 'tests/recording_catalog_test.c'],
    'recording_index_store': ['src/recording_index_store.c', 'tests/recording_index_store_test.c'],
    'network_transition': ['src/network_transition.c', 'tests/network_transition_test.c'],
    'network_transaction_store': ['src/network_transaction_store.c', 'src/network_transition.c',
                                  'src/settings_store.c', 'tests/network_transaction_store_test.c'],
    'security_rng': ['../bringup/qc07f/userland/security-probe/security_rng.c', 'tests/security_rng_test.c'],
    'security_memory': ['../bringup/qc07f/userland/security-probe/security_rng.c',
                        '../bringup/qc07f/userland/security-probe/security_memory.c', 'tests/security_memory_test.c'],
    'rtsp_cursor': ['src/rtp_h264.c', 'src/packet_hub.c', 'src/rtsp_request.c', 'src/rtsp_input.c', 'tests/rtsp_cursor_test.c'],
    'memory_budget': ['src/memory_budget.c', 'tests/memory_budget_test.c'],
    'sc1346_controls': ['tests/sc1346_controls_test.c'],
    'sc1346_transaction': ['tests/sc1346_transaction_test.c'],
}
RECEIPT_SOURCES = ['src/producer.c', 'src/packet_hub.c', 'src/rtp_h264.c',
                   'src/rtsp_request.c', 'src/rtsp_input.c', 'src/rtsp_server.c',
                   'tests/receipt-model/capture-retirement.c', 'tests/rtsp_receipt_adapter_test.c']
RECEIPT_CASES = ['ttl', 'limit128', 'sessions', 'namespace', 'held-observer',
                 'double-expiry', 'generation-wrap',
                 *[f'prequery-{i}' for i in [1, 2]], *[f'extra-{i}' for i in range(5)],
                 *[f'output-{i}' for i in [1, 2, 3]], *[f'clock-{i}' for i in range(5)],
                 *[f'schema-{i}' for i in [1, 2, 3]], *[f'bind-{i}' for i in range(7)],
                 *[f'arm-{i}' for i in range(8)], *[f'next-{i}' for i in range(10)],
                 *[f'close-{i}' for i in range(17)], *[f'retired-{i}' for i in range(3)],
                 *[f'stale-{i}' for i in range(3)], *[f'fdclose-{i}' for i in range(3)]]

def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()

def digest(path):
    data = path.read_bytes()
    return {'path': str(path), 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cc', default='clang')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(mode=0o700)  # Existing output directories are never reused.
    (out / 'tmp').mkdir()
    env = os.environ.copy()
    env.update(TMPDIR=str(out / 'tmp'), TEMP=str(out / 'tmp'), TMP=str(out / 'tmp'),
               PYTHONDONTWRITEBYTECODE='1', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    report = {'start_utc': utc(), 'hardware_test': False, 'cases': [], 'inputs': []}
    inputs = sorted({ROOT / p for case in CASES.values() for p in case} |
                    set((ROOT / 'include/camera').glob('*.h')) | {Path(__file__).resolve(),
                    ROOT.parent / 'bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp-whitebalance.h',
                    ROOT.parent / 'bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp-stream.h',
                    ROOT.parent / 'bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/isp-reference.h',
                    ROOT.parent / 'bringup/qc07f/userland/sc1346-controls.h',
                    ROOT.parent / 'bringup/qc07f/userland/sc1346-transaction.h', ROOT / 'src/rtsp_server.c',
                    ROOT.parent / 'bringup/qc07f/userland/rtspd.c',
                     ROOT.parent / 'bringup/qc07f/userland/crc-bzip2.h',
                    ROOT.parent / 'bringup/qc07f/userland/h264-baseline.h',
                    ROOT.parent / 'bringup/qc07f/hwenc/qc07f_hwenc.h'} |
                     set((ROOT / 'tests').glob('*.h')) |
                     set((ROOT / 'tests').glob('*.py')) |
                    set((ROOT.parent / 'bringup/qc07f/userland/security-probe').glob('*.h')))
    report['inputs'] = [digest(p) for p in inputs]
    report['inputs'] += [digest(ROOT / p) for p in RECEIPT_SOURCES if ROOT / p not in inputs]
    report['receipt_matrix'] = {'cases': [], 'build_result': None, 'hardware_test': False}
    failed = False
    try:
        report['environment'] = {k: env[k] for k in ['TMPDIR', 'TEMP', 'TMP', 'PYTHONDONTWRITEBYTECODE', 'ASAN_OPTIONS', 'UBSAN_OPTIONS']}
        version = subprocess.run([args.cc, '--version'], capture_output=True, check=True, timeout=10, env=env)
        report['compiler_command'] = [args.cc, '--version']
        report['compiler_executable'] = digest(Path(shutil.which(args.cc) or args.cc).resolve())
        report['compiler_version'] = version.stdout.decode('utf-8', errors='replace')
        for name, sources in CASES.items():
            binary = out / name
            command = [args.cc, '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                       '-fsanitize=address,undefined', '-I' + str(ROOT / 'include'),
                       '-I' + str(ROOT.parent / 'bringup/qc07f/userland/security-probe')]
            command += [str(ROOT / p) for p in sources] + ['-o', str(binary)]
            if name in ('rtsp_producer_lifecycle', 'rtsp_parser_integration', 'rtsp_fix'):
                command.append('-ljpeg')  # Included QC adapter; no device calls in fixture.
            if name == 'rtsp_fix':
                command.append('-Wl,--wrap=clock_gettime')
            if name == 'settings_store':
                command += ['-DCAMERA_SETTINGS_WRAP_UID', '-Wl,--wrap=geteuid']
            case = {'name': name, 'build_argv': command, 'start_utc': utc()}
            report['cases'].append(case)
            # Compiler-computed full transitive dependency closure, including
            # source-included server/QC entry and public system/JPEG headers.
            closure = set()
            for index, source in enumerate(sources):
                dep = out / f'{name}-{index}.d'
                argv = [args.cc, '-std=c11', '-M', '-MF', str(dep),
                        '-I' + str(ROOT / 'include'),
                        '-I' + str(ROOT.parent / 'bringup/qc07f/userland/security-probe'), str(ROOT / source)]
                case.setdefault('dependency_argv', []).append(argv)
                subprocess.run(argv, check=True, timeout=30, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                closure.update(Path(p).resolve() for p in dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split())
            case['dependency_inputs'] = [digest(p) for p in sorted(closure)]
            with (out / (name + '-build.log')).open('xb') as log:
                built = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=30, env=env)
            case['build_result'] = built.returncode
            if built.returncode:
                failed = True
                break
            case['binary'] = digest(binary)
            case['test_argv'] = [str(binary)]
            if name == 'settings_store':
                case['test_argv'].append(str(out / 'settings-store-fixtures'))
            if name == 'recording_index_store':
                case['test_argv'].append(str(out / 'recording-index-public-fixtures'))
            if name == 'network_transaction_store':
                case['test_argv'].append(str(out / 'network-transaction-public-fixtures'))
            with (out / (name + '-test.log')).open('xb') as log:
                tested = subprocess.run(case['test_argv'], stdout=log, stderr=subprocess.STDOUT, timeout=30, env=env)
            case['test_result'] = tested.returncode
            case['end_utc'] = utc()
            if tested.returncode:
                failed = True
                break
        if not failed:
            report['python_cases'] = []
            for name in ['rtsp_capture_test.py', 'rtsp_reply_test.py']:
                argv = ['python3', str(ROOT / 'tests' / name)]
                with (out / (name + '.log')).open('xb') as log:
                    r = subprocess.run(argv, stdout=log, stderr=subprocess.STDOUT, timeout=30, env=env)
                report['python_cases'].append({'argv': argv, 'returncode': r.returncode})
                if r.returncode:
                    failed = True
                    break
        if not failed:
            matrix = report['receipt_matrix']
            binary = out / 'rtsp_receipt_adapter'
            flags = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                     '-Wno-misleading-indentation', '-pedantic', '-fsanitize=address,undefined',
                     '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', '-I' + str(ROOT / 'include')]
            argv = [args.cc, *flags, *[str(ROOT / p) for p in RECEIPT_SOURCES],
                    '-Wl,--wrap=clock_gettime', '-ljpeg', '-o', str(binary)]
            matrix['build_argv'] = argv
            closure = set()
            matrix['dependency_argv'] = []
            for index, source in enumerate(RECEIPT_SOURCES):
                dep = out / f'receipt-{index}.d'
                cmd = [args.cc, *[f for f in flags if f != '-no-pie'], '-M', '-MF', str(dep), str(ROOT / source)]
                matrix['dependency_argv'].append(cmd)
                subprocess.run(cmd, check=True, timeout=30, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                closure.update(Path(p).resolve() for p in dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split())
            matrix['dependency_inputs'] = [digest(p) for p in sorted(closure)]
            with (out / 'receipt-build.log').open('xb') as log:
                built = subprocess.run(argv, stdout=log, stderr=subprocess.STDOUT, timeout=30, env=env)
            matrix['build_result'] = built.returncode
            failed = bool(built.returncode)
            if not failed:
                matrix['binary'] = digest(binary)
                for name in RECEIPT_CASES:
                    cmd = [str(binary), name]
                    with (out / f'receipt-{name}.log').open('xb') as log:
                        tested = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, timeout=90, env=env)
                    matrix['cases'].append({'name': name, 'argv': cmd, 'returncode': tested.returncode})
                    if tested.returncode:
                        failed = True
                        break
    except Exception as error:
        failed = True
        report['error'] = str(error)
    finally:
        report['end_utc'] = utc()
        report['all_passed'] = not failed and len(report['cases']) == len(CASES) and all(
            c.get('test_result') == 0 for c in report['cases']) and len(report.get('python_cases', [])) == 2 and all(
            c['returncode'] == 0 for c in report['python_cases']) and report['receipt_matrix']['build_result'] == 0 and len(
            report['receipt_matrix']['cases']) == len(RECEIPT_CASES) and all(c['returncode'] == 0 for c in report['receipt_matrix']['cases'])
        with (out / 'manifest.json').open('x') as manifest:
            json.dump(report, manifest, indent=2)
    print('Host framework fixtures:', 'PASS' if report['all_passed'] else 'FAIL', out)
    return 0 if report['all_passed'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
