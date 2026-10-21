#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Parent-only PC client ownership. No private-file reads or PID-based cancellation."""
from pathlib import Path
import hashlib,os,signal,subprocess,sys
from recipe import Hold,CLIENT_PASS,CLIENT_ASSETS,CLIENT_REVOKE

class OwnedClient:
    def __init__(self,source,source_sha256,popen=subprocess.Popen,pidfd_open=os.pidfd_open,
                 send=signal.pidfd_send_signal,close=os.close,read=os.read,blocking=os.set_blocking):
        self.source=Path(source);self.pin=source_sha256;self.popen=popen;self.open=pidfd_open;self.send=send
        self.close=close;self.read=read;self.blocking=blocking;self.proc=None;self.fd=None;self.pending=bytearray();self.queue=[];self.eof=False
    def preflight(self):
        if self.source.name!='private_client.py' or self.source.is_symlink() or hashlib.sha256(self.source.read_bytes()).hexdigest()!=self.pin:raise Hold('canonical_client_source_pin')
        # Parent's own process only; capability/FD availability check, no proc inspection.
        fd=self.open(os.getpid(),0);self.close(fd)
    def start(self,directory):
        if self.proc is not None:raise Hold('no_second_PC_client')
        argv=[sys.executable,'-B',str(self.source),'--private-directory',directory,'--static-ui-assets','--after-owner-revoke']
        self.proc=self.popen(argv,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                             env={'PATH':'/usr/bin:/bin','LC_ALL':'C'},close_fds=True)
        # Before any poll/wait/reap, the direct owned child cannot be PID-reused.
        # On failure there is deliberately no PID-based signal fallback or PASS.
        try:self.fd=self.open(self.proc.pid,0)
        except OSError:raise Hold('PC_pidfd_open_failed_no_PID_fallback')
        self.blocking(self.proc.stdout.fileno(),False)
    def drain(self):
        if self.proc is None:return
        while not self.eof:
            try:data=self.read(self.proc.stdout.fileno(),512)
            except BlockingIOError:break
            if not data:
                self.eof=True
                if self.pending:raise Hold('client_truncated_line')
                break
            for byte in data:
                if byte!=10:
                    self.pending.append(byte)
                    if len(self.pending)>512:raise Hold('client_line_bound')
                    continue
                line=bytes(self.pending);self.pending.clear()
                if line.endswith(b'\r'):line=line[:-1]
                try:text=line.decode('ascii')
                except UnicodeDecodeError:raise Hold('client_non_scalar_output')
                if text not in (CLIENT_PASS,CLIENT_ASSETS,CLIENT_REVOKE):raise Hold('client_redacted_failure_or_unknown')
                self.queue.append(text)
                if len(self.queue)>3:raise Hold('client_receipt_bound')
    def lines(self):
        self.drain();result=self.queue;self.queue=[];return result
    def poll(self):
        code=self.proc.poll()
        if code is None:return None
        # Drain completed child's pipe before publishing terminal status; a late PASS
        # cannot be lost to the poll-vs-stdout race. Engine consumes queued lines next tick.
        self.drain()
        return code if self.eof and not self.queue else None
    def cancel(self):
        if self.proc is None:return
        if self.fd is None:raise Hold('PC_pidfd_missing_no_PID_fallback')
        try:self.send(self.fd,signal.SIGTERM)
        except ProcessLookupError:pass # exact descriptor already dead; never reacquire a PID
        try:self.proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            try:self.send(self.fd,signal.SIGKILL)
            except ProcessLookupError:pass
            self.proc.wait(timeout=2)
        finally:self.close(self.fd);self.fd=None
    def dispose(self):
        if self.proc is not None and self.proc.poll() is None:self.cancel()
        elif self.fd is not None:self.close(self.fd);self.fd=None
        if self.proc is not None:self.proc.stdout.close()
