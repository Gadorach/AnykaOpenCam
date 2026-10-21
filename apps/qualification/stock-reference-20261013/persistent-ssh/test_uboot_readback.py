"""Pure parser/dispatch fixtures; no private file, socket, process or UART IO."""
import unittest
from unittest.mock import patch
import parent_uboot_readback as proof


class Tests(unittest.TestCase):
    def test_exact_full_compare(self):
        good=b'\nTotal of 1048576 byte(s) were the same\r\n\r\nanyka$'
        self.assertTrue(proof.same_bytes(good,proof.SIZE))
        for bad in (good+good,good.replace(b'1048576',b'1048575'),good+b'\nError\n',
                    good+b'\nbyte != byte\n',b'Total of 1048576 bytes were the same\n'):
            self.assertFalse(proof.same_bytes(bad,proof.SIZE))

    def test_negative_requires_mismatch_and_zero_prefix(self):
        good=b'byte at 0x82408000 (0x68) != byte at 0x82608000 (0x00)\r\nTotal of 0 byte(s) were the same\r\nanyka$'
        self.assertTrue(proof.same_bytes(good,0))
        self.assertFalse(proof.same_bytes(good,proof.SIZE))
        self.assertFalse(proof.same_bytes(good.replace(b'!=',b'=='),0))
        self.assertFalse(proof.same_bytes(good.replace(b'Total of 0',b'Total of 1'),0))

    def test_read_exact_range_once(self):
        good=b'\nSF: 1048576 bytes @ 0x200000 Read: OK\r\nanyka$'
        self.assertTrue(proof.full_read(good))
        for bad in (good+good,good.replace(b'1048576',b'1048575'),good.replace(b'0x200000',b'0x300000'),
                    good+b'\nSF read failed\n',b'SF: read OK\nanyka$'):
            self.assertFalse(proof.full_read(bad))

    def test_strict_SF_complete_record_and_no_other_diagnostics(self):
        for verb in ('Read','Erased','Written'):
            good=f'SF: 1048576 bytes @ 0x200000 {verb}: OK\r\nanyka$'.encode()
            self.assertTrue(proof.sf_record(good,0x200000,verb))
            for suffix in (b'_NOT_CONFIRMED',b'AY',b' extra'):
                self.assertFalse(proof.sf_record(good.replace(b'OK',b'OK'+suffix),0x200000,verb))
            for diagnostic in (b'short read',b'SF: 0 bytes @ 0x200000 Erased: FAIL',
                               b'ERROR: operation failed',b'unknown diagnostic'):
                self.assertFalse(proof.sf_record(good.replace(b'anyka$',diagnostic+b'\r\nanyka$'),0x200000,verb))

    def test_strict_CRC_no_extra_diagnostics(self):
        good=b'CRC32 for 82708000 ... 82808000 ==> 12345678\r\nanyka$'
        self.assertTrue(proof.strict_crc(good,0x82708000,proof.SIZE,'12345678'))
        for text in (b'ERROR: CRC command failed',b'short read',b'CRC failed',b'unknown diagnostic'):
            self.assertFalse(proof.strict_crc(good.replace(b'anyka$',text+b'\r\nanyka$'),0x82708000,proof.SIZE,'12345678'))

    def test_measured_erase_progress_prefix_only(self):
        summary=b'SF: 1048576 bytes @ 0x200000 Erased: OK\r\n\r\nanyka$'
        progress=b'\n   \rEraseing, 4294965348% 394758 B/s'
        self.assertTrue(proof.sf_record(progress+summary,0x200000,'Erased'))
        repeated=b'\rEraseing, 25% 123 B/s\rEraseing, 100% 456 B/s'
        self.assertTrue(proof.sf_record(repeated+summary,0x200000,'Erased'))
        for bad in (progress+b'ERROR: erase failed\r\n'+summary,
                    progress+b'short read\r\n'+summary,
                    progress+summary.replace(b'OK',b'OK_NOT_CONFIRMED'),
                    b'Unknown progress '+summary,
                    progress+summary.replace(b'Erased',b'Written')):
            self.assertFalse(proof.sf_record(bad,0x200000,'Erased'))

    def test_only_fixed_readback_additions_dispatch(self):
        instance=proof.RestoreReadback.__new__(proof.RestoreReadback)
        with patch.object(proof.private.boot.Boot,'command',return_value=b'FIXTURE') as command:
            for value in (proof.READ,proof.CMP,proof.CRC,proof.NEGATIVE):
                self.assertEqual(instance.command(value),b'FIXTURE')
            self.assertEqual(command.call_count,4)

    def test_no_program_boot_or_arbitrary_memory_entry(self):
        instance=proof.RestoreReadback.__new__(proof.RestoreReadback)
        for value in ('sf erase 0x200000 0x100000','sf write 0x82408000 0x200000 0x100000',
                      'saveenv','bootm 0x81808000','mw.b 0x08000000 0 1',
                      'sf read 0x82608000 0 0x100000','cmp.b 0 0 1'):
            with self.assertRaises(proof.private.boot.Refused):instance.command(value)


if __name__=='__main__':unittest.main()
