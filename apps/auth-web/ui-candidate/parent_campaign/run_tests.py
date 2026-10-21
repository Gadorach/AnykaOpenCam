#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh PUBLICDUMMY model receipts; no native entry imports or private-file enumeration."""
from pathlib import Path
import argparse,datetime,hashlib,json,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
p=argparse.ArgumentParser();p.add_argument('name');args=p.parse_args()
if not args.name.isalnum():raise ValueError('fresh simple evidence name required')
out=ROOT/'evidence/delegated/20261012-native-ui/parent-campaign'/args.name;out.mkdir()
command=[sys.executable,'-B','-m','unittest','discover','-s',str(HERE),'-p','test_*.py','-v']
start=datetime.datetime.now(datetime.timezone.utc).isoformat()
r=subprocess.run(command,capture_output=True,text=True,timeout=20,cwd=ROOT)
(out/'tests.txt').write_text(r.stdout+r.stderr)
import re
count=re.search(r'Ran (\d+) tests',r.stderr)
record={'start':start,'end':datetime.datetime.now(datetime.timezone.utc).isoformat(),'command':command,'exit':r.returncode,
 'tests':int(count[1]) if count else None,'python':sys.version,'python_executable_sha256':hashlib.sha256(Path(sys.executable).read_bytes()).hexdigest(),
 'source':{x.name:{'sha256':hashlib.sha256(x.read_bytes()).hexdigest(),'bytes':x.stat().st_size} for x in HERE.iterdir() if x.is_file()},
 'input_receipts':json.loads((out.parent/'input-pins.json').read_text()),'scope':'PUBLICDUMMY logical fake UART/owned-PC proof only',
 'actual_TLS_or_HARDWARE':False,'old_native01_TLS_requests':0,'G3_G8_product_passed':False}
(out/'manifest.json').write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps({'exit':r.returncode,'tests':record['tests'],'output':str(out)}));raise SystemExit(r.returncode)
