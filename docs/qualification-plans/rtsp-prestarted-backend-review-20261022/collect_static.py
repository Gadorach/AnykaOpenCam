#!/usr/bin/env python3
"""Independent public read/hash/receipt/ar/ELF collection; no subprocess/import of candidate code."""
import datetime
import hashlib
import json
from pathlib import Path
import re
import shlex
import struct
import sys
import traceback

OWN = Path(__file__).resolve().parent
ROOT = OWN.parents[2]
APP = ROOT/'apps/rtsp-auth/prestarted-backend-candidate'
DOC = ROOT/'docs/qualification-plans/rtsp-prestarted-backend-20261022'
RUN = DOC/'runs/build01'
FROZEN = ROOT/'apps/rtsp-auth/isolated-server-host-tail'
SOURCE = '867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90'
SEAL = '9efc3a84ab6342e9ad789ac1b6448358ea8798cde0339726b78fb47dec3a23c9'
BASE = '2cc37be5839961f3371f35de81774f0e24bf02d80b7b032295076249827c825f'
seen = {}


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def read(path):
    path = Path(path)
    if not path.is_absolute():
        path = ROOT/path
    name = str(path)
    assert not any(x in name for x in ('owned-listener','/.git/','/AGENTS.md','/docs/plan/',
        '/evidence/','/private/','/raw/','kernel76-station-attach-rx'))
    assert path.is_file(), name
    data = path.read_bytes()
    value = {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    assert name not in seen or seen[name] == value, 'changed during collection: '+name
    seen[name] = value
    return data


def pin(path):
    data = read(path)
    return {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}


def check(path, expected):
    actual = pin(path)
    assert actual['sha256'] == expected if isinstance(expected,str) else actual == {k:expected[k] for k in ('bytes','sha256')}, str(path)
    return actual


def load(path):
    return json.loads(read(path))


def aggregate(entries):
    h = hashlib.sha256()
    for name,item in sorted(entries.items()):
        h.update(name.encode()+b'\0'+bytes.fromhex(item['sha256']))
    return h.hexdigest()


def ar_members(data):
    assert data[:8] == b'!<arch>\n'
    pos,longs,members = 8,b'',{}
    while pos < len(data):
        header = data[pos:pos+60]
        assert len(header) == 60 and header[58:] == b'`\n'
        name = header[:16].decode().strip(); size = int(header[48:58])
        body = data[pos+60:pos+60+size]; assert len(body) == size
        if name == '//':
            longs = body
        elif name not in ('/','/SYM64/'):
            if name.startswith('/'):
                off = int(name[1:]); name = longs[off:longs.index(b'/\n',off)].decode()
            else:
                name = name.rstrip('/')
            assert name not in members
            members[name] = body
        pos += 60+size+size%2
    assert pos == len(data)
    return members


def elf(data):
    assert data[:4] == b'\x7fELF' and data[4] in (1,2) and data[5:7] == b'\x01\x01'
    bits = 64 if data[4] == 2 else 32
    h = struct.unpack_from('<HHIQQQIHHHHHH' if bits == 64 else '<HHIIIIIHHHHHH',data,16)
    typ,machine,_,entry,phoff,shoff,flags,_,_,phnum,shsize,shnum,shstr = h
    assert typ == 1 and entry == phoff == phnum == 0
    fmt = '<IIQQQQIIQQ' if bits == 64 else '<IIIIIIIIII'
    assert shsize == struct.calcsize(fmt)
    secs = [struct.unpack_from(fmt,data,shoff+i*shsize) for i in range(shnum)]
    secnames = data[secs[shstr][4]:secs[shstr][4]+secs[shstr][5]]

    def string(raw,off):
        return raw[off:raw.index(b'\0',off)].decode()

    tables = {}
    for idx,s in enumerate(secs):
        _,stype,_,_,off,size,link,_,_,entsize = s
        if stype != 2:
            continue
        strings = data[secs[link][4]:secs[link][4]+secs[link][5]]
        table = []
        for pos in range(off,off+size,entsize):
            if bits == 64:
                name,info,_,ndx,value,length = struct.unpack_from('<IBBHQQ',data,pos)
            else:
                name,value,length,info,_,ndx = struct.unpack_from('<IIIBBH',data,pos)
            table.append({'name':string(strings,name),'binding':info>>4,'section':ndx,'type':info&15,'bytes':length})
        tables[idx] = table
    defined = sorted({s['name'] for t in tables.values() for s in t if s['section'] and s['binding'] in (1,2)})
    undefined = sorted({s['name'] for t in tables.values() for s in t if not s['section'] and s['name']})
    refs = set(); relocations = 0
    for s in secs:
        _,stype,_,_,off,size,link,_,_,entsize = s
        if stype not in (4,9):
            continue
        for pos in range(off,off+size,entsize):
            info = struct.unpack_from('<Q' if bits == 64 else '<I',data,pos+(8 if bits == 64 else 4))[0]
            symbol = tables[link][info>>(32 if bits == 64 else 8)]
            if not symbol['section'] and symbol['name']:
                refs.add(symbol['name'])
            relocations += 1
    return {'bits':bits,'type':'REL','machine':machine,'entry':entry,'program_headers':phnum,
        'flags':flags,'section_names':[string(secnames,s[0]) for s in secs],
        'defined_global':defined,'undefined':undefined,'relocation_count':relocations,
        'undefined_relocated':sorted(refs)}


def verify_delta(old,new,patch):
    """Check every original/context line and reconstruct new file from recorded unified diff."""
    old = old.decode().splitlines(True); patch = patch.decode().splitlines(True)
    assert patch[0].startswith('--- ') and patch[1].startswith('+++ ')
    output = []; cursor = 0; at = 2; hunks = 0
    while at < len(patch):
        match = re.fullmatch(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@.*\n',patch[at])
        assert match, patch[at]
        start = int(match[1])-1; assert start >= cursor
        output += old[cursor:start]; cursor = start; at += 1
        oldcount = newcount = 0
        while at < len(patch) and not patch[at].startswith('@@ '):
            line = patch[at]; tag = line[0]; body = line[1:]
            assert tag in (' ','-','+')
            if tag in (' ','-'):
                assert old[cursor] == body
                cursor += 1; oldcount += 1
            if tag in (' ','+'):
                output.append(body); newcount += 1
            at += 1
        assert oldcount == int(match[2] or 1) and newcount == int(match[4] or 1)
        hunks += 1
    output += old[cursor:]
    assert ''.join(output).encode() == new
    return hunks


def main(result):
    check(DOC/'SOURCE_SEAL01.json',SEAL)
    source_seal = load(DOC/'SOURCE_SEAL01.json'); build_seal = load(DOC/'BUILD_SEAL01.json')
    report = load(RUN/'REPORT.json')
    assert source_seal['source_tree_sha256'] == SOURCE
    actual = {str(p.relative_to(ROOT)):pin(p) for p in sorted(APP.iterdir()) if p.is_file()}
    assert len(actual) == 11 and actual == source_seal['source_files'] and aggregate(actual) == SOURCE
    assert not any(p.is_symlink() for p in APP.iterdir())
    for name,item in source_seal['evidence_pins'].items():
        check(name,item)
    assert len(source_seal['evidence_pins']) == 592
    check(RUN/'REPORT.json',build_seal['report'])
    check(DOC/'OWNERSHIP_CONTRACT.md',source_seal['contract_before_code'])
    oldseal = load(ROOT/'docs/qualification-plans/rtsp-tail-20261022/harness-correction/CANDIDATE_SOURCE_SEAL01.json')
    baseline = {}
    for name,digest in oldseal['candidate_source_sha256'].items():
        original = ROOT/name
        item = check(original,digest)
        check(DOC/'baseline'/original.relative_to(FROZEN),item)
        baseline[name] = item
    assert len(baseline) == 41 and aggregate(baseline) == BASE
    assert report['server_pin'] == BASE
    retention_counts = {}
    for name,expected_count in (('INPUTS.sha256',91),('OUTPUTS.sha256',5)):
        path = ROOT/'docs/qualification-plans/qc07f-retention-owner-20261022'/name
        raw = read(path)
        check(RUN/'provenance'/('retention-'+name),pin(path))
        lines = raw.decode().splitlines(); assert len(lines) == expected_count
        for line in lines:
            digest,relative = line.split('  ',1)
            check(relative,digest)  # hash-only preservation; no semantic sibling review
        retention_counts[name] = len(lines)
    combined = {}
    for label,count in (('inputs',200),('frozen',168),('closure',282)):
        assert len(report[label]) == count
    for label in ('inputs','frozen','closure','artifacts','toolchain_pins'):
        for name,item in report[label].items():
            check(name,item)
            combined[name] = {k:item[k] for k in ('bytes','sha256')}
    assert combined == build_seal['verified_recorded_input_pins'] and len(combined) == 661
    snapshots = {}
    for name,item in actual.items():
        snapshots[str(ROOT/name)] = RUN/'source/app'/Path(name).name
    vendor = ROOT/'apps/auth-web/service/vendor/mbedtls'
    vendor_pins = {}
    for path in sorted(vendor.rglob('*')):
        if path.is_file():
            vendor_pins[str(path.relative_to(vendor))] = pin(path)
            if path.suffix == '.h' or path.name.startswith(('LICENSE','COPYING')):
                snapshots[str(path)] = RUN/'source/vendor'/path.relative_to(vendor)
    assert len(vendor_pins) == 277 and aggregate(vendor_pins) == report['vendor_pin'] == 'bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5'
    for path in sorted((ROOT/'framework/include/camera').glob('*.h')):
        snapshots[str(path)] = RUN/'source/framework/include/camera'/path.name
    snapshots[str(ROOT/'apps/rtsp-auth/isolated-verifier/verifier.h')] = RUN/'source/verifier/verifier.h'
    snapshots[str(DOC/'OWNERSHIP_CONTRACT.md')] = RUN/'provenance/OWNERSHIP_CONTRACT.md'
    snapshots[str(ROOT/'docs/qualification-plans/rtsp-tail-20261022/harness-correction/CANDIDATE_SOURCE_SEAL01.json')] = RUN/'provenance/frozen-server-seal.json'
    for name in ('INPUTS.sha256','OUTPUTS.sha256'):
        snapshots[str(ROOT/'docs/qualification-plans/qc07f-retention-owner-20261022'/name)] = RUN/'provenance'/('retention-'+name)
    assert set(snapshots) == set(report['inputs'])
    for name,path in snapshots.items():
        check(path,report['inputs'][name])
    for name,item in report['closure'].items():
        check(RUN/'dependencies'/Path(name).relative_to('/'),item)
    for path in (DOC/'baseline/runtime-licenses').rglob('*'):
        if path.is_file():
            check(RUN/'source/runtime-licenses'/path.relative_to(DOC/'baseline/runtime-licenses'),pin(path))
    changed = ['adapter_state.h','candidate_auth.c','candidate_auth.h','rtsp_server_candidate.c','verifier_adapter_overlay.c']
    unchanged = ['delay_pin.h','host_pin.h','native_pin.h','server_crypto_config.h']
    assert build_seal['changed_imports'] == changed and build_seal['unchanged_imports'] == unchanged
    deltas = {}
    for name in changed:
        deltas[name] = verify_delta(read(DOC/'baseline'/name),read(APP/name),read(RUN/'logs'/('delta-'+name+'.txt')))
    for name in unchanged:
        assert read(APP/name) == read(DOC/'baseline'/name)
    assert report['status'] == 'HOST_ARM_CHANGED_UNITS_COMPILE_ARCHIVE_REL_PASS_NOT_RUN'
    assert not report['programs_tests_generators_listeners_executed'] and not report['native_or_composite_acceptance']
    assert len(report['commands']) == 31
    for cmd in report['commands']:
        check(cmd['log'],cmd['log_pin'])
        assert cmd['exit'] == (1 if Path(cmd['argv'][0]).name == 'diff' else 0)
    compiles = [c for c in report['commands'] if '-c' in c['argv']]
    assert len(compiles) == 6 and all(c['log_pin']['bytes'] == 0 for c in compiles)
    closure = {}; platform_metadata = {}
    for platform in ('host','arm'):
        tools = report['tools'][platform]
        for item in tools.values():
            check(item['path'],item)
        commands = [c['argv'] for c in compiles if Path(c['argv'][-1]).name.startswith(platform+'-')]
        assert len(commands) == 3
        objects = []
        common = ['-std=gnu11','-Wall','-Wextra','-Werror','-fno-common','-fno-strict-aliasing',
            '-ffunction-sections','-fdata-sections','-fstack-protector-strong','-D_FORTIFY_SOURCE=2',
            '-fstack-usage','-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']
        common += ['-I'+str(RUN/'source'/d) for d in ('app','framework/include','verifier','vendor/include')]
        flags = ['-O2'] if platform == 'host' else ['-Os','-march=armv5te','-marm','-mfloat-abi=soft']
        for name,argv in zip(('candidate_auth','rtsp_server_candidate','verifier_adapter_overlay'),commands):
            obj = RUN/'objects'/(platform+'-'+name+'.o'); src = RUN/'source/app'/(name+'.c')
            exception = ['-Wno-misleading-indentation'] if name == 'rtsp_server_candidate' else []
            expected = [tools['cc']['path'],*common,*flags,*exception,'-MD','-MF',str(obj.with_suffix('.d')),'-c',str(src),'-o',str(obj)]
            assert argv == expected
            assert not any('PUBLIC_ONLY' in a or 'DELAY_TEST' in a for a in argv)
            objects.append(obj)
            for dep in shlex.split(read(obj.with_suffix('.d')).decode().replace('\\\n','').split(':',1)[1]):
                closure[dep] = pin(dep)
        archive = RUN/('libprestarted-backend-'+platform+'.a')
        arcmd = next(c['argv'] for c in report['commands'] if 'rcsD' in c['argv'] and c['argv'][2] == str(archive))
        assert arcmd == [tools['ar']['path'],'rcsD',str(archive),*map(str,objects)]
        members = ar_members(read(archive)); assert set(members) == {o.name for o in objects}
        for obj in objects:
            assert members[obj.name] == read(obj)
            assert elf(read(obj))['machine'] == (62 if platform == 'host' else 40)
        reloc = RUN/('prestarted-backend-'+platform+'-NOT-EXECUTABLE.o')
        expected_link = [tools['cc']['path'],*flags,'-nostdlib','-r',*map(str,objects),'-Wl,-Map='+str(RUN/(platform+'.map')),'-o',str(reloc)]
        assert any(c['argv'] == expected_link for c in report['commands'])
        meta = elf(read(reloc)); assert meta['machine'] == (62 if platform == 'host' else 40)
        assert all(n in meta['defined_global'] for n in ('candidate_prestarted_start','candidate_prestarted_finish','camera_rtsp_prestarted_candidate_serve','is_rv_running','rv_start','rv_stop','rv_destroy'))
        assert not any(n in meta['defined_global'] for n in ('main','candidate_backend_init','candidate_backend_destroy','candidate_backend_finish','camera_rtsp_isolated_candidate_serve'))
        actual_undefined = sorted(line.split()[-1] for line in read(RUN/'logs'/(platform+'-undefined.txt')).decode().splitlines() if line.strip())
        assert meta['undefined'] == actual_undefined
        assert not any(n.startswith(('candidate_','is_rv_')) for n in meta['undefined_relocated'])
        platform_metadata[platform] = {'objects':{str(o.relative_to(ROOT)):pin(o) for o in objects},
            'archive':pin(archive),'archive_members':list(members),'relocatable':pin(reloc),'ELF':meta}
    assert closure == report['closure']
    failure = load(DOC/'SEAL_FAILURE01.json')
    check(DOC/'seal-initial01.py',failure['original_finalizer_sha256'])
    assert not failure['repeat_compilation'] and not failure['production_or_build_source_changed']
    # Compare static-finalizer source only; never import or execute either script.
    old = read(DOC/'seal-initial01.py').decode().splitlines()
    new = read(DOC/'seal.py').decode().splitlines()
    assert len(old) < len(new)
    result.update(status='STATIC PIN/RECEIPT/ARCHIVE/ELF VERIFICATION PASS; SOURCE DEFECT REVIEW SEPARATE',
        source_count=11,source_pin=SOURCE,source_seal_pin=SEAL,baseline_count=41,baseline_pin=BASE,
        retention_preserved=retention_counts,evidence_files=592,input_map_count=200,
        consumed_closure_count=282,frozen_map_count=168,author_verified_union=661,
        maps_overlap_not_summed=True,recorded_commands=report['commands'],recorded_compile_count=6,
        rerun_compiler=False,started_author_build=report['started'],ended_author_build=report['ended'],
        delta_hunks=deltas,platform_metadata=platform_metadata)


if __name__ == '__main__':
    assert len(sys.argv) == 2 and sys.argv[1] in ('before01','after01')
    path = OWN/('STATIC_'+sys.argv[1]+'.json'); assert not path.exists()
    result = {'started':utc(),'argv':sys.argv,'status':'INCOMPLETE',
        'scope':'STATIC_PUBLIC_READ_HASH_RECEIPT_ARCHIVE_ELF_ONLY',
        'candidate_compiler_helper_test_runtime_execution':False,'target_network_private_IO':False,
        'other_sibling_source_semantic_review_or_composition':False,'policy_rejection_observed':False}
    try:
        main(result)
    except BaseException as exc:
        result.update(status='COLLECTION ERROR; NOT CANDIDATE FAILURE',error=type(exc).__name__+': '+str(exc),traceback=traceback.format_exc())
        raise
    finally:
        result.update(input_pins=dict(sorted(seen.items())),input_count=len(seen),input_aggregate=aggregate(seen),ended=utc())
        with path.open('x') as out:
            json.dump(result,out,indent=2); out.write('\n')
        print(path)
