#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Controlled v1 recipe overlay: derived HTML/bootstrap, byte-exact v3 modules."""
from pathlib import Path
import hashlib,json,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;BASE=HERE.parent;ROOT=BASE.parents[2]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def serialized_html(data):
 text=data.decode('utf-8')
 old='<title>Camera browser candidate</title><link rel="stylesheet" href="./player.css">\n<script type="module" src="./app.mjs"></script>'
 new='<title>Camera browser candidate</title><script defer src="./bootstrap.js"></script>'
 if text.count(old)!=1:raise RuntimeError('HTML head drift')
 text=text.replace(old,new)
 if text.count('<form id="login">')!=1 or text.count('</button></form>')!=1:raise RuntimeError('HTML login drift')
 text=text.replace('<form id="login">','<p id="bootstrap-status" role="status" aria-live="polite">Loading local UI; login disabled.</p>\n<form id="login"><fieldset id="bootstrap-gate" disabled>')
 text=text.replace('</button></form>','</button></fieldset></form>')
 return text.encode('utf-8')
def main():
 receipt=ROOT/'evidence/delegated/20261012-native-ui/handoff01/REPORT.json'
 reviewed=json.loads(receipt.read_text());source=BASE/'prepare.py'
 if sha(source)!=reviewed['source_scripts']['prepare.py']:raise RuntimeError('v1 preparation recipe drift')
 text=source.read_text()
 replacements={"SNAP=HERE/'snapshot'":"SNAP=HERE/'serialized/snapshot'",
  "'sample.mjs','player.mjs','frame-proof.mjs','player.css')":"'sample.mjs','player.mjs','frame-proof.mjs','player.css','bootstrap.js')",
  "  p=PLAYER/name;data=p.read_bytes();pin=tested['source'][name]\n  if digest(p)!=pin['sha256'] or len(data)>7168 or b'\\0' in data:raise RuntimeError('asset pin/size/NUL: '+name)\n  shutil.copyfile(p,assets/name)":
  "  p=HERE/'serialized/bootstrap.js' if name=='bootstrap.js' else PLAYER/name\n  data=p.read_bytes()\n  if name!='bootstrap.js' and digest(p)!=tested['source'][name]['sha256']:raise RuntimeError('v3 input drift '+name)\n  if name=='index.html':data=serialized_html(data)\n  pin={'sha256':hashlib.sha256(data).hexdigest()}\n  if len(data)>7168 or b'\\0' in data:raise RuntimeError('asset size/NUL: '+name)\n  (assets/name).write_bytes(data)",
  "'routes':11":"'routes':12"}
 for old,new in replacements.items():
  if text.count(old)!=1:raise RuntimeError('recipe marker drift '+old[:50])
  text=text.replace(old,new)
 scope={'__name__':'__serialized_prepare__','__file__':str(source),'serialized_html':serialized_html}
 exec(compile(text,str(source),'exec'),scope);scope['main']()
 manifest=HERE/'snapshot/manifest.json';data=json.loads(manifest.read_text())
 data['serialization']={'css_first':'/player.css','topological_modules':['contract.mjs','session.mjs','wire.mjs','sample.mjs','player.mjs','core.mjs','frame-proof.mjs','app.mjs'],
  'login_gate':'disabled until module script load succeeds','max_loader_inflight':1,'step_deadline_ms':8000,
  'original_HTML_sha256':sha(BASE/'snapshot/assets/index.html'),'bootstrap_source_sha256':sha(HERE/'bootstrap.js'),
  'v3_modules_and_CSS_byte_exact':True,'API_fetch_implementation_unchanged':True}
 manifest.write_text(json.dumps(data,indent=2)+'\n')
if __name__=='__main__':main()
