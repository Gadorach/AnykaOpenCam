#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Same realTLS checks, plus bootstrap; use generated HTML hash instead of v3 HTML."""
from pathlib import Path
import hashlib,json,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;BASE=HERE.parent;ROOT=BASE.parents[2]
source=BASE/'asset_tls_test.py';reviewed=json.loads((ROOT/'evidence/delegated/20261012-native-ui/handoff01/REPORT.json').read_text())
if hashlib.sha256(source.read_bytes()).hexdigest()!=reviewed['source_scripts']['asset_tls_test.py']:raise RuntimeError('v1 asset test drift')
text=source.read_text().replace("HERE/'snapshot/", "HERE/'serialized/snapshot/")
text=text.replace('asset-source-v3-byte-identical','asset-source-pinned-byte-identical')
text=text.replace("asset(s,pins['assets'][-1],'/player.css')", "asset(s,next(row for row in pins['assets'] if row['name']=='player.css'),'/player.css')")
exec(compile(text,str(source),'exec'),{'__name__':'__main__','__file__':str(source)})
