#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Static source seal/diffs ONLY; no builder import, images or startup inputs."""
from pathlib import Path
import datetime
import difflib
import hashlib
import json

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
APP = ROOT/'apps/rtsp-auth/isolated-server-host-tail'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    inventory_path = OUT/'CANDIDATE_SOURCE_SEAL01.json'
    diff_path = OUT/'frozen-tail-to-host-harness01.diff'
    if inventory_path.exists() or diff_path.exists():
        raise RuntimeError('fresh_evidence_required')
    provenance = json.loads((APP/'SOURCE_TAIL_PROVENANCE.json').read_text())
    originals = []
    differences = []
    for rec in provenance['files']:
        old, new = ROOT/rec['source'], ROOT/rec['copy']
        old_pin = sha(old)
        if old_pin != rec['sha256_before_edits']:
            raise RuntimeError('immutable_original_changed')
        row = {'original': rec['source'], 'copy': rec['copy'], 'original_sha256': old_pin,
               'candidate_sha256': sha(new), 'changed': old.read_bytes()!=new.read_bytes()}
        originals.append(row)
        if row['changed']:
            differences.extend(difflib.unified_diff(old.read_text().splitlines(keepends=True),
                new.read_text().splitlines(keepends=True), fromfile=rec['source'], tofile=rec['copy']))
    files = {str(p.relative_to(ROOT)): sha(p) for p in sorted(APP.rglob('*')) if p.is_file()}
    # Canonical public candidate pin: sorted workspace-relative path + NUL + raw
    # SHA-256 bytes, concatenated; same recipe recorded for independent checking.
    aggregate = hashlib.sha256()
    for path, pin in sorted(files.items()):
        aggregate.update(path.encode()+b'\0'+bytes.fromhex(pin))
    frozen_review = json.loads((OUT.parent/'source-review/final-source-receipt01.json').read_text())
    preserved_review = {p: sha(ROOT/p)==h for p,h in frozen_review['review_artifact_sha256'].items()}
    authority = json.loads((OUT.parent/'source-review/public-source-pins01.json').read_text())
    verifier_pins = {p: sha(ROOT/p)==h for p,h in authority['additional_public_source_sha256'].items()
                     if p.startswith('apps/rtsp-auth/') or p.startswith('framework/')}
    legacy = ROOT/'evidence/delegated/20261012-rtsp-isolated-server'
    halt_pins = {'HALT-SEAL.json': authority['HALT_SEAL_sha256'],
                 'tested05-to-frozen-HALT.diff': authority['historical_diff_sha256']}
    halt_matches = {p: sha(legacy/p)==h for p,h in halt_pins.items()}
    if not all([*preserved_review.values(),*verifier_pins.values(),*halt_matches.values()]):
        raise RuntimeError('preserved_review_or_authority_changed')
    with diff_path.open('x') as f:
        f.writelines(differences)
    report = {'scope': 'PUBLIC SOURCE seal; guard dummy tests only; C/RTSP/ARM NOT built/executed',
              'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'command': 'python3 -B docs/qualification-plans/rtsp-tail-20261022/harness-correction/seal_candidate01.py',
              'aggregate_recipe': 'sha256(concat(sorted(path_utf8 + NUL + raw_sha256_bytes)))',
              'candidate_pin': aggregate.hexdigest(), 'candidate_file_count': len(files),
              'candidate_source_sha256': files, 'baseline_comparisons': originals,
              'source_diff_sha256': sha(diff_path), 'completed_source_review_unchanged': preserved_review,
              'reviewed_verifier_framework_inputs_unchanged': verifier_pins,
              'HALT_evidence_unchanged': halt_matches,
              'dummy_report_sha256': sha(OUT/'dummy-guard02/REPORT.json'),
              'syntax_report_sha256': sha(OUT/'syntax02.json'),
              'builder_executed': False, 'C_or_ARM_compiled': False,
              'RTSP_or_network_executed': False, 'generated_startup_records_accessed': False}
    with inventory_path.open('x') as f:
        json.dump(report,f,indent=2);f.write('\n')
    print(json.dumps({'candidate_pin': report['candidate_pin'], 'files': len(files),
                      'baseline_changed': [r['copy'] for r in originals if r['changed']],
                      'originals_review_authority_HALT_preserved': True}))


if __name__ == '__main__':
    main()
