#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Pinned v1 builder wrapper, consuming the serialized candidate snapshot only."""
from pathlib import Path
import hashlib,json,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;BASE=HERE.parent;ROOT=BASE.parents[2]
source=BASE/'build.py';reviewed=json.loads((ROOT/'evidence/delegated/20261012-native-ui/handoff01/REPORT.json').read_text())
if hashlib.sha256(source.read_bytes()).hexdigest()!=reviewed['source_scripts']['build.py']:raise RuntimeError('v1 builder drift')
text=source.read_text();old="SNAP=HERE/'snapshot'"
if text.count(old)!=1:raise RuntimeError('snapshot marker drift')
text=text.replace(old,"SNAP=HERE/'serialized/snapshot'")
exec(compile(text,str(source),'exec'),{'__name__':'__main__','__file__':str(source)})
