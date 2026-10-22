#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Bounded standalone HOST dummy-process regressions, NOT RTSP qualification."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

APP = Path(__file__).resolve().parent
ROOT = APP.parents[2]


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    name, = sys.argv[1:]
    if not name or not all(c.isalnum() or c in '-_' for c in name) or not os.getuid():
        raise RuntimeError('fresh_nonroot_HOST_run_required')
    out = ROOT / 'docs/qualification-plans/rtsp-tail-20261022/harness-correction' / name
    out.mkdir()
    pins = {p.name: sha(p) for p in (APP/'host_guard.py', APP/'host_process.py',
                                   APP/'guard_dummy.py', APP/'guard_regression.py')}
    rows = []
    cases = [('normal', True, 2), ('normal_fixture', True, 2),
             ('escape_rejected', True, 2), ('constructor_failure', False, 2),
             ('partial_ready', False, 2), ('no_ready', False, 2),
             ('ignored_sigterm', False, 2), ('fixture_timeout', False, 2),
             ('late_child', False, 0.5), ('supervisor_deadline', False, 0.5),
             ('retained_uncertainty', False, 2), ('orphan_exit_zero', False, 2)]
    report = {'scope': 'HOST standalone dummy processes ONLY; RTSP/ARM/native=0',
              'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'python': sys.version, 'source_sha256': pins, 'cases': rows, 'status': 'RUNNING'}
    try:
        for mode, normal, deadline in cases:
            receipt = out / (mode + '-guard.json')
            argv = [sys.executable, '-B', str(APP/'host_guard.py'), '--receipt', str(receipt),
                    '--deadline', str(deadline), '--term-grace', '0.2', '--reap-grace', '2',
                    '--', sys.executable, '-B', str(APP/'guard_dummy.py'), mode]
            started = time.monotonic()
            with (out / (mode + '.txt')).open('xb') as log:
                owned = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT)
                try:
                    result = owned.wait(timeout=deadline + 4)
                except subprocess.TimeoutExpired:
                    # No repeated kill/restart or unsafe abandonment of a HOLD
                    # owner: retain this parent and exact handle for disposition.
                    report['status'] = 'HOLD'; report['owner_pid'] = owned.pid
                    with (out/'HOLD.json').open('x') as f:
                        json.dump(report, f, indent=2)
                    print('HOST_DUMMY_GUARD HOLD owner_retained=1', flush=True)
                    owned.wait()  # explicit HOLD, never continue the test queue
                    raise RuntimeError('owner_HOLD_not_PASS')
            data = json.loads(receipt.read_text())
            row = {'case': mode, 'argv': argv, 'exit': result,
                   'elapsed_ms': int((time.monotonic()-started)*1000), 'guard': data}
            rows.append(row)
            if not (data['session_owned'] and data['barrier_released'] and
                    data['subreaper_observed'] and data['leader_reaped'] and
                    data['all_children_retired'] and data['normal_pass'] == normal and
                    result == (0 if normal else 2) and data['term_attempts'] == data['kill_attempts'] == 1):
                raise RuntimeError('scalar_guard_expectation')
            if mode in ('constructor_failure', 'late_child', 'supervisor_deadline',
                        'retained_uncertainty', 'orphan_exit_zero') and data['adopted_reaped'] < 1:
                raise RuntimeError('actual_descendant_wait_missing')
            if mode in ('late_child', 'supervisor_deadline') and data['reason'] != 'deadline':
                raise RuntimeError('actual_deadline_missing')
            if mode == 'orphan_exit_zero' and data['leader_exit'] != 0:
                raise RuntimeError('zero_exit_control_missing')
        if pins != {name: sha(APP/name) for name in pins}:
            raise RuntimeError('source_changed')
        report['status'] = 'PASS standalone guards ONLY'
    except BaseException as exc:
        report['status'] = 'FAIL/HOLD'; report['error_type'] = type(exc).__name__
        raise
    finally:
        report['ended_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        with (out/'REPORT.json').open('x') as f:
            json.dump(report, f, indent=2); f.write('\n')
    print('HOST_DUMMY_GUARD PASS cases=' + str(len(rows)) + ' RTSP=0 native=0')


if __name__ == '__main__':
    main()
