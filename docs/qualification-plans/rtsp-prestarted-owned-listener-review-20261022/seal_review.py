#!/usr/bin/env python3
"""Independent static review hash seal; no subprocesses, candidate imports or execution."""
import datetime
import hashlib
import json
from pathlib import Path

OWN = Path(__file__).resolve().parent
ROOT = OWN.parents[2]


def pin(path):
    h = hashlib.sha256(); count = 0
    with Path(path).open('rb') as stream:
        while True:
            raw = stream.read(65536)
            if not raw:
                break
            h.update(raw); count += len(raw)
    return {'bytes':count,'sha256':h.hexdigest()}


def aggregate(entries):
    h = hashlib.sha256()
    for name,item in sorted(entries.items()):
        h.update(name.encode()+b'\0'+bytes.fromhex(item['sha256']))
    return h.hexdigest()


before = json.loads((OWN/'STATIC_before01.json').read_bytes())
after = json.loads((OWN/'STATIC_after01.json').read_bytes())
assert before['status'] == after['status'] == 'STATIC COMPOSITION PIN/RECEIPT/ARCHIVE/ELF CHECK PASS; NOT EXECUTION'
assert before['input_pins'] == after['input_pins']
for name,item in after['input_pins'].items():
    assert pin(name) == item, name
assert not any(p.is_symlink() for p in OWN.rglob('*'))
outputs = {str(p.relative_to(ROOT)):pin(p) for p in sorted(OWN.rglob('*')) if p.is_file()}
result = {
    'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'scope':'Independent focused ordinary SOURCE composition/API custody and static compile evidence only',
    'disposition':'Narrow SOURCE composition/API custody sufficiency; no correction-required ordinary defect identified',
    'command':'python3 -B docs/qualification-plans/rtsp-prestarted-owned-listener-review-20261022/seal_review.py',
    'source_count':14,'source_pin':after['source_pin'],'author_source_seal_sha256':after['source_seal_pin'],
    'author_build_seal_sha256':after['build_seal_pin'],
    'before_after_and_final_fresh_reader_hashes_unchanged':True,
    'input_count':len(after['input_pins']),'input_aggregate':aggregate(after['input_pins']),'input_pins':after['input_pins'],
    'output_count':len(outputs),'output_aggregate':aggregate(outputs),'output_pins':outputs,
    'aggregate_recipe':'sha256(concat(sorted(path_utf8 + NUL + raw_sha256_bytes))); reader paths absolute, outputs workspace-relative',
    'self_excluded_from_output_map':True,
    'collection_notes':'One initial nonexistent sibling-header basename lookup preserved; static collectors succeeded; no policy rejection',
    'policy_rejection_observed':False,
    'candidate_helper_scriptmain_compiler_generator_test_guard_listener_app_client_execution':False,
    'target_network_private_IO':False,'whole_backend_bootstrap_native_concurrency_fullapp_product_acceptance':False,
    'automatic_review_retry_source_provider_adoption_build_test_native_queue':False,
}
with (OWN/'FINAL_SEAL.json').open('x') as stream:
    json.dump(result,stream,indent=2); stream.write('\n')
print(json.dumps({k:result[k] for k in ('disposition','input_count','input_aggregate','output_count','output_aggregate')},indent=2))
print('FINAL_SEAL.json SHA256 '+pin(OWN/'FINAL_SEAL.json')['sha256'])
