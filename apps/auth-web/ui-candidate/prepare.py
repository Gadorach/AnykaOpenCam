#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Pin public canonical inputs and v3 assets; generate only an isolated UI fork."""
from pathlib import Path
import datetime,difflib,hashlib,json,shutil,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SERVICE=ROOT/'apps/auth-web/service'
PLAYER=ROOT/'apps/web-media/player'
SNAP=HERE/'snapshot'
ASSETS=('index.html','app.mjs','core.mjs','contract.mjs','session.mjs','wire.mjs',
        'sample.mjs','player.mjs','frame-proof.mjs','player.css')
CSP="default-src 'none'; script-src 'self'; style-src 'self'; connect-src 'self'; media-src blob:; form-action 'self'; base-uri 'none'; frame-ancestors 'none'"
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def public_files(path):
 for p in sorted(path.rglob('*')):
  if '__pycache__' in p.parts or 'vendor' in p.relative_to(path).parts:continue
  if p.is_symlink():raise RuntimeError('refuse symlink '+str(p))
  if p.is_file():yield p
def mime(name):
 return 'text/html; charset=utf-8' if name.endswith('.html') else 'text/css; charset=utf-8' if name.endswith('.css') else 'text/javascript; charset=utf-8'
def headers(content_type):
 return (f'Content-Type: {content_type}\r\nCache-Control: no-store\r\nConnection: close\r\n'
  'X-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\n'
  f'Content-Security-Policy: {CSP}\r\n')
def main():
 SNAP.mkdir() # Refuse overwriting any previous inputs or generated source.
 canonical=SNAP/'canonical-service';canonical.mkdir()
 records={}
 for p in public_files(SERVICE):
  rel=p.relative_to(SERVICE);dest=canonical/rel;dest.parent.mkdir(parents=True,exist_ok=True)
  shutil.copyfile(p,dest);records[str(rel)]={'sha256':digest(p),'bytes':p.stat().st_size}
 scripts=SNAP/'canonical-support';scripts.mkdir()
 for name in ['build_support.py','dependency-pins.json']:
  p=SERVICE.parent/name;shutil.copyfile(p,scripts/name)
  records['../'+name]={'sha256':digest(p),'bytes':p.stat().st_size}
 licences=SNAP/'licenses';licences.mkdir()
 for vendor in ['mbedtls','mongoose']:
  shutil.copyfile(SERVICE/'vendor'/vendor/'LICENSE',licences/(vendor+'-LICENSE'))
 old=(canonical/'web_service.c').read_text()
 if digest(canonical/'web_service.c')!='dde8aa80dae8552de882ce696b31ea00109bc22892dc4b17c053ddb0dec12f31':
  raise RuntimeError('canonical service differs from reviewed v3 contract')
 receipt=ROOT/'evidence/delegated/20261012-browser-player/v3-review/node-78-validation.json'
 tested=json.loads(receipt.read_text());shutil.copyfile(receipt,SNAP/'player-v3-validation.json')
 if tested['pass']!=78 or tested['fail']!=0:raise RuntimeError('v3 prerequisite not passed')
 assets=SNAP/'assets';assets.mkdir();generated=SNAP/'generated';generated.mkdir()
 table=['/* Generated fixed local UI table; asset licence not inferred from C licence. */',
  '#include <stddef.h>',
  'struct ui_asset {const char *route,*type;const unsigned char *data;size_t bytes;};']
 asset_records=[]
 for i,name in enumerate(ASSETS):
  p=PLAYER/name;data=p.read_bytes();pin=tested['source'][name]
  if digest(p)!=pin['sha256'] or len(data)>7168 or b'\0' in data:raise RuntimeError('asset pin/size/NUL: '+name)
  shutil.copyfile(p,assets/name)
  table.append('/* '+name+' SHA256 '+pin['sha256']+' */')
  table.append('static const unsigned char ui_data_'+str(i)+'[]={')
  for start in range(0,len(data),24):table.append(','.join('0x%02x'%b for b in data[start:start+24])+',')
  table+=['0};','_Static_assert(sizeof(ui_data_'+str(i)+')-1<=7168U,"UI body budget");']
  wire_bytes=len(data)+len(headers(mime(name)).encode())+len(b'HTTP/1.1 200 OK\r\nContent-Length:            \r\n\r\n')
  if wire_bytes>8192 or len(headers(mime(name)).encode())>=512:raise RuntimeError('full reply bound '+name)
  asset_records.append({'name':name,'sha256':pin['sha256'],'bytes':len(data),'mime':mime(name),'full_wire_bytes':wire_bytes})
 table.append('static const struct ui_asset ui_assets[]={')
 for i,name in enumerate(ASSETS):
  routes=['/','/index.html'] if name=='index.html' else ['/'+name]
  for route in routes:table.append('{'+json.dumps(route)+','+json.dumps(mime(name))+',ui_data_'+str(i)+',sizeof(ui_data_'+str(i)+')-1},')
 table.append('};\n');(generated/'ui_assets.h').write_text('\n'.join(table))
 start=old.index(' if(equal(h->method,"GET")&&(equal(h->uri,"/")||equal(h->uri,"/app.js"))){')
 end=old.index(' if(equal(h->method,"GET")&&equal(h->uri,"/api/session")){',start)
 replacement=(HERE/'static_dispatch.inc').read_text()
 new=old[:start]+replacement+old[end:]
 if new.count('#include "web_assets.h"')!=1:raise RuntimeError('include boundary changed')
 new=new.replace('#include "web_assets.h"','#include "ui_assets.h"')
 # Reversal is byte-exact: the sole service deltas are include + public static branch.
 restored=new.replace('#include "ui_assets.h"','#include "web_assets.h"').replace(replacement,old[start:end],1)
 if restored!=old:raise RuntimeError('non-static service drift')
 (generated/'web_service.c').write_text(new)
 (generated/'canonical-to-ui.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='canonical/web_service.c',tofile='candidate/web_service.c')))
 metadata={'created':datetime.datetime.now(datetime.timezone.utc).isoformat(),'canonical_source':str(SERVICE),
  'canonical_inputs':records,'player_receipt_sha256':digest(receipt),'assets':asset_records,'routes':11,
  'csp':CSP,'response_budget':8192,'body_budget':7168,'private_ABI':2,'service_non_static_byte_identical':True,
  'generated':{n:digest(generated/n) for n in ['ui_assets.h','web_service.c','canonical-to-ui.patch']},
  'asset_licence':'Original player has no redistribution grant; local candidate only. No workspace-wide licence inferred.',
  'canonical_licence':'Original GPL-2.0-only notices preserved; vendor/runtime licences stay authoritative/read-only.',
  'licence_hashes':{p.name:digest(p) for p in licences.iterdir()},
  'media':'501 unsupported_unqualified','native_qualified':False}
 (SNAP/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
 print(json.dumps({'snapshot':str(SNAP),'assets':len(ASSETS),'max_full_wire_bytes':max(x['full_wire_bytes'] for x in asset_records)}))
if __name__=='__main__':main()
