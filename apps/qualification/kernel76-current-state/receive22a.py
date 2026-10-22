#!/usr/bin/env python3
"""Fresh receive-only ownership for an already-powered, unverified target.

Unlike cold-boot entry, this never asserts camera OFF or existing runtime/boot
continuity. It starts the unchanged private recorder, arms its future-only catch,
and records ownership. It sends NO manual target bytes and preserves old HOLD.
"""
from pathlib import Path
import argparse
import datetime
import fcntl
import hashlib
import json
import os
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'scripts'))
import qc07f_private_root_transfer as private

b = private.boot
PART = 'foundation-20261022a'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', type=Path, required=True)
    args = parser.parse_args()
    if not __debug__:
        raise b.Refused('Assertions required')
    os.umask(0o077)
    private.private_path(args.base, directory=True, fresh=True)
    args.base.mkdir(mode=0o700)
    source = b.IMPL / 'uart_capture.py'
    if hashlib.sha256(source.read_bytes()).hexdigest() != b.RECORDER_SHA:
        raise b.Refused('Pinned recorder source changed')
    with (b.CONTROL / 'peripheral-gui-operation.lock').open('r+') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        old_hold = lock.read()
        if json.loads(old_hold) != {'state': 'HOLD', 'operation': 'one_radio_native_load_RUN21a'}:
            raise b.Refused('Original retained HOLD differs')
        if os.path.lexists(b.CONTROL / 'uart.sock') or os.path.lexists(b.receipt_path(PART)):
            raise b.Refused('Existing socket/receipt; do not take over')
        found = subprocess.run(['fuser', str(Path(b.ADAPTER).resolve())],
                               capture_output=True, text=True, timeout=3)
        if not Path(b.ADAPTER).exists() or found.returncode != 1 or found.stdout.strip():
            raise b.Refused('Adapter absent or owned')
        capture = args.base / 'uart-capture22a'
        instance = private.PrivateBoot(args.base / 'owner-admission22a01', capture)
        argv = [sys.executable, '-B', str(source), '--capture', '--part', PART,
                '--port', b.ADAPTER, '--seconds', '14400', '--private-capture-dir', str(capture)]
        with (args.base / 'receiver-console.private.txt').open('xb') as log:
            child = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=log,
                                     stderr=log, start_new_session=True)
        start = b.ticks(child.pid)
        instance.log('receive_only_child_spawned', pid=child.pid, start_ticks=start,
                     camera_OFF_not_asserted=True, current_runtime_unverified=True)
        deadline = time.monotonic() + 5
        while not (b.CONTROL / 'uart.sock').exists():
            if child.poll() is not None or time.monotonic() >= deadline:
                raise b.Refused('Recorder startup failed; preserve evidence')
            time.sleep(.025)
        state, peer = instance.call({'action': 'status'})
        instance.admit_state(state)
        found = subprocess.run(['fuser', str(Path(b.ADAPTER).resolve())],
                               capture_output=True, text=True, timeout=3)
        if (peer != (child.pid, os.getuid(), os.getgid()) or b.ticks(child.pid) != start or
                state['part'] != PART or state['adapter'] != b.ADAPTER or
                state['tx_bytes'] != 0 or state['tx_busy'] or
                found.returncode != 0 or found.stdout.split() != [str(child.pid)]):
            raise b.Refused('Fresh ownership not established')
        ack, same_peer = instance.call({'action': 'arm_abort', 'mode': 'hold-cli',
                                        'volatile_cli_hold': True}, expected_peer=peer)
        if same_peer != peer or not ack.get('armed') or ack.get('persistent_write'):
            raise b.Refused('Future-only catch not armed')
        state, same_peer = instance.call({'action': 'status'}, expected_peer=peer)
        if same_peer != peer or not state['abort_armed'] or state['tx_busy'] or state['tx_bytes'] != 0:
            raise b.Refused('Unexpected activity during receive-only startup')
        record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  'part': PART, 'actual_sole_recorder_PID': peer[0], 'start_ticks': start,
                  'capture_source_sha256': b.RECORDER_SHA, 'private_capture_dir': str(capture),
                  'fresh_bounded_nextboot_Uboot_hold_ACK': {
                      'mode': ack['mode'], 'ends_by': ack['ends_by']},
                  'camera_OFF_not_asserted': True, 'current_runtime_unverified': True,
                  'receive_only_start_no_manual_target_TX': True,
                  'original_failed_HOLD_preserved': True}
        with b.receipt_path(PART).open('x') as stream:
            json.dump(record, stream, indent=2)
            stream.write('\n')
        admitted = instance.owner(margin=1200)
        if admitted['tx_bytes'] != 0 or instance.peer != peer:
            raise b.Refused('Unexpected new sender')
        lock.seek(0)
        if lock.read() != old_hold:
            raise b.Refused('Original HOLD changed')
        instance.log('fresh_receive_only_owner_future_catch_admitted',
                     manual_target_TX=False, runtime_verified=False)
        print('Fresh private receive-only owner and future catch ready; no manual target TX; runtime unverified')


if __name__ == '__main__':
    try:
        main()
    except Exception:
        print('Receive-only admission HOLD; preserve evidence; no takeover or automatic retry', file=sys.stderr)
        raise SystemExit(1)
