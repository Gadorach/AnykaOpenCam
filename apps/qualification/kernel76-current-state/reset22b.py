#!/usr/bin/env python3
"""One explicitly selected idle watchdog reset, ending at held U-Boot only."""
from pathlib import Path
import datetime
import hashlib
import json
import os
import sys
import time
import traceback
import watchdog22b as smoke

BASE, EVIDENCE, b, private = smoke.BASE, smoke.EVIDENCE, smoke.b, smoke.private
COMMAND = b'wdt reset\r'
ARMED = b'WATCHDOG RESET ARMED: nominal1s; fd remains open, no keepalive'


def inputs():
    smoke.inputs()
    receipt = json.loads((EVIDENCE / 'WATCHDOG_SMOKE22B01.json').read_bytes())
    assert receipt['state'] == 'RAM76_WATCHDOG_SMOKE_NORMAL_FUNCTIONAL_PASS_NOT_RESET_PROOF'
    assert receipt['recipe_sha256'] == hashlib.sha256(Path(smoke.__file__).read_bytes()).hexdigest()
    assert receipt['part'] == smoke.PART and receipt['image_sha256'] == smoke.post.SHA
    assert receipt['sameboot_pre_post_idle_PID1_console_fds_only']
    assert receipt['watchdog_logically_inactive_after']


def main():
    assert __debug__ and sys.argv[1:] in (['--check-inputs'], ['--one-idle-reset-stop-at-uboot'])
    inputs()
    if sys.argv[1:] == ['--check-inputs']:
        print('Idle reset source/smoke receipt pins PASS; no RPC/device access')
        return
    os.umask(0o077)
    with smoke.operation('one_idle_watchdog_reset22b_stop_at_uboot'):
        session = private.PrivateBoot(BASE / 'watchdog-reset22b01', BASE / 'uart-capture22b')
        try:
            checkpoint = json.loads(private.private_path(BASE / 'watchdog-smoke22b01.private-checkpoint.json').read_bytes())
            current = session.owner(margin=600)
            assert tuple(checkpoint['peer']) == session.peer
            assert checkpoint['start_ticks'] == b.ticks(session.peer[0])
            assert current['tx_bytes'] == checkpoint['tx_bytes'] and current['abort_armed'] and not current['tx_busy']
            session.epoch_cursor = checkpoint['rx_bytes']
            trial = smoke.Trial(session, checkpoint['tx_bytes'], time.monotonic() + 240)
            trial.inventory(checkpoint['boot_id'])
            trial.pid1_fds()
            assert trial.query(smoke.ATTRS['identity']).strip() == 'AK3918EV200 TOP watchdog'
            assert trial.query(smoke.ATTRS['timeout']).strip() == '5'
            current = session.owner(margin=360)
            assert current['tx_bytes'] == trial.tx and current['abort_armed'] and not current['tx_busy']
            trial.local(90)
            assert current['recent_rx'].endswith('qc07f> ')
            events_path = private.private_path(session.capture_dir / f'uart-events-{smoke.PART}.jsonl')
            event_start = events_path.stat().st_size
            start_rx, start_tx = current['rx_bytes'], trial.tx
            intent = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      'command': COMMAND.decode('ascii'), 'one_attempt_zero_retries': True,
                      'old_boot_id': checkpoint['boot_id'], 'peer': session.peer,
                      'start_ticks': trial.start, 'rx_bytes': start_rx, 'tx_bytes': start_tx,
                      'future_catch_armed': True, 'capture_seconds_remaining': session.remaining,
                      'stop_at_held_uboot_no_reload': True}
            with (BASE / 'watchdog-reset22b01-intent.private.json').open('x') as stream:
                json.dump(intent, stream, indent=2)
                stream.write('\n'); stream.flush(); os.fsync(stream.fileno())
            t0 = time.monotonic()
            session.send(COMMAND, 'One selected idle watchdog reset; no resend')
            # No send, arm, query, check_stream or Boot.wait is permitted below:
            # the new CPU boot is expected and invalidates the old Linux epoch.
            first_arm = first_banner = first_hold = None
            while True:
                trial.local()
                state, peer = trial.call_original({'action': 'status'}, expected_peer=trial.peer)
                assert peer == trial.peer
                session.admit_state(state)
                data = session.raw_since(start_rx, state)
                elapsed = time.monotonic() - t0
                if ARMED in data and first_arm is None:first_arm = elapsed
                if b.BANNER in data and first_banner is None:first_banner = elapsed
                assert data.count(b.BANNER) <= 1, 'Repeated CPU reset; no retry'
                assert b'Starting kernel' not in data and b'Linux version ' not in data
                assert b'SF:' not in data and b'Please enter password:' not in data
                assert not smoke.BAD.search(data)
                if elapsed >= 5:assert first_arm is not None or first_banner is not None
                caught = (b.BANNER in data and b'board_version: 5' in data and b'DRAM:  64 MiB' in data
                          and data.rstrip().endswith(b'anyka$') and not state['abort_armed'] and not state['tx_busy'])
                if caught and first_hold is None:
                    assert elapsed <= 20
                    first_hold = elapsed
                if first_hold is None:
                    assert elapsed < 20, 'Missing reset/catch; manual OFF fallback required'
                else:
                    assert caught, 'Held U-Boot lost; no repair TX'
                    if elapsed - first_hold >= 10:break
                assert elapsed < 31
                time.sleep(.025)
            # Positive command proof requires the complete arm receipt as well as
            # genuine reset/catch. Incomplete receipt remains HOLD, even if caught.
            assert first_arm is not None and first_arm <= 5 and first_banner <= 20
            with events_path.open('rb') as stream:
                stream.seek(event_start)
                tail = stream.read()
            assert tail.endswith(b'\n')
            events = [json.loads(line) for line in tail.splitlines()]
            permitted = {'tx_protected_intent', 'tx_complete', 'abort_arm_triggered',
                         'abort_burst_cancelled', 'new_boot_window_reset'}
            assert all(row['kind'] in permitted for row in events)
            assert sum(row['kind'] == 'abort_arm_triggered' for row in events) == 1
            lengths = [row['bytes'] for row in events if row['kind'] == 'tx_protected_intent']
            completed = [row['bytes'] for row in events if row['kind'] == 'tx_complete']
            assert lengths == completed and lengths[0] == len(COMMAND)
            assert 1 <= len(lengths[1:]) <= 3 and set(lengths[1:]) == {19}
            assert state['tx_bytes'] == start_tx + sum(lengths)
            # Revalidate actual sole owner/lifetime, without sending to U-Boot.
            final = session.owner(margin=300)
            assert session.peer == trial.peer and final['tx_bytes'] == state['tx_bytes']
            assert final['rx_bytes'] == state['rx_bytes'] and not final['abort_armed'] and not final['tx_busy']
            record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      'state': 'ONE_IDLE_WATCHDOG_RESET_AND_STABLE_UBOOT_CATCH_PASS',
                      'part': smoke.PART, 'image_before_reset_sha256': smoke.post.SHA,
                      'complete_nominal1s_arm_receipt': True, 'genuine_main_CPU_reset_banner': True,
                      'stable_held_uboot_seconds': 10, 'reset_attempts': 1,
                      'automatic_catch_packets': len(lengths)-1, 'new_TX_bytes_after_preflight': sum(lengths),
                      'old_Linux_epoch_invalidated_not_normally_closed': True,
                      'future_catch_consumed_rearm_required_before_effect': True,
                      'no_reload_RUN_GPIO_media_or_persistent_write': True,
                      'original_radio_HOLD_preserved': True, 'radio_rail_cold_reset_proved': False,
                      'active_DMA_reset_retirement_proved': False,
                      'recipe_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
            with (BASE / 'watchdog-reset22b01-checkpoint.private.json').open('x') as stream:
                json.dump({'peer': trial.peer, 'start_ticks': trial.start,
                           'rx_bytes': final['rx_bytes'], 'tx_bytes': final['tx_bytes'],
                           'event_file_end': event_start + len(tail),
                           'first_arm_observed_seconds': first_arm, 'first_banner_observed_seconds': first_banner,
                           'first_hold_observed_seconds': first_hold}, stream, indent=2)
                stream.write('\n')
            with (EVIDENCE / 'WATCHDOG_RESET22B01.json').open('x') as stream:
                json.dump(record, stream, indent=2); stream.write('\n')
            session.log('one_idle_watchdog_reset_stable_uboot_complete', **record)
            print('One idle watchdog reset and10s stable original U-Boot catch PASS; stopped, no reload')
        except BaseException:
            with (BASE / 'watchdog-reset22b01-error.private.txt').open('x') as stream:
                traceback.print_exc(file=stream)
            session.log('HOLD_watchdog_reset_no_retry_or_rescue_TX')
            print('Idle reset HOLD; no retry/rescue TX; retain evidence; manual OFF if catch failed', file=sys.stderr)
            raise SystemExit(1)


if __name__ == '__main__':
    main()
