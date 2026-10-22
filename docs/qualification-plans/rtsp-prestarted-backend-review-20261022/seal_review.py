#!/usr/bin/env python3
"""Static read/hash-only independent review seal; no subprocess or candidate import."""
import datetime
import hashlib
import json
from pathlib import Path

OWN = Path(__file__).resolve().parent
ROOT = OWN.parents[2]


def pin(path):
    data = Path(path).read_bytes()
    return {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}


def aggregate(entries):
    h = hashlib.sha256()
    for name,item in sorted(entries.items()):
        h.update(name.encode()+b'\0'+bytes.fromhex(item['sha256']))
    return h.hexdigest()


before = json.loads((OWN/'STATIC_before01.json').read_bytes())
after = json.loads((OWN/'STATIC_after01.json').read_bytes())
assert before['status'] == after['status'] == 'STATIC PIN/RECEIPT/ARCHIVE/ELF VERIFICATION PASS; SOURCE DEFECT REVIEW SEPARATE'
assert before['input_pins'] == after['input_pins']
for name,item in after['input_pins'].items():
    assert pin(name) == item, name
assert not any(p.is_symlink() for p in OWN.rglob('*'))
outputs = {str(p.relative_to(ROOT)):pin(p) for p in sorted(OWN.rglob('*')) if p.is_file()}
result = {
    'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'scope':'Independent focused ordinary SOURCE/API prestarted-backend review only; NOT composition',
    'disposition':'CORRECTION REQUIRED: R1 observer-reaped child loses open parent IPC FD custody before client destruction',
    'command':'python3 -B docs/qualification-plans/rtsp-prestarted-backend-review-20261022/seal_review.py',
    'source_pin':after['source_pin'],'source_count':11,'author_source_seal_sha256':after['source_seal_pin'],
    'baseline_pin':after['baseline_pin'],'baseline_count':41,
    'reader_inputs_before_after_and_final_rehash_unchanged':True,
    'input_count':len(after['input_pins']),'input_aggregate':aggregate(after['input_pins']),
    'input_pins':after['input_pins'],
    'output_count':len(outputs),'output_aggregate':aggregate(outputs),'output_pins':outputs,
    'aggregate_recipe':'sha256(concat(sorted(path_utf8 + NUL + raw_sha256_bytes))); input paths absolute; outputs workspace-relative',
    'self_excluded_from_output_map':True,
    'collection_error_or_policy_rejection_observed':False,
    'candidate_compiler_generator_test_helper_guard_listener_app_execution':False,
    'target_network_private_IO':False,'native_concurrency_composite_fullapp_acceptance':False,
    'automatic_source_build_test_integration_native_queue':False,
}
with (OWN/'FINAL_SEAL.json').open('x') as out:
    json.dump(result,out,indent=2); out.write('\n')
print(json.dumps({k:result[k] for k in ('disposition','input_count','input_aggregate','output_count','output_aggregate')},indent=2))
print('FINAL_SEAL.json SHA256 '+pin(OWN/'FINAL_SEAL.json')['sha256'])
