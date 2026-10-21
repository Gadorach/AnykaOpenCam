#!/usr/bin/env python3
"""Parent's fixed restore-transport proof. NO flash erase/program or Linux boot.

Requires the private uploader to have put the sealed restore ROOT at82408000.
Only verification RAM82608000 is modified here; logs remain protected outside Git.
"""
import argparse
from pathlib import Path
import re
import sys
import fcntl

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT/'scripts'))
import qc07f_private_root_transfer as private

SIZE = 0x100000
READ = 'sf read 0x82608000 0x200000 0x100000'
CMP = 'cmp.b 0x82408000 0x82608000 0x100000'
CRC = 'crc32 0x82608000 0x100000'
NEGATIVE = 'mw.b 0x82608000 0 1'


def response_lines(response):
    return [line.strip() for line in response.replace(b'\r',b'').splitlines() if line.strip()]


def sf_record(response,offset,verb):
    lines=response_lines(response)
    if len(lines)<2 or lines[-1]!=b'anyka$':return False
    # Exact retained retail erase progress grammar (including its unsigned
    # percentage bug), never arbitrary text before a successful summary.
    # Oct08 native evidence emits progress and "SF:" on the SAME CR-updated line.
    prefix=(rb'(?:Eraseing,[ \t]+[0-9]{1,20}%[ \t]+[0-9]{1,20}[ \t]+B/s[ \t\n]*)*'
            if verb=='Erased' else b'')
    body=b'\n'.join(lines[:-1])
    row=re.fullmatch(prefix+rb'SF:[ \t]+([0-9]+)[ \t]+bytes[ \t]+@[ \t]*(0x[0-9a-fA-F]+)[ \t]+'+
                     re.escape(verb.encode())+rb':[ \t]*OK',body)
    return bool(row and int(row[1])==SIZE and int(row[2],16)==offset)


def strict_crc(response,address,length,expected):
    lines=response_lines(response)
    return (len(lines)==2 and lines[-1]==b'anyka$' and lines[0].startswith(b'CRC32 for ') and
            private.boot.crc_verified(response,address,length,expected))


def same_bytes(response, count):
    lines=response_lines(response)
    if len(lines)!=(2 if count==SIZE else 3) or lines[-1]!=b'anyka$':return False
    rows = re.findall(rb'(?m)^Total of ([0-9]+) byte\(s\) were the same\r?$', response)
    if len(rows) != 1 or int(rows[0]) != count:
        return False
    if re.search(rb'(?i)error|failed|unknown command', response):
        return False
    return (b'!=' not in response) if count == SIZE else (b'!=' in response)


def full_read(response):
    return sf_record(response,0x200000,'Read')


class RestoreReadback(private.PrivateBoot):
    def command(self, command, **kwargs):
        if command in (READ, CMP, CRC, NEGATIVE):
            return private.boot.Boot.command(self, command, **kwargs)
        return super().command(command, **kwargs)


def main():
    parser=private.PrivateParser(description=__doc__,allow_abbrev=False)
    for name in ('capture-dir','output','restore','ledger'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    data,info=private.private_payload(args.restore,args.ledger)
    with (private.boot.CONTROL/'peripheral-gui-operation.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX|fcntl.LOCK_NB)
        b=RestoreReadback(args.output,args.capture_dir,vendor_simple_parser=True)
        try:
            b.caught(5)
            b.owner(margin=300)
            # Prove transferred RAM identity independently before comparing ROM.
            staged=b.command('crc32 0x82408000 0x100000')
            if not strict_crc(staged,0x82408000,SIZE,info['crc32_bzip2']):
                raise private.boot.Refused('Staged restore RAM identity failed')
            if not full_read(b.command(READ,timeout=30)):
                raise private.boot.Refused('No fresh full ROOT read')
            if not same_bytes(b.command(CMP,timeout=30),SIZE):
                raise private.boot.Refused('Fresh ROOT differs from transferred restore')
            b.command(NEGATIVE)
            if not same_bytes(b.command(CMP,timeout=30),0):
                raise private.boot.Refused('RAM compare negative control failed')
            # Restore verification scratch from FLASH READ, not a RAM copy.
            if not full_read(b.command(READ,timeout=30)):
                raise private.boot.Refused('Fresh restore read failed')
            if not same_bytes(b.command(CMP,timeout=30),SIZE):
                raise private.boot.Refused('Final full restore equality failed')
            if not strict_crc(b.command(CRC),0x82608000,SIZE,info['crc32_bzip2']):
                raise private.boot.Refused('Final restore-buffer CRC failed')
            b.log('restore_transport_proven_no_programming',bytes_compared=SIZE,
                  negative_compare_control=True,ROM_range=[0x200000,0x300000],
                  transferred_restore_RAM=[0x82408000,0x82508000],
                  verified_live_restore_RAM=[0x82608000,0x82708000],
                  Linux_ROOT_not_mounted=True,flash_write=False)
        except BaseException:
            b.log('HOLD_failed',no_automatic_continuation=True)
            raise
    print('Restore transport and full ROOT compare verified; no flash write')


if __name__=='__main__':
    try:main()
    except (Exception,KeyboardInterrupt):
        print('Private restore proof HOLD; inspect protected evidence',file=sys.stderr)
        raise SystemExit(1)
