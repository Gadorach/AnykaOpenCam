#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Read-only build metadata collection; no subprocess/runtime/client invocation."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
APP = ROOT / 'apps/qualification/encoder-query77-client'
DOC = Path(__file__).resolve().parent
OUT = APP / 'build01'


def pin(path):
    data = path.read_bytes()
    result = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    if path.is_symlink():
        result['symlink'] = os.readlink(path)
        result['resolved'] = str(path.resolve())
    return result


def main():
    report = json.loads((OUT / 'REPORT.json').read_bytes())
    inputs = json.loads((OUT / 'INPUTS.json').read_bytes())
    output_seal = json.loads((OUT / 'OUTPUT_SEAL.json').read_bytes())
    for name, expected in inputs.items():
        if pin(Path(name)) != expected:
            raise RuntimeError('input changed: ' + name)
    for name, expected in output_seal.items():
        if pin(OUT / name) != expected:
            raise RuntimeError('build output changed: ' + name)
    # Original collector only matched absolute "file=... generating link map".
    # Keep that narrow receipt intact; additionally resolve explicit loader init/
    # transfer names from the already-captured compiler logs (no ldd or /proc).
    loaded = {}
    programs = set()
    for path in sorted((OUT / 'logs').glob('*-loader.*')):
        text = path.read_text()
        for match in re.finditer(r'(?:calling init|initialize program|transferring control):\s+(\S+)', text):
            name = match.group(1)
            if not name.startswith('/'):
                if '/' in name:
                    name = str(ROOT / name)
                else:
                    name = '/usr/bin/' + name
            observed = Path(name)
            if not observed.is_file():
                raise RuntimeError('unresolved observed compiler ELF: ' + name)
            loaded[name] = pin(observed)
        for match in re.finditer(r'transferring control:\s+(\S+)', text):
            programs.add(match.group(1))
    for name in json.loads((OUT / 'LOADED_TOOL_ELF_FINAL.json').read_bytes()):
        loaded[name] = pin(Path(name))
    # Actual dynamic loader was included by "calling init". No read of host
    # status/configuration or dependency execution occurs in this collector.
    loads, stacks = {}, {}
    for arch in ('host', 'arm'):
        text = (OUT / 'logs' / (arch + '-elf.txt')).read_text()
        segments = []
        for line in text.splitlines():
            fields = line.split()
            if fields and fields[0] == 'LOAD':
                # readelf may print ELF64 program headers over two lines.
                # -W was not selected; derive ELF header bytes directly below.
                segments.append(line)
        elf = Path(report['artifacts'][arch]['path']).read_bytes()
        little = elf[5] == 1
        if not little:
            raise RuntimeError('unexpected ELF byte order')
        def integer(offset, count):
            return int.from_bytes(elf[offset:offset + count], 'little')
        is32 = elf[4] == 1
        offset = integer(28 if is32 else 32, 4 if is32 else 8)
        size = integer(42 if is32 else 54, 2)
        count = integer(44 if is32 else 56, 2)
        real = []
        for index in range(count):
            start = offset + index * size
            kind = integer(start, 4)
            if kind == 3:
                raise RuntimeError('PT_INTERP in static ELF')
            if kind != 1:
                continue
            if is32:
                address, filesz, memsz, flags = (integer(start + n, 4) for n in (8, 16, 20, 24))
            else:
                flags = integer(start + 4, 4)
                address, filesz, memsz = (integer(start + n, 8) for n in (16, 32, 40))
            if flags & 3 == 3:
                raise RuntimeError('writable executable LOAD')
            real.append({'virtual_start': address, 'file_bytes': filesz,
                         'memory_bytes': memsz, 'flags': flags})
        loads[arch] = {'segments': real, 'load_memory_bytes_sum': sum(s['memory_bytes'] for s in real),
                       'is_runtime_fit_proof': False}
        entries = []
        for line in (OUT / arch / 'encoder-query77-qualification.su').read_text().splitlines():
            location, frame, kind = line.split('\t')
            if kind != 'static':
                raise RuntimeError('unbounded authored stack record')
            entries.append({'location': location, 'frame_bytes': int(frame)})
        stacks[arch] = {'records': entries, 'largest_authored_frame': max(e['frame_bytes'] for e in entries),
                        'libc_kernel_or_total_live_stack_measured': False}
    app_sources = {str(path.relative_to(ROOT)): pin(path) for path in sorted(APP.glob('*')) if path.is_file()}
    aggregate = hashlib.sha256()
    for name, expected in app_sources.items():
        aggregate.update(name.encode() + b'\0' + bytes.fromhex(expected['sha256']))
    result = {'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'scope': 'SOURCE/compile/link/static metadata only, no tests or payload execution',
              'captured_input_paths_unchanged': len(inputs), 'build_output_seal_verified': pin(OUT / 'OUTPUT_SEAL.json'),
              'source_files': app_sources, 'source_tree_sha256': aggregate.hexdigest(),
              'source_tree_hash_method': 'sorted workspace-relative UTF-8 path + NUL + raw SHA256 bytes',
              'build_report': pin(OUT / 'REPORT.json'), 'compiler_commands_exit0': len(report['commands']),
              'loaded_tool_elf_final_pins': loaded, 'observed_transfer_names': sorted(programs),
              'loader_pins_are_final_observed_not_all_preuse': True,
              'static_loads': loads, 'authored_stack_records': stacks,
              'artifacts': report['artifacts'], 'runtime_or_tests_executed': False,
              'native_or_memory_acceptance': False}
    with (DOC / 'OFFLINE_CLOSURE01.json').open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True)
        stream.write('\n')


if __name__ == '__main__':
    main()
