#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fixed offline real-TU HOST/ARM compile + rooted REL; NEVER candidate calls."""
from pathlib import Path
import datetime
import difflib
import hashlib
import json
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/qc07f-span-runtime-20261024'
OLD = ROOT / 'docs/qualification-plans/rtsp-owned-runtime-20261023/runs/build02'
ORIGINAL = APP.parent / 'qc07f-owned-runtime'
MEMORY = APP.parent / 'qc07f-span-memory'
MEMDOC = ROOT / 'docs/qualification-plans/qc07f-span-memory-20261024'
TOOLPINS = ROOT / 'docs/qualification-plans/span-hub-v2-20261024/runs/build01/TOOLS.json'
PAIRS = ('owned_runtime.c', 'owned_runtime_v1.h', 'native/native_adapter.c',
         'native/native_adapter.h', 'native/provider-return.inc')


def pin(p):
    data = p.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    out = DOC / 'runs/build01'
    out.mkdir(parents=True, exist_ok=False)  # fixed fresh name, never overwrite/retry
    for n in ('logs', 'objects', 'members-host', 'members-arm', 'tmp'):
        (out / n).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(out / 'tmp'),
           'SOURCE_DATE_EPOCH': '1791763200', 'PYTHONDONTWRITEBYTECODE': '1'}
    report = {'started': now(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'dependencies': {}, 'reused_dependencies': {}, 'tools': {},
              'artifacts': {}, 'environment': env, 'runtime_calls': 0,
              'scope': 'ACTUAL_SOURCE_COMPILE_ROOTED_REL_ONLY', 'executable': False}
    retained_tools = {p['resolved']: p for p in json.loads(TOOLPINS.read_text())}

    def keep(p):
        report['inputs'][str(p)] = pin(p)

    def tool(p):
        p = Path(p)
        resolved = p.resolve()
        expected = retained_tools.get(str(resolved))
        if not expected or pin(resolved) != {k: expected[k] for k in ('bytes', 'sha256')}:
            raise RuntimeError('STOP: retained tool mismatch: ' + str(p))
        report['tools'][str(p)] = {'resolved': str(resolved), **pin(resolved)}

    def command(argv, label, cwd=ROOT):
        argv = list(map(str, argv))
        tool(argv[0])
        log = out / 'logs' / (label + '.txt')
        entry = {'argv': argv, 'cwd': str(cwd), 'started': now(), 'log': str(log)}
        report['commands'].append(entry)
        with log.open('xb') as f:
            result = subprocess.run(argv, cwd=cwd, env=env, stdout=f,
                                    stderr=subprocess.STDOUT, timeout=120)
        entry.update(exit=result.returncode, ended=now(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('STOP: offline tool failed: ' + label)
        return log.read_text()

    try:
        tool('/usr/bin/python3')
        for p in (OLD / 'REPORT.json', TOOLPINS, DOC / 'CONTRACT.md',
                  DOC / 'AUTHOR_AUDIT.md', MEMDOC / 'PARENT_DISPOSITION.md',
                  MEMDOC / 'CONTRACT.md', MEMDOC / 'HANDOFF.md',
                  ROOT / 'docs/qualification-plans/span-hub-parent-20261024/V2_DISPOSITION.md'):
            keep(p)
        old = json.loads((OLD / 'REPORT.json').read_text())
        if old['status'] != 'HOST_ARM_COMPILE_ARCHIVE_REL_PASS_NOT_RUN':
            raise RuntimeError('STOP: unaccepted retained build status')
        # Archive/nm/ELF target tools are also retained by the actual original
        # build; the smaller hub pin set intentionally contains no archiver.
        for role_tools in old['tools'].values():
            for item in role_tools.values():
                resolved = str(Path(item['path']).resolve())
                if resolved not in retained_tools:
                    retained_tools[resolved] = item
        for name, expected in old['inputs'].items():
            if name.startswith(str(ORIGINAL) + '/') or name.startswith(str(ROOT / 'framework') + '/'):
                if pin(Path(name)) != expected:
                    raise RuntimeError('STOP: original source drift: ' + name)
                keep(Path(name))
        for p in sorted(APP.rglob('*')):
            if p.is_file():
                keep(p)
        expected = {
            MEMORY / 'memory_reserve.c': '3de7e750b5f846c1a77804113862881947074f42ed5718627efe70942337316a',
            MEMORY / 'memory_runtime.c': '0e0434b5a8a80493e2582f8efa890fb634dd21543da35808ffd706c3a0a00332',
            APP.parent / 'span-hub-v2/span_hub.c': 'bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1',
            APP.parent / 'span-hub-v2/span_hub.h': '310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454',
            APP.parent / 'rtp-span-candidate/rtp_h264_spans.c': 'a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e',
            APP.parent / 'rtp-span-candidate/rtp_h264_spans.h': '41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91',
        }
        memory_inputs = json.loads((MEMDOC / 'runs/build01/INPUTS.json').read_text())
        keep(MEMDOC / 'runs/build01/INPUTS.json')
        for item in memory_inputs:
            p = Path(item['path'])
            if p.parent == MEMORY:
                if pin(p) != {k: item[k] for k in ('bytes', 'sha256')}:
                    raise RuntimeError('STOP: accepted memory input drift')
                keep(p)
        for p, digest in expected.items():
            if pin(p)['sha256'] != digest:
                raise RuntimeError('STOP: accepted source drift: ' + str(p))
            keep(p)
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
                   APP.parent / 'rtp-span-candidate/rtp_h264_spans.c']
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
                    'rtsp_server_candidate', 'packet_hub'}
        for role in ('host', 'arm'):
            tools = old['tools'][role]
            for item in tools.values():
                p = Path(item['path'])
                if pin(p.resolve()) != {k: item[k] for k in ('bytes', 'sha256')}:
                    raise RuntimeError('STOP: original tool drift')
                tool(p)
            cc, ar, nm, elf = (tools[k]['path'] for k in ('cc', 'ar', 'nm', 'readelf'))
            tc = Path(cc).parent.parent
            if role == 'arm':
                for relative, digest in old['toolchain_pins'].items():
                    p = tc / relative
                    if pin(p)['sha256'] != digest:
                        raise RuntimeError('STOP: retained compiler/support drift')
                    keep(p)
                tool(str(cc) + '.br_real')
            command([cc, '--version'], role + '-compiler-version')
            command([cc, '-dumpmachine'], role + '-compiler-target')
            for name in ('cc1', 'as', 'ld'):
                path = command([cc, '-print-prog-name=' + name], role + '-tool-' + name).strip()
                tool(path if '/' in path else '/usr/bin/' + path)
            archived = next(c['argv'] for c in old['commands']
                            if 'rcsD' in c['argv'] and c['argv'][2].endswith('-' + role + '.a'))
            archive = Path(archived[2])
            keep(archive)
            if pin(archive) != {k: old['artifacts'][str(archive)][k] for k in ('bytes', 'sha256')}:
                raise RuntimeError('STOP: old archive drift')
            objects = list(map(Path, archived[3:]))
            members = command([ar, 't', archive], role + '-archive-members').splitlines()
            if members != [p.name for p in objects]:
                raise RuntimeError('STOP: old archive membership drift')
            selected = []
            for obj in objects:
                if pin(obj) != {k: old['artifacts'][str(obj)][k] for k in ('bytes', 'sha256')}:
                    raise RuntimeError('STOP: old object drift')
                keep(obj)
                c = next(c for c in old['commands'] if '-c' in c['argv'] and c['argv'][-1] == str(obj))
                source = Path(c['argv'][c['argv'].index('-c') + 1])
                defines = [v for v in c['argv'] if v.startswith('-D')]
                if defines != ['-D_FORTIFY_SOURCE=2', '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']:
                    raise RuntimeError('STOP: original role/fixture defines mismatch')
                omit = source.stem in excluded
                report.setdefault('object_roles', {})[str(obj)] = {
                    'source': str(source), 'defines': defines, 'excluded': omit}
                if omit:
                    continue
                selected.append(obj.name)
                dep = obj.with_suffix('.d')
                keep(dep)
                for name in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                    p = Path(name)
                    if p.name in {'owned_runtime_v1.h', 'memory_runtime.h', 'memory_reserve.h',
                                  'native_adapter.h', 'handoff.h'}:
                        raise RuntimeError('STOP: reused consumer needs new layout compilation')
                    if pin(p) != old['dependencies'][name]:
                        raise RuntimeError('STOP: reused source/header drift')
                    report['reused_dependencies'][name] = old['dependencies'][name]
            member_dir = out / ('members-' + role)
            command([ar, 'x', archive, *selected], role + '-selected-members', member_dir)
            reused = [member_dir / n for n in selected]
            for p in reused:
                if pin(p) != pin(OLD / 'objects' / p.name):
                    raise RuntimeError('STOP: selected archive body drift')
            command([elf, '-h', '-A', *reused], role + '-member-roles')
            flags = ['-O2'] if role == 'host' else ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft']
            fresh = []
            for source in sources:
                obj = out / 'objects' / (role + '-' + source.stem + '.o')
                command([cc, *common, *flags, '-MD', '-MF', obj.with_suffix('.d'),
                         '-c', source, '-o', obj], role + '-compile-' + source.stem)
                fresh.append(obj)
            reloc = out / (role + '-span-runtime-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', *fresh, *reused, '-Wl,--gc-sections',
                     *('-Wl,--undefined=' + n for n in roots),
                     '-Wl,-Map=' + str(out / (role + '.map')), '-o', reloc], role + '-REL')
            defined = command([nm, '-g', '--defined-only', reloc], role + '-defined')
            undefined = command([nm, '-u', reloc], role + '-undefined')
            all_symbols = command([nm, '-a', reloc], role + '-all-symbols')
            command([elf, '-h', '-A', '-S', reloc], role + '-ELF')
            relocations = command([elf, '-r', '-W', reloc], role + '-relocations')
            required = (*roots, 'nh_memory_prefault_span_hub', 'nh_memory_span_hub_forecast',
                        'nh_memory_credit_span_hub', 'nh_memory_read_fixed', 'camera_rtp_h264_spans_packet_v1')
            if any(defined.split().count(n) != 1 for n in required):
                raise RuntimeError('STOP: missing/duplicate real API root')
            if any(n in defined.split() for n in ('main', 'camera_rtsp_prestarted_owned_listener_serve_v1',
                    'camera_rtsp_prestarted_span_listener_serve_v1', 'camera_hub_publish')):
                raise RuntimeError('STOP: server/old hub/executable selection')
            private = {n: sum(line.split()[-1:] == [n] for line in all_symbols.splitlines())
                       for n in ('native', 'memory_state', 'runtime')}
            report.setdefault('private_owner_symbols', {})[role] = private
            if any(v != 1 for v in private.values()):
                raise RuntimeError('STOP: actual private singleton shape mismatch')
            unresolved = [line.split()[-1] for line in undefined.splitlines() if line.strip()]
            referenced = [n for n in unresolved if any(n in line.split() for line in relocations.splitlines())]
            report.setdefault('raw_undefined', {})[role] = unresolved
            report.setdefault('relocation_referenced_undefined', {})[role] = referenced
            if any(n.startswith(('camera_', 'candidate_', 'qc07f_', 'qc_', 'qw_', 'qp_',
                                 'nh_', 'rv_', 'is_rv_', 'mbedtls_')) for n in referenced):
                raise RuntimeError('STOP: actual project relocation unresolved; partial link ONLY')
        for dep in sorted((out / 'objects').glob('*.d')):
            for name in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][name] = pin(Path(name))
                if name.startswith(str(ORIGINAL / 'native') + '/') and Path(name).name in {
                        'native_adapter.h', 'handoff.h', 'memory_runtime.h', 'memory_reserve.h'}:
                    raise RuntimeError('STOP: original memory layout header consumed')
        if any(pin(Path(p)) != value for p, value in report['inputs'].items()):
            raise RuntimeError('STOP: source/artifact input changed during build')
        if any(pin(Path(p).resolve()) != {k: value[k] for k in ('bytes', 'sha256')}
               for p, value in report['tools'].items()):
            raise RuntimeError('STOP: tools changed during build')
        report.update(status='HOST_ARM_12_COMPILES_2_ROOTED_REL_PASS_NOT_RUN',
                      C_compiles=12, rooted_REL_links=2, inputs_unchanged=True)
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
