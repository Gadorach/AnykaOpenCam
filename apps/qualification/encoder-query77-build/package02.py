#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh corrected packager: actual kernel XZ+size; unchanged GZIP initramfs."""
import argparse
import gzip
import json
import lzma
from pathlib import Path
import struct
import zlib

from build import LOCALVERSION, META, OLD, ROOT, Build, bzip_crc, elf, now, pin, require, write
from package import fdt, sections, uimage


def package(b):
    built = json.loads((b.out / 'BUILT.json').read_text())
    for name, value in built['outputs'].items():
        require(pin(b.build / name) == value, 'built output drift: ' + name)
    config = (b.build / '.config').read_text()
    require('CONFIG_KERNEL_XZ=y\n' in config and 'CONFIG_INITRAMFS_COMPRESSION_GZIP=y\n' in config,
            'unreviewed compression selection')
    old_image = (OLD / 'images/QC6U76.UIM').read_bytes()
    old_header = uimage(old_image)
    require(old_header['load'] == 0x82408000 and old_header['entry'] == 0x82408040 and
            [old_header[k] for k in ('timestamp', 'OS', 'arch', 'type', 'outer_compression')] == [0, 5, 2, 2, 0],
            'original package format not established')
    dtb = (b.art / 'inputs/attempt45.dtb').read_bytes()
    require(old_image[64:] == (META / 'build/arch/arm/boot/zImage').read_bytes() + dtb,
            'original zImage+DTB association differs')
    require(pin(b.art / 'inputs/attempt45.dtb')['sha256'] ==
            'd837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5', 'DTB drift')
    require(bzip_crc(old_image) == 0x58eb4a22, 'original full-image BZIP2 association differs')
    props, reserves = fdt(dtb)
    require(props['/:#address-cells'] == props['/:#size-cells'] == struct.pack('>I', 1), 'DTB cells')
    require(props['/memory@81800000:reg'] == struct.pack('>II', 0x81800000, 0x02800000), 'not measured40MiB DT')
    require(not any(k.startswith('/reserved-memory') for k in props), 'review DT reserved-memory')
    image = (b.build / 'arch/arm/boot/Image').read_bytes()
    piggy = (b.build / 'arch/arm/boot/compressed/piggy_data').read_bytes()
    require(struct.unpack_from('<I', piggy, len(piggy) - 4)[0] == len(image), 'XZ appended LE32 Image size')
    require(lzma.decompress(piggy[:-4]) == image, 'actual XZ stream not equal real Image')
    init = (OLD / 'inputs/initramfs76.cpio').read_bytes()
    require(gzip.decompress((b.build / 'usr/initramfs_inc_data').read_bytes()) == init,
            'embedded GZIP initramfs not exact old bytes')
    symbols = elf(b.build / 'vmlinux')
    compressed = elf(b.build / 'arch/arm/boot/compressed/vmlinux')
    sec = sections(b.build / 'arch/arm/boot/compressed/vmlinux')
    require('.stack' in sec, 'real decompressor stack missing')
    phys = lambda value: value - 0xc0000000 + 0x81800000
    text, end = phys(symbols['_text'][0]), phys(symbols['_end'][0])
    bss_start, bss_end = phys(symbols['__bss_start'][0]), phys(symbols['__bss_stop'][0])
    pgdir = phys(symbols['swapper_pg_dir'][0])
    require(pgdir == 0x81804000 and pgdir + 16384 == text == 0x81808000 and
            text + len(image) <= bss_start <= bss_end <= end < 0x82408000,
            'real decompressed image/BSS/page-table overlap')
    require(compressed['_kernel_bss_size'][0] == bss_end - bss_start, 'decompressor BSS association')
    zimage = (b.build / 'arch/arm/boot/zImage').read_bytes()
    require(struct.unpack_from('<I', zimage, 0x24)[0] == 0x016f2818, 'zImage magic')
    start, file_end = struct.unpack_from('<II', zimage, 0x28)
    require(file_end - start == len(zimage) and compressed['_edata'][0] - compressed['_start'][0] == len(zimage),
            'zImage/ELF boundary differs')
    dtb_growth = min(max(((len(dtb) + len(dtb) // 2 + 7) & ~7), 32768), 1048576)
    base = 0x82408040 - compressed['_start'][0]
    stack_end = base + sec['.stack']['address'] + sec['.stack']['bytes']
    live_end = stack_end + dtb_growth + 65536
    require(end < 0x82408040 + compressed['wont_overwrite'][0] and live_end < 0x82800000,
            'decompressor live scratch/DTB growth overlaps CMA')
    require(all(not (lo < live_end and pgdir < hi) for lo, hi in reserves), 'DT reserved range overlap')
    payload = zimage + dtb
    header = struct.pack('>7I4B32s', 0x27051956, 0, 0, len(payload), 0x82408000, 0x82408040,
                         zlib.crc32(payload) & 0xffffffff, 5, 2, 2, 0,
                         b'QC07F encoder-query77 RAM ONLY'.ljust(32, b'\0'))
    header = header[:4] + struct.pack('>I', zlib.crc32(header) & 0xffffffff) + header[8:]
    data = header + payload
    require(len(data) <= 0x400000 and 0x82408000 + len(data) < 0x82800000,
            'actual UIM exceeds original RAM payload/CMA boundary')
    parsed = uimage(data)
    output = b.images / 'QC6EQ77.UIM'
    with output.open('xb') as f:
        f.write(data)
    by_name = {r['path']: r for r in json.loads((b.out / 'inputs/initramfs-inventory.json').read_text())}
    result = {'ended': now(), 'status': 'RAM_ONLY_CANDIDATE_BUILT_PACKAGED_NOT_BOOTED',
              'image': str(output.relative_to(ROOT)), 'image_pin': pin(output),
              'fullimage_CRC32_BZIP2': f'{bzip_crc(data):08x}', 'header': parsed, 'old_header': old_header,
              'kernel_release': '6.18.55' + LOCALVERSION, 'internal_kernel_compression': 'XZ+LE32 size',
              'internal_initramfs_compression': 'GZIP',
              'zImage_pin': pin(b.build / 'arch/arm/boot/zImage'), 'Image_pin': pin(b.build / 'arch/arm/boot/Image'),
              'DTB_pin': pin(b.art / 'inputs/attempt45.dtb'), 'initramfs_pin': pin(OLD / 'inputs/initramfs76.cpio'),
              'embedded_init': by_name['init'], 'embedded_station76': by_name['hgics.ko'],
              'station76_paired': False, 'PLAT_02_cleared': False,
              'memory': {'RAM': [0x81800000, 0x84000000], 'decompressed_file': [text, text + len(image)],
                         'initial_page_directory': [pgdir, pgdir + 16384], 'kernel_BSS': [bss_start, bss_end],
                         'kernel_live': [text, end], 'staged_uImage': [0x82408000, 0x82408000 + len(data)],
                         'appended_DTB': [0x82408040 + len(zimage), 0x82408040 + len(zimage) + len(dtb)],
                         'DTB_growth_bound': dtb_growth, 'decompressor_stack_original_end': stack_end,
                         'decompressor_live_upper_bound': live_end, 'decompressor_malloc_bytes': 65536,
                         'decompressor_stack_section': sec['.stack'],
                         'decompressor_BSS_symbols': {k: compressed[k] for k in ('__bss_start', '_end', '_edata')},
                         'DT_reservations': reserves, 'existing_volatile_media_CMA': [0x82800000, 0x83800000],
                         'compiled_CMA_default_MiB': 0, 'combined_load_or_DMA_placement_proved': False},
              'packager_pin': pin(Path(__file__)), 'shared_inspector_pin': pin(Path(__file__).with_name('package.py')),
              'builder_pin': pin(Path(__file__).with_name('build.py')), 'argv': __import__('sys').argv,
              'target_or_test_execution': False, 'hardware_boot_authorized_by_artifact': False,
              'G2': False, 'G3': False, 'G8': False}
    write(b.out / 'PACKAGE02.json', result)
    print(json.dumps(result, indent=2))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--run', required=True)
    a = p.parse_args()
    require(a.run.isdigit() and len(a.run) == 2, 'two-digit run')
    b = Build(a.run)
    try:
        package(b)
    except BaseException as exc:
        write(b.out / 'PACKAGE02_FAILURE.json', {'ended': now(), 'error': type(exc).__name__ + ': ' + str(exc),
              'outputs_preserved': True, 'image_not_qualified': True})
        raise


if __name__ == '__main__':
    main()
