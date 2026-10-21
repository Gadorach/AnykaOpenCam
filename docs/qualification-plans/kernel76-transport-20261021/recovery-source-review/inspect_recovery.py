#!/usr/bin/env python3
"""Offline watchdog source/pin/text inspection; no target or firmware execution.

Reads only named source/build artifacts and safe binding manifests. Never opens
serial/RPC, raw captures, provisioning, radio payload inputs or credential files.
Creates outputs exclusively; preserve results and choose a fresh directory to rerun.
"""
import datetime
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
BASE = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
TREE = BASE / 'artifacts/native-usb76-source01'
BUILD = BASE / 'analysis/native-usb76-package01/build'
MONITOR = ROOT / 'work/lightbulb-camera/implementation-20261008T205404Z/artifacts/init-monitor-attempt50'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def watchdog_node(data):
    """Read only watchdog properties from this retained flattened DT in memory."""
    header = struct.unpack_from('>10I', data)
    require(header[0] == 0xd00dfeed and header[1] == len(data), 'DTB header')
    pos, strings = header[2], header[3]
    stack, found = [], {}
    while True:
        token = struct.unpack_from('>I', data, pos)[0]
        pos += 4
        if token == 1:
            end = data.index(0, pos)
            stack.append(data[pos:end].decode('ascii'))
            pos = (end + 4) & ~3
        elif token == 2:
            stack.pop()
        elif token == 3:
            size, nameoff = struct.unpack_from('>II', data, pos)
            pos += 8
            end = data.index(0, strings + nameoff)
            name = data[strings + nameoff:end].decode('ascii')
            value = data[pos:pos + size]
            if stack[-1] == 'watchdog@80000e4' and name in (
                    'compatible', 'reg', 'clock-frequency', 'status'):
                found[name] = (value.rstrip(b'\0').decode('ascii')
                               if name in ('compatible', 'status') else
                               list(struct.unpack('>' + 'I' * (size // 4), value)))
            pos = (pos + size + 3) & ~3
        elif token == 4:
            continue
        elif token == 9:
            break
        else:
            raise ValueError('DTB token')
    require(found.get('clock-frequency') == [46875], 'watchdog DT rate')
    require(found.get('status', 'okay') in ('ok', 'okay'), 'watchdog DT enabled')
    require(found.get('reg') == [0x080000e4, 8], 'watchdog DT resource')
    return found


def main():
    require(not sys.flags.optimize, 'do not use Python -O')
    names = ('monitor-watchdog.disasm', 'kernel-watchdog.disasm', 'binding.json')
    require(not any((OUT / n).exists() for n in names), 'preserve prior outputs')
    binding = ROOT / 'docs/qualification-plans/kernel76-transport-20261021/source-review/entry-bind.json'
    prior = json.loads(binding.read_text())
    manifest_path = BASE / 'analysis/native-usb76-package01/package76-manifest.json'
    manifest = json.loads(manifest_path.read_text())
    require(sha(MONITOR) == prior['init_sha256'], 'bound monitor pin')
    kernel = BUILD / 'vmlinux'
    require(sha(kernel) == manifest['output_sha256'][str(kernel.relative_to(ROOT))], 'final kernel pin')
    init = ROOT / 'bringup/qc07f/initramfs/init.c'
    require(sha(init) == prior['source_pins'][str(init.relative_to(ROOT))]['sha256'], 'bound init source pin')
    driver = TREE / 'linux/arch/arm/mach-ak3918ev200/watchdog.c'
    require(driver.read_bytes() == (ROOT / 'bringup/qc07f/mainline/arch/arm/mach-ak3918ev200/watchdog.c').read_bytes(), 'driver overlay equality')
    cmd = BUILD / 'arch/arm/mach-ak3918ev200/.watchdog.o.cmd'
    require(str(driver) in cmd.read_text(), 'recorded driver source path')
    selected = [
        (MONITOR, 'watchdog_test,watchdog_stoppable', names[0]),
        (kernel, 'ak_wdt_start,ak_wdt_stop,ak_wdt_set_timeout,ak_wdt_restart,watchdog_open,watchdog_ioctl,watchdog_release', names[1]),
    ]
    commands, texts = [], {}
    for path, symbols, output in selected:
        argv = ['llvm-objdump', '-d', '--disassemble-symbols=' + symbols, str(path)]
        text = subprocess.check_output(argv, text=True)
        for symbol in symbols.split(','):
            require('<' + symbol + '>:' in text, 'selected symbol: ' + symbol)
        commands.append(argv)
        texts[output] = text
    config = (BUILD / '.config').read_text().splitlines()
    options = [line for line in config if line.startswith((
        'CONFIG_WATCHDOG', '# CONFIG_WATCHDOG'))]
    require('CONFIG_WATCHDOG_AK3918EV200=y' in options, 'built-in watchdog')
    require('# CONFIG_WATCHDOG_NOWAYOUT is not set' in options, 'stoppable default')
    paths = [init, driver, TREE / 'linux/drivers/watchdog/watchdog_dev.c',
             TREE / 'linux/drivers/watchdog/watchdog_core.c',
             TREE / 'inputs/attempt45.dtb', cmd,
             BUILD / 'arch/arm/mach-ak3918ev200/watchdog.o', kernel, binding,
             manifest_path, Path(__file__).resolve()]
    report = {
        'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'scope': 'source/pin/selected ARM text only; no target operation or firmware execution',
        'python': sys.version,
        'llvm': subprocess.check_output(['llvm-objdump', '--version'], text=True).strip(),
        'input_sha256': {str(p.relative_to(ROOT)): sha(p) for p in paths},
        'monitor_matches_prior_embedded_init_binding': True,
        'final_vmlinux_matches_package_manifest': True,
        'init_source_matches_bound_build50_pin': True,
        'driver_matches_mainline_overlay': True,
        'driver_recorded_compile_source_path_matches': True,
        'selected_watchdog_build_options': options,
        'retained_candidate_DTB_watchdog_only': watchdog_node((TREE / 'inputs/attempt45.dtb').read_bytes()),
        'disassembly_argv': commands,
        'output_sha256': {n: hashlib.sha256(t.encode()).hexdigest() for n, t in texts.items()},
        'runtime_queried': False,
        'native_reset_proved': False,
    }
    for name, text in texts.items():
        with (OUT / name).open('x') as stream:
            stream.write(text)
    with (OUT / 'binding.json').open('x') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')
    print('Offline watchdog binding PASS; no target/native reset claim.')


if __name__ == '__main__':
    main()
