#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline tool/version/ELF custody; name avoids stdlib inspect shadowing."""
import argparse
import json
from pathlib import Path

from build import Build, elf, now, pin, require, write


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--run', required=True)
    a = p.parse_args()
    require(a.run.isdigit() and len(a.run) == 2, 'two-digit run')
    b = Build(a.run)
    tools = ('clang', 'clang++', 'ld.lld', 'llvm-as', 'llc', 'llvm-ar', 'llvm-nm',
             'llvm-objcopy', 'llvm-objdump', 'llvm-readelf', 'llvm-strip',
             'llvm-ranlib', 'llvm-size', 'llvm-strings', 'm4', 'gzip',
             'bzip2', 'perl', 'python3')
    for tool in tools:
        b.command('inspect-version-' + tool, ['/usr/bin/' + tool, '--version'], 60)
    if (b.out / 'BUILT.json').is_file():
        b.command('encoder-real-object-ELF', ['/usr/bin/llvm-readelf', '-h', '-A', '-s',
                  b.build / 'arch/arm/mach-ak3918ev200/qc07f_hwenc.o'])
        b.command('kernel-symbols', ['/usr/bin/llvm-nm', '-n', b.build / 'vmlinux'])
        b.command('kernel-size', ['/usr/bin/llvm-size', '-A', b.build / 'vmlinux'])
        b.command('encoder-disassembly', ['/usr/bin/llvm-objdump', '-dr',
                  b.build / 'arch/arm/mach-ak3918ev200/qc07f_hwenc.o'])
        symbols = elf(b.build / 'vmlinux')
        selected = {k: v for k, v in symbols.items() if k in
                    ('_text', '_end', '_edata', '_etext', '__bss_start', '__bss_stop',
                     'swapper_pg_dir', '__init_begin', '__init_end', '__initramfs_start',
                     '__initramfs_size', 'encoder_ioctl', 'encoder_open', 'encoder_release')}
        write(b.out / 'ELF_SELECTED_SYMBOLS.json', selected)
    write(b.out / 'INSPECTION.json', {'ended': now(), 'commands': b.records,
          'inspector_pin': pin(Path(__file__)), 'outputs_executed': False,
          'tests_or_target_operations': False})


if __name__ == '__main__':
    main()
