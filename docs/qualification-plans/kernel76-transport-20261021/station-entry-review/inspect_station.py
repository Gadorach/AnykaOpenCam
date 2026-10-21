#!/usr/bin/env python3
"""Fixed offline station76 audit; no imports of project helpers or target I/O.

Reads public manifests, named source/config files and the own station module only.
Does not open images/cpio/vmlinux, radio payloads, credentials or private state.
Outputs are exclusive: a second invocation refuses before inspecting inputs.
"""
import sys

if sys.flags.optimize or sys.argv[1:]:
    raise SystemExit("Refuse optimized Python or arguments")

from pathlib import Path
import datetime
import hashlib
import json
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
OUT = HERE / "run02"
OUT.mkdir(exist_ok=False)
FOUND = ROOT / "work/lightbulb-camera/foundation-20261011T041148Z"
ART = FOUND / "artifacts/native-usb76-source01"
ST = ART / "station76-final"
PKG = FOUND / "analysis/native-usb76-package01"
inputs = {}
checks = {}


def read(path):
    path = Path(path)
    rel = str(path.relative_to(ROOT))
    data = path.read_bytes()
    inputs[rel] = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    return data


def check(name, condition):
    checks[name] = bool(condition)
    if not condition:
        raise RuntimeError("Offline source check failed: " + name)


package = json.loads(read(PKG / "package76-manifest.json"))
prepare = json.loads(read(FOUND / "analysis/native-usb76-build01/prepare76-manifest.json"))
verification = json.loads(read(ROOT / "evidence/qualification/qc07f/kernel76-20261020/final-verify01/result.json"))
check("package_manifest_matches_retained_independent_receipt",
      inputs[str((PKG / "package76-manifest.json").relative_to(ROOT))]["sha256"] == verification["manifest_sha256"])
check("retained_final_archive_and_module_comparison_pass",
      verification["all_pass"] and verification["checks"]["final_build_embeds_selected_new_archive"]
      and verification["checks"]["fresh_final_radio_equals_embedded_and_bootstrap"])
check("image_pin_in_manifest", package["image_sha256"] == "bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f")
source_count = 0
for name, pin in prepare["paired_radio_source75_pins"].items():
    rel = Path(name).relative_to("station75")
    check("station_source_pin:" + str(rel), hashlib.sha256(read(ST / rel)).hexdigest() == pin)
    source_count += 1

module = ST / "hgic_smac/hgics.ko"
check("own_module_pin_matches_package", hashlib.sha256(read(module)).hexdigest() == package["entry_details"]["hgics.ko"]["sha256"])
for path in (PKG / "build/.config", PKG / "build/Module.symvers"):
    check("final_build_pin:" + path.name, hashlib.sha256(read(path)).hexdigest() == package["output_sha256"][str(path.relative_to(ROOT))])

extra = [
    ROOT / "bringup/qc07f/initramfs/init.c",
    ROOT / "bringup/qc07f/initramfs/wifi-monitor.h",
    ROOT / "bringup/qc07f/userland/dhcp-script-bridge.c",
    ROOT / "bringup/qc07f/userland/netbench.c",
    ROOT / "work/lightbulb-camera/implementation-20261008T205404Z/build_network_qualification.py",
    ROOT / "work/lightbulb-camera/implementation-20261008T205404Z/build_wifi_userland.py",
    ROOT / "work/lightbulb-camera/implementation-20261008T205404Z/sources/busybox-1.37.0/busybox-1.37.0/networking/ping.c",
    ROOT / "evidence/qualification/qc07f/stock-reference-20261013/optical-live01/parent_console20a.py",
    ROOT / "evidence/qualification/qc07f/stock-reference-20261013/optical-live01/STATION_PAIR_REVIEW20A01.json",
    ROOT / "evidence/qualification/qc07f/stock-reference-20261013/optical-live01/SENSOR_STATION20A01.json",
    ROOT / "evidence/qualification/qc07f/stock-reference-20261013/optical-live01/NETWORK20A01.json",
    PKG / "recipe-snapshot/package.py",
    ART / "linux/net/mac80211/main.c",
    PKG / "build/include/generated/utsrelease.h",
    PKG / "build/include/generated/autoconf.h",
    ST / "hgic_smac/hgics.mod.c",
    ST / "hgic_smac/.core.o.cmd",
    ST / "utils/.if_usb.o.cmd",
]
texts = {str(p.relative_to(ROOT)): read(p).decode() for p in extra}
core = (ST / "hgic_smac/core.c").read_text()
usb = (ST / "utils/if_usb.c").read_text()
fw = (ST / "utils/fwctrl.c").read_text()
bgn = (ST / "hgic_smac/11bgn.c").read_text()
wifi = texts["bringup/qc07f/initramfs/wifi-monitor.h"]
config = (PKG / "build/.config").read_text()
check("no_op_download_function", "return 1;" in core[core.index("static int hgics_download_fw"):core.index("static void hgics_delay_init")])
check("FWINFO_chip_only_init_gate", "ret != 24 || hg->fwinfo.chip_id != HGIC_WLAN_8400" in core)
check("asynchronous_init", "schedule_work(&hg->delay_init);" in core)
check("RX_URBs_submitted_before_probe_post", usb.index("ret = hgic_usb_submit_rx_urbs(usbdev);") < usb.index("usbdev->bus.probe_post(usbdev->bus.bus_priv);"))
check("no_direct_USB_reset_or_GPIO_API", not any(s in usb + core for s in ("usb_reset_device(", "usb_clear_halt(", "gpio_set_value(", "gpiod_set_value(")))
check("TX_boot_data_and_RUN_not_allowlisted", "skb->data[8] == 0" in usb and "hdr->type != HGIC_HDR_TYPE_BOOTDL_DATA" not in usb)
check("automatic_detect_on_TX_failure", "schedule_work(&hg->detect_work);" in core)
check("automatic_detect_reinitialization", "status = hgic_bootdl_cmd_enter" in core and "schedule_work(&hg->delay_init);" in core)
check("station_only_advertised", "hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION)" in bgn and "hw->wiphy->n_iface_combinations = 0;" in bgn)
check("CCMP_only_advertised", "initial_cipher_profile[] = { WLAN_CIPHER_SUITE_CCMP }" in bgn)
check("country_command_denied_by_allowlist", "HGIC_CMD_SET_COUNTERY" not in fw[fw.index("switch (cmd_id)"):fw.index("memset(&resp")])
check("guard_fixed_settle_not_retirement", "long settle[2]={2,0};" in wifi and "no_bootdl=1" in wifi and "cleanup:" in wifi)
check("no_bootdl_not_consulted_by_current_core", core.count("no_bootdl") == 2)
check("HZ100_TX_semaphore_wait_is_10s", "CONFIG_HZ=100\n" in config and "down_timeout(&usbdev->tx_sema, 1000)" in usb)
check("module_versions_disabled", "# CONFIG_MODVERSIONS is not set" in config)
check("IPv6_enabled_not_silently_disabled", "CONFIG_IPV6=y" in config)
check("DHCP_ELF_runs_default_shell_script", '"/bin/udhcpc.script"' in texts["bringup/qc07f/userland/dhcp-script-bridge.c"])
buildtext = texts["work/lightbulb-camera/implementation-20261008T205404Z/build_network_qualification.py"]
check("default_DHCP_changes_route_and_DNS", "route add default" in buildtext and "/etc/resolv.conf" in buildtext)
ping = texts["work/lightbulb-camera/implementation-20261008T205404Z/sources/busybox-1.37.0/busybox-1.37.0/networking/ping.c"]
check("BusyBox_ping_has_no_M_DF_option", '#define OPT_STRING "qvAc:+s:t:+w:+W:+I:np:i:4"' in ping and "IP_MTU_DISCOVER" not in ping)
check("old_netbench_pins_stale_literal_peer", 'strcmp(argv[1],"192.168.1.15")' in texts["bringup/qc07f/userland/netbench.c"])

commands = []
for label, argv in (
    ("llvm-version", ["llvm-readelf", "--version"]),
    ("module-metadata", ["llvm-readelf", "-h", "-A", "-p", ".modinfo", str(module)]),
    ("module-undefined", ["llvm-nm", "--undefined-only", str(module)]),
):
    start = datetime.datetime.now(datetime.timezone.utc).isoformat()
    result = subprocess.run(argv, capture_output=True, text=True, timeout=30, check=True)
    for suffix, value in (("stdout", result.stdout), ("stderr", result.stderr)):
        (OUT / (label + "." + suffix)).open("x").write(value)
    commands.append({"argv": argv, "start_utc": start, "exit": result.returncode})
metadata = (OUT / "module-metadata.stdout").read_text()
undefined = (OUT / "module-undefined.stdout").read_text()
check("module_ARM_little_endian_relocatable", "ARM" in metadata and "little endian" in metadata and "REL (Relocatable file)" in metadata)
check("module_USB_alias_only8400", "usb:vA012p8400" in metadata)
check("module_no_external_module_dependencies", "depends=" in metadata)
check("retained_maintenance_code_still_links_request_firmware", "request_firmware" in undefined)
check("compiled_module_no_GPIO_or_USB_reset_reference", not any(s in undefined for s in ("gpio_", "gpiod_", "usb_reset_device", "usb_clear_halt")))

excerpts = {
    ST / "hgic_smac/core.c": [(95, 130), (703, 725), (900, 1018), (1044, 1089), (1129, 1218), (1253, 1288)],
    ST / "utils/if_usb.c": [(17, 59), (74, 122), (184, 247), (265, 336), (339, 478)],
    ST / "utils/fwctrl.c": [(49, 116), (156, 188), (219, 246), (322, 346)],
    ST / "hgic_smac/11bgn.c": [(111, 190)],
    ROOT / "bringup/qc07f/initramfs/wifi-monitor.h": [(1, 37)],
    ROOT / "bringup/qc07f/userland/dhcp-script-bridge.c": [(1, 11)],
    ROOT / "work/lightbulb-camera/implementation-20261008T205404Z/build_network_qualification.py": [(47, 71)],
}
with (OUT / "excerpts.txt").open("x") as out:
    for path, ranges in excerpts.items():
        lines = path.read_text().splitlines()
        out.write("\n" + str(path.relative_to(ROOT)) + "\n")
        for begin, end in ranges:
            for number in range(begin, min(end, len(lines)) + 1):
                out.write(f"{number}: {lines[number - 1]}\n")

result = {
    "utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "scope": "OFFLINE_SOURCE_REVIEW_ONLY_EXECUTION_NO_GO",
    "invocation": "python3 -B docs/qualification-plans/kernel76-transport-20261021/station-entry-review/inspect_station.py",
    "python": sys.version,
    "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    "station_source_count": source_count,
    "inputs": inputs,
    "checks": checks,
    "check_count": len(checks),
    "all_source_checks_pass": all(checks.values()),
    "commands": commands,
    "image_cpio_vmlinux_payload_read": False,
    "embedded_image_binding": "Cited retained package + independent final verification only; not freshly rederived from image bytes",
    "target_RPC_status_network_build_ARM_execution_private_input_Git": False,
    "native_station_association_MTU_qualified": False,
}
with (OUT / "binding.json").open("x") as out:
    json.dump(result, out, indent=2)
    out.write("\n")
print(f"Offline station source audit: {len(checks)} checks PASS; {source_count} source pins; execution NO-GO")
