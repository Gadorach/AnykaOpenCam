#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Standalone public process dummies ONLY. No RTSP, sockets, images or material."""
import errno
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
from host_process import GuardFailure, OwnedProcess, require_guard


def sleep_forever():
    while True:
        time.sleep(0.05)


def fixture(mode):
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    if mode == 'constructor_failure':
        if os.fork() == 0:
            sleep_forever()
        print('WRONG_READY', flush=True)
    elif mode == 'partial_ready':
        os.write(1, b'DUMMY_REA')  # no newline: former readline hang
    elif mode in ('ignored_sigterm', 'fixture_timeout'):
        print('DUMMY_READY', flush=True)
    elif mode == 'normal_fixture':
        print('DUMMY_READY', flush=True)
        os.read(0, 1)
        return 0
    sleep_forever()  # no_ready, fixture timeout, and ignored TERM


def workload(mode):
    require_guard()
    if mode == 'normal':
        return 0
    if mode == 'escape_rejected':
        for call in (os.setsid, lambda: os.setpgid(0, 0)):
            try:
                call()
            except OSError as exc:
                if exc.errno != errno.EPERM:
                    return 4
            else:
                return 5
        # Also exercise inherited filter in a fork+exec descendant, reaped normally.
        return subprocess.run([sys.executable, '-B', __file__, '--escape-child']).returncode
    if mode == 'late_child':
        def late(sig, frame):
            del sig, frame
            signal.signal(signal.SIGTERM, signal.SIG_IGN)
            if os.fork() == 0:
                sleep_forever()
        signal.signal(signal.SIGTERM, late)
        sleep_forever()
    if mode == 'supervisor_deadline':
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
        if os.fork() == 0:
            sleep_forever()
        sleep_forever()
    if mode == 'retained_uncertainty':
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
        if os.fork() == 0:
            sleep_forever()
        print('DUMMY_HOLD normal_retirement=0', flush=True)
        return 74
    if mode == 'orphan_exit_zero':
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
        if os.fork() == 0:
            sleep_forever()
        return 0  # must NOT turn into guard normal_pass

    class Constructor:
        def __init__(self):
            self.owned = OwnedProcess([sys.executable, '-B', str(Path(__file__).resolve()),
                '--fixture-mode', mode], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL, bufsize=0)
            try:
                line = self.owned.read_line(0.2)
                if line != 'DUMMY_READY':
                    raise GuardFailure('constructor_rejected')
            except BaseException:
                self.owned.abort()
                self.owned.close()
                print('DUMMY constructor_cleanup=1', flush=True)
                raise

    try:
        instance = Constructor()
        if mode == 'normal_fixture':
            instance.owned.process.stdin.write(b'Q')
            instance.owned.process.wait(timeout=1)
            instance.owned.read_end()
            instance.owned.close()
            return 0
        if mode == 'fixture_timeout':
            # A real wait timeout on an actual direct child, not mocked time.
            instance.owned.process.wait(timeout=0.1)
        instance.owned.close()  # bounded courtesy TERM; final KILL is outer guard's
        return 6
    except (GuardFailure, subprocess.TimeoutExpired):
        if 'instance' in locals():
            instance.owned.abort()
            try:
                instance.owned.close()
            except GuardFailure:
                pass  # failure already sticky; final cleanup belongs to guard
        print('DUMMY failure=1', flush=True)
        return 2


def main():
    if sys.argv[1] == '--fixture-mode':
        return fixture(sys.argv[2])
    if sys.argv[1] == '--escape-child':
        for call in (os.setsid, lambda: os.setpgid(0, 0)):
            try:
                call()
            except OSError as exc:
                if exc.errno != errno.EPERM:
                    return 4
            else:
                return 5
        return 0
    return workload(sys.argv[1])


if __name__ == '__main__':
    raise SystemExit(main())
