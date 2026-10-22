#!/usr/bin/env python3
"""Fresh independent static public hash/receipt/archive/ELF inspection; no subprocesses."""
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
APP = ROOT/'apps/rtsp-auth/prestarted-backend-v2'
DOC = ROOT/'docs/qualification-plans/rtsp-prestarted-backend-v2-20261022'
RUN = DOC/'runs/build01'
V1DOC = ROOT/'docs/qualification-plans/rtsp-prestarted-backend-20261022'
V1APP = ROOT/'apps/rtsp-auth/prestarted-backend-candidate'
V1REVIEW = ROOT/'docs/qualification-plans/rtsp-prestarted-backend-review-20261022'
SOURCE = '5a42cdc9d76628031c5380edd2e3b2c4dd06aefc6675138c8e10e1b098a28868'
SEAL = '2bfa06a6b239c0c62b1003df6ee0f0ef9dd17ef0088efad00cccaf1ada7e7a8a'
V1SOURCE = '867a8b94c43096b90b80f659f87e34ec610e45ba42aa3678d1fca9940b8b4f90'
V1SEAL = '9efc3a84ab6342e9ad789ac1b6448358ea8798cde0339726b78fb47dec3a23c9'
V1REVIEWSEAL = 'd060aaa15deea1526920414f4015505f1024fa73e2f49eff6a502a90049ce323'
seen = {}


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def read(path):
    path = Path(path)
    if not path.is_absolute():
        path = ROOT/path
    name = str(path)
    assert not any(s in name for s in ('owned-listener','/.git/','/AGENTS.md','/docs/plan/',
        '/evidence/','/private/','/raw/','kernel76-station-attach-rx'))
    assert path.is_file(), name
    raw = path.read_bytes()
    value = {'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
    assert name not in seen or seen[name] == value, 'changed during collection: '+name
    seen[name] = value
    return raw


def pin(path):
    raw = read(path)
    return {'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}


def check(path,expected):
    actual = pin(path)
    expected = {'sha256':expected} if isinstance(expected,str) else expected
    assert all(actual[k] == expected[k] for k in ('bytes','sha256') if k in expected), str(path)
    return actual


def load(path):
    return json.loads(read(path))


def aggregate(entries):
    h = hashlib.sha256()
    for name,item in sorted(entries.items()):
        h.update(name.encode()+b'\0'+bytes.fromhex(item['sha256']))
    return h.hexdigest()


def ar_members(raw):
    assert raw[:8] == b'!<arch>\n'
    pos,longs,members = 8,b'',{}
    while pos < len(raw):
        h = raw[pos:pos+60]; assert len(h) == 60 and h[58:] == b'`\n'
        name = h[:16].decode().strip(); size = int(h[48:58]); body = raw[pos+60:pos+60+size]
        assert len(body) == size
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
    assert pos == len(raw)
    return members


def elf(raw):
    assert raw[:4] == b'\x7fELF' and raw[4] in (1,2) and raw[5:7] == b'\x01\x01'
    bits = 64 if raw[4] == 2 else 32
    h = struct.unpack_from('<HHIQQQIHHHHHH' if bits == 64 else '<HHIIIIIHHHHHH',raw,16)
    typ,machine,_,entry,phoff,shoff,flags,_,_,phnum,shsize,shnum,shstr = h
    assert typ == 1 and entry == phoff == phnum == 0
    fmt = '<IIQQQQIIQQ' if bits == 64 else '<IIIIIIIIII'
    assert shsize == struct.calcsize(fmt)
    secs = [struct.unpack_from(fmt,raw,shoff+i*shsize) for i in range(shnum)]

    def string(data,off):
        return data[off:data.index(b'\0',off)].decode()

    names = raw[secs[shstr][4]:secs[shstr][4]+secs[shstr][5]]
    tables = {}
    for index,s in enumerate(secs):
        _,kind,_,_,off,size,link,_,_,stride = s
        if kind != 2:
            continue
        strings = raw[secs[link][4]:secs[link][4]+secs[link][5]]
        table = []
        for pos in range(off,off+size,stride):
            if bits == 64:
                name,info,_,ndx,_,length = struct.unpack_from('<IBBHQQ',raw,pos)
            else:
                name,_,length,info,_,ndx = struct.unpack_from('<IIIBBH',raw,pos)
            table.append({'name':string(strings,name),'binding':info>>4,'type':info&15,'section':ndx,'bytes':length})
        tables[index] = table
    defined = sorted({s['name'] for t in tables.values() for s in t if s['section'] and s['binding'] in (1,2)})
    undefined = sorted({s['name'] for t in tables.values() for s in t if not s['section'] and s['name']})
    refs = set(); count = 0
    for s in secs:
        _,kind,_,_,off,size,link,_,_,stride = s
        if kind not in (4,9):
            continue
        for pos in range(off,off+size,stride):
            info = struct.unpack_from('<Q' if bits == 64 else '<I',raw,pos+(8 if bits == 64 else 4))[0]
            sym = tables[link][info>>(32 if bits == 64 else 8)]
            if not sym['section'] and sym['name']:
                refs.add(sym['name'])
            count += 1
    return {'bits':bits,'type':'REL','machine':machine,'entry':entry,'program_headers':phnum,
        'flags':flags,'section_names':[string(names,s[0]) for s in secs],
        'defined_global':defined,'undefined':undefined,'undefined_relocated':sorted(refs),'relocation_count':count}


def delta(old,new,patch):
    old = old.decode().splitlines(True); patch = patch.decode().splitlines(True)
    assert patch[0].startswith('--- ') and patch[1].startswith('+++ ')
    result = []; cursor = 0; at = 2; hunks = 0
    while at < len(patch):
        m = re.fullmatch(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@.*\n',patch[at]); assert m
        start = int(m[1])-1; assert start >= cursor
        result += old[cursor:start]; cursor = start; at += 1; a = b = 0
        while at < len(patch) and not patch[at].startswith('@@ '):
            tag,body = patch[at][0],patch[at][1:]; assert tag in (' ','-','+')
            if tag in (' ','-'):
                assert old[cursor] == body; cursor += 1; a += 1
            if tag in (' ','+'):
                result.append(body); b += 1
            at += 1
        assert a == int(m[2] or 1) and b == int(m[4] or 1)
        hunks += 1
    result += old[cursor:]
    assert ''.join(result).encode() == new
    return hunks


def segment(raw,start,end):
    return raw[raw.index(start):raw.index(end)]


def main(result):
    check(DOC/'SOURCE_SEAL01.json',SEAL)
    source = load(DOC/'SOURCE_SEAL01.json'); build = load(DOC/'BUILD_SEAL01.json'); report = load(RUN/'REPORT.json')
    actual = {str(p.relative_to(ROOT)):pin(p) for p in sorted(APP.iterdir()) if p.is_file()}
    assert not any(p.is_symlink() for p in APP.iterdir())
    assert len(actual) == 11 and actual == source['source_files'] and aggregate(actual) == SOURCE == source['source_tree_sha256']
    for name,item in source['evidence_pins'].items():
        check(name,item)
    assert len(source['evidence_pins']) == 570
    check(RUN/'REPORT.json',build['report']); check(DOC/'R1_CONTRACT.md',source['contract_before_code'])
    check(V1DOC/'SOURCE_SEAL01.json',V1SEAL); check(V1REVIEW/'FINAL_SEAL.json',V1REVIEWSEAL)
    v1 = load(V1DOC/'SOURCE_SEAL01.json'); review = load(V1REVIEW/'FINAL_SEAL.json')
    assert aggregate(v1['source_files']) == V1SOURCE and len(v1['source_files']) == 11
    for table in (v1['source_files'],v1['evidence_pins'],review['input_pins'],review['output_pins']):
        for name,item in table.items():
            check(name,item)  # frozen historical source/evidence custody; no sibling adoption
    for name,item in v1['source_files'].items():
        check(DOC/'baseline-v1-app'/Path(name).relative_to(V1APP.relative_to(ROOT)),item)
    for path in sorted((DOC/'baseline-provenance').iterdir()):
        origin = (V1REVIEW if path.name in ('FINAL_SEAL.json','REVIEW.md') else V1DOC)/path.name
        check(path,pin(origin)); check(RUN/'provenance'/path.name,pin(origin))
    assert (len(report['inputs']),len(report['closure']),len(report['frozen'])) == (204,282,1328)
    combined = {}
    for label in ('inputs','frozen','closure','artifacts','toolchain_pins'):
        for name,item in report[label].items():
            check(name,item); combined[name] = {k:item[k] for k in ('bytes','sha256')}
    assert combined == build['verified_recorded_input_pins'] and len(combined) == 1377
    for alias,target in report['frozen_tool_aliases'].items():
        assert str(Path(alias).resolve()) == target
    changed = [p.name for p in sorted(APP.iterdir()) if pin(p) != pin(DOC/'baseline-v1-app'/p.name)]
    assert changed == build['changed_files'] == ['README.md','adapter_state.h','build.py','verifier_adapter_overlay.c']
    assert len(build['unchanged_imports']) == 7
    hunks = {name:delta(read(DOC/'baseline-v1-app'/name),read(APP/name),read(RUN/'logs'/('delta-'+name+'.txt'))) for name in changed}
    old = read(DOC/'baseline-v1-app/verifier_adapter_overlay.c'); new = read(APP/'verifier_adapter_overlay.c')
    spans = {}
    for label,a,b in (('clock_reap_signal_barrier_launch_image',b'static int clock_owner',b'int rv_start('),
                       ('RPC_policy_submit_complete_query',b'int rv_rpc(',b'int rv_stop(')):
        before,after = segment(old,a,b),segment(new,a,b); assert before == after
        spans[label] = {'bytes':len(after),'sha256':hashlib.sha256(after).hexdigest()}
    tail = b'#ifdef RV_TEST_PUBLIC_ONLY\nint rv_test_owned_pidfd'
    before = segment(old,b'int rv_destroy(',tail); after = segment(new,b'int rv_destroy(',b'int is_rv_ipc_close_outcome(')
    assert before == after and old[old.index(tail):] == new[new.index(tail):]
    spans['destroy_retirement_observers_running'] = {'bytes':len(after),'sha256':hashlib.sha256(after).hexdigest()}
    assert spans == build['preserved_adapter_body_spans']
    before = segment(old,b'int rv_start(',b'int rv_rpc('); after = segment(new,b'int rv_start(',b'int rv_rpc(')
    a = b' if(!c->barrier_released&&c->fd>=0){close(c->fd);c->fd=-1;} /*release-barrier EOF BEFORE any exec/material*/'
    b = b' if(!c->barrier_released&&c->fd>=0)(void)ipc_close_once(c); /*barrier EOF; retain any close error through actual stop/wait*/'
    assert before.count(a) == 1 and before.replace(a,b) == after
    before = segment(old,b'int rv_stop(',b'int rv_destroy('); after = segment(new,b'int rv_stop(',b'int rv_destroy(')
    for a,b in ((b'if(c->retired)return RV_OK;',b'if(c->retired)return ipc_close_once(c);'),
                (b'r=reap(c);if(r==RV_OK){if(c->fd>=0){close(c->fd);c->fd=-1;}return RV_OK;}',b'r=reap(c);if(r==RV_OK)return ipc_close_once(c);')):
        assert before.count(a) == 1; before = before.replace(a,b)
    assert before == after
    snapshots = {str(ROOT/n):RUN/'source/app'/Path(n).name for n in actual}
    vendor = ROOT/'apps/auth-web/service/vendor/mbedtls'; vendor_pins = {}
    for path in sorted(vendor.rglob('*')):
        if path.is_file():
            vendor_pins[str(path.relative_to(vendor))] = pin(path)
            if path.suffix == '.h' or path.name.startswith(('LICENSE','COPYING')):
                snapshots[str(path)] = RUN/'source/vendor'/path.relative_to(vendor)
    assert len(vendor_pins) == 277 and aggregate(vendor_pins) == report['vendor_pin'] == 'bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5'
    for path in sorted((ROOT/'framework/include/camera').glob('*.h')):
        snapshots[str(path)] = RUN/'source/framework/include/camera'/path.name
    snapshots[str(ROOT/'apps/rtsp-auth/isolated-verifier/verifier.h')] = RUN/'source/verifier/verifier.h'
    for path in sorted((DOC/'baseline-provenance').iterdir()):
        snapshots[str(path)] = RUN/'provenance'/path.name
    snapshots[str(DOC/'R1_CONTRACT.md')] = RUN/'provenance/R1_CONTRACT.md'
    assert set(snapshots) == set(report['inputs'])
    for name,path in snapshots.items():
        check(path,report['inputs'][name])
    for name,item in report['closure'].items():
        check(RUN/'dependencies'/Path(name).relative_to('/'),item)
    licenses = V1DOC/'baseline/runtime-licenses'
    for path in licenses.rglob('*'):
        if path.is_file():
            check(RUN/'source/runtime-licenses'/path.relative_to(licenses),pin(path))
    assert report['status'] == 'HOST_ARM_CHANGED_UNITS_COMPILE_ARCHIVE_REL_PASS_NOT_RUN'
    assert not report['programs_tests_generators_listeners_executed'] and not report['native_or_composite_acceptance']
    assert len(report['commands']) == 30
    for cmd in report['commands']:
        check(cmd['log'],cmd['log_pin']); assert cmd['exit'] == (1 if Path(cmd['argv'][0]).name == 'diff' else 0)
    compiles = [c for c in report['commands'] if '-c' in c['argv']]
    assert len(compiles) == 6 and all(c['log_pin']['bytes'] == 0 for c in compiles)
    closure = {}; metadata = {}
    for platform in ('host','arm'):
        tools = report['tools'][platform]
        for item in tools.values():
            check(item['path'],item)
        cmds = [c['argv'] for c in compiles if Path(c['argv'][-1]).name.startswith(platform+'-')]; assert len(cmds) == 3
        common = ['-std=gnu11','-Wall','-Wextra','-Werror','-fno-common','-fno-strict-aliasing',
            '-ffunction-sections','-fdata-sections','-fstack-protector-strong','-D_FORTIFY_SOURCE=2',
            '-fstack-usage','-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']
        common += ['-I'+str(RUN/'source'/d) for d in ('app','framework/include','verifier','vendor/include')]
        flags = ['-O2'] if platform == 'host' else ['-Os','-march=armv5te','-marm','-mfloat-abi=soft']
        objects = []; object_meta = {}; matching = []
        for name,argv in zip(('candidate_auth','rtsp_server_candidate','verifier_adapter_overlay'),cmds):
            obj = RUN/'objects'/(platform+'-'+name+'.o'); src = RUN/'source/app'/(name+'.c')
            exception = ['-Wno-misleading-indentation'] if name == 'rtsp_server_candidate' else []
            assert argv == [tools['cc']['path'],*common,*flags,*exception,'-MD','-MF',str(obj.with_suffix('.d')),'-c',str(src),'-o',str(obj)]
            assert not any('PUBLIC_ONLY' in a or 'DELAY_TEST' in a for a in argv)
            objects.append(obj); object_meta[obj.name] = elf(read(obj))
            assert object_meta[obj.name]['machine'] == (62 if platform == 'host' else 40)
            oldobj = V1DOC/'runs/build01/objects'/obj.name
            if read(oldobj) == read(obj):
                matching.append(obj.name)
            for name in shlex.split(read(obj.with_suffix('.d')).decode().replace('\\\n','').split(':',1)[1]):
                closure[name] = pin(name)
        archive = RUN/('libprestarted-backend-v2-'+platform+'.a')
        assert any(c['argv'] == [tools['ar']['path'],'rcsD',str(archive),*map(str,objects)] for c in report['commands'])
        members = ar_members(read(archive)); assert set(members) == {p.name for p in objects}
        assert all(members[p.name] == read(p) for p in objects)
        reloc = RUN/('prestarted-backend-v2-'+platform+'-NOT-EXECUTABLE.o')
        assert any(c['argv'] == [tools['cc']['path'],*flags,'-nostdlib','-r',*map(str,objects),'-Wl,-Map='+str(RUN/(platform+'.map')),'-o',str(reloc)] for c in report['commands'])
        linked = elf(read(reloc)); assert linked['machine'] == (62 if platform == 'host' else 40)
        assert all(n in linked['defined_global'] for n in ('candidate_prestarted_start','candidate_prestarted_finish','camera_rtsp_prestarted_candidate_serve','rv_start','rv_stop','rv_destroy','is_rv_ipc_close_outcome','is_rv_context_bytes'))
        assert not any(n in linked['defined_global'] for n in ('main','candidate_backend_init','candidate_backend_destroy','candidate_backend_finish','camera_rtsp_isolated_candidate_serve'))
        undefined = sorted(line.split()[-1] for line in read(RUN/'logs'/(platform+'-undefined.txt')).decode().splitlines() if line.strip())
        assert undefined == linked['undefined']
        assert not any(n.startswith(('candidate_','is_rv_')) for n in linked['undefined_relocated'])
        metadata[platform] = {'members':{p.name:pin(p) for p in objects},'object_ELF':object_meta,
            'archive':pin(archive),'relocatable':pin(reloc),'ELF':linked,'v1_byte_matching_fresh_compiles':matching}
    assert closure == report['closure']
    result.update(status='STATIC SOURCE/RECEIPT/ARCHIVE/ELF PIN CHECK PASS; NOT RUNTIME',
        source_count=11,source_pin=SOURCE,source_seal_pin=SEAL,author_evidence_count=570,
        v1_source_pin=V1SOURCE,v1_review_seal=V1REVIEWSEAL,input_map_count=204,
        closure_count=282,frozen_count=1328,author_verified_union=1377,maps_overlap_not_summed=True,
        delta_hunks=hunks,preserved_adapter_spans=spans,recorded_commands=report['commands'],
        compile_count=6,rerun_compiler=False,author_started=report['started'],author_ended=report['ended'],platform_metadata=metadata)


if __name__ == '__main__':
    assert len(sys.argv) == 2 and sys.argv[1] in ('before01','after01')
    destination = OWN/('STATIC_'+sys.argv[1]+'.json'); assert not destination.exists()
    result = {'started':utc(),'argv':sys.argv,'status':'INCOMPLETE',
        'scope':'STATIC_PUBLIC_READ_HASH_DIFF_RECEIPT_ARCHIVE_ELF_ONLY',
        'candidate_compiler_generator_test_helper_runtime_execution':False,
        'target_network_private_IO':False,'policy_rejection_observed':False,'listener_composition':False}
    try:
        main(result)
    except BaseException as exc:
        result.update(status='COLLECTION ERROR; NOT CANDIDATE FAILURE',error=type(exc).__name__+': '+str(exc),traceback=traceback.format_exc())
        raise
    finally:
        result.update(input_pins=dict(sorted(seen.items())),input_count=len(seen),input_aggregate=aggregate(seen),ended=utc())
        with destination.open('x') as out:
            json.dump(result,out,indent=2); out.write('\n')
        print(destination)
