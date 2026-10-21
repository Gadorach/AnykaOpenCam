#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh focused v2 tests/objects using immutable v1 generated headers.

Run with the v2 directory containing v1-preservation.json. No source-tree copy,
Kconfig, Kbuild/header preparation, DT build, full kernel or installation.
Reuse audited saved compiler argv, redirect only source/include/output/depfile.
"""
import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OWN = ROOT / 'bringup/qc07f/mmc-candidate'
EVIDENCE = ROOT / 'evidence/delegated/20261012-mmc-candidate'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    v2 = Path(sys.argv[1]).resolve()
    assert v2.parent == EVIDENCE
    preservation = json.loads((v2 / 'v1-preservation.json').read_text())
    v1 = ROOT / preservation['source_build']
    seal = ROOT / preservation['source_seal']
    build = v1 / 'kbuild'
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    out = v2 / ('focused-' + stamp)
    out.mkdir()
    logs = out / 'logs'
    logs.mkdir()
    inputs = out / 'input'
    inputs.mkdir()
    tmp = out / 'tmp'
    tmp.mkdir()
    env = dict(os.environ, TMPDIR=str(tmp), LC_ALL='C')
    report = {'started_utc': utc(), 'entry_argv': [sys.executable, *sys.argv],
              'cwd': str(Path.cwd()), 'commands': [], 'status': 'INCOMPLETE',
              'v1_source_build': str(v1), 'v1_source_seal': str(seal),
              'target_operations': False, 'native_provider': False,
              'full_source_copy': False, 'Kbuild_or_DT_repeated': False,
              'runtime_linkage_proved': False}

    def run(name, argv, cwd=ROOT, timeout=120):
        rec = {'name': name, 'argv': list(map(str, argv)), 'cwd': str(cwd),
               'started_utc': utc(), 'timeout_s': timeout}
        with (logs / (name + '.log')).open('xb') as f:
            rec['exit'] = subprocess.run(rec['argv'], cwd=cwd, env=env, stdout=f,
                                      stderr=subprocess.STDOUT, timeout=timeout).returncode
        rec['ended_utc'] = utc()
        rec['log_sha256'] = sha(logs / (name + '.log'))
        report['commands'].append(rec)
        assert rec['exit'] == 0, name

    try:
        preserved = v2 / 'v1-preserved'
        for rel, digest in preservation['snapshot_sha256'].items():
            assert sha(preserved / rel) == digest, rel
        old = json.loads((v1 / 'REPORT.json').read_text())
        for tool in ['gcc', 'clang', 'ld.lld', 'llvm-readelf']:
            assert sha(Path(shutil.which(tool))) == old['tool_sha256'][tool], tool
            run('version-' + tool, [tool, '--version'])
        headers = json.loads((seal / 'arm-header-closure.json').read_text())
        assert all(sha(Path(p)) == digest for p, digest in headers.items())
        immutable_files = [v1 / 'REPORT.json', seal / 'REPORT.json', build / '.config']
        object_dir = build / 'drivers/mmc/host/qc07f-candidate'
        immutable_files.extend(p for p in object_dir.iterdir() if p.is_file())
        immutable = {str(p): sha(p) for p in immutable_files}
        report['v1_immutable_inputs_sha256'] = immutable
        report['preserved_header_count'] = len(headers)
        names = ['host-core.c', 'host-core.h', 'qc07f-mci.c', 'test-host-core.c']
        diffs = []
        for name in names:
            shutil.copyfile(OWN / name, inputs / name)
            before = (preserved / 'source' / name).read_text()
            after = (inputs / name).read_text()
            diffs.append(''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                         fromfile='v1/' + name, tofile='v2/' + name)))
        shutil.copyfile(Path(__file__), inputs / Path(__file__).name)
        (out / 'v1-to-v2.patch').write_text(''.join(diffs))
        for name in ['Kconfig', 'Makefile', 'candidate.dtsi', 'filesystems-candidate.config']:
            assert sha(OWN / name) == sha(preserved / 'source' / name)
        oldglue = (preserved / 'source/qc07f-mci.c').read_text()
        newglue = (inputs / 'qc07f-mci.c').read_text()
        assert oldglue.split('static int qc_probe',1)[1] == newglue.split('static int qc_probe',1)[1]
        for kind, flags in [('optimized', ['-O2']), ('sanitized', ['-O1', '-g',
                            '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
            binary = out / ('test-' + kind)
            run('compile-' + kind, ['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror', *flags,
                inputs / 'host-core.c', inputs / 'test-host-core.c', '-o', binary])
            run('test-' + kind, [binary])  # Newly built host fixture only.
        closure = {}
        for name in ['host-core', 'qc07f-mci']:
            saved = object_dir / ('.' + name + '.o.cmd')
            line = saved.read_text().splitlines()[0].split(' := ', 1)[1]
            argv = shlex.split(line)
            assert argv[0] == 'clang' and '-Werror' in argv and '-march=armv5te' in argv
            assert not any(';' in a or '\n' in a for a in argv)
            # Translate only write destinations, source, and driver-local include
            # paths. Generated/common/architecture headers remain v1 read-only.
            for i, token in enumerate(argv):
                if token.startswith('-Wp,-MMD,'):
                    argv[i] = '-Wp,-MMD,' + str(out / (name + '.d'))
                elif token == '-Idrivers/mmc/host/qc07f-candidate' or (
                     token.startswith('-I') and token.endswith('/drivers/mmc/host/qc07f-candidate')):
                    argv[i] = '-I' + str(inputs)
                elif i > 0 and argv[i-1] == '-o':
                    argv[i] = str(out / (name + '.arm.o'))
                elif token.endswith('/qc07f-candidate/' + name + '.c'):
                    argv[i] = str(inputs / (name + '.c'))
            assert argv[-1] == str(inputs / (name + '.c'))
            report.setdefault('saved_compiler_commands_sha256', {})[str(saved)] = sha(saved)
            run('arm-' + name, argv, cwd=build)
            dep = (out / (name + '.d')).read_text().replace('\\\n', ' ')
            for token in dep.split(':', 1)[1].split():
                p = Path(token)
                if not p.is_absolute(): p = build / p
                assert p.is_file(), token
                closure[str(p)] = sha(p)
        linked = out / 'qc07f-mci-candidate-v2.arm-relocatable.o'
        run('arm-relocatable-link', ['ld.lld', '-r', out / 'qc07f-mci.arm.o',
                                   out / 'host-core.arm.o', '-o', linked])
        run('arm-elf', ['llvm-readelf', '-h', '-A', '-s', linked])
        run('arm-undefined', ['llvm-nm', '--undefined-only', linked])
        assert 'qc_request_ready' in (logs / 'arm-elf.log').read_text()
        (out / 'actual-arm-input-closure.json').write_text(json.dumps(closure, indent=2) + '\n')
        assert all(sha(Path(p)) == digest for p, digest in headers.items())
        assert all(sha(Path(p)) == digest for p, digest in immutable.items())
        assert all(sha(preserved / rel) == digest
                   for rel, digest in preservation['snapshot_sha256'].items())
        report['actual_arm_dependency_count'] = len(closure)
        report['v1_headers_objects_logs_config_preserved'] = True
        report['DT_Kconfig_probe_resource_constants_unchanged'] = True
        report['status'] = 'V2_HOST_TESTS_PASS_ARM_SOURCE_OVERRIDE_OBJECTS_LINK_PASS_NOT_NATIVE'
    finally:
        report['ended_utc'] = utc()
        report['input_sha256'] = {str(p.relative_to(inputs)): sha(p) for p in inputs.iterdir() if p.is_file()}
        report['output_sha256'] = {str(p.relative_to(out)): sha(p) for p in out.iterdir() if p.is_file()}
        (out / 'REPORT.json').write_text(json.dumps(report, indent=2) + '\n')
        print(out)


if __name__ == '__main__':
    main()
