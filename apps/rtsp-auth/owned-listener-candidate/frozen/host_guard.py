#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Linux x86_64 HOST-only, dedicated owned-session/subreaper supervisor.

No discovery, target support or credentials. Workload cannot execute before the
owned-session barrier. An inherited seccomp filter forbids session/group escape.
Keep the session leader UNREAPED through the last group signal; then actually
wait for every adopted descendant until ECHILD. Any adopted child or forced exit
is FAIL, never a normal-retirement PASS. UNKNOWN parks this owner in HOLD without
additional signals/restarts. Do not put an external kill timeout around this owner.
"""
import argparse
import ctypes
import errno
import hashlib
import json
import os
from pathlib import Path
import platform
import resource
import select
import signal
import subprocess
import sys
import time


def prctl(option, value=0):
    libc = ctypes.CDLL(None, use_errno=True)
    if libc.prctl(option, value, 0, 0, 0):
        raise OSError(ctypes.get_errno(), 'host_prctl')


def host_only():
    if (sys.platform != 'linux' or platform.machine() != 'x86_64' or
            os.getuid() == 0 or os.getuid() != os.geteuid() or os.getgid() != os.getegid()):
        raise RuntimeError('nonroot_linux_x86_64_HOST_only')


def forbid_escape():
    """Default-allow HOST lifecycle restriction, NOT the verifier's policy.

    Installed AFTER setsid by Popen, BEFORE the release barrier/exec. It is
    inherited across fork/exec; no_new_privs prevents privilege-based removal.
    Deny setpgid/setsid/unshare/setns and x32/other syscall architectures.
    """
    class Filter(ctypes.Structure):
        _fields_ = [('code', ctypes.c_ushort), ('jt', ctypes.c_ubyte),
                    ('jf', ctypes.c_ubyte), ('k', ctypes.c_uint)]

    class Program(ctypes.Structure):
        _fields_ = [('length', ctypes.c_ushort), ('filter', ctypes.POINTER(Filter))]

    deny = 0x00050000 | errno.EPERM
    rules = [(0x20, 0, 0, 4), (0x15, 1, 0, 0xC000003E), (0x06, 0, 0, 0x80000000),
             (0x20, 0, 0, 0), (0x35, 0, 1, 0x40000000), (0x06, 0, 0, deny)]
    for number in (109, 112, 272, 308):
        rules += [(0x15, 0, 1, number), (0x06, 0, 0, deny)]
    rules.append((0x06, 0, 0, 0x7FFF0000))
    array = (Filter * len(rules))(*(Filter(*row) for row in rules))
    program = Program(len(rules), array)
    prctl(38, 1)  # PR_SET_NO_NEW_PRIVS
    libc = ctypes.CDLL(None, use_errno=True)
    if libc.prctl(22, 2, ctypes.byref(program), 0, 0):  # PR_SET_SECCOMP/FILTER
        raise OSError(ctypes.get_errno(), 'host_escape_filter')


def leader(go, ready, parent, command):
    host_only()
    # If the supervisor vanished before arming, no workload is admitted.
    prctl(1, signal.SIGKILL)  # PR_SET_PDEATHSIG
    if os.getppid() != parent or parent <= 1:
        return 71
    forbid_escape()
    os.write(ready, b'R')
    os.close(ready)
    release = os.read(go, 1)
    os.close(go)
    if release != b'G' or os.getppid() != parent:
        return 72
    env = dict(os.environ)
    env['RTSP_HOST_GUARD_SHA256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    os.execvpe(command[0], command, env)
    return 73


def supervise(command, receipt, deadline, term_grace, reap_grace):
    host_only()
    # This process is dedicated; do not import/call supervise in a busy parent.
    try:
        os.waitpid(-1, os.WNOHANG)
    except ChildProcessError:
        pass
    else:
        raise RuntimeError('dedicated_owner_required')
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    prctl(4, 0)  # non-dumpable
    prctl(36, 1)  # PR_SET_CHILD_SUBREAPER, before fork
    value = ctypes.c_int()
    libc = ctypes.CDLL(None, use_errno=True)
    if libc.prctl(37, ctypes.byref(value), 0, 0, 0) or value.value != 1:
        raise RuntimeError('subreaper_not_observed')
    interrupted = []

    def stop(sig, frame):
        del frame
        if not interrupted:
            interrupted.append(sig)

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGCHLD, signal.SIG_DFL)
    parent = os.getppid()
    prctl(1, signal.SIGTERM)
    if parent <= 1 or os.getppid() != parent:
        interrupted.append(signal.SIGTERM)
    report = {'HOST_only': True, 'supervisor_pid': os.getpid(), 'barrier_released': False,
              'session_owned': False, 'subreaper_observed': True, 'term_attempts': 0,
              'kill_attempts': 0, 'leader_reaped': False, 'adopted_reaped': 0,
              'signaled_reaped': 0, 'all_children_retired': False, 'normal_pass': False}
    # Reserve receipt before spawn; no overwritten evidence or pathname retry.
    with Path(receipt).open('x') as output:
        process = None
        opened = []
        reason = 'setup_failure'
        started = time.monotonic()
        try:
            go_r, go_w = os.pipe2(os.O_CLOEXEC)
            ready_r, ready_w = os.pipe2(os.O_CLOEXEC)
            opened = [go_r, go_w, ready_r, ready_w]
            process = subprocess.Popen(
                [sys.executable, '-B', str(Path(__file__).resolve()), '--leader',
                 str(go_r), str(ready_w), str(os.getpid()), *command],
                stdin=subprocess.DEVNULL, pass_fds=(go_r, ready_w), start_new_session=True)
            report['leader_pid'] = process.pid
            os.close(go_r); opened.remove(go_r)
            os.close(ready_w); opened.remove(ready_w)
            # Unreaped OWN child, not PID lookup/discovery. No workload released yet.
            report['session_owned'] = os.getpgid(process.pid) == os.getsid(process.pid) == process.pid
            if not report['session_owned']:
                raise RuntimeError('session_not_owned')
            until = min(started + deadline, time.monotonic() + 3)
            armed = False
            while not interrupted and time.monotonic() < until:
                if select.select([ready_r], [], [], min(0.02, max(0, until-time.monotonic())))[0]:
                    armed = os.read(ready_r, 2) == b'R'
                    break
            if not armed or interrupted:
                reason = 'barrier_failure'
            else:
                os.write(go_w, b'G')
                report['barrier_released'] = True
                reason = 'deadline'
                while not interrupted and time.monotonic() < started + deadline:
                    # WNOWAIT is essential: no Popen.poll/wait before LAST group signal.
                    observed = os.waitid(os.P_PID, process.pid, os.WEXITED | os.WNOHANG | os.WNOWAIT)
                    if observed is not None:
                        reason = 'leader_exit'
                        break
                    time.sleep(0.01)
                if interrupted:
                    reason = 'owner_interrupted'
        except BaseException as exc:
            report['error_type'] = type(exc).__name__  # never exception data/argv
            reason = 'setup_failure'
        finally:
            for fd in opened:
                try:
                    os.close(fd)  # barrier EOF on every pre-release failure
                except OSError:
                    report['cleanup_error'] = True
            if process is not None and report['session_owned']:
                # One finite courtesy phase, one final kill; leader remains held.
                # Sending to a zombie-only group is not forced *termination*.
                # Any actually adopted/signaled process makes this run FAIL below.
                report['term_attempts'] += 1
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                except OSError:
                    report['cleanup_error'] = True
                until = time.monotonic() + term_grace
                while time.monotonic() < until:
                    time.sleep(max(0, min(0.01, until-time.monotonic())))
                report['kill_attempts'] += 1
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                except OSError:
                    report['cleanup_error'] = True
            # No signals after this point: group ID may cease to be pinned.
            until = time.monotonic() + reap_grace
            while True:
                try:
                    child, status = os.waitpid(-1, os.WNOHANG)
                except ChildProcessError:
                    report['all_children_retired'] = True
                    break
                except InterruptedError:
                    continue
                except OSError:
                    report['cleanup_error'] = True
                    break
                if child:
                    if process is not None and child == process.pid:
                        report['leader_reaped'] = True
                        report['leader_exit'] = os.waitstatus_to_exitcode(status)
                        process.returncode = report['leader_exit']
                    else:
                        report['adopted_reaped'] += 1
                    if os.WIFSIGNALED(status):
                        report['signaled_reaped'] += 1
                    continue
                if time.monotonic() >= until:
                    break
                time.sleep(0.01)
        report['reason'] = reason
        report['normal_pass'] = bool(reason == 'leader_exit' and report['barrier_released'] and
            report['leader_reaped'] and report.get('leader_exit') == 0 and
            report['all_children_retired'] and report['adopted_reaped'] == 0 and
            report['signaled_reaped'] == 0 and not interrupted and not report.get('cleanup_error'))
        report['status'] = ('PASS' if report['normal_pass'] else
                            'FAIL' if report['all_children_retired'] else 'HOLD')
        report['elapsed_ms'] = int((time.monotonic()-started)*1000)
        try:
            json.dump(report, output, indent=2); output.write('\n'); output.flush(); os.fsync(output.fileno())
        except OSError:
            print('HOST_GUARD receipt_io_failure=1 normal_pass=0', flush=True)
            if report['all_children_retired']:
                return 2
            # Cannot abandon the subreaper merely because the receipt store failed.
            while True:
                signal.pause()
    print('HOST_GUARD status=' + report['status'] + ' retired=' +
          str(int(report['all_children_retired'])), flush=True)
    if not report['all_children_retired']:
        # Retain the only subreaper/ownership context; explicit parent disposition
        # required. No repeated signals, wait-as-success, respawn or silent orphan.
        while True:
            signal.pause()
    return 0 if report['normal_pass'] else 2


def main():
    if len(sys.argv) > 1 and sys.argv[1] == '--leader':
        return leader(*map(int, sys.argv[2:5]), sys.argv[5:])
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--receipt', type=Path, required=True)
    p.add_argument('--deadline', type=float, required=True)
    p.add_argument('--term-grace', type=float, default=0.3)
    p.add_argument('--reap-grace', type=float, default=2)
    p.add_argument('command', nargs=argparse.REMAINDER)
    a = p.parse_args()
    command = a.command[1:] if a.command[:1] == ['--'] else a.command
    if not command or not 0.05 <= a.deadline <= 600 or not 0.05 <= a.term_grace <= 2 or not 0.1 <= a.reap_grace <= 5:
        p.error('bounded_HOST_arguments_required')
    return supervise(command, a.receipt, a.deadline, a.term_grace, a.reap_grace)


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except Exception as exc:
        print('HOST_GUARD error_type=' + type(exc).__name__, flush=True)
        raise SystemExit(2)
