#!/usr/bin/env python3
"""Independent public static read/hash/diff/receipt/ar/ELF collection. No subprocesses or candidate imports."""
from collections import Counter
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
APP = ROOT/'apps/rtsp-auth/prestarted-owned-listener'
DOC = ROOT/'docs/qualification-plans/rtsp-prestarted-owned-listener-20261022'
RUN = DOC/'runs/build01'
SOURCE = '09325264c224a43b5664b48123bce1d05ea1269e6f209dcd2c9967424fdcba33'
SOURCE_SEAL = 'fa9d056819662781a46307d94a10b4d3ce820f0eb24678fc0e591371b52ba95b'
BUILD_SEAL = '0b7f3cce05f024a782357d1903312013a4e765fb749d9ba7a7ff271d04d67d19'
PROJECT = ('camera_','candidate_','owned_','rv_','is_rv_','mbedtls_')
seen = {}


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def path_of(path):
    p = Path(path)
    if not p.is_absolute():
        p = ROOT/p
    name = str(p)
    assert not name.startswith(('/proc/','/sys/','/dev/'))
    assert not any(s in name for s in ('encoder-query77-client','/.git/','/AGENTS.md','/docs/plan/',
        '/evidence/','/private/','/raw/','kernel76-station-attach-rx'))
    assert p.is_file(), name
    return p


def record(p,raw=None):
    p = path_of(p); h = hashlib.sha256(); count = 0
    if raw is None:
        with p.open('rb') as stream:
            while True:
                chunk = stream.read(65536)
                if not chunk:
                    break
                h.update(chunk); count += len(chunk)
    else:
        h.update(raw); count = len(raw)
    value = {'bytes':count,'sha256':h.hexdigest()}; name = str(p)
    assert name not in seen or seen[name] == value, 'changed during collection: '+name
    seen[name] = value
    return value


def read(path):
    p = path_of(path); raw = p.read_bytes(); record(p,raw); return raw


def pin(path):
    return record(path)


def check(path,expected):
    value = pin(path)
    expected = {'sha256':expected} if isinstance(expected,str) else expected
    assert all(value[k] == expected[k] for k in ('bytes','sha256') if k in expected), str(path)
    return value


def load(path):
    return json.loads(read(path))


def aggregate(entries):
    h = hashlib.sha256()
    for name,item in sorted(entries.items()):
        h.update(name.encode()+b'\0'+bytes.fromhex(item['sha256']))
    return h.hexdigest()


def ar_members(raw):
    assert raw[:8] == b'!<arch>\n'
    pos,longs,members = 8,b'',[]
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
            members.append((name,body))
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
    names = raw[secs[shstr][4]:secs[shstr][4]+secs[shstr][5]]
    def string(data,off):
        return data[off:data.index(b'\0',off)].decode()
    tables = {}; definitions = []; undefined = []
    for index,s in enumerate(secs):
        _,kind,_,_,off,size,link,_,_,stride = s
        if kind != 2:
            continue
        strings = raw[secs[link][4]:secs[link][4]+secs[link][5]]; table = []
        for pos in range(off,off+size,stride):
            if bits == 64:
                name,info,_,ndx,_,length = struct.unpack_from('<IBBHQQ',raw,pos)
            else:
                name,_,length,info,_,ndx = struct.unpack_from('<IIIBBH',raw,pos)
            name = string(strings,name)
            table.append((name,ndx,info))
            if name and not ndx:
                undefined.append(name)  # Include GC-localized undefined names, not just globals.
            elif name and info>>4 in (1,2):
                definitions.append(name)
        tables[index] = table
    refs = set(); count = 0
    for s in secs:
        _,kind,_,_,off,size,link,_,_,stride = s
        if kind not in (4,9):
            continue
        for pos in range(off,off+size,stride):
            info = struct.unpack_from('<Q' if bits == 64 else '<I',raw,pos+(8 if bits == 64 else 4))[0]
            name,ndx,_ = tables[link][info>>(32 if bits == 64 else 8)]
            if name and not ndx:
                refs.add(name)
            count += 1
    return {'bits':bits,'machine':machine,'type':'REL','entry':entry,'program_headers':phnum,'flags':flags,
        'section_names':[string(names,s[0]) for s in secs],'definitions':definitions,
        'undefined':sorted(undefined),'referenced_undefined':sorted(refs),'relocation_count':count}


def delta(old,new,patch):
    original = old.decode().splitlines(True); patch = patch.decode().splitlines(True)
    assert patch[0].startswith('--- ') and patch[1].startswith('+++ ')
    result = []; cursor = 0; at = 2; hunks = 0
    while at < len(patch):
        m = re.fullmatch(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@.*\n',patch[at]); assert m
        start = max(int(m[1])-1,0); assert start >= cursor
        result += original[cursor:start]; cursor = start; at += 1; a = b = 0
        while at < len(patch) and not patch[at].startswith('@@ '):
            tag,body = patch[at][0],patch[at][1:]; assert tag in (' ','-','+')
            if tag in (' ','-'):
                assert original[cursor] == body; cursor += 1; a += 1
            if tag in (' ','+'):
                result.append(body); b += 1
            at += 1
        assert a == int(m[2] if m[2] is not None else 1) and b == int(m[4] if m[4] is not None else 1)
        hunks += 1
    result += original[cursor:]
    assert ''.join(result).encode() == new
    return hunks


def span(raw,start,end):
    return raw[raw.index(start):raw.index(end)]


def main(result):
    check(DOC/'SOURCE_SEAL01.json',SOURCE_SEAL); check(DOC/'BUILD_SEAL01.json',BUILD_SEAL)
    source = load(DOC/'SOURCE_SEAL01.json'); build = load(DOC/'BUILD_SEAL01.json'); report = load(RUN/'REPORT.json')
    actual = {str(p.relative_to(ROOT)):pin(p) for p in sorted(APP.iterdir()) if p.is_file()}
    assert len(actual) == 14 and actual == source['source_files'] and aggregate(actual) == SOURCE == source['source_tree_sha256']
    assert not any(p.is_symlink() for p in APP.iterdir())
    for name,item in source['evidence_pins'].items():
        check(name,item)
    assert len(source['evidence_pins']) == 888 and aggregate(source['evidence_pins']) == source['evidence_aggregate']
    check(RUN/'REPORT.json',build['report']); check(DOC/'COMPOSITION_CONTRACT.md',source['pre_code_contract'])
    baseline_specs = (
        ('rtsp-prestarted-backend-v2-20261022','SOURCE_SEAL01.json','2bfa06a6b239c0c62b1003df6ee0f0ef9dd17ef0088efad00cccaf1ada7e7a8a',ROOT/'apps/rtsp-auth/prestarted-backend-v2',DOC/'baseline-backend-v2',11,'5a42cdc9d76628031c5380edd2e3b2c4dd06aefc6675138c8e10e1b098a28868'),
        ('rtsp-owned-listener-20261022','FINAL_SEAL.json','835aeb64c0967dfe8cdef1d13d3e376a984bf3485bc09b882812b2a9be814ed0',ROOT/'apps/rtsp-auth/owned-listener-candidate',DOC/'baseline-listener',47,'febab8640027f71dfc8a1ec4aa53c26cba219048ca4bf74578050377101ce557'))
    baseline_records = []
    for directory,name,digest,origin,destination,count,expected in baseline_specs:
        sealpath = ROOT/'docs/qualification-plans'/directory/name; check(sealpath,digest); seal = load(sealpath)
        assert len(seal['source_files']) == count and aggregate(seal['source_files']) == expected
        for file,item in seal['source_files'].items():
            check(file,item); check(destination/Path(file).relative_to(origin.relative_to(ROOT)),item)
        baseline_records.append({'source_pin':expected,'files':count,'seal':pin(sealpath)})
    for directory,digest in (('rtsp-prestarted-backend-v2-review-20261022','b3310e3a85cc7295137fa46e3aca4aae8fb12653b7377a55ad9d433a42532292'),('rtsp-owned-listener-review-20261022','2f39d960ba6cff2b236d215d65dea206f0005fad0fc430a621e24de25bd3db61')):
        path = ROOT/'docs/qualification-plans'/directory/'FINAL_SEAL.json'; check(path,digest)
        seal = load(path)
        for table in ('input_pins','output_pins'):
            for file,item in seal[table].items():
                check(file,item)
    counts = {'inputs':337,'frozen':2669,'closure':308,'artifacts':34}; combined = {}
    for table in (*counts,'toolchain_pins'):
        if table in counts:
            assert len(report[table]) == counts[table]
        for name,item in report[table].items():
            combined[name] = check(name,item)
    assert combined == build['verified_recorded_pins'] and len(combined) == 2767
    for alias,target in report['frozen_tool_aliases'].items():
        assert str(Path(alias).resolve()) == target
    copies = {}
    for name,item in report['inputs'].items():
        path = Path(name); pairs = ((APP,RUN/'source/app'),(DOC/'baseline-provenance',RUN/'provenance'),
            (ROOT/'apps/auth-web/service/vendor/mbedtls',RUN/'source/vendor'),(ROOT/'framework',RUN/'source/framework'),
            (ROOT/'apps/rtsp-auth/isolated-verifier',RUN/'source/verifier'))
        if path == DOC/'COMPOSITION_CONTRACT.md':
            target = RUN/'provenance/COMPOSITION_CONTRACT.md'
        else:
            matches = [dst/path.relative_to(src) for src,dst in pairs if path.is_relative_to(src)]
            assert len(matches) == 1, name
            target = matches[0]
        copies[str(target)] = check(target,item)
    assert copies == build['input_snapshot_pins']
    for name,item in report['closure'].items():
        check(RUN/'dependencies'/Path(name).relative_to('/'),item)
    for name,item in build['runtime_license_pins'].items():
        check(DOC/'baseline-listener/frozen/runtime-licenses'/name,item)
        check(RUN/'source/runtime-licenses'/name,item)
    for p in sorted((DOC/'baseline-provenance').rglob('*')):
        if p.is_file():
            relative = p.relative_to(DOC/'baseline-provenance')
            roots = {'backend':'rtsp-prestarted-backend-v2-20261022','backend-review':'rtsp-prestarted-backend-v2-review-20261022',
                     'listener':'rtsp-owned-listener-20261022','listener-review':'rtsp-owned-listener-review-20261022'}
            origin = roots.get(relative.parts[0])
            if origin is None:
                assert relative.parts[0] == 'history'
                origin = 'rtsp-prestarted-backend-20261022' if p.name == 'SEAL_FAILURE01.json' else 'rtsp-prestarted-backend-review-20261022'
            check(p,pin(ROOT/'docs/qualification-plans'/origin/p.name))
    oldroot = DOC/'baseline-backend-v2'; listenerroot = DOC/'baseline-listener'
    unchanged = ['adapter_state.h','candidate_auth.c','delay_pin.h','host_pin.h','native_pin.h','server_crypto_config.h','verifier_adapter_overlay.c']
    for name in unchanged:
        assert read(APP/name) == read(oldroot/name)
    assert read(APP/'listener_custody.c') == read(listenerroot/'listener_custody.c')
    assert read(APP/'listener_custody.h') == read(listenerroot/'listener_custody.h').replace(b'#include "owned_listener_v1.h"',b'#include "prestarted_owned_listener_v1.h"')
    deltas = {}
    for name in ('candidate_auth.h','rtsp_server_candidate.c','README.md','build.py'):
        deltas[name] = delta(read(oldroot/name),read(APP/name),read(RUN/'logs'/('delta-'+name+'.txt')))
    deltas['listener_custody.h'] = delta(read(listenerroot/'listener_custody.h'),read(APP/'listener_custody.h'),read(RUN/'logs/delta-listener_custody.h.txt'))
    deltas['prestarted_owned_listener_v1.h'] = delta(b'',read(APP/'prestarted_owned_listener_v1.h'),read(RUN/'logs/delta-new-source-api.txt'))
    old,new = read(oldroot/'rtsp_server_candidate.c'),read(APP/'rtsp_server_candidate.c')
    prefix = span(new,b'// SPDX',b'static int serve_prestarted_body(').replace(b'#include "listener_custody.h"\n',b'').replace(b'/* Serialized synchronous entry only, not a thread lock or ownership grant. */\nstatic int owned_entry_active;\n',b'')
    assert prefix == span(old,b'// SPDX',b'static int serve_prestarted_body(')
    end = b'else if(!retirement_failed){int retired=idle_checked(p);if(retired!=CAMERA_OK)result=retired;}'
    assert span(old,b'while(!*o->stop){',end)+end == span(new,b'while(!*o->stop){',end)+end
    h_old,h_new = read(oldroot/'candidate_auth.h'),read(APP/'candidate_auth.h')
    assert h_old[:h_old.index(b'/* Consumes one serve attempt.')] == h_new[:h_new.index(b'/* The ONLY serve entry')]
    assert h_old[h_old.index(b'/* Explicit one attempt,'):] == h_new[h_new.index(b'/* Explicit one attempt,'):]
    vendor = ROOT/'apps/auth-web/service/vendor/mbedtls'
    vendor_pins = {str(p.relative_to(vendor)):pin(p) for p in sorted(vendor.rglob('*')) if p.is_file()}
    assert len(vendor_pins) == 277 and aggregate(vendor_pins) == report['vendor_pin'] == 'bab965e248c4b691cc84e8bccc7a6bd40101305b8ea789408896b64bfe2281f5'
    assert report['status'] == 'HOST_ARM_CHANGED_UNITS_COMPILE_ARCHIVE_REL_PASS_NOT_RUN' and len(report['commands']) == 58
    assert not report['programs_tests_generators_listeners_executed'] and not report['native_or_composite_acceptance']
    for c in report['commands']:
        check(c['log'],c['log_pin']); assert c['cwd'] == str(ROOT)
        assert c['exit'] == (1 if Path(c['argv'][0]).name == 'diff' else 0)
    compiles = [c for c in report['commands'] if '-c' in c['argv']]
    assert len(compiles) == 30 and all(c['log_pin']['bytes'] == 0 for c in compiles)
    common = ['-std=gnu11','-Wall','-Wextra','-Werror','-fno-common','-fno-strict-aliasing',
        '-ffunction-sections','-fdata-sections','-fstack-protector-strong','-D_FORTIFY_SOURCE=2','-fstack-usage','-DMBEDTLS_CONFIG_FILE="server_crypto_config.h"']
    common += ['-I'+str(RUN/'source'/n) for n in ('app','framework/include','verifier','vendor/include','vendor/library')]
    roots = ('camera_rtsp_prestarted_owned_listener_serve_v1','candidate_prestarted_start','candidate_prestarted_finish',
        'candidate_backend_revoke','candidate_backend_cancel','is_rv_ipc_close_outcome','is_rv_context_bytes')
    forbidden = ('main','camera_rtsp_isolated_candidate_serve','camera_rtsp_prestarted_candidate_serve',
        'camera_rtsp_owned_listener_serve_v1','candidate_backend_init','candidate_backend_destroy','candidate_backend_finish')
    sources = [RUN/'source/app'/(n+'.c') for n in ('candidate_auth','rtsp_server_candidate','verifier_adapter_overlay','listener_custody')]
    sources += [RUN/'source/verifier/ipc.c']
    sources += [RUN/'source/framework/src'/(n+'.c') for n in ('rtsp_digest_wire','rtsp_request','rtsp_input','backend','packet_hub','producer','rtp_h264')]
    sources += [RUN/'source/vendor/library'/(n+'.c') for n in ('sha256','constant_time','platform_util')]
    closure = {}; metadata = {}
    for platform in ('host','arm'):
        tools = report['tools'][platform]
        for item in tools.values():
            check(item['path'],item)
        flags = ['-O2'] if platform == 'host' else ['-Os','-march=armv5te','-marm','-mfloat-abi=soft']
        cmds = [c['argv'] for c in compiles if Path(c['log']).name.startswith(platform+'-')]
        assert len(cmds) == 15
        objects = []; object_meta = {}; project_definitions = []
        for index,(src,cmd) in enumerate(zip(sources,cmds)):
            obj = RUN/'objects'/(platform+'-'+str(index)+'-'+src.stem+'.o')
            exception = ['-Wno-misleading-indentation'] if src.stem in ('rtsp_server_candidate','rtsp_input') else []
            if src.stem == 'rtsp_input':
                exception.append('-Wno-error=nonnull')
            assert cmd == [tools['cc']['path'],*common,*flags,*exception,'-MD','-MF',str(obj.with_suffix('.d')),'-c',str(src),'-o',str(obj)]
            assert not any('PUBLIC_ONLY' in a or 'FIXTURE' in a or 'RV_TEST' in a for a in cmd)
            words = shlex.split(read(obj.with_suffix('.d')).decode().replace('\\\n','').split(':',1)[1]); assert str(src) in words
            for name in words:
                closure[name] = pin(name)
            objects.append(obj); parsed = elf(read(obj)); object_meta[obj.name] = parsed
            assert parsed['machine'] == (62 if platform == 'host' else 40)
            project_definitions.extend(n for n in parsed['definitions'] if n.startswith(PROJECT))
        assert all(v == 1 for v in Counter(project_definitions).values())
        archive = RUN/('libprestarted-owned-listener-'+platform+'.a')
        members = ar_members(read(archive)); assert [n for n,_ in members] == [p.name for p in objects]
        assert all(raw == read(path) for (_,raw),path in zip(members,objects))
        assert any(c['argv'] == [tools['ar']['path'],'rcsD',str(archive),*map(str,objects)] for c in report['commands'])
        reloc = RUN/('prestarted-owned-listener-'+platform+'-NOT-EXECUTABLE.o')
        expected_link = [tools['cc']['path'],*flags,'-nostdlib','-r',*map(str,objects),'-Wl,--gc-sections',*('-Wl,--undefined='+n for n in roots),'-Wl,-Map='+str(RUN/(platform+'.map')),'-o',str(reloc)]
        assert any(c['argv'] == expected_link for c in report['commands'])
        maploads = re.findall(r'^LOAD (.+)$',read(RUN/(platform+'.map')).decode(),re.M)
        assert maploads == [*map(str,objects),*(['linker stubs'] if platform == 'arm' else [])]
        parsed = elf(read(reloc)); definitions = Counter(parsed['definitions'])
        assert parsed['machine'] == (62 if platform == 'host' else 40)
        required = (*roots,'candidate_backend_poll','candidate_peer_open','candidate_peer_close','rv_start','rv_stop','rv_destroy','is_rv_running','owned_listener_observe','owned_listener_move','owned_listener_close_once')
        assert all(definitions[n] == 1 for n in required) and not any(definitions[n] for n in forbidden)
        assert not any(n.startswith(PROJECT) for n in parsed['referenced_undefined'])
        expected_undefined = report['undefined'][platform]
        assert parsed['undefined'] == sorted(expected_undefined['symbol_table'])
        assert parsed['referenced_undefined'] == sorted(expected_undefined['referenced_by_relocation'])
        unused = sorted(set(parsed['undefined'])-set(parsed['referenced_undefined']))
        assert unused == sorted(expected_undefined['unreferenced_symbol_table_only']) == ['camera_rtsp_digest_close','camera_rtsp_digest_issue','camera_rtsp_digest_verify']
        logdefined = sorted(line.split()[-1] for line in read(RUN/'logs'/(platform+'-defined.txt')).decode().splitlines() if line.strip())
        assert logdefined == sorted(parsed['definitions'])
        metadata[platform] = {'members':{p.name:pin(p) for p in objects},'object_ELF':object_meta,'archive':pin(archive),
            'relocatable':pin(reloc),'ELF':parsed,'map_loads':maploads,'project_definitions_once':sorted(project_definitions),'GC_unreferenced_undefined':unused}
    assert closure == report['closure']
    for name,script in (('SEAL_FAILURE01.json','seal.py'),('SEAL_FAILURE02.json','seal02.py')):
        failure = load(DOC/name); check(DOC/script,failure['initial_finalizer_sha256'])
        assert not failure['production_or_build_source_changed'] and not failure['repeat_compilation'] and not failure['policy_rejection_or_reroute']
    pin(DOC/'seal03.py')
    result.update(status='STATIC COMPOSITION PIN/RECEIPT/ARCHIVE/ELF CHECK PASS; NOT EXECUTION',
        source_pin=SOURCE,source_count=14,source_seal_pin=SOURCE_SEAL,build_seal_pin=BUILD_SEAL,baseline_records=baseline_records,
        author_evidence_count=888,author_evidence_aggregate=source['evidence_aggregate'],counts=counts,
        author_verified_union=2767,maps_overlap_not_summed=True,delta_hunks=deltas,compiled_units=30,
        recorded_commands=report['commands'],author_started=report['started'],author_ended=report['ended'],
        rerun_compiler=False,platform_metadata=metadata)


if __name__ == '__main__':
    assert len(sys.argv) == 2 and sys.argv[1] in ('before01','after01')
    destination = OWN/('STATIC_'+sys.argv[1]+'.json'); assert not destination.exists()
    result = {'started':utc(),'argv':sys.argv,'status':'INCOMPLETE',
        'scope':'STATIC_PUBLIC_READ_HASH_DIFF_RECEIPT_ARCHIVE_ELF_ONLY',
        'candidate_helper_compiler_generator_test_guard_listener_app_client_execution':False,
        'target_network_private_IO':False,'policy_rejection_observed':False}
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
