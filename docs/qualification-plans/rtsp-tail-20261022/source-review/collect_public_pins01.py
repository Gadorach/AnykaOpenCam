#!/usr/bin/env python3
"""Static public-source inventory only; no imports/execution of project code.

Never enumerate run outputs or read/hash generated startup/material records.
All output names are exclusive-created; reruns refuse existing output files.
"""
from pathlib import Path
import datetime
import difflib
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
APP = ROOT / 'apps/rtsp-auth/isolated-server-candidate'
OLD = ROOT / 'evidence/delegated/20261012-rtsp-isolated-server'
SEAL = OLD / 'HALT-SEAL.json'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    outputs = [OUT / n for n in ('public-source-pins01.json',
               'recomputed-tested05-to-frozen01.diff', 'adapter-overlay01.diff')]
    if any(p.exists() for p in outputs):
        raise RuntimeError('fresh evidence names required')
    seal = json.loads(SEAL.read_text())
    rows, differences = [], []
    # The seal lists only public source, provenance and license documents.
    # No walk of integration05 or any generated fixture directory is performed.
    for relative, expected in seal['public_source_sha256'].items():
        rel = Path(relative)
        if rel.is_absolute() or '..' in rel.parts or rel.parts[0] != 'frozen-source':
            raise RuntimeError('unexpected source path')
        tail = Path(*rel.parts[1:])
        frozen = OLD / rel
        current = APP / tail
        tested = OLD / 'runs/integration05/inputs/apps/rtsp-auth/isolated-server-candidate' / tail
        row = {'source': str(current.relative_to(ROOT)), 'sealed_sha256': expected,
               'frozen_sha256': sha(frozen), 'current_sha256': sha(current),
               'tested05_sha256': sha(tested)}
        row['frozen_matches_seal'] = row['frozen_sha256'] == expected
        row['current_matches_frozen'] = row['current_sha256'] == row['frozen_sha256']
        row['tested_matches_frozen'] = row['tested05_sha256'] == row['frozen_sha256']
        rows.append(row)
        if not row['tested_matches_frozen']:
            differences.extend(difflib.unified_diff(
                tested.read_text().splitlines(keepends=True), frozen.read_text().splitlines(keepends=True),
                fromfile='TESTED-integration05/' + tail.as_posix(),
                tofile='FROZEN-HALT/' + tail.as_posix()))
    # Public verifier authority actually used by this candidate, plus the
    # explicit public headers/source inspected for this review. No images read.
    extra = [
        'apps/rtsp-auth/build.py',
        *['apps/rtsp-auth/isolated-verifier/' + n for n in
          ('verifier.h', 'adapter.c', 'engine.c', 'child.c', 'isolation.c', 'ipc.c')],
        'framework/include/camera/access_policy.h',
        'framework/include/camera/rtsp_digest_wire.h',
        'docs/plan/camera-foundation/08-STREAMING.md',
        'docs/plan/camera-foundation/09-SECURITY-AND-CONFIGURATION.md',
        'docs/plan/camera-foundation/REMAINING_FEATURES_AND_BUGS_GATED_PLAN_20261021.md',
    ]
    additional = {p: sha(ROOT / p) for p in extra}
    provenance = json.loads((APP / 'PROVENANCE.json').read_text())
    lineage = []
    for rec in provenance['derived_V3'] + provenance['reviewed_V2']:
        source = Path(rec['source'])
        # Pin only the explicitly named authored source, never report/image paths.
        source.relative_to(ROOT / 'apps/rtsp-auth')
        actual = sha(source)
        lineage.append({'source': str(source.relative_to(ROOT)), 'expected_sha256': rec['sha256'],
                        'actual_sha256': actual, 'matches': actual == rec['sha256']})
    overlay = list(difflib.unified_diff(
        (ROOT / 'apps/rtsp-auth/isolated-verifier/adapter.c').read_text().splitlines(keepends=True),
        (APP / 'verifier_adapter_overlay.c').read_text().splitlines(keepends=True),
        fromfile='reviewed-V2/adapter.c', tofile='isolated/verifier_adapter_overlay.c'))
    with outputs[1].open('x') as f:
        f.writelines(differences)
    with outputs[2].open('x') as f:
        f.writelines(overlay)
    report = {
        'evidence_class': 'SOURCE REVIEW ONLY; static hashes and textual diffs, not tests',
        'observed_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'command': 'python3 -B docs/qualification-plans/rtsp-tail-20261022/source-review/collect_public_pins01.py',
        'python': sys.version, 'collector_sha256': sha(Path(__file__)),
        'HALT_SEAL_sha256': sha(SEAL),
        'historical_diff_sha256': sha(OLD / 'tested05-to-frozen-HALT.diff'),
        'recomputed_diff_sha256': sha(outputs[1]),
        'recomputed_diff_byte_equal': outputs[1].read_bytes() == (OLD / 'tested05-to-frozen-HALT.diff').read_bytes(),
        'adapter_overlay_diff_sha256': sha(outputs[2]),
        'public_sources': rows, 'additional_public_source_sha256': additional,
        'provenance_source_comparisons': lineage,
        'source_count': len(rows),
        'changed_since_tested05': [r['source'] for r in rows if not r['tested_matches_frozen']],
        'all_frozen_match_seal': all(r['frozen_matches_seal'] for r in rows),
        'all_current_match_frozen': all(r['current_matches_frozen'] for r in rows),
        'all_provenance_sources_match': all(r['matches'] for r in lineage),
        'builder_executed': False, 'tests_executed': False, 'images_read': False,
        'generated_startup_records_read_copied_or_hashed': False,
        'native_executed': False, 'network_or_device_operations': False,
    }
    with outputs[0].open('x') as f:
        json.dump(report, f, indent=2)
        f.write('\n')
    print(json.dumps({k: report[k] for k in ('source_count', 'all_frozen_match_seal',
                     'all_current_match_frozen', 'all_provenance_sources_match',
                     'recomputed_diff_byte_equal', 'changed_since_tested05')}))


if __name__ == '__main__':
    main()
