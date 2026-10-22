#!/usr/bin/env python3
"""Read/hash/format verification only; no candidate import, build or target IO."""
import datetime
import gzip
import hashlib
import json
import lzma
import os
from pathlib import Path
import re
import struct
import zlib

ROOT = Path(__file__).resolve().parents[3]
DOC = Path(__file__).resolve().parent
BASE = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
OUT = BASE / 'analysis/encoder-query77-build02'
NEW = BASE / 'artifacts/encoder-query77-source02/linux'
OLD = BASE / 'artifacts/native-usb76-source01'
BUILD = OUT / 'build'
COUNTS = {}
CACHE = {}


def load(path):
    return json.loads(Path(path).read_bytes())


def digest(path):
    path = Path(path)
    key = str(path.absolute())
    if key not in CACHE:
        h = hashlib.sha256()
        with path.open('rb') as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b''):
                h.update(chunk)
        CACHE[key] = {'bytes': path.stat().st_size, 'sha256': h.hexdigest()}
    return CACHE[key]


def verify(root, rows, label):
    for name, pin in rows.items():
        path = Path(root) / name
        if 'link' in pin:
            assert path.is_symlink() and os.readlink(path) == pin['link'], path
        else:
            actual = digest(path)
            assert actual['sha256'] == pin['sha256'], path
            assert 'bytes' not in pin or actual['bytes'] == pin['bytes'], path
            if 'resolved' in pin:
                assert str(path.resolve()) == pin['resolved'], path
    COUNTS[label] = len(rows)


def names(root, omit_git=False):
    result = set()
    for parent, dirs, files in os.walk(root, followlinks=False):
        if omit_git and Path(parent) == root and '.git' in dirs:
            dirs.remove('.git')
        for name in dirs + files:
            path = Path(parent) / name
            if path.is_symlink() or path.is_file():
                result.add(str(path.relative_to(root)))
    return result


def config(path):
    rows = {}
    for line in path.read_text().splitlines():
        m = re.fullmatch(r'(CONFIG_\w+)=(.*)', line)
        n = re.fullmatch(r'# (CONFIG_\w+) is not set', line)
        if m:
            assert m[1] not in rows
            rows[m[1]] = m[2]
        elif n:
            assert n[1] not in rows
            rows[n[1]] = 'n'
    return rows


def symbols(path):
    data = path.read_bytes()
    assert data[:7] == b'\x7fELF\x01\x01\x01'
    assert struct.unpack_from('<H', data, 18)[0] == 40
    offset = struct.unpack_from('<I', data, 32)[0]
    size, count, string_index = struct.unpack_from('<HHH', data, 46)
    assert size == 40 and offset + count * size <= len(data)
    sections = [struct.unpack_from('<10I', data, offset + size * i) for i in range(count)]
    s = sections[string_index]
    section_names = data[s[4]:s[4] + s[5]]
    sec = {section_names[s[0]:section_names.index(b'\0', s[0])].decode(): s
           for s in sections if s[0]}
    result = {}
    for s in sections:
        if s[1] != 2:
            continue
        strings = sections[s[6]]
        text = data[strings[4]:strings[4] + strings[5]]
        assert s[9] == 16
        for pos in range(s[4], s[4] + s[5], 16):
            name, value, length, _, _, _ = struct.unpack_from('<IIIBBH', data, pos)
            if name:
                result[text[name:text.index(b'\0', name)].decode()] = value
    return result, sec


def main():
    seal = load(DOC / 'SOURCE_SEAL01.json')
    verify(ROOT, seal['source_files'], 'author_source_report_pins')
    manifest = load(DOC / 'BUILD_MANIFEST01.json')
    verify(ROOT, manifest['local_evidence_manifest_pins'], 'local_evidence_manifests')
    assert digest(OUT / 'BUILD_SEAL02.json') == seal['work_final_build_seal']
    verify(OUT, load(OUT / 'BUILD_SEAL02.json')['file_pins'], 'final_work_seal')
    old_rows = load(OUT / 'inputs/retained76-source.json')
    new_rows = load(OUT / 'final-source-files.json')
    verify(OLD / 'linux', old_rows, 'retained_source')
    verify(NEW, new_rows, 'new_source')
    assert names(OLD / 'linux', True) == set(old_rows)
    assert names(NEW) == set(new_rows)
    assert not (NEW / '.git').exists()
    verify(OLD / 'linux/.git', load(OUT / 'inputs/retained76-scm.json'), 'retained_scm_data')
    verify(BASE / 'analysis/native-usb76-package01/build',
           load(OUT / 'inputs/retained76-build.json'), 'retained_build')
    verify(ROOT, load(OUT / 'inputs/custody.json'), 'prior_query_package_custody')
    verify(BUILD, load(OUT / 'final-build-files.json'), 'new_build_files')
    assert names(BUILD) == set(load(OUT / 'final-build-files.json'))
    changed = sorted(k for k in old_rows.keys() | new_rows.keys() if old_rows.get(k) != new_rows.get(k))
    board = 'arch/arm/mach-ak3918ev200/'
    assert changed == sorted(board + n for n in
        ('qc07f_hwenc.c', 'qc07f_hwenc.h', 'encoder-open-query.h', 'encoder-open-query.inc'))
    candidate = ROOT / 'docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate'
    for name in changed:
        assert (NEW / name).read_bytes() == (candidate / Path(name).name).read_bytes()
    before = config(OUT / 'inputs/retained.config')
    after = config(BUILD / '.config')
    delta = {k: {'old': before.get(k), 'new': after.get(k)}
             for k in before.keys() | after.keys() if before.get(k) != after.get(k)}
    assert delta == manifest['config_review']['delta'] and len(delta) == 5
    assert set(delta) == {'CONFIG_CC_VERSION_TEXT', 'CONFIG_CLANG_VERSION',
                          'CONFIG_AS_VERSION', 'CONFIG_LLD_VERSION', 'CONFIG_LOCALVERSION'}
    assert after['CONFIG_TOOLS_SUPPORT_RELR'] == before['CONFIG_TOOLS_SUPPORT_RELR'] == 'y'
    assert after['CONFIG_CLANG_VERSION'] == after['CONFIG_AS_VERSION'] == after['CONFIG_LLD_VERSION'] == '230103'
    assert (BUILD / 'include/config/kernel.release').read_text().strip() == '6.18.55-qc07f-encoder-query77'
    for name in ('loaded-host-ELF.json', 'consumed-kbuild-source-headers.json',
                 'additional-host-support-inputs02.json', 'inputs/host-tools.json',
                 'inputs/host-headers-and-gcc.json', 'inputs/precompile-tools-and-ELF.json'):
        verify(ROOT, load(OUT / name), name)
    verify('/usr/lib/clang/23', load(OUT / 'inputs/clang-resources.json'), 'clang_resources')
    verify(BUILD, load(OUT / 'kbuild-command-files.json'), 'kbuild_command_files')
    verify(BUILD, manifest['built_outputs'], 'built_outputs')
    commands = manifest['top_level_host_tool_commands']
    assert len(commands) == 40 and all(c['exit'] == 0 for c in commands)
    compiles = [c for c in commands if '-j4' in c['argv']]
    assert len(compiles) == 1 and compiles[0]['argv'][-2:] == ['zImage', 'vmlinux']
    assert all('modules' not in c['argv'] and 'install' not in c['argv'] for c in commands)
    for c in commands:
        assert digest(ROOT / c['log']) == c['log_pin']
    image_path = ROOT / seal['image']
    assert digest(image_path) == seal['image_pin']
    image = image_path.read_bytes()
    header = struct.unpack_from('>7I4B32s', image)
    assert header[:1] == (0x27051956,) and header[2] == 0 and header[3] == len(image) - 64
    assert header[4:6] == (0x82408000, 0x82408040) and header[7:11] == (5, 2, 2, 0)
    assert zlib.crc32(image[:4] + b'\0' * 4 + image[8:64]) == header[1]
    assert zlib.crc32(image[64:]) == header[6]
    # Independent non-reflected CRC implementation, without the author's lookup table.
    crc = 0xffffffff
    for byte in image:
        crc ^= byte << 24
        for _ in range(8):
            crc = ((crc << 1) ^ (0x04c11db7 if crc & 0x80000000 else 0)) & 0xffffffff
    crc ^= 0xffffffff
    assert crc == 0x63ea4eac
    dtb = (OLD / 'inputs/attempt45.dtb').read_bytes()
    zimage = (BUILD / 'arch/arm/boot/zImage').read_bytes()
    assert image[64:] == zimage + dtb
    assert dtb == (BASE / 'artifacts/encoder-query77-source02/inputs/attempt45.dtb').read_bytes()
    assert struct.unpack_from('>II', dtb) == (0xd00dfeed, len(dtb))
    raw = (BUILD / 'arch/arm/boot/Image').read_bytes()
    piggy = (BUILD / 'arch/arm/boot/compressed/piggy_data').read_bytes()
    assert lzma.decompress(piggy[:-4]) == raw and struct.unpack('<I', piggy[-4:])[0] == len(raw)
    initramfs = gzip.decompress((BUILD / 'usr/initramfs_inc_data').read_bytes())
    assert initramfs == (OLD / 'inputs/initramfs76.cpio').read_bytes()
    old_build = BASE / 'analysis/native-usb76-package01/build'
    assert initramfs == gzip.decompress((old_build / 'usr/initramfs_inc_data').read_bytes())
    ks, _ = symbols(BUILD / 'vmlinux')
    zs, sections = symbols(BUILD / 'arch/arm/boot/compressed/vmlinux')
    physical = lambda v: v - 0xc0000000 + 0x81800000
    text, end = physical(ks['_text']), physical(ks['_end'])
    pgdir = physical(ks['swapper_pg_dir'])
    bs, be = physical(ks['__bss_start']), physical(ks['__bss_stop'])
    assert pgdir == 0x81804000 and pgdir + 16384 == text == 0x81808000
    assert text + len(raw) == bs <= be == end < 0x82408000
    assert zs['_kernel_bss_size'] == be - bs
    assert zs['_edata'] - zs['_start'] == len(zimage)
    stack = sections['.stack']
    assert stack[5] == 4096
    growth = min(max((len(dtb) + len(dtb) // 2 + 7) & ~7, 32768), 1048576)
    live_end = 0x82408040 - zs['_start'] + stack[3] + stack[5] + growth + 65536
    assert end < 0x82408040 - zs['_start'] + zs['wont_overwrite']
    assert 0x81800000 <= pgdir < end < 0x82408000 < 0x82408000 + len(image) < live_end < 0x82800000 < 0x83800000 <= 0x84000000
    assert len(image) < 0x400000 and live_end == 0x82790578
    record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'scope': 'Parent offline integrity/source/config/package/ELF review only; not a build rerun or hardware admission',
              'counts': COUNTS, 'distinct_paths_hashed': len(CACHE),
              'exact_source_delta': changed, 'config_delta': delta,
              'whole_image_crc32_bzip2': f'{crc:08x}', 'header_crc32': f'{header[1]:08x}',
              'payload_crc32': f'{header[6]:08x}', 'image_pin': digest(image_path),
              'kernel_live': [text, end], 'kernel_bss': [bs, be],
              'staged_image': [0x82408000, 0x82408000 + len(image)],
              'decompressor_live_upper_bound': live_end, 'gap_to_cma_bytes': 0x82800000 - live_end,
              'DTB_and_embedded_initramfs_byte_identical': True,
              'compiler_or_target_execution': False, 'station_module_paired': False,
              'hardware_effect_or_boot_selected': False, 'verifier_pin': digest(Path(__file__))}
    with (DOC / 'PARENT_VERIFICATION01.json').open('x') as stream:
        json.dump(record, stream, indent=2, sort_keys=True)
        stream.write('\n')
    print('Parent offline verification PASS:', len(CACHE), 'distinct paths; exact source/config/package/ELF match; no target operation')


if __name__ == '__main__':
    main()
