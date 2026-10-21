#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Isolated audio source/HOST/ARM-object qualification, no header prep/devices."""
import argparse
import datetime
import difflib
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
OWN = Path(__file__).resolve().parent
LANE = ROOT / "evidence/delegated/20261012-audio-candidate"
PUBLIC = ROOT / "work/lightbulb-camera/deep-bringup-20261008T055753Z/sources/ak3918ev200/vendor/linux-3.4.35-ak3918-diff"
MMC = ROOT / "evidence/delegated/20261012-mmc-candidate/build-20261012T174515.810242Z"
SEAL = MMC / "seal-20261012T175950.282234Z"
NAMES = ["capture_core.h", "capture_core.c", "test_capture_core.c", "qc07f-adc2.c", "header_link.cpp", "size_probe.c", "Kconfig", "Makefile", "candidate.dtsi", "SOURCE_AUDIT.md", "README.md", "build.py"]
PUBLIC_NAMES = ["sound/arm/ak_pcm.c", "sound/arm/ak39_codec.c", "arch/arm/plat-anyka/l2.c", "arch/arm/plat-anyka/include/plat/l2.h"]
DOCS = ["AGENTS.md", "docs/plan/camera-foundation/12-AUDIO.md", "docs/qualification-plans/peripherals-audio-nvr/MATRIX.md"]
REA = "work/lightbulb-camera/driver-re-20261008T110053Z/contracts/pseudocode/kernel-filtered/"
ARM_PINS = {"bin/toolchain-wrapper": "f4db963a226e01244ce6f7b5d1ec6d57474dd811119ff8834429f022835ce9c1",
 "bin/arm-buildroot-linux-musleabi-gcc.br_real": "e24ea223e7b521cce1f5889d63c25417e930cdbb35653b072f7b48d9a4b41729",
 "arm-buildroot-linux-musleabi/sysroot/lib/libc.a": "0982970abc52a36d247326a8e0b5dd487410222ff9728528cc99a4e8bd62e7b5",
 "lib/gcc/arm-buildroot-linux-musleabi/14.3.0/libgcc.a": "f707f3a5a94033cbbde76f76640d56dbc5dc615ab421be4efc2641c14344f19a"}


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run");parser.add_argument("--toolchain", type=Path, required=True)
    args = parser.parse_args()
    args.toolchain = args.toolchain.expanduser().resolve()
    if not args.run or not all(c.isalnum() or c in "-_" for c in args.run):parser.error("fresh simple run name required")
    LANE.mkdir(parents=True, exist_ok=True)
    if LANE.resolve() != LANE:raise RuntimeError("lane symlink refused")
    out = LANE / args.run;out.mkdir()
    for name in ("input", "upstream", "logs", "objects", "tmp"): (out / name).mkdir()
    env = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "LC_ALL": "C", "TMPDIR": str(out / "tmp"),
           "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    report = {"started_utc": now(), "command": ["python3", str(Path(__file__).relative_to(ROOT)), *os.sys.argv[1:]], "python": os.sys.version,
              "commands": [], "state": "RUNNING", "version": "v2-all-stage-deadlines", "toolchain_normalized": str(args.toolchain),
              "tasks": ["AUDIO-01", "AUDIO-02"], "inputs": {}, "binaries": {},
              "native_microphone": False, "target_operations": False, "audio_output_files": False, "image_or_module_built": False,
              "active_configuration_changed": False, "full_kernel_or_header_preparation": False, "G4": False, "G7": False, "G8": False}

    def copy(p, destination, kind):
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open("xb") as f:f.write(p.read_bytes())
        report["inputs"][str(p.relative_to(ROOT))] = {"sha256": sha(destination), "bytes": destination.stat().st_size,
            "class": kind, "copy": str(destination.relative_to(ROOT))}

    def run(label, argv, cwd=out, timeout=120, expected_exit=0):
        argv = list(map(str, argv));log = out / "logs" / (label + ".txt")
        rec = {"argv": argv, "cwd": str(cwd), "started_utc": now(), "timeout_seconds": timeout,
               "expected_exit_code": expected_exit, "log": str(log.relative_to(ROOT))}
        report["commands"].append(rec)
        with log.open("xb") as f:r = subprocess.run(argv, cwd=cwd, env=env, stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
        rec.update(exit_code=r.returncode, finished_utc=now(), sha256=sha(log))
        if r.returncode!=expected_exit:raise RuntimeError("Failed command preserved: " + str(log))
        return log.read_text()

    def compiler(cc, label):
        report.setdefault("compilers", {})[label] = {"version": run(label + "-version", [cc, "--version"]),
            "target": run(label + "-target", [cc, "-dumpmachine"]).strip(), "sha256": sha(Path(cc).resolve())}
        for item in ("cc1", "cc1plus", "as", "ld", "libgcc.a", "libasan.so", "libubsan.so", "libstdc++.so", "crt1.o", "Scrt1.o", "crti.o", "crtbeginS.o", "crtbeginT.o", "crtend.o", "crtn.o", "libc.a", "libc.so"):
            flag = "-print-prog-name=" if item in ("cc1", "cc1plus", "as", "ld") else "-print-file-name="
            value = run(label + "-closure-" + item.replace(".", "_"), [cc, flag + item]).strip()
            p = Path(value) if "/" in value else Path(shutil.which(value) or value)
            if p.is_file():report.setdefault("compiler_runtime_closure", {})[str(p.resolve())] = sha(p)

    try:
        before = {str(OWN / name): sha(OWN / name) for name in NAMES}
        v1=LANE / "v1-snapshot/bringup/qc07f/audio-candidate/capture_core.c"
        preservation=json.loads((LANE / "v1-snapshot/PRESERVATION.json").read_text())
        expected_v1=preservation["files"]["bringup/qc07f/audio-candidate/capture_core.c"]["sha256"]
        if sha(v1)!=expected_v1:raise RuntimeError("Preserved V1 source mismatch")
        copy(v1,out / "input/v1-capture-core.c","preserved GPL V1 source for negative regression")
        for name in NAMES:copy(OWN / name,out / "input" / name,"candidate GPL source")
        for relative in PUBLIC_NAMES:copy(PUBLIC / relative,out / "upstream" / relative,"public family source, not routing proof")
        copyright = PUBLIC.parent / "linux-3.4.35/COPYING"
        copy(copyright,out / "upstream/COPYING","original GPL license")
        for relative in DOCS + [REA + "286-ak39_codec_adc2_open.c.txt", REA + "288-ak39_codec_set_adc2_samplerate.c.txt"]:
            copy(ROOT / relative,out / "input/annotations" / relative,"requirements or retail static annotation, not physical measurement")
        diff = ''.join(difflib.unified_diff((out / "upstream/sound/arm/ak_pcm.c").read_text().splitlines(True),
                         (out / "input/qc07f-adc2.c").read_text().splitlines(True),fromfile="public-3.4.35/ak_pcm.c",tofile="candidate-Linux6/qc07f-adc2.c"))
        with (out / "adaptation.diff").open("x") as f:f.write(diff)
        for name in NAMES:
            if name.endswith((".h", ".c", ".cpp")) and "SPDX-License-Identifier: GPL-2.0-only" not in (OWN / name).read_text().splitlines()[0]:
                raise RuntimeError("Owned SPDX notice absent")
        report["licenses"] = "Original Anyka ak_pcm.c v2-or-later notice and L2 GPLv2 notices, unmodified codec MODULE_LICENSE and GPL COPYING preserved; compatible own GPL-2.0-only adaptation"
        for rel, expected in ARM_PINS.items():
            if sha(args.toolchain / rel) != expected:raise RuntimeError("ARM pin mismatch: " + rel)
        report["arm_toolchain_pins"] = ARM_PINS
        report["arm_license_metadata"] = {"sha256": sha(args.toolchain / "summary.csv"),
            "selected_lines": [l for l in (args.toolchain / "summary.csv").read_text().splitlines() if l.startswith(('"gcc-final",', '"musl",', '"binutils",'))]}
        common = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-Wconversion", "-Wshadow", "-fno-common", "-fstack-usage", "-I" + str(out / "input")]
        compiler("/usr/bin/cc", "host");compiler("/usr/bin/c++", "cxx")

        def build(cc,label,flags,execute):
            folder = out / "objects" / label;folder.mkdir();objects = []
            for i,name in enumerate(("capture_core.c", "test_capture_core.c")):
                obj = folder / (str(i) + "-" + name[:-2] + ".o")
                run(label + "-compile-" + str(i), [cc,*common,*flags,"-MD","-MF",obj.with_suffix(".d"),"-c",out / "input" / name,"-o",obj]);objects.append(obj)
            binary=out / (label + "-test")
            run(label + "-link",[cc,*flags,*objects,"-Wl,-t","-Wl,-Map="+str(folder / "link.map"),"-o",binary])
            report["binaries"][label]={"sha256":sha(binary),"bytes":binary.stat().st_size,"executed":execute}
            report.setdefault("costs",{})[label]=run(label+"-size",["size",*objects])
            if execute:report.setdefault("tests",{})[label]=run(label+"-test",[binary],timeout=30)
            else:run("arm-static-elf",[args.toolchain / "bin/arm-buildroot-linux-musleabi-readelf","-h","-A","-l",binary])
            return objects

        host=build("/usr/bin/cc","optimized",["-O2"],True)
        oldobj=out / "objects/v1-deadline-regression.o";oldbin=out / "v1-deadline-regression"
        run("v1-regression-compile",["/usr/bin/cc",*common,"-O2","-MD","-MF",oldobj.with_suffix(".d"),"-c",out / "input/v1-capture-core.c","-o",oldobj])
        run("v1-regression-link",["/usr/bin/cc","-O2",oldobj,host[1],"-Wl,-t","-o",oldbin])
        original_failure=run("v1-deadline-regression",[oldbin,"--deadline-only"],timeout=30,expected_exit=1)
        if "deadline-regression operation=1 result=-3 submits=1 fault=0" not in original_failure:
            raise RuntimeError("V1 regression failed outside intended CLAIM deadline counterexample")
        report["v1_timeout_regression"]={"state":"EXPECTED HOST failure: CLAIM deadline returns BUSY with no STOP/fault",
            "source_sha256":expected_v1,"log":original_failure,"binary_sha256":sha(oldbin),"native_executed":False}
        build("/usr/bin/cc","sanitized",["-O1","-g","-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie"],True)
        cpp=out / "cxx-test"
        run("cxx-link",["/usr/bin/c++","-std=c++11","-O2","-Wall","-Wextra","-Werror","-MD","-MF",out / "objects/cxx.d","-I"+str(out / "input"),out / "input/header_link.cpp",host[0],"-Wl,-t","-o",cpp]);run("cxx-test",[cpp]);report["CXX"]="LINK/RUN PASS"
        armcc=args.toolchain / "bin/arm-buildroot-linux-musleabi-gcc";compiler(str(armcc),"arm")
        build(str(armcc),"arm-static",["-Os","-march=armv5te","-marm","-mfloat-abi=soft","-static"],False)
        report["ARM_static"]="PASS NOT EXECUTED; pinned runtime merged attributes do not prove native ABI"
        # Reuse the IMMUTABLE already prepared MMC Linux6.18.55 source/headers.
        old=json.loads((MMC / "REPORT.json").read_text());headers=json.loads((SEAL / "arm-header-closure.json").read_text())
        if old["linux_pin"]!="725bd2f3c81d54edccb66fead01d4c0c222e2231":raise RuntimeError("Linux source pin mismatch")
        for path,digest in headers.items():
            if sha(Path(path))!=digest:raise RuntimeError("Sealed header changed: "+path)
        kbuild=MMC / "kbuild";linux=MMC / "linux-scratch"
        immutable={str(MMC / "REPORT.json"):sha(MMC / "REPORT.json"),str(SEAL / "REPORT.json"):sha(SEAL / "REPORT.json"),str(kbuild / ".config"):sha(kbuild / ".config")}
        saved=kbuild / "drivers/mmc/host/qc07f-candidate/.qc07f-mci.o.cmd"
        immutable[str(saved)]=sha(saved)
        for tool in ("clang","ld.lld","llvm-readelf"):
            path=Path(shutil.which(tool));
            if sha(path)!=old["tool_sha256"][tool]:raise RuntimeError("Sealed Kbuild tool changed: "+tool)
            run("kernel-tool-"+tool,[tool,"--version"])
        original=shlex.split(saved.read_text().splitlines()[0].split(" := ",1)[1])
        if original[0]!="clang" or "-march=armv5te" not in original or "-Werror" not in original or any(";" in t or "\n" in t for t in original):raise RuntimeError("Saved compiler command audit failed")
        kernel_objects=[];closure={}
        inventory=json.loads((MMC / "copied-linux-inputs.json").read_text())
        for name in ("capture_core","qc07f-adc2","size_probe"):
            obj=out / "objects" / (name+".kernel.arm.o");dep=obj.with_suffix(".d");argv=original[:]
            for i,token in enumerate(argv):
                if token.startswith("-Wp,-MMD,"):argv[i]="-Wp,-MMD,"+str(dep)
                elif token.startswith("-I") and "drivers/mmc/host/qc07f-candidate" in token:argv[i]="-I"+str(out / "input")
                elif token.startswith("-DKBUILD_BASENAME="):argv[i]='-DKBUILD_BASENAME="'+name.replace('-','_')+'"'
                elif token.startswith("-DKBUILD_MODNAME="):argv[i]='-DKBUILD_MODNAME="qc07f_adc2_candidate"'
                elif i>0 and argv[i-1]=="-o":argv[i]=str(obj)
            argv[-1]=str(out / "input" / (name+".c"));argv.insert(-4,"-I"+str(out / "input"));argv.insert(-4,"-fstack-usage")
            run("kernel-"+name,argv,cwd=kbuild)
            if name!="size_probe":kernel_objects.append(obj)
            else:report["kernel_storage_sizes"]=run("kernel-storage-symbols",["llvm-nm","-S","--size-sort",obj])
            for token in shlex.split(dep.read_text().replace("\\\n", " ").split(":",1)[1]):
                p=Path(token) if Path(token).is_absolute() else kbuild / token
                if p.is_relative_to(linux):
                    relative=str(p.relative_to(linux))
                    if relative not in inventory or sha(p)!=inventory[relative]:raise RuntimeError("Additional Linux header inventory mismatch: "+relative)
                closure[str(p)]=sha(p)
        linked=out / "qc07f-adc2-candidate.kernel.arm-relocatable.o"
        run("kernel-relocatable-link",["ld.lld","-r",*kernel_objects,"-o",linked]);run("kernel-elf",["llvm-readelf","-h","-A","-s",linked])
        run("kernel-undefined",["llvm-nm","--undefined-only",linked]);report["kernel_costs"]=run("kernel-size",["size",*kernel_objects,linked])
        report["kernel_header_closure"]=closure
        for p,digest in immutable.items():
            if sha(Path(p))!=digest:raise RuntimeError("Read-only MMC evidence/config changed: "+p)
        for p,digest in headers.items():
            if sha(Path(p))!=digest:raise RuntimeError("Read-only headers changed during compile")
        report["kernel_proof"]={"state":"ARM OBJECTS/RELOCATABLE LINK PASS ONLY; NO runtime/active ALSA ABI proof",
            "method":"audited saved Kbuild compiler argv replay, source/output override, sealed read-only headers; no make/prep/tree copy",
            "MMC_immutable_inputs":immutable,"sealed_header_count":len(headers),"additional_actual_dependency_count":len(closure),"source_inventory_sha256":sha(MMC / "copied-linux-inputs.json")}
        if before!={str(OWN / n):sha(OWN / n) for n in NAMES}:raise RuntimeError("Owned source changed during build")
        report["state"]="HOST/CXX/ARM STATIC LINK/KERNEL OBJECT RELOCATABLE PASS; ALL NATIVE AUDIO GATES OPEN"
    except BaseException as exc:
        report["state"]="FAIL OR CONCRETE BUILD BLOCKER";report["failure"]=type(exc).__name__+": "+str(exc);raise
    finally:
        closure={}
        for d in (out / "objects").rglob("*.d"):
            # Kernel deps use read-only kbuild-relative includes, preserved above.
            if ".kernel.arm" in d.name:continue
            for token in shlex.split(d.read_text().replace("\\\n", " ").split(":",1)[1]):
                p=Path(token)
                if p.is_file():closure[str(p.resolve())]=sha(p)
        report["host_ARM_source_header_closure"]=closure
        report["stack_usage"]={str(p.relative_to(ROOT)):p.read_text() for p in (out / "objects").rglob("*.su")}
        linked={}
        for log in (out / "logs").glob("*-link.txt"):
            for line in log.read_text().splitlines():
                p=Path(line)
                if p.is_absolute() and p.is_file():linked[str(p.resolve())]=sha(p)
        report["link_runtime_closure"]=linked;report["finished_utc"]=now()
        with (out / "REPORT.json").open("x") as f:json.dump(report,f,indent=2);f.write("\n")
        print("evidence="+str((out / "REPORT.json").relative_to(ROOT)))


if __name__=="__main__":main()
