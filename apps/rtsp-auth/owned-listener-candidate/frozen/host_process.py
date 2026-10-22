#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""HOST-only direct fixture ownership and bounded pipe reads, under host_guard.

pidfd signaling + actual own wait; never process discovery. This direct helper
does NOT claim descendant retirement: dedicated outer subreaper must attest it.
"""
import hashlib
import os
from pathlib import Path
import select
import signal
import subprocess
import time


class GuardFailure(Exception):
    pass


def require_guard():
    expected = hashlib.sha256(Path(__file__).with_name('host_guard.py').read_bytes()).hexdigest()
    if (os.getuid() == 0 or os.environ.get('RTSP_HOST_GUARD_SHA256') != expected or
            os.getsid(0) != os.getpid()):
        raise GuardFailure('reviewed_HOST_entry_required')


class OwnedProcess:
    def __init__(self, argv, **kwargs):
        self.process = subprocess.Popen(argv, **kwargs)
        self.pidfd = -1
        self.aborted = False
        self.buffer = bytearray()
        try:
            # OWN still-unreaped child, so no PID reuse window before pidfd_open.
            self.pidfd = os.pidfd_open(self.process.pid)
            os.set_blocking(self.process.stdout.fileno(), False)
        except BaseException:
            # No numeric-PID fallback. Outer guarded campaign owns this child
            # even if acquisition failed; failing the campaign triggers cleanup.
            raise GuardFailure('pidfd_setup_failed') from None

    def read_line(self, timeout, limit=4096):
        until = time.monotonic() + timeout
        while time.monotonic() < until:
            if b'\n' in self.buffer:
                line, rest = self.buffer.split(b'\n', 1)
                self.buffer = bytearray(rest)
                return bytes(line).decode('ascii').strip()
            if len(self.buffer) >= limit:
                raise GuardFailure('READY_bound')
            if select.select([self.process.stdout], [], [], max(0, until-time.monotonic()))[0]:
                data = os.read(self.process.stdout.fileno(), min(1024, limit-len(self.buffer)))
                if not data:
                    raise GuardFailure('READY_EOF')
                self.buffer.extend(data)
        raise GuardFailure('READY_deadline')

    def read_end(self, timeout=2, limit=65536):
        until = time.monotonic() + timeout
        while time.monotonic() < until:
            if len(self.buffer) >= limit:
                raise GuardFailure('stdout_bound')
            if select.select([self.process.stdout], [], [], max(0, until-time.monotonic()))[0]:
                data = os.read(self.process.stdout.fileno(), min(4096, limit-len(self.buffer)))
                if not data:
                    result = bytes(self.buffer).decode('ascii')
                    self.buffer.clear()
                    return result
                self.buffer.extend(data)
        raise GuardFailure('stdout_deadline')

    def abort(self):
        """One courtesy TERM/wait only; sole final KILL belongs to outer guard.

        A still-live child remains owned, not declared retired. Failing the
        campaign hands it to the dedicated subreaper for final bounded cleanup.
        Thus no second kill sequence follows a local final-kill uncertainty.
        """
        if self.aborted:
            return
        self.aborted = True
        if self.process.poll() is None:
            try:
                signal.pidfd_send_signal(self.pidfd, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                self.process.wait(timeout=0.3)
            except subprocess.TimeoutExpired:
                pass  # not retirement: close() fails and outer guard must reap

    def close(self):
        try:
            if self.process.poll() is None:
                self.abort()
                raise GuardFailure('fixture_forced_cleanup')
        finally:
            if self.pidfd >= 0:
                os.close(self.pidfd)
                self.pidfd = -1
            for pipe in (self.process.stdin, self.process.stdout):
                if pipe is not None:
                    pipe.close()
