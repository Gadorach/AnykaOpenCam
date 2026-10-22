#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Data-only author source/build custody finalizer; no subprocess or candidate import."""
import collections
import datetime
import hashlib
import json
from pathlib import Path
import shlex
import struct

DOC = Path(__file__).resolve().parent
ROOT = DOC.parents[2]
APP = ROOT / 'apps/rtsp-auth/prestarted-owned-listener'
RUN = DOC / 'runs/build01'
SOURCE_PIN = '09325264c224a43b5664b48123bce1d05ea1269e6f209dcd2c9967424fdcba33'
CONTRACT_PIN = '07d271826d37aedaf0422878537764f78edc61a3ecfabba2b81ad6debb7ad3b9'
RECIPE = 'sha256(concat(sorted(workspace_relative_path_utf8 + NUL + raw_sha256_bytes)))'
PROJECT = ('camera_', 'candidate_', 'owned_', 'rv_', 'is_rv_', 'mbedtls_')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def pin_bytes(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def pin(path):
    return pin_bytes(path.read_bytes())


def verify(path, expected):
    actual = pin(path)
    require(all(actual[k] == expected[k] for k in ('bytes', 'sha256')),
            'recorded bytes changed: ' + str(path))
    return actual


def aggregate(entries):
    digest = hashlib.sha256()
    for name, expected in sorted(entries.items()):
        digest.update(name.encode() + b'\0' + bytes.fromhex(expected['sha256']))
    return digest.hexdigest()


def create(name, value):
    with (DOC / name).open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def span(data, start, end):
    return data[data.index(start):data.index(end)]


def elf_data(data):
    """Inspect little-endian ELF32/64 symbol/relocation tables as inert bytes."""
    require(data[:4] == b'\x7fELF' and data[4] in (1, 2) and data[5] == 1,
            'unexpected ELF encoding')
    wide = data[4] == 2
    header = struct.unpack_from('<HHIQQQIHHHHHH' if wide else '<HHIIIIIHHHHHH', data, 16)
    require(header[0] == 1, 'not ET_REL')
    section_format = '<IIQQQQIIQQ' if wide else '<IIIIIIIIII'
    require(header[10] == struct.calcsize(section_format), 'section header size')
    sections = [struct.unpack_from(section_format, data, header[5] + i * header[10])
                for i in range(header[11])]
    symbols, definitions, undefined = {}, [], []
    for index, section in enumerate(sections):
        if section[1] != 2:  # SHT_SYMTAB
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4] + strings[5]]
        parsed = []
        symbol_format = '<IBBHQQ' if wide else '<IIIBBH'
        require(section[9] == struct.calcsize(symbol_format), 'symbol entry size')
        for offset in range(section[4], section[4] + section[5], section[9]):
            record = struct.unpack_from(symbol_format, data, offset)
            name = names[record[0]:].split(b'\0', 1)[0].decode()
            info, shndx = (record[1], record[3]) if wide else (record[3], record[5])
            parsed.append((name, shndx, info))
            if name and not shndx:
                undefined.append(name)  # nm -u includes GC-localized undefined names.
            elif name and info >> 4 in (1, 2):
                definitions.append(name)
        symbols[index] = parsed
    referenced = set()
    for section in sections:
        if section[1] not in (4, 9):  # SHT_RELA, SHT_REL
            continue
        require(section[6] in symbols and section[9] > 0, 'relocation link/size')
        for offset in range(section[4], section[4] + section[5], section[9]):
            info = struct.unpack_from('<QQ' if wide else '<II', data, offset)[1]
            name, shndx, _ = symbols[section[6]][info >> (32 if wide else 8)]
            if name and shndx == 0:
                referenced.add(name)
    return {'bits': 64 if wide else 32, 'machine': header[1],
            'definitions': definitions, 'undefined': sorted(undefined),
            'referenced_undefined': sorted(referenced)}


def archive_data(data):
    """Read standard GNU ar member bytes; never load or execute any member."""
    require(data[:8] == b'!<arch>\n', 'not a regular ar archive')
    offset, names, members = 8, b'', []
    while offset < len(data):
        header = data[offset:offset + 60]
        require(len(header) == 60 and header[58:] == b'`\n', 'ar header')
        size, name = int(header[48:58]), header[:16].decode().strip()
        payload = data[offset + 60:offset + 60 + size]
        require(len(payload) == size, 'ar member truncated')
        if name == '//':
            names = payload
        elif name not in ('/', '/SYM64/'):
            if name.startswith('/') and name[1:].isdigit():
                name = names[int(name[1:]):].split(b'/\n', 1)[0].decode()
            else:
                name = name.removesuffix('/')
            members.append((name, payload))
        offset += 60 + size + size % 2
    require(offset == len(data), 'ar trailing bytes')
    return members


def main():
    for name in ('BUILD_SEAL01.json', 'SOURCE_SEAL01.json'):
        require(not (DOC / name).exists(), 'exclusive finalizer output already exists: ' + name)
    initial_finalizer = pin(DOC / 'seal.py')
    require(initial_finalizer['sha256'] ==
            'fc183bc0841b2887a773ad9aa7ad6a4ccd05d3baf1c97a693e7aa5a04809795b',
            'initial finalizer history changed')
    failure = json.loads((DOC / 'SEAL_FAILURE01.json').read_bytes())
    require(failure['initial_finalizer_sha256'] == initial_finalizer['sha256'] and
            failure['exit'] == 1 and not failure['partial_final_seals_written'] and
            not failure['production_or_build_source_changed'] and not failure['policy_rejection_or_reroute'],
            'initial static failure receipt changed')
    second_finalizer = pin(DOC / 'seal02.py')
    require(second_finalizer['sha256'] ==
            '9232c5c5f2580bc3fe38a7a71b9622f1c3fedcf3edab5b340d87882c1a49193d',
            'second finalizer history changed')
    second_failure = json.loads((DOC / 'SEAL_FAILURE02.json').read_bytes())
    require(second_failure['initial_finalizer_sha256'] == second_finalizer['sha256'] and
            second_failure['exit'] == 1 and not second_failure['partial_final_seals_written'] and
            not second_failure['production_or_build_source_changed'] and not second_failure['policy_rejection_or_reroute'],
            'second static failure receipt changed')
    report = json.loads((RUN / 'REPORT.json').read_bytes())
    require(report['status'] == 'HOST_ARM_CHANGED_UNITS_COMPILE_ARCHIVE_REL_PASS_NOT_RUN',
            'missing successful compile-only report')
    require(not report['programs_tests_generators_listeners_executed'] and
            not report['native_or_composite_acceptance'], 'scope flags changed')
    counts = {'inputs': 337, 'frozen': 2669, 'closure': 308, 'artifacts': 34}
    verified = {}
    for table in (*counts, 'toolchain_pins'):
        if table in counts:
            require(len(report[table]) == counts[table], 'record count changed: ' + table)
        for name, expected in report[table].items():
            verified[name] = verify(Path(name), expected)
    for group in report['tools'].values():
        for expected in group.values():
            verify(Path(expected['path']).resolve(), expected)
    for alias, resolution in report['frozen_tool_aliases'].items():
        require(str(Path(alias).resolve()) == resolution, 'compiler alias resolution changed')
    for name, expected in report['closure'].items():
        verify(RUN / 'dependencies' / Path(name).relative_to('/'), expected)

    # Reconstruct each input snapshot destination without executing its recipe.
    copies = {}
    for name, expected in report['inputs'].items():
        path = Path(name)
        if path.is_relative_to(APP):
            copy = RUN / 'source/app' / path.relative_to(APP)
        elif path.is_relative_to(DOC / 'baseline-provenance'):
            copy = RUN / 'provenance' / path.relative_to(DOC / 'baseline-provenance')
        elif path == DOC / 'COMPOSITION_CONTRACT.md':
            copy = RUN / 'provenance/COMPOSITION_CONTRACT.md'
        elif path.is_relative_to(ROOT / 'apps/auth-web/service/vendor/mbedtls'):
            copy = RUN / 'source/vendor' / path.relative_to(ROOT / 'apps/auth-web/service/vendor/mbedtls')
        elif path.is_relative_to(ROOT / 'framework'):
            copy = RUN / 'source/framework' / path.relative_to(ROOT / 'framework')
        elif path.is_relative_to(ROOT / 'apps/rtsp-auth/isolated-verifier'):
            copy = RUN / 'source/verifier' / path.relative_to(ROOT / 'apps/rtsp-auth/isolated-verifier')
        else:
            raise RuntimeError('unmapped input snapshot: ' + name)
        verify(copy, expected)
        copies[str(copy)] = expected
    license_origin = DOC / 'baseline-listener/frozen/runtime-licenses'
    license_files = {}
    for path in sorted(license_origin.rglob('*')):
        if path.is_file():
            relative = path.relative_to(license_origin)
            expected = pin(path)
            verify(RUN / 'source/runtime-licenses' / relative, expected)
            verify(ROOT / 'apps/rtsp-auth/owned-listener-candidate/frozen/runtime-licenses' / relative, expected)
            license_files[relative.as_posix()] = expected
    require(license_files, 'missing retained runtime licenses')
    source_snapshot_files = {str(p) for p in (RUN / 'source').rglob('*') if p.is_file()}
    expected_source = {p for p in copies if Path(p).is_relative_to(RUN / 'source')}
    expected_source |= {str(RUN / 'source/runtime-licenses' / p) for p in license_files}
    require(source_snapshot_files == expected_source, 'unmapped generated source file')

    commands = report['commands']
    require(len(commands) == 58, 'command count changed')
    compiles = [entry for entry in commands if '-c' in entry['argv']]
    require(len(compiles) == 30, 'not thirty real C compiles')
    dependency_paths = set()
    for entry in commands:
        verify(Path(entry['log']), entry['log_pin'])
        require(entry['cwd'] == str(ROOT), 'command working directory changed')
        require(entry['exit'] == (1 if Path(entry['argv'][0]).name == 'diff' else 0),
                'unexpected command status')
    for entry in compiles:
        argv = entry['argv']
        require(not Path(entry['log']).read_bytes(), 'nonempty compile diagnostic log')
        source = Path(argv[argv.index('-c') + 1])
        obj = Path(argv[argv.index('-o') + 1])
        require(source.is_relative_to(RUN / 'source') and str(source) in copies,
                'non-snapshotted real source')
        dep = Path(argv[argv.index('-MF') + 1])
        require(dep == obj.with_suffix('.d'), 'dependency destination changed')
        words = shlex.split(dep.read_text().replace('\\\n', '').split(':', 1)[1])
        require(str(source) in words, 'real unit absent from dependency closure')
        dependency_paths.update(words)
        require(str(obj) in report['artifacts'], 'unrecorded compiled object')
        require(not any(arg.startswith('-D') and ('FIXTURE' in arg or 'RV_TEST' in arg) for arg in argv),
                'fixture/test compile macro')
    require(dependency_paths == set(report['closure']), 'actual per-unit dependency set changed')

    source_files, changed, unchanged, added = {}, [], [], []
    for path in sorted(APP.iterdir()):
        require(path.is_file() and not path.is_symlink(), 'unexpected app entry')
        expected = pin(path)
        verify(RUN / 'source/app' / path.name, expected)
        source_files[path.relative_to(ROOT).as_posix()] = expected
        original = DOC / 'baseline-backend-v2' / path.name
        (added if not original.exists() else unchanged if pin(original) == expected else changed).append(path.name)
    require(len(source_files) == 14 and aggregate(source_files) == SOURCE_PIN, 'candidate source changed')
    require(changed == ['README.md', 'build.py', 'candidate_auth.h', 'rtsp_server_candidate.c'] and
            added == ['listener_custody.c', 'listener_custody.h', 'prestarted_owned_listener_v1.h'] and
            len(unchanged) == 7, 'unexpected source delta')
    contract = pin(DOC / 'COMPOSITION_CONTRACT.md')
    require(contract['sha256'] == CONTRACT_PIN, 'pre-code contract changed')
    verify(RUN / 'provenance/COMPOSITION_CONTRACT.md', contract)
    require((APP / 'listener_custody.c').read_bytes() ==
            (DOC / 'baseline-listener/listener_custody.c').read_bytes(), 'helper bodies changed')
    require((APP / 'listener_custody.h').read_bytes() ==
            (DOC / 'baseline-listener/listener_custody.h').read_bytes().replace(
                b'#include "owned_listener_v1.h"', b'#include "prestarted_owned_listener_v1.h"'),
            'listener header changed beyond include')
    old = (DOC / 'baseline-backend-v2/rtsp_server_candidate.c').read_bytes()
    new = (APP / 'rtsp_server_candidate.c').read_bytes()
    prefix = span(new, b'// SPDX', b'static int serve_prestarted_body(')
    prefix = prefix.replace(b'#include "listener_custody.h"\n', b'').replace(
        b'/* Serialized synchronous entry only, not a thread lock or ownership grant. */\n'
        b'static int owned_entry_active;\n', b'')
    require(prefix == span(old, b'// SPDX', b'static int serve_prestarted_body('),
            'original server helpers changed')
    end = b'else if(!retirement_failed){int retired=idle_checked(p);if(retired!=CAMERA_OK)result=retired;}'
    old_loop = span(old, b'while(!*o->stop){', end) + end
    new_loop = span(new, b'while(!*o->stop){', end) + end
    require(old_loop == new_loop, 'original loop/client/source cleanup changed')
    header_old = (DOC / 'baseline-backend-v2/candidate_auth.h').read_bytes()
    header_new = (APP / 'candidate_auth.h').read_bytes()
    require(header_old[:header_old.index(b'/* Consumes one serve attempt.')] ==
            header_new[:header_new.index(b'/* The ONLY serve entry')] and
            header_old[header_old.index(b'/* Explicit one attempt,'):] ==
            header_new[header_new.index(b'/* Explicit one attempt,'):], 'backend layout/API drift')
    preserved = {'server_helpers_without_include_and_entry_guard': pin_bytes(prefix),
                 'real_loop_client_hub_source_cleanup': pin_bytes(new_loop),
                 'listener_helper': pin(APP / 'listener_custody.c')}

    # Verify one-copy archive bytes, actual linked object lists, maps and ELF data.
    inventory = {}
    required = ('candidate_prestarted_start', 'candidate_prestarted_finish',
                'camera_rtsp_prestarted_owned_listener_serve_v1', 'candidate_backend_poll',
                'candidate_peer_open', 'candidate_peer_close', 'rv_start', 'rv_stop', 'rv_destroy',
                'is_rv_running', 'is_rv_ipc_close_outcome', 'is_rv_context_bytes',
                'owned_listener_observe', 'owned_listener_move', 'owned_listener_close_once')
    forbidden = ('main', 'camera_rtsp_isolated_candidate_serve', 'camera_rtsp_prestarted_candidate_serve',
                 'camera_rtsp_owned_listener_serve_v1', 'candidate_backend_init',
                 'candidate_backend_destroy', 'candidate_backend_finish')
    for label, bits, machine in (('host', 64, 62), ('arm', 32, 40)):
        platform = [entry for entry in compiles if Path(entry['log']).name.startswith(label + '-')]
        require(len(platform) == 15, 'platform compile count')
        objects = [Path(e['argv'][e['argv'].index('-o') + 1]) for e in platform]
        require(len(set(objects)) == 15, 'duplicate object')
        expected_stems = ('candidate_auth', 'rtsp_server_candidate', 'verifier_adapter_overlay',
                          'listener_custody', 'ipc', 'rtsp_digest_wire', 'rtsp_request', 'rtsp_input',
                          'backend', 'packet_hub', 'producer', 'rtp_h264', 'sha256',
                          'constant_time', 'platform_util')
        require([Path(e['argv'][e['argv'].index('-c') + 1]).stem for e in platform] ==
                list(expected_stems), 'not the fifteen actual selected units')
        archive = RUN / ('libprestarted-owned-listener-' + label + '.a')
        members = archive_data(archive.read_bytes())
        require([name for name, _ in members] == [p.name for p in objects], 'archive member set/order')
        all_project_definitions = []
        for (name, data), obj in zip(members, objects):
            require(data == obj.read_bytes(), 'archive did not contain actual rebuilt object: ' + name)
            inspected = elf_data(data)
            require(inspected['bits'] == bits and inspected['machine'] == machine, 'object architecture')
            all_project_definitions += [n for n in inspected['definitions'] if n.startswith(PROJECT)]
        require(not [n for n, count in collections.Counter(all_project_definitions).items() if count != 1],
                'duplicate real project definitions across archive')
        archive_entry = next(e for e in commands if Path(e['log']).name == label + '-archive.txt')
        require(archive_entry['argv'][1:] == ['rcsD', str(archive), *map(str, objects)], 'archive arguments')
        link = next(e for e in commands if Path(e['log']).name == label + '-relocatable-link.txt')
        linked_objects = [arg for arg in link['argv'] if arg in report['artifacts'] and '/objects/' in arg]
        require(linked_objects == list(map(str, objects)) and
                not any(arg.endswith('.a') for arg in link['argv']) and
                all(arg in link['argv'] for arg in ('-nostdlib', '-r', '-Wl,--gc-sections')),
                'not single actual object composition')
        map_path = RUN / (label + '.map')
        loads = [line.removeprefix('LOAD ') for line in map_path.read_text().splitlines() if line.startswith('LOAD ')]
        pseudo_loads = ['linker stubs'] if label == 'arm' else []
        require(loads == [*map(str, objects), *pseudo_loads], 'map actual object/pseudo load inventory')
        reloc = RUN / ('prestarted-owned-listener-' + label + '-NOT-EXECUTABLE.o')
        require(link['argv'][link['argv'].index('-o') + 1] == str(reloc), 'REL output changed')
        inspected = elf_data(reloc.read_bytes())
        require(inspected['bits'] == bits and inspected['machine'] == machine, 'REL architecture')
        definitions = collections.Counter(inspected['definitions'])
        require(all(definitions[symbol] == 1 for symbol in required) and
                not any(definitions[symbol] for symbol in forbidden), 'missing/legacy actual API')
        require(not any(name.startswith(PROJECT) for name in inspected['referenced_undefined']),
                'unresolved actual project relocation')
        undefined = report['undefined'][label]
        require(inspected['undefined'] == sorted(undefined['symbol_table']) and
                inspected['referenced_undefined'] == sorted(undefined['referenced_by_relocation']),
                'ELF and recorded relocation/undefined inventory differ')
        unused = sorted(set(inspected['undefined']) - set(inspected['referenced_undefined']))
        require(unused == sorted(undefined['unreferenced_symbol_table_only']) ==
                ['camera_rtsp_digest_close', 'camera_rtsp_digest_issue', 'camera_rtsp_digest_verify'],
                'unused symbol-table bookkeeping changed')
        defined_log = (RUN / 'logs' / (label + '-defined.txt')).read_text().splitlines()
        require(sorted(line.split()[-1] for line in defined_log if line.strip()) == sorted(definitions.elements()),
                'ELF and defined-symbol tool inventory differ')
        inventory[label] = {'archive': pin(archive), 'member_count': len(members),
                            'member_pins': {name: pin_bytes(data) for name, data in members},
                            'project_definitions_once': sorted(all_project_definitions),
                            'REL': pin(reloc), 'ELF': inspected, 'unused_undefined_names': unused,
                            'link_map': pin(map_path), 'map_actual_input_loads': list(map(str, objects)),
                            'map_linker_generated_pseudo_loads': pseudo_loads}

    utc = datetime.datetime.now(datetime.timezone.utc).isoformat()
    create('BUILD_SEAL01.json', {
        'UTC': utc, 'scope': 'AUTHOR_STATIC_SOURCE_COMPOSITION_AND_COMPILE_CUSTODY_ONLY',
        'command': 'python3 -B docs/qualification-plans/rtsp-prestarted-owned-listener-20261022/seal03.py',
        'preserved_initial_finalizer': initial_finalizer,
        'preserved_static_failure': pin(DOC / 'SEAL_FAILURE01.json'),
        'preserved_second_finalizer': second_finalizer,
        'preserved_second_static_failure': pin(DOC / 'SEAL_FAILURE02.json'),
        'report': pin(RUN / 'REPORT.json'), 'commands': len(commands),
        'successful_compile_tool_archive_REL_commands': 52, 'expected_diff_exit1': 6,
        'actual_C_compiles': len(compiles), 'compile_diagnostic_bytes': 0,
        'changed_from_v2': changed, 'unchanged_from_v2': unchanged, 'added': added,
        'preserved_spans': preserved, 'record_counts': counts,
        'verified_recorded_pins': verified, 'input_snapshot_pins': copies,
        'runtime_license_pins': license_files, 'tools': report['tools'],
        'frozen_tool_aliases': report['frozen_tool_aliases'], 'platform_inventory': inventory,
        'artifacts': report['artifacts'], 'source_changed_after_compile': False,
        'candidate_output_or_test_executed': False, 'target_or_private_IO': False,
        'independent_composition_or_native_or_product_acceptance': False,
    })
    evidence = {}
    for path in sorted(DOC.rglob('*')):
        require(not path.is_symlink(), 'evidence symlink refused')
        if path.is_file() and path != DOC / 'SOURCE_SEAL01.json':
            evidence[path.relative_to(ROOT).as_posix()] = pin(path)
    create('SOURCE_SEAL01.json', {
        'UTC': utc, 'scope': 'ONE_UNREVIEWED_PRESTARTED_OWNED_LISTENER_SOURCE_COMPOSITION_CANDIDATE',
        'hash_method': RECIPE, 'source_tree_sha256': aggregate(source_files),
        'source_count': len(source_files), 'source_files': source_files,
        'evidence_count': len(evidence), 'evidence_aggregate': aggregate(evidence),
        'evidence_pins': evidence, 'pre_code_contract': contract,
        'independent_composition_runtime_native_product_acceptance': False,
        'automatic_review_test_native_next_queue': False,
    })
    print(json.dumps({'source_tree_sha256': aggregate(source_files), 'source_files': len(source_files),
                      'evidence_count': len(evidence), 'verified_unique_recorded_pins': len(verified),
                      'BUILD_SEAL01.json': pin(DOC / 'BUILD_SEAL01.json'),
                      'SOURCE_SEAL01.json': pin(DOC / 'SOURCE_SEAL01.json')}, indent=2))


if __name__ == '__main__':
    main()
