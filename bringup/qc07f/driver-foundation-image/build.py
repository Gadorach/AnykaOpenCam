#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""ONE offline built-in driver-foundation kernel/package; never target execution.

Fixed roots, no retry/repair/resume, no old-helper import. See CONTRACT.md and
NOTICES.md. Format/layout readers adapted from accepted query77 recipe TEXT.
"""
import datetime
import difflib
import gzip
import hashlib
import json
import lzma
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import struct
import subprocess
import traceback
import zlib

ROOT = Path(__file__).resolve().parents[3]
APP = Path(__file__).resolve().parent
DOC = ROOT / 'docs/qualification-plans/driver-foundation-image-20261024'
BASE = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
META = BASE / 'analysis/encoder-query77-build02'
OLD = BASE / 'artifacts/encoder-query77-source02'
OUT = ROOT / 'work/lightbulb-camera/driver-foundation-20261024'
SRC, BUILD = OUT / 'linux', OUT / 'build'
PTZ = 'arch/arm/mach-ak3918ev200/ptz.c'
CANDIDATE = ROOT / 'bringup/qc07f/ptz-off-retain-candidate/ptz.c'
PTZ_SHA = 'a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87'
VERSION = '-qc07f-driver-foundation01'
ARCHIVE = OUT / 'inputs/initramfs-driver-foundation01.cpio'
DTB = OUT / 'inputs/attempt45.dtb'
COMMANDS = []
ENV = dict(PATH='/usr/bin:/bin', HOME=str(OUT / 'home'), TMPDIR=str(OUT / 'tmp'),
           LANG='C', LC_ALL='C', PYTHONDONTWRITEBYTECODE='1',
           SOURCE_DATE_EPOCH='1792627200', KBUILD_BUILD_USER='qualification',
           KBUILD_BUILD_HOST='offline', KBUILD_BUILD_TIMESTAMP='2026-10-22 00:00:00 UTC',
           KBUILD_BUILD_VERSION='1', LOCALVERSION='', LD_DEBUG='files,libs',
           LD_DEBUG_OUTPUT=str(OUT / 'loader/trace'))
MAKE = ['/usr/bin/make', '-C', str(SRC), 'O=' + str(BUILD), 'ARCH=arm',
        'LLVM=1', 'LLVM_IAS=1', 'HOSTCC=clang', 'HOSTCXX=clang++', 'LOCALVERSION=', 'V=1']


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def pin(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for chunk in iter(lambda: f.read(1048576), b''):
            h.update(chunk)
    return dict(bytes=Path(path).stat().st_size, sha256=h.hexdigest())


def write(path, value):
    with Path(path).open('x') as f:
        json.dump(value, f, indent=2, sort_keys=True)
        f.write('\n')


def save(path, data):
    with Path(path).open('xb') as f:
        f.write(data)


def inventory(root, confined=True):
    rows = {}
    for parent, dirs, files in os.walk(root, followlinks=False):
        for name in sorted(dirs + files):
            p = Path(parent) / name
            rel, mode = str(p.relative_to(root)), p.lstat().st_mode
            if stat.S_ISLNK(mode):
                target = os.readlink(p)
                if confined:
                    require(not Path(target).is_absolute() and p.resolve().is_relative_to(root.resolve()),
                            'source symlink escape: ' + rel)
                rows[rel] = {'link': target}
            elif stat.S_ISREG(mode):
                rows[rel] = dict(pin(p), mode=oct(stat.S_IMODE(mode)))
            else:
                require(stat.S_ISDIR(mode), 'special source file: ' + rel)
    return rows


def command(label, argv):
    log = OUT / 'logs' / (label + '.log')
    row = dict(argv=list(map(str, argv)), cwd=str(ROOT), environment=ENV,
               started=now(), log=str(log.relative_to(ROOT)), recipe=pin(__file__))
    COMMANDS.append(row)
    print('START ' + label, flush=True)
    try:
        with log.open('xb') as f:
            result = subprocess.run(row['argv'], cwd=ROOT, env=ENV, stdout=f,
                                    stderr=subprocess.STDOUT, timeout=2400)
        row['exit'] = result.returncode
    finally:
        row['ended'] = now()
        if log.is_file():
            row['log_pin'] = pin(log)
        with (OUT / 'commands.jsonl').open('a') as f:
            f.write(json.dumps(row) + '\n')
    require(row.get('exit') == 0, 'command failed: ' + label)
    print('END ' + label, flush=True)
    return log.read_text(errors='replace')


def config(text):
    rows = {}
    for line in text.splitlines():
        m = re.fullmatch(r'(CONFIG_\w+)=(.*)', line)
        n = re.fullmatch(r'# (CONFIG_\w+) is not set', line)
        if m:
            rows[m[1]] = m[2]
        elif n:
            rows[n[1]] = 'n'
    return rows


def cpio(data):
    """Headers/graph first, no extraction or opaque payload inspection."""
    rows, pos = {}, 0
    regular = {'init', 'hgics.ko', 'bin/busybox', 'bin/wpa_supplicant', 'bin/wpa_cli',
               'bin/rtspd', 'bin/udhcpc.script', 'bin/qc07f-dhcp-script-bridge'}
    directories = {'dev', 'proc', 'sys', 'bin', 'run', 'etc', 'tmp'}
    while True:
        require(data[pos:pos + 6] == b'070701' and pos + 110 <= len(data), 'newc header')
        w = [int(data[pos + 6 + i:pos + 14 + i], 16) for i in range(0, 104, 8)]
        raw_name = data[pos + 110:pos + 110 + w[11]]
        require(raw_name.endswith(b'\0') and b'\0' not in raw_name[:-1], 'newc name')
        name = raw_name[:-1].decode('ascii')
        p = PurePosixPath(name)
        require(name and name == str(p) and not p.is_absolute() and '..' not in p.parts
                and name not in rows, 'unsafe/duplicate newc path')
        start = (pos + 110 + w[11] + 3) & ~3
        end = start + w[6]
        next_pos = (end + 3) & ~3
        require(next_pos <= len(data), 'newc extent')
        row = dict(words=w, body_start=start, body_end=end, record_start=pos, record_end=next_pos)
        require(w[4] == 1, 'unsupported newc hardlink group')
        if name == 'TRAILER!!!':
            require(w[1] == w[6] == 0, 'trailer content')
        elif stat.S_ISREG(w[1]):
            require(name in regular, 'unselected/private regular payload path: ' + name)
        elif stat.S_ISDIR(w[1]):
            require(name in directories and w[6] == 0, 'unexpected directory')
        elif stat.S_ISLNK(w[1]):
            # The observed package contains only bin applet links to busybox.
            target = data[start:end].decode('ascii')
            require(p.parent == PurePosixPath('bin') and target == 'busybox', 'unsupported symlink')
            row['link'] = target
        else:
            require(name == 'dev/console' and stat.S_ISCHR(w[1]) and w[6] == 0
                    and w[9:11] == [5, 1], 'unexpected special member')
        rows[name] = row
        pos = next_pos
        if name == 'TRAILER!!!':
            require(not any(data[pos:]), 'nonzero archive trailer padding')
            break
    for name, row in rows.items():
        p = PurePosixPath(name)
        for parent in p.parents:
            if str(parent) != '.':
                require(str(parent) in rows and stat.S_ISDIR(rows[str(parent)]['words'][1]),
                        'member below absent/non-directory/symlink ancestor')
        if 'link' in row:
            target = str(p.parent / row['link'])
            require(target in rows and stat.S_ISREG(rows[target]['words'][1]), 'dangling link')
    return rows


def public_archive_rows(data, rows):
    # Only after the graph/regular payload allowlist has been admitted.
    return {name: dict(header_words=r['words'], link=r.get('link'),
                       body_sha256=hashlib.sha256(data[r['body_start']:r['body_end']]).hexdigest())
            for name, r in rows.items()}


def fdt(data):
    magic, size, structure, strings, reserve, version, last, cpu, ns, nt = struct.unpack_from('>10I', data)
    require(magic == 0xd00dfeed and size == len(data) and structure + nt <= size
            and strings + ns <= size, 'DTB extent')
    reservations = []
    while True:
        require(reserve + 16 <= size, 'DTB reserve extent')
        address, length = struct.unpack_from('>QQ', data, reserve)
        reserve += 16
        if address == length == 0:
            break
        reservations.append([address, address + length])
    names, pos, end, stack, props = data[strings:strings + ns], structure, structure + nt, [], {}
    while pos < end:
        tag = struct.unpack_from('>I', data, pos)[0]
        pos += 4
        if tag == 1:
            close = data.index(b'\0', pos, end)
            stack.append(data[pos:close].decode('ascii'))
            pos = (close + 4) & ~3
        elif tag == 2:
            require(stack, 'DTB underflow')
            stack.pop()
        elif tag == 3:
            length, name = struct.unpack_from('>II', data, pos)
            pos += 8
            require(pos + length <= end and name < len(names), 'DTB property extent')
            key = names[name:names.index(b'\0', name)].decode('ascii')
            props['/' + '/'.join(stack[1:]) + ':' + key] = data[pos:pos + length]
            pos = (pos + length + 3) & ~3
        elif tag == 4:
            continue
        elif tag == 9:
            require(not stack, 'DTB unclosed')
            return props, reservations
        else:
            raise RuntimeError('DTB unknown tag')
    raise RuntimeError('DTB missing end')


def elf(path):
    data = Path(path).read_bytes()
    require(data[:7] == b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H', data, 18)[0] == 40, 'ELF ARM32 LE')
    offset = struct.unpack_from('<I', data, 32)[0]
    size, count, si = struct.unpack_from('<HHH', data, 46)
    require(size == 40 and offset + count * size <= len(data), 'ELF sections extent')
    rows = [struct.unpack_from('<10I', data, offset + i * size) for i in range(count)]
    names = data[rows[si][4]:rows[si][4] + rows[si][5]]
    sections = {names[r[0]:names.index(b'\0', r[0])].decode():
                dict(type=r[1], flags=r[2], address=r[3], offset=r[4], bytes=r[5]) for r in rows if r[0]}
    symbols = {}
    for r in rows:
        if r[1] != 2:
            continue
        text = data[rows[r[6]][4]:rows[r[6]][4] + rows[r[6]][5]]
        require(r[9] == 16, 'ELF symbol size')
        for i in range(0, r[5], 16):
            name, value, length, info, other, index = struct.unpack_from('<IIIBBH', data, r[4] + i)
            if name:
                symbols[text[name:text.index(b'\0', name)].decode()] = [value, length, index]
    return symbols, sections


def uimage(data):
    magic, hc, timestamp, size, load, entry, dc, os_, arch, kind, compression, name = struct.unpack_from('>7I4B32s', data)
    require(magic == 0x27051956 and len(data) == size + 64, 'UIM extent/magic')
    require(zlib.crc32(data[:4] + b'\0' * 4 + data[8:64]) & 0xffffffff == hc, 'UIM header CRC')
    require(zlib.crc32(data[64:]) & 0xffffffff == dc, 'UIM data CRC')
    return dict(load=load, entry=entry, timestamp=timestamp, payload_bytes=size,
                header_CRC32_IEEE=f'{hc:08x}', data_CRC32_IEEE=f'{dc:08x}', OS=os_, arch=arch,
                type=kind, outer_compression=compression, name=name.rstrip(b'\0').decode())


def bzip_crc(data):
    table = []
    for i in range(256):
        n = i << 24
        for _ in range(8):
            n = ((n << 1) ^ (0x04c11db7 if n & 0x80000000 else 0)) & 0xffffffff
        table.append(n)
    n = 0xffffffff
    for byte in data:
        n = ((n << 8) & 0xffffffff) ^ table[((n >> 24) ^ byte) & 255]
    return n ^ 0xffffffff


def main():
    require(not OUT.exists() and not OUT.is_symlink(), 'fresh output root required; no retry')
    OUT.mkdir()
    for name in ('inputs', 'logs', 'loader', 'home', 'tmp', 'build', 'images'):
        (OUT / name).mkdir()
    for p in (Path(__file__), APP / 'NOTICES.md', DOC / 'CONTRACT.md'):
        shutil.copyfile(p, OUT / 'inputs' / p.name)
    baseline_doc = ROOT / 'docs/qualification-plans/encoder-query77-build-20261022'
    manifest = json.loads((baseline_doc / 'BUILD_MANIFEST01.json').read_text())
    source_map_file = META / 'final-source-files.json'
    build_map_file = META / 'final-build-files.json'
    for p in (source_map_file, build_map_file, META / 'inputs/host-tools.json', META / 'inputs/clang-resources.json'):
        require(pin(p) == manifest['local_evidence_manifest_pins'][str(p.relative_to(ROOT))], 'baseline manifest drift')
    source_map = json.loads(source_map_file.read_text())
    build_map = json.loads(build_map_file.read_text())
    require(inventory(OLD / 'linux') == source_map, 'baseline source set/content/mode drift')
    require(inventory(META / 'build', confined=False) == build_map, 'baseline build drift')
    require(not any(p.name == '.git' for p in (OLD / 'linux').rglob('.git'))
            and not list((OLD / 'linux').glob('localversion*'))
            and not (OLD / 'linux/.scmversion').exists(), 'unadmitted SCM/version metadata')
    require(pin(CANDIDATE)['sha256'] == PTZ_SHA, 'reviewed PTZ source drift')
    baseline_inputs = [baseline_doc / 'PARENT_DISPOSITION.md', baseline_doc / 'BUILD_MANIFEST01.json',
                       source_map_file, build_map_file, CANDIDATE,
                       ROOT / 'docs/qualification-plans/ptz-off-retain-20261024/CONTRACT.md',
                       ROOT / 'docs/qualification-plans/ptz-off-retain-20261024/PARENT_DISPOSITION.md',
                       ROOT / 'docs/qualification-plans/ptz-off-retain-review-20261024/REVIEW.md',
                       ROOT / 'docs/plan/camera-foundation/SUPERVISED_DRIVER_BENCH_20261024.md',
                       ROOT / manifest['package']['image'], OLD / 'inputs/attempt45.dtb',
                       OLD / 'inputs/initramfs76.cpio',
                       BASE / 'artifacts/native-security75-source01/images/QC6S75.UIM',
                       BASE / 'artifacts/native-security75-source01/images/hgics-linux6-station75.ko',
                       BASE / 'artifacts/native-usb76-source01/images/QC6U76.UIM',
                       BASE / 'artifacts/native-usb76-source01/station76-final/hgic_smac/hgics.ko']
    custody = {str(p.relative_to(ROOT)): pin(p) for p in baseline_inputs}
    require(pin(ROOT / manifest['package']['image']) == manifest['package']['image_pin'], 'query77 image drift')
    require(pin(OLD / 'inputs/initramfs76.cpio') == manifest['package']['initramfs_pin'], 'old archive drift')
    write(OUT / 'inputs/baseline-and-rollback.json', custody)
    tools = json.loads((META / 'inputs/host-tools.json').read_text())
    for path, row in tools.items():
        require(pin(path) == {k: row[k] for k in ('bytes', 'sha256')}
                and str(Path(path).resolve()) == row['resolved'], 'baseline tool drift: ' + path)
    resources = json.loads((META / 'inputs/clang-resources.json').read_text())
    require(inventory(Path('/usr/lib/clang/23')) == resources, 'Clang resource drift')
    write(OUT / 'inputs/tools-before.json', tools)
    write(OUT / 'inputs/source-baseline-reference.json', dict(path=str(source_map_file.relative_to(ROOT)),
          pin=pin(source_map_file), members=len(source_map), SCM_present=False, all_links_confined=True))
    old_archive = (OLD / 'inputs/initramfs76.cpio').read_bytes()
    require(gzip.decompress((META / 'build/usr/initramfs_inc_data').read_bytes()) == old_archive, 'old embedded archive')
    old_rows = cpio(old_archive)
    removed = {'hgics.ko', 'bin/rtspd'}
    require(removed <= old_rows.keys(), 'exact removal paths absent')
    for name in removed:
        require(stat.S_ISREG(old_rows[name]['words'][1]), 'removal path not regular')
    new_archive = b''.join(old_archive[r['record_start']:r['record_end']]
                           for name, r in old_rows.items() if name not in removed)
    new_archive += b'\0' * (-len(new_archive) % 512)
    new_rows = cpio(new_archive)
    require(list(new_rows) == [n for n in old_rows if n not in removed], 'new archive membership/order')
    for name, r in new_rows.items():
        before = old_rows[name]
        require(new_archive[r['record_start']:r['record_end']] ==
                old_archive[before['record_start']:before['record_end']], 'retained record differs: ' + name)
    save(ARCHIVE, new_archive)
    archive_proof = dict(original=pin(OLD / 'inputs/initramfs76.cpio'), fresh=pin(ARCHIVE),
                         removed=sorted(removed), all_retained_raw_records_identical=True,
                         before=public_archive_rows(old_archive, old_rows),
                         after=public_archive_rows(new_archive, new_rows), extracted_or_executed=False)
    write(OUT / 'inputs/initramfs-transform.json', archive_proof)
    shutil.copyfile(OLD / 'inputs/attempt45.dtb', DTB)
    require(pin(DTB) == manifest['package']['DTB_pin'], 'DTB drift')
    props, reserves = fdt(DTB.read_bytes())
    require(props['/memory@81800000:reg'] == struct.pack('>II', 0x81800000, 0x02800000)
            and not reserves and not any(k.startswith('/reserved-memory') for k in props), 'RAM/DT reserved layout')
    provider = '/soc/gpio@20170000'
    require(props[provider + ':compatible'] == b'anyka,ak3918ev200-gpio\0'
            and props[provider + ':status'] == b'okay\0'
            and props[provider + ':phandle'] == struct.pack('>I', 3)
            and props[provider + ':#gpio-cells'] == struct.pack('>I', 2), 'direct GPIO provider')
    for axis, pins in enumerate(((10, 11, 19, 20), (15, 14, 13, 24))):
        node = '/soc/ptz-axis' + str(axis)
        require(props[node + ':status'] == b'okay\0'
                and props[node + ':compatible'] == b'anyka,qc07f-ptz-diagnostic\0'
                and props[node + ':anyka,retail-board-version'] == struct.pack('>I', 5)
                and props[node + ':anyka,axis'] == struct.pack('>I', axis)
                and props[node + ':phase-gpios'] == b''.join(struct.pack('>III', 3, p, 0) for p in pins),
                'actual PTZ lookup/flags/board mismatch')
    command('retained-DTB', ['/usr/bin/dtc', '-I', 'dtb', '-O', 'dts', DTB])
    shutil.copytree(OLD / 'linux', SRC, symlinks=True)
    require(inventory(SRC) == source_map, 'fresh full source copy differs')
    shutil.copyfile(CANDIDATE, SRC / PTZ)
    fresh_map = inventory(SRC)
    delta = {k: dict(old=source_map.get(k), new=fresh_map.get(k))
             for k in source_map.keys() | fresh_map.keys() if source_map.get(k) != fresh_map.get(k)}
    require(set(delta) == {PTZ} and fresh_map[PTZ]['sha256'] == PTZ_SHA, 'unrelated source delta')
    write(OUT / 'inputs/source-delta.json', delta)
    save(OUT / 'inputs/ptz.patch', ''.join(difflib.unified_diff((OLD / 'linux' / PTZ).read_text().splitlines(True),
         (SRC / PTZ).read_text().splitlines(True), fromfile='accepted-query77/' + PTZ,
         tofile='driver-foundation01/' + PTZ)).encode())
    save(OUT / 'inputs/reviewed-ptz.c', CANDIDATE.read_bytes())
    oldcfg = (META / 'build/.config').read_text()
    cfg = config(oldcfg)
    require(cfg['CONFIG_LOCALVERSION'] == '"-qc07f-encoder-query77"'
            and cfg['CONFIG_LOCALVERSION_AUTO'] == 'n'
            and cfg['CONFIG_PTZ_DIAGNOSTIC_QC07F'] == 'y' and cfg.get('CONFIG_SOUND') == 'n'
            and not any(v == 'm' for v in cfg.values()), 'baseline built-in/audio/config selection')
    freshcfg = oldcfg.replace('CONFIG_LOCALVERSION=' + cfg['CONFIG_LOCALVERSION'],
                              'CONFIG_LOCALVERSION=' + json.dumps(VERSION))
    freshcfg = freshcfg.replace('CONFIG_INITRAMFS_SOURCE=' + cfg['CONFIG_INITRAMFS_SOURCE'],
                                'CONFIG_INITRAMFS_SOURCE=' + json.dumps(str(ARCHIVE)))
    save(BUILD / '.config', freshcfg.encode())
    for name in ('clang', 'ld.lld', 'make', 'llvm-ar', 'llvm-objcopy', 'llvm-readelf', 'dtc'):
        text = command(name + '-version', ['/usr/bin/' + name, '--version'])
        if name == 'clang':
            require('clang version 23.1.3' in text, 'compiler version')
        if name == 'ld.lld':
            require('LLD 23.1.3' in text, 'linker version')
    command('olddefconfig', MAKE + ['olddefconfig'])
    actualcfg = (BUILD / '.config').read_text()
    after = config(actualcfg)
    cfgdelta = {k: dict(old=cfg.get(k), new=after.get(k)) for k in cfg.keys() | after.keys()
                if cfg.get(k) != after.get(k)}
    require(set(cfgdelta) == {'CONFIG_LOCALVERSION', 'CONFIG_INITRAMFS_SOURCE'}, 'unexpected config delta')
    require(after['CONFIG_CC_VERSION_TEXT'] == '"clang version 23.1.3"'
            and all(after[k] == '230103' for k in ('CONFIG_CLANG_VERSION', 'CONFIG_AS_VERSION', 'CONFIG_LLD_VERSION')),
            'incoherent compiler config')
    save(OUT / 'inputs/config.diff', ''.join(difflib.unified_diff(oldcfg.splitlines(True), actualcfg.splitlines(True),
         fromfile='accepted-query77.config', tofile='driver-foundation01.config')).encode())
    write(OUT / 'CONFIG.json', dict(delta=cfgdelta, pin=pin(BUILD / '.config')))
    preconfig = pin(BUILD / '.config')
    write(OUT / 'PREBUILD.json', dict(UTC=now(), recipe=pin(__file__), environment=ENV,
          full_build_argv=MAKE + ['-j4', 'zImage', 'vmlinux'], source_delta=delta,
          config_delta=cfgdelta, new_archive=pin(ARCHIVE), DTB=pin(DTB), attempt=1))
    command('kernel-builtins', MAKE + ['-j4', 'zImage', 'vmlinux'])
    require(pin(BUILD / '.config') == preconfig, 'build mutated selected config')
    require((BUILD / 'include/config/kernel.release').read_text().strip() == '6.18.55' + VERSION, 'release identity')
    require(gzip.decompress((BUILD / 'usr/initramfs_inc_data').read_bytes()) == new_archive, 'new embedded archive differs')
    ptzcmd = (BUILD / 'arch/arm/mach-ak3918ev200/.ptz.o.cmd').read_text()
    oldcmd = (META / 'build/arch/arm/mach-ak3918ev200/.ptz.o.cmd').read_text()
    require(ptzcmd.splitlines()[0] == oldcmd.splitlines()[0].replace(str(OLD / 'linux'), str(SRC)),
            'PTZ compiler flags differ from accepted baseline')
    require(str(SRC / PTZ) in ptzcmd and str(OLD / 'linux') not in ptzcmd
            and '-DMODULE' not in ptzcmd.splitlines()[0], 'PTZ built-in source/header association')
    require('clang version 23.1.3' in (BUILD / 'include/generated/compile.h').read_text(), 'generated compiler identity')
    for label, args in (
        ('vmlinux-ELF', ['llvm-readelf', '-h', '-l', '-S', BUILD / 'vmlinux']),
        ('compressed-ELF', ['llvm-readelf', '-h', '-l', '-S', BUILD / 'arch/arm/boot/compressed/vmlinux']),
        ('vmlinux-symbols', ['llvm-nm', '-n', BUILD / 'vmlinux']),
        ('PTZ-ELF', ['llvm-readelf', '-h', '-A', '-s', BUILD / 'arch/arm/mach-ak3918ev200/ptz.o']),
        ('PTZ-object-code', ['llvm-objdump', '-dr', BUILD / 'arch/arm/mach-ak3918ev200/ptz.o']),
        ('PTZ-linked-code', ['llvm-objdump', '-d', '--disassemble-symbols=pulse_store,state_show,ak_ptz_probe', BUILD / 'vmlinux'])):
        command(label, ['/usr/bin/' + args[0], *args[1:]])
    symbols, sections = elf(BUILD / 'vmlinux')
    compressed, csections = elf(BUILD / 'arch/arm/boot/compressed/vmlinux')
    wanted = ('pulse_store', 'state_show', 'ak_ptz_probe', 'ak_ptz_hold', 'ak_ptz_driver')
    require(all(k in symbols and symbols[k][2] != 0 for k in wanted), 'actual linked PTZ definitions absent')
    image = (BUILD / 'arch/arm/boot/Image').read_bytes()
    piggy = (BUILD / 'arch/arm/boot/compressed/piggy_data').read_bytes()
    require(after['CONFIG_KERNEL_XZ'] == after['CONFIG_INITRAMFS_COMPRESSION_GZIP'] == 'y'
            and struct.unpack_from('<I', piggy, len(piggy) - 4)[0] == len(image)
            and lzma.decompress(piggy[:-4]) == image, 'actual XZ/LE32/Image association')
    phys = lambda v: v - 0xc0000000 + 0x81800000
    text, end = phys(symbols['_text'][0]), phys(symbols['_end'][0])
    bs, be = phys(symbols['__bss_start'][0]), phys(symbols['__bss_stop'][0])
    pgdir = phys(symbols['swapper_pg_dir'][0])
    require(pgdir == 0x81804000 and pgdir + 16384 == text == 0x81808000
            and text + len(image) <= bs <= be <= end < 0x82408000, 'kernel/BSS/page table overlap')
    require(compressed['_kernel_bss_size'][0] == be - bs, 'compressed kernel BSS association')
    zimage = (BUILD / 'arch/arm/boot/zImage').read_bytes()
    require(struct.unpack_from('<I', zimage, 0x24)[0] == 0x016f2818, 'zImage magic')
    zs, ze = struct.unpack_from('<II', zimage, 0x28)
    require(ze - zs == len(zimage) == compressed['_edata'][0] - compressed['_start'][0], 'zImage extent')
    dtb = DTB.read_bytes()
    growth = min(max((len(dtb) + len(dtb) // 2 + 7) & ~7, 32768), 1048576)
    stack_end = 0x82408040 - compressed['_start'][0] + csections['.stack']['address'] + csections['.stack']['bytes']
    live_end = stack_end + growth + 65536
    require(end < 0x82408040 + compressed['wont_overwrite'][0] and live_end < 0x82800000,
            'decompressor/DTB/stack/malloc exceeds original CMA split')
    old_image = (ROOT / manifest['package']['image']).read_bytes()
    old_header = uimage(old_image)
    require(old_header['load'] == 0x82408000 and old_header['entry'] == 0x82408040
            and [old_header[k] for k in ('timestamp', 'OS', 'arch', 'type', 'outer_compression')] == [0, 5, 2, 2, 0]
            and old_image[64:] == (META / 'build/arch/arm/boot/zImage').read_bytes() + dtb, 'baseline format association')
    payload = zimage + dtb
    header = struct.pack('>7I4B32s', 0x27051956, 0, 0, len(payload), 0x82408000, 0x82408040,
                         zlib.crc32(payload) & 0xffffffff, 5, 2, 2, 0,
                         b'QC07F driver-foundation01 RAM'.ljust(32, b'\0'))
    header = header[:4] + struct.pack('>I', zlib.crc32(header) & 0xffffffff) + header[8:]
    data = header + payload
    require(len(data) <= 0x400000 and 0x82408000 + len(data) < 0x82800000, 'UIM exceeds original stage')
    parsed = uimage(data)
    output = OUT / 'images/QC6DF01.UIM'
    save(output, data)
    output_names = ('.config', 'vmlinux', 'System.map', 'vmlinux.symvers', 'arch/arm/boot/Image',
                    'arch/arm/boot/zImage', 'arch/arm/boot/compressed/vmlinux', 'arch/arm/boot/compressed/piggy_data',
                    'arch/arm/mach-ak3918ev200/ptz.o', 'arch/arm/mach-ak3918ev200/.ptz.o.cmd',
                    'arch/arm/mach-ak3918ev200/built-in.a', 'usr/initramfs_inc_data',
                    'include/config/kernel.release', 'include/generated/compile.h', 'include/generated/autoconf.h')
    commands, consumed = {}, {}
    for p in sorted(BUILD.rglob('*.cmd')):
        commands[str(p.relative_to(BUILD))] = pin(p)
        for line in p.read_text(errors='replace').replace('\\\n', ' ').splitlines():
            if not re.match(r'(?:source_|deps_)\S*\s*:=', line):
                continue
            deps = re.sub(r'\$\(wildcard ([^)]+)\)', r'\1', line.split(':=', 1)[1])
            for name in deps.split():
                if '$' in name:
                    continue
                dep = Path(name) if Path(name).is_absolute() else BUILD / name
                if str(dep) not in consumed and dep.is_file():
                    consumed[str(dep)] = dict(pin(dep), resolved=str(dep.resolve()))
    write(OUT / 'inputs/kbuild-command-files.json', commands)
    write(OUT / 'inputs/consumed-kbuild-source-headers.json', consumed)
    loaded = {}
    for log in (OUT / 'loader').glob('trace.*'):
        for name in re.findall(r'(?:calling init:|calling fini:|file=|transferring control:)\s*(/[^\s;\[\n]+)',
                               log.read_text(errors='replace')):
            p = Path(name)
            if name not in loaded and p.is_file():
                loaded[name] = dict(pin(p), resolved=str(p.resolve()))
    write(OUT / 'inputs/loaded-host-ELF-final-only.json', loaded)
    require(inventory(OLD / 'linux') == source_map and inventory(SRC) == fresh_map, 'source custody drift')
    require(inventory(META / 'build', confined=False) == build_map, 'baseline build custody drift')
    for name, row in custody.items():
        require(pin(ROOT / name) == row, 'baseline/rollback input drift: ' + name)
    for name, row in tools.items():
        require(pin(name) == {k: row[k] for k in ('bytes', 'sha256')}
                and str(Path(name).resolve()) == row['resolved'], 'tool drift during build: ' + name)
    require(inventory(Path('/usr/lib/clang/23')) == resources, 'resource drift during build')
    result = dict(status='OFFLINE_FULL_KERNEL_AND_PACKAGE_PASS_NOT_BOOTED', UTC=now(),
                  image=str(output.relative_to(ROOT)), image_pin=pin(output), header=parsed,
                  fullimage_CRC32_BZIP2=f'{bzip_crc(data):08x}',
                  kernel_release='6.18.55' + VERSION, source=str(SRC.relative_to(ROOT)),
                  build=str(BUILD.relative_to(ROOT)), recipe=pin(__file__),
                  source_delta=delta, config_delta=cfgdelta, PTZ_source=pin(SRC / PTZ),
                  PTZ_linked_symbols={k: symbols[k] for k in wanted}, PTZ_builtin_no_MODULE=True,
                  PTZ_saved_cmd_exact_baseline_flags_new_source=True, DTB=pin(DTB),
                  initramfs=dict(before=archive_proof['original'], after=pin(ARCHIVE),
                                 removed=sorted(removed), retained_raw_records_identical=True,
                                 monitor_unchanged=True, embedded_verified=True),
                  outputs={n: pin(BUILD / n) for n in output_names},
                  memory=dict(RAM=[0x81800000, 0x84000000], initial_page_directory=[pgdir, pgdir + 16384],
                              kernel_live=[text, end], decompressed_file=[text, text + len(image)],
                              kernel_BSS=[bs, be], staged_uImage=[0x82408000, 0x82408000 + len(data)],
                              appended_DTB=[0x82408040 + len(zimage), 0x82408040 + len(zimage) + len(dtb)],
                              decompressor_stack=csections['.stack'], decompressor_stack_original_end=stack_end,
                              DTB_growth_bound=growth, decompressor_malloc_bytes=65536,
                              decompressor_live_upper_bound=live_end, remaining_to_CMA_bytes=0x82800000-live_end,
                              existing_CMA=[0x82800000, 0x83800000], combined_load_or_DMA_proved=False),
                  counts=dict(source_members=len(source_map), baseline_build_members=len(build_map),
                              kbuild_commands=len(commands), consumed_dependencies=len(consumed),
                              host_tools=len(tools), clang_resources=len(resources), loaded_host_ELF=len(loaded),
                              top_level_commands=len(COMMANDS), full_Kbuild_invocations=1),
                  all_baseline_source_build_selected_rollback_unchanged=True,
                  tools_and_resources_unchanged=True, no_target_or_test_execution=True,
                  no_modules_install_or_hardware=True, independent_review_required=True,
                  strict_C1_proved=False, driver_ready=False, PLAT_02_complete=False)
    write(OUT / 'BUILD_MANIFEST.json', result)
    print(json.dumps({k: result[k] for k in ('status', 'image', 'image_pin', 'fullimage_CRC32_BZIP2', 'memory', 'counts')}, indent=2))


if __name__ == '__main__':
    try:
        main()
    except BaseException as exc:
        if OUT.is_dir() and not (OUT / 'FAILURE.json').exists():
            write(OUT / 'FAILURE.json', dict(UTC=now(), error=type(exc).__name__ + ': ' + str(exc),
                  traceback=traceback.format_exc(), commands=COMMANDS, outputs_preserved=True,
                  retry_authorized=False, image_eligible=False))
        raise
