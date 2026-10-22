#!/usr/bin/env python3
"""Independent public file/hash/receipt/ar/ELF inspection only; no subprocesses.

Never imports candidate code or executes compiler, generator, harness, helper,
test, listener or application. Writes exclusively fresh review receipts.
"""
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import shlex
import struct
import sys
import traceback

ROOT = Path(__file__).resolve().parents[3]
OWN = Path(__file__).resolve().parent
APP = ROOT / 'apps/rtsp-auth/owned-listener-candidate'
DOC = ROOT / 'docs/qualification-plans/rtsp-owned-listener-20261022'
RUN = DOC / 'runs/build01'
FROZEN = ROOT / 'apps/rtsp-auth/isolated-server-host-tail'
SOURCE_PIN = 'febab8640027f71dfc8a1ec4aa53c26cba219048ca4bf74578050377101ce557'
FROZEN_PIN = '2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f'
seen = {}


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def read(path):
    path = Path(path)
    if not path.is_absolute():
        path = ROOT / path
    text = str(path)
    # Do not enter the other active lane, private acquisitions, target evidence,
    # Git/index, central plans, or the unrelated P1 subsystem.
    assert not any(x in text for x in ('prestarted-backend', '/.git/',
        'kernel76-station-attach-rx', '/evidence/', '/private/', '/raw/'))
    assert path.is_file(), text
    data = path.read_bytes()
    item = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    assert text not in seen or seen[text] == item, 'changed during collection: ' + text
    seen[text] = item
    return data


def pin(path):
    data = read(path)
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def check(path, expected):
    actual = pin(path)
    if isinstance(expected, str):
        assert actual['sha256'] == expected, str(path)
    else:
        assert actual == {k: expected[k] for k in ('bytes', 'sha256')}, str(path)
    return actual


def load(path):
    return json.loads(read(path))


def aggregate(entries):
    h = hashlib.sha256()
    for name, item in sorted(entries.items()):
        h.update(name.encode() + b'\0' + bytes.fromhex(item['sha256']))
    return h.hexdigest()


def elf(data):
    assert data[:4] == b'\x7fELF' and data[5:7] == b'\x01\x01'
    bits = 64 if data[4] == 2 else 32
    assert data[4] in (1, 2)
    head = struct.unpack_from('<HHIQQQIHHHHHH' if bits == 64 else '<HHIIIIIHHHHHH', data, 16)
    typ, machine, _, entry, phoff, shoff, flags, _, _, phnum, shsize, shnum, shstr = head
    assert typ == 1 and entry == 0 and phoff == 0 and phnum == 0
    fmt = '<IIQQQQIIQQ' if bits == 64 else '<IIIIIIIIII'
    assert shsize == struct.calcsize(fmt)
    sections = [struct.unpack_from(fmt, data, shoff+i*shsize) for i in range(shnum)]
    names = data[sections[shstr][4]:sections[shstr][4]+sections[shstr][5]]

    def string(raw, offset):
        return raw[offset:raw.index(b'\0', offset)].decode()

    tables = {}
    for index, section in enumerate(sections):
        _, stype, _, _, offset, size, link, _, _, entsize = section
        if stype != 2:
            continue
        strs = data[sections[link][4]:sections[link][4]+sections[link][5]]
        table = []
        for off in range(offset, offset+size, entsize):
            if bits == 64:
                name, info, other, ndx, value, length = struct.unpack_from('<IBBHQQ', data, off)
            else:
                name, value, length, info, other, ndx = struct.unpack_from('<IIIBBH', data, off)
            table.append({'name': string(strs, name), 'binding': info >> 4, 'section': ndx,
                          'type': info & 15, 'bytes': length})
        tables[index] = table
    defined = sorted({s['name'] for t in tables.values() for s in t if s['section'] and s['binding'] in (1, 2)})
    undefined = sorted({s['name'] for t in tables.values() for s in t if not s['section'] and s['name']})
    refs = set()
    relcount = 0
    for section in sections:
        _, stype, _, _, offset, size, link, _, _, entsize = section
        if stype not in (4, 9):
            continue
        for off in range(offset, offset+size, entsize):
            info = struct.unpack_from('<Q' if bits == 64 else '<I', data, off+(8 if bits == 64 else 4))[0]
            sym = tables[link][info >> (32 if bits == 64 else 8)]
            if not sym['section'] and sym['name']:
                refs.add(sym['name'])
            relcount += 1
    return {'class': bits, 'type': 'REL', 'machine': machine, 'entry': entry,
            'program_headers': phnum, 'flags': flags, 'sections': shnum,
            'section_names': [string(names, s[0]) for s in sections],
            'defined_global': defined, 'undefined': undefined,
            'relocation_count': relcount, 'undefined_relocated': sorted(refs)}


def archive(data):
    assert data[:8] == b'!<arch>\n'
    pos, longs, result = 8, b'', {}
    while pos < len(data):
        h = data[pos:pos+60]
        assert len(h) == 60 and h[58:] == b'`\n'
        name = h[:16].decode().strip()
        size = int(h[48:58]); body = data[pos+60:pos+60+size]
        assert len(body) == size
        if name == '//':
            longs = body
        elif name not in ('/', '/SYM64/'):
            if name.startswith('/'):
                offset = int(name[1:]); name = longs[offset:longs.index(b'/\n', offset)].decode()
            else:
                name = name.rstrip('/')
            assert name not in result
            result[name] = body
        pos += 60+size+(size % 2)
    assert pos == len(data)
    return result


def main(r):
    seal = load(DOC / 'FINAL_SEAL.json')
    report = load(RUN / 'REPORT.json')
    before = load(DOC / 'FROZEN_BEFORE.json')
    oldsealpath = ROOT / 'docs/qualification-plans/rtsp-tail-20261022/harness-correction/CANDIDATE_SOURCE_SEAL01.json'
    oldseal = load(oldsealpath)
    assert seal['source_pin'] == SOURCE_PIN and seal['source_count'] == 47
    assert len(seal['source_files']) == 47 and len(seal['files']) == 642
    actual = {str(p.relative_to(ROOT)): pin(p) for p in sorted(APP.rglob('*')) if p.is_file()}
    assert actual == seal['source_files'] and aggregate(actual) == SOURCE_PIN
    assert not any(p.is_symlink() for p in APP.rglob('*'))
    for path, expected in seal['files'].items():
        check(path, expected)
    assert aggregate(seal['files']) == seal['evidence_pin']
    check(RUN / 'REPORT.json', seal['build_report_pin'])
    check(DOC / 'FROZEN_BEFORE.json', seal['frozen_before_receipt'])
    assert pin(oldsealpath)['sha256'] == before['seal_sha256']
    frozen = {str(p.relative_to(ROOT)): pin(p) for p in sorted(FROZEN.rglob('*')) if p.is_file()}
    assert len(frozen) == 41 and aggregate(frozen) == FROZEN_PIN
    assert frozen == report['frozen_before'] == report['frozen_after'] == seal['frozen_after']
    assert {k: v['sha256'] for k,v in frozen.items()} == before['source_sha256'] == oldseal['candidate_source_sha256']
    for name in frozen:
        original = ROOT / name
        assert read(APP / 'frozen' / original.relative_to(FROZEN)) == read(original)
    r['source'] = {'count': 47, 'aggregate': SOURCE_PIN, 'original_imports': 41,
                   'original_aggregate': FROZEN_PIN, 'author_evidence_files': 642,
                   'author_evidence_aggregate': seal['evidence_pin']}
    for label in ('inputs', 'prior_source_header_closure', 'compiled_source_header_closure', 'toolchain_inputs'):
        for path, expected in report[label].items():
            check(path, expected)
    for path, expected in seal['compile_input_pins_reverified'].items():
        check(path, expected)
    assert (len(report['inputs']), len(report['prior_source_header_closure']), len(report['compiled_source_header_closure'])) == (441,332,293)
    for path, expected in report['inputs'].items():
        check(RUN / 'inputs' / Path(path).relative_to(ROOT), expected)
    priorpath = ROOT / 'local/build/rtsp-app/build02/REPORT.json'
    prior = load(priorpath)
    check(priorpath, report['prior_build_receipt'])
    assert prior['source_header_closure'] == report['prior_source_header_closure']
    assert report['status'] == 'COMPILE/ARCHIVE/RELOCATABLE PASS; NOT RUN OR DEPLOYED'
    assert len(report['commands']) == 52 and all(c['exit'] == 0 for c in report['commands'])
    assert all(not report[k] for k in ('application_or_tests_executed','target_operation','private_input_read','native_adoption','G3','G8'))
    for command in report['commands']:
        check(command['log'], command['log_pin'])
    original = read(APP / 'frozen/rtsp_server_candidate.c').decode().splitlines(True)
    changed = read(APP / 'rtsp_server_owned.c').decode().splitlines(True)
    delta = ''.join(difflib.unified_diff(original,changed,fromfile='frozen/rtsp_server_candidate.c',tofile='rtsp_server_owned.c'))
    assert read(RUN / 'SERVER_DELTA.patch') == delta.encode()
    r['build'] = {'started': report['started'], 'ended': report['ended'],
        'commands': report['commands'], 'explicit_inputs':441, 'prior_closure':332,
        'new_closure':293, 'author_reverified_union':len(seal['compile_input_pins_reverified']),
        'counts_overlap_not_summed':True, 'rerun':False, 'platforms':{}}
    deps = {}
    for platform in ('host', 'arm'):
        old = [x['argv'] for x in prior['commands'] if '-c' in x['argv'] and Path(x['argv'][-1]).parent.name == platform+'-integration']
        commands = [x['argv'] for x in report['commands'] if '-c' in x['argv'] and Path(x['argv'][-1]).parent.name == platform]
        assert len(old) == 14 and len(commands) == 15
        assert not any(any(m in token for m in ('IS_HOST_PUBLIC_ONLY', 'RV_TEST_PUBLIC_ONLY')) for cmd in commands for token in cmd)
        expected_objects = []
        source_list = []
        for i, (oldcmd, cmd) in enumerate(zip(old+[old[0]], commands)):
            src = Path(oldcmd[oldcmd.index('-c')+1])
            if i == 0:
                src = APP / 'rtsp_server_owned.c'
            elif i == 14:
                src = APP / 'listener_custody.c'
            assert src.name not in ('fixture.c', 'rtsp_server_candidate.c')
            obj = RUN / 'objects' / platform / (str(i)+'-'+src.stem+'.o')
            expected = []
            for token in oldcmd[:oldcmd.index('-MD')]:
                if token.startswith('-I'+str(ROOT)+'/'):
                    token = '-I'+str(RUN/'inputs'/Path(token[2:]).relative_to(ROOT))
                if i == 14 and token == '-Wno-misleading-indentation':
                    continue
                expected.append(token)
            expected += ['-I'+str(RUN/'inputs'/APP.relative_to(ROOT)), '-MD','-MF',str(obj.with_suffix('.d')),
                '-c',str(RUN/'inputs'/src.relative_to(ROOT)), '-o',str(obj)]
            assert cmd == expected, 'compile provenance ' + str(i)
            expected_objects.append(obj)
            source_list.append(str(src.relative_to(ROOT)))
            for name in shlex.split(read(obj.with_suffix('.d')).decode().replace('\\\n','').split(':',1)[1]):
                deps[name] = pin(name)
        arcmd = next(c['argv'] for c in report['commands'] if 'rcsD' in c['argv'] and c['argv'][2].endswith(platform+'.a'))
        assert arcmd[3:] == list(map(str,expected_objects))
        for label,item in report['tools'][platform].items():
            if label in ('cc1','as','ld'):
                path = item['path']
            elif label == 'cc':
                path = commands[0][0]
            else:
                suffix = {'ar':'-archive.txt','nm':'-undefined.txt','readelf':'-ELF.txt'}[label]
                path = next(c['argv'][0] for c in report['commands'] if c['log'].endswith(platform+suffix))
            check(path, item)
        members = archive(read(RUN / ('libowned-loopback-listener-'+platform+'.a')))
        assert set(members) == {p.name for p in expected_objects}
        member_pins = {}
        for path in expected_objects:
            body = members[path.name]
            assert body == read(path)
            metadata = elf(body)
            assert metadata['machine'] == (62 if platform == 'host' else 40)
            member_pins[path.name] = pin(path)
        output = RUN / ('owned-loopback-listener-'+platform+'-NOT-EXECUTABLE.o')
        metadata = elf(read(output))
        assert metadata['machine'] == (62 if platform == 'host' else 40)
        assert 'camera_rtsp_owned_listener_serve_v1' in metadata['defined_global']
        assert 'camera_rtsp_isolated_candidate_serve' not in metadata['defined_global']
        assert 'main' not in metadata['defined_global']
        assert not any(n.startswith(('camera_', 'candidate_', 'owned_', 'rv_', 'is_rv_', 'mbedtls_')) for n in metadata['undefined_relocated'])
        assert metadata['undefined_relocated'] == sorted(report['undefined'][platform]['referenced_by_relocation'])
        assert metadata['undefined'] == sorted(report['undefined'][platform]['symbol_table'])
        r['build']['platforms'][platform] = {'compiled_sources':source_list,
            'archive_members':member_pins, 'relocatable':metadata, 'artifact':pin(output)}
    assert deps == report['compiled_source_header_closure']
    for name,item in report['artifacts'].items():
        check(RUN/name,item)
    for path in ('apps/rtsp-auth/qc07f-runtime-candidate/rtsp_native_source.c',
                 'apps/rtsp-auth/qc07f-runtime-candidate/rtsp_native_source.h',
                 'framework/src/packet_hub.c'):
        pin(path)
    r['author_seal_verified'] = True
    r['status'] = 'STATIC RECEIPT/HASH/ARCHIVE/ELF CHECK PASS; NOT EXECUTION OR BEHAVIORAL PROOF'


if __name__ == '__main__':
    assert len(sys.argv) == 2 and sys.argv[1] in ('before01','after01')
    destination = OWN / ('STATIC_'+sys.argv[1]+'.json')
    assert not destination.exists()
    receipt = {'started':utc(), 'argv':sys.argv, 'scope':'STATIC_PUBLIC_READ_HASH_RECEIPT_AR_ELF_ONLY',
        'candidate_or_tool_execution':False, 'runtime_tests':False, 'target_IO':False,
        'policy_rejection_observed':False, 'status':'INCOMPLETE'}
    try:
        main(receipt)
    except BaseException as exc:
        receipt['status'] = 'COLLECTION ERROR; NOT A CANDIDATE FAILURE'
        receipt['error'] = type(exc).__name__+': '+str(exc)
        receipt['traceback'] = traceback.format_exc()
        raise
    finally:
        receipt['input_pins'] = dict(sorted(seen.items()))
        receipt['input_count'] = len(seen)
        receipt['input_aggregate'] = aggregate(seen)
        receipt['ended'] = utc()
        with destination.open('x') as stream:
            json.dump(receipt,stream,indent=2)
            stream.write('\n')
        print(destination)
