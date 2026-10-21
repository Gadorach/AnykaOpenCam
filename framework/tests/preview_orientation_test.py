#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Byte-exact host display-flip fixture; no sensor/ISP or camera access."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import subprocess

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve();out.mkdir(mode=0o700)
    width,height=8,6
    pixels=[bytes((x*29,y*41,(x+y)*17)) for y in range(height) for x in range(width)]
    source=out/'asymmetric.rgb';source.open('xb').write(b''.join(pixels))
    version=subprocess.run(['ffmpeg','-version'],capture_output=True,check=True).stdout.decode()
    report={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'fixture_only':True,'actual_camera_test':False,'ffmpeg_version':version,'input_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'cases':[]}
    for name,hflip,vflip in [('horizontal',True,False),('vertical',False,True),('both',True,True)]:
        filters=','.join((["vflip"] if vflip else [])+(["hflip"] if hflip else []))
        dest=out/(name+'.rgb')
        argv=['ffmpeg','-hide_banner','-loglevel','error','-f','rawvideo','-pixel_format','rgb24','-video_size',f'{width}x{height}','-i',str(source),'-frames:v','1','-vf',filters,'-pix_fmt','rgb24','-f','rawvideo','-n',str(dest)]
        with (out/(name+'.log')).open('xb') as log:
            subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,check=True)
        expected=b''.join(pixels[(height-1-y if vflip else y)*width+(width-1-x if hflip else x)] for y in range(height) for x in range(width))
        assert dest.read_bytes()==expected
        report['cases'].append({'name':name,'argv':argv,'bytes':len(expected),'sha256':hashlib.sha256(expected).hexdigest(),'byte_exact':True})
    report['all_passed']=True;(out/'manifest.json').open('x').write(json.dumps(report,indent=2))
    print('PASS independent horizontal/vertical/combined host flips; no camera orientation write')

if __name__=='__main__':
    main()
