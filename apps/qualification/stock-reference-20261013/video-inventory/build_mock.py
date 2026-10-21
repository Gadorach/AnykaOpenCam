#!/usr/bin/env python3
"""Fresh scoped build; run host MOCK only, never the ARM metadata payload."""
from pathlib import Path
import datetime
import hashlib
import json
import re
import struct
import subprocess
import sys

SRC = Path(__file__).resolve().parent
ROOT = SRC.parents[3]
BASE = ROOT / "evidence/delegated/20261013-stock-reference/video-inventory"
TC = ROOT / ("work/lightbulb-camera/implementation-20261008T205404Z/sources/"
             "toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1")
OLD = ROOT / ("work/lightbulb-camera/deep-bringup-20261008T055753Z/sources/"
              "ak3918ev200/vendor/linux-3.4.35")


def sha(p):
    h = hashlib.sha256()
    with p.open("rb") as f:
        for b in iter(lambda: f.read(1048576), b""): h.update(b)
    return h.hexdigest()


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r"build[0-9]{2}", sys.argv[1]):
        raise SystemExit("usage: build_mock.py buildNN (fresh)")
    BASE.mkdir(parents=True, exist_ok=True)
    out = BASE / sys.argv[1]; out.mkdir(exist_ok=False); (out / "tmp").mkdir()
    env = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "TMPDIR": str(out / "tmp"),
           "SOURCE_DATE_EPOCH": "0", "PYTHONDONTWRITEBYTECODE": "1"}
    commands, tests = [], []
    report = {"started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "state": "running", "commands": commands, "tests": tests,
              "native_executed": False, "target_operations": False}
    tools = {}

    def run(args, label, check=True, case=None, executable=None):
        e = dict(env)
        if case: e["VIDEO_MOCK_CASE"] = case
        p = subprocess.run([str(a) for a in args], cwd=out, env=e, executable=executable,
                           capture_output=True, text=True, timeout=30)
        (out / (label + ".stdout.txt")).write_text(p.stdout)
        (out / (label + ".stderr.txt")).write_text(p.stderr)
        commands.append({"argv": [str(a) for a in args], "label": label,
                         "executable": executable, "returncode": p.returncode,
                         "mock_case": case})
        if check and p.returncode: raise RuntimeError(label + ": " + p.stderr)
        return p

    try:
        for name in ["gcc", "readelf", "nm", "size", "objdump"]:
            tools[name] = TC / ("bin/arm-buildroot-linux-musleabi-" + name)
        cc = tools["gcc"]
        run([cc, "--version"], "compiler")
        run([cc, "-v"], "compiler-config")
        run([tools["readelf"], "--version"], "binutils")
        run(["/usr/bin/cc", "--version"], "host-compiler")
        flags = ["-std=c11", "-Os", "-Wall", "-Wextra", "-Werror", "-fno-common",
                 "-ffunction-sections", "-fdata-sections", "-fstack-usage", "-fno-ident"]
        arm = ["-march=armv5te", "-marm", "-mfloat-abi=soft"]
        oldheader = OLD / "include/linux/videodev2.h"
        for name, extra in [("old", ["-D__EXPORTED_HEADERS__", "-I" + str(OLD / "include"),
                                     '-DORIGINAL_V4L2_HEADER="' + str(oldheader) + '"']),
                            ("new", [])]:
            run([cc, *flags, *arm, *extra, "-MD", "-MF", out / (name + ".d"),
                 "-c", SRC / "abi_compare.c", "-o", out / (name + ".o")], "abi-" + name)
            run([tools["readelf"], "-x", ".rodata.header_values", out / (name + ".o")],
                "abi-values-" + name)
        run([cc, *flags, *arm, "-MD", "-MF", out / "native.d", "-c", SRC / "inventory.c",
             "-o", out / "native.o"], "native-compile")
        run([cc, *arm, "-static", "-no-pie", "-Wl,--gc-sections,--build-id=none,-z,noexecstack",
             "-Wl,-Map=" + str(out / "native.map"), out / "native.o", "-o", out / "stock-video-inventory-armv5"],
            "native-link")
        native = out / "stock-video-inventory-armv5"
        run([tools["readelf"], "-h", "-l", "-A", native], "native-elf")
        run([tools["size"], "-A", native], "native-size")
        imports = run([tools["nm"], "-u", out / "native.o"], "application-imports").stdout
        assert not re.search(r"\b(malloc|calloc|realloc|free|fork|execve|dlopen|socket|mmap)\b", imports)
        run([tools["nm"], "-a", native], "native-symbols")
        run([tools["objdump"], "-d", native], "native-disassembly")
        data = native.read_bytes()
        assert data[:6] == b"\x7fELF\x01\x01" and struct.unpack_from("<H", data, 18)[0] == 40
        po = struct.unpack_from("<I", data, 28)[0]
        ps, pn = struct.unpack_from("<HH", data, 42)
        ph = [struct.unpack_from("<8I", data, po + ps*i) for i in range(pn)]
        assert all(x[0] not in (2, 3) for x in ph)
        assert any(x[0] == 0x6474e551 and not x[6] & 1 for x in ph)
        rw = sum(x[5] for x in ph if x[0] == 1 and x[6] & 2)
        frames = [int(x.split("\t")[1]) for x in (out / "native.su").read_text().splitlines()]
        assert sum(frames) < 4096 and rw < 4096, (frames, rw)
        report["application_frame_sum_bytes"] = sum(frames)
        report["elf_writable_load_bytes"] = rw
        # musl's optional ENOTTY translation table must not translate these requests.
        nm = (out / "native-symbols.stdout.txt").read_text()
        table_symbols = [line for line in nm.splitlines() if re.search(r"\bconvert_map$", line)]
        # Keep a full static listing even if the library's local table name differs.
        report["ioctl_translation_table_symbol_candidates"] = table_symbols
        run(["/usr/bin/cc", *flags, "-DSTOCK_VIDEO_MOCK", SRC / "inventory.c", SRC / "mock.c",
             "-o", out / "stock-video-inventory-host-mock"], "host-mock-build")
        host = str(out / "stock-video-inventory-host-mock")

        def test(case, code=1, opens=(41,42), qs=3, closes=(42,41), args=None):
            argv = args or [host, "--mock-stock-video-inventory"]
            label = f"mock-{len(tests):02d}-{case}"
            p = run(argv, label, check=False, case=case, executable=host)
            assert p.returncode == code, (label, p.returncode, p.stderr)
            for kind, expected in [("open", list(opens)), ("ioctl", list(range(1,qs+1))), ("close", list(closes))]:
                got = [int(n) for n in re.findall(r"MOCK " + kind + r" ([0-9]+)", p.stderr)]
                assert got == expected, (label, kind, got, expected)
            assert "PRIVATE_" not in p.stdout
            if code == 0:
                d = json.loads(p.stdout)
                assert d["expected_wire_bytes"] == [104,204,64]
                assert d["querycap"]["version"] == 0x30423
                assert d["querycap"]["capabilities"] == 0x4000001
                assert d["querycap"]["bus_info_omitted"] is True
                assert "bus_info" not in d["querycap"]
                f = d["cached_format"]
                values = [0]*8 if case == "zero-format" else [1280,720,0x56595559,1,2560,1843200,7,0x11223344]
                if case == "unknown-fourcc": values[2] = 0x01020304
                names = ["width","height","pixelformat","field","bytesperline","sizeimage","colorspace","priv"]
                assert [f[k] for k in names] == values
                assert bytes.fromhex(f["pix32_hex"]) == struct.pack("<8I", *values)
                assert d["enum0"] == {"index":0,"type":1,"flags":2,"pixelformat":0x56595559}
                assert d["close_order"] == "video_then_isp" and d["physical_frame_or_ir_proof"] is False
                if case == "private-labels":
                    assert d["querycap"]["driver_label"] == d["querycap"]["card_label"] == "unrecognized_omitted"
            else: assert not p.stdout, (label, "unexpected result output")
            tests.append({"case": case, "exit": code, "opens": list(opens), "queries": qs,
                          "closes": list(closes), "passed": True})

        for case in ["success", "zero-format", "unknown-fourcc", "private-labels"]: test(case, 0)
        for case in ["uid","euid","pid1","kernel","uname","sigsetup","sigmask","clockstart",
                     "clockinvalid","isp-missing","video-missing","isp-link","video-link","regular"]:
            test(case, opens=(), qs=0, closes=())
        test("isp-open", opens=(41,), qs=0, closes=())
        for case in ["isp-fstat", "isp-race"]: test(case, opens=(41,), qs=0, closes=(41,))
        test("video-open", qs=0, closes=(41,))
        for case in ["video-fstat", "video-race", "video-notchar"]: test(case, qs=0)
        for n in range(1,4):
            for prefix in ["fail", "short", "positive", "overrun", "underrun"]:
                test(prefix + str(n), qs=n)
        for case in ["enum-enotty", "enum-einval", "enum-index", "enum-type"]: test(case)
        test("fmt-type", qs=2)
        for case in ["video-close", "isp-close", "both-close", "clockend", "backward", "deadline"]: test(case)
        for n in range(1,4): test("alarm" + str(n), 124, qs=n)
        test("alarm-isp-open", 124, opens=(41,), qs=0, closes=(41,))
        test("alarm-video-open", 124, qs=0)
        test("alarm-close", 124)
        test("interrupt", 130, qs=2); test("terminate", 143, qs=2)
        for n, argv in enumerate([[host], [host,"--native-stock-video-inventory"],
                                  [host,"--mock-stock-video-inventory","/dev/video1"],
                                  [host,"--mock-stock-video-inventory","STREAMON"],
                                  [host,"--mock-stock-get"], ["./probe","--mock-stock-video-inventory"]]):
            test("selector" + str(n), opens=(), qs=0, closes=(), args=argv)

        inputs = {p.resolve() for p in SRC.iterdir() if p.is_file()}
        for dep in out.glob("*.d"):
            inputs.update(Path(p).resolve() for p in dep.read_text().replace("\\\n", " ").split(":",1)[1].split())
        inputs.update(t.resolve() for t in tools.values())
        inputs.update([TC / "summary.csv", TC / "buildroot.config", TC / "README.txt",
                       OLD / "include/asm-generic/ioctl.h", OLD / "drivers/media/video/v4l2-ioctl.c",
                       OLD / "drivers/media/video/soc_camera.c"])
        for name in ["cc1", "as", "ld"]:
            p = run([cc, "-print-prog-name="+name], "subtool-"+name).stdout.strip()
            inputs.add(Path(p).resolve())
        for line in (out / "native.map").read_text().splitlines():
            if line.startswith("LOAD "):
                p = Path(line[5:].strip()).resolve()
                if p.is_file(): inputs.add(p)
        report["input_closure"] = [{"path":str(p),"bytes":p.stat().st_size,"sha256":sha(p)} for p in sorted(inputs)]
        report["environment"] = env
        report["native_sha256"] = sha(native); report["native_bytes"] = native.stat().st_size
        report["state"] = "headers-verified-host-mocks-pass-native-not-executed"
        snapshot = out / "source"; snapshot.mkdir()
        for p in SRC.iterdir():
            if p.is_file(): (snapshot / p.name).write_bytes(p.read_bytes())
    except Exception as exc:
        report["state"] = "failed"; report["failure"] = repr(exc); raise
    finally:
        report["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        (out / "report.json").write_text(json.dumps(report,indent=2)+"\n")
        (out / "SHA256SUMS").write_text("".join(f"{sha(p)}  {p.relative_to(out)}\n" for p in sorted(out.rglob("*")) if p.is_file()))
    print(json.dumps({"state":report["state"],"tests":len(tests),"payload":str(native),
                      "sha256":report["native_sha256"],"bytes":report["native_bytes"]}))


if __name__ == "__main__": main()
