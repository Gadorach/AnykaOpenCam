#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fixed offline compile/rooted REL only. Never import/call any candidate."""
from pathlib import Path
import datetime
import difflib
import hashlib
import json
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/qc07f-span-serve-20261024'
OLD = ROOT / 'docs/qualification-plans/rtsp-owned-runtime-20261023/runs/build02'
SPAN = APP.parent / 'qc07f-span-runtime-candidate'
SENDER = APP.parent / 'rtsp-span-server-v3'
ORIGINAL = APP.parent / 'qc07f-owned-runtime'
MEMORY = APP.parent / 'qc07f-span-memory'
PRIOR = ROOT / 'docs/qualification-plans/qc07f-span-runtime-20261024/runs/build02/REPORT.json'
PAIRS = ((SPAN, 'owned_runtime.c'), (SPAN, 'owned_runtime_v1.h'),
         (SENDER, 'rtsp_span_server.c'), (SENDER, 'rtsp_span_server_v1.h'))


def pin(p):
    data = Path(p).read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    out = DOC / 'runs/build01'
    out.mkdir(parents=True, exist_ok=False)
    for n in ('logs', 'objects', 'members-host', 'members-arm', 'tmp'):
        (out / n).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(out / 'tmp'),
           'SOURCE_DATE_EPOCH': '1791763200', 'PYTHONDONTWRITEBYTECODE': '1'}
    old = json.loads((OLD / 'REPORT.json').read_text())
    prior = json.loads(PRIOR.read_text())
    retained = {p['resolved']: p for p in prior['tools'].values()}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'dependencies': {}, 'reused_dependencies': {}, 'object_roles': {},
              'tools': {}, 'environment': env, 'runtime_calls': 0, 'executable': False,
              'scope': 'ACTUAL_COMPOSED_SOURCE_COMPILE_ROOTED_REL_ONLY'}

    def keep(p, expected=None):
        actual = pin(p)
        if expected and actual != {k: expected[k] for k in ('bytes', 'sha256')}:
            raise RuntimeError('STOP: retained input drift: ' + str(p))
        report['inputs'][str(p)] = actual

    def tool(p):
        resolved = str(Path(p).resolve())
        expected = retained.get(resolved)
        if not expected or pin(resolved) != {k: expected[k] for k in ('bytes', 'sha256')}:
            raise RuntimeError('STOP: tool mismatch: ' + str(p))
        report['tools'][str(p)] = {'resolved': resolved, **pin(resolved)}

    def command(argv, label, cwd=ROOT):
        argv = list(map(str, argv))
        tool(argv[0])
        log = out / 'logs' / (label + '.txt')
        item = {'argv': argv, 'cwd': str(cwd), 'started': now(), 'log': str(log)}
        report['commands'].append(item)
        with log.open('xb') as f:
            result = subprocess.run(argv, cwd=cwd, env=env, stdout=f,
                                    stderr=subprocess.STDOUT, timeout=120)
        item.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('STOP: offline tool failed: ' + label)
        return log.read_text()

    try:
        tool('/usr/bin/python3')
        if old['status'] != 'HOST_ARM_COMPILE_ARCHIVE_REL_PASS_NOT_RUN' or \
           prior['status'] != 'HOST_ARM_14_COMPILES_2_ROOTED_REL_PASS_NOT_RUN':
            raise RuntimeError('STOP: retained baseline status')
        for item in json.loads((DOC / 'FROZEN_INPUTS.json').read_text()):
            keep(ROOT / item['path'], item)
        for p in (OLD / 'REPORT.json', PRIOR, DOC / 'CONTRACT.md',
                  DOC / 'AUTHOR_AUDIT.md', DOC / 'FROZEN_INPUTS.json'):
            keep(p)
        for name, expected in old['inputs'].items():
            if name.startswith(str(ORIGINAL) + '/') or name.startswith(str(ROOT / 'framework') + '/'):
                keep(Path(name), expected)
        for p in sorted(APP.iterdir()):
            if p.is_file():
                keep(p)
        for direction, inverse in (('BASELINES_TO_CANDIDATE.diff', False),
                                   ('CANDIDATE_TO_BASELINES.diff', True)):
            with (out / direction).open('x') as f:
                for baseline, name in PAIRS:
                    a, b = baseline / name, APP / name
                    if inverse:
                        a, b = b, a
                    f.writelines(difflib.unified_diff(a.read_text().splitlines(True),
                        b.read_text().splitlines(True), str(a), str(b)))
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common',
                  '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
                  '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2', '-fstack-usage',
                  '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"', '-Wno-misleading-indentation']
        for p in (APP, ORIGINAL / 'server', ORIGINAL / 'native',
                  ROOT / 'framework/include', APP.parent / 'isolated-verifier'):
            common.append('-I' + str(p))
        sources = [APP / 'owned_runtime.c', APP / 'rtsp_span_server.c',
                   MEMORY / 'memory_reserve.c', MEMORY / 'memory_runtime.c', MEMORY / 'memory_reader.c',
                   APP.parent / 'span-hub-v2/span_hub.c',
                   APP.parent / 'rtp-span-candidate/rtp_h264_spans.c', ORIGINAL / 'native/sensor_baseline.c']
        roots = ('qc07f_owned_runtime_prepare_v1', 'qc07f_owned_runtime_bind_postfork_v1',
                 'qc07f_owned_runtime_cleanup_v1', 'qc07f_owned_runtime_memory_span_hub_v1',
                 'qc07f_owned_runtime_span_serve_v1', 'nh_native_watchdog_take',
                 'nh_native_provider_take', 'nh_native_storage_select', 'nh_native_optin_exact139',
                 'nh_native_memory_start', 'nh_native_memory_fixed_buffers',
                 'nh_native_return_pre_memory_unknown_bootstrap', 'nh_native_watchdog_give_back',
                 'nh_native_next', 'candidate_prestarted_start', 'candidate_prestarted_finish',
                 'candidate_backend_revoke', 'candidate_backend_cancel',
                 'camera_span_hub_init_v1', 'camera_span_hub_subscribe_v1', 'camera_span_hub_resync_v1',
                 'camera_span_hub_unsubscribe_v1', 'camera_span_hub_publish_v1',
                 'camera_span_hub_acquire_v1', 'camera_span_hub_release_v1',
                 'camera_span_hub_lease_valid_v1', 'camera_span_hub_packet_v1',
                 'camera_span_hub_stop_v1', 'camera_span_hub_drained_v1', 'camera_span_hub_dispose_v1')
        excluded = {'owned_runtime', 'memory_reserve', 'memory_runtime', 'memory_reader',
                    'rtsp_server_candidate', 'packet_hub', 'sensor_baseline'}
        for role in ('host', 'arm'):
            tools = old['tools'][role]
            for item in tools.values():
                keep(Path(item['path']).resolve(), item)
                tool(item['path'])
            cc, ar, nm, elf = (tools[k]['path'] for k in ('cc', 'ar', 'nm', 'readelf'))
            if role == 'arm':
                for relative, digest in old['toolchain_pins'].items():
                    p = Path(cc).parent.parent / relative
                    if pin(p)['sha256'] != digest:
                        raise RuntimeError('STOP: retained compiler support mismatch')
                    keep(p)
                tool(cc + '.br_real')
            command([cc, '--version'], role + '-compiler-version')
            command([cc, '-dumpmachine'], role + '-compiler-target')
            for name in ('cc1', 'as', 'ld'):
                p = command([cc, '-print-prog-name=' + name], role + '-tool-' + name).strip()
                tool(p if '/' in p else '/usr/bin/' + p)
            archive_command = next(c['argv'] for c in old['commands'] if 'rcsD' in c['argv']
                                   and c['argv'][2].endswith('-' + role + '.a'))
            archive = Path(archive_command[2])
            keep(archive, old['artifacts'][str(archive)])
            objects = list(map(Path, archive_command[3:]))
            if command([ar, 't', archive], role + '-archive-members').splitlines() != [p.name for p in objects]:
                raise RuntimeError('STOP: archive membership mismatch')
            selected = []
            for obj in objects:
                keep(obj, old['artifacts'][str(obj)])
                c = next(c for c in old['commands'] if '-c' in c['argv'] and c['argv'][-1] == str(obj))
                source = Path(c['argv'][c['argv'].index('-c') + 1])
                defines = [v for v in c['argv'] if v.startswith('-D')]
                if defines != ['-D_FORTIFY_SOURCE=2', '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']:
                    raise RuntimeError('STOP: reused role/fixture defines')
                omit = source.stem in excluded
                report['object_roles'][str(obj)] = {'source': str(source), 'defines': defines, 'excluded': omit}
                if omit:
                    continue
                selected.append(obj.name)
                dep = obj.with_suffix('.d')
                keep(dep)
                for name in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                    p = Path(name)
                    if p.name in {'owned_runtime_v1.h', 'memory_runtime.h', 'memory_reserve.h',
                                  'native_adapter.h', 'handoff.h', 'rtsp_span_server_v1.h', 'span_hub.h'}:
                        raise RuntimeError('STOP: layout consumer reuse refused')
                    if pin(p) != old['dependencies'][name]:
                        raise RuntimeError('STOP: reused dependency mismatch')
                    report['reused_dependencies'][name] = old['dependencies'][name]
            member_dir = out / ('members-' + role)
            command([ar, 'x', archive, *selected], role + '-selected-members', member_dir)
            reused = [member_dir / n for n in selected]
            for p in reused:
                if pin(p) != pin(OLD / 'objects' / p.name):
                    raise RuntimeError('STOP: extracted body mismatch')
            command([elf, '-h', '-A', *reused], role + '-member-roles')
            flags = ['-O2'] if role == 'host' else ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft']
            fresh = []
            for source in sources:
                obj = out / 'objects' / (role + '-' + source.stem + '.o')
                selection = (['-D_POSIX_C_SOURCE=200809L', '-D_DEFAULT_SOURCE',
                              '-include', str(MEMORY / 'handoff.h')]
                             if source.name == 'sensor_baseline.c' else [])
                command([cc, *common, *flags, *selection, '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', source, '-o', obj], role + '-compile-' + source.stem)
                fresh.append(obj)
            reloc = out / (role + '-span-serve-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *fresh, *reused, '-Wl,--gc-sections',
                     *('-Wl,--undefined=' + n for n in roots),
                     '-Wl,-Map=' + str(out / (role + '.map')), '-o', reloc], role + '-REL')
            defined = command([nm, '-g', '--defined-only', reloc], role + '-defined')
            undefined = command([nm, '-u', reloc], role + '-undefined')
            symbols = command([nm, '-a', reloc], role + '-all-symbols')
            command([elf, '-h', '-A', '-S', reloc], role + '-ELF')
            table = command([elf, '-s', '-W', reloc], role + '-symbol-table')
            relocations = command([elf, '-r', '-W', reloc], role + '-relocations')
            live = (*roots, 'qc07f_owned_runtime_span_entry_v1', 'qc07f_owned_runtime_span_end_borrow_v1',
                    'nh_memory_prefault_span_hub', 'nh_memory_span_hub_forecast',
                    'nh_memory_credit_span_hub', 'nh_memory_read_fixed', 'camera_rtp_h264_spans_packet_v1')
            if any(defined.split().count(n) != 1 for n in live):
                raise RuntimeError('STOP: real integration API absent/duplicate')
            forbidden = ('main', 'camera_hub_publish', 'camera_rtsp_prestarted_owned_listener_serve_v1',
                         'camera_rtsp_prestarted_span_listener_serve_v1')
            if any(n in defined.split() for n in forbidden):
                raise RuntimeError('STOP: excluded server/old hub/executable entry')
            private = {n: sum(line.split()[-1:] == [n] for line in symbols.splitlines())
                       for n in ('native', 'memory_state', 'runtime')}
            report.setdefault('private_owner_symbols', {})[role] = private
            if any(v != 1 for v in private.values()):
                raise RuntimeError('STOP: private singleton count')
            raw = [line.split()[-1] for line in undefined.splitlines() if line.strip()]
            referenced = [n for n in raw if any(n in line.split() for line in relocations.splitlines())]
            report.setdefault('raw_undefined', {})[role] = raw
            report.setdefault('relocation_referenced_undefined', {})[role] = referenced
            digest = ('camera_rtsp_digest_close', 'camera_rtsp_digest_issue', 'camera_rtsp_digest_verify')
            records = {}
            for n in digest:
                rows = [line.split() for line in table.splitlines() if line.split()[-1:] == [n]]
                references = [line for line in relocations.splitlines() if n in line.split()]
                if len(rows) != 1 or rows[0][3:7] != ['NOTYPE', 'LOCAL', 'DEFAULT', 'UND'] or \
                   rows[0][2] != '0' or references or n not in raw:
                    raise RuntimeError('STOP: raw Digest UND role/reference mismatch')
                records[n] = {'symbol': rows[0], 'retained_references': 0}
            report.setdefault('raw_digest_undefined_preserved', {})[role] = records
            if any(n.startswith(('camera_', 'candidate_', 'qc07f_', 'qc_', 'qw_', 'qp_',
                                 'nh_', 'rv_', 'is_rv_', 'mbedtls_')) for n in referenced):
                raise RuntimeError('STOP: project relocation unresolved; partial REL only')
        for dep in sorted((out / 'objects').glob('*.d')):
            for name in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][name] = pin(name)
                if name.startswith(str(ORIGINAL / 'native') + '/') and Path(name).name in {'memory_runtime.h', 'memory_reserve.h'}:
                    raise RuntimeError('STOP: old memory definitions selected')
        if any(pin(p) != expected for p, expected in report['inputs'].items()):
            raise RuntimeError('STOP: source/artifact drift during compile')
        if any(pin(Path(p).resolve()) != {k: expected[k] for k in ('bytes', 'sha256')}
               for p, expected in report['tools'].items()):
            raise RuntimeError('STOP: tool drift during compile')
        report.update(status='HOST_ARM_16_COMPILES_2_ROOTED_REL_PASS_NOT_RUN',
                      C_compiles=16, rooted_REL_links=2, inputs_unchanged=True)
    except BaseException as exc:
        report.update(status='FAILED_PRESERVED', error=type(exc).__name__ + ': ' + str(exc))
        raise
    finally:
        report['artifacts'] = {str(p): pin(p) for p in sorted(out.rglob('*')) if p.is_file()}
        report['ended'] = now()
        with (out / 'REPORT.json').open('x') as f:
            json.dump(report, f, indent=2)
            f.write('\n')
        print(out / 'REPORT.json')


if __name__ == '__main__':
    main()
