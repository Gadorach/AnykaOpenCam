#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Explicit parent-reviewed entry only. Default invocation never contacts a target."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,sys,time
sys.dont_write_bytecode=True
from recipe import Spec,Expected,Campaign,Hold,redacted_reason
from owned_client import OwnedClient

def load_entry(path,sha256,roots):
    path=Path(path)
    if not path.is_absolute() or path.is_symlink() or not any(path.resolve().is_relative_to(Path(root).resolve()) for root in roots):raise Hold('explicit_public_entry_root')
    if hashlib.sha256(path.read_bytes()).hexdigest()!=sha256:raise Hold('parent_entry_source_pin')
    module_spec=importlib.util.spec_from_file_location('reviewed_parent_entry',path);module=importlib.util.module_from_spec(module_spec);module_spec.loader.exec_module(module)
    for name in ['uart_capture','probe','checked_command','prove_bootstrap','poststate']:
        if not callable(getattr(module,name,None)):raise Hold('existing_entry_callback_missing')
    # Child bindings are actual existing public code paths, not import/search aliases.
    inputs=getattr(module,'PUBLIC_SOURCE_INPUTS',None)
    if not isinstance(inputs,dict) or set(inputs)!= {'uart_capture','probe','checked_command'}:raise Hold('existing_source_bindings_missing')
    for record in inputs.values():
        p=Path(record['path'])
        if not p.is_absolute() or p.is_symlink() or not any(p.resolve().is_relative_to(Path(root).resolve()) for root in roots) or hashlib.sha256(p.read_bytes()).hexdigest()!=record['sha256']:raise Hold('existing_source_binding_pin')
    return module

def load_ownership(record,roots):
    # Actual parent-owned function-only source; never import the audited CLI scripts.
    if not isinstance(record,dict) or set(record)!= {'path','sha256','function'} or record['function']!='ownership':raise Hold('existing_source_binding_pin')
    p=Path(record['path'])
    if not p.is_absolute() or p.is_symlink() or p.name in ('probe.py','checked_command.py','uart_capture.py') or not any(p.resolve().is_relative_to(Path(root).resolve()) for root in roots) or hashlib.sha256(p.read_bytes()).hexdigest()!=record['sha256']:raise Hold('existing_source_binding_pin')
    module_spec=importlib.util.spec_from_file_location('reviewed_parent_ownership',p);module=importlib.util.module_from_spec(module_spec);module_spec.loader.exec_module(module)
    callback=getattr(module,'ownership',None)
    if not callable(callback):raise Hold('existing_entry_callback_missing')
    return callback

def main():
    p=argparse.ArgumentParser(description='Offline by default; parent-only single finite job, no retries/staging/owner creation')
    p.add_argument('--parent-run',action='store_true');p.add_argument('--entry-file');p.add_argument('--entry-sha256')
    p.add_argument('--entry-root',action='append',default=[]);p.add_argument('--public-plan',type=Path);p.add_argument('--output',type=Path)
    args=p.parse_args()
    if not args.parent_run:
        print('OFFLINE_ONLY: run tests; native requires parent-reviewed explicit existing entry and fresh measured public admission');return
    if not all([args.entry_file,args.entry_sha256,args.entry_root,args.public_plan,args.output]):raise Hold('explicit_parent_inputs_required')
    evidence=Path(__file__).resolve().parents[4]/'evidence/delegated/20261012-native-ui/parent-campaign'
    if not args.output.resolve().is_relative_to(evidence.resolve()) or args.output.exists():raise Hold('fresh_receipt_output_scope')
    plan=json.loads(args.public_plan.read_text())
    base_fields={'spec','expected','client_source','client_sha256'}
    if set(plan) not in (base_fields,base_fields|{'entry_proofs','ownership_callback'}):raise Hold('public_plan_fields')
    spec_values=plan['spec'];spec_values['pins']=tuple(spec_values['pins']);spec=Spec(**spec_values)
    expected_values=plan['expected'];expected_values['identity']=tuple(expected_values['identity']);expected=Expected(**expected_values)
    source=Path(plan['client_source']);canonical=Path(__file__).resolve().parents[2]/'native-launch/private_client.py'
    if source.resolve()!=canonical.resolve():raise Hold('canonical_PC_client_path_only')
    backend=load_entry(args.entry_file,args.entry_sha256,args.entry_root)
    holder=[None]
    if callable(getattr(backend,'configure',None)):
        if not {'entry_proofs','ownership_callback'}.issubset(plan):raise Hold('explicit_parent_inputs_required')
        owner=load_ownership(plan['ownership_callback'],args.entry_root)
        def budget():
            job=holder[0];now=time.monotonic()
            if getattr(job,'stage',None)=='HOLD':return min(now+1,getattr(job,'end',expected.arm_end-5))
            return min(getattr(job,'phase_end',now+1),getattr(job,'end',expected.arm_end-5))
        backend.configure(spec,expected,plan['entry_proofs'],callbacks={'ownership':owner,'deadline':budget})
    client=OwnedClient(source,plan['client_sha256']);job=Campaign(spec,expected,backend,client,time.monotonic)
    holder[0]=job
    try:
        job.start()
        while job.stage not in ('PASS','HOLD'):
            job.tick();time.sleep(0.02)
    except BaseException as error:job.abort(redacted_reason(error))
    finally:
        try:client.dispose()
        except Exception:job.abort('PC_disposal_unconfirmed')
        args.output.parent.mkdir(parents=True,exist_ok=True)
        audit=getattr(backend,'audit_summary',None)
        entry_audit=audit() if callable(audit) else None
        with args.output.open('x') as out:json.dump({'state':job.stage,'failure':job.failed,'events':job.history,
          'entry_audit':entry_audit,
          'scope':'parent-only campaign receipt; no credentials/raw UART/client text/material hashes',
          'limits':{'PC_client':140,'service_job':180,'watchdog':5,'feed_interval':4,'feed_counter':45},
          'all_G3_G8_media_product_flags':False},out,indent=2)
    if job.stage!='PASS':raise SystemExit(1)
if __name__=='__main__':
    try:main()
    except BaseException as error:print('HOLD '+redacted_reason(error));raise SystemExit(1)
