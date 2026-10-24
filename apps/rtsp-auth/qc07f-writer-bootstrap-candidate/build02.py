#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""ONE fixed offline bootstrap compile/rooted REL/metadata; NEVER candidate calls."""
from pathlib import Path
import datetime
import difflib
import hashlib
import json
import re
import shlex
import subprocess

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]
DOC = ROOT / 'docs/qualification-plans/qc07f-writer-bootstrap-20261024'
AUTH = APP.parent / 'qc07f-owned-runtime'
WRITER = ROOT / 'docs/qualification-plans/qc07f-writer-serve-20261024/runs/build01'
OLD = ROOT / 'docs/qualification-plans/rtsp-owned-runtime-20261023/runs/build02'
PP = ROOT / 'docs/qualification-plans/rtsp-provisioner-process-20261023/runs/build01'
RNG = ROOT / 'docs/qualification-plans/rtsp-os-random-20261023/runs/build01'
BASE = APP.parent / 'qc07f-bootstrap-material-retire'


def pin(p):
    b = Path(p).read_bytes()
    return {'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()}


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    out = DOC / 'runs/build02'
    out.mkdir(parents=True, exist_ok=False)
    for n in ('logs', 'objects', 'source', 'tmp'):
        (out / n).mkdir()
    env = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TMPDIR': str(out / 'tmp'),
           'SOURCE_DATE_EPOCH': '1791763200', 'PYTHONDONTWRITEBYTECODE': '1'}
    report = {'started': utc(), 'status': 'BUILDING', 'commands': [], 'inputs': {},
              'tools': {}, 'reused_dependencies': {}, 'dependencies': {}, 'object_roles': {},
              'environment': env, 'candidate_calls': 0, 'executable': False}
    retained = {}

    def keep(p, expected=None):
        p = Path(p);actual = pin(p)
        if expected is not None and actual != {k: expected[k] for k in ('bytes', 'sha256')}:
            raise RuntimeError('STOP consumed pin drift: ' + str(p))
        report['inputs'][str(p)] = actual

    def tool(p):
        resolved = str(Path(p).resolve())
        if resolved not in retained:
            raise RuntimeError('STOP unretained offline tool: ' + str(p))
        keep(resolved, retained[resolved])
        report['tools'][str(p)] = {'resolved': resolved, **pin(resolved)}

    def command(argv, label):
        argv = list(map(str, argv));tool(argv[0])
        log = out / 'logs' / (label + '.txt')
        c = {'argv': argv, 'cwd': str(ROOT), 'started': utc(), 'log': str(log)}
        report['commands'].append(c)
        with log.open('xb') as f:
            result = subprocess.run(argv, cwd=ROOT, env=env, stdout=f,
                                    stderr=subprocess.STDOUT, timeout=120)
        c.update(exit=result.returncode, ended=utc(), log_pin=pin(log))
        if result.returncode:
            raise RuntimeError('STOP offline tool failure: ' + label)
        return log.read_text()

    def absolute(n):
        p = Path(n)
        return p if p.is_absolute() else ROOT / p

    def dependencies(obj, expected, dep_pin):
        dep = obj.with_suffix('.d');keep(dep, dep_pin)
        paths = shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1])
        for n in paths:
            p = absolute(n)
            # Exactly consumed-source dependency records, NEVER inputs fallback.
            keep(p, expected[n]);report['reused_dependencies'][str(p)] = pin(p)
        return paths

    def production(c, crypto_config):
        a = c['argv']
        defines = [n for n in a if n.startswith('-D')]
        allowed = ['-D_FORTIFY_SOURCE=2', '-DMBEDTLS_CONFIG_FILE="' + crypto_config + '"']
        sensor = '-c' in a and a[a.index('-c') + 1].endswith('/sensor_baseline.c')
        if sensor:
            allowed += ['-D_POSIX_C_SOURCE=200809L', '-D_DEFAULT_SOURCE']
        if c.get('exit', c.get('returncode')) != 0 or defines != allowed:
            raise RuntimeError('STOP nonproduction/failed consumed object role')
        if isinstance(c['log'], dict):
            keep(absolute(c['log']['path']), c['log'])
        else:
            keep(absolute(c['log']), c['log_pin'])

    def live_snapshot(n):
        p = Path(n)
        if not p.is_relative_to(OLD / 'source'):
            return None
        tail = p.relative_to(OLD / 'source')
        prefix = tail.parts[0]
        directory = {'app': AUTH, 'framework': ROOT / 'framework',
                     'verifier': APP.parent / 'isolated-verifier',
                     'vendor': ROOT / 'apps/auth-web/service/vendor/mbedtls'}[prefix]
        return directory.joinpath(*tail.parts[1:])

    def no_old_layout(paths):
        forbidden = {'owned_runtime_v1.h', 'owned_runtime_v4.h', 'bootstrap_v1.h',
                     'writer_bootstrap_v1.h', 'rtsp_span_server_v1.h', 'memory_runtime.h',
                     'memory_reserve.h', 'native_adapter.h', 'handoff.h', 'span_hub.h'}
        if any(Path(n).name in forbidden for n in paths):
            raise RuntimeError('STOP old/changed layout consumer reuse')

    try:
        wr = json.loads((WRITER / 'REPORT.json').read_text())
        old = json.loads((OLD / 'REPORT.json').read_text())
        pp = json.loads((PP / 'REPORT.json').read_text())
        rng = json.loads((RNG / 'REPORT.json').read_text())
        retained.update({v['resolved']: v for v in wr['tools'].values()})
        tool('/usr/bin/python3')
        frozen_path = DOC / 'FROZEN_INPUTS.json';keep(frozen_path)
        frozen = json.loads(frozen_path.read_text())
        for n, expected in frozen['files'].items():
            keep(ROOT / n, expected)
        for n in frozen['roots']:
            actual = {str(p.relative_to(ROOT)) for p in (ROOT / n).rglob('*') if p.is_file()}
            if actual != {p for p in frozen['files'] if p.startswith(n + '/')}:
                raise RuntimeError('STOP frozen initial membership')
        for directory in (WRITER, OLD, PP, RNG):
            keep(directory / 'REPORT.json')
        if wr['status'] != 'HOST_ARM_16_COMPILES_2_ROOTED_REL_PASS_NOT_RUN':
            raise RuntimeError('STOP wrong accepted writer evidence')
        exact = {BASE / 'bootstrap.c': 'b89782a3522dd16d201a1dc3b24e64b8ce0a5c2e519af3a89e9ad45c7a054b60',
                 BASE / 'bootstrap_v1.h': 'aec5a98dd3d3a7cd867ebf93d224c7e049b44d2c8caff8fcf0fd2795c450f529',
                 APP.parent / 'qc07f-writer-serve-candidate/owned_runtime.c': 'd83ccc8db4129a62a4f6abc74b0924a9732b489c2b2f8e636b22303e00f21551',
                 APP.parent / 'qc07f-writer-serve-candidate/owned_runtime_v4.h': '51922d1769d1bf2b88231b3e9b1a8f1933fc73929353eae0e70a2374284447c3',
                 APP.parent / 'qc07f-writer-serve-candidate/rtsp_span_server.c': 'a5b2854aeaae87275db1dbd01fd6691610365690fe437399d87c031dbf3c86ce'}
        for p, digest in exact.items():
            keep(p)
            if pin(p)['sha256'] != digest:
                raise RuntimeError('STOP exact accepted source')
        for p in sorted(APP.iterdir()):
            if p.is_file():
                keep(p)
                with (out / 'source' / p.name).open('xb') as f:
                    f.write(p.read_bytes())
        keep(DOC / 'CONTRACT.md');keep(DOC / 'AUTHOR_AUDIT.md')
        proof = {'unchanged_regions': {}, 'deltas': {}}
        for a, b in [(BASE / 'bootstrap.c', APP / 'bootstrap.c'),
                     (BASE / 'bootstrap_v1.h', APP / 'writer_bootstrap_v1.h')]:
            x, y = a.read_text(), b.read_text()
            nd = list(difflib.ndiff(x.splitlines(True), y.splitlines(True)))
            if ''.join(difflib.restore(nd, 1)) != x or ''.join(difflib.restore(nd, 2)) != y:
                raise RuntimeError('STOP literal inverse proof')
            for inverse in (False, True):
                u, v = (b, a) if inverse else (a, b)
                with (out / (b.name + ('.inverse.diff' if inverse else '.forward.diff'))).open('x') as f:
                    f.writelines(difflib.unified_diff(u.read_text().splitlines(True), v.read_text().splitlines(True), str(u), str(v)))
            proof['deltas'][b.name] = [{'tag': tag, 'baseline_0based': [i, j], 'candidate_0based': [k, l]}
                for tag, i, j, k, l in difflib.SequenceMatcher(None, x.splitlines(), y.splitlines(), autojunk=False).get_opcodes() if tag != 'equal']
        x = (BASE / 'bootstrap.c').read_text();y = (APP / 'bootstrap.c').read_text()
        for start, end in [('static int same_inode(', 'int qc07f_bootstrap_begin_v1('),
                           ('int qc07f_bootstrap_begin_v1(', 'int qc07f_bootstrap_start_v1('),
                           (' if(!material_check(b))return leave(hold(b,QC07F_BS_E_MATERIAL_PREFLIGHT));', None)]:
            a, b = x[x.index(start):], y[y.index(start):]
            if end is not None:
                a, b = a[:a.index(end)], b[:b.index(end)]
            if a != b:
                raise RuntimeError('STOP required unchanged region: ' + start)
            proof['unchanged_regions'][start] = {'bytes': len(a.encode()), 'sha256': hashlib.sha256(a.encode()).hexdigest()}
        with (out / 'SOURCE_PROOF.json').open('x') as f:
            json.dump(proof, f, indent=2);f.write('\n')
        ppdep_path = PP / 'DEPENDENCIES.json'
        keep(ppdep_path, pp['dependencies_manifest'])
        ppdeps = json.loads(ppdep_path.read_text())
        rngdep_path = RNG / 'DEPENDENCIES.json'
        rngart = {str(absolute(v['path'])): v for v in rng['artifacts']}
        keep(rngdep_path, rngart[str(rngdep_path)])
        rngdeps = json.loads(rngdep_path.read_text())
        ppart = {str(absolute(v['path'])): v for v in pp['artifacts']}
        roots = wr['roots'] + re.findall(r'^int (qc07f_bootstrap_\w+)\(', (APP / 'writer_bootstrap_v1.h').read_text(), re.M)
        roots += ['qc07f_rtsp_provisioner_begin_v1', 'qc07f_rtsp_provisioner_observe_v1', 'qc07f_rtsp_provisioner_bind_v1',
                  'qc07f_rtsp_private_handoff_create_v1', 'qc07f_rtsp_private_handoff_move_v1',
                  'qc07f_rtsp_private_provisioner_harden_v1', 'qc07f_rtsp_startup_material_build_v1',
                  'qc07f_rtsp_startup_material_clear_v1', 'qc07f_rtsp_os_random_ready_v1']
        roots = list(dict.fromkeys(roots));report['roots'] = roots
        common = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-fno-common', '-fno-strict-aliasing',
                  '-ffunction-sections', '-fdata-sections', '-fstack-protector-strong', '-D_FORTIFY_SOURCE=2',
                  '-fstack-usage', '-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']
        common += ['-I' + str(p) for p in (APP, AUTH / 'server', AUTH / 'native', ROOT / 'framework/include', APP.parent / 'isolated-verifier')]
        for role in ('host', 'arm'):
            ts = old['tools'][role]
            cc, nm, elf = [ts[k]['path'] for k in ('cc', 'nm', 'readelf')]
            for v in ts.values():
                tool(v['path']);keep(Path(v['path']).resolve(), v)
            if role == 'arm':
                tool(cc + '.br_real')
                for n, digest in old['toolchain_pins'].items():
                    p = Path(cc).parent.parent / n;keep(p)
                    if pin(p)['sha256'] != digest:
                        raise RuntimeError('STOP retained compiler support drift')
            command([cc, '--version'], role + '-version')
            command([cc, '-dumpmachine'], role + '-machine')
            for n in ('cc1', 'as', 'ld'):
                p = command([cc, '-print-prog-name=' + n], role + '-tool-' + n).strip()
                tool(p if '/' in p else '/usr/bin/' + p)
            # Exact 24 INDIVIDUAL bodies from accepted API5 link, not its REL.
            link = next(c for c in wr['commands'] if '-r' in c['argv'] and c['argv'][-1] == str(WRITER / (role + '-WRITER-SERVE-NOT-EXECUTABLE.o')))
            reused = [Path(n) for n in link['argv'][link['argv'].index('-r') + 1:] if n.endswith('.o') and str(WRITER) in n]
            if len(reused) != 25 or reused[-1].name != role + '-WRITER-SERVE-NOT-EXECUTABLE.o':
                raise RuntimeError('STOP exact writer individual membership')
            reused = reused[:-1]
            for obj in reused:
                keep(obj, wr['artifacts'][str(obj)])
                if obj.parent == WRITER / 'objects':
                    c = next(c for c in wr['commands'] if '-c' in c['argv'] and c['argv'][-1] == str(obj))
                    production(c, 'server_crypto_config.h')
                    paths = dependencies(obj, wr['dependencies'], wr['artifacts'][str(obj.with_suffix('.d'))])
                    source = c['argv'][c['argv'].index('-c') + 1]
                else:
                    original = OLD / 'objects' / obj.name
                    keep(original, old['artifacts'][str(original)])
                    if pin(obj) != pin(original):
                        raise RuntimeError('STOP extracted body mismatch')
                    c = next(c for c in old['commands'] if '-c' in c['argv'] and c['argv'][-1] == str(original))
                    production(c, 'server_crypto_config.h')
                    paths = dependencies(original, old['dependencies'], wr['inputs'][str(original.with_suffix('.d'))])
                    no_old_layout(paths);source = c['argv'][c['argv'].index('-c') + 1]
                    for n in paths:
                        live = live_snapshot(n)
                        if live is not None:
                            keep(live, old['dependencies'][n])
                report['object_roles'][str(obj)] = {'source': source, 'compile': c, 'class': 'ACCEPTED_API5_PRODUCTION_INDIVIDUAL'}
            for i in (0, 1, 2, 3, 9):
                obj = PP / 'objects' / (role + '-' + str(i) + '.o')
                keep(obj, ppart[str(obj)])
                c = next(c for c in pp['commands'] if '-c' in c['argv'] and c['argv'][-1] == str(obj))
                production(c, 'rtsp_crypto_config.h')
                dp = str(obj.with_suffix('.d').relative_to(ROOT))
                expected = {str(absolute(v['path'])): v for v in ppdeps if v['dependency_file'] == dp}
                paths = dependencies(obj, expected, ppart[str(obj.with_suffix('.d'))]);no_old_layout(paths)
                report['object_roles'][str(obj)] = {'source': c['argv'][c['argv'].index('-c') + 1], 'compile': c, 'class': 'REAL_PROVISIONER_PRODUCTION'}
                reused.append(obj)
            obj = RNG / 'objects' / (role + '-random.o');keep(obj, rngart[str(obj)])
            c = next(c for c in rng['commands'] if '-c' in c['argv'] and c['argv'][-1] == str(obj))
            production(c, 'rtsp_crypto_config.h')
            expected = {v['path']: v for group in rngdeps if group['unit'] == role + '-random.d' for v in group['files']}
            paths = dependencies(obj, expected, rngart[str(obj.with_suffix('.d'))]);no_old_layout(paths)
            report['object_roles'][str(obj)] = {'source': c['argv'][c['argv'].index('-c') + 1], 'compile': c, 'class': 'REAL_OS_RANDOM_PRODUCTION'}
            reused.append(obj)
            command([elf, '-h', '-A', *reused], role + '-reused-ELF')
            command([nm, '-g', '--defined-only', *reused], role + '-reused-symbols')
            flags = ['-O2'] if role == 'host' else ['-Os', '-march=armv5te', '-marm', '-mfloat-abi=soft']
            obj = out / 'objects' / (role + '-bootstrap.o')
            command([cc, *common, *flags, '-MD', '-MF', obj.with_suffix('.d'), '-c', APP / 'bootstrap.c', '-o', obj], role + '-compile-bootstrap')
            reloc = out / (role + '-WRITER-BOOTSTRAP-NOT-EXECUTABLE.o')
            command([cc, *flags, '-nostdlib', '-r', obj, *reused, '-Wl,--gc-sections',
                     *('-Wl,--undefined=' + n for n in roots), '-Wl,-Map=' + str(out / (role + '.map')), '-o', reloc], role + '-REL')
            defined = command([nm, '-g', '--defined-only', reloc], role + '-defined')
            command([nm, '-u', reloc], role + '-undefined')
            symbols = command([nm, '-a', reloc], role + '-all-symbols')
            command([elf, '-h', '-A', '-S', reloc], role + '-ELF')
            table = command([elf, '-s', '-W', reloc], role + '-symbol-table')
            rels = command([elf, '-r', '-W', reloc], role + '-relocations')
            if any(defined.split().count(n) != 1 for n in roots):
                raise RuntimeError('STOP real public root count')
            forbidden = ('main', '_start', 'qc07f_owned_runtime_prepare_v1', 'qc07f_owned_runtime_span_serve_v1',
                         'camera_rtsp_prestarted_owned_listener_serve_v1', 'camera_rtsp_prestarted_span_listener_serve_v1')
            if any(n in defined.split() for n in forbidden):
                raise RuntimeError('STOP old frontend/runtime/main')
            private = {n: sum(line.split()[-1:] == [n] for line in symbols.splitlines()) for n in ('native', 'memory_state', 'runtime')}
            report.setdefault('private_owner_symbols', {})[role] = private
            if any(v != 1 for v in private.values()):
                raise RuntimeError('STOP private owner duplication')
            # Exact symbol INDEX, not name matching (LOCAL UND and GLOBAL defined
            # Digest names can coexist in GNU rooted partial REL symbol tables).
            rows = {int(p[0][:-1]): p for line in table.splitlines()
                    if len(p := line.split()) >= 7 and p[0].endswith(':') and p[0][:-1].isdigit()}
            references = {}
            for line in rels.splitlines():
                p = line.split()
                if len(p) >= 3 and p[2].startswith('R_'):
                    index = int(p[1], 16) >> (32 if role == 'host' else 8)
                    references[index] = references.get(index, 0) + 1
            unresolved = [{'index': i, 'row': p, 'retained_references': references.get(i, 0)}
                          for i, p in rows.items() if p[6] == 'UND' and len(p) >= 8]
            report.setdefault('raw_undefined_symbol_records', {})[role] = unresolved
            for v in unresolved:
                if v['retained_references'] and v['row'][7].startswith(('camera_', 'candidate_', 'qc07f_', 'qc_', 'qw_', 'qp_', 'nh_', 'rv_', 'is_rv_', 'mbedtls_')):
                    raise RuntimeError('STOP unresolved actual project relocation')
        for dep in (out / 'objects').glob('*.d'):
            for n in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][n] = pin(absolute(n))
        for n in frozen['roots']:
            actual = {str(p.relative_to(ROOT)) for p in (ROOT / n).rglob('*') if p.is_file()}
            if actual != {p for p in frozen['files'] if p.startswith(n + '/')}:
                raise RuntimeError('STOP frozen final membership')
        if any(pin(n) != expected for n, expected in report['inputs'].items()):
            raise RuntimeError('STOP final consumed pin drift')
        report.update(status='HOST_ARM_2_COMPILES_2_ROOTED_REL_PASS_NOT_RUN', C_compiles=2,
                      rooted_REL_links=2, reused_individual_objects_per_target=30, frozen_unchanged=True)
    except BaseException as exc:
        report.update(status='FAILED_PRESERVED', error=type(exc).__name__ + ': ' + str(exc))
        raise
    finally:
        for dep in (out / 'objects').glob('*.d'):
            for n in shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1]):
                report['dependencies'][n] = pin(absolute(n))
        report['ended'] = utc()
        report['artifacts'] = {str(p): pin(p) for p in sorted(out.rglob('*')) if p.is_file()}
        with (out / 'REPORT.json').open('x') as f:
            json.dump(report, f, indent=2);f.write('\n')
        print(out / 'REPORT.json')


if __name__ == '__main__':
    main()
