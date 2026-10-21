#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Each path alias/collision must fail before preflight/probe/TX; pure logical mocks."""
from dataclasses import replace
import sys,unittest
sys.dont_write_bytecode=True
from recipe import Spec,Campaign,Hold
from test_recipe import SPEC

class NeverCallbacks:
    def __init__(self,calls):self.calls=calls
    def __getattr__(self,name):
        def forbidden(*args,**kwargs):
            self.calls.append(name);raise AssertionError('callback must not run for an invalid lexical Spec')
        return forbidden

class BoundaryTests(unittest.TestCase):
    def reject_before_any_callback(self,field,path):
        calls=[];backend=NeverCallbacks(calls);client=NeverCallbacks(calls)
        with self.assertRaises(Hold):
            spec=replace(SPEC,**{field:path})
            Campaign(spec,None,backend,client,lambda:100.).start()
        self.assertEqual(calls,[])

def alias_case(field,root,leaf):
    def case(self):self.reject_before_any_callback(field,'/'+root+'/'+leaf)
    return case
for field in ['launcher','private_scope','service','child']:
    for root in ['run','tmp']:
        for leaf,tag in [('.','dot'),('..','dotdot')]:
            setattr(BoundaryTests,f'test_{field}_{root}_{tag}_before_callbacks',alias_case(field,root,leaf))
for field in ['launcher','service','child']:
    def case(self,field=field):self.reject_before_any_callback('private_scope',getattr(SPEC,field))
    setattr(BoundaryTests,f'test_private_scope_equals_{field}_before_callbacks',case)

if __name__=='__main__':unittest.main(verbosity=2)
