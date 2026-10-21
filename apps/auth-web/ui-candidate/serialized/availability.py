#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Preserve the exact491 availability campaign; only candidate snapshot path changes."""
from pathlib import Path
import hashlib,json,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;BASE=HERE.parent;ROOT=BASE.parents[2]
source=BASE/'availability.py';reviewed=json.loads((ROOT/'evidence/delegated/20261012-native-ui/handoff01/REPORT.json').read_text())
if hashlib.sha256(source.read_bytes()).hexdigest()!=reviewed['source_scripts']['availability.py']:raise RuntimeError('v1 availability wrapper drift')
text=source.read_text().replace("HERE/'snapshot/", "HERE/'serialized/snapshot/")
exec(compile(text,str(source),'exec'),{'__name__':'__main__','__file__':str(source)})
