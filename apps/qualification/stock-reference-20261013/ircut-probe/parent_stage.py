#!/usr/bin/env python3
"""Parent-only protected UART staging and inert IR-Cut GO STATUS/calibration.

NO pulse selector, GPIO setup, firmware/ENV/SD write or Linux boot is exposed.
Expanded code range is fixed2176B at82001000, within the reviewed DRAM page.
"""
from pathlib import Path
import sys
import json
import struct
import hashlib
import fcntl
import re

ROOT=Path(__file__).resolve().parents[4]
sys.path.insert(0,str(ROOT/'scripts'))
import qc07f_private_root_transfer as private

BASE=0x82001000
SIZE=2176
TEXT=0x83c
ARTIFACT=ROOT/'evidence/delegated/20261016-ircut/uboot-probe01/run05/ircut-uboot.bin'
SHA='eecf1e6c76759e4f11a40da9a56125c23e8cda6cb7a1f4868f929568fb78b5ce'

def crc_ok(response,address,length,expected):
    rows=[line.strip() for line in response.replace(b'\r',b'').splitlines() if line.strip()]
    return (len(rows)==2 and rows[-1]==b'anyka$' and
            private.boot.crc_verified(response,address,length,expected))

class InertProbe(private.PrivateBoot):
    def command(self,command,**kwargs):
        extra={'bdinfo',f'crc32 {BASE:#x} {SIZE:#x}',f'crc32 {BASE:#x} {TEXT:#x}',
               f'go {BASE:#x} STATUS',f'go {BASE:#x} CALIBRATE_ONLY'}
        word=re.fullmatch(r'mw\.l (0x[0-9a-f]+) (0x[0-9a-f]+) 1',command)
        if word:
            address,value=(int(x,16) for x in word.groups())
            if BASE<=address<BASE+SIZE and (address-BASE)%4==0 and value<=0xffffffff:
                return private.boot.Boot.command(self,command,**kwargs)
        if command in extra:return private.boot.Boot.command(self,command,**kwargs)
        return super().command(command,**kwargs)

def main():
    parser=private.PrivateParser(description=__doc__,allow_abbrev=False)
    parser.add_argument('action',choices=['stage-status','calibrate-only'])
    parser.add_argument('--capture-dir',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    data=ARTIFACT.read_bytes()
    if len(data)!=SIZE or hashlib.sha256(data).hexdigest()!=SHA:raise private.boot.Refused('Public code pin changed')
    with (private.boot.CONTROL/'peripheral-gui-operation.lock').open('a+') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX|fcntl.LOCK_NB);lease.seek(0)
        if lease.read(1):raise private.boot.Refused('Unretired operation HOLD')
        b=InertProbe(args.output,args.capture_dir,vendor_simple_parser=True)
        try:
            b.caught(5);b.owner(margin=600)
            if args.action=='stage-status':
                b.pin_bootloader()
                info=b.command('bdinfo')
                numbers={int(x,16) for x in re.findall(rb'0x[0-9a-fA-F]+',info)}
                if b"Unknown command 'bdinfo'" in info:
                    # Measured retail build omits bdinfo. This fixed page lies
                    # inside the already native-tested RAM/GO range, on the same
                    # pinned BOOT/64MiB target caught above; no guessed bank.
                    b.log('retail_bdinfo_absent_fixed_RAM_admission',
                          DRAM_banner_64MiB_checked=True,BOOT_CRC_checked=True,
                          staging_page=[0x82001000,0x82002000],bytes=SIZE,
                          prior_native_GO_loader_same_page=True)
                elif 0x80000000 not in numbers or 0x04000000 not in numbers:
                    raise private.boot.Refused('Unexpected DRAM bank; no code staged')
                for offset in range(0,SIZE,4):
                    value=struct.unpack_from('<I',data,offset)[0]
                    b.command(f'mw.l {BASE+offset:#x} {value:#010x} 1')
                response=b.command(f'crc32 {BASE:#x} {SIZE:#x}')
                if not crc_ok(response,BASE,SIZE,f'{private.boot.crc_bzip2(data):08x}'):
                    raise private.boot.Refused('Staged code CRC mismatch')
            else:
                response=b.command(f'crc32 {BASE:#x} {TEXT:#x}')
                if not crc_ok(response,BASE,TEXT,f'{private.boot.crc_bzip2(data[:TEXT]):08x}'):
                    raise private.boot.Refused('Immutable staged code mismatch')
            response=b.command(f'go {BASE:#x} STATUS',timeout=15)
            rows=re.findall(rb'(?m)^IC1 STATUS ([^\r\n]+)\r?$',response)
            if len(rows)!=1 or b'Application terminated, rc = 0x0' not in response:
                raise private.boot.Refused('No complete inert STATUS/return')
            values=dict(re.findall(rb'([A-Z0-9]+)=([0-9a-fA-F]+)',rows[0]))
            for field in (b'74',b'78',b'DIR',b'OUT',b'IN',b'USED',b'CAL',b'CTL'):
                if field not in values:raise private.boot.Refused('Missing status field')
            values={k.decode():int(v,16) for k,v in values.items()}
            idle=(values['74']&1)==0 and (values['78']&0xf0)==0x50 and (values['DIR']&0xc1)==0xc0 and (values['OUT']&0xc0)==0
            b.log('own_probe_inert_STATUS_returned',public_code_sha256=SHA,bytes=SIZE,
                  reviewed_RAM_range=[BASE,BASE+SIZE],parsed=values,pair_idle_admission=idle,
                  no_GPIO_writes=True,calibration_attempted=False)
            if args.action=='calibrate-only':
                if not idle or values['USED']!=0:raise private.boot.Refused('Current pin state not admitted; no setup or pulse')
                response=b.command(f'go {BASE:#x} CALIBRATE_ONLY',timeout=15)
                results=re.findall(rb'(?m)^IC1 RESULT ([^\r\n]+)\r?$',response)
                if len(results)!=1 or b'Application terminated, rc = 0x0' not in response or not re.search(rb'\brc=0\b',results[0]):
                    raise private.boot.Refused('Inert calibration refused; no pulse')
                b.log('inert_calibration_returned',response=response.decode('ascii','backslashreplace'),no_GPIO_writes=True)
            print('Own IR-Cut RAM probe inert status verified; no pulse' if args.action=='stage-status' else
                  'Own IR-Cut inert calibration returned; no pulse')
        except BaseException:
            b.log('HOLD_failed',no_automatic_continuation=True,no_pulse_entry_in_this_program=True)
            raise

if __name__=='__main__':
    try:main()
    except (Exception,KeyboardInterrupt):
        print('IR-Cut inert probe HOLD; inspect private evidence; no pulse',file=sys.stderr)
        raise SystemExit(1)
