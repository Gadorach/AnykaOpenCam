#!/usr/bin/env python3
"""Independent static public file/hash/receipt/ELF/ar inspection; no subprocesses or candidate imports."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import struct
import sys
import traceback

OWN = Path(__file__).resolve().parent
ROOT = OWN.parents[2]
APP = ROOT/'apps/qualification/encoder-query77-client'
DOC = ROOT/'docs/qualification-plans/encoder-query77-client-20261022'
BUILD = APP/'build01'
K = ROOT/'work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux'
PLATFORM = K/'arch/arm/mach-ak3918ev200'
SOURCE = '101afbb482c4562d02e58ab7c2aafed0d3cbb76a7d7b4cc36cf08e667fc80600'
QUERY = 'ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520'
seen = {}


def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def path_of(path):
    p = Path(path)
    if not p.is_absolute():
        p = ROOT/p
    name = str(p)
    assert not name.startswith(('/proc/','/sys/','/dev/'))
    assert not any(s in name for s in ('prestarted-owned-listener','owned-listener-candidate',
        '/.git/','/AGENTS.md','/docs/plan/','/evidence/','/private/','/raw/','kernel76-station-attach-rx'))
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
    value = {'bytes':count,'sha256':h.hexdigest()}
    if p.is_symlink():
        value.update(symlink=os.readlink(p),resolved=str(p.resolve()))
    name = str(p)
    assert name not in seen or seen[name] == value, 'changed during collection: '+name
    seen[name] = value
    return value


def read(path):
    p = path_of(path); raw = p.read_bytes(); record(p,raw); return raw


def pin(path):
    return record(path)


def check(path,expected):
    actual = pin(path)
    expected = {'sha256':expected} if isinstance(expected,str) else expected
    assert all(actual.get(k) == v for k,v in expected.items() if k in ('bytes','sha256','symlink','resolved')), str(path)
    return actual


def load(path):
    return json.loads(read(path))


def aggregate(entries):
    h = hashlib.sha256()
    for name,item in sorted(entries.items()):
        h.update(name.encode()+b'\0'+bytes.fromhex(item['sha256']))
    return h.hexdigest()


def elf(raw,expected_type):
    assert raw[:4] == b'\x7fELF' and raw[4] in (1,2) and raw[5:7] == b'\x01\x01'
    bits = 64 if raw[4] == 2 else 32
    h = struct.unpack_from('<HHIQQQIHHHHHH' if bits == 64 else '<HHIIIIIHHHHHH',raw,16)
    typ,machine,_,entry,phoff,shoff,flags,_,phsize,phnum,shsize,shnum,shstr = h
    assert typ == expected_type
    segments = []
    for i in range(phnum):
        pos = phoff+i*phsize
        if bits == 64:
            kind,flg,off,virt,phys,filesz,memsz,align = struct.unpack_from('<IIQQQQQQ',raw,pos)
        else:
            kind,off,virt,phys,filesz,memsz,flg,align = struct.unpack_from('<IIIIIIII',raw,pos)
        assert kind != 3, 'PT_INTERP'
        assert off+filesz <= len(raw)
        if kind == 1:
            assert not flg&1 or not flg&2, 'writable executable LOAD'
        if kind == 0x6474e551:
            assert flg == 6, 'stack not RW/nonexecuting'
        segments.append({'kind':kind,'offset':off,'virtual_start':virt,'file_bytes':filesz,'memory_bytes':memsz,'flags':flg,'alignment':align})
    fmt = '<IIQQQQIIQQ' if bits == 64 else '<IIIIIIIIII'
    assert shsize == struct.calcsize(fmt)
    sections = [struct.unpack_from(fmt,raw,shoff+i*shsize) for i in range(shnum)]
    names = raw[sections[shstr][4]:sections[shstr][4]+sections[shstr][5]]
    def string(data,off):
        return data[off:data.index(b'\0',off)].decode()
    assert not any(s[1] == 6 for s in sections), 'dynamic section'
    symbols = []
    for s in sections:
        _,kind,_,_,off,size,link,_,_,stride = s
        if kind != 2:
            continue
        strings = raw[sections[link][4]:sections[link][4]+sections[link][5]]
        for pos in range(off,off+size,stride):
            if bits == 64:
                name,info,_,ndx,value,length = struct.unpack_from('<IBBHQQ',raw,pos)
            else:
                name,value,length,info,_,ndx = struct.unpack_from('<IIIBBH',raw,pos)
            symbols.append({'name':string(strings,name),'binding':info>>4,'type':info&15,'section':ndx,'bytes':length})
    if expected_type == 2:
        assert any(p['kind'] == 1 and p['flags']&1 and p['virtual_start'] <= entry < p['virtual_start']+p['memory_bytes'] for p in segments)
        assert any(p['kind'] == 0x6474e551 for p in segments)
        assert not any(s['name'] and not s['section'] and s['binding'] == 1 for s in symbols), 'strong undefined symbol'
    assert any(s['name'] == 'main' and s['section'] and s['bytes'] > 0 for s in symbols)
    loads = [{k:p[k] for k in ('virtual_start','file_bytes','memory_bytes','flags')} for p in segments if p['kind'] == 1]
    return {'bits':bits,'type':'EXEC' if expected_type == 2 else 'REL','machine':machine,'entry':entry,'flags':flags,
        'segments':segments,'loads':loads,'load_memory_bytes_sum':sum(p['memory_bytes'] for p in loads),
        'section_names':[string(names,s[0]) for s in sections],
        'main':[s for s in symbols if s['name'] == 'main'],
        'strong_undefined':[s['name'] for s in symbols if s['name'] and not s['section'] and s['binding'] == 1]}


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
            members.setdefault(name,[]).append({'bytes':len(body),'sha256':hashlib.sha256(body).hexdigest()})
        pos += 60+size+size%2
    assert pos == len(raw)
    return members


def deps(path):
    return shlex.split(read(path).decode().replace('\\\n',' ').split(':',1)[1])


def main(result):
    handoff = load(DOC/'HANDOFF_SEAL01.json'); offline = load(DOC/'OFFLINE_CLOSURE01.json')
    report = load(BUILD/'REPORT.json'); inputs = load(BUILD/'INPUTS.json'); pre = load(BUILD/'PRE_INPUTS.json'); outputs = load(BUILD/'OUTPUT_SEAL.json')
    for name,item in handoff['documentation_files'].items():
        check(name,item)
    for name,key in (('REPORT.json','local_build_report'),('INPUTS.json','local_build_inputs'),('OUTPUT_SEAL.json','local_build_output_seal')):
        check(BUILD/name,handoff[key])
    actual = {str(p.relative_to(ROOT)):pin(p) for p in sorted(APP.iterdir()) if p.is_file()}
    assert len(actual) == 5 and actual == handoff['source_files'] == offline['source_files']
    assert aggregate(actual) == SOURCE == handoff['source_tree_sha256'] == offline['source_tree_sha256']
    assert len(inputs) == 3710 and len(outputs) == 252 and len(pre) == 3572
    for name,item in inputs.items():
        check(name,item)
    assert all(name in inputs and inputs[name] == item for name,item in pre.items())
    for name,item in outputs.items():
        assert not Path(name).is_absolute() and '..' not in Path(name).parts
        check(BUILD/name,item)
    check(BUILD/'OUTPUT_SEAL.json',offline['build_output_seal_verified']); check(BUILD/'REPORT.json',offline['build_report'])
    frozen = load(ROOT/'docs/qualification-plans/encoder-identity-20261022/SOURCE_SEAL01.json')
    assert len(frozen['source_files']) == 5 and aggregate(frozen['source_files']) == QUERY == report['query_source_pin']
    for name,item in frozen['source_files'].items():
        check(name,item)
    original = ROOT/'apps/qualification/kernel76-encoder-identity/encoder-open-query.h'
    assert read(APP/'encoder-open-query.h') == read(original) == read(PLATFORM/'encoder-open-query.h')
    for name in ('qc07f_hwenc.c','qc07f_hwenc.h','encoder-open-query.h','encoder-open-query.inc'):
        if name in ('encoder-open-query.h','encoder-open-query.inc'):
            expected = frozen['source_files']['apps/qualification/kernel76-encoder-identity/'+name]
        else:
            expected = frozen['evidence_pins']['docs/qualification-plans/encoder-identity-20261022/runs/build01/candidate/'+name]
        check(PLATFORM/name,expected)
    binding = load(ROOT/'docs/qualification-plans/kernel76-transport-20261021/recovery-source-review/binding.json')
    for suffix in ('arch/arm/mach-ak3918ev200/watchdog.c','drivers/watchdog/watchdog_dev.c','drivers/watchdog/watchdog_core.c'):
        matches = [v for n,v in binding['input_sha256'].items() if n.endswith('/'+suffix)]
        assert matches and all(pin(K/suffix)['sha256'] == v for v in matches)
    image = ROOT/'work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-images02/QC6EQ77.UIM'
    check(image,'f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e')
    assert not any(report[k] for k in ('outputs_executed','tests_executed','target_private_IO','native_admitted','Git_index_central_plan_change'))
    assert report['status'] == 'PASS' and report['all_captured_inputs_unchanged'] and len(report['commands']) == 36
    for command in report['commands']:
        assert command['exit'] == 0; check(command['log'],command['log_pin'])
        pin(command['argv'][0])
    toolchain = Path(report['toolchain_root'])
    for name,digest in report['retained_ARM_pins'].items():
        check(toolchain/name,digest)
    initial = load(BUILD/'LOADED_TOOL_ELF_FINAL.json'); assert len(initial) == 2 == report['loaded_tool_elf_paths']
    loaded = {}; transfers = set()
    for path in sorted((BUILD/'logs').glob('*-loader.*')):
        text = read(path).decode()
        for match in re.finditer(r'(?:calling init|initialize program|transferring control):\s+(\S+)',text):
            name = match[1]
            if not name.startswith('/'):
                name = str(ROOT/name) if '/' in name else '/usr/bin/'+name
            loaded[name] = pin(name)
        transfers.update(re.findall(r'transferring control:\s+(\S+)',text))
    for name,item in initial.items():
        loaded[name] = check(name,item)
    assert loaded == offline['loaded_tool_elf_final_pins'] and len(loaded) == 58
    assert sorted(transfers) == offline['observed_transfer_names'] and len(transfers) == 13
    assert offline['loader_pins_are_final_observed_not_all_preuse']
    platforms = {}; archives_cache = {}
    common = ['-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-fno-common','-fno-strict-aliasing',
        '-ffunction-sections','-fdata-sections','-fstack-protector-strong','-fstack-usage',
        '-Wframe-larger-than=8192','-Wstack-usage=8192','-I'+str(APP),'-ffile-prefix-map='+str(ROOT)+'=WORKSPACE']
    for arch,count in (('host',119),('arm',37)):
        cc = '/usr/bin/cc' if arch == 'host' else str(toolchain/'bin/arm-buildroot-linux-musleabi-gcc')
        flags = ['-O2'] if arch == 'host' else ['-Os','-march=armv5te','-marm','-mfloat-abi=soft']
        directory = BUILD/arch; source = APP/'encoder-query77-qualification.c'; obj = directory/'encoder-query77-qualification.o'; binary = directory/'encoder-query77-qualification'
        expected = ([cc,*common,*flags,'-M','-MF',str(directory/'pre.d'),str(source)],
            [cc,*common,*flags,'-MD','-MF',str(directory/'actual.d'),'-c',str(source),'-o',str(obj)],
            [cc,*flags,'-static','-no-pie',str(obj),'-Wl,--gc-sections','-Wl,-Map='+str(directory/'link.map'),'-Wl,-t','-o',str(binary)])
        assert all(any(c['argv'] == cmd for c in report['commands']) for cmd in expected)
        before,after = deps(directory/'pre.d'),deps(directory/'actual.d')
        assert len(before) == count and set(before) == set(after)
        for name in before:
            assert name in inputs
            check(BUILD/'inputs'/Path(name).absolute().relative_to('/'),inputs[name])
        stack = []
        for line in read(directory/'encoder-query77-qualification.su').decode().splitlines():
            location,frame,kind = line.split('\t'); assert kind == 'static'
            stack.append({'location':location,'frame_bytes':int(frame)})
        assert stack == offline['authored_stack_records'][arch]['records']
        assert max(x['frame_bytes'] for x in stack) == (1504 if arch == 'host' else 1496)
        check(binary,report['artifacts'][arch]); parsed = elf(read(binary),2); object_meta = elf(read(obj),1)
        assert parsed['machine'] == object_meta['machine'] == (62 if arch == 'host' else 40)
        assert parsed['loads'] == offline['static_loads'][arch]['segments']
        assert parsed['load_memory_bytes_sum'] == offline['static_loads'][arch]['load_memory_bytes_sum']
        if arch == 'arm':
            assert parsed['flags']&0xff000000 == 0x05000000 and parsed['flags']&0x200 and not parsed['flags']&0x400
            assert 'Tag_CPU_arch: v5TE' in read(BUILD/'logs/arm-elf.txt').decode()
        linkmap = read(directory/'link.map').decode(); trace = read(BUILD/'logs'/(arch+'-static-link.txt')).decode()
        link_inputs = set(re.findall(r'^LOAD (.+)$',linkmap,re.M))
        link_inputs.update(line.strip() for line in trace.splitlines() if line.strip().startswith('/'))
        for name in link_inputs:
            if arch == 'arm' and name == 'linker stubs':
                continue  # BFD-generated LOAD label, not a filesystem input.
            p = Path(name)
            if p.is_relative_to(BUILD):
                pin(p)
            else:
                assert name in inputs, name
                check(p,inputs[name])
        selected = {}
        for archive,member in set(re.findall(r'^(/\S+\.a)\(([^)]+)\)',linkmap,re.M)):
            assert archive in inputs
            if archive not in archives_cache:
                archives_cache[archive] = ar_members(read(archive))
            assert member in archives_cache[archive], (archive,member)
            selected.setdefault(archive,{})[member] = archives_cache[archive][member]
        assert selected, 'no real static archive association'
        platforms[arch] = {'artifact':pin(binary),'ELF':parsed,'object_ELF':object_meta,
            'consumed_dependencies':count,'authored_stack':stack,'link_inputs':sorted(link_inputs),'linkmap_selected_archive_members':selected}
    # Directly read/pin inspected public source/API files, including additive trace UAPI.
    for path in (PLATFORM/'stage-telemetry-uapi.h',K/'include/linux/watchdog.h'):
        pin(path)
    result.update(status='STATIC PUBLIC PIN/RECEIPT/ELF/ARCHIVE VERIFICATION PASS; NOT EXECUTION',
        source_pin=SOURCE,source_count=5,query_source_pin=QUERY,original_inputs=3710,pre_inputs=3572,
        original_outputs=252,final_loader_path_records=58,observed_transfer_spellings=13,
        loader_closure_final_observed_not_all_preuse=True,compiler_commands=report['commands'],
        author_started=report['started'],author_ended=report['ended'],rerun_compiler=False,platforms=platforms)


if __name__ == '__main__':
    assert len(sys.argv) == 2 and sys.argv[1] in ('before01','after01','before02','after02','before03','after03')
    destination = OWN/('STATIC_'+sys.argv[1]+'.json'); assert not destination.exists()
    result = {'started':utc(),'argv':sys.argv,'status':'INCOMPLETE',
        'scope':'STATIC_PUBLIC_READ_HASH_RECEIPT_ELF_ARCHIVE_ONLY',
        'candidate_script_helper_compiler_generator_test_guard_listener_client_execution':False,
        'target_network_private_IO':False,'policy_rejection_observed':False,'other_lane_inspection_or_composition':False}
    try:
        main(result)
    except BaseException as exc:
        result.update(status='COLLECTION ERROR; NOT CANDIDATE FAILURE',error=type(exc).__name__+': '+str(exc),traceback=traceback.format_exc())
        raise
    finally:
        result.update(input_pins=dict(sorted(seen.items())),input_count=len(seen),input_aggregate=aggregate(seen),ended=utc())
        with destination.open('x') as stream:
            json.dump(result,stream,indent=2); stream.write('\n')
        print(destination)
