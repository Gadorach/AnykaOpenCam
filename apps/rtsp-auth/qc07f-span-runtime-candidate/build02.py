#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""One bounded offline build: recompile real sensor consumer, never reuse it."""
from pathlib import Path
import json
import shlex
import subprocess
import difflib
from build import APP, ROOT, DOC, OLD, ORIGINAL, MEMORY, MEMDOC, TOOLPINS, PAIRS, pin, now


def main():
    out = DOC / 'runs/build02'
    out.mkdir(parents=True, exist_ok=False)
    for n in ('logs', 'objects', 'members-host', 'members-arm', 'tmp'):
        (out / n).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(out / 'tmp'),
           'SOURCE_DATE_EPOCH': '1791763200', 'PYTHONDONTWRITEBYTECODE': '1'}
    old = json.loads((OLD / 'REPORT.json').read_text())
    retained = {p['resolved']: p for p in json.loads(TOOLPINS.read_text())}
    for role_tools in old['tools'].values():
        for p in role_tools.values():
            retained.setdefault(str(Path(p['path']).resolve()), p)
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'dependencies': {}, 'reused_dependencies': {}, 'object_roles': {},
              'tools': {}, 'environment': env, 'runtime_calls': 0, 'executable': False,
              'scope': 'ACTUAL_SOURCE_COMPILE_ROOTED_REL_ONLY',
              'prior_failure': str(DOC / 'runs/build01/REPORT.json')}

    def keep(p, expected=None):
        actual = pin(p)
        if expected and actual != {k: expected[k] for k in ('bytes', 'sha256')}:
            raise RuntimeError('STOP: retained input drift: ' + str(p))
        report['inputs'][str(p)] = actual

    def tool(p):
        resolved = str(Path(p).resolve())
        expected = retained.get(resolved)
        if not expected or pin(Path(resolved)) != {k: expected[k] for k in ('bytes', 'sha256')}:
            raise RuntimeError('STOP: tool mismatch: ' + str(p))
        report['tools'][str(p)] = {'resolved': resolved, **pin(Path(resolved))}

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
        if old['status'] != 'HOST_ARM_COMPILE_ARCHIVE_REL_PASS_NOT_RUN':
            raise RuntimeError('STOP: retained baseline status')
        for p in (OLD / 'REPORT.json', TOOLPINS, DOC / 'CONTRACT.md',
                  DOC / 'AUTHOR_AUDIT.md', DOC / 'BUILD01_DISPOSITION.md',
                  DOC / 'AUTHOR_AUDIT02.md', DOC / 'runs/build01/REPORT.json',
                  MEMDOC / 'PARENT_DISPOSITION.md', MEMDOC / 'runs/build01/INPUTS.json',
                  ROOT / 'docs/qualification-plans/span-hub-parent-20261024/V2_DISPOSITION.md'):
            keep(p)
        for name, expected in old['inputs'].items():
            if name.startswith(str(ORIGINAL) + '/') or name.startswith(str(ROOT / 'framework') + '/'):
                keep(Path(name), expected)
        for p in sorted(APP.rglob('*')):
            if p.is_file():
                keep(p)
        # Pin accepted reader/header/C inputs to the actual memory build record.
        for item in json.loads((MEMDOC / 'runs/build01/INPUTS.json').read_text()):
            p = Path(item['path'])
            if p.parent == MEMORY or p.parent in (APP.parent / 'span-hub-v2', APP.parent / 'rtp-span-candidate'):
                keep(p, item)
        for name, a, b in (('ORIGINAL_TO_CANDIDATE.diff', ORIGINAL, APP),
                            ('CANDIDATE_TO_ORIGINAL.diff', APP, ORIGINAL)):
            with (out / name).open('x') as f:
                for relative in PAIRS:
                    f.writelines(difflib.unified_diff((a / relative).read_text().splitlines(True),
                        (b / relative).read_text().splitlines(True), str(a / relative), str(b / relative)))
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common',
                  '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
                  '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2', '-fstack-usage',
                  '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"', '-Wno-misleading-indentation']
        for p in (APP, APP / 'native', ORIGINAL / 'server', ORIGINAL / 'native',
                  ROOT / 'framework/include', APP.parent / 'isolated-verifier'):
            common.append('-I' + str(p))
        sources = [APP / 'owned_runtime.c', MEMORY / 'memory_reserve.c',
                   MEMORY / 'memory_runtime.c', MEMORY / 'memory_reader.c',
                   APP.parent / 'span-hub-v2/span_hub.c',
                   APP.parent / 'rtp-span-candidate/rtp_h264_spans.c',
                   ORIGINAL / 'native/sensor_baseline.c']
        roots = ('qc07f_owned_runtime_prepare_v1', 'qc07f_owned_runtime_bind_postfork_v1',
                 'qc07f_owned_runtime_cleanup_v1', 'qc07f_owned_runtime_memory_span_hub_v1',
                 'nh_native_watchdog_take', 'nh_native_provider_take', 'nh_native_storage_select',
                 'nh_native_optin_exact139', 'nh_native_memory_start', 'nh_native_memory_fixed_buffers',
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
                path = command([cc, '-print-prog-name=' + name], role + '-tool-' + name).strip()
                tool(path if '/' in path else '/usr/bin/' + path)
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
                    if p.name in {'owned_runtime_v1.h', 'memory_runtime.h', 'memory_reserve.h', 'native_adapter.h', 'handoff.h'}:
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
                # Recompile actual frozen sensor body, not a wrapper/stub. The
                # COMPLETE new handoff is explicitly selected before its legacy
                # include. Its legitimate guard excludes old definitions, not
                # required declarations. Feature macros precede system headers.
                selection = (['-D_POSIX_C_SOURCE=200809L', '-D_DEFAULT_SOURCE',
                              '-include', str(MEMORY / 'handoff.h')]
                             if source.name == 'sensor_baseline.c' else [])
                command([cc, *common, *flags, *selection, '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', source, '-o', obj], role + '-compile-' + source.stem)
                fresh.append(obj)
            reloc = out / (role + '-span-runtime-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *fresh, *reused, '-Wl,--gc-sections',
                     *('-Wl,--undefined=' + n for n in roots),
                     '-Wl,-Map=' + str(out / (role + '.map')), '-o', reloc], role + '-REL')
            defined = command([nm, '-g', '--defined-only', reloc], role + '-defined')
            undefined = command([nm, '-u', reloc], role + '-undefined')
            symbols = command([nm, '-a', reloc], role + '-all-symbols')
            command([elf, '-h', '-A', '-S', reloc], role + '-ELF')
            relocations = command([elf, '-r', '-W', reloc], role + '-relocations')
            if any(defined.split().count(n) != 1 for n in (*roots, 'nh_memory_prefault_span_hub',
                    'nh_memory_span_hub_forecast', 'nh_memory_credit_span_hub',
                    'nh_memory_read_fixed', 'camera_rtp_h264_spans_packet_v1')):
                raise RuntimeError('STOP: real integration API root absent/duplicate')
            if any(n in defined.split() for n in ('main', 'camera_hub_publish',
                    'camera_rtsp_prestarted_owned_listener_serve_v1', 'camera_rtsp_prestarted_span_listener_serve_v1')):
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
            if any(n.startswith(('camera_', 'candidate_', 'qc07f_', 'qc_', 'qw_', 'qp_',
                                 'nh_', 'rv_', 'is_rv_', 'mbedtls_')) for n in referenced):
                raise RuntimeError('STOP: project relocation unresolved; partial REL only')
        for dep in sorted((out / 'objects').glob('*.d')):
            for name in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][name] = pin(Path(name))
                if name.startswith(str(ORIGINAL / 'native') + '/') and Path(name).name in {'memory_runtime.h', 'memory_reserve.h'}:
                    raise RuntimeError('STOP: actual old memory definitions selected')
        if any(pin(Path(p)) != expected for p, expected in report['inputs'].items()):
            raise RuntimeError('STOP: source/artifact drift during compile')
        if any(pin(Path(p).resolve()) != {k: expected[k] for k in ('bytes', 'sha256')}
               for p, expected in report['tools'].items()):
            raise RuntimeError('STOP: tool drift during compile')
        report.update(status='HOST_ARM_14_COMPILES_2_ROOTED_REL_PASS_NOT_RUN',
                      C_compiles=14, rooted_REL_links=2, inputs_unchanged=True)
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
