#!/usr/bin/env python3
"""One parent-selected read-only public-artifact call; no target or C1 use."""
from dataclasses import asdict
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[3]
DOC = Path(__file__).resolve().parent
SOURCE = ROOT / 'apps/qualification/encoder-query77-inputs/selected_inputs.py'
REVIEW = ROOT / 'docs/qualification-plans/encoder-query77-inputs-review-20261022/FINAL_SEAL01.json'


def pin(path):
    raw = path.read_bytes()
    return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def main():
    # Exclusive directory is an attempt reservation, never resumed or overwritten.
    out = DOC / 'parent-observation01'
    out.mkdir(mode=0o700)
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    source_pin = pin(SOURCE)
    review_pin = pin(REVIEW)
    if source_pin != {'bytes': 18643, 'sha256': '5fbcc656882075c44c1e4f4e5df8a896fe83433a33e770deec1fd50c3792d3f0'}:
        raise RuntimeError('Parent selected-source mismatch; no validator call')
    if review_pin['sha256'] != '38283e05b5fe2d72799ad2743db9cb7c91d5cea9af4e208af53e5711cad1cc78':
        raise RuntimeError('Parent selected-review mismatch; no validator call')
    record = {
        'scope': 'ONE_OFFLINE_PUBLIC_ARTIFACT_INSPECTION_NOT_ADMISSION',
        'started_utc': started, 'source_pin': source_pin, 'review_pin': review_pin,
        'wrapper_pin': pin(Path(__file__)), 'python_version': sys.version,
        'python_executable_pin': pin(Path(sys.executable).resolve()),
        'argv': ['python3', '-B', str(Path(__file__).relative_to(ROOT))],
        'candidate_calls_selected': 1, 'candidate_calls_attempted': 0,
        'C1_or_private_HOLD_or_current_state_or_target_access': False,
        'client_or_firmware_or_test_or_generator_or_listener_execution': False,
        'automatic_retry_or_native_queue': False,
    }
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location('query77_public_inputs_parent01', SOURCE)
    if spec is None or spec.loader is None:
        raise RuntimeError('Parent loader unavailable; no validator call')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    try:
        record['candidate_calls_attempted'] = 1
        manifest = module.validate_selected_inputs()
        record['result'] = asdict(manifest)
        record['state'] = 'OFFLINE_OBSERVATION_RETURNED'
        succeeded = manifest.scope == 'OFFLINE_NOT_ADMISSION' and len(manifest.files) == 25
        record['expected_manifest_shape_observed'] = succeeded
    except module.InputsError as error:
        record['state'] = 'OFFLINE_OBSERVATION_REFUSED_NO_RETRY'
        record['primary_code'] = error.primary_code.value
        record['postcheck_code'] = error.postcheck_code.value
        record['cleanup_code'] = error.cleanup_code.value
        succeeded = False
    record['source_unchanged_after_call'] = pin(SOURCE) == source_pin
    record['review_unchanged_after_call'] = pin(REVIEW) == review_pin
    record['ended_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    succeeded = succeeded and record['source_unchanged_after_call'] and record['review_unchanged_after_call']
    with (out / 'RESULT.json').open('x') as stream:
        json.dump(record, stream, indent=2, sort_keys=True)
        stream.write('\n')
        stream.flush()
        os.fsync(stream.fileno())
    print('Offline public-artifact inspection ' + ('PASS: 25 selected inputs; no native admission' if succeeded else 'REFUSED: preserve result; no retry'))
    return 0 if succeeded else 1


if __name__ == '__main__':
    raise SystemExit(main())
