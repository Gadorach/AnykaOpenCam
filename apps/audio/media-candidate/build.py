#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Bounded HOST audio vector/span/timing proof, fresh own outputs, no devices."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import wave

ROOT=Path(__file__).resolve().parents[3]
APP=Path(__file__).resolve().parent
LANE=ROOT / "evidence/delegated/20261012-audio-media"
CAPTURE=ROOT / "bringup/qc07f/audio-candidate"
MMC=ROOT / "evidence/delegated/20261012-mmc-candidate/build-20261012T174515.810242Z"
LINUX=MMC / "linux-scratch"
OWN=["g711.h","g711.c","audio_packet.h","audio_packet.c","test_audio_packet.c","test_v1_preborrow.c","oracle.py","header_link.cpp","size_probe.c","README.md","build.py"]
CAPTURE_PINS={"capture_core.h":"89be0fa8a9ba190a79fed7beaa29fc0e80403b84dc557a8290e142c899fcb99e",
 "capture_core.c":"1b3ae2588dbdbfbb929ceae397d2e16793fe983fe170b52f4134c47601033bf2"}
POLICY=["framework/include/camera/backend.h","framework/include/camera/access_policy.h","framework/src/access_policy.c"]
ARM_PINS={"bin/toolchain-wrapper":"f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1",
 "bin/arm-buildroot-linux-musleabi-gcc.br_real":"e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729",
 "arm-buildroot-linux-musleabi/sysroot/lib/libc.a":"0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5",
 "lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a":"f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a"}


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("run");parser.add_argument("--toolchain",type=Path,required=True)
    args=parser.parse_args();args.toolchain=args.toolchain.expanduser().resolve()
    if not args.run or not all(c.isalnum() or c in "-_" for c in args.run):parser.error("fresh simple run name required")
    LANE.mkdir(parents=True,exist_ok=True)
    if LANE.resolve()!=LANE:raise RuntimeError("Evidence symlink refused")
    out=LANE / args.run;out.mkdir()
    for directory in ("input","upstream","objects","logs","tmp","vectors-optimized","vectors-sanitized"):(out / directory).mkdir()
    env={"PATH":os.environ.get("PATH","/usr/bin:/bin"),"LC_ALL":"C","TMPDIR":str(out / "tmp"),
         "ASAN_OPTIONS":"detect_leaks=1:halt_on_error=1","UBSAN_OPTIONS":"halt_on_error=1:print_stacktrace=1"}
    report={"started_utc":now(),"command":["python3",str(Path(__file__).relative_to(ROOT)),*os.sys.argv[1:]],"python":os.sys.version,
            "commands":[],"inputs":{},"state":"RUNNING","version":"v2-preborrow-authority-fresh-clock","tasks":["AUDIO-04","AUDIO-05","AUDIO-06"],
            "native_microphone":False,"actual_audio_files":False,"synthetic_vector_files_only":True,"native_qualified":False,
            "browser_MSE_audio":False,"track_SDP_or_service_enabled":False,"G4":False,"G7":False,"G8":False,"AV_150ms_passed":False}
    def run(label,argv,timeout=120,expected_exit=0):
        argv=list(map(str,argv));log=out / "logs" / (label+".txt")
        record={"argv":argv,"cwd":str(out),"started_utc":now(),"timeout_seconds":timeout,"expected_exit":expected_exit,"log":str(log.relative_to(ROOT))};report["commands"].append(record)
        with log.open("xb") as f:r=subprocess.run(argv,cwd=out,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
        record.update(exit_code=r.returncode,finished_utc=now(),sha256=sha(log))
        if r.returncode!=expected_exit:raise RuntimeError("Failed command preserved: "+str(log))
        return log.read_text()
    def copy(p,dest,kind):
        dest.parent.mkdir(parents=True,exist_ok=True)
        with dest.open("xb") as f:f.write(p.read_bytes())
        report["inputs"][str(p.relative_to(ROOT))]={"sha256":sha(dest),"bytes":dest.stat().st_size,"class":kind,"copy":str(dest.relative_to(ROOT))}
    def compiler(cc,label):
        report.setdefault("compilers",{})[label]={"version":run(label+"-version",[cc,"--version"]),"target":run(label+"-target",[cc,"-dumpmachine"]),"sha256":sha(Path(cc).resolve())}
        for name in ("cc1","cc1plus","as","ld","libgcc.a","libasan.so","libubsan.so","libstdc++.so","crt1.o","Scrt1.o","crti.o","crtbeginS.o","crtbeginT.o","crtend.o","crtn.o","libc.a","libc.so"):
            flag="-print-prog-name=" if name in ("cc1","cc1plus","as","ld") else "-print-file-name="
            value=run(label+"-closure-"+name.replace('.','_'),[cc,flag+name]).strip();p=Path(value) if '/' in value else Path(shutil.which(value) or value)
            if p.is_file():report.setdefault("compiler_runtime_closure",{})[str(p.resolve())]=sha(p)
    try:
        before={str(APP / n):sha(APP / n) for n in OWN}
        preservation=json.loads((LANE / 'v1-snapshot/PRESERVATION.json').read_text())
        for name in ('audio_packet.c','audio_packet.h'):
            relative='apps/audio/media-candidate/'+name;original=ROOT / preservation['files'][relative]['copy']
            if sha(original)!=preservation['files'][relative]['sha256']:raise RuntimeError('Preserved V1 packet changed')
            copy(original,out / 'input/v1' / name,'exact preserved V1 regression source')
        for n in OWN:copy(APP / n,out / "input" / n,"owned source, per-file license")
        for n,expected in CAPTURE_PINS.items():
            if sha(CAPTURE / n)!=expected:raise RuntimeError("Reviewed capture V2 pin changed")
            copy(CAPTURE / n,out / "input/capture-contract" / n,"exact reviewed capture V2 GPL source, read-only")
        policy_before={p:sha(ROOT / p) for p in POLICY}
        for p in POLICY:copy(ROOT / p,out / "input/policy" / p,"read-only shared real policy")
        original=LINUX / "sound/core/oss/mulaw.c";expected="1f82a1030f6bfb384552ddd91cf96e3d9070197cb4691e46fedc7a1abf07e4cf"
        if sha(original)!=expected:raise RuntimeError("Licensed reference source pin mismatch")
        copy(original,out / "upstream/mulaw.c","unmodified LGPL2-or-later original")
        for license_name in ("LGPL-2.0","GPL-2.0"):copy(LINUX / "LICENSES/preferred" / license_name,out / "upstream" / license_name,"original complete license text")
        lines=original.read_text().splitlines(True)
        leaves=''.join(lines[:22]+lines[28:135])
        if "static unsigned char linear2ulaw" not in leaves or "static int ulaw2linear" not in leaves:raise RuntimeError("Reference leaf audit failed")
        reference=out / "input/mulaw-reference.c"
        with reference.open("x") as f:f.write(leaves+'\nunsigned char reference_mu_encode(int value){return linear2ulaw(value);}\nint reference_mu_decode(unsigned char code){return ulaw2linear(code);}\n')
        report["reference_subset"]={"source_sha256":expected,"lines":"1-22 +29-135","subset_sha256":sha(reference),"license":"LGPL-2.0-or-later notice preserved; reference math only, not firmware execution"}
        for p,expected in ARM_PINS.items():
            if sha(args.toolchain / p)!=expected:raise RuntimeError("Pinned ARM toolchain mismatch")
        report["arm_toolchain_pins"]=ARM_PINS;report["toolchain_normalized"]=str(args.toolchain)
        run("independent-oracle",["python3",out / "input/oracle.py",out / "oracle"],timeout=60)
        common=["-std=c11","-Wall","-Wextra","-Werror","-Wconversion","-Wshadow","-fno-common","-fstack-usage","-I"+str(out / "input"),
                "-I"+str(out / "input/capture-contract"),"-I"+str(out / "input/policy/framework/include")]
        sources=[out / "input/g711.c",out / "input/audio_packet.c",out / "input/capture-contract/capture_core.c",
                 out / "input/policy/framework/src/access_policy.c",reference,out / "input/test_audio_packet.c"]
        compiler("/usr/bin/cc","host");compiler("/usr/bin/c++","cxx")
        def build(cc,label,flags,execute):
            folder=out / "objects" / label;folder.mkdir();objects=[]
            for i,p in enumerate(sources):
                obj=folder / (str(i)+'-'+p.stem+'.o')
                # Original Sun/ALSA reference keeps its int/uchar conversions;
                # selected leaves are unmodified; strict new code stays Wconversion.
                warnings=["-Wno-conversion"] if p==reference else []
                run(label+'-compile-'+str(i),[cc,*common,*warnings,*flags,"-MD","-MF",obj.with_suffix('.d'),"-c",p,"-o",obj]);objects.append(obj)
            binary=out / (label+'-test');run(label+'-link',[cc,*flags,*objects,"-Wl,-t","-Wl,-Map="+str(folder / "link.map"),"-o",binary])
            report.setdefault("binaries",{})[label]={"sha256":sha(binary),"bytes":binary.stat().st_size,"executed":execute}
            report.setdefault("costs",{})[label]=run(label+'-size',["size",*objects])
            if execute:
                vector_dir=out / ('vectors-'+label)
                arguments=[binary,out / "oracle",vector_dir]
                if label=='optimized':arguments.append('--host-cost')
                report.setdefault("tests",{})[label]=run(label+'-test',arguments,timeout=60)
            else:run("arm-elf",[args.toolchain / "bin/arm-buildroot-linux-musleabi-readelf","-h","-A","-l",binary])
            return objects
        host=build("/usr/bin/cc","optimized",["-O2"],True)
        v1_objects=[]
        for label,p in (('packet',out / 'input/v1/audio_packet.c'),('test',out / 'input/test_v1_preborrow.c')):
            obj=out / 'objects' / ('v1-'+label+'.o')
            run('v1-compile-'+label,['/usr/bin/cc','-I'+str(out / 'input/v1'),*common,'-O2','-MD','-MF',obj.with_suffix('.d'),'-c',p,'-o',obj]);v1_objects.append(obj)
        oldbin=out / 'v1-preborrow-regression'
        run('v1-link',['/usr/bin/cc','-O2',host[0],host[2],host[3],*v1_objects,'-Wl,-t','-o',oldbin])
        old_failure=run('v1-preborrow-regression',[oldbin],expected_exit=1)
        if 'result=-3 borrowed=1 packet_serial=1' not in old_failure:raise RuntimeError('V1 did not reproduce intended preborrow authority defect')
        report['v1_regression']={'state':'EXPECTED HOST failure: real revoked principal borrows READY before denial','log':old_failure,'binary_sha256':sha(oldbin)}
        build("/usr/bin/cc","sanitized",["-O1","-g","-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie"],True)
        cpp=out / "cxx-test";run("cxx-link",["/usr/bin/c++","-std=c++11","-O2","-Wall","-Wextra","-Werror","-MD","-MF",out / "objects/cxx.d",
            "-I"+str(out / "input"),"-I"+str(out / "input/capture-contract"),out / "input/header_link.cpp",*host[:4],"-Wl,-t","-o",cpp]);run("cxx-test",[cpp]);report["CXX"]="PASS C++ header -> actual C API link/run"
        armcc=args.toolchain / "bin/arm-buildroot-linux-musleabi-gcc";compiler(str(armcc),"arm")
        build(str(armcc),"arm",["-Os","-march=armv5te","-marm","-mfloat-abi=soft","-static"],False);report["ARM"]="STATIC LINK PASS NOT EXECUTED"
        probe=out / "objects/size-probe.arm.o";run("arm-size-probe",[armcc,*common,"-Os","-march=armv5te","-marm","-mfloat-abi=soft","-c",out / "input/size_probe.c","-o",probe]);report["arm_storage_sizes"]=run("arm-storage-symbols",[args.toolchain / "bin/arm-buildroot-linux-musleabi-nm","-S","--size-sort",probe])
        ffmpeg=shutil.which("ffmpeg")
        if ffmpeg:
            report["ffmpeg"]={"path":ffmpeg,"sha256":sha(Path(ffmpeg).resolve()),"version":run("ffmpeg-version",[ffmpeg,"-version"]),"license":run("ffmpeg-license",[ffmpeg,"-L"])}
            linked=run("ffmpeg-runtime",["ldd",ffmpeg]);runtime={}
            for line in linked.splitlines():
                for token in line.split():
                    if token.startswith('/') and Path(token).is_file():runtime[str(Path(token).resolve())]=sha(Path(token))
            report["ffmpeg"]["runtime_sha256"]=runtime
            for name,demux in (("mu","mulaw"),("a","alaw")):
                decoded=out / (name+'-ffmpeg-decoded.s16le')
                run('ffmpeg-'+name,[ffmpeg,"-nostdin","-hide_banner","-loglevel","error","-n","-f",demux,"-ar","8000","-ac","1","-i",out / 'vectors-optimized' / (name+'-all-codes.g711'),"-f","s16le","-acodec","pcm_s16le",decoded])
                expected=(out / 'vectors-optimized' / (name+'-c-decoded.s16le')).read_bytes()
                if decoded.read_bytes()!=expected or expected!=(out / 'oracle' / (name+'-decode-oracle.s16le')).read_bytes():raise RuntimeError("Independent FFmpeg decoder mismatch: "+name)
            report["FFmpeg_decode"]="PASS all256 code words/law bitexact C/Python; synthetic file-only, not browser/NVR/native"
        else:report["FFmpeg_decode"]="BLOCKED: installed ffmpeg absent, no installation attempted"
        # Synthetic PCM/WAV fixture only, no device samples or media advertisement.
        wav=out / "synthetic-codebook-pcm.wav"
        with wav.open("xb") as f:
            with wave.open(f,"wb") as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(8000);w.writeframes((out / 'vectors-optimized/mu-c-decoded.s16le').read_bytes())
        report["synthetic_WAV"]={"sha256":sha(wav),"frames":256,"purpose":"container fixture, not browser MSE or mic proof"}
        if before!={str(APP / n):sha(APP / n) for n in OWN} or policy_before!={p:sha(ROOT / p) for p in POLICY}:raise RuntimeError("Source changed during run")
        if any(sha(CAPTURE / n)!=expected for n,expected in CAPTURE_PINS.items()):raise RuntimeError("Capture source changed during run")
        report["state"]="SOURCE/HOST/CXX/ARM-LINK candidate PASS; native/browser/NVR/entropy/AUDIO gates OPEN"
    except BaseException as exc:
        report["state"]="FAIL OR CONCRETE BLOCKER";report["failure"]=type(exc).__name__+': '+str(exc);raise
    finally:
        headers={}
        for d in (out / 'objects').rglob('*.d'):
            for token in shlex.split(d.read_text().replace('\\\n',' ').split(':',1)[1]):
                p=Path(token)
                if p.is_file():headers[str(p.resolve())]=sha(p)
        report['source_header_closure']=headers
        report['stack_usage']={str(p.relative_to(ROOT)):p.read_text() for p in (out / 'objects').rglob('*.su')}
        runtime={}
        for log in (out / 'logs').glob('*-link.txt'):
            for line in log.read_text().splitlines():
                p=Path(line)
                if p.is_absolute() and p.is_file():runtime[str(p.resolve())]=sha(p)
        report['link_runtime_closure']=runtime;report['finished_utc']=now()
        with (out / 'REPORT.json').open('x') as f:json.dump(report,f,indent=2);f.write('\n')
        print('evidence='+str((out / 'REPORT.json').relative_to(ROOT)))


if __name__=='__main__':main()
