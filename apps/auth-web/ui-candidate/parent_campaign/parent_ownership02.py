#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Parent's native02 ownership callback; import performs no I/O.

Borrows the existing receiver; never opens UART, arms/reset recovery or reads
private material. The plan contains measured public facts only. The parent must
hold the job lock and keep all other target transfers stopped for its lifetime.
"""
from pathlib import Path
import fcntl,json,os,subprocess,time
from recipe import Hold

ROOT=Path(__file__).resolve().parents[4]
BASE=ROOT/'evidence/qualification/qc07f/native-ui-20261012/native02'
PLAN=BASE/'parent-facts.json'
LOCK=BASE/'parent-job.lock'
_last_fuser=None

def ownership(identity,part,adapter,deadline):
    if not 0<deadline-time.monotonic()<=1:raise Hold('current_admission_missing')
    # A conflicting parent coordinator must not launch another target operation.
    # This is job-local serialization, not a new device owner or guardian.
    fd=os.open(LOCK,os.O_RDWR|os.O_NOFOLLOW|os.O_CLOEXEC)
    try:
        try:
            fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
        except BlockingIOError:
            pass  # the actual parent runner holds the lock
        else:
            fcntl.flock(fd,fcntl.LOCK_UN)
            raise Hold('current_admission_missing')
    finally:os.close(fd)
    plan=json.loads(PLAN.read_text())
    expected=plan['expected'];proofs=plan['entry_proofs']
    if tuple(expected['identity'])!=tuple(identity) or proofs['receiver']['part']!=part or proofs['receiver']['adapter']!=adapter:
        raise Hold('current_admission_missing')
    left=deadline-time.monotonic()
    if left<=0:raise Hold('current_admission_missing')
    global _last_fuser
    key=(tuple(identity),part,adapter)
    if _last_fuser is None or _last_fuser[0]!=key or time.monotonic()-_last_fuser[1]>.2:
        result=subprocess.run(['fuser',str(Path(adapter).resolve())],capture_output=True,text=True,timeout=left)
        pids=[int(p) for p in result.stdout.split() if p.isdecimal()]
        observed=time.monotonic()
        if result.returncode!=0 or pids!=[identity[2]] or observed>deadline:
            raise Hold('current_admission_missing')
        _last_fuser=(key,observed,pids)
    # Preserve the actual observation time even within this short proof cache.
    observed,pids=_last_fuser[1:]
    # Explicit current parent work ledger, not a field invented in UART status.
    ledger=plan['parent_exclusive_work']
    if (ledger['parent_pid']!=os.getpid() or ledger['part']!=part or
        ledger['instance']!=proofs['receiver']['instance'] or ledger['other_target_transfers']!=[]):
        raise Hold('current_admission_missing')
    facts=plan['parent_fact_lifetime']
    return {'source_id':'parent-fuser-native02','identity':tuple(identity),'part':part,
            'adapter':adapter,'observed':observed,'receive_pids':pids,
            'transfers':ledger['other_target_transfers'],'fact_lifetime':facts}
