#!/usr/bin/env python3
"""Offline metadata/disassembly only; never execute a target image or export blobs."""
import datetime
import gzip
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import zlib

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
OLD = ROOT / 'work/lightbulb-camera/implementation-20261008T205404Z'
NEW = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
MANIFEST = NEW / 'analysis/native-usb76-package01/package76-manifest.json'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def entries(data):
    result = {}
    pos = 0
    while True:
        require(data[pos:pos + 6] == b'070701', 'newc magic')
        h = [int(data[pos + 6 + i * 8:pos + 14 + i * 8], 16) for i in range(13)]
        end = pos + 110 + h[11]
        require(end <= len(data) and data[end - 1] == 0, 'name bounds')
        name = data[pos + 110:end - 1].decode('ascii')
        start = (end + 3) & ~3
        require(start + h[6] <= len(data) and name not in result, 'entry bounds/duplicate')
        result[name] = data[start:start + h[6]]
        pos = (start + h[6] + 3) & ~3
        if name == 'TRAILER!!!':
            require(not any(data[pos:]), 'archive suffix')
            return result


def elf_region(data, address, size):
    require(data[:7] == b'\x7fELF\x01\x01\x01', 'ELF32 LE')
    require(struct.unpack_from('<H', data, 18)[0] == 40, 'ARM ELF')
    phoff = struct.unpack_from('<I', data, 28)[0]
    phsize, phnum = struct.unpack_from('<HH', data, 42)
    for i in range(phnum):
        p = struct.unpack_from('<8I', data, phoff + i * phsize)
        if p[0] == 1 and p[2] <= address and address + size <= p[2] + p[4]:
            off = p[1] + address - p[2]
            return data[off:off + size]
    raise ValueError('ELF region not file backed')


def main():
    manifest = json.loads(MANIFEST.read_text())
    packaged = ROOT / next(p for p in manifest['output_sha256'] if p.endswith('initramfs76.cpio'))
    cpio = packaged.read_bytes()
    require(sha(cpio) == manifest['output_sha256'][str(packaged.relative_to(ROOT))], 'selected cpio pin')
    embedded_path = NEW / 'analysis/native-usb76-package01/build/usr/initramfs_inc_data'
    embedded = gzip.decompress(embedded_path.read_bytes())
    require(embedded == cpio, 'actual final embedded cpio equality')
    image_path = ROOT / manifest['image']
    require(sha(image_path.read_bytes()) == manifest['image_sha256'], 'candidate76 image pin')
    current = entries(embedded)
    baseline = entries((OLD / 'artifacts/initramfs-attempt50.cpio').read_bytes())
    monitor_path = OLD / 'artifacts/init-monitor-attempt50'
    monitor = monitor_path.read_bytes()
    require(current['init'] == baseline['init'] == monitor, 'all three monitor bytes')
    build50 = json.loads((OLD / 'analysis/mainline-build-attempt50.json').read_text())
    init_receipt = next(a for a in build50['artifacts'] if a['path'].endswith('/init-monitor-attempt50'))
    require(sha(monitor) == init_receipt['sha256'], 'build50 init receipt')
    historical = [json.loads(line) for line in (OLD / 'logs/commands.jsonl').read_text().splitlines()
                  if '"net50-compile-init"' in line]
    require(len(historical) == 1 and historical[0]['returncode'] == 0, 'single successful build50 command')
    command = historical[0]
    sources = {}
    for item in command['inputs']:
        p = Path(item['path']).resolve()
        # Emit only source-code pins, never payload/config digests.
        if p.suffix not in ('.c', '.h'):
            continue
        sources[str(p.relative_to(ROOT))] = {
            'sha256': sha(p.read_bytes()),
            'matches_build50_input': sha(p.read_bytes()) == item['sha256'],
        }
    nm = subprocess.check_output(['llvm-nm', '-n', str(monitor_path)], text=True)
    symbols = {line.split()[2]: int(line.split()[0], 16) for line in nm.splitlines() if len(line.split()) == 3}
    payload = elf_region(monitor, symbols['radio_payload_begin'],
                         symbols['radio_payload_end'] - symbols['radio_payload_begin'])
    local = (OLD / 'artifacts/retail-radio-payload36.bin').read_bytes()
    prep = json.loads((OLD / 'analysis/radio-payload36-preparation.json').read_text())
    original = Path(prep['input']).read_bytes()
    offset = struct.unpack_from('<I', original, 12)[0]
    private_checks = {
        'embedded_equals_local_payload': payload == local,
        'local_matches_retained_private_preparation_pin': sha(local) == prep['payload_sha256'],
        'original_matches_retained_private_input_pin': sha(original) == prep['input_sha256'],
        'local_equals_original_payload_slice': local == original[offset:],
        'exact_length_151056': len(payload) == 151056,
        'raw_reflected_CRC_residue_zero': (zlib.crc32(payload) ^ 0xffffffff) == 0,
        'header_chip8400_base20000000_offset3072_AESoff_CRCenabled':
            struct.unpack_from('<H', original, 0x78)[0] == 0x8400
            and struct.unpack_from('<I', original, 4)[0] == 0x20000000
            and offset == 3072 and not original[27] & 4 and bool(original[27] & 8),
    }
    require(all(private_checks.values()), 'private/local firmware admission mismatch')
    selected = ['init_main', 'radio_load', 'radio_boot_command', 'radio_boot_response',
                'radio_bulk', 'radio_reset_toggles', 'radio_info', 'radio_dev_open',
                'sysfs_prefix_value', 'watchdog_stoppable']
    argv = ['llvm-objdump', '-d', '--disassemble-symbols=' + ','.join(selected), str(monitor_path)]
    disassembly = subprocess.check_output(argv, text=True)
    with (OUT / 'monitor50-selected.disasm').open('x') as stream:
        stream.write(disassembly)
    report = {
        'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'scope': 'offline source and bounded ARM text disassembly; no target or binary execution',
        'python': sys.version,
        'llvm_objdump': subprocess.check_output(['llvm-objdump', '--version'], text=True).splitlines()[0],
        'script_sha256': sha(Path(__file__).read_bytes()),
        'image_sha256': manifest['image_sha256'],
        'embedded_cpio_equals_final_selected_cpio': True,
        'embedded_init_equals_attempt50_cpio_and_retained_init': True,
        'init_bytes': len(monitor),
        'init_sha256': sha(monitor),
        'init_matches_build50_manifest': True,
        'build50_compile_start_utc': command['start_utc'],
        'build50_compile_argv': command['argv'],
        'source_pins': sources,
        'private_firmware_admission_booleans_only': private_checks,
        'selected_text_symbols': {s: hex(symbols[s]) for s in selected},
        'disassembly_argv': argv,
        'disassembly_sha256': sha(disassembly.encode()),
        'target_operation': False,
        'native_transport_pass': False,
    }
    with (OUT / 'entry-bind.json').open('x') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')
    print('Offline binding PASS; metadata and selected text disassembly saved. No firmware digest exported.')


if __name__ == '__main__':
    main()
