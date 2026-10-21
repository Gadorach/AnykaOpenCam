#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Validate real included-C CLI receipts. Only x86_64 mock ELF may execute."""
import json
from pathlib import Path
import re
import subprocess
import sys

FIELDS = ('abi', 'flags', 'fault', 'owned', 'white_latch', 'white_pad', 'direction',
          'mux_ok', 'remaining_ms', 'limit_ms', 'prior_pad', 'prior_direction',
          'sequence', 'monotonic_ns')
KEYS = {'action', 'nonce', 'result', 'ioctl_completed', 'command_errno', 'snapshot_errno',
        'transport_errno', 'verification_errno', 'close_errno', 'lease_mode', 'state'}

def main():
    binary = Path(sys.argv[1]).resolve()
    header = binary.read_bytes()[:20]
    if header[:6] != b'\x7fELF\x02\x01' or header[18:20] != b'\x3e\x00':
        raise RuntimeError('refuse non-host ELF')
    r = subprocess.run([str(binary)], capture_output=True, text=True, timeout=20)
    print(r.stdout, end='')
    if r.returncode or r.stderr:
        raise RuntimeError(f'host mock failed: {r.returncode}: {r.stderr}')
    receipts = []
    for line in r.stdout.splitlines():
        if not line.startswith('RECEIPT '):
            continue
        _, name, code, raw = line.split(' ', 3)
        obj = json.loads(raw)
        assert set(obj) == KEYS and len(raw) < 1536
        assert obj['action'] in (None, 'status', 'white-off', 'white-on')
        assert obj['nonce'] in (None, '0123456789abcdef0123456789abcdef')
        assert type(obj['ioctl_completed']) is bool
        assert obj['lease_mode'] == ('self' if obj['action'] == 'white-on' else 'none')
        assert (obj['result'] == 'OK') == (code == '0')
        for key in KEYS:
            if key.endswith('_errno'):
                assert type(obj[key]) is int and obj[key] >= 0
        state = obj['state']
        if state is not None:
            assert set(state) == set(FIELDS)
            for i, key in enumerate(FIELDS):
                assert type(state[key]) is int and 0 <= state[key] < 2 ** (32 if i < 12 else 64)
            if name == 'max-fields':
                assert all(state[k] == 2**32 - 1 for k in FIELDS[:12])
                assert state['sequence'] == state['monotonic_ns'] == 2**64 - 1
            else:
                assert state['sequence'] == 18364758544493064720
                assert state['monotonic_ns'] == 121456789123
                if name == 'distinct-fields':
                    assert [state[k] for k in FIELDS[:12]] == [1, *range(11, 132, 11)]
        if obj['result'] == 'OK':
            assert obj['ioctl_completed'] and state is not None
            assert not any(obj[k] for k in KEYS if k.endswith('_errno'))
        if name in ('self-on', 'self-short-write', 'holder-exit'):
            assert state['flags'] == 7 and state['white_latch'] == state['white_pad'] == 1
            assert 0 < state['remaining_ms'] <= 4900 and obj['lease_mode'] == 'self'
        if name == 'input-high-off':
            assert state['prior_pad'] == 1 and state['prior_direction'] == 0
            assert state['flags'] == 13 and not state['white_pad'] and not state['white_latch']
        if name in ('invalid-on', 'invalid-off'):
            assert obj['ioctl_completed'] and obj['verification_errno'] != 0
        receipts.append(name)
    match = re.search(r'^CLI_MOCK_PASS cases=(\d+) receipts=(\d+) no_real_syscalls_no_forks$', r.stdout, re.M)
    assert match and int(match[2]) == len(receipts) and int(match[1]) == len(receipts) + 5
    assert receipts.count('invalid-on') == receipts.count('invalid-off') == 13
    assert receipts.count('holder-exit') == 6
    print(f'CLI_JSON_PASS receipts={len(receipts)} host_cases={match[1]} actual_uapi_fields=14')

if __name__ == '__main__':
    main()
