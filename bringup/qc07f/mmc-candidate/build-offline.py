#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh isolated fixture/Kbuild object proof; no image, module, install or IO.

Run from the workspace root. Every created path stays inside the two delegated
directories. Never invokes a vendor SDK/auto_build script or target binary.
"""
import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OWN = ROOT / 'bringup/qc07f/mmc-candidate'
EVIDENCE = ROOT / 'evidence/delegated/20261012-mmc-candidate'
FOUND = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
LINUX = FOUND / 'artifacts/native-security75-source01/linux'
CONFIG = FOUND / 'analysis/native-security75-build01/build/.config'
VENDOR_ROOT = ROOT / 'work/lightbulb-camera/deep-bringup-20261008T055753Z/sources/ak3918ev200'
VENDOR = VENDOR_ROOT / 'vendor/linux-3.4.35'
PIN = '725bd2f3c81d54edccb66fead01d4c0c222e2231'
CONFIG_PIN = '22acc5e770af2d6d4804ca8f64cd7e9ffe71f69f2e5c8a0fa455a23dc9217bf8'


def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda: f.read(1024*1024), b''):
            h.update(b)
    return h.hexdigest()


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    out = EVIDENCE / ('build-' + stamp)
    out.mkdir(parents=True, exist_ok=False)
    logs = out / 'logs'
    logs.mkdir()
    tmp = out / 'tmp'
    tmp.mkdir()
    env = dict(os.environ, TMPDIR=str(tmp), LC_ALL='C', KBUILD_BUILD_USER='mmc-offline',
               KBUILD_BUILD_HOST='isolated', KBUILD_BUILD_TIMESTAMP='2026-10-12T00:00:00Z')
    records = []
    report = {'started_utc': utc(), 'entry_argv': [sys.executable, *sys.argv],
              'cwd': str(Path.cwd()), 'commands': records,
              'status': 'INCOMPLETE', 'target_operations': False,
              'host_provider': 'ABSENT: PROBE UNCONDITIONALLY REFUSES',
              'native_mmc': False, 'image_built': False, 'product_gates': False}

    def run(name, argv, timeout=300):
        rec = {'name': name, 'argv': list(map(str, argv)), 'cwd': str(ROOT),
               'started_utc': utc(), 'timeout_s': timeout}
        print('START', name, flush=True)
        with (logs / (name + '.log')).open('xb') as f:
            try:
                rec['exit'] = subprocess.run(rec['argv'], cwd=ROOT, env=env,
                    stdout=f, stderr=subprocess.STDOUT, timeout=timeout).returncode
            except subprocess.TimeoutExpired:
                rec['exit'] = 'TIMEOUT'
        rec['ended_utc'] = utc()
        rec['log_sha256'] = sha(logs / (name + '.log'))
        records.append(rec)
        print('END', name, rec['exit'], flush=True)
        assert rec['exit'] == 0, name

    try:
        assert sha(CONFIG) == CONFIG_PIN
        run('linux-pin', ['git', '-C', LINUX, 'rev-parse', 'HEAD'])
        assert (logs / 'linux-pin.log').read_text().strip() == PIN
        run('vendor-pin', ['git', '-C', VENDOR_ROOT, 'rev-parse', 'HEAD'])
        report['linux_pin'] = PIN
        report['vendor_container_pin'] = (logs / 'vendor-pin.log').read_text().strip()
        upstream = out / 'upstream'
        upstream.mkdir()
        vendor_inputs = ['drivers/mmc/host/akmci.c', 'include/plat-anyka/akmci.h',
                         'arch/arm/plat-anyka/l2.c',
                         'arch/arm/plat-anyka/include/plat/l2.h',
                         'arch/arm/mach-ak39/devices.c', 'arch/arm/mach-ak39/reset.c',
                         'arch/arm/mach-ak39/clock.c', 'arch/arm/mach-ak39/ak39-gpio.c',
                         'arch/arm/mach-ak39/mach-aimer39_ak3918.c',
                         'arch/arm/mach-ak39/include/mach/clock.h',
                         'arch/arm/mach-ak39/include/mach/irqs.h',
                         'arch/arm/mach-ak39/include/mach/map.h', 'COPYING']
        report['source_inputs'] = {}
        for rel in vendor_inputs:
            src = VENDOR / rel
            dest = upstream / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, dest)
            report['source_inputs'][str(src.relative_to(ROOT))] = sha(src)
        contracts = ROOT / 'work/lightbulb-camera/driver-re-20261008T110053Z/contracts/pseudocode'
        for rel in ['akmci-ko/001-akmci_l2xfer.c.txt', 'akmci-ko/006-akmci_start_data.c.txt',
                    'akmci-ko/009-akmci_irq.c.txt', 'akmci-ko/013-akmci_probe.c.txt',
                    'akmci-ko-remaining/003-akmci_get_ro.c.txt']:
            report['source_inputs'][str((contracts / rel).relative_to(ROOT))] = sha(contracts / rel)
        for name in ['uart.c', 'irq.c', 'usb.c', 'io.h']:
            src = ROOT / 'bringup/qc07f/variants/security75/mach-ak3918ev200' / name
            report['source_inputs'][str(src.relative_to(ROOT))] = sha(src)
        snapshot = out / 'candidate-input'
        shutil.copytree(OWN, snapshot, ignore=shutil.ignore_patterns('__pycache__'))
        report['candidate_input_hashes'] = {str(p.relative_to(snapshot)): sha(p)
                                          for p in sorted(snapshot.rglob('*')) if p.is_file()}
        for tool in ['python3', 'gcc', 'clang', 'ld.lld', 'make', 'llvm-readelf', 'dtc']:
            run('version-' + tool, [tool, '--version'])
        report['tool_sha256'] = {tool: sha(Path(shutil.which(tool)))
                                for tool in ['python3', 'gcc', 'clang', 'ld.lld', 'make',
                                             'llvm-readelf', 'dtc']}
        for kind, extra in [('optimized', ['-O2']),
                            ('sanitized', ['-O1', '-g', '-fsanitize=address,undefined',
                                           '-fno-omit-frame-pointer'])]:
            binary = out / ('test-' + kind)
            run('compile-' + kind, ['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                 *extra, snapshot / 'host-core.c', snapshot / 'test-host-core.c', '-o', binary])
            run('test-' + kind, [binary])  # Newly built host fixture, NOT firmware.
        src = out / 'linux-scratch'
        # Preserve audited upstream relative INTERNAL links. Reject escape or
        # absolute links; never compile through links into another checkout.
        links = {str(p.relative_to(LINUX)): os.readlink(p) for p in LINUX.rglob('*')
                 if '.git' not in p.parts and p.is_symlink()}
        for rel, target in links.items():
            assert not Path(target).is_absolute(), (rel, target)
            assert (LINUX / rel).resolve().is_relative_to(LINUX.resolve()), (rel, target)
        report['copied_internal_symlinks'] = links
        shutil.copytree(LINUX, src, symlinks=True, ignore=shutil.ignore_patterns('.git'))
        inventory = {str(p.relative_to(src)): sha(p) for p in sorted(src.rglob('*'))
                     if p.is_file()}
        (out / 'copied-linux-inputs.json').write_text(json.dumps(inventory, indent=2) + '\n')
        # Every copied source byte matches the unchanged read-only input tree.
        assert all(sha(LINUX / rel) == digest for rel, digest in inventory.items())
        driver = src / 'drivers/mmc/host/qc07f-candidate'
        driver.mkdir()
        for name in ['host-core.c', 'host-core.h', 'qc07f-mci.c', 'Kconfig', 'Makefile']:
            shutil.copyfile(snapshot / name, driver / name)
        edits = {}
        for rel, addition in [
            ('drivers/mmc/host/Makefile', '\nobj-$(CONFIG_MMC_QC07F_CANDIDATE) += qc07f-candidate/\n'),
            ('drivers/mmc/host/Kconfig', '\nsource "drivers/mmc/host/qc07f-candidate/Kconfig"\n')]:
            path = src / rel
            before = path.read_text()
            after = before + addition
            path.write_text(after)
            edits[rel] = ''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                fromfile='a/' + rel, tofile='b/' + rel))
        (out / 'integration.patch').write_text(''.join(edits.values()))
        build = out / 'kbuild'
        build.mkdir()
        original = CONFIG.read_text()
        candidate = original.replace('# CONFIG_MMC is not set',
                       'CONFIG_MMC=y\nCONFIG_MMC_QC07F_CANDIDATE=y\n# CONFIG_MMC_BLOCK is not set\n'
                       '# CONFIG_PWRSEQ_EMMC is not set\n# CONFIG_PWRSEQ_SIMPLE is not set')
        (build / '.config').write_text(candidate)
        args = ['make', '-C', src, 'O=' + str(build), 'ARCH=arm', 'LLVM=1']
        run('olddefconfig', args + ['olddefconfig'])
        run('prepare', args + ['-j2', 'prepare'], 600)
        run('arm-kbuild-objects', args + ['KCFLAGS=-Werror', '-j2',
                                       'drivers/mmc/host/qc07f-candidate/'], 600)
        objects = [build / 'drivers/mmc/host/qc07f-candidate' / name
                   for name in ['qc07f-mci.o', 'host-core.o']]
        # Relocatable link, not vmlinux/ABI/module/full runtime linkage proof.
        linked = out / 'qc07f-mci-candidate.arm-relocatable.o'
        run('arm-relocatable-link', ['ld.lld', '-r', *objects, '-o', linked])
        run('arm-elf', ['llvm-readelf', '-h', '-A', '-s', linked])
        wrapper = out / 'candidate-wrapper.dts'
        wrapper.write_text('/dts-v1/;\n/include/ "' + str(snapshot / 'candidate.dtsi') + '"\n')
        run('disabled-dt', ['dtc', '-I', 'dts', '-O', 'dtb', '-o', out / 'candidate-disabled.dtb', wrapper])
        assert not (logs / 'disabled-dt.log').read_text().strip()
        final = (build / '.config').read_text()
        assert 'CONFIG_CPU_ARM926T=y' in final and 'CONFIG_MMC_QC07F_CANDIDATE=y' in final
        assert '# CONFIG_MMC_BLOCK is not set' in final and '# CONFIG_EXT4_FS is not set' in final
        assert '# CONFIG_PWRSEQ_EMMC is not set' in final and '# CONFIG_PWRSEQ_SIMPLE is not set' in final
        (out / 'config75-to-object-candidate.patch').write_text(''.join(difflib.unified_diff(
            original.splitlines(True), final.splitlines(True), fromfile='kernel75.config',
            tofile='mmc-object-only.config')))
        # Kbuild .cmd captures the actual header/command dependency closure.
        headers = {}
        for p in sorted((build / 'drivers/mmc/host/qc07f-candidate').glob('.*.cmd')):
            for token in p.read_text().replace('\\\n', ' ').split():
                if token.endswith('.h'):
                    q = Path(token)
                    if not q.is_absolute():
                        q = build / q
                    if q.is_file(): headers[str(q)] = sha(q)
        (out / 'arm-header-closure.json').write_text(json.dumps(headers, indent=2) + '\n')
        changed = {rel: sha(src / rel) for rel in inventory if sha(src / rel) != inventory[rel]}
        assert set(changed) == set(edits), changed
        assert sha(CONFIG) == CONFIG_PIN
        report['input_config75_sha256'] = CONFIG_PIN
        report['build_config_sha256'] = sha(build / '.config')
        report['header_count'] = len(headers)
        report['source_count'] = len(inventory)
        report['copied_source_changes'] = changed
        report['status'] = 'SOURCE_AND_HOST_FIXTURE_PASS_ARM_OBJECT_LINK_ONLY_PROBE_REFUSES'
    finally:
        report['ended_utc'] = utc()
        report['environment'] = {key: env[key] for key in ['TMPDIR','LC_ALL','KBUILD_BUILD_USER',
                               'KBUILD_BUILD_HOST','KBUILD_BUILD_TIMESTAMP']}
        report['artifacts_sha256'] = {str(p.relative_to(out)): sha(p) for p in sorted(out.iterdir())
                                     if p.is_file()}
        (out / 'REPORT.json').write_text(json.dumps(report, indent=2) + '\n')
        print(out, flush=True)


if __name__ == '__main__':
    main()
