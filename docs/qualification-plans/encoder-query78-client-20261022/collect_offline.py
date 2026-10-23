#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Public SOURCE/build metadata custody only; no subprocess or candidate import."""
import ast
import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
DOC = Path(__file__).resolve().parent
APP = ROOT / 'apps/qualification/encoder-query78-client'
OLD = ROOT / 'apps/qualification/encoder-query77-client'
OLD_DOC = ROOT / 'docs/qualification-plans/encoder-query77-client-20261022'
REVIEW = ROOT / 'docs/qualification-plans/encoder-query77-client-review-20261022'
OUT = APP / 'build01'
CHANGES = [
    (b'--one-parent-admitted-query77-trial', b'--one-parent-admitted-query78-trial'),
    (b'6.18.55-qc07f-encoder-query77', b'6.18.55-qc07f-encoder-query78'),
]


def pin(path):
    data = path.read_bytes()
    item = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    if path.is_symlink():
        item.update(symlink=os.readlink(path), resolved=str(path.resolve()))
    return item


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def load(path):
    return json.loads(path.read_bytes())


def aggregate(items):
    value = hashlib.sha256()
    for name, item in sorted(items.items()):
        value.update(name.encode() + b'\0' + bytes.fromhex(item['sha256']))
    return value.hexdigest()


def sources():
    return {str(p.relative_to(ROOT)): pin(p) for p in sorted(APP.iterdir()) if p.is_file()}


def verify(items, base=ROOT):
    for name, expected in items.items():
        if pin(base / name) != expected:
            raise RuntimeError('changed captured input: ' + name)


def check_delta():
    old = (OLD / 'encoder-query77-qualification.c').read_bytes()
    new = (APP / 'encoder-query78-qualification.c').read_bytes()
    assert hashlib.sha256(old).hexdigest() == 'd132a2eeaae170586aa437a2e7a3102027d9ff656077c5102f57dd79d95268b7'
    restored = new
    for previous, current in CHANGES:
        assert old.count(previous) == new.count(current) == 1
        assert old.count(current) == new.count(previous) == 0
        restored = restored.replace(current, previous)
    assert restored == old and len(new.splitlines()) == 267
    header = APP / 'encoder-open-query.h'
    assert header.read_bytes() == (OLD / header.name).read_bytes()
    assert pin(header) == {'bytes': 2455, 'sha256': '6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37'}
    before = (OLD / 'compile_only.py').read_bytes()
    after = (APP / 'compile_only.py').read_bytes()
    assert before.replace(b'encoder-query77-client-20261022', b'encoder-query78-client-20261022').replace(b'encoder-query77-qualification', b'encoder-query78-qualification') == after
    ast.parse(after)  # Parse text only; no import/main of either builder.
    return old, new, before, after


def baseline():
    handoff = load(OLD_DOC / 'HANDOFF_SEAL01.json')
    verify(handoff['source_files'])
    assert aggregate(handoff['source_files']) == handoff['source_tree_sha256'] == '101afbb482c4562d02e58ab7c2aafed0d3cbb76a7d7b4cc36cf08e667fc80600'
    assert pin(REVIEW / 'FINAL_SEAL.json')['sha256'] == 'e05a777864ff87de8d8773caa1bc9c1d6da7581e24a4968f65dcd809818333ab'
    inputs = load(OLD / 'build01/INPUTS.json')
    verify(inputs)
    outputs = load(OLD / 'build01/OUTPUT_SEAL.json')
    verify(outputs, OLD / 'build01')
    tool_elf = load(OLD_DOC / 'OFFLINE_CLOSURE01.json')['loaded_tool_elf_final_pins']
    verify(tool_elf)
    retained = dict(inputs)
    for directory in (OLD, OLD_DOC, REVIEW):
        for path in sorted(directory.rglob('*')):
            if path.is_file():
                retained[str(path)] = pin(path)
    retained.update(tool_elf)
    return retained, {'old_build_inputs_verified': len(inputs),
                      'old_build_outputs_verified': len(outputs),
                      'old_final_tool_elf_paths_verified': len(tool_elf)}


def elf_metadata(path, arch):
    data = path.read_bytes()
    assert data[:4] == b'\x7fELF' and data[5] == 1
    is32 = data[4] == 1
    assert data[4] in (1, 2)
    def integer(offset, count):
        return int.from_bytes(data[offset:offset + count], 'little')
    assert integer(16, 2) == 2
    machine = integer(18, 2)
    assert (arch == 'arm' and is32 and machine == 40) or (arch == 'host' and not is32 and machine == 62)
    entry = integer(24, 4 if is32 else 8)
    flags = integer(36 if is32 else 48, 4)
    if arch == 'arm':
        assert flags & 0xff000000 == 0x05000000 and flags & 0x200 and not flags & 0x400
    phoff = integer(28 if is32 else 32, 4 if is32 else 8)
    phsize, phnum = integer(42 if is32 else 54, 2), integer(44 if is32 else 56, 2)
    loads, stack = [], None
    for index in range(phnum):
        start = phoff + index * phsize
        kind = integer(start, 4)
        pf = integer(start + (24 if is32 else 4), 4)
        assert kind not in (2, 3)  # No PT_DYNAMIC or PT_INTERP.
        if kind == 0x6474e551:
            assert pf == 6
            stack = {'flags': pf, 'nonexecuting': True}
        if kind == 1:
            assert pf & 3 != 3
            vaddr = integer(start + (8 if is32 else 16), 4 if is32 else 8)
            filesz = integer(start + (16 if is32 else 32), 4 if is32 else 8)
            memsz = integer(start + (20 if is32 else 40), 4 if is32 else 8)
            loads.append({'virtual_start': vaddr, 'file_bytes': filesz, 'memory_bytes': memsz, 'flags': pf})
    assert stack and any(p['flags'] & 1 and p['virtual_start'] <= entry < p['virtual_start'] + p['memory_bytes'] for p in loads)
    shoff = integer(32 if is32 else 40, 4 if is32 else 8)
    shsize, shnum = integer(46 if is32 else 58, 2), integer(48 if is32 else 60, 2)
    sections = []
    for index in range(shnum):
        start = shoff + index * shsize
        kind = integer(start + 4, 4)
        assert kind != 6
        sections.append({'type': kind, 'offset': integer(start + (16 if is32 else 24), 4 if is32 else 8),
                         'size': integer(start + (20 if is32 else 32), 4 if is32 else 8),
                         'link': integer(start + (24 if is32 else 40), 4),
                         'entsize': integer(start + (36 if is32 else 56), 4 if is32 else 8)})
    main = None
    for section in sections:
        if section['type'] != 2:
            continue
        strings = sections[section['link']]
        for offset in range(section['offset'], section['offset'] + section['size'], section['entsize']):
            name_start = strings['offset'] + integer(offset, 4)
            name = data[name_start:data.index(b'\0', name_start)].decode()
            info = data[offset + (12 if is32 else 4)]
            shndx = integer(offset + (14 if is32 else 6), 2)
            assert not (shndx == 0 and info >> 4 not in (0, 2))
            if name == 'main':
                value = integer(offset + (4 if is32 else 8), 4 if is32 else 8)
                size = integer(offset + (8 if is32 else 16), 4 if is32 else 8)
                assert shndx != 0 and size > 0 and info & 15 == 2
                assert any(p['flags'] & 1 and p['virtual_start'] <= value < p['virtual_start'] + p['memory_bytes'] for p in loads)
                main = {'value': value, 'bytes': size}
    assert main
    return {'class': 32 if is32 else 64, 'type': 'EXEC', 'machine': machine,
            'entry': entry, 'entry_flags': flags, 'main': main, 'segments': loads,
            'load_memory_bytes_sum': sum(p['memory_bytes'] for p in loads), 'GNU_stack': stack,
            'strong_undefined_symbols': 0, 'runtime_or_memory_fit_proof': False}


def after():
    pre = load(DOC / 'SOURCE_BEFORE.json')
    verify(pre['immutable_baseline_pins'])
    verify(pre['new_source_files'])
    verify(pre['new_documentation_prebuild_pins'])
    check_delta()
    report = load(OUT / 'REPORT.json')
    assert report['status'] == 'PASS' and len(report['commands']) == 36
    assert all(c['exit'] == 0 for c in report['commands'])
    assert not report['outputs_executed'] and not report['tests_executed'] and not report['native_admitted']
    inputs, outputs = load(OUT / 'INPUTS.json'), load(OUT / 'OUTPUT_SEAL.json')
    verify(inputs)
    verify(outputs, OUT)
    pre_inputs = load(OUT / 'PRE_INPUTS.json')
    assert all(inputs[name] == item for name, item in pre_inputs.items())
    loaded, transfers = {}, set()
    for path in sorted((OUT / 'logs').glob('*-loader.*')):
        text = path.read_text()
        for match in re.finditer(r'(?:calling init|initialize program|transferring control):\s+(\S+)', text):
            name = match.group(1)
            if not name.startswith('/'):
                name = str(ROOT / name) if '/' in name else '/usr/bin/' + name
            observed = Path(name)
            if not observed.is_file():
                raise RuntimeError('unresolved compiler tool ELF: ' + name)
            loaded[name] = pin(observed)
        transfers.update(re.findall(r'transferring control:\s+(\S+)', text))
    for name in load(OUT / 'LOADED_TOOL_ELF_FINAL.json'):
        loaded[name] = pin(Path(name))
    static, stacks, dependency_counts, link_inputs = {}, {}, {}, {}
    for arch in ('host', 'arm'):
        artifact = report['artifacts'][arch]
        assert pin(Path(artifact['path'])) == {k: artifact[k] for k in ('bytes', 'sha256')}
        old_artifact = load(OLD / 'build01/REPORT.json')['artifacts'][arch]
        assert artifact['sha256'] != old_artifact['sha256']
        static[arch] = elf_metadata(Path(artifact['path']), arch)
        inspection = (OUT / 'logs' / (arch + '-elf.txt')).read_text()
        if arch == 'arm':
            assert 'v5TE' in inspection and 'Version5 EABI' in inspection
        records = []
        for line in (OUT / arch / 'encoder-query78-qualification.su').read_text().splitlines():
            location, frame, kind = line.split('\t')
            assert kind == 'static'
            records.append({'location': location, 'frame_bytes': int(frame)})
        stacks[arch] = {'records': records, 'largest_authored_frame': max(r['frame_bytes'] for r in records),
                        'not_total_live_stack_or_RSS': True}
        def deps(path):
            import shlex
            return set(shlex.split(path.read_text().replace('\\\n', ' ').split(':', 1)[1]))
        consumed = deps(OUT / arch / 'pre.d')
        assert consumed == deps(OUT / arch / 'actual.d')
        for name in consumed:
            path = Path(name).absolute()
            assert pin(path) == inputs[str(path)]
            assert path.read_bytes() == (OUT / 'inputs' / path.relative_to('/')).read_bytes()
        dependency_counts[arch] = len(consumed)
        paths = set()
        text = (OUT / 'logs' / (arch + '-static-link.txt')).read_text()
        for line in text.splitlines():
            path = Path(line.strip())
            if path.is_absolute() and path.is_file() and not path.is_relative_to(OUT):
                paths.add(str(path))
        for name in re.findall(r'^LOAD (.+)$', (OUT / arch / 'link.map').read_text(), re.M):
            if name == 'linker stubs':  # BFD synthetic label, never a filesystem input.
                continue
            path = Path(name)
            assert path.is_file()
            if not path.is_relative_to(OUT):
                paths.add(str(path))
        assert all(name in inputs for name in paths)
        link_inputs[arch] = {name: inputs[name] for name in sorted(paths)}
    source = sources()
    return {'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'scope': 'UNREVIEWED SOURCE/HOST+ARM compile/static ELF custody only',
            'source_files': source, 'source_tree_sha256': aggregate(source),
            'source_tree_hash_method': 'sorted workspace-relative UTF-8 path + NUL + raw SHA256 bytes',
            'immutable_baseline_paths_unchanged': len(pre['immutable_baseline_pins']),
            'captured_input_paths_unchanged': len(inputs), 'pre_input_paths': len(pre_inputs),
            'build_output_files_verified': len(outputs), 'build_output_seal': pin(OUT / 'OUTPUT_SEAL.json'),
            'build_report': pin(OUT / 'REPORT.json'), 'compiler_tool_commands_exit0': len(report['commands']),
            'loaded_tool_elf_final_pins': loaded, 'observed_transfer_names': sorted(transfers),
            'loader_pins_final_observed_not_all_preuse': True,
            'actual_source_header_dependency_counts': dependency_counts, 'link_trace_map_inputs': link_inputs,
            'static_ELF': static, 'authored_stack_records': stacks, 'artifacts': report['artifacts'],
            'programs_tests_native_executed': False, 'embedding_image_ABI_or_memory_fit_claim': False}


def main():
    if sys.argv[1:] == ['before']:
        old, new, before_builder, after_builder = check_delta()
        retained, counts = baseline()
        for filename, previous, current in (('CLIENT_DELTA.diff', old, new),
                                             ('BUILDER_DELTA.diff', before_builder, after_builder)):
            with (DOC / filename).open('x') as stream:
                stream.writelines(difflib.unified_diff(previous.decode().splitlines(True),
                                                     current.decode().splitlines(True),
                                                     fromfile='immutable-query77-source', tofile='new-query78-source'))
        source = sources()
        docs = {str(p.relative_to(ROOT)): pin(p) for p in sorted(DOC.iterdir()) if p.is_file()}
        save(DOC / 'SOURCE_BEFORE.json', {
            'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'counts': counts, 'immutable_baseline_pins': retained,
            'new_source_files': source, 'new_source_tree_sha256': aggregate(source),
            'new_documentation_prebuild_pins': docs,
            'single_occurrence_two_literal_reverse_full_byte_identity': True,
            'header_byte_identity': True, 'builder_only_DOC_and_source_artifact_name_edits': True,
            'old_source_tree_sha256': '101afbb482c4562d02e58ab7c2aafed0d3cbb76a7d7b4cc36cf08e667fc80600',
            'old_independent_review_seal_sha256': pin(REVIEW / 'FINAL_SEAL.json')['sha256'],
            'baseline_query77_association_not_query78_kernel_ABI': True})
    elif sys.argv[1:] == ['after']:
        save(DOC / 'OFFLINE_CLOSURE01.json', after())
    elif sys.argv[1:] == ['seal']:
        # Recheck all original/source/build inputs again after the prose report.
        fresh = after()
        closure = load(DOC / 'OFFLINE_CLOSURE01.json')
        verify(closure['loaded_tool_elf_final_pins'])
        assert fresh['source_tree_sha256'] == closure['source_tree_sha256']
        save(DOC / 'HANDOFF_SEAL01.json', {
            'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'scope': 'UNREVIEWED SOURCE/build candidate, parent independent review required',
            'source_files': sources(), 'source_tree_sha256': fresh['source_tree_sha256'],
            'documentation_files': {str(p.relative_to(ROOT)): pin(p) for p in sorted(DOC.iterdir()) if p.is_file()},
            'local_build_report': pin(OUT / 'REPORT.json'), 'local_build_inputs': pin(OUT / 'INPUTS.json'),
            'local_build_output_seal': pin(OUT / 'OUTPUT_SEAL.json'),
            'unchanged_baseline_paths': fresh['immutable_baseline_paths_unchanged'],
            'unchanged_build_inputs': fresh['captured_input_paths_unchanged'],
            'verified_build_outputs': fresh['build_output_files_verified'],
            'final_tool_ELF_paths': len(closure['loaded_tool_elf_final_pins']),
            'executed_client_or_tests': False, 'native_or_image_integration_selected': False,
            'next': 'parent independent ordinary SOURCE/compile-custody review only; no automatic queue'})
    else:
        raise RuntimeError('select exactly before, after or seal (public static metadata only)')


if __name__ == '__main__':
    main()
