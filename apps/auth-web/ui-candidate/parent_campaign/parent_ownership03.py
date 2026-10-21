#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Fresh UI03 paths for the pinned existing parent ownership callback.

No device/status/secret access at import; actual observations happen only when
the parent invokes ownership. Original UI02 proof files remain untouched.
"""
from pathlib import Path
import hashlib
from recipe import Hold

SOURCE=Path(__file__).resolve().with_name('parent_ownership02.py')
PIN='1200a9a6ad56bd94c1ced8e7b266973a7046d22b2510519fb1d455c86228cb39'
if SOURCE.is_symlink() or hashlib.sha256(SOURCE.read_bytes()).hexdigest()!=PIN:
    raise Hold('parent_entry_source_pin')
_scope={'__name__':'__pinned_parent_ownership03__','__file__':str(SOURCE)}
exec(compile(SOURCE.read_text(),str(SOURCE),'exec'),_scope)
_scope['BASE']=_scope['ROOT']/'evidence/qualification/qc07f/native-ui-20261012/native03'
_scope['PLAN']=_scope['BASE']/'parent-facts.json'
_scope['LOCK']=_scope['BASE']/'parent-job.lock'

def ownership(identity,part,adapter,deadline):
    return _scope['ownership'](identity,part,adapter,deadline)
