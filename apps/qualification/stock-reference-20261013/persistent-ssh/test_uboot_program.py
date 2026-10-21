"""In-memory U-Boot transaction fixtures. No actual private data or target IO."""
import unittest
from unittest.mock import patch
import parent_uboot_program as program


class Fake:
    def __init__(self,baseline,candidate,fault=None):
        self.flash=bytearray(baseline);self.candidate=candidate
        self.restore=baseline[2*program.SIZE:3*program.SIZE];self.scratch=b''
        self.stage='verify';self.fault=fault;self.commands=[];self.erases=0;self.writes=0;self.owners=0
    def caught(self,timeout):pass
    def sf_probe(self):pass
    def check_stream(self):pass
    def log(self,*args,**kwargs):pass
    def owner(self,margin):
        self.owners+=1
        if self.fault=='late-owner' and self.owners==2:raise RuntimeError('fixture owner changed')
    def command(self,command,**kwargs):
        self.commands.append(command)
        if command==program.ERASE:
            assert self.stage=='erase-admitted';self.stage='erase-in-flight';self.erases+=1
            if self.fault=='erase-fail':return b'SF: erase failed\r\nanyka$'
            self.flash[2*program.SIZE:3*program.SIZE]=b'\xff'*program.SIZE
            if self.fault=='erase-incomplete':self.flash[2*program.SIZE]=0
            return b'SF: 1048576 bytes @ 0x200000 Erased: OK\r\nanyka$'
        if command==program.WRITE:
            assert self.stage=='write-admitted';self.stage='write-in-flight';self.writes+=1
            if self.fault=='write-fail':return b'SF: write failed\r\nanyka$'
            self.flash[2*program.SIZE:3*program.SIZE]=self.candidate
            if self.fault=='readback':self.flash[2*program.SIZE]=self.candidate[0]^1
            if self.fault=='collateral':self.flash[0]^=1
            return b'SF: 1048576 bytes @ 0x200000 Written: OK\r\nanyka$'
        if command.startswith('sf read '):
            _,_,address,offset,length=command.split();offset=int(offset,16);length=int(length,16)
            assert int(address,16)==program.VERIFY and length==program.SIZE and 0<=offset<=7*program.SIZE
            self.scratch=bytes(self.flash[offset:offset+length])
            if self.fault=='short-read':return b'SF: 1 bytes @ 0x200000 Read: OK\r\nanyka$'
            return f'SF: {length} bytes @ {offset:#x} Read: OK\r\nanyka$'.encode()
        if command.startswith('crc32 '):
            _,address,length=command.split();address=int(address,16);length=int(length,16)
            data={0x82408000:self.candidate,0x82608000:self.restore,program.VERIFY:self.scratch}[address]
            digest=program.private.boot.crc_bzip2(data)
            if self.fault=='candidate-CRC' and address==0x82408000:digest^=1
            if self.fault=='restore-CRC' and address==0x82608000:digest^=1
            return f'CRC32 for {address:08x} ... {address+length:08x} ==> {digest:08x}\r\nanyka$'.encode()
        if command in (program.CMP_RESTORE,program.CMP_CANDIDATE):
            expected=self.restore if command==program.CMP_RESTORE else self.candidate
            count=program.SIZE if expected==self.scratch else 0
            return ((b'byte != byte\r\n' if count==0 else b'')+
                    f'Total of {count} byte(s) were the same\r\nanyka$'.encode())
        raise AssertionError(command)


class Tests(unittest.TestCase):
    def setUp(self):
        self.baseline=bytes(range(256))*(8*program.SIZE//256)
        self.restore=self.baseline[2*program.SIZE:3*program.SIZE]
        self.candidate=b'C'*program.SIZE
    def test_default_verification_does_not_write(self):
        target=Fake(self.baseline,self.candidate)
        self.assertFalse(program.execute(target,self.candidate,self.restore,self.baseline))
        self.assertEqual((target.erases,target.writes),(0,0));self.assertEqual(target.flash,self.baseline)
    def test_exact_program_and_full_readback(self):
        target=Fake(self.baseline,self.candidate)
        self.assertTrue(program.execute(target,self.candidate,self.restore,self.baseline,True))
        self.assertEqual((target.erases,target.writes),(1,1));self.assertEqual(target.stage,'complete')
        self.assertEqual(target.flash,self.baseline[:2*program.SIZE]+self.candidate+self.baseline[3*program.SIZE:])
    def test_prewrite_failures_zero_mutations(self):
        for fault in ('candidate-CRC','restore-CRC','short-read','late-owner'):
            target=Fake(self.baseline,self.candidate,fault)
            with self.assertRaises((RuntimeError,program.private.boot.Refused)):
                program.execute(target,self.candidate,self.restore,self.baseline,True)
            self.assertEqual((target.erases,target.writes),(0,0),fault)
        target=Fake(self.baseline,self.candidate);target.flash[2*program.SIZE]^=1
        with self.assertRaises(program.private.boot.Refused):program.execute(target,self.candidate,self.restore,self.baseline,True)
        self.assertEqual((target.erases,target.writes),(0,0))
    def test_erase_or_erased_readback_failure_never_writes(self):
        for fault in ('erase-fail','erase-incomplete'):
            target=Fake(self.baseline,self.candidate,fault)
            with self.assertRaises(program.private.boot.Refused):program.execute(target,self.candidate,self.restore,self.baseline,True)
            self.assertEqual((target.erases,target.writes),(1,0));self.assertNotEqual(target.stage,'complete')
    def test_write_readback_collateral_failures_no_retry_or_success(self):
        for fault in ('write-fail','readback','collateral'):
            target=Fake(self.baseline,self.candidate,fault)
            with self.assertRaises(program.private.boot.Refused):program.execute(target,self.candidate,self.restore,self.baseline,True)
            self.assertEqual((target.erases,target.writes),(1,1));self.assertNotEqual(target.stage,'complete')
    def test_dispatch_permission_consumed_before_TX(self):
        target=program.Transaction.__new__(program.Transaction);target.stage='verify'
        with self.assertRaises(program.private.boot.Refused):target.command(program.ERASE)
        target.stage='erase-admitted'
        with patch.object(program.private.boot.Boot,'command',side_effect=RuntimeError('lost response')):
            with self.assertRaises(RuntimeError):target.command(program.ERASE)
        self.assertEqual(target.stage,'erase-in-flight')
        with self.assertRaises(program.private.boot.Refused):target.command(program.ERASE)
        with self.assertRaises(program.private.boot.Refused):target.command(program.WRITE)
    def test_no_arbitrary_write_or_boot_command(self):
        target=program.Transaction.__new__(program.Transaction);target.stage='verify'
        for command in ('sf erase 0 0x100000','sf write 0x82608000 0 0x100000','saveenv',
                        'bootm 0x81808000','mw.b 0x82608000 0 1'):
            with self.assertRaises(program.private.boot.Refused):target.command(command)
    def test_wrong_baseline_restore_refuses_before_any_command(self):
        target=Fake(self.baseline,self.candidate)
        with self.assertRaises(program.private.boot.Refused):program.execute(target,self.candidate,b'R'*program.SIZE,self.baseline,True)
        self.assertFalse(target.commands)

    def test_contradictory_SF_success_never_reaches_erase(self):
        class Mixed(Fake):
            def command(self,command,**kwargs):
                result=super().command(command,**kwargs)
                return result.replace(b'anyka$',b'short read\r\nanyka$') if command.startswith('sf read ') else result
        target=Mixed(self.baseline,self.candidate)
        with self.assertRaises(program.private.boot.Refused):program.execute(target,self.candidate,self.restore,self.baseline,True)
        self.assertEqual((target.erases,target.writes),(0,0))

    def test_matching_CRC_with_error_never_reaches_erase(self):
        class Mixed(Fake):
            def command(self,command,**kwargs):
                result=super().command(command,**kwargs)
                return result.replace(b'anyka$',b'ERROR: CRC command failed\r\nanyka$') if command.startswith('crc32 ') else result
        target=Mixed(self.baseline,self.candidate)
        with self.assertRaises(program.private.boot.Refused):program.execute(target,self.candidate,self.restore,self.baseline,True)
        self.assertEqual((target.erases,target.writes),(0,0))


if __name__=='__main__':unittest.main()
