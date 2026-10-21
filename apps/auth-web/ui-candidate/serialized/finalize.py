#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Pinned v1 collector overlay for new snapshot/counts, plus loader evidence."""
from pathlib import Path
import hashlib,json,shutil,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;BASE=HERE.parent;ROOT=BASE.parents[2]
source=BASE/'finalize.py';reviewed=json.loads((ROOT/'evidence/delegated/20261012-native-ui/handoff01/REPORT.json').read_text())
if hashlib.sha256(source.read_bytes()).hexdigest()!=reviewed['source_scripts']['finalize.py']:raise RuntimeError('v1 collector drift')
text=source.read_text().replace("HERE/'snapshot/", "HERE/'serialized/snapshot/")
text=text.replace("out=E/'handoff01';out.mkdir()", "out=E/'serialized02/handoff02';out.mkdir(parents=True)")
text=text.replace("assets['checks']!=127", "assets['checks']!=134").replace("'assets':127", "'assets':134").replace('assets127','assets134')
old=" if digest(ROOT/'apps/web-media/player'/row['name'])!=row['sha256']:raise RuntimeError('player source drift')"
new=" if row['name']=='bootstrap.js':\n  original=HERE/'serialized/bootstrap.js';expected=row['sha256']\n else:\n  original=ROOT/'apps/web-media/player'/row['name'];expected=snapshot['serialization']['original_HTML_sha256'] if row['name']=='index.html' else row['sha256']\n if digest(original)!=expected:raise RuntimeError('player/bootstrap source drift')"
if text.count(old)!=1:raise RuntimeError('collector provenance marker drift')
text=text.replace(old,new)
text=text.replace("p in HERE.iterdir()", "p in (HERE/'serialized').iterdir()")
scope={'__name__':'__main__','__file__':str(source)}
exec(compile(text,str(source),'exec'),scope)
out=ROOT/'evidence/delegated/20261012-native-ui/serialized02/handoff02'
model=ROOT/'evidence/delegated/20261012-native-ui/serialized02/model01/manifest.json'
receipt=json.loads(model.read_text())
if receipt['pass']!=19 or receipt['fail']!=0:raise RuntimeError('loader19 model prerequisite failed')
shutil.copyfile(model,out/'loader19-model.json')
shutil.copyfile(model.parent/'tests.tap',out/'loader19-tests.tap')
report=out/'REPORT.json';result=json.loads(report.read_text())
result['serialization']={'CSS_first':True,'fixed_topological_order':True,'v3_JS_CSS_byte_exact':True,
 'loader_max_inflight_model':1,'per_step_deadline_ms':8000,'login_disabled_until_app_load':True,
 'no_eval_inline_blob_script_serviceworker_or_persistent_cache':True,'model_tests':19,
 'actual_browser':'BLOCKED: browser.disconnected, own HTTP fixture closed; no parent tab accessed',
 'TLS_browser':'Not attempted: no explicitly scoped legitimate browser trust context; no warning bypass/global trust changes',
 'API_semantics':'Untouched; existing v3 status/capabilities Promise.all is not serialized by this loader'}
result['browser_parallel_module_graph']='Serialized one-at-a-time by fixed bootstrap;19 model tests passed; real browser/TLS/native proof remains open'
report.write_text(json.dumps(result,indent=2)+'\n')
