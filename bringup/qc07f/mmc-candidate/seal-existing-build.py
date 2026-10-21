#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Resume ONLY pending link/DT/closure sealing; never repeat fixture/Kbuild.

Usage: python3 bringup/qc07f/mmc-candidate/seal-existing-build.py <build-dir>
The input REPORT and all earlier evidence remain immutable.
"""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
EVIDENCE = ROOT / 'evidence/delegated/20261012-mmc-candidate'
FOUND = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
ORIGINAL = FOUND / 'artifacts/native-security75-source01/linux'
CONFIG = FOUND / 'analysis/native-security75-build01/build/.config'


def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda: f.read(1024*1024), b''):
            h.update(b)
    return h.hexdigest()


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    base = Path(sys.argv[1]).resolve()
    assert base.parent == EVIDENCE
    before = json.loads((base / 'REPORT.json').read_text())
    assert all(r['exit'] == 0 for r in before['commands'])
    assert before['commands'][-1]['name'] == 'arm-kbuild-objects'
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    out = base / ('seal-' + stamp)
    out.mkdir()
    src = base / 'linux-scratch'
    build = base / 'kbuild'
    candidate = base / 'candidate-input'
    report = {'started_utc': utc(), 'input_report_sha256': sha(base / 'REPORT.json'),
              'seal_script_sha256': sha(Path(__file__)),
              'entry_argv': [sys.executable, *sys.argv], 'cwd': str(Path.cwd()),
              'commands': [], 'status': 'INCOMPLETE', 'native_mmc': False,
              'target_operations': False, 'image_built': False,
              'host_provider': 'ABSENT: PROBE UNCONDITIONALLY REFUSES'}

    def run(name, argv):
        rec = {'name': name, 'argv': list(map(str, argv)), 'cwd': str(ROOT),
               'started_utc': utc(), 'timeout_s': 60}
        with (out / (name + '.log')).open('xb') as f:
            rec['exit'] = subprocess.run(rec['argv'], cwd=ROOT, stdout=f,
                       stderr=subprocess.STDOUT, timeout=60).returncode
        rec['ended_utc'] = utc()
        report['commands'].append(rec)
        assert rec['exit'] == 0, name

    try:
        for rel, digest in before['candidate_input_hashes'].items():
            assert sha(candidate / rel) == digest
            assert sha(ROOT / 'bringup/qc07f/mmc-candidate' / rel) == digest
        linked = out / 'qc07f-mci-candidate.arm-relocatable.o'
        objects = [build / 'drivers/mmc/host/qc07f-candidate' / name
                   for name in ['qc07f-mci.o', 'host-core.o']]
        report['input_object_hashes'] = {str(p): sha(p) for p in objects}
        run('arm-relocatable-link', ['ld.lld', '-r', *objects, '-o', linked])
        run('arm-elf', ['llvm-readelf', '-h', '-A', '-s', linked])
        elf = (out / 'arm-elf.log').read_text()
        assert 'ARM' in elf and 'qc_work' in elf and 'qc_run' in elf
        wrapper = out / 'candidate-wrapper.dts'
        wrapper.write_text('/dts-v1/;\n/include/ "' + str(candidate / 'candidate.dtsi') + '"\n')
        run('disabled-dt', ['dtc', '-I', 'dts', '-O', 'dtb', '-o', out / 'candidate-disabled.dtb', wrapper])
        assert not (out / 'disabled-dt.log').read_text().strip()
        run('disabled-dt-roundtrip', ['dtc', '-I', 'dtb', '-O', 'dts', out / 'candidate-disabled.dtb'])
        assert 'status = "disabled"' in (out / 'disabled-dt-roundtrip.log').read_text()
        headers = {}
        for p in sorted((build / 'drivers/mmc/host/qc07f-candidate').glob('.*.cmd')):
            for token in p.read_text().replace('\\\n', ' ').split():
                if token.endswith('.h'):
                    q = Path(token)
                    if not q.is_absolute(): q = build / q
                    if q.is_file(): headers[str(q)] = sha(q)
        assert len(headers) > 500
        (out / 'arm-header-closure.json').write_text(json.dumps(headers, indent=2) + '\n')
        original = CONFIG.read_text()
        final = (build / '.config').read_text()
        assert sha(CONFIG) == '22acc5e770af2d6d4804ca8f64cd7e9ffe71f69f2e5c8a0fa455a23dc9217bf8'
        for option in ['MMC_BLOCK', 'EXT4_FS', 'PWRSEQ_SIMPLE', 'PWRSEQ_EMMC']:
            assert '# CONFIG_' + option + ' is not set' in final
        assert 'CONFIG_CPU_ARM926T=y' in final and 'CONFIG_MMC_QC07F_CANDIDATE=y' in final
        (out / 'config75-to-object-candidate.patch').write_text(''.join(difflib.unified_diff(
            original.splitlines(True), final.splitlines(True), fromfile='kernel75.config',
            tofile='mmc-object-only.config')))
        inventory = json.loads((base / 'copied-linux-inputs.json').read_text())
        assert all(sha(ORIGINAL / rel) == digest for rel, digest in inventory.items())
        changes = {rel: sha(src / rel) for rel, digest in inventory.items() if sha(src / rel) != digest}
        assert set(changes) == {'drivers/mmc/host/Kconfig', 'drivers/mmc/host/Makefile'}
        report['copied_source_changes'] = changes
        report['original_source_preserved_count'] = len(inventory)
        report['header_count'] = len(headers)
        report['config_sha256'] = sha(build / '.config')
        report['status'] = 'FIXTURE_PASS_3190_ARM_KBUILD_WERROR_AND_RELOCATABLE_LINK_ONLY_PROBE_REFUSES'
    finally:
        report['ended_utc'] = utc()
        report['artifacts_sha256'] = {str(p.relative_to(out)): sha(p) for p in out.iterdir() if p.is_file()}
        (out / 'REPORT.json').write_text(json.dumps(report, indent=2) + '\n')
        print(out)


if __name__ == '__main__':
    main()
