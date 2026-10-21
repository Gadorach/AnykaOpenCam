#!/usr/bin/env python3
"""Fixed parent ROOT transaction. Default VERIFY ONLY; no boot or auto rollback.

Only --program-root-only permits the two exact destructive vendor commands, after
all buffer/baseline checks. Every command/response goes to the protected envelope.
"""
import fcntl
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[4]
sys.path.insert(0,str(ROOT/'scripts'))
import qc07f_private_root_transfer as private
from parent_uboot_readback import same_bytes, sf_record, strict_crc

SIZE=0x100000
VERIFY=0x82708000
ERASE='sf erase 0x200000 0x100000'
WRITE='sf write 0x82408000 0x200000 0x100000'
READ_ROOT='sf read 0x82708000 0x200000 0x100000'
CMP_RESTORE='cmp.b 0x82608000 0x82708000 0x100000'
CMP_CANDIDATE='cmp.b 0x82408000 0x82708000 0x100000'
CRC_VERIFY='crc32 0x82708000 0x100000'
CRC_RESTORE='crc32 0x82608000 0x100000'


def sf_success(response,offset,verb):
    return sf_record(response,offset,verb)


class Transaction(private.PrivateBoot):
    def __init__(self,*args,**kwargs):
        self.stage='verify'
        super().__init__(*args,**kwargs)

    def command(self,command,**kwargs):
        if command in (ERASE,WRITE):
            required='erase-admitted' if command==ERASE else 'write-admitted'
            if self.stage!=required:
                raise private.boot.Refused('Destructive phase not admitted')
            # Consume permission BEFORE transmission: never replay uncertainty.
            self.stage='erase-in-flight' if command==ERASE else 'write-in-flight'
            return private.boot.Boot.command(self,command,**kwargs)
        reads={f'sf read {VERIFY:#x} {n*SIZE:#x} {SIZE:#x}' for n in range(8)}
        if command in reads|{CMP_RESTORE,CMP_CANDIDATE,CRC_VERIFY,CRC_RESTORE}:
            return private.boot.Boot.command(self,command,**kwargs)
        return super().command(command,**kwargs)


def require(condition,message):
    if not condition:raise private.boot.Refused(message)


def check_crc(instance,address,expected):
    response=instance.command(f'crc32 {address:#x} {SIZE:#x}')
    require(strict_crc(response,address,SIZE,expected),'Private RAM/ROM CRC mismatch')


def read_chunk(instance,index):
    offset=index*SIZE
    response=instance.command(f'sf read {VERIFY:#x} {offset:#x} {SIZE:#x}',timeout=45)
    require(sf_success(response,offset,'Read'),'No fresh exact-range read success')


def nonroot_equal(instance,baseline):
    for index in (0,1,3,4,5,6,7):
        read_chunk(instance,index)
        expected=f'{private.boot.crc_bzip2(baseline[index*SIZE:(index+1)*SIZE]):08x}'
        check_crc(instance,VERIFY,expected)


def execute(instance,candidate,restore,baseline,program=False):
    require(len(candidate)==len(restore)==SIZE and len(baseline)==8*SIZE,'Wrong image extent')
    require(baseline[2*SIZE:3*SIZE]==restore,'Restore image is not fresh baseline ROOT')
    instance.caught(5)
    instance.owner(margin=900)
    instance.sf_probe()
    check_crc(instance,0x82408000,f'{private.boot.crc_bzip2(candidate):08x}')
    check_crc(instance,0x82608000,f'{private.boot.crc_bzip2(restore):08x}')
    nonroot_equal(instance,baseline)
    read_chunk(instance,2)
    require(same_bytes(instance.command(CMP_RESTORE,timeout=45),SIZE),'Current ROOT changed since restore proof')
    instance.log('prewrite_buffers_and_full_flash_baseline_verified',program_requested=program,
                 restore_compared_bytes=SIZE,nonroot_chunks=7)
    if not program:return False
    # Recheck immediately before any destructive CR, retaining reset/catch time.
    instance.owner(margin=900)
    instance.check_stream()
    instance.stage='erase-admitted'
    require(sf_success(instance.command(ERASE,timeout=120),2*SIZE,'Erased'),'Erase did not report full success')
    read_chunk(instance,2)
    check_crc(instance,VERIFY,f'{private.boot.crc_bzip2(b"\xff"*SIZE):08x}')
    instance.stage='write-admitted'
    require(sf_success(instance.command(WRITE,timeout=120),2*SIZE,'Written'),'Write did not report full success')
    read_chunk(instance,2)
    require(same_bytes(instance.command(CMP_CANDIDATE,timeout=45),SIZE),'Full programmed ROOT readback differs')
    check_crc(instance,VERIFY,f'{private.boot.crc_bzip2(candidate):08x}')
    nonroot_equal(instance,baseline)
    instance.stage='complete'
    instance.log('ROOT_program_and_full_compare_verified',physical_range=[2*SIZE,3*SIZE],
                 bytes_compared=SIZE,nonroot_chunks_unchanged=7,boot_not_attempted=True,
                 automatic_rollback=False)
    return True


def main():
    parser=private.PrivateParser(description=__doc__,allow_abbrev=False)
    for field in ('capture-dir','output','candidate','candidate-ledger','restore','restore-ledger',
                  'baseline','baseline-ledger'):
        parser.add_argument('--'+field,type=Path,required=True)
    parser.add_argument('--program-root-only',action='store_true')
    args=parser.parse_args()
    candidate,_=private.private_payload(args.candidate,args.candidate_ledger)
    restore,_=private.private_payload(args.restore,args.restore_ledger)
    baseline=private.read_private(args.baseline,(8*SIZE,))
    pin=private.read_private(args.baseline_ledger,(64,65))
    require(re.fullmatch(b'[0-9a-f]{64}\n?',pin) is not None,'Malformed private baseline ledger')
    require(private.hashlib.sha256(baseline).hexdigest().encode()==pin.rstrip(b'\n'),'Fresh baseline identity mismatch')
    with (private.boot.CONTROL/'peripheral-gui-operation.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX|fcntl.LOCK_NB)
        instance=Transaction(args.output,args.capture_dir,vendor_simple_parser=True)
        try:written=execute(instance,candidate,restore,baseline,args.program_root_only)
        except BaseException:
            instance.log('HOLD_failed',no_automatic_continuation=True,stage=instance.stage)
            raise
    print('ROOT programmed and compared; boot not attempted' if written else
          'ROOT prewrite verification passed; no flash write')


if __name__=='__main__':
    try:main()
    except (Exception,KeyboardInterrupt):
        print('Parent ROOT operation HOLD; inspect protected evidence; no automatic continuation',file=sys.stderr)
        raise SystemExit(1)
