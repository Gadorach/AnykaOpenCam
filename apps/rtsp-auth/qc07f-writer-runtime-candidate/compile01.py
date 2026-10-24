#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fixed production compile/rooted REL/metadata ONLY; never import/run candidates."""
from pathlib import Path
import datetime
import difflib
import hashlib
import json
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/qc07f-writer-runtime-20261024'
BASE = APP.parent / 'qc07f-owned-runtime'
MEM = APP.parent / 'qc07f-producer-memory'
HUB = APP.parent / 'span-hub-producer-candidate'
ENC = ROOT / 'apps/qualification/qc07f-encoder-output-spans'
OLD = ROOT / 'docs/qualification-plans/rtsp-owned-runtime-20261023/runs/build02'
PRIOR = ROOT / 'docs/qualification-plans/qc07f-span-serve-20261024/runs/build01/REPORT.json'
FROZEN = [BASE, APP.parent / 'qc07f-span-runtime-candidate', APP.parent / 'qc07f-span-serve-candidate',
          MEM, HUB, APP.parent / 'rtp-span-candidate', ENC]
PAIRS = [(APP.parent / 'qc07f-span-serve-candidate/owned_runtime.c', APP / 'owned_runtime.c'),
         (APP.parent / 'qc07f-span-serve-candidate/owned_runtime_v1.h', APP / 'owned_runtime_v4.h'),
         (APP.parent / 'qc07f-span-runtime-candidate/native/native_adapter.c', APP / 'native_adapter.inc'),
         (BASE / 'native/native-watchdog.inc', APP / 'native-watchdog.inc')]


def pin(p):
    b = Path(p).read_bytes()
    return {'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    out = DOC / 'runs/build01'
    out.mkdir(parents=True, exist_ok=False)
    for n in ('logs', 'objects', 'members-host', 'members-arm', 'source', 'tmp'):
        (out / n).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(out / 'tmp'),
           'SOURCE_DATE_EPOCH': '1791763200', 'PYTHONDONTWRITEBYTECODE': '1'}
    old = json.loads((OLD / 'REPORT.json').read_text())
    prior = json.loads(PRIOR.read_text())
    retained = {v['resolved']: v for v in prior['tools'].values()}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'dependencies': {}, 'reused_dependencies': {}, 'tools': {}, 'object_roles': {},
              'environment': env, 'candidate_calls': 0, 'executable': False}

    def keep(p, expected=None):
        actual = pin(p)
        if expected and actual != {k: expected[k] for k in ('bytes', 'sha256')}:
            raise RuntimeError('STOP retained input drift: ' + str(p))
        report['inputs'][str(p)] = actual

    def tool(p):
        resolved = str(Path(p).resolve())
        if resolved not in retained or pin(resolved) != {k: retained[resolved][k] for k in ('bytes', 'sha256')}:
            raise RuntimeError('STOP retained tool drift: ' + str(p))
        report['tools'][str(p)] = {'resolved': resolved, **pin(resolved)}

    def command(argv, label, cwd=ROOT):
        argv = list(map(str, argv));tool(argv[0])
        log = out / 'logs' / (label + '.txt')
        c = {'argv': argv, 'cwd': str(cwd), 'started': now(), 'log': str(log)}
        report['commands'].append(c)
        with log.open('xb') as f:
            result = subprocess.run(argv, cwd=cwd, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=120)
        c.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('STOP offline tool failure: ' + label)
        return log.read_text()

    try:
        tool('/usr/bin/python3')
        keep(OLD / 'REPORT.json');keep(PRIOR);keep(DOC / 'CONTRACT.md')
        if old['status'] != 'HOST_ARM_COMPILE_ARCHIVE_REL_PASS_NOT_RUN' or prior['status'] != 'HOST_ARM_16_COMPILES_2_ROOTED_REL_PASS_NOT_RUN':
            raise RuntimeError('STOP baseline status')
        exact = {HUB / 'span_hub.c': '72e06177ae86c8659972ba8a59c81208302d44e0cb78f29b6114f8a4d5631f93',
                 HUB / 'span_hub.h': '460320064f85d1f1f8ada1a9afd1e282c287583fdff34787ba63c7354adeed8e',
                 MEM / 'memory_reserve.c': 'f5c3d87aa2c6272fc5a54eb3f95af6590ceca7a5d6e95e52c1dd969790c1ec48',
                 MEM / 'memory_reserve.h': '429563e8a85b71ce66fd9e0f6ba6d6ef386173126efa3bcadf5ce7e543c14195',
                 ENC / 'encoder-output-spans.h': '4267d4074177468898937926b35029fbfb2b392c25ea987832c83137841cb6e6'}
        for p, digest in exact.items():
            if pin(p)['sha256'] != digest:
                raise RuntimeError('STOP accepted source identity: ' + str(p))
        frozen = {str(p.relative_to(ROOT)): pin(p) for d in FROZEN for p in sorted(d.rglob('*')) if p.is_file()}
        with (out / 'FROZEN_INPUTS.json').open('x') as f:
            json.dump({'roots': [str(d.relative_to(ROOT)) for d in FROZEN], 'files': frozen}, f, indent=2);f.write('\n')
        for n, expected in frozen.items():
            keep(ROOT / n, expected)
        for lane in ('span-hub-producer', 'qc07f-producer-memory', 'qc07f-encoder-output-spans', 'qc07f-span-serve'):
            for p in (ROOT / f'docs/qualification-plans/{lane}-20261024/PARENT_DISPOSITION.md',
                      ROOT / f'docs/qualification-plans/{lane}-review-20261024/REVIEW.md',
                      ROOT / f'docs/qualification-plans/{lane}-20261024/CONTRACT.md'):
                keep(p)
        for p in sorted(APP.iterdir()):
            if p.is_file():
                keep(p)
                with (out / 'source' / p.name).open('xb') as f:
                    f.write(p.read_bytes())
        for a, b in PAIRS:
            for inverse in (False, True):
                x, y = (b, a) if inverse else (a, b)
                name = b.name + ('.inverse.diff' if inverse else '.forward.diff')
                with (out / name).open('x') as f:
                    f.writelines(difflib.unified_diff(x.read_text().splitlines(True), y.read_text().splitlines(True), str(x), str(y)))
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common', '-fno-strict-aliasing',
                  '-ffunction-sections', '-fdata-sections', '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2',
                  '-fstack-usage', '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"', '-Wno-misleading-indentation']
        for d in (APP, BASE / 'server', BASE / 'native', ROOT / 'framework/include', APP.parent / 'isolated-verifier'):
            common.append('-I' + str(d))
        sources = [APP / 'owned_runtime.c', MEM / 'memory_reserve.c', MEM / 'memory_runtime.c', MEM / 'memory_reader.c',
                   HUB / 'span_hub.c', APP.parent / 'rtp-span-candidate/rtp_h264_spans.c', BASE / 'native/sensor_baseline.c']
        roots = ['qc07f_owned_runtime_prepare_v4', 'qc07f_owned_runtime_bind_postfork_v4',
                 'qc07f_owned_runtime_memory_span_hub_v4', 'qc07f_owned_runtime_cleanup_v4',
                 'nh_native_watchdog_take', 'nh_native_provider_take', 'nh_native_storage_select', 'nh_native_optin_exact139',
                 'nh_native_memory_start', 'nh_native_memory_fixed_buffers', 'nh_native_return_pre_memory_unknown_bootstrap',
                 'nh_native_watchdog_give_back', 'nh_native_next', 'nh_native_next_keyframe', 'nh_native_release',
                 'nh_native_retire_held', 'nh_native_retire', 'candidate_prestarted_start', 'candidate_prestarted_finish',
                 'candidate_backend_revoke', 'candidate_backend_cancel']
        # Actual declarations, not an invented subset; all production memory/hub APIs.
        import re
        for p in (MEM / 'memory_reserve.h', MEM / 'memory_runtime.h', HUB / 'span_hub.h'):
            roots += re.findall(r'^(?:int|void)\s+((?:nh_memory_|camera_span_hub_)\w+)\(', p.read_text(), re.M)
        roots = list(dict.fromkeys(roots))
        report['roots'] = roots
        excluded = {'owned_runtime', 'memory_reserve', 'memory_runtime', 'memory_reader', 'rtsp_server_candidate', 'packet_hub', 'sensor_baseline'}
        for role in ('host', 'arm'):
            ts = old['tools'][role]
            cc, ar, nm, elf = [ts[k]['path'] for k in ('cc', 'ar', 'nm', 'readelf')]
            for v in ts.values():
                keep(Path(v['path']).resolve(), v);tool(v['path'])
            if role == 'arm':
                for n, digest in old['toolchain_pins'].items():
                    p = Path(cc).parent.parent / n
                    if pin(p)['sha256'] != digest:
                        raise RuntimeError('STOP retained compiler support drift')
                    keep(p)
                tool(cc + '.br_real')
            command([cc, '--version'], role + '-version');command([cc, '-dumpmachine'], role + '-machine')
            for n in ('cc1', 'as', 'ld'):
                p = command([cc, '-print-prog-name=' + n], role + '-tool-' + n).strip();tool(p if '/' in p else '/usr/bin/' + p)
            ac = next(c['argv'] for c in old['commands'] if 'rcsD' in c['argv'] and c['argv'][2].endswith('-' + role + '.a'))
            archive = Path(ac[2]);keep(archive, old['artifacts'][str(archive)])
            objects = list(map(Path, ac[3:]))
            if command([ar, 't', archive], role + '-archive-members').splitlines() != [p.name for p in objects]:
                raise RuntimeError('STOP archive membership')
            selected = []
            for obj in objects:
                keep(obj, old['artifacts'][str(obj)])
                c = next(c for c in old['commands'] if '-c' in c['argv'] and c['argv'][-1] == str(obj))
                source = Path(c['argv'][c['argv'].index('-c') + 1]);keep(source, old['inputs'][str(source)])
                defines = [v for v in c['argv'] if v.startswith('-D')]
                if defines != ['-D_FORTIFY_SOURCE=2', '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']:
                    raise RuntimeError('STOP reused nonproduction role')
                omit = source.stem in excluded
                report['object_roles'][str(obj)] = {'source': str(source), 'defines': defines, 'excluded': omit}
                if omit:
                    continue
                selected.append(obj.name);dep = obj.with_suffix('.d');keep(dep)
                for n in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                    p = Path(n)
                    if p.name in {'owned_runtime_v1.h', 'memory_runtime.h', 'memory_reserve.h', 'native_adapter.h', 'handoff.h', 'span_hub.h'}:
                        raise RuntimeError('STOP old layout consumer reuse')
                    keep(p, old['dependencies'][n]);report['reused_dependencies'][n] = pin(p)
            directory = out / ('members-' + role)
            command([ar, 'x', archive, *selected], role + '-extract', directory)
            reused = [directory / n for n in selected]
            for p in reused:
                if pin(p) != pin(OLD / 'objects' / p.name):
                    raise RuntimeError('STOP extracted body mismatch')
            command([elf, '-h', '-A', *reused], role + '-member-ELF')
            flags = ['-O2'] if role == 'host' else ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft']
            fresh = []
            for source in sources:
                obj = out / 'objects' / (role + '-' + source.stem + '.o')
                select = ['-D_POSIX_C_SOURCE=200809L', '-D_DEFAULT_SOURCE', '-include', str(MEM / 'handoff.h')] if source.name == 'sensor_baseline.c' else []
                command([cc, *common, *flags, *select, '-MD', '-MF', obj.with_suffix('.d'), '-c', source, '-o', obj], role + '-compile-' + source.stem)
                fresh.append(obj)
            reloc = out / (role + '-WRITER-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *fresh, *reused, '-Wl,--gc-sections',
                     *('-Wl,--undefined=' + n for n in roots), '-Wl,-Map=' + str(out / (role + '.map')), '-o', reloc], role + '-REL')
            defined = command([nm, '-g', '--defined-only', reloc], role + '-defined')
            raw = command([nm, '-u', reloc], role + '-undefined')
            symbols = command([nm, '-a', reloc], role + '-all-symbols')
            command([elf, '-h', '-A', '-S', reloc], role + '-ELF')
            table = command([elf, '-s', '-W', reloc], role + '-symbol-table')
            rels = command([elf, '-r', '-W', reloc], role + '-relocations')
            if any(defined.split().count(n) != 1 for n in [*roots, 'camera_rtp_h264_spans_packet_v1']):
                raise RuntimeError('STOP missing/duplicate production API')
            forbidden = ('main', '_start', 'camera_hub_publish', 'qc07f_owned_runtime_prepare_v1', 'qc07f_owned_runtime_span_serve_v1',
                         'camera_rtsp_prestarted_owned_listener_serve_v1', 'camera_rtsp_prestarted_span_listener_serve_v1')
            if any(n in defined.split() for n in forbidden):
                raise RuntimeError('STOP excluded frontend/old runtime/executable')
            for n in ('native', 'memory_state', 'runtime'):
                if sum(line.split()[-1:] == [n] for line in symbols.splitlines()) != 1:
                    raise RuntimeError('STOP private singleton count')
            unresolved = [line.split()[-1] for line in raw.splitlines() if line.strip()]
            referenced = [n for n in unresolved if any(n in line.split() for line in rels.splitlines())]
            report.setdefault('raw_undefined', {})[role] = unresolved
            report.setdefault('referenced_undefined', {})[role] = referenced
            for n in ('camera_rtsp_digest_close', 'camera_rtsp_digest_issue', 'camera_rtsp_digest_verify'):
                rows = [line.split() for line in table.splitlines() if line.split()[-1:] == [n]]
                if len(rows) != 1 or rows[0][3:7] != ['NOTYPE', 'LOCAL', 'DEFAULT', 'UND'] or rows[0][2] != '0' or n in referenced or n not in unresolved:
                    raise RuntimeError('STOP raw unreferenced Digest UND disposition')
            if any(n.startswith(('camera_', 'candidate_', 'qc07f_', 'qc_', 'qw_', 'nh_', 'rv_', 'is_rv_', 'mbedtls_')) for n in referenced):
                raise RuntimeError('STOP project relocation unresolved: honest partial REL only')
        for dep in sorted((out / 'objects').glob('*.d')):
            for n in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][n] = pin(n)
                if str(BASE / 'native/memory_') in n:
                    raise RuntimeError('STOP old native memory layout')
        for d in FROZEN:
            if {str(p.relative_to(ROOT)) for p in d.rglob('*') if p.is_file()} != {n for n in frozen if n.startswith(str(d.relative_to(ROOT)) + '/')}:
                raise RuntimeError('STOP frozen membership drift')
        if any(pin(n) != expected for n, expected in report['inputs'].items()):
            raise RuntimeError('STOP consumed input drift')
        report.update(status='HOST_ARM_14_COMPILES_2_ROOTED_REL_PASS_NOT_RUN', C_compiles=14, rooted_REL_links=2, frozen_unchanged=True)
    except BaseException as exc:
        report.update(status='FAILED_PRESERVED', error=type(exc).__name__ + ': ' + str(exc));raise
    finally:
        # Also retain dependency custody if a fresh compile was the failure.
        for dep in sorted((out / 'objects').glob('*.d')):
            for n in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][n] = pin(n)
        report['ended'] = now()
        report['artifacts'] = {str(p): pin(p) for p in sorted(out.rglob('*')) if p.is_file()}
        with (out / 'REPORT.json').open('x') as f:
            json.dump(report, f, indent=2);f.write('\n')
        print(out / 'REPORT.json')


if __name__ == '__main__':
    main()
