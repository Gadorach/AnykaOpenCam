#!/usr/bin/env python3
"""Public input revalidation and exclusive static W1-W3 review output seal."""
import datetime
import hashlib
import json
from pathlib import Path
import posixpath
import shlex
import sys

ROOT = Path('/home/gadorach/Desktop/AnykaOpenCam')
OUT = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-v2-review-20261022'
RUN = ROOT / 'docs/qualification-plans/qc07f-watchdog-owner-v2-20261022/runs/build01'


def pin(path):
    if path.is_symlink():
        raise ValueError('unexpected symlink')
    raw = path.read_bytes()
    return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def main():
    reader = json.loads((OUT / 'reader-receipt.json').read_bytes())
    for rel, expected in reader['inputs'].items():
        if Path(rel).is_absolute() or '..' in Path(rel).parts:
            raise ValueError('unexpected relative input')
        if pin(ROOT / rel) != expected:
            raise ValueError('public input changed: ' + rel)
    report = json.loads((RUN / 'REPORT.json').read_bytes())
    changed_rows = []
    for mode in ('host', 'arm'):
        for unit in ('native_adapter', 'watchdog_owner'):
            depfile = RUN / 'objects' / (mode + '-' + unit + '.d')
            deps = shlex.split(depfile.read_text().replace('\\\n', '').split(':', 1)[1])
            projects = []
            prefix = str(RUN / 'source') + '/'
            for name in deps:
                normalized = posixpath.normpath(name)
                if normalized.startswith(prefix):
                    rel = normalized[len(prefix):]
                    if report['compiled_source_header_closure'][name] != report['source_files'][rel]:
                        raise ValueError('changed-unit project dependency differs from final source')
                    projects.append(rel)
            if 'watchdog_owner.h' not in projects:
                raise ValueError('changed consumer did not consume new owner header')
            commands = [c for c in report['commands'] if '-c' in c['argv'] and
                        str(RUN / 'source' / (unit + '.c')) in c['argv'] and
                        Path(c['log']).name.startswith(mode + '-')]
            if len(commands) != 1:
                raise ValueError('missing unique changed-unit compile command')
            changed_rows.append({'mode': mode, 'unit': unit, 'dependency_count': len(deps),
                                 'final_project_dependencies_verified': projects})
    paths = sorted(p for p in OUT.iterdir() if p.is_file())
    data = {
        'UTC': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'argv': sys.argv,
        'cwd': str(ROOT), 'python': sys.version,
        'reader_inputs_revalidated_unchanged': len(reader['inputs']),
        'changed_consumer_final_header_dependencies': changed_rows,
        'candidate_source_tree_sha256': 'e511920a5c4cc53eac99d4d56e1b50e547c726764b790dbc881aa16ef94c7d8b',
        'disposition': 'Narrow static SOURCE/API W1-W3 sufficiency and changed/reused-object custody sufficiency only; no functional/native/provider/RTSP/full-app adoption',
        'scope': 'public static source/diff/dependency/archive-byte/hash metadata only; no compiler/generator/tests/target/private/P1/Git work',
        'files': {str(p.relative_to(ROOT)): pin(p) for p in paths},
    }
    with (OUT / 'REVIEW_SEAL01.json').open('x') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'inputs_unchanged': len(reader['inputs']), 'changed_consumers_verified': len(changed_rows),
                      'review_files': len(data['files']), 'seal': pin(OUT / 'REVIEW_SEAL01.json')}))


if __name__ == '__main__':
    if sys.flags.optimize or sys.argv[1:]:
        raise SystemExit('no arguments or optimized Python')
    main()
