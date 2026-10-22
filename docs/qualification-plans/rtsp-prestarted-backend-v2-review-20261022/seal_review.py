#!/usr/bin/env python3
"""Static read/hash-only review seal; no subprocesses, candidate imports or execution."""
import datetime
import hashlib
import json
from pathlib import Path

OWN = Path(__file__).resolve().parent
ROOT = OWN.parents[2]


def pin(path):
    raw = Path(path).read_bytes()
    return {'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}


def aggregate(entries):
    h = hashlib.sha256()
    for name,item in sorted(entries.items()):
        h.update(name.encode()+b'\0'+bytes.fromhex(item['sha256']))
    return h.hexdigest()


before = json.loads((OWN/'STATIC_before01.json').read_bytes())
after = json.loads((OWN/'STATIC_after01.json').read_bytes())
assert before['status'] == after['status'] == 'STATIC SOURCE/RECEIPT/ARCHIVE/ELF PIN CHECK PASS; NOT RUNTIME'
assert before['input_pins'] == after['input_pins']
for name,item in after['input_pins'].items():
    assert pin(name) == item, name
assert not any(p.is_symlink() for p in OWN.rglob('*'))
outputs = {str(p.relative_to(ROOT)):pin(p) for p in sorted(OWN.rglob('*')) if p.is_file()}
result = {
    'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'scope':'Independent focused ordinary R1 SOURCE/API correction and compile custody only; NOT composition/runtime',
    'disposition':'Narrow R1 SOURCE/API sufficiency; no additional correction-required R1 ordinary defect identified',
    'command':'python3 -B docs/qualification-plans/rtsp-prestarted-backend-v2-review-20261022/seal_review.py',
    'source_count':11,'source_pin':after['source_pin'],'author_source_seal_sha256':after['source_seal_pin'],
    'v1_source_pin':after['v1_source_pin'],'v1_review_seal':after['v1_review_seal'],
    'reader_inputs_before_after_and_final_rehash_unchanged':True,
    'input_count':len(after['input_pins']),'input_aggregate':aggregate(after['input_pins']),'input_pins':after['input_pins'],
    'output_count':len(outputs),'output_aggregate':aggregate(outputs),'output_pins':outputs,
    'aggregate_recipe':'sha256(concat(sorted(path_utf8 + NUL + raw_sha256_bytes))); reader paths absolute, outputs workspace-relative',
    'self_excluded_from_output_map':True,'collection_error_or_policy_rejection_observed':False,
    'candidate_compiler_helper_generator_test_guard_listener_program_execution':False,
    'target_network_private_IO':False,'native_concurrency_composite_fullapp_acceptance':False,
    'automatic_review_retry_source_build_test_native_queue':False,
}
with (OWN/'FINAL_SEAL.json').open('x') as out:
    json.dump(result,out,indent=2); out.write('\n')
print(json.dumps({k:result[k] for k in ('disposition','input_count','input_aggregate','output_count','output_aggregate')},indent=2))
print('FINAL_SEAL.json SHA256 '+pin(OWN/'FINAL_SEAL.json')['sha256'])
