"""Five independent-review scenarios at the corrected transaction boundary.

CRC assertion deliberately targets local strict_crc, not the unchanged general
boot parser: review B2 explicitly allows a local transaction guard. Original
expected-red witnesses/evidence remain unchanged.
"""
import unittest
import parent_uboot_program as program
from test_uboot_program import Fake


class Tests(unittest.TestCase):
    def test_read_success_plus_short_read(self):
        self.assertFalse(program.sf_success(b'SF: 1048576 bytes @ 0x200000 Read: OK\r\nshort read\r\nanyka$',0x200000,'Read'))
    def test_conflicting_second_status(self):
        self.assertFalse(program.sf_success(b'SF: 1048576 bytes @ 0x200000 Erased: OK\r\nSF: 0 bytes @ 0x200000 Erased: FAIL\r\nanyka$',0x200000,'Erased'))
    def test_success_token_suffix(self):
        self.assertFalse(program.sf_success(b'SF: 1048576 bytes @ 0x200000 Written: OK_NOT_CONFIRMED\r\nanyka$',0x200000,'Written'))
    def test_crc_success_plus_error(self):
        self.assertFalse(program.strict_crc(b'CRC32 for 82708000 ... 82808000 ==> 12345678\r\nERROR: CRC command failed\r\nanyka$',program.VERIFY,program.SIZE,'12345678'))
    def test_transaction_holds_on_short_read_without_mutation(self):
        class Diagnostic(Fake):
            def command(self,command,**kwargs):
                result=super().command(command,**kwargs)
                return result.replace(b'anyka$',b'short read\r\nanyka$') if command.startswith('sf read ') else result
        baseline=bytes(range(256))*(8*program.SIZE//256);candidate=b'C'*program.SIZE
        target=Diagnostic(baseline,candidate)
        with self.assertRaises(program.private.boot.Refused):
            program.execute(target,candidate,baseline[2*program.SIZE:3*program.SIZE],baseline,True)
        self.assertEqual((target.erases,target.writes),(0,0))


if __name__=='__main__':unittest.main()
