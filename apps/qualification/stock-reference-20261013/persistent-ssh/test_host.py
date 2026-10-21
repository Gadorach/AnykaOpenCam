"""Offline mocks only. Never execute ARM firmware, mount, or touch an interface."""
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import sys
import tempfile
import unittest

import build


class HostTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.base = Path(self.tmp.name)
        self.root = self.base / "root"
        for name in ("proc/sys/kernel", "sys/class/net/wlan0", "dev", "tmp/bench-ssh/keys", "bench-ssh/private", "usr/bin"):
            (self.root / name).mkdir(parents=True, exist_ok=True)
        (self.root / "proc/modules").write_text("")
        for name in ("mounts",):
            (self.root / "proc" / name).write_text("")
        for name in build.DUMMY:
            (self.root / "bench-ssh/private" / name).write_bytes(build.DUMMY[name])
        self.log = self.base / "calls.jsonl"
        self.wrapper = self.base / "mock-busybox"
        self.wrapper.write_text("#!/usr/bin/python3\n" + r'''
import json, os, pathlib, shutil, sys
a=sys.argv[1:]
with open(os.environ['MOCK_LOG'],'a') as f: f.write(json.dumps(a)+'\n')
if os.environ.get('MOCK_FAIL') == a[0]: sys.exit(1)
if a[0]=='id': print('0')
elif a[0]=='uname': print('3.4.35')
elif a[0]=='cat': sys.stdout.write(pathlib.Path(a[1]).read_text())
elif a[0]=='awk': sys.exit(1 if '/dev' in a[1] else 0)
elif a[0]=='mkdir':
    for p in a[2:]: pathlib.Path(p).mkdir(parents=True,exist_ok=True)
elif a[0]=='cp': shutil.copyfile(a[1],a[2])
elif a[0]=='chmod':
    for p in a[2:]: pathlib.Path(p).chmod(int(a[1],8))
elif a[0]=='touch': pathlib.Path(a[1]).touch()
elif a[0]=='mv': pathlib.Path(a[1]).replace(a[2])
elif a[0]=='rm':
    for p in a[2:]: pathlib.Path(p).unlink(missing_ok=True)
elif a[0]=='udhcpc':
    pathlib.Path(os.environ['MOCK_ROOT']+'/tmp/bench-ssh/address').write_text('192.168.1.42\n')
sys.exit(0)
''')
        self.wrapper.chmod(0o700)
        for name in ("usr/bin/wpa_supplicant", "bench-ssh/dropbear"):
            path = self.root / name
            path.write_text("#!/usr/bin/python3\nimport json,os,sys\nwith open(os.environ['MOCK_LOG'],'a') as f: f.write(json.dumps([" + repr(path.name) + "]+sys.argv[1:])+'\\n')\nif os.environ.get('MOCK_FAIL') == " + repr(path.name) + ": sys.exit(1)\n")
            path.chmod(0o700)

    def tearDown(self):
        self.tmp.cleanup()

    def run_script(self, name, args=(), **environment):
        text = (build.HERE / "overlay" / name).read_text()
        # Fixed absolute target paths become fixture-only paths. NO target binary
        # or real /proc,/dev,/sys/network path is reachable in this test copy.
        text = text.replace("/bin/busybox", str(self.wrapper))
        text = re.sub(r"(?<![A-Za-z0-9_/])/(?:proc|sys|dev|tmp|var|usr|bench-ssh)(?=[/\s\";]|$)", lambda m: str(self.root) + m.group(), text)
        target = self.base / (name + ".test.sh")
        target.write_text(text)
        env = dict(os.environ, MOCK_LOG=str(self.log), MOCK_ROOT=str(self.root), **environment)
        return subprocess.run(["/bin/sh", str(target), *args], env=env, capture_output=True, timeout=10)

    def calls(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def test_startup_order_ro_mounts_only_three_modules_private_ram(self):
        result = self.run_script("bootstrap")
        self.assertEqual(result.returncode, 0, result.stderr)
        calls = self.calls()
        first = {name: next(i for i, item in enumerate(calls) if item[0] == name) for name in ("mount", "sha256sum", "mdev", "cp", "insmod", "wpa_supplicant", "udhcpc", "touch")}
        self.assertEqual(list(first.values()), sorted(first.values()))
        mounts = [row for row in calls if row[0] == "mount"]
        usr = [row for row in mounts if row[-1] == str(self.root / "usr")]
        self.assertEqual(len(usr), 1)
        self.assertIn("ro,nodev,nosuid", usr[0])
        self.assertTrue(any("mode=0755,uid=0,gid=0,nosuid,nodev" in row and row[-1] == str(self.root / "tmp") for row in mounts))
        self.assertEqual([Path(row[1]).name for row in calls if row[0] == "insmod"], ["ak39_top_wdt.ko", "otg-hs.ko", "hgics.ko"])
        copied = [row for row in calls if row[0] == "cp"]
        self.assertEqual(len(copied), 3)
        for row in copied:
            self.assertTrue(row[-1].startswith(str(self.root / "tmp/bench-ssh")))
            self.assertEqual(stat.S_IMODE(Path(row[-1]).stat().st_mode), 0o600)
        self.assertEqual((self.root / "proc/sys/kernel/hotplug").read_text().strip(), "/sbin/mdev")
        self.assertTrue((self.root / "tmp/bench-ssh/ready").exists())
        text = json.dumps(calls)
        for forbidden in ("mtdblock6", "mmc", "camera.sh", "service.sh", "anyka_ipc", "motor", "akpcm", "gpio", "route", "resolv", "watchdog -", "rcS", "rc.local"):
            self.assertNotIn(forbidden, text)

    def test_all_boot_failures_stop_without_app_fallthrough(self):
        for command in ("mount", "sha256sum", "mdev", "cp", "insmod", "wpa_supplicant", "udhcpc"):
            self.log.unlink(missing_ok=True)
            result = self.run_script("bootstrap", MOCK_FAIL=command)
            self.assertNotEqual(result.returncode, 0)
            self.assertNotIn("BOOTSTRAP_COMPLETED", result.stdout.decode())
            self.assertFalse(any(row[0] == "dropbear" for row in self.calls()))
            self.assertNotIn("anyka_ipc", json.dumps(self.calls()))

    def test_dummy_image_blocks_before_module_or_network_effects(self):
        (self.root / "bench-ssh/NOTFLASHABLE").write_text("dummy")
        result = self.run_script("bootstrap")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(any(row[0] in ("insmod", "wpa_supplicant", "udhcpc") for row in self.calls()))

    def test_dhcp_subnet_interface_validation_and_atomic_current_address(self):
        for event in ("bound", "renew"):
            result = self.run_script("local-only-dhcp", [event], interface="wlan0", subnet="255.255.255.0", ip="192.168.1.42", router="8.8.8.8", dns="1.1.1.1")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((self.root / "tmp/bench-ssh/address").read_text(), "192.168.1.42\n")
        self.assertTrue(all(row[0] in ("ifconfig", "mv") for row in self.calls()))
        for interface, subnet, ip in (("eth0", "255.255.255.0", "192.168.1.42"), ("wlan0", "255.255.0.0", "192.168.1.42"),
                                      ("wlan0", "255.255.255.0", "192.168.2.42"), ("wlan0", "255.255.255.0", "192.168.1.255"),
                                      ("wlan0", "255.255.255.0", "192.168.1.1;id")):
            count = len(self.calls())
            result = self.run_script("local-only-dhcp", ["bound"], interface=interface, subnet=subnet, ip=ip)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(len(self.calls()), count)
        self.run_script("local-only-dhcp", ["deconfig"], interface="wlan0")
        self.assertFalse((self.root / "tmp/bench-ssh/address").exists())

    def test_server_explicit_actual_ip_keyonly_flags_no_E_R(self):
        (self.root / "tmp/bench-ssh/ready").touch()
        (self.root / "tmp/bench-ssh/address").write_text("192.168.1.42\n")
        result = self.run_script("serve")
        self.assertEqual(result.returncode, 0, result.stderr)
        argv = self.calls()[-1]
        self.assertEqual(argv[0], "dropbear")
        for option in ("-F", "-s", "-r", "-D", "-P", "-p", "-I", "-M", "-T"):
            self.assertIn(option, argv)
        self.assertIn("192.168.1.42:22222", argv)
        for bad in ("-E", "-R", "0.0.0.0:22222", "192.168.1.73:22222"):
            self.assertNotIn(bad, argv)
        self.assertEqual(argv[argv.index("-I") + 1], "90")
        self.assertEqual(argv[argv.index("-M") + 1], "300")

    def test_server_without_success_marker_or_valid_ip_remains_bench(self):
        for marker, address in ((False, "192.168.1.42\n"), (True, "0.0.0.0\n"), (True, "192.168.1.42;id\n")):
            ready = self.root / "tmp/bench-ssh/ready"
            ready.unlink(missing_ok=True)
            if marker:
                ready.touch()
            (self.root / "tmp/bench-ssh/address").write_text(address)
            self.log.unlink(missing_ok=True)
            self.run_script("serve")
            self.assertEqual(self.calls()[-1], ["sleep", "86400"])

    def test_audit_rejects_traversal_descendants_and_escaping_links(self):
        root = "drwxr-xr-x 0/0 1 2023-01-01 00:00:00 squashfs-root\n"
        for tail in ("-rwxr-xr-x 0/0 1 date time squashfs-root/../outside\n",
                     "lrwxrwxrwx 0/0 1 date time squashfs-root/link -> ../../outside\n",
                     "lrwxrwxrwx 0/0 1 date time squashfs-root/link -> /tmp\n-rw-r--r-- 0/0 1 date time squashfs-root/link/file\n"):
            with self.assertRaises(ValueError):
                build.audit_listing(root + tail)
        self.assertEqual(build.audit_listing(root + "lrwxrwxrwx 0/0 1 date time squashfs-root/link -> /etc/target\n"), 2)

    def test_private_inputs_reject_modes_symlinks_extra_client_key(self):
        inputs = self.base / "private-inputs"
        inputs.mkdir(mode=0o700)
        for name, data in build.DUMMY.items():
            (inputs / name).write_bytes(data)
            (inputs / name).chmod(0o600)
        build.credentials(inputs, True)
        (inputs / "hostkey").chmod(0o644)
        with self.assertRaises(ValueError):
            build.credentials(inputs, True)
        (inputs / "hostkey").chmod(0o600)
        (inputs / "hostkey").unlink()
        (inputs / "hostkey").symlink_to(self.base / "not-a-key")
        with self.assertRaises(ValueError):
            build.credentials(inputs, True)
        (inputs / "hostkey").unlink()
        (inputs / "hostkey").write_bytes(build.DUMMY["hostkey"])
        (inputs / "hostkey").chmod(0o600)
        (inputs / "client-private-key").write_text("NOT A REAL KEY")
        with self.assertRaises(ValueError):
            build.credentials(inputs, True)

    def test_parent_policy_validation_uses_public_noncredential_fixtures_only(self):
        inputs = self.base / "policy-fixtures"
        inputs.mkdir(mode=0o700)
        values = {"hostkey": b"NOTFLASHABLE PUBLIC FORMAT TEST ONLY" * 2,
                  "authorized_keys": b"restrict ssh-ed25519 AAAABBBBPublicFixtureOnly\n",
                  "wpa.conf": (b'ctrl_interface=/tmp/bench-ssh/wpa\nupdate_config=0\nnetwork={\nssid="VertexHotspot"\n'
                               b'psk="PUBLIC_NOT_A_REAL_PASSWORD"\nkey_mgmt=WPA-PSK\nproto=RSN\npairwise=CCMP\ngroup=CCMP\n}\n')}
        for name, value in values.items():
            (inputs / name).write_bytes(value)
            (inputs / name).chmod(0o600)
        build.credentials(inputs, False)
        for changed in (values["wpa.conf"].replace(b"VertexHotspot", b"UnapprovedSSID"),
                        values["wpa.conf"].replace(b"update_config=0", b"update_config=1"),
                        values["wpa.conf"] + b'network={\nssid="Another"\n}\n'):
            (inputs / "wpa.conf").write_bytes(changed)
            with self.assertRaises(ValueError):
                build.credentials(inputs, False)
        (inputs / "wpa.conf").write_bytes(values["wpa.conf"])
        (inputs / "authorized_keys").write_bytes(values["authorized_keys"].replace(b"restrict ", b""))
        with self.assertRaises(ValueError):
            build.credentials(inputs, False)

    def test_shell_syntax_and_no_stock_or_actuator_entrypoint(self):
        for path in (build.HERE / "overlay").iterdir():
            result = subprocess.run(["/bin/sh", "-n", str(path)], capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
        script = (build.HERE / "overlay/bootstrap").read_text()
        for forbidden in ("/etc/init.d/rcS", "/etc/init.d/rc.local", "camera.sh", "wifi.sh", "akmci", "motor", "akpcm", "gpio", "mtdblock6", "passwd", "shadow"):
            self.assertNotIn(forbidden, script)

    def test_fakeroot_roundtrip_preserves_nonroot_ids_hardlinks_and_device(self):
        # Synthetic filesystem only; fake mknod NEVER creates a real host device.
        code = r'''
import os, pathlib, stat, subprocess, sys
sys.path.insert(0, sys.argv[1])
import build
base=pathlib.Path(sys.argv[2]); tree=base/'synthetic'; tree.mkdir(mode=0o755)
(tree/'etc').mkdir(mode=0o777)
(tree/'etc/inittab').write_bytes(b'public fixture')
os.chown(tree/'etc/inittab',985,1501); os.chmod(tree/'etc/inittab',0o754)
os.link(tree/'etc/inittab',tree/'hardlink')
os.symlink('/target-root-not-host',tree/'link')
os.mknod(tree/'fake-device', stat.S_IFCHR|0o640,os.makedev(1,3))
os.chown(tree/'fake-device',12,34)
for path in (tree,tree/'etc',tree/'etc/inittab',tree/'hardlink',tree/'link',tree/'fake-device'):
    os.utime(path,(1234567890,1234567890),follow_symlinks=False)
image=base/'synthetic.sqfs'
subprocess.run(['/usr/bin/mksquashfs',str(tree),str(image),'-noappend','-comp','xz','-processors','1','-no-progress'],check=True,stdout=subprocess.DEVNULL)
recovered=base/'recovered'
subprocess.run(['/usr/bin/unsquashfs','-strict-errors','-no-progress','-processors','1','-d',str(recovered),str(image)],check=True,stdout=subprocess.DEVNULL)
build.compare(tree,recovered,{},b'public fixture')
assert stat.S_ISCHR((recovered/'fake-device').lstat().st_mode)
assert (recovered/'fake-device').lstat().st_rdev == os.makedev(1,3)
assert (recovered/'hardlink').lstat().st_ino == (recovered/'etc/inittab').lstat().st_ino
assert (recovered/'etc/inittab').lstat().st_uid == 985
'''
        env = dict(os.environ, TMPDIR=str(self.base))
        result = subprocess.run(["/usr/bin/fakeroot", "--", sys.executable, "-B", "-c", code, str(build.HERE), str(self.base)],
                                env=env, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_live_baseline_only_prior_password_file_delta_allowed(self):
        import shutil
        acquired=self.base/'acquired'; acquired.mkdir(); (acquired/'etc').mkdir()
        (acquired/'etc/passwd').write_bytes(b'PUBLIC ORIGINAL ACCOUNT FIXTURE')
        (acquired/'etc/inittab').write_bytes(b'PUBLIC INIT FIXTURE')
        live=self.base/'live'; shutil.copytree(acquired,live,copy_function=shutil.copy2)
        self.assertFalse(build.validate_live_root(acquired,live)['only_prior_passwd_delta'])
        (live/'etc/passwd').write_bytes(b'PUBLIC PRIOR UPDATE FIXTURE')
        source=acquired/'etc/passwd'; os.utime(live/'etc/passwd',ns=(source.stat().st_atime_ns,source.stat().st_mtime_ns))
        self.assertTrue(build.validate_live_root(acquired,live)['only_prior_passwd_delta'])
        (live/'etc/inittab').write_bytes(b'UNREVIEWED STARTUP')
        source=acquired/'etc/inittab'; os.utime(live/'etc/inittab',ns=(source.stat().st_atime_ns,source.stat().st_mtime_ns))
        with self.assertRaises(ValueError):build.validate_live_root(acquired,live)

    def test_live_baseline_metadata_or_path_delta_is_refused(self):
        import shutil
        acquired=self.base/'acquired'; acquired.mkdir(); (acquired/'etc').mkdir(); (acquired/'etc/passwd').write_bytes(b'PUBLIC FIXTURE')
        live=self.base/'live'; shutil.copytree(acquired,live,copy_function=shutil.copy2)
        (live/'etc/passwd').chmod(0o700)
        with self.assertRaises(ValueError):build.validate_live_root(acquired,live)
        (live/'etc/passwd').chmod((acquired/'etc/passwd').stat().st_mode&0o777)
        (live/'extra').write_bytes(b'UNREVIEWED')
        with self.assertRaises(ValueError):build.validate_live_root(acquired,live)

    def test_live_regions_preserve_existing_ENV_and_JFFS_without_admitting_code_changes(self):
        original=bytes(build.PINS['flash_bytes']);live=bytearray(original)
        live[0x60000]=1;live[0x200000]=2;live[0x780000]=3
        build.validate_live_regions(original,live)
        for offset in (0,0x70000,0x80000,0x300000):
            changed=bytearray(live);changed[offset]=4
            with self.assertRaises(ValueError):build.validate_live_regions(original,changed)

    def test_private_dotdot_spelling_is_refused_before_containment(self):
        (self.base/'hop').mkdir(mode=0o700);git=self.base/'mock-git-root';git.mkdir(mode=0o700)
        file=git/'PUBLIC-FIXTURE';file.write_bytes(b'PUBLIC-NOT-A-SECRET');file.chmod(0o600)
        spelled=self.base/'hop'/'..'/'mock-git-root'/'PUBLIC-FIXTURE'
        self.assertFalse(spelled.is_relative_to(git));self.assertTrue(spelled.resolve().is_relative_to(git))
        with self.assertRaises(ValueError):build.private(spelled)
        with self.assertRaises(ValueError):build.private(self.base/'hop'/'..'/'mock-git-root',True)
        self.assertEqual(build.private(file),file.resolve())

    def test_private_hardlink_alias_is_refused(self):
        file=self.base/'PUBLIC-FIXTURE';file.write_bytes(b'PUBLIC-NOT-A-SECRET');file.chmod(0o600)
        os.link(file,self.base/'alias')
        with self.assertRaises(ValueError):build.private(file)

    def test_real_build_without_live_baseline_stops_before_input_access(self):
        from unittest.mock import patch
        args=['build.py','--original-flash','/NOT-TO-BE-READ','--private-inputs','/NOT-TO-BE-READ',
              '--private-output','/NOT-TO-BE-CREATED']
        with patch.object(sys,'argv',args),self.assertRaisesRegex(ValueError,'Fresh reviewed live backup'):
            build.main()


if __name__ == "__main__":
    unittest.main()
