#!/usr/bin/env python3
"""One normal RAM76 watchdog timeout/ping/magic-close functional trial.

Known-idle22b only; no deliberate reset, station, media, peripheral or disk write.
The actual monitor arms5s before its finite smoke sequence; a hang must use that
guard/future catch, never a second sender/reset. All raw evidence stays private.
"""
from pathlib import Path
import datetime
import hashlib
import json
import os
import re
import sys
import time
import traceback
import uuid

ROOT = Path(__file__).resolve().parents[3]
EVIDENCE = ROOT / 'evidence/qualification/qc07f/execution-20261022/ram76-resume22b'
sys.path.insert(0, str(EVIDENCE))
import postcheck as post
from preserved_hold import operation

private, b, BASE = post.private, post.b, post.BASE
PART = 'foundation-20261022b'
PROMPT = b'qc07f> '
ATTRS = {
    'timeout': 'cat /sys/class/watchdog/watchdog0/timeout',
    'identity': 'cat /sys/class/watchdog/watchdog0/identity',
}
ALLOWED = set(post.QUERIES.values()) | set(ATTRS.values()) | {'shell', 'exit', 'wdt smoke'}
BAD = re.compile(rb'BUG:|WARNING:|Oops:|Kernel panic|Unhandled fault|Babble|babble')


def inputs():
    receipt = json.loads((EVIDENCE / 'RAM76_POSTCHECK22B01.json').read_bytes())
    derivation = json.loads((EVIDENCE / 'SOURCE_DERIVATION22B01.json').read_bytes())
    pin = next(row['new_sha256'] for row in derivation['recipe_source_transforms']
               if row['file'] == 'postcheck.py')
    assert hashlib.sha256((EVIDENCE / 'postcheck.py').read_bytes()).hexdigest() == pin
    assert receipt['recipe_sha256'] == pin and receipt['part'] == PART
    assert receipt['state'] == 'RAM76_NATIVE_BOOT_READONLY_IDLE_POSTCHECK_PASS'
    assert receipt['image_sha256'] == post.SHA
    assert receipt['watchdog_inactive_nowayout_zero'] and receipt['modules_empty']
    binding = json.loads((ROOT / 'docs/qualification-plans/kernel76-transport-20261021/recovery-source-review/binding.json').read_bytes())
    for name, expected in binding['input_sha256'].items():
        assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected, name
    return receipt


class Trial:
    def __init__(self, session, expected_tx, deadline):
        self.s, self.tx, self.deadline = session, expected_tx, deadline
        self.peer = session.peer
        self.start = b.ticks(self.peer[0])
        self.call_original = session.call
        self.old_hold = (b.CONTROL / 'peripheral-gui-operation.lock').read_bytes()
        # Actual Boot.send/wait internal RPCs share the same phase/time/peer fence.
        session.call = self.call
        self.begin_rx = session.state['rx_bytes']

    def local(self, reserve=0):
        assert self.deadline - time.monotonic() > reserve
        assert self.s.capture_deadline - time.monotonic() > 300
        assert self.s.peer == self.peer and b.ticks(self.peer[0]) == self.start
        assert (b.CONTROL / 'peripheral-gui-operation.lock').read_bytes() == self.old_hold

    def call(self, request, expected_peer=None):
        assert request['action'] in ('status', 'send')
        self.local(10 if request['action'] == 'send' else 0)
        result = self.call_original(request, expected_peer=expected_peer)
        self.local()
        return result

    def state(self):
        self.local()
        value = self.s.check_stream()
        assert value['part'] == PART and value['tx_bytes'] == self.tx
        assert value['abort_armed'] and not value['tx_busy']
        assert not BAD.search(self.s.raw_since(self.begin_rx, value))
        return value

    def fixed(self, wire, marker=PROMPT, timeout=6):
        assert 0 < len(wire) <= 64
        before = self.state()
        self.s.send(wire, 'Fixed22b watchdog functional smoke/readonly admission')
        self.tx += len(wire)
        result = self.s.wait(marker, before['rx_bytes'], timeout, reject_reset=True)
        self.state()
        return result

    def query(self, command, timeout=6):
        assert command in ALLOWED and self.state()['recent_rx'].endswith('qc07f> ')
        return post.body(self.fixed(command.encode() + b'\r', timeout=timeout), command)

    def inventory(self, expected_boot):
        rows = {key: self.query(command) for key, command in list(post.QUERIES.items())
                + [('boot_id_after', post.QUERIES['boot_id'])]}
        memory = post.parse(rows)
        assert rows['boot_id'].strip() == expected_boot
        return memory

    def pid1_fds(self):
        answer = self.fixed(b'shell\r', marker=b'# ')
        assert re.search(rb'(?:^|\n)# $', answer.replace(b'\r', b''))
        nonce = uuid.uuid4().hex
        command = f'echo WDTFD_BEGIN_{nonce}; /bin/busybox ls -l /proc/1/fd; echo WDTFD_DONE_{nonce}:$?'
        assert len(command) < 950 and re.fullmatch('[0-9a-f]{32}', nonce)
        start = self.state()['rx_bytes']
        for offset in range(0, len(command), 64):
            self.s.send(command[offset:offset+64].encode(), 'Fixed cached PID1 FD inventory')
            self.tx += len(command[offset:offset+64])
            self.state()
            time.sleep(.025)
        until = time.monotonic() + 3
        while True:
            state = self.state()
            echo = re.sub(rb'\r+\n', b'', self.s.raw_since(start, state))
            if echo == command.encode():
                break
            assert command.encode().startswith(echo) and time.monotonic() < until
            time.sleep(.01)
        body = self.fixed(b'\r', marker=b'# ', timeout=10).decode('ascii').replace('\r', '')
        lines = body.splitlines()
        assert lines.count('WDTFD_BEGIN_' + nonce) == lines.count('WDTFD_DONE_' + nonce + ':0') == 1
        first = lines.index('WDTFD_BEGIN_' + nonce)
        last = lines.index('WDTFD_DONE_' + nonce + ':0')
        assert all(line == '' for line in lines[:first]) and lines[last+1:] == ['# ']
        rows = lines[first+1:last]
        assert len(rows) == 4
        entries = [re.fullmatch(r'lrwx------[ \t]+1[ \t]+64[ \t]+([0-3]) -> /dev/(?:console|ttyAK0)', row) for row in rows]
        assert all(entries) and {entry[1] for entry in entries} == {'0', '1', '2', '3'}
        answer = self.fixed(b'exit\r')
        assert b'RAM-only shell returned to monitor' in answer


def main():
    assert not sys.flags.optimize
    assert sys.argv[1:] in (['--check-inputs'], ['--one-owned-idle-smoke-only'])
    receipt = inputs()
    if sys.argv[1:] == ['--check-inputs']:
        print('22b watchdog smoke source/image/idle-input pins checked; no RPC or device access')
        return
    os.umask(0o077)
    with operation('one_RAM76_watchdog_smoke22b'):
        checkpoint = json.loads(private.private_path(BASE / 'boot-checkpoint22b01.private.json').read_bytes())
        runtime = json.loads(private.private_path(BASE / 'runtime22b01.private.json').read_bytes())
        expected = checkpoint['tx_bytes'] + 320 + int(receipt['one_empty_monitor_refresh'])
        session = private.PrivateBoot(BASE / 'watchdog-smoke22b01', BASE / 'uart-capture22b')
        try:
            current = session.owner(margin=600)
            assert current['part'] == PART and current['tx_bytes'] == expected
            assert session.peer[0] == checkpoint['recorder_pid'] and b.ticks(session.peer[0]) == checkpoint['start_ticks']
            assert current['abort_armed'] and not current['tx_busy']
            session.epoch_cursor = checkpoint['rx_bytes']
            trial = Trial(session, expected, time.monotonic() + 240)
            if not trial.state()['recent_rx'].endswith('qc07f> '):
                assert 'qc07f> ' in current['recent_rx']
                assert trial.fixed(b'\r').replace(b'\r', b'') == b'\nqc07f> '
            boot_id = runtime['boot_id'].strip()
            trial.inventory(boot_id)
            trial.pid1_fds()
            assert trial.query(ATTRS['identity']).strip() == 'AK3918EV200 TOP watchdog'
            timeout = trial.query(ATTRS['timeout']).strip()
            assert re.fullmatch(r'[0-9]+', timeout) and 1 <= int(timeout) <= 60
            owner = session.owner(margin=360)
            assert session.peer == trial.peer and owner['tx_bytes'] == trial.tx and owner['abort_armed']
            trial.local(90)  # do not arm the device if normal postflight won't fit
            result = trial.query('wdt smoke', timeout=10)
            assert result.splitlines() == [
                'Explicit watchdog register test; no GPIO or SPI writes',
                'WATCHDOG PING ACCEPTED', 'WATCHDOG TIMEOUT VERIFIED5',
                'WATCHDOG MAGIC CLOSE COMPLETED']
            time.sleep(10)  # intentional passive no-reset observation, not feeding
            trial.state()
            after = trial.inventory(boot_id)
            trial.pid1_fds()
            assert trial.query(ATTRS['timeout']).strip() == '5'
            final = trial.state()
            record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      'state': 'RAM76_WATCHDOG_SMOKE_NORMAL_FUNCTIONAL_PASS_NOT_RESET_PROOF',
                      'part': PART, 'image_sha256': post.SHA, 'nominal_timeout_seconds': 5,
                      'actual_monitor_ping_GETTIMEOUT_magic_close_receipts': True,
                      'sameboot_pre_post_idle_PID1_console_fds_only': True,
                      'watchdog_logically_inactive_after': True, 'passive_no_reset_seconds': 10,
                      'electrical_stop_or_deliberate_reset_proved': False,
                      'new_TX_bytes': trial.tx - expected, 'memory_KiB_after': after,
                      'original_failed_HOLD_unchanged': True, 'no_radio_media_GPIO_or_persistent_write': True,
                      'recipe_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
            with (BASE / 'watchdog-smoke22b01.private-checkpoint.json').open('x') as stream:
                json.dump({'peer': trial.peer, 'start_ticks': trial.start,
                           'tx_bytes': trial.tx, 'rx_bytes': final['rx_bytes'],
                           'boot_id': boot_id}, stream, indent=2)
                stream.write('\n')
            with (EVIDENCE / 'WATCHDOG_SMOKE22B01.json').open('x') as stream:
                json.dump(record, stream, indent=2)
                stream.write('\n')
            session.log('watchdog_smoke_normal_functional_complete', **record)
            print('Watchdog5s timeout/ping/magic-close and sameboot idle PASS; not reset/electrical-stop proof')
        except BaseException:
            with (BASE / 'watchdog-smoke22b01-error.private.txt').open('x') as stream:
                traceback.print_exc(file=stream)
            session.log('HOLD_watchdog_smoke_no_retry_or_second_reset')
            print('Watchdog smoke HOLD; no retry/repair/reset; retain capture and evidence', file=sys.stderr)
            raise


if __name__ == '__main__':
    main()
