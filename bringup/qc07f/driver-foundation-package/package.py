#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""FIRST packaging-only actual artifact; stdlib data reads, no build/target calls."""
import datetime
import gzip
import hashlib
import json
from pathlib import Path, PurePosixPath
import lzma
import re
import stat
import struct
import sys
import traceback
import zlib

ROOT = Path(__file__).resolve().parents[3]
APP = Path(__file__).resolve().parent
DOC = ROOT / 'docs/qualification-plans/driver-foundation-package-20261024'
W = ROOT / 'work/lightbulb-camera/driver-foundation-20261024'
B = W / 'build'
OUT = ROOT / 'work/lightbulb-camera/driver-foundation-package-20261024'
OLD_DOC = ROOT / 'docs/qualification-plans/driver-foundation-image-20261024'
REVIEW = ROOT / 'docs/qualification-plans/driver-foundation-build-review-20261024/REVIEW.md'
REVIEW_SHA = '592d780b157c9426293f7295d321f5a2bacb728886b4c8600f273d448ee5aeef'
LOAD, ENTRY, SPLIT = 0x82408000, 0x82408040, 0x82800000
START = datetime.datetime.now(datetime.timezone.utc).isoformat()


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def pin(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1048576), b''):
            h.update(block)
    return dict(bytes=path.stat().st_size, sha256=h.hexdigest())


def write(path, obj):
    with path.open('x') as f:
        json.dump(obj, f, indent=2, sort_keys=True)
        f.write('\n')


def exact(path, expected):
    require(path.is_file() and not path.is_symlink() and pin(path) == expected,
            'input pin mismatch: ' + str(path))


class ELF:
    """Keep indexed symbols/sections; PT_LOAD LMA mapping, never local-name dicts."""
    def __init__(self, path):
        self.data = path.read_bytes()
        h = struct.unpack_from('<16sHHIIIIIHHHHHH', self.data)
        require(h[0][:7] == b'\x7fELF\x01\x01\x01' and h[2] == 40 and h[1] in (1, 2), 'ARM32 LE ELF')
        require(h[8] == 52 and h[11] == 40 and h[6] + h[11] * h[12] <= len(self.data), 'ELF section table')
        self.sections = [dict(zip(('name_offset', 'type', 'flags', 'address', 'offset', 'bytes',
                                   'link', 'info', 'align', 'entry_bytes'),
                                  struct.unpack_from('<10I', self.data, h[6] + i * 40)), index=i)
                         for i in range(h[12])]
        names = self.body(self.sections[h[13]])
        for s in self.sections:
            s['name'] = self.string(names, s['name_offset'])
            if s['type'] != 8:
                require(s['offset'] + s['bytes'] <= len(self.data), 'ELF section extent')
        require(h[10] == 0 or (h[9] == 32 and h[5] + h[9] * h[10] <= len(self.data)), 'ELF program table')
        self.loads = []
        for i in range(h[10]):
            row = struct.unpack_from('<8I', self.data, h[5] + i * 32)
            if row[0] == 1:
                p = dict(zip(('type', 'offset', 'vma', 'lma', 'file_bytes', 'mem_bytes', 'flags', 'align'), row), index=i)
                require(p['offset'] + p['file_bytes'] <= len(self.data) and p['file_bytes'] <= p['mem_bytes'], 'PT_LOAD extent')
                self.loads.append(p)
        tables = [s for s in self.sections if s['type'] == 2]
        require(len(tables) == 1 and tables[0]['entry_bytes'] == 16, 'one indexed ELF symtab')
        table = tables[0]
        text, unit, self.symbols = self.body(self.sections[table['link']]), None, []
        for i in range(table['bytes'] // 16):
            n, v, size, info, other, si = struct.unpack_from('<IIIBBH', self.data, table['offset'] + 16 * i)
            name = self.string(text, n)
            if info & 15 == 4:
                unit = name
            self.symbols.append(dict(index=i, name=name, value=v, bytes=size, bind=info >> 4,
                                     type=info & 15, section=si, unit=unit))

    @staticmethod
    def string(data, offset):
        require(offset < len(data), 'ELF string offset')
        return data[offset:data.index(b'\0', offset)].decode('ascii')

    def body(self, section):
        return self.data[section['offset']:section['offset'] + section['bytes']]

    def section(self, name):
        rows = [s for s in self.sections if s['name'] == name]
        require(len(rows) == 1, 'unique ELF section: ' + name)
        return rows[0]

    def symbol(self, index, name, value=None, size=None, bind=0, kind=None, unit=None):
        s = self.symbols[index]
        require(s['name'] == name and s['bind'] == bind and s['section'] != 0
                and (value is None or s['value'] == value) and (size is None or s['bytes'] == size)
                and (kind is None or s['type'] == kind) and (unit is None or s['unit'] == unit),
                'indexed ELF symbol identity: ' + str(index) + '/' + name)
        return s

    def global_anchor(self, name):
        rows = [s for s in self.symbols if s['name'] == name and s['bind'] == 1 and s['section'] != 0]
        require(len(rows) == 1, 'unique defined GLOBAL: ' + name)
        return rows[0]

    def at(self, address, count):
        rows = [s for s in self.sections if s['type'] != 8 and s['flags'] & 2
                and s['address'] <= address and address + count <= s['address'] + s['bytes']]
        require(len(rows) == 1, 'file-backed VMA extent')
        s = rows[0]
        offset = s['offset'] + address - s['address']
        return self.data[offset:offset + count]

    def word(self, address):
        return struct.unpack('<I', self.at(address, 4))[0]

    def cstring(self, address):
        rows = [s for s in self.sections if s['type'] != 8 and s['flags'] & 2
                and s['address'] <= address < s['address'] + s['bytes']]
        require(len(rows) == 1, 'string VMA')
        count = min(256, rows[0]['address'] + rows[0]['bytes'] - address)
        data = self.at(address, count)
        return data[:data.index(b'\0')].decode('ascii')

    def associate(self, image, base):
        result = []
        for s in self.sections:
            if not s['flags'] & 2 or s['type'] == 8 or not s['bytes']:
                continue
            loads = [p for p in self.loads if p['offset'] <= s['offset']
                     and s['offset'] + s['bytes'] <= p['offset'] + p['file_bytes']
                     and p['vma'] + s['offset'] - p['offset'] == s['address']]
            require(len(loads) == 1, 'actual containing PT_LOAD: ' + s['name'])
            p = loads[0]
            lma = p['lma'] + s['offset'] - p['offset']
            offset = lma - base
            require(0 <= offset and offset + s['bytes'] <= len(image)
                    and self.body(s) == image[offset:offset + s['bytes']], 'LMA/image bytes: ' + s['name'])
            result.append(dict(section_index=s['index'], section=s['name'], VMA=s['address'], LMA=lma,
                               PT_LOAD_index=p['index'], image_offset=offset, bytes=s['bytes']))
        require(result, 'allocated file-backed section association empty')
        return result


def archive(data):
    """Header/path graph first; payloads retained as offsets, never extracted."""
    rows, pos = {}, 0
    regular = {'init', 'hgics.ko', 'bin/rtspd', 'bin/busybox', 'bin/wpa_supplicant',
               'bin/wpa_cli', 'bin/udhcpc.script', 'bin/qc07f-dhcp-script-bridge'}
    directories = {'dev', 'proc', 'sys', 'bin', 'run', 'etc', 'tmp'}
    while True:
        require(data[pos:pos + 6] == b'070701' and pos + 110 <= len(data), 'newc header')
        w = [int(data[pos + 6 + i:pos + 14 + i], 16) for i in range(0, 104, 8)]
        raw = data[pos + 110:pos + 110 + w[11]]
        require(raw.endswith(b'\0') and b'\0' not in raw[:-1], 'newc name')
        name = raw[:-1].decode('ascii')
        p = PurePosixPath(name)
        require(name and str(p) == name and not p.is_absolute() and '..' not in p.parts
                and name not in rows and w[4] == 1, 'newc path/hardlink')
        start = (pos + 110 + w[11] + 3) & ~3
        end = start + w[6]
        after = (end + 3) & ~3
        require(after <= len(data), 'newc extent')
        r = dict(words=w, start=pos, end=after, body_start=start, body_end=end)
        if name == 'TRAILER!!!':
            require(w[1] == w[6] == 0, 'newc trailer')
        elif stat.S_ISREG(w[1]):
            require(name in regular, 'unselected/private payload path')
        elif stat.S_ISDIR(w[1]):
            require(name in directories and w[6] == 0, 'newc directory')
        elif stat.S_ISLNK(w[1]):
            require(p.parent == PurePosixPath('bin') and data[start:end] == b'busybox', 'newc link')
            r['link'] = 'bin/busybox'
        else:
            require(name == 'dev/console' and stat.S_ISCHR(w[1]) and w[6] == 0 and w[9:11] == [5, 1], 'newc special')
        rows[name] = r
        pos = after
        if name == 'TRAILER!!!':
            require(not any(data[pos:]), 'newc nonzero trailing data')
            break
    for name, r in rows.items():
        for parent in PurePosixPath(name).parents:
            if str(parent) != '.':
                require(str(parent) in rows and stat.S_ISDIR(rows[str(parent)]['words'][1]), 'newc ancestor')
        if 'link' in r:
            require(r['link'] in rows and stat.S_ISREG(rows[r['link']]['words'][1]), 'newc dangling link')
    return rows, len(data) - pos


def config(data):
    result = {}
    for line in data.decode().splitlines():
        m = re.fullmatch(r'(CONFIG_\w+)=(.*)', line)
        n = re.fullmatch(r'# (CONFIG_\w+) is not set', line)
        if m:
            result[m[1]] = m[2]
        elif n:
            result[n[1]] = 'n'
    return result


def decode_header(data):
    require(len(data) >= 64, 'legacy header extent')
    magic, hc, stamp, size, load, entry, dc, os_, arch, kind, comp, name = struct.unpack_from('>7I4B32s', data)
    require(magic == 0x27051956 and len(data) == size + 64, 'legacy length/magic')
    require(zlib.crc32(data[:4] + b'\0' * 4 + data[8:64]) & 0xffffffff == hc, 'header IEEE CRC')
    require(zlib.crc32(data[64:]) & 0xffffffff == dc, 'payload IEEE CRC')
    return dict(load=load, entry=entry, timestamp=stamp, payload_bytes=size, header_bytes=64,
                header_CRC32_IEEE=f'{hc:08x}', data_CRC32_IEEE=f'{dc:08x}', OS=os_, arch=arch,
                type=kind, outer_compression=comp, name=name.rstrip(b'\0').decode('ascii'))


def bzip2_crc(data):
    # CRC-32/BZIP2: non-reflected 04c11db7, init/xorout ffffffff; NOT zlib IEEE.
    table = []
    for byte in range(256):
        n = byte << 24
        for _ in range(8):
            n = ((n << 1) ^ (0x04c11db7 if n & 0x80000000 else 0)) & 0xffffffff
        table.append(n)
    value = 0xffffffff
    for byte in data:
        value = ((value << 8) & 0xffffffff) ^ table[(value >> 24) ^ byte]
    return value ^ 0xffffffff


def main():
    (OUT / 'images').mkdir()
    (OUT / 'inputs').mkdir()
    exact(REVIEW, dict(bytes=REVIEW.stat().st_size, sha256=REVIEW_SHA))
    exact(OLD_DOC / 'BUILD_MANIFEST.json', dict(bytes=36507, sha256='41ac7af70b87453b9304b653ea07ba9bb07d0ba2286db33b61ef42676b7f84a3'))
    m = json.loads((OLD_DOC / 'BUILD_MANIFEST.json').read_text())
    require(m['status'] == 'FAILED_PRESERVED_POSTBUILD_SYMBOL_ASSERTION_NO_PACKAGE', 'original failure status')
    selected = {B / name: value for name, value in m['outputs'].items()}
    selected.update({ROOT / name: value for name, value in m['input_and_failure_metadata_pins'].items()})
    selected[W / 'linux/arch/arm/mach-ak3918ev200/ptz.c'] = m['PTZ_actual_source']
    for command in m['commands']:
        selected[ROOT / command['log']] = command['log_pin']
    delivery = json.loads((OLD_DOC / 'DELIVERY.json').read_text())
    require(pin(OLD_DOC / 'DELIVERY.json')['sha256'] ==
            'c9efd2a48f7b06320a87cedcfd825ed567b0373116e3a7e6abc41067b99cd512', 'original delivery pin')
    selected.update({ROOT / name: value for name, value in delivery['pins'].items()})
    for p, value in selected.items():
        exact(p, value)
    # Bounded original evidence graphs; never walk/copy the complete source/build.
    roots = [ROOT / 'bringup/qc07f/driver-foundation-image', OLD_DOC, W,
             W / 'inputs', W / 'logs', W / 'failure-handback', W / 'images']
    def graph():
        result = {}
        for root in roots:
            members = {}
            for p in sorted(root.iterdir()):
                require(not p.is_symlink(), 'unexpected original evidence symlink')
                members[p.name] = 'file' if p.is_file() else 'directory'
                require(p.is_file() or p.is_dir(), 'special original evidence')
            result[str(root.relative_to(ROOT))] = members
        require(not result[str((W / 'images').relative_to(ROOT))], 'original images must remain empty')
        return result
    before_graph = graph()
    for root in roots:
        for p in root.iterdir():
            if p.is_file():
                selected.setdefault(p, pin(p))
    selected[REVIEW] = pin(REVIEW)
    selected[Path(__file__)] = pin(Path(__file__))
    selected[APP / 'NOTICES.md'] = pin(APP / 'NOTICES.md')
    selected[DOC / 'CONTRACT.md'] = pin(DOC / 'CONTRACT.md')
    for name, expected in m['licenses'].items():
        selected[W / 'linux' / name] = expected
    custody = json.loads((W / 'inputs/baseline-and-rollback.json').read_text())
    baseline_doc = ROOT / 'docs/qualification-plans/encoder-query77-build-20261022/BUILD_MANIFEST01.json'
    exact(baseline_doc, custody[str(baseline_doc.relative_to(ROOT))])
    baseline = json.loads(baseline_doc.read_text())
    old_image = ROOT / baseline['package']['image']
    old_config = ROOT / baseline['build'] / '.config'
    old_archive = ROOT / baseline['source']
    old_archive = old_archive.parent / 'inputs/initramfs76.cpio'
    selected.update({baseline_doc: pin(baseline_doc), old_image: baseline['package']['image_pin'],
                     old_config: baseline['built_outputs']['.config'], old_archive: baseline['package']['initramfs_pin']})
    depmap = json.loads((W / 'failure-handback/consumed-dependencies-final-only.json').read_text())
    head = W / 'linux/arch/arm/boot/compressed/head.S'
    selected[head] = {k: depmap[str(head)][k] for k in ('bytes', 'sha256')}
    cmdmap = json.loads((W / 'failure-handback/kbuild-command-files.json').read_text())
    headcmd = B / 'arch/arm/boot/compressed/.head.o.cmd'
    selected[headcmd] = cmdmap[str(headcmd.relative_to(B))]
    for p, expected in selected.items():
        exact(p, expected)
    write(OUT / 'inputs/HELD_PINS.json', {str(p.relative_to(ROOT)): v for p, v in selected.items()})
    write(OUT / 'inputs/ORIGINAL_GRAPH.json', before_graph)
    for p in (Path(__file__), APP / 'NOTICES.md', DOC / 'CONTRACT.md'):
        with (OUT / 'inputs' / p.name).open('xb') as f:
            f.write(p.read_bytes())

    k = ELF(B / 'vmlinux')
    obj = ELF(B / 'arch/arm/mach-ak3918ev200/ptz.o')
    c = ELF(B / 'arch/arm/boot/compressed/vmlinux')
    # Exact indexed identities from the pinned independent review, not prefixes.
    identities = [(1578, 'ptz.c', 0, 0, 4), (1579, 'ak_ptz_driver_init', 0xc068f254, 16, 2),
                  (1582, 'ak_ptz_driver', 0xc077ef90, 104, 1), (1586, 'ak_ptz_probe', 0xc0028eb0, 236, 2),
                  (1589, 'ak_ptz_group', 0xc05656b0, 24, 1), (1590, 'pulse_store', 0xc0028f9c, 2104, 2),
                  (1592, 'ak_ptz_off', 0xc00297d4, 120, 2), (1596, 'ak_ptz_phases', 0xc062d248, 4, 1),
                  (1605, 'state_show', 0xc002984c, 220, 2),
                  (1607, '__initcall__kmod_ptz__161_233_ak_ptz_driver_init6', 0xc06b2ae4, 4, 1),
                  (1608, 'ak_ptz_attrs', 0xc077eff8, 12, 1), (1609, 'dev_attr_pulse', 0xc077f004, 16, 1),
                  (1610, 'dev_attr_state', 0xc077f020, 16, 1)]
    checked = [k.symbol(i, n, v, z, kind=t, unit='ptz.c') for i, n, v, z, t in identities]
    for oi, ki in ((12, 1586), (16, 1590), (18, 1592), (31, 1605)):
        s = k.symbols[ki]
        obj.symbol(oi, s['name'], size=s['bytes'], kind=2, unit='ptz.c')
    holds = [(1595, 21, 0), (1597, 23, 1), (1599, 25, 3), (1598, 24, 4),
             (1600, 26, 5), (1601, 27, 6), (1603, 29, 7), (1602, 28, 8)]
    hold_rows = []
    for offset, (ki, oi, field) in enumerate(holds):
        name, address = 'ak_ptz_hold.' + str(field), 0xc07fb32c + offset * 4
        s = k.symbol(ki, name, address, 4, kind=1, unit='ptz.c')
        require(k.sections[s['section']]['type'] == 8, 'HOLD actual BSS')
        obj.symbol(oi, name, offset * 4, 4, kind=1, unit='ptz.c')
        hold_rows.append(s)
    require(k.word(0xc06b2ae4) == 0xc068f254 and k.word(0xc068f260) == 0xc077ef90
            and k.word(0xc068f254) == 0xe59f0004 and k.word(0xc068f258) == 0xe3a01000, 'actual initcall/driver literal/NULL module')
    branch = k.word(0xc068f25c)
    delta = (branch & 0xffffff) << 2
    if delta & 0x2000000:
        delta -= 0x4000000
    register = k.global_anchor('__platform_driver_register')
    require(branch & 0xff000000 == 0xea000000 and 0xc068f25c + 8 + delta == register['value'], 'actual init branch target')
    require(k.word(0xc077ef90) == 0xc0028eb0 and k.word(0xc077ef90 + 44) == 0xc0565528, 'actual driver probe/match pointers')
    require(k.cstring(0xc0565528 + 64) == 'anyka,qc07f-ptz-diagnostic'
            and not any(k.at(0xc0565528 + 196, 196)), 'actual match-table/terminator')
    require(k.at(0xc0028eb0, 236).count(struct.pack('<I', 0xc05656b0)) == 1
            and k.word(0xc05656b0 + 16) == 0xc077eff8, 'probe group literal/attrs')
    require(k.at(0xc077eff8, 12) == struct.pack('<III', 0xc077f004, 0xc077f020, 0), 'actual attr array')
    require(k.word(0xc077f004 + 8) == 0 and k.word(0xc077f004 + 12) == 0xc0028f9c
            and k.word(0xc077f020 + 8) == 0xc002984c and k.word(0xc077f020 + 12) == 0, 'specific sysfs callbacks')
    require(k.cstring(k.word(0xc077f004)) == 'pulse' and k.cstring(k.word(0xc077f020)) == 'state'
            and k.at(0xc062d248, 4) == bytes((3, 6, 12, 9)), 'attribute names/phase bytes')
    ptz_proof = dict(reused='ALL95 relocation targets and seven normalized section associations from pinned independent REVIEW; NOT freshly recomputed',
                     review_pin=pin(REVIEW), source_pin=m['PTZ_actual_source'], object_pin=m['outputs']['arch/arm/mach-ak3918ev200/ptz.o'],
                     vmlinux_pin=m['outputs']['vmlinux'], fresh_indexed_identities=checked,
                     fresh_split_HOLD=hold_rows, register_target=register, fresh_callback_pointer_checks=True,
                     other_state_show_locals_preserved=[s for s in k.symbols if s['name'] == 'state_show'])

    anchors = {name: k.global_anchor(name) for name in ('_text', '_end', '__bss_start', '__bss_stop', 'swapper_pg_dir')}
    comp = {name: c.global_anchor(name) for name in ('_start', '_edata', '__bss_start', '_end', '_kernel_bss_size')}
    wont = c.symbol(11, 'wont_overwrite', 0x204, kind=0)
    require({name: row['index'] for name, row in anchors.items()} ==
            dict(_text=59734, _end=59737, __bss_start=59743, __bss_stop=59744, swapper_pg_dir=59730), 'kernel anchor indices')
    image = (B / 'arch/arm/boot/Image').read_bytes()
    zi = (B / 'arch/arm/boot/zImage').read_bytes()
    piggy = (B / 'arch/arm/boot/compressed/piggy_data').read_bytes()
    kernel_sections = k.associate(image, anchors['_text']['value'])
    compressed_sections = c.associate(zi, comp['_start']['value'])
    require(len(kernel_sections) == 23 and len(compressed_sections) == 6, 'reviewed allocated section membership')
    require(c.body(c.section('.piggydata')) == piggy and struct.unpack_from('<I', piggy, len(piggy) - 4)[0] == len(image), 'actual piggydata/LE32')
    xz = lzma.LZMADecompressor(format=lzma.FORMAT_XZ)
    require(xz.decompress(piggy[:-4]) == image and xz.eof and not xz.unused_data, 'exact XZ stream/Image')
    require(struct.unpack_from('<I', zi, 0x24)[0] == 0x016f2818, 'ARM zImage magic')
    zs, ze = struct.unpack_from('<II', zi, 0x28)
    require(zs == comp['_start']['value'] == 0 and ze == comp['_edata']['value'] == len(zi), 'zImage true extent')
    cfg, oldcfg = config((B / '.config').read_bytes()), config(old_config.read_bytes())
    cfgdelta = {n: dict(old=oldcfg.get(n), new=cfg.get(n)) for n in cfg.keys() | oldcfg.keys() if cfg.get(n) != oldcfg.get(n)}
    require(cfgdelta == m['config']['delta'] and set(cfgdelta) == {'CONFIG_LOCALVERSION', 'CONFIG_INITRAMFS_SOURCE'}
            and cfg['CONFIG_PTZ_DIAGNOSTIC_QC07F'] == 'y' and cfg['CONFIG_SOUND'] == 'n'
            and cfg['CONFIG_KERNEL_XZ'] == cfg['CONFIG_ARM_APPENDED_DTB'] == cfg['CONFIG_ARM_ATAG_DTB_COMPAT'] == 'y', 'exact config policy')
    archive_path = W / 'inputs/initramfs-driver-foundation01.cpio'
    embedded = (B / 'usr/initramfs_inc_data').read_bytes()
    fresh = archive_path.read_bytes()
    require(gzip.decompress(embedded) == fresh and k.data.count(embedded) == image.count(embedded) == 1, 'embedded GZIP/cpio/Image association')
    oldbytes = old_archive.read_bytes()
    oldrows, oldpad = archive(oldbytes)
    newrows, newpad = archive(fresh)
    removed = {'hgics.ko', 'bin/rtspd'}
    require(len(oldrows) == 50 and len(newrows) == 48 and list(newrows) == [n for n in oldrows if n not in removed], 'exact archive removals/order')
    for name, r in newrows.items():
        before = oldrows[name]
        require(fresh[r['start']:r['end']] == oldbytes[before['start']:before['end']], 'retained raw record: ' + name)
    archive_proof = dict(removed=sorted(removed), retained_raw_records=48, before_padding=oldpad, after_padding=newpad,
                         retained_types_modes_owners_links_timestamps_bodies_order_identical=True,
                         GZIP_vmlinux_offset=k.data.index(embedded), GZIP_Image_offset=image.index(embedded),
                         fresh_cpio=pin(archive_path), embedded_GZIP=pin(B / 'usr/initramfs_inc_data'))

    dtb = (W / 'inputs/attempt45.dtb').read_bytes()
    require(struct.unpack_from('>II', dtb) == (0xd00dfeed, len(dtb)), 'actual appended DTB magic/extent')
    # Exact pinned DTB's decoded40MiB/empty reservations/board5 maps are reused from review.
    require('-DMALLOC_SIZE=65536' in headcmd.read_text() and '#MALLOC_SIZE' in head.read_text(), 'compiled/source malloc bound')
    physical = lambda n: n - 0xc0000000 + 0x81800000
    text, end = physical(anchors['_text']['value']), physical(anchors['_end']['value'])
    bs, be = physical(anchors['__bss_start']['value']), physical(anchors['__bss_stop']['value'])
    pgdir = physical(anchors['swapper_pg_dir']['value'])
    require((text, end, pgdir) == (0x81808000, 0x82029be0, 0x81804000)
            and pgdir + 16384 == text and text + len(image) == bs <= be == end < LOAD, 'kernel/page table/BSS layout')
    require(comp['_kernel_bss_size']['value'] == be - bs and comp['__bss_start']['value'] == ze, 'compressed BSS/kernel association')
    stack, cbss = c.section('.stack'), c.section('.bss')
    require(stack['type'] == cbss['type'] == 8 and stack['address'] == comp['_end']['value']
            and stack['bytes'] == 4096 and cbss['address'] == ze and cbss['address'] + cbss['bytes'] == stack['address'], 'real compressed BSS/stack')
    base = ENTRY - comp['_start']['value']
    stack_end = base + stack['address'] + stack['bytes']
    growth = min(max((len(dtb) + len(dtb) // 2 + 7) & ~7, 32768), 1048576)
    aligned_dtb = (len(dtb) + 7) & ~7
    conservative_end = stack_end + growth + 65536
    adjusted_output = len(image) + max(0, comp['_kernel_bss_size']['value'] - (ze - wont['value']))
    require(text + adjusted_output <= ENTRY + wont['value'] and end < ENTRY + wont['value']
            and conservative_end < SPLIT and growth >= aligned_dtb, 'no self-relocation / scratch exceeds existing split')
    old_header = decode_header(old_image.read_bytes())
    expected_format = dict(load=LOAD, entry=ENTRY, timestamp=0, OS=5, arch=2, type=2, outer_compression=0, header_bytes=64)
    require(all(old_header[n] == v for n, v in expected_format.items()), 'old query77 FORMAT only')
    payload = zi + dtb
    header = struct.pack('>7I4B32s', 0x27051956, 0, 0, len(payload), LOAD, ENTRY,
                         zlib.crc32(payload) & 0xffffffff, 5, 2, 2, 0,
                         b'QC07F driver-foundation01 RAM'.ljust(32, b'\0'))
    header = header[:4] + struct.pack('>I', zlib.crc32(header) & 0xffffffff) + header[8:]
    data = header + payload
    require(LOAD + len(data) < SPLIT and len(data) <= 0x400000, 'actual package exceeds original stage')
    layout = dict(RAM=[0x81800000, 0x84000000], page_directory=[pgdir, text], kernel_live=[text, end],
                  Image_file=[text, text + len(image)], kernel_BSS=[bs, be], staged_UIM=[LOAD, LOAD + len(data)],
                  appended_DTB=[ENTRY + len(zi), ENTRY + len(zi) + len(dtb)], compressed_BSS=[base + ze, base + stack['address']],
                  compressed_stack=[base + stack['address'], stack_end], DTB_aligned_bytes=aligned_dtb,
                  DTB_growth_bound=growth, malloc_bytes=65536, conservative_decompressor_live_end=conservative_end,
                  actual_persistent_scratch_end=stack_end + aligned_dtb + 65536,
                  remaining_to_CMA_bytes=SPLIT - conservative_end, CMA=[SPLIT, 0x83800000],
                  relocation_needed=False, static_layout_only=True, combined_load_or_DMA_or_allocation_cap_proved=False)
    proof = dict(indexed_PTZ=ptz_proof, kernel_anchors=anchors, compressed_anchors=comp, wont_overwrite=wont,
                 kernel_LMA_sections=kernel_sections, compressed_LMA_sections=compressed_sections,
                 archive=archive_proof, config_delta=cfgdelta, layout=layout,
                 DTB_decoding='REUSED exact-pinned independent review: RAM40MiB, empty reservations, active board5 pin groups flags0',
                 sole_PTZ_source_delta='REUSED full-source independent review; no new91k census',
                 old_query77_header_FORMAT_only=old_header)
    write(OUT / 'PREPACKAGE_PROOF.json', proof)
    output = OUT / 'images/QC6DF01.UIM'
    with output.open('xb') as f:
        f.write(data)
    final = output.read_bytes()
    decoded = decode_header(final)
    require(all(decoded[n] == v for n, v in expected_format.items()) and final == data
            and final[64:64 + len(zi)] == zi and final[64 + len(zi):] == dtb, 'reread final header/payload/suffix')
    for p, expected in selected.items():
        exact(p, expected)
    require(graph() == before_graph, 'original evidence graph/membership drift')
    result = dict(status='PACKAGED_OFFLINE_NOT_BOOTED_REVIEW_REQUIRED', started_UTC=START,
                  ended_UTC=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  actual_argv=[sys.executable, *sys.argv], python_version=sys.version,
                  recipe=pin(Path(__file__)), image=str(output.relative_to(ROOT)), image_pin=pin(output),
                  header=decoded, fullimage_CRC32_BZIP2=f'{bzip2_crc(final):08x}',
                  kernel_release=m['kernel_release'], layout=layout, proof=pin(OUT / 'PREPACKAGE_PROOF.json'),
                  held_pins=pin(OUT / 'inputs/HELD_PINS.json'), original_graph=pin(OUT / 'inputs/ORIGINAL_GRAPH.json'),
                  all_selected_input_hashes_and_original_graph_before_after_unchanged=True,
                  original_images_empty=True, original_failure_status_unchanged=m['status'],
                  native_eligible=False, boot_admission=False, parent_image_selection=False,
                  independent_package_layout_review_required=True, no_build_or_target_execution=True,
                  provenance_limit_N2='Three Brotli HOST libraries1.2.0->1.2.1 in82 FINAL-ONLY records; no unchanged complete tool closure/pre-use capture/exact query77 reproduction claim',
                  build_indexed95_target_proof_reused_not_recomputed=True,
                  full_build_invocations=0, actual_package_attempts=1, retries=0)
    write(OUT / 'PACKAGE_MANIFEST.json', result)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    require(not OUT.exists() and not OUT.is_symlink(), 'fresh root required; no retry')
    OUT.mkdir()
    try:
        main()
    except BaseException as exc:
        write(OUT / 'FAILURE.json', dict(status='FAILED_PRESERVED_NO_RETRY', UTC=datetime.datetime.now(datetime.timezone.utc).isoformat(),
              error=type(exc).__name__ + ': ' + str(exc), traceback=traceback.format_exc(),
              recipe=pin(Path(__file__)), image_eligible=False, outputs_preserved=True, retry_authorized=False))
        raise
