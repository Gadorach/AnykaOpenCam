#!/usr/bin/env python3
"""Freeze fresh independent review receipts; static read/hash only."""
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
assert before['status'] == after['status'] == 'STATIC RECEIPT/HASH/ARCHIVE/ELF CHECK PASS; NOT EXECUTION OR BEHAVIORAL PROOF'
assert before['input_pins'] == after['input_pins']
for name,item in after['input_pins'].items():
    assert pin(name) == item, name
outputs = {str(p.relative_to(ROOT)):pin(p) for p in sorted(OWN.rglob('*')) if p.is_file()}
assert all(not p.is_symlink() for p in OWN.rglob('*'))
result = {
    'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'scope':'Independent focused ordinary SOURCE/API listener custody sufficiency only',
    'command':'python3 -B docs/qualification-plans/rtsp-owned-listener-review-20261022/seal_review.py',
    'source_pin':after['source']['aggregate'],
    'source_count':47,
    'original_frozen_pin':after['source']['original_aggregate'],
    'original_imports':41,
    'reader_inputs_unchanged_before_after_and_final_rehash':True,
    'input_count':len(after['input_pins']),
    'input_aggregate':aggregate(after['input_pins']),
    'input_pins':after['input_pins'],
    'output_count':len(outputs),
    'output_aggregate':aggregate(outputs),
    'output_pins':outputs,
    'aggregate_recipe':'sha256(concat(sorted(path_utf8 + NUL + raw_sha256_bytes))); output paths workspace-relative, reader paths absolute',
    'self_excluded_from_output_map':True,
    'candidate_compiler_generator_test_helper_listener_app_execution':False,
    'target_network_private_IO':False,
    'native_concurrency_composite_fullapp_acceptance':False,
    'automatic_test_build_integration_native_queue':False,
    'policy_rejection_observed':False,
    'collection_errors':'Two initial nonexistent-path read lookups preserved in COLLECTION_ERRORS.md; static collection succeeded',
}
with (OWN/'FINAL_SEAL.json').open('x') as stream:
    json.dump(result,stream,indent=2)
    stream.write('\n')
print(json.dumps({k:result[k] for k in ('input_count','input_aggregate','output_count','output_aggregate')},indent=2))
print('FINAL_SEAL.json SHA256 '+pin(OWN/'FINAL_SEAL.json')['sha256'])
