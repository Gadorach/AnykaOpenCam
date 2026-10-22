#!/usr/bin/env python3
"""Static review input/output seal only; no subprocess/candidate import or execution."""
import datetime
import hashlib
import json
import os
from pathlib import Path

OWN = Path(__file__).resolve().parent
ROOT = OWN.parents[2]


def pin(path):
    p = Path(path); h = hashlib.sha256(); count = 0
    with p.open('rb') as stream:
        while True:
            raw = stream.read(65536)
            if not raw:
                break
            h.update(raw); count += len(raw)
    result = {'bytes':count,'sha256':h.hexdigest()}
    if p.is_symlink():
        result.update(symlink=os.readlink(p),resolved=str(p.resolve()))
    return result


def aggregate(entries):
    h = hashlib.sha256()
    for name,item in sorted(entries.items()):
        h.update(name.encode()+b'\0'+bytes.fromhex(item['sha256']))
    return h.hexdigest()


before = json.loads((OWN/'STATIC_before03.json').read_bytes())
after = json.loads((OWN/'STATIC_after03.json').read_bytes())
assert before['status'] == after['status'] == 'STATIC PUBLIC PIN/RECEIPT/ELF/ARCHIVE VERIFICATION PASS; NOT EXECUTION'
assert before['input_pins'] == after['input_pins']
for name,item in after['input_pins'].items():
    assert pin(name) == item, name
for receipt in ('STATIC_before01.json','STATIC_before02.json'):
    failed = json.loads((OWN/receipt).read_bytes())
    assert failed['status'] == 'COLLECTION ERROR; NOT CANDIDATE FAILURE'
    for name,item in failed['input_pins'].items():
        assert pin(name) == item, name
assert not any(p.is_symlink() for p in OWN.rglob('*'))
outputs = {str(p.relative_to(ROOT)):pin(p) for p in sorted(OWN.rglob('*')) if p.is_file()}
result = {
    'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'scope':'Independent focused ordinary ENC-02/PLAT-01 query-client SOURCE/API and static compile custody only',
    'disposition':'Narrow conditional SOURCE/API query-client sufficiency; no correction-required ordinary defect identified',
    'command':'python3 -B docs/qualification-plans/encoder-query77-client-review-20261022/seal_review.py',
    'source_count':5,'source_pin':after['source_pin'],'query_source_pin':after['query_source_pin'],
    'input_count':len(after['input_pins']),'input_aggregate':aggregate(after['input_pins']),'input_pins':after['input_pins'],
    'successful_before_after_and_final_rehash_unchanged':True,
    'preserved_failed_collection_inputs_rehashed_unchanged':True,
    'output_count':len(outputs),'output_aggregate':aggregate(outputs),'output_pins':outputs,
    'aggregate_recipe':'sha256(concat(sorted(path_utf8 + NUL + raw_sha256_bytes))); reader paths absolute, outputs workspace-relative; aliases are path records',
    'self_excluded_from_output_map':True,
    'collection_errors':'Two reviewer-only schema/generated-linker-label bookkeeping failures preserved; not candidate or policy failures',
    'policy_rejection_observed':False,
    'candidate_script_helper_compiler_generator_test_guard_listener_client_execution':False,
    'target_network_private_IO':False,'native_recipe_runtime_memory_composition_product_acceptance':False,
    'automatic_next_implementation_build_test_recipe_native_queue':False,
}
with (OWN/'FINAL_SEAL.json').open('x') as stream:
    json.dump(result,stream,indent=2); stream.write('\n')
print(json.dumps({k:result[k] for k in ('disposition','input_count','input_aggregate','output_count','output_aggregate')},indent=2))
print('FINAL_SEAL.json SHA256 '+pin(OWN/'FINAL_SEAL.json')['sha256'])
