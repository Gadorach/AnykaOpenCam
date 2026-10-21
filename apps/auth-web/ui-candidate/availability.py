#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run the byte-exact canonical491 campaign against this build, without protocol edits."""
from pathlib import Path
import sys,json
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];SERVICE=ROOT/'apps/auth-web/service'
sys.path.insert(0,str(HERE))
from build import owned_output
sys.path.insert(0,str(SERVICE.parent))
from build_support import digest
if len(sys.argv)!=3:raise ValueError('candidate build and fresh test output required')
sys.argv[1]=str(owned_output(sys.argv[1]));sys.argv[2]=str(owned_output(sys.argv[2]))
pins=json.loads((HERE/'snapshot/manifest.json').read_text())
script=HERE/'snapshot/canonical-service/host_https_test.py'
if digest(script)!=pins['canonical_inputs']['host_https_test.py']['sha256']:raise RuntimeError('campaign pin drift')
exec(compile(script.read_text(),str(script),'exec'),{'__name__':'__main__','__file__':str(SERVICE/'host_https_test.py')})
