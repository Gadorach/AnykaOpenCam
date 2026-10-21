#!/usr/bin/env python3
"""Explicit neutral-only GPIO6/7 setup/restore in held original U-Boot.

This measured cold-boot context starts SYS78/DIR0/OUT0=0 and timer3/4 disabled.
Only DIR0 pair bits and SYS78 pair fields are changed; no HIGH latch write,
GPIO0/UART/pull/clock/timer change, pulse or persistent programming exists here.
"""
from pathlib import Path
import sys
import json
import os
import re
import fcntl
ROOT=Path(__file__).resolve().parents[4]
sys.path.insert(0,str(ROOT/'scripts'))
import qc07f_private_root_transfer as private

BASE=Path('/home/gadorach/.local/state/anyka-bench-ssh/ircut-uboot-20261016T0515Z')
REGS={'s74':0x08000074,'s78':0x08000078,'dir':0x20170000,'out':0x2017000c,
      'in':0x20170018,'p0':0x08000080,'p67':0x08000084,'t3':0x080000c8,'t4':0x080000d0,
      'pll0':0x08000000,'pll4':0x08000004,'pll8':0x08000008}
SET_DIR='mw.l 0x20170000 0xc0 1'
SET_MUX='mw.l 0x8000078 0x50 1'
RESTORE_MUX='mw.l 0x8000078 0x0 1'
RESTORE_DIR='mw.l 0x20170000 0x0 1'

class Neutral(private.PrivateBoot):
    def command(self,command,**kwargs):
        reads={f'md.l {address:#x} 1' for address in REGS.values()}
        if command in reads|{SET_DIR,SET_MUX,RESTORE_MUX,RESTORE_DIR,'go 0x82001000 STATUS','crc32 0x82001000 0x83c'}:
            return private.boot.Boot.command(self,command,**kwargs)
        return super().command(command,**kwargs)

def read_all(b):
    result={}
    for name,address in REGS.items():
        response=b.command(f'md.l {address:#x} 1')
        rows=re.findall(rb'(?m)^0?'+f'{address:x}'.encode()+rb':\s+([0-9a-fA-F]{8})',response)
        if len(rows)!=1 or re.search(rb'(?i)error|failed|unknown command',response):
            raise private.boot.Refused('Register read mismatch')
        result[name]=int(rows[0],16)
    status=b.command('go 0x82001000 STATUS')
    ctl=re.findall(rb'\bCTL=([0-9a-fA-F]{8})\b',status)
    if len(ctl)!=1 or b'Application terminated, rc = 0x0' not in status:
        raise private.boot.Refused('No immutable inert probe/control return')
    result['ctl']=int(ctl[0],16)
    if result['ctl']&5:raise private.boot.Refused('MMU/D-cache state not admitted')
    return result

def require(condition):
    if not condition:raise private.boot.Refused('Neutral setup state/refusal; no pulse')

def main():
    os.umask(0o077)
    parser=private.PrivateParser(description=__doc__,allow_abbrev=False)
    parser.add_argument('action',choices=['setup','restore'])
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();lease_path=BASE/'PAIR_NEUTRAL_LEASE.private.json'
    with (private.boot.CONTROL/'peripheral-gui-operation.lock').open('a+') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB);lock.seek(0);require(not lock.read(1))
        b=Neutral(args.output,BASE/'uart-capture16b',vendor_simple_parser=True)
        try:
            b.caught(5);b.owner(margin=600)
            from parent_stage import ARTIFACT,SHA,TEXT,BASE as CODE,crc_ok
            import hashlib
            data=ARTIFACT.read_bytes();require(hashlib.sha256(data).hexdigest()==SHA)
            code_crc=b.command('crc32 0x82001000 0x83c')
            require(crc_ok(code_crc,CODE,TEXT,f'{private.boot.crc_bzip2(data[:TEXT]):08x}'))
            now=read_all(b)
            require(now['s74']&1==0 and now['dir']&1==0 and now['out']==0 and now['in']&0xc0==0 and now['t3']==now['t4']==0)
            if args.action=='setup':
                require(not lease_path.exists() and now['s78']==0 and now['dir']==0)
                # Intent persisted first. Unexpected partial states are inspected,
                # never automatically retried or opportunistically repaired.
                with lease_path.open('x') as f:json.dump({'original':now,'intent':'TWO_NEUTRAL_PAIR_SETUP_STORES_ONLY'},f);f.write('\n')
                b.command(SET_DIR);check=read_all(b)
                require(all(check[k]==now[k] for k in now if k not in ('dir','in')) and check['dir']==0xc0 and check['in']&0xc0==0)
                b.command(SET_MUX);after=read_all(b)
                require(all(after[k]==now[k] for k in now if k not in ('dir','s78','in')) and after['dir']==0xc0 and after['s78']==0x50 and after['in']&0xc0==0)
                b.log('explicit_neutral_GPIO6_7_setup_verified',before=now,after=after,no_HIGH_store=True,no_pulse=True)
            else:
                original=json.loads(private.read_private(lease_path,tuple(range(1,4097))).decode())['original']
                require(now['s78']==0x50 and now['dir']==0xc0 and original['s78']==original['dir']==original['out']==0)
                require(all(now[k]==original[k] for k in now if k not in ('dir','s78','in')))
                b.command(RESTORE_MUX);check=read_all(b)
                require(check['s78']==0 and check['dir']==0xc0 and check['out']==0 and check['in']&0xc0==0 and check['t3']==check['t4']==0)
                b.command(RESTORE_DIR);after=read_all(b)
                require(all(after[k]==original[k] for k in after if k!='in') and after['in']&0xc0==0)
                b.log('original_GPIO6_7_configuration_restored',original=original,after=after,no_HIGH_store=True)
        except BaseException:
            b.log('HOLD_failed',no_pulse_entry=True,no_automatic_repair_or_retry=True)
            raise
    print('GPIO6/7 neutral-only setup verified; no pulse' if args.action=='setup' else
          'GPIO6/7 original neutral configuration restored')

if __name__=='__main__':
    try:main()
    except (Exception,KeyboardInterrupt):
        print('IR-Cut neutral setup HOLD; no pulse; inspect protected evidence',file=sys.stderr)
        raise SystemExit(1)
