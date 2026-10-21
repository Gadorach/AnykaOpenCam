#!/usr/bin/env python3
"""Small, disposable fixtures for the handoff helpers; no hardware is accessed."""

import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import random
import struct
import sys
import tempfile
import types
import unittest
from unittest import mock
import zlib


SCRIPTS = Path(__file__).resolve().parents[1]


def load(name):
    spec = importlib.util.spec_from_file_location(name, SCRIPTS / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


validate = load('validate_dumps')
inspect = load('inspect_firmware')
capture = load('capture_uart')


def invoke(module, arguments):
    stdout, stderr = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
        try:
            code = module.main([str(a) for a in arguments])
        except SystemExit as exc:
            code = exc.code
    return code, stdout.getvalue(), stderr.getvalue()


def uimage(payload):
    header = struct.pack('>7I4B32s', 0x27051956, 0, 1700000000, len(payload),
                         0x81008000, 0x81008040, zlib.crc32(payload),
                         5, 2, 2, 0, b'Linux fixture')
    header = header[:4] + struct.pack('>I', zlib.crc32(header)) + header[8:]
    return header + payload


def pad4(data):
    return data + bytes((-len(data)) % 4)


def dtb():
    names = b'model\0compatible\0bootargs\0'
    def prop(name, data):
        return struct.pack('>3I', 3, len(data), names.index(name + b'\0')) + pad4(data)
    body = struct.pack('>I', 1) + bytes(4)
    body += prop(b'model', b'fixture camera\0')
    body += prop(b'compatible', b'anyka,ak3918av100\0fixture,board\0')
    body += struct.pack('>I', 1) + pad4(b'chosen\0')
    body += prop(b'bootargs', b'console=ttySAK0,115200n8\0')
    body += struct.pack('>3I', 2, 2, 9)
    off_struct, off_strings = 56, 56 + len(body)
    header = struct.pack('>10I', 0xd00dfeed, off_strings + len(names), off_struct,
                         off_strings, 40, 17, 16, 0, len(names), len(body))
    return header + bytes(16) + body + names


def squash_superblock():
    missing = 0xffffffffffffffff
    header = struct.pack('<5I6H8Q', 0x73717368, 1, 1700000000, 65536, 0,
                         4, 16, 0, 1, 4, 0, 0, 256, 208, missing,
                         96, 160, missing, missing)
    return header + bytes(256 - len(header))


class FixtureCase(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='anyka-helper-fixtures-')
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def file(self, name, content):
        path = self.root / name
        path.write_bytes(content)
        return path


class DumpTests(FixtureCase):
    def test_identical_reads_and_json_and_input_preservation(self):
        content = b'header: firmware fixture\0' + bytes(range(256)) + b'\xff' * 9000
        a, b = self.file('a.bin', content), self.file('b.bin', content)
        report = self.root / 'validation.json'
        code, _, _ = invoke(validate, [a, b, '--expected-size', len(content), '--json-out', report])
        self.assertEqual(code, 0)
        data = json.loads(report.read_text())
        self.assertEqual(data['verdict'], 'matching_nontrivial_reads')
        self.assertEqual(data['inputs'][0]['sha256'], hashlib.sha256(content).hexdigest())
        self.assertTrue(data['all_byte_identical'])
        self.assertEqual(a.read_bytes(), content)
        self.assertEqual(b.read_bytes(), content)

    def test_difference_after_chunk_boundary_in_eight_mib_reads(self):
        content = b'fixture\0' + bytes([255]) * (8388608 - 8)
        changed = bytearray(content)
        changed[4 * 1024 * 1024 + 17] = 0x5a
        a, b = self.file('a.bin', content), self.file('b.bin', changed)
        report = self.root / 'different.json'
        code, _, _ = invoke(validate, [a, b, '--expected-size', '8388608', '--json-out', report])
        self.assertEqual(code, 1)
        result = json.loads(report.read_text())
        self.assertEqual(result['comparisons'][0]['first_difference_offset'], 4194321)
        self.assertEqual(result['verdict'], 'reads_differ')

    def test_short_read_reports_eof_difference(self):
        a = self.file('a.bin', b'nontrivial payload 1234')
        b = self.file('b.bin', b'nontrivial payload')
        self.assertEqual(validate.first_difference(a, b), len(b.read_bytes()))
        self.assertEqual(invoke(validate, [a, b])[0], 1)

    def test_size_is_optional_and_only_enforced_when_requested(self):
        a, b = self.file('a.bin', b'valid fixture'), self.file('b.bin', b'valid fixture')
        self.assertEqual(invoke(validate, [a, b])[0], 0)
        self.assertEqual(invoke(validate, [a, b, '--expected-size', '8388608'])[0], 1)

    def test_blank_uniform_empty_and_repeating_are_diagnostic_failures(self):
        for content in (b'', b'\xff' * 8192, b'\0' * 8192, b'\xa5' * 8192, b'ABCD' * 2048):
            with self.subTest(size=len(content), first=content[:4]):
                a, b = self.file('a.bin', content), self.file('b.bin', content)
                self.assertEqual(invoke(validate, [a, b])[0], 1)

    def test_high_entropy_is_not_a_corruption_verdict(self):
        rng = random.Random(20261007)
        content = bytes(rng.randrange(256) for _ in range(16384))
        a, b = self.file('a.bin', content), self.file('b.bin', content)
        result = validate.make_report([a, b])
        self.assertGreater(result['inputs'][0]['entropy_bits_per_byte'], 7.95)
        self.assertEqual(result['exit_code'], 0)

    def test_duplicate_file_reference_is_not_independent_input(self):
        a = self.file('a.bin', b'nontrivial firmware fixture')
        self.assertEqual(invoke(validate, [a, a])[0], 1)

    def test_two_inputs_required_and_missing_input_fails(self):
        a = self.file('a.bin', b'nontrivial firmware fixture')
        self.assertEqual(invoke(validate, [a])[0], 2)
        self.assertEqual(invoke(validate, [a, self.root / 'absent.bin'])[0], 2)

    def test_json_never_overwrites_existing_file_or_an_input(self):
        a, b = self.file('a.bin', b'firmware'), self.file('b.bin', b'firmware')
        target = self.file('existing.json', b'KEEP')
        self.assertEqual(invoke(validate, [a, b, '--json-out', target])[0], 2)
        self.assertEqual(target.read_bytes(), b'KEEP')
        self.assertEqual(invoke(validate, [a, b, '--json-out', a])[0], 2)
        self.assertEqual(a.read_bytes(), b'firmware')


class FirmwareTests(FixtureCase):
    def test_valid_uimage_and_corrupt_payload_and_header(self):
        image = uimage(b'Linux version fixture\0payload123')
        result = inspect.inspect_uimage(image, 0)
        self.assertTrue(result['valid_checks'])
        self.assertTrue(result['header_crc_valid'])
        self.assertTrue(result['payload_crc_valid'])
        changed = bytearray(image)
        changed[32] ^= 1
        self.assertIn('header_crc_mismatch', inspect.inspect_uimage(changed, 0)['errors'])
        changed = bytearray(image)
        changed[-1] ^= 1
        self.assertIn('payload_crc_mismatch', inspect.inspect_uimage(changed, 0)['errors'])

    def test_uimage_truncated_header_and_declared_payload(self):
        image = uimage(b'payload123')
        self.assertEqual(inspect.inspect_uimage(image[:20], 0)['errors'], ['truncated_header'])
        result = inspect.inspect_uimage(image[:-1], 0)
        self.assertFalse(result['valid_checks'])
        self.assertTrue(result['header_crc_valid'])
        self.assertIsNone(result['payload_crc_valid'])
        self.assertIn('payload_extends_beyond_input', result['errors'])

    def test_valid_fdt_at_nonzero_offset_samples_properties(self):
        image = bytes(27) + dtb() + b'padding'
        result = inspect.inspect_fdt(image, 27)
        self.assertTrue(result['valid_checks'], result)
        self.assertEqual(result['sample_properties']['/:model']['strings'], ['fixture camera'])
        self.assertEqual(result['sample_properties']['/:compatible']['strings'][0], 'anyka,ak3918av100')
        self.assertIn('/chosen:bootargs', result['sample_properties'])

    def test_fdt_truncation_and_unbalanced_tokens(self):
        image = dtb()
        self.assertFalse(inspect.inspect_fdt(image[:-1], 0)['valid_checks'])
        changed = bytearray(image)
        struct.pack_into('>I', changed, 56, 2)
        self.assertIn('unbalanced_end_node', inspect.inspect_fdt(changed, 0)['errors'])

    def test_fdt_malicious_property_length_and_name_offset(self):
        image = dtb()
        for location, value in ((68, 0xffffffff), (72, 0xfffffffe)):
            changed = bytearray(image)
            struct.pack_into('>I', changed, location, value)
            result = inspect.inspect_fdt(changed, 0)
            self.assertFalse(result['valid_checks'])
            self.assertIn('property_value_or_name_offset_out_of_bounds', result['errors'])

    def test_fdt_alignment_overlap_and_old_version(self):
        for location, value in ((8, 57), (12, 56), (20, 16)):
            changed = bytearray(dtb())
            struct.pack_into('>I', changed, location, value)
            self.assertFalse(inspect.inspect_fdt(changed, 0)['valid_checks'])

    def test_squashfs_superblock_and_truncation(self):
        image = squash_superblock()
        self.assertTrue(inspect.inspect_squashfs(image, 0)['plausible_v4_superblock'])
        self.assertFalse(inspect.inspect_squashfs(image[:60], 0)['plausible_v4_superblock'])
        self.assertIn('filesystem_extent_out_of_bounds', inspect.inspect_squashfs(image[:-1], 0)['errors'])
        changed = bytearray(image)
        struct.pack_into('<I', changed, 12, 60000)
        self.assertIn('invalid_block_size_or_log', inspect.inspect_squashfs(changed, 0)['errors'])

    def test_full_scan_keeps_input_untouched_and_has_entropy_windows(self):
        content = b'prefix' + uimage(b'payload') + bytes(11) + dtb() + squash_superblock() + b'U-Boot 2019.10 AnyCloud\0'
        image = self.file('firmware.bin', content)
        output = self.root / 'scan.json'
        before_files = set(self.root.iterdir())
        code, _, _ = invoke(inspect, [image, '--window-size', '64', '--json-out', output])
        self.assertEqual(code, 0)
        self.assertEqual(image.read_bytes(), content)
        self.assertEqual(set(self.root.iterdir()), before_files | {output})
        report = json.loads(output.read_text())
        self.assertTrue(report['uimage_candidates'][0]['valid_checks'])
        self.assertTrue(report['fdt_candidates'][0]['valid_checks'])
        self.assertTrue(report['squashfs_candidates'][0]['plausible_v4_superblock'])
        self.assertGreater(len(report['entropy_windows']), 1)
        self.assertTrue(any('U-Boot' in hit['text'] for hit in report['interesting_strings']))

    def test_scan_empty_signature_only_limits_and_no_overwrite(self):
        image = self.file('empty.bin', b'')
        self.assertEqual(invoke(inspect, [image])[0], 0)
        image.write_bytes(b'\x27\x05\x19\x56' * 10)
        report = inspect.make_report(image.read_bytes(), image, candidate_limit=2)
        self.assertEqual(len(report['uimage_candidates']), 2)
        self.assertTrue(report['scan_limits_reached']['uimage'])
        saved = image.read_bytes()
        self.assertEqual(invoke(inspect, [image, '--json-out', image])[0], 2)
        self.assertEqual(image.read_bytes(), saved)
        self.assertEqual(invoke(inspect, [image, '--max-bytes', '8'])[0], 2)


class UARTTests(FixtureCase):
    def serial_module(self):
        instances = []
        class FakeSerial:
            def __init__(self, **kwargs):
                self.kwargs = kwargs
                self.dtr = self.rts = None
                self.port = None
                self.opened = self.closed = False
                self.chunks = iter((b'\0boot\x1b[31m\xff', b'kernel\r\n'))
                instances.append(self)
            def open(self):
                assert self.kwargs['port'] is None
                assert self.dtr is False and self.rts is False
                self.opened = True
            def read(self, size):
                return next(self.chunks, b'')
            def write(self, data):
                raise AssertionError('UART transmit is forbidden')
            def close(self):
                self.closed = True
        module = types.SimpleNamespace(Serial=FakeSerial, EIGHTBITS=8, PARITY_NONE='N',
                                       STOPBITS_ONE=1, SerialException=OSError)
        return module, instances

    def test_receive_only_capture_exact_raw_bytes_and_inactive_lines_before_open(self):
        serial, instances = self.serial_module()
        log = self.root / 'uart.bin'
        with mock.patch.dict(sys.modules, {'serial': serial}), mock.patch.object(capture.time, 'monotonic', side_effect=[0.0, 0.0, 0.5, 1.0]):
            code, stdout, _ = invoke(capture, ['--port', 'FAKE', '--baud', '115200', '--seconds', '1', '--log', log])
        self.assertEqual(code, 0)
        self.assertEqual(log.read_bytes(), b'\0boot\x1b[31m\xffkernel\r\n')
        self.assertNotIn('\x1b', stdout)
        self.assertTrue(instances[0].opened and instances[0].closed)
        self.assertFalse(instances[0].kwargs['xonxoff'])
        self.assertFalse(instances[0].kwargs['rtscts'])
        self.assertFalse(instances[0].kwargs['dsrdtr'])

    def test_existing_log_refused_before_port_is_touched(self):
        serial, instances = self.serial_module()
        log = self.file('uart.bin', b'KEEP')
        with mock.patch.dict(sys.modules, {'serial': serial}):
            code, _, _ = invoke(capture, ['--port', 'FAKE', '--baud', '115200', '--seconds', '1', '--log', log])
        self.assertEqual(code, 2)
        self.assertEqual(log.read_bytes(), b'KEEP')
        self.assertFalse(instances)

    def test_missing_dependency_does_not_install_or_create_log(self):
        log = self.root / 'uart.bin'
        with mock.patch.dict(sys.modules, {'serial': None}):
            code, _, error = invoke(capture, ['--port', 'FAKE', '--baud', '115200', '--seconds', '1', '--log', log])
        self.assertEqual(code, 2)
        self.assertIn('installs nothing', error)
        self.assertFalse(log.exists())


if __name__ == '__main__':
    unittest.main(verbosity=2)
