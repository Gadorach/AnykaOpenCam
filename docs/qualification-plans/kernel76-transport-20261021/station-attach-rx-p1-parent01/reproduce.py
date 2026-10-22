#!/usr/bin/env python3
"""Fresh parent HOST reproduction, relocating only the collector evidence root.

The sealed source, test bodies and collector remain unchanged. The one textual
replacement below is recorded explicitly, not claimed a byte-identical runner.
No target, network, private input, cross compiler or old executable is used.
"""
from pathlib import Path
import datetime
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
DOC = ROOT / 'docs/qualification-plans/kernel76-transport-20261021'
AUTHOR = DOC / 'station-attach-rx-p1-host01'
RUNNER = ROOT / 'apps/qualification/kernel76-station-attach-rx/run_host_tests.py'


def pin(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def write_new(path, record):
    with path.open('x') as stream:
        json.dump(record, stream, indent=2)
        stream.write('\n')


def main():
    if sys.flags.optimize or sys.argv[1:] != ['--run', 'run04']:
        raise RuntimeError('unoptimized Python and exact fresh --run run04 required')
    seal = json.loads((AUTHOR / 'candidate-seal.json').read_bytes())
    for relative, expected in seal['files'].items():
        assert pin(ROOT / relative) == expected, relative
    original = RUNNER.read_bytes()
    old = b"E = DOC / 'station-attach-rx-p1-host01'"
    new = b"E = DOC / 'station-attach-rx-p1-parent01'"
    assert original.count(old) == 1
    relocated = original.replace(old, new)
    assert not (OUT / 'run04').exists()
    baseline = (AUTHOR / 'copy-baseline.json').read_bytes()
    with (OUT / 'copy-baseline.json').open('xb') as stream:
        stream.write(baseline)
    record = {
        'utc_start': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
        'scope': 'PARENT_FRESH_SYNTHETIC_HOST_REPRODUCTION_ONLY',
        'wrapper': pin(Path(__file__)), 'original_collector': pin(RUNNER),
        'candidate_seal': pin(AUTHOR / 'candidate-seal.json'),
        'relocation_old': old.decode(), 'relocation_new': new.decode(),
        'relocated_collector_sha256': hashlib.sha256(relocated).hexdigest(),
        'only_collector_change': 'evidence directory binding; baseline copy byte-exact',
        'reported_runner_sha256_is_original_file': True,
        'native_or_ARM_operation': False,
    }
    write_new(OUT / 'invocation.json', record)
    try:
        exec(compile(relocated, str(RUNNER), 'exec'),
             {'__name__': '__main__', '__file__': str(RUNNER)})
        result = json.loads((OUT / 'run04/result.json').read_bytes())
        assert result['all_pass']
        for relative, expected in seal['files'].items():
            assert pin(ROOT / relative) == expected, relative
        record['all_pass'] = True
        record['all_453_candidate_files_unchanged_after_run'] = len(seal['files']) == 453
    except BaseException as exc:
        record['all_pass'] = False
        record['exception_type'] = type(exc).__name__
        raise
    finally:
        record['utc_end'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        write_new(OUT / 'completion.json', record)


if __name__ == '__main__':
    main()
