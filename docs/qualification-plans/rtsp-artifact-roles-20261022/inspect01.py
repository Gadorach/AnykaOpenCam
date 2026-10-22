#!/usr/bin/env python3
"""Read existing public build artifacts; never execute or rebuild an artifact."""
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
BUILD = ROOT / 'local/build/rtsp-app/build02'
DOC = Path(__file__).resolve().parent


def pin(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        while data := stream.read(4096):
            h.update(data)
    return {'bytes': path.stat().st_size, 'sha256': h.hexdigest()}


def require(value, reason):
    if not value:
        raise RuntimeError(reason)


def elf(head):
    require(head[:4] == b'\x7fELF' and head[5:7] == b'\x01\x01', 'ELF encoding')
    require(head[4] in (1, 2), 'ELF class')
    return {'class_bits': 32 if head[4] == 1 else 64,
            'type': struct.unpack_from('<H', head, 16)[0],
            'machine': struct.unpack_from('<H', head, 18)[0],
            'flags': struct.unpack_from('<I', head, 36 if head[4] == 1 else 48)[0]}


def main():
    # Exclusive output reserves this one inspection. Failure is not retried.
    with (DOC / 'RESULT01.json').open('x') as output:
        record = {'scope': 'STATIC_BUILD_ROLE_INSPECTION_NOT_RELEASE_OR_NATIVE_ABI',
                  'programs_or_tests_or_generators_executed': False,
                  'rebuilt_or_repackaged_or_deployed': False,
                  'state': 'INSPECTION_STARTED', 'artifacts': {}, 'groups': {}}
        try:
            report_path = BUILD / 'REPORT.json'
            report_pin = pin(report_path)
            require(report_pin['sha256'] == '7c995a0a23e6dcc9e7e2a4405eb1c192b309316cc78ad1da564c0afaf3eb334d', 'Report pin')
            report = json.loads(report_path.read_bytes())
            record['report_pin'] = report_pin
            for key in ('source_pins', 'source_header_closure'):
                for name, expected in report[key].items():
                    require(pin(ROOT / name) == expected, 'Recorded input changed')
                record[key + '_rechecked'] = len(report[key])
            for command in report['commands']:
                require(command['exit'] == 0 and pin(Path(command['log'])) == command['log_pin'], 'Command log changed')
            record['recorded_successful_command_logs_rechecked'] = len(report['commands'])
            object_records = {}
            for command in report['commands']:
                args = command['argv']
                if '-c' not in args:
                    continue
                path = Path(args[args.index('-o') + 1])
                source = Path(args[args.index('-c') + 1])
                group = path.parent.name
                with path.open('rb') as stream:
                    header = elf(stream.read(64))
                arm = group.startswith('arm-')
                require(header['type'] == 1 and header['machine'] == (40 if arm else 62) and
                        header['class_bits'] == (32 if arm else 64), 'Object architecture')
                if arm:
                    require(header['flags'] & 0xff000000 == 0x05000000 and
                            all(flag in args for flag in ('-march=armv5te', '-marm', '-mfloat-abi=soft')), 'ARM recorded flags')
                defines = [arg for arg in args if arg.startswith('-D')]
                bench = group == 'host-bench'
                config = 'rtsp_crypto_config.h' if group.endswith('-verifier') else 'server_crypto_config.h'
                expected = ['-DMBEDTLS_CONFIG_FILE="' + config + '"']
                if bench:
                    expected = ['-DIS_HOST_PUBLIC_ONLY', '-DIS_SYNTHETIC_SOURCE_ONLY', *expected]
                require(defines == expected, 'Compile role defines')
                if not bench:
                    require(source.name not in ('fixture.c', 'fault_clock.c', 'wait_receipt.c', 'pread_receipt.c', 'prefix_receipt.c'), 'Fixture in strict group')
                entry = {'name': path.name, 'source': str(source.relative_to(ROOT)),
                         'pin': pin(path), 'elf_header': header}
                object_records[str(path)] = entry
                record['groups'].setdefault(group, {'defines': defines, 'objects': []})['objects'].append(entry)
            for name, expected in report['artifacts'].items():
                path = BUILD / name
                before = pin(path)
                require(before == {key: expected[key] for key in ('bytes', 'sha256')}, 'Artifact pin')
                item = {'pin': before, 'role': expected['role']}
                if path.suffix == '.a':
                    members = []
                    with path.open('rb') as stream:
                        require(stream.read(8) == b'!<arch>\n', 'Archive format')
                        while header := stream.read(60):
                            require(len(header) == 60 and header[58:] == b'`\n', 'Archive header')
                            count = int(header[48:58])
                            member = header[:16].decode('ascii').strip()
                            start = stream.tell()
                            if member not in ('/', '//', '/SYM64/'):
                                facts = elf(stream.read(64))
                                require(facts['machine'] == (40 if '-arm.' in name else 62) and facts['type'] == 1, 'Archive member architecture')
                                stream.seek(start)
                                digest = hashlib.sha256()
                                remaining = count
                                while remaining:
                                    data = stream.read(min(4096, remaining))
                                    require(bool(data), 'Short archive member')
                                    digest.update(data)
                                    remaining -= len(data)
                                members.append({'name_field': member, 'bytes': count,
                                                'sha256': digest.hexdigest(), 'elf_header': facts})
                            stream.seek(start + count + count % 2)
                    group = 'arm-integration' if '-arm.' in name else 'host-integration'
                    require(sorted((x['bytes'], x['sha256']) for x in members) ==
                            sorted((x['pin']['bytes'], x['pin']['sha256']) for x in record['groups'][group]['objects']), 'Archive/object bodies differ')
                    item['members'] = members
                else:
                    with path.open('rb') as stream:
                        facts = elf(stream.read(64))
                    arm = name.endswith('.arm') or name.startswith('arm-')
                    require(facts['machine'] == (40 if arm else 62) and facts['class_bits'] == (32 if arm else 64) and facts['type'] in (2, 3), 'Executable architecture')
                    if arm:
                        require(facts['flags'] & 0xff000000 == 0x05000000 and facts['flags'] & 0x200 and not facts['flags'] & 0x400, 'ARM executable ABI flags')
                    item['elf_header'] = facts
                require(pin(path) == before, 'Artifact changed during inspection')
                record['artifacts'][name] = item
            require({key: len(value['objects']) for key, value in record['groups'].items()} ==
                    {'host-integration': 14, 'host-verifier': 13, 'host-bench': 19,
                     'arm-integration': 14, 'arm-verifier': 13}, 'Compile group count')
            require(pin(report_path) == report_pin, 'Report changed during inspection')
            record['inspection_source_pin'] = pin(Path(__file__))
            record['state'] = 'STATIC_ROLE_INSPECTION_PASS_NOT_RELEASE_OR_NATIVE_ABI'
        except BaseException as error:
            record['state'] = 'STATIC_INSPECTION_FAILED_NO_RETRY'
            record['error'] = str(error)
            raise
        finally:
            json.dump(record, output, indent=2, sort_keys=True)
            output.write('\n')
    print('Static inspection PASS: 7 artifacts, 73 compile objects, 28 archive members; no program run')


if __name__ == '__main__':
    main()
