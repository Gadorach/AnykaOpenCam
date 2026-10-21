#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Bounded complete public scalar parsing; no echo/substring or native-pass flags."""
import re
class ParseError(ValueError):pass
class Parser:
 def __init__(self):self.records={};self.last_elapsed=0;self.pending=bytearray()
 def feed(self,data):
  for byte in data:
   if byte!=10:
    self.pending.append(byte)
    if len(self.pending)>256:raise ParseError('line_bound')
    continue
   line=bytes(self.pending);self.pending.clear()
   if line.endswith(b'\r'):line=line[:-1]
   if line.startswith(b'# '):line=line[2:]
   try:text=line.decode('ascii')
   except UnicodeDecodeError:continue
   patterns=[('ready',r'GUARDQUAL_HELPER_READY version=1 lifetime_ns=7000000000 start_ns=([0-9]+)'),
    ('lifetime',r'GUARDQUAL_HELPER_LIFETIME version=1 elapsed_ns=([0-9]+)'),
    ('cancel',r'GUARDQUAL_HELPER_CANCEL version=1 signal=(2|15) elapsed_ns=([0-9]+)'),
    ('exit',r'GUARDQUAL_HELPER_EXIT version=1 status=(0|20) elapsed_ns=([0-9]+)'),
    ('error',r'GUARDQUAL_HELPER_ERROR version=1 class=(arguments|FD3_not_closed|hardening|clock_or_signal|clock)')]
   for key,pattern in patterns:
    match=re.fullmatch(pattern,text)
    if not match:continue
    if key in self.records:raise ParseError('duplicate_'+key)
    self.records[key]=match.groups()
    if key in ('lifetime','cancel','exit'):
     elapsed=int(match.groups()[-1])
     if elapsed<self.last_elapsed:raise ParseError('backward_elapsed')
     self.last_elapsed=elapsed
 def normal(self,exit_code):
  return exit_code==0 and set(self.records)=={'ready','lifetime','exit'} and self.records['exit'][0]=='0' and int(self.records['exit'][1])>=7000000000
 def cancelled(self,exit_code):
  return exit_code==20 and 'ready' in self.records and 'cancel' in self.records and self.records.get('exit',())[0:1]==('20',)
