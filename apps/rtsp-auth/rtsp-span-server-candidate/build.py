#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fixed offline HOST/ARM compile + real-member REL only; no program/test calls."""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/rtsp-span-server-20261024'
OLD = ROOT / 'docs/qualification-plans/rtsp-owned-runtime-20261023/runs/build02'
RUNTIME = ROOT / 'apps/rtsp-auth/qc07f-owned-runtime'


def pin(p):
    data = p.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    out = DOC / 'build03'
    out.mkdir()  # fixed fresh name; build01/02 failures preserved, no automatic retry
    for name in ('logs', 'objects', 'members-host', 'members-arm', 'tmp'):
        (out / name).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(out / 'tmp'),
           'SOURCE_DATE_EPOCH': '1791763200'}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'dependencies': {}, 'artifacts': {}, 'environment': env,
              'scope': 'SOURCE_COMPILE_REL_ONLY', 'runtime_calls': 0}

    def keep(p):
        report['inputs'][str(p)] = pin(p)

    def command(argv, label, cwd=ROOT):
        argv = list(map(str, argv))
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'cwd': str(cwd), 'started': now(), 'log': str(log)}
        report['commands'].append(entry)
        with log.open('xb') as f:
            r = subprocess.run(argv, cwd=cwd, env=env, stdout=f,
                               stderr=subprocess.STDOUT, timeout=120)
        entry.update(exit=r.returncode, ended=now(), log_pin=pin(log))
        if r.returncode:
            raise RuntimeError('offline tool failure: ' + label)
        return log.read_text()

    try:
        keep(OLD / 'REPORT.json')
        old = json.loads((OLD / 'REPORT.json').read_text())
        if old['status'] != 'HOST_ARM_COMPILE_ARCHIVE_REL_PASS_NOT_RUN':
            raise RuntimeError('unexpected retained baseline')
        tc = Path(old['tools']['arm']['cc']['path']).parent.parent
        for name, digest in old['toolchain_pins'].items():
            p = tc / name
            if pin(p)['sha256'] != digest:
                raise RuntimeError('retained ARM compiler/support pin changed')
            keep(p)
        # Selected old source/header closure and all reused artifact identities,
        # not a new snapshot of unrelated vendor trees or old receipts.
        for name, expected in old['inputs'].items():
            p = Path(name)
            if str(p).startswith(str(RUNTIME) + '/') or str(p).startswith(str(ROOT / 'framework') + '/'):
                if pin(p) != expected:
                    raise RuntimeError('retained original input changed: ' + name)
                keep(p)
        for p in sorted(APP.iterdir()):
            if p.is_file():
                keep(p)
                with (out / ('consumed-' + p.name)).open('xb') as f:
                    f.write(p.read_bytes())
        for directory in ('span-hub-v2', 'rtp-span-candidate'):
            for suffix in ('*.c', '*.h'):
                for p in sorted((APP.parent / directory).glob(suffix)):
                    keep(p)
        keep(DOC / 'CONTRACT.md')
        base = RUNTIME / 'server/rtsp_server_candidate.c'
        new = APP / 'rtsp_span_server.c'
        for name, a, b in (('SERVER_DELTA.patch', base, new),
                           ('SERVER_INVERSE.patch', new, base)):
            with (out / name).open('x') as f:
                f.writelines(difflib.unified_diff(a.read_text().splitlines(True),
                             b.read_text().splitlines(True), str(a), str(b)))
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common',
                  '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
                  '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2', '-fstack-usage',
                  '-Wno-misleading-indentation']
        for p in (RUNTIME, RUNTIME / 'server', RUNTIME / 'native',
                  ROOT / 'framework/include', APP.parent / 'isolated-verifier'):
            common.append('-I' + str(p))
        sources = [new, APP.parent / 'span-hub-v2/span_hub.c',
                   APP.parent / 'rtp-span-candidate/rtp_h264_spans.c']
        for target in ('host', 'arm'):
            tools = old['tools'][target]
            for tool in tools.values():
                p = Path(tool['path']).resolve()
                if pin(p) != {k: tool[k] for k in ('bytes', 'sha256')}:
                    raise RuntimeError('retained tool changed: ' + str(p))
                keep(p)
            cc, ar, nm, elf = (tools[k]['path'] for k in ('cc', 'ar', 'nm', 'readelf'))
            flags = ['-O2'] if target == 'host' else ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft']
            command([cc, '--version'], target + '-compiler-version')
            command([cc, '-dumpmachine'], target + '-compiler-target')
            # Inspect real archive roles/defines/pins before selecting members.
            archived = next(c['argv'] for c in old['commands']
                            if 'rcsD' in c['argv'] and c['argv'][2].endswith('-' + target + '.a'))
            archive = Path(archived[2])
            expected = old['artifacts'][str(archive)]
            if pin(archive) != {k: expected[k] for k in ('bytes', 'sha256')}:
                raise RuntimeError('retained archive changed')
            keep(archive)
            members = command([ar, 't', archive], target + '-archive-members').splitlines()
            objects = list(map(Path, archived[3:]))
            if members != [p.name for p in objects]:
                raise RuntimeError('archive membership mismatch')
            selected = []
            for obj in objects:
                expected = old['artifacts'][str(obj)]
                if pin(obj) != {k: expected[k] for k in ('bytes', 'sha256')}:
                    raise RuntimeError('retained object changed')
                keep(obj)
                compile_entry = next(c for c in old['commands']
                                     if '-c' in c['argv'] and c['argv'][-1] == str(obj))
                defines = [v for v in compile_entry['argv'] if v.startswith('-D')]
                if defines != ['-D_FORTIFY_SOURCE=2', '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']:
                    raise RuntimeError('unexpected fixture/role defines')
                report.setdefault('reused_roles', {})[str(obj)] = {
                    'source': compile_entry['argv'][compile_entry['argv'].index('-c') + 1],
                    'defines': defines, 'excluded': obj.stem.endswith(('-rtsp_server_candidate', '-packet_hub'))}
                if not report['reused_roles'][str(obj)]['excluded']:
                    selected.append(obj.name)
                    dep = obj.with_suffix('.d')
                    keep(dep)
                    for name in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                        expected_dep = old['dependencies'][name]
                        if pin(Path(name)) != expected_dep:
                            raise RuntimeError('reused dependency changed: ' + name)
                        report.setdefault('reused_dependencies', {})[name] = expected_dep
            member_dir = out / ('members-' + target)
            command([ar, 'x', archive, *selected], target + '-selected-members', member_dir)
            reused = [member_dir / n for n in selected]
            for p in reused:
                if pin(p) != pin(OLD / 'objects' / p.name):
                    raise RuntimeError('member body mismatch')
            command([elf, '-h', '-A', *reused], target + '-member-roles')
            fresh = []
            for i, source in enumerate(sources):
                obj = out / 'objects' / (target + '-' + source.stem + '.o')
                command([cc, *common, *flags, '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', source, '-o', obj], target + '-compile-' + str(i))
                fresh.append(obj)
            reloc = out / (target + '-span-server-NOT-EXECUTABLE.o')
            # Same explicit rooted section-GC method as retained build02.
            # Generic unused digest wrappers in framework rtsp_digest_wire.o
            # otherwise refer to unselected camera_rtsp_digest_*; build01 records
            # that actual unrooted closure failure, not a passing composition.
            roots = ('camera_rtsp_prestarted_span_listener_serve_v1',
                     'candidate_prestarted_start', 'candidate_prestarted_finish',
                     'qc07f_owned_runtime_prepare_v1', 'qc07f_owned_runtime_bind_postfork_v1',
                     'nh_native_watchdog_take', 'nh_native_provider_take',
                     'nh_native_storage_select', 'nh_native_optin_exact139',
                     'nh_native_memory_start', 'nh_native_memory_fixed_buffers',
                     'nh_native_return_pre_memory_unknown_bootstrap',
                     'nh_native_watchdog_give_back', 'nh_native_next',
                     'candidate_backend_revoke', 'candidate_backend_cancel',
                     'camera_span_hub_init_v1', 'camera_span_hub_dispose_v1')
            command([cc, *flags, '-nostdlib', '-r', *fresh, *reused,
                     '-Wl,--gc-sections', *('-Wl,--undefined=' + n for n in roots),
                     '-Wl,-Map=' + str(out / (target + '.map')), '-o', reloc], target + '-REL')
            defined = command([nm, '-g', '--defined-only', reloc], target + '-defined')
            undefined = command([nm, '-u', reloc], target + '-undefined')
            command([elf, '-h', '-A', '-S', reloc], target + '-ELF')
            relocations = command([elf, '-r', '-W', reloc], target + '-relocations')
            required = (*roots, 'camera_rtsp_prestarted_span_listener_serve_v1', 'candidate_prestarted_start',
                     'candidate_prestarted_finish', 'qc07f_owned_runtime_cleanup_v1',
                     'camera_span_hub_publish_v1', 'camera_span_hub_packet_v1',
                     'camera_rtp_h264_spans_packet_v1')
            if any(defined.split().count(name) != 1 for name in required):
                raise RuntimeError('missing/duplicate actual entry')
            if any(name in defined.split() for name in ('main', 'camera_rtsp_prestarted_owned_listener_serve_v1', 'camera_hub_publish')):
                raise RuntimeError('old server/hub or executable entry included')
            unresolved = [line.split()[-1] for line in undefined.splitlines() if line.strip()]
            report.setdefault('undefined', {})[target] = unresolved
            # GNU partial links preserve unused UND names even after their only
            # referencing sections were GC'd. Retained build02 checks actual
            # relocation references, not presence in nm's raw symbol table.
            referenced = [n for n in unresolved if any(n in line.split() for line in relocations.splitlines())]
            report.setdefault('unresolved_references', {})[target] = referenced
            if any(n.startswith(('camera_', 'candidate_', 'qc07f_', 'qw_', 'qp_', 'nh_', 'rv_', 'is_rv_', 'mbedtls_')) for n in referenced):
                raise RuntimeError('unresolved actual project symbol')
        for dep in sorted((out / 'objects').glob('*.d')):
            for name in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][name] = pin(Path(name))
        if any(pin(Path(n)) != expected for n, expected in report['inputs'].items()):
            raise RuntimeError('input changed during build')
        report['status'] = 'HOST_ARM_6_COMPILES_2_REAL_REL_PASS_NOT_RUN'
    except BaseException as exc:
        report.update(status='FAILED_PRESERVED', error=type(exc).__name__ + ': ' + str(exc))
        raise
    finally:
        for p in sorted(out.rglob('*')):
            if p.is_file():
                report['artifacts'][str(p)] = pin(p)
        report['ended'] = now()
        with (out / 'REPORT.json').open('x') as f:
            json.dump(report, f, indent=2)
            f.write('\n')
        print(out / 'REPORT.json')


if __name__ == '__main__':
    main()
