/* SPDX-License-Identifier: GPL-2.0-only */
/* Local login/status shell, NOT a dashboard or fake media page. */
static const char web_html[]=
 "<!doctype html><html lang=en><meta charset=utf-8><meta name=viewport content='width=device-width'>"
 "<title>Camera protected status</title><h1>Protected camera status</h1>"
 "<p>Independent UI identity. RTSP credentials are not accepted here.</p>"
 "<form id=login><label>Account <input id=account autocomplete=username required maxlength=63></label> "
 "<label>Password <input id=password type=password autocomplete=current-password required></label> "
 "<button>Sign in</button></form><button id=logout hidden>Sign out</button>"
 "<p id=message>Checking HTTPS session…</p><pre id=status></pre>"
 "<p>Browser video is unsupported/unqualified; no live feed is simulated.</p>"
 "<script src=/app.js defer></script></html>";
static const char web_js[]=
 "'use strict';let csrf='';const el=id=>document.getElementById(id);"
 "async function api(path,options={}){const r=await fetch(path,{credentials:'same-origin',cache:'no-store',...options});"
 "const data=await r.json();if(!r.ok)throw new Error(data.error||'Request denied');return data;}"
 "async function refresh(){const s=await api('/api/session');csrf=s.csrf;el('login').hidden=s.authenticated;"
 "el('logout').hidden=!s.authenticated;el('message').textContent=s.authenticated?'Authenticated':'Sign in to read protected status';"
 "el('status').textContent=s.authenticated?JSON.stringify(await api('/api/status'),null,2):'';}"
 "el('login').addEventListener('submit',async e=>{e.preventDefault();const password=new TextEncoder().encode(el('password').value);"
 "el('password').value='';el('message').textContent='Verifying (target cost is about 25 seconds)…';"
 "try{const s=await api('/api/login',{method:'POST',headers:{'Content-Type':'application/octet-stream',"
 "'X-Account':el('account').value,'X-CSRF-Token':csrf},body:password});csrf=s.csrf;await refresh();}"
 "catch(error){el('message').textContent=error.message;try{await refresh();}catch(_) {}}finally{password.fill(0);}});"
 "el('logout').addEventListener('click',async()=>{try{await api('/api/logout',{method:'POST',headers:{'X-CSRF-Token':csrf},body:''});"
 "await refresh();}catch(error){el('message').textContent=error.message;}});"
 "refresh().catch(error=>{el('message').textContent=error.message;});";
