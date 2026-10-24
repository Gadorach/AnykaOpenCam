#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Closed supervised driver-foundation01 RAM boot/RO inventory/OFF candidate.

Read CONTRACT.md. Parent-only invocation AFTER independent review. Not a command
interface. No imports/calls/tests of this candidate are authorized by its creation.
"""
import argparse
import ast
import fcntl
import hashlib
import importlib.util
import ipaddress
import os
from pathlib import Path
import re
import socket
import stat
import struct
import subprocess
import sys
import time
import zlib
from urllib.parse import urlsplit

ROOT = Path(__file__).absolute().parents[3]
V4 = ROOT / 'apps/qualification/qc07f-cold-entry23-v4/cold_entry.py'
V4_SHA = '40794530f6ef7bb9fec15104fee5f3c06e841ba6a08947794041bac99b3cc8b5'
BOOT = ROOT / 'scripts/qc07f_boot.py'
BOOT_SHA = '6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985'
LOADER = ROOT / 'work/lightbulb-camera/implementation-20261008T205404Z/artifacts/uart-ram-loader-wordaligned.bin'
LOADER_SHA = 'a1630c90f7f44666e941704778cbf119e5e2e13355113adca52f842bd9046ad1'
RELEASE = '6.18.55-qc07f-driver-foundation01'
PROFILE = 'qc07f-driver-foundation01'
DTB_SHA = 'd837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5'
PTZ_SHA = 'a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87'
BOOTARGS = 'earlycon console=ttyAK0,115200n8 rdinit=/init loglevel=8 panic=0 cma=16M@0x82800000-0x83800000'
TOTAL, TRANSITION, RECOVERY = 5400, 30, 120
DEST, CEILING = 0x82408000, 0x82800000
PROMPT = b'qc07f> '
NEUTRAL = ((0x08000074, 0x02000006, 0xffffffff),
           (0x08000078, 0, 0xffffffff), (0x0800007c, 0, 0xffffffff),
           (0x20170000, 0, 0xffffffff), (0x2017000c, 0, 0xffffffff),
           (0x20170018, 0, 0x8000c0), (0x080000c8, 0, 0xffffffff),
           (0x080000d0, 0, 0xffffffff), (0x080000e8, 0, 1))
QUERIES = (
    ('boot_id', 'cat /proc/sys/kernel/random/boot_id'),
    ('pid1', 'cat /proc/1/comm'), ('identity', 'cat /proc/1/status'),
    ('release', 'cat /proc/sys/kernel/osrelease'), ('cmdline', 'cmdline'),
    ('modules', 'cat /proc/modules'), ('mounts', 'cat /proc/mounts'),
    ('memory', 'mem'), ('iomem', 'iomem'), ('ISP', 'isp state'),
    ('watchdog', 'cat /sys/class/watchdog/watchdog0/state'),
    ('nowayout', 'cat /sys/class/watchdog/watchdog0/nowayout'),
    ('UART', 'cat /sys/class/tty/ttyAK0/rx_state'),
    ('ptz0', 'cat /sys/bus/platform/devices/soc:ptz-axis0/state'),
    ('ptz1', 'cat /sys/bus/platform/devices/soc:ptz-axis1/state'),
    ('boot_id_after', 'cat /proc/sys/kernel/random/boot_id'))
# Exact source-defined help records, RECEIVED only, never a TX allowlist. Optional
# compile-time groups retain their source order; unknown/unframed lines refuse.
MONITOR_HELP = (
    'QC07F experimental mainline qualification monitor',
    'help | uptime | mem | irqs | cpu | cmdline | iomem | cat /proc/FILE',
    'No flash/persistent writes; explicit hardware qualification operations below.',
    'gpio info: read-only chip/line inspection (no requests or values).',
    'wdt smoke: explicit5s timeout/ping/magic-close register test.',
    'wdt reset: explicit nominal1s reset; arm recovery capture first.',
    'sensor id: fixed address0x30 registers3107/3108; no sensor value writes or scans.',
    'sensor state | sensor init (exact139-pair table) | sensor start | sensor stop',
    'isp capture (bounded2s DMA test) | isp state | frame head | frame dump',
    'frame roi: exact512x256 luma crop x256/y448, with transfer CRC',
    'frame anchor: exact512x256 luma crop x768/y448, with transfer CRC',
    'usb info: onboard device descriptors/sysfs only; no radio firmware/network credentials',
    'radio info: guarded read-only firmware-info command43; no downloads/association',
    'radio state: retail keyed boot-state query only; no downloads/flash/OTP',
    'radio repeat: four sequenced state queries to qualify repeated bulk exchange',
    'radio resetrepeat: same queries with standard endpoint-toggle reset before each',
    'radio load: exact chip8400 retail RAM-only load, state/security/CRC guarded; no SPI/OTA/OTP',
    'radio native-load: same image/guards,2048-byte fragments without endpoint resets; tests corrected HCD',
    'ptz0 + | ptz0 - | ptz1 + | ptz1 - : bounded4-step pulse,5s guard; no homing',
    'wifi attach: explicit audited station-only module; verify RAM firmware first',
    'shell: RAM-only BusyBox shell; exit returns to monitor; no credentials in built image')


def require(ok, message='Closed driver profile refused; HOLD'):
    if not ok:
        raise ValueError(message)


def pinned_helpers():
    """Only exact pinned definitions, never v4.main or an old Boot instance."""
    require(V4.resolve(strict=True) == V4)
    data = V4.read_bytes()
    require(hashlib.sha256(data).hexdigest() == V4_SHA)
    tree = ast.parse(data)
    names = {'require', 'strict_json', 'identity', 'tty_association', 'ancestry',
             'protected', 'Retained', 'new_json', 'rpc', 'ticks', 'fuser',
             'http_power', 'observe'}
    constants = {'ROOT', 'RECORDER', 'BOOT', 'CLASSIFIER', 'PINS', 'CONTROL', 'SOCKET', 'PACKET'}
    nodes = [n for n in tree.body if isinstance(n, (ast.Import, ast.ImportFrom)) or
             (isinstance(n, (ast.FunctionDef, ast.ClassDef)) and n.name in names) or
             (isinstance(n, ast.Assign) and len(n.targets) == 1 and
              isinstance(n.targets[0], ast.Name) and n.targets[0].id in constants)]
    require({n.name for n in nodes if isinstance(n, (ast.FunctionDef, ast.ClassDef))} == names)
    namespace = {'__file__': str(V4), '__name__': '_entry24_exact_v4_definitions'}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), str(V4), 'exec'), namespace)
    return namespace


class Entry:
    def __init__(self, args, helpers):
        self.a, self.h = args, helpers
        self.retained, self.errors, self.sources = [], [], []
        self.child = self.start = self.peer = self.argv = None
        self.out = self.out_chain = self.private_chain = None
        self.realport = self.port_meta = self.control_chain = None
        self.on_attempted = self.off_attempted = self.off_proved = False
        self.initial_off = self.observed = self.final_ok = False
        self.phase, self.failed_phase = 'preflight', None
        self.hold = None
        self.power = {'initial_GET': None, 'ON_POST_ack': False, 'ON_GET': None,
                      'OFF_POST_ack': False, 'OFF_GET': None}
        self.cursor = self.tx = 0
        self.raw_fd = self.raw_meta = None
        self.classified = hashlib.sha256()
        self.sent = []
        self.cold_events = self.cold_raw = None
        self.cold_count = 0
        self.cap_m = self.cap_b = None
        self.offsets, self.previous = None, None
        self.stage_end = None
        self.io_end = None
        self.clock(0, 0)  # Original cap begins BEFORE even preflight/child creation.
        self.origin = self.previous[0]

    def fail(self, message):
        if self.hold is None:
            self.hold = message
            self.failed_phase = self.phase
        raise ValueError(self.hold)

    def clock(self, cost=0, reserve=TRANSITION + RECOVERY):
        if self.hold is not None:
            self.fail(self.hold)
        m0 = time.monotonic_ns()
        b = time.clock_gettime_ns(time.CLOCK_BOOTTIME)
        r = time.time_ns()
        m1 = time.monotonic_ns()
        if m1 < m0:
            self.fail('MONOTONIC bracket reversed')
        intervals = ((b - m1, b - m0), (r - m1, r - m0))
        if self.previous is not None:
            pm, pb, pr = self.previous
            if m0 < pm or b < pb or r < pr:
                self.fail('Clock order reversed')
            intervals = tuple((max(old[0], new[0]), min(old[1], new[1]))
                              for old, new in zip(self.offsets, intervals))
            if any(lo > hi for lo, hi in intervals):
                self.fail('Clock offset intervals disjoint; supervised cutoff required')
        self.offsets, self.previous = intervals, (m1, b, r)
        if self.cap_m is None:
            self.cap_m = m0 + TOTAL * 10**9
            self.cap_b = b - (m1 - m0) + TOTAL * 10**9
        if self.start is not None:
            # /proc start uses BOOTTIME. Round DOWN to an earlier nanosecond bound.
            child_cap = self.start * 10**9 // os.sysconf('SC_CLK_TCK') + TOTAL * 10**9
            self.cap_b = min(self.cap_b, child_cap)
        self.cap_m = min(self.cap_m, self.cap_b - intervals[0][1])
        end = min(self.cap_m, self.stage_end or self.cap_m)
        margin = int((cost + reserve) * 10**9)
        if m1 + margin >= end or m1 + intervals[0][1] + margin >= self.cap_b:
            self.fail('Original deadline/reserve exhausted; no further UART')
        remaining = (end - m1) / 10**9 - reserve
        if self.io_end is not None:
            if m1 + int(cost * 10**9) >= self.io_end:
                self.fail('Original operation deadline exhausted')
            remaining = min(remaining, (self.io_end - m1) / 10**9)
        return remaining

    def step(self, function, *args, cost=3, **kwargs):
        cost = min(cost, self.clock() - .001)
        require(cost > 0)
        self.clock(cost)
        before = time.monotonic_ns()
        try:
            value = function(*args, **kwargs)
        except BaseException as exc:
            self.fail('Blocking operation failed: ' + repr(exc))
        self.clock()
        if time.monotonic_ns() - before > int(cost * 10**9):
            self.fail('Blocking operation exceeded admitted local cost; no more UART')
        return value

    def _control(self, request, recovery=False):
        """v4 peer/metadata/JSON guards, with ONE nonrenewing RPC deadline.

        Unlike the original helper's per-recv timeout, clamp every socket action
        to this operation and the first whole-run cap. Recovery permits only stop.
        """
        require(request.get('action') in {'status', 'arm_abort', 'send_protected', 'stop'})
        if recovery:
            require(request == {'action': 'stop'} and self.off_proved)
        remaining = 3 if recovery else min(3, self.clock())
        end = time.monotonic_ns() + int(remaining * 10**9)
        meta = self.h['SOCKET'].lstat()
        require(stat.S_ISSOCK(meta.st_mode) and meta.st_uid == os.getuid() and
                stat.S_IMODE(meta.st_mode) == 0o600 and meta.st_nlink == 1)
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
            def timeout():
                left = (end - time.monotonic_ns()) / 10**9
                if not recovery:
                    left = min(left, self.clock())
                require(left > 0)
                connection.settimeout(left)
            timeout()
            connection.connect(str(self.h['SOCKET']))
            timeout()
            actual = struct.unpack('3i', connection.getsockopt(socket.SOL_SOCKET, socket.SO_PEERCRED, 12))
            mismatch = actual != self.peer
            timeout()
            connection.sendall(self.h['json'].dumps({'action': 'status'} if mismatch else request).encode() + b'\n')
            data = bytearray()
            while True:
                timeout()
                chunk = connection.recv(4096)
                if not chunk:
                    break
                data.extend(chunk)
                require(len(data) <= 65536)
            require(time.monotonic_ns() < end and not mismatch)
        if not recovery:
            self.clock()
        answer = self.h['strict_json'](data)
        require(type(answer) is dict and 'error' not in answer)
        return answer

    def sole_owner(self):
        timeout = min(3, self.clock() - .001)
        require(timeout > 0)
        result = subprocess.run(['/usr/bin/fuser', str(self.realport)], capture_output=True, timeout=timeout)
        self.clock()
        require((result.returncode, result.stdout.split()) == (0, [str(self.child.pid).encode()]))

    def delay(self, seconds):
        # Scheduling overhead is bounded by admission, not falsely zero. This is
        # not an offset tolerance and never changes an original deadline.
        self.clock(seconds + .25)
        self.step(time.sleep, seconds, cost=seconds + .25)

    def power_call(self, key, action=None):
        self.clock(3)
        before = time.monotonic_ns()
        try:
            value = self.h['http_power'](self.endpoint, action)
        except BaseException as exc:
            self.fail('Power operation uncertain: ' + repr(exc))
        # Preserve the actual GET Boolean BEFORE any post-cost/clock rejection.
        self.power[key] = value if action is None else True
        self.clock()
        if time.monotonic_ns() - before > 3 * 10**9:
            self.fail('Power operation exceeded original local bound')
        return value

    def keep(self, path, mode=None, limit=100000, source=False):
        obj = self.h['Retained'](path, mode, limit)
        self.retained.append(obj)
        if source:
            self.sources.append(obj)
        return obj

    def public_record(self, record, extra=False):
        require(type(record) is dict and set(record) ==
                ({'path', 'sha256', 'bytes', 'crc32_bzip2'} if extra else {'path', 'sha256', 'bytes'}))
        require(type(record['path']) is str and type(record['bytes']) is int and
                0 < record['bytes'] <= 32 * 1024 * 1024 and
                type(record['sha256']) is str and re.fullmatch('[0-9a-f]{64}', record['sha256']))
        path = Path(record['path'])
        require(str(path) == record['path'] and ROOT in path.parents and
                '.git' not in path.parts and path.resolve(strict=True) == path)
        obj = self.keep(path, limit=record['bytes'])
        require(len(obj.data) == record['bytes'] and obj.digest == record['sha256'])
        return obj

    def select_image(self):
        selection = self.keep(Path(self.a.selection), limit=32768)
        require(re.fullmatch('[0-9a-f]{64}', self.a.selection_sha256) and
                selection.digest == self.a.selection_sha256)
        require(ROOT in selection.path.parents and '.git' not in selection.path.parts)
        value = self.h['strict_json'](selection.data)
        require(type(value) is dict and set(value) == {'profile', 'image', 'kernel_release',
                'dtb', 'ptz_source', 'metadata', 'build_review', 'parent_context'})
        require(value['profile'] == PROFILE and value['kernel_release'] == RELEASE)
        image = self.public_record(value['image'], True)
        dtb = self.public_record(value['dtb'])
        ptz = self.public_record(value['ptz_source'])
        metadata = self.public_record(value['metadata'])
        self.public_record(value['build_review'])
        self.public_record(value['parent_context'])
        require(dtb.digest == DTB_SHA and ptz.digest == PTZ_SHA)
        require(self.h['strict_json'](metadata.data) == {k: v for k, v in value.items() if k != 'metadata'})
        data = image.data
        require(64 <= len(data) <= 0x400000 and DEST + len(data) < CEILING and data.endswith(dtb.data))
        h = struct.unpack('>7I4B32s', data[:64])
        header = bytearray(data[:64]); header[4:8] = b'\0' * 4
        require(h[0] == 0x27051956 and h[3] == len(data) - 64 and
                h[4:6] == (DEST, DEST + 64) and tuple(h[7:11]) == (5, 2, 2, 0) and
                zlib.crc32(header) & 0xffffffff == h[1] and
                zlib.crc32(data[64:]) & 0xffffffff == h[6])
        self.image_name = h[11].split(b'\0', 1)[0].decode('ascii')
        require(self.image_name and all(32 <= c <= 126 for c in self.image_name.encode()))
        crc = value['image']['crc32_bzip2']
        require(type(crc) is str and re.fullmatch('[0-9a-f]{8}', crc) and
                self.crc(data) == int(crc, 16))
        self.image, self.image_info = image, value['image']
        self.data, self.image_crc = data, crc

    def identity(self):
        require(self.child is not None and self.child.poll() is None and self.start is not None and
                self.h['ticks'](self.child.pid) == self.start)
        proc = Path('/proc') / str(self.child.pid)
        require((proc / 'cmdline').read_bytes().split(b'\0') == [os.fsencode(x) for x in self.argv] + [b''] and
                (proc / 'exe').resolve() == Path(sys.executable).resolve() and
                (proc / 'cwd').resolve() == ROOT and
                Path(self.a.port).resolve(strict=True) == self.realport and
                self.h['tty_association'](self.realport.stat()) == self.port_meta)

    def state(self):
        self.step(self.identity, cost=.1)
        value = self.step(self._control, {'action': 'status'})
        require(value.get('private_capture_v1') is True and value.get('protected_send_v1') is True and
                value.get('conditional_send_v1') is True and
                value.get('capture_source_sha256') == self.h['PINS'][self.h['RECORDER']] and
                value.get('part') == self.a.part and value.get('adapter') == self.a.port and
                value.get('session') == value.get('private_capture_dir') == str(self.capture) and
                value.get('raw_path') == str(self.rawpath) and value.get('recent_rx') == '' and
                value.get('tx_busy') is False and value.get('read_only_inspection_active') is False and
                type(value.get('rx_bytes')) is int and type(value.get('tx_bytes')) is int)
        self.step(self.sole_owner)
        return value

    def snapshot(self, path, limit):
        obj = self.h['Retained'](path, 0o600, limit)
        try:
            require(self.h['protected'](self.capture))
            return obj.data
        finally:
            os.close(obj.fd)

    def events(self):
        return [self.h['strict_json'](line) for line in self.snapshot(self.eventpath, 64 * 1024 * 1024).splitlines()]

    def fresh(self):
        value = self.state()
        require(value['tx_bytes'] == self.tx and value['rx_bytes'] >= self.cursor and
                value['abort_armed'] is self.reset_arm)
        # Retain original offset0 forever; cursor only marks fully classified bytes.
        # This file is LIVE append-only output, not an immutable Retained input.
        # Read ONLY the status-advertised prefix; a later append is NOT discarded
        # or adopted into the watermark. CAS rejects it before any next TX.
        require(self.raw_fd is not None and self.h['protected'](self.capture))
        fdstat, named = os.fstat(self.raw_fd), self.rawpath.lstat()
        meta = lambda s: (s.st_dev, s.st_ino, s.st_uid, s.st_gid, s.st_mode, s.st_nlink)
        require(meta(fdstat) == meta(named) == self.raw_meta and
                self.cursor <= value['rx_bytes'] <= fdstat.st_size <= 4 * 1024 * 1024)
        data = os.pread(self.raw_fd, value['rx_bytes'] - self.cursor, self.cursor)
        require(len(data) == value['rx_bytes'] - self.cursor)
        if b'U-Boot' in data or b'QCL_NAK' in data or b'QCL_BAD_' in data or b'QCL_BUSY_' in data:
            self.fail('Unexpected reset/loader rejection; no retry')
        if self.phase == 'fixed_RO_inventory' and (b'Linux version ' in data or b'Starting kernel' in data or
                re.search(rb'(?m)^\[[ \t]*\d+\.\d+\]', data)):
            self.fail('Asynchronous kernel/reset output during RO query; no refresh')
        self.clock()
        return value, data

    def _transmit(self, data):
        require(type(data) is bytes and 0 < len(data) <= 128)
        state, extra = self.fresh()
        require(not extra, 'Unclassified RX before TX; no watermark repair')
        for obj in self.sources:
            self.step(obj.check, cost=.1)
        self.clock(3)
        answer = self.step(self._control, {'action': 'send_protected', 'hex': data.hex(),
                           'expected_rx_bytes': state['rx_bytes']})
        require(answer.get('not_sent') is not True and answer.get('transmitted_bytes') == len(data),
                'CAS/partial TX refused; STOP without retry')
        self.tx += len(data)
        self.sent.append(len(data))

    def exact(self, expected, timeout):
        """Whole ordered response, including co-arriving last ACK+DONE. No skips."""
        self.clock(timeout)
        end = min(time.monotonic_ns() + int(timeout * 10**9), self.cap_m - (TRANSITION + RECOVERY) * 10**9,
                  (self.stage_end or self.cap_m) - (TRANSITION + RECOVERY) * 10**9)
        self.io_end = end
        try:
            while True:
                state, data = self.fresh()
                self.clock()
                require(expected.startswith(data), 'Unexpected/duplicate RX record')
                if data == expected:
                    self.accept(state, data)  # ONCE, only after full classification.
                    return data
                require(time.monotonic_ns() < end, 'Incomplete/timeout RX; no retry')
                self.delay(.005)
        finally:
            self.io_end = None

    def echo(self, expected):
        self.clock(3)
        end = min(time.monotonic_ns() + 3 * 10**9, self.cap_m - 150 * 10**9,
                  (self.stage_end or self.cap_m) - 150 * 10**9)
        self.io_end = end
        try:
            while True:
                state, data = self.fresh()
                self.clock()
                # Original audited Boot.command permits console line wrapping only:
                # CR+LF pairs, not arbitrary whitespace/prefix removal or repair.
                normalized = re.sub(rb'\r+\n', b'', data)
                incomplete_wrap = normalized.endswith(b'\r')
                prefix = normalized.rstrip(b'\r') if incomplete_wrap else normalized
                require(expected.startswith(prefix), 'Echo differs; execution CR withheld')
                if normalized == expected:
                    self.accept(state, data)
                    return
                require(time.monotonic_ns() < end, 'Incomplete echo; execution CR withheld')
                self.delay(.005)
        finally:
            self.io_end = None

    def response(self, timeout, prompt=b'anyka$'):
        self.clock(timeout)
        end = min(time.monotonic_ns() + int(timeout * 10**9), self.cap_m - (TRANSITION + RECOVERY) * 10**9,
                  (self.stage_end or self.cap_m) - (TRANSITION + RECOVERY) * 10**9)
        self.io_end = end
        try:
            while True:
                state, data = self.fresh()
                self.clock()
                require(len(data) <= 262144 and all(c in (9, 10, 13) or 32 <= c <= 126 for c in data))
                require(not re.search(rb'(?i)unknown command|open failed|read failed|kernel panic|oops:|BUG:|QCL_TRANSFER_REJECTED', data))
                if prompt in data:
                    require(data.endswith(prompt) and data.count(prompt) == 1)
                    # Caller validates before advancing the original logical cursor.
                    return state, data
                require(time.monotonic_ns() < end, 'No fresh completed response')
                self.delay(.005)
        finally:
            self.io_end = None

    def _text(self, command, timeout=10, expected=None, kind='prompt'):
        """Private internal transport; caller consumes a preconstructed closed plan."""
        require(self.plan_index < len(self.plan) and command == self.plan[self.plan_index])
        self.plan_index += 1
        wire = command.encode('ascii')
        require(wire and len(wire) <= 256 and not any(c in wire for c in (0, 10, 13)))
        for offset in range(0, len(wire), 64):
            piece = wire[offset:offset + 64]
            self._transmit(piece)
            # Classify each complete echo before any further piece, never assume it.
            self.echo(piece)
            if offset + 64 < len(wire):
                self.delay(.025)
        self._transmit(b'\r')
        if kind == 'ready':
            # Actual U-Boot go prints its own invocation before the loader READY.
            state, data = self.response(timeout, b'QCL_READY_1 RAM_ONLY\r\n')
            normal = data.replace(b'\r', b'')
            require(re.fullmatch(rb'\n## Starting application at 0x82001000 \.\.\.\n\nQCL_READY_1 RAM_ONLY\n', normal))
        elif kind == 'boot':
            state, data = self.response(timeout, PROMPT)
            require(data.count(b'Starting kernel ...') == 1 and
                    data.count(('Linux version ' + RELEASE + ' ').encode()) == 1 and
                    data.count(b'QC07F experimental mainline qualification monitor') == 1 and
                    b'anyka$' not in data and b'Linux version ' in data)
            # Boot-only bounded ASCII transcript is retained, not a healthy-driver assertion.
            require(b'USB qualification startup watchdog magic-close completed' in data)
            self.classify_boot(data)
        else:
            state, data = self.response(timeout)
            if expected is not None:
                require(data.replace(b'\r', b'').strip() == expected)
        self.clock()
        return state, data

    def classify_boot(self, data):
        """Closed boot envelopes + timestamp-framed kernel log data + exact help.

        Kernel message contents are observational data, NOT healthy-driver proof.
        Unframed/unknown console text, duplicate transitions/help and later printk
        during RO queries are not silently attributed to boot or refreshed away.
        """
        lines = data[:-len(PROMPT)].replace(b'\r', b'').decode('ascii').split('\n')
        phase, help_positions, starting, linux = 'uboot', [], 0, 0
        header_seen = set()
        fixed = {'## Booting kernel from Legacy Image at 82408000 ...',
                 '   Image Name:   ' + self.image_name,
                 '   Image Type:   ARM Linux Kernel Image (uncompressed)',
                 '   Load Address: 82408000', '   Entry Point:  82408040',
                 '   Verifying Checksum ... OK', '   Loading Kernel Image ... OK', 'OK'}
        for line in lines:
            if not line:
                continue
            if phase == 'uboot':
                if line == 'Starting kernel ...':
                    starting += 1
                    phase = 'kernel'
                elif line in fixed:
                    require(line not in header_seen, 'Duplicate U-Boot handoff record')
                    header_seen.add(line)
                    continue
                else:
                    size = re.fullmatch(r'   Data Size:    (\d+) Bytes = [0-9.]+ [kMGT]?i?B', line)
                    require(size and int(size[1]) == len(self.data) - 64, 'Unknown U-Boot handoff record')
                    require('data_size' not in header_seen, 'Duplicate image-size record')
                    header_seen.add('data_size')
            elif phase == 'kernel':
                if line == 'Uncompressing Linux... done, booting the kernel.':
                    continue
                if line == 'USB qualification startup watchdog magic-close completed':
                    phase = 'help'
                    continue
                require(re.fullmatch(r'\[[ \t]*\d+\.\d+\] .*', line), 'Unframed boot RX')
                linux += int(('Linux version ' + RELEASE + ' ') in line)
            else:
                require(line in MONITOR_HELP, 'Unknown userspace startup RX')
                help_positions.append(MONITOR_HELP.index(line))
        require(phase == 'help' and starting == linux == 1 and
                help_positions[:3] == [0, 1, 2] and
                help_positions == sorted(set(help_positions)))

    def accept(self, state, data=None):
        require(state['rx_bytes'] >= self.cursor)
        if data is None:
            data = os.pread(self.raw_fd, state['rx_bytes'] - self.cursor, self.cursor)
        require(len(data) == state['rx_bytes'] - self.cursor)
        self.classified.update(data)
        self.cursor = state['rx_bytes']

    def command(self, command, expected=b'anyka$', timeout=10):
        state, data = self._text(command, timeout, expected)
        self.accept(state, data)
        return data

    def crc_command(self, address, size, expected):
        state, data = self._text(f'crc32 {address:#x} {size:#x}')
        require(self.crc_verified(data, address, size, expected))
        require(re.fullmatch(rb'\nCRC32 for (?:0x)?[0-9a-fA-F]+ \.\.\. (?:0x)?[0-9a-fA-F]+ ==> [0-9a-fA-F]{8}\n\nanyka\$',
                             data.replace(b'\r', b'')))
        self.accept(state, data)

    def arm(self):
        answer = self.step(self._control, {'action': 'arm_abort', 'mode': 'hold-cli',
                           'volatile_cli_hold': True, 'inspect': False})
        require(answer.get('armed') is True and answer.get('mode') == 'hold-cli' and
                answer.get('persistent_write') is False and answer.get('volatile_bootargs') is False and
                answer.get('read_only_commands') == [])

    def prepare_plan(self):
        self.plan = ['sf probe 0:0 20000000 0', 'mw.b 0x82408000 0 0x60000',
                     'crc32 0x82408000 0x60000', 'sf read 0x82408000 0x0 0x60000',
                     'crc32 0x82408000 0x60000']
        self.plan += [f'md.l {address:#x} 1' for address, _, _ in NEUTRAL]
        self.plan += [f'mw.l {0x82001000 + off:#x} {struct.unpack_from("<I", self.loader, off)[0]:#010x} 1'
                      for off in range(0, 1872, 4)]
        self.plan += ['crc32 0x82001000 0x750', 'go 0x82001000',
                      f'crc32 0x82408000 {len(self.data):#x}',
                      'mw.l 0x80000e4 0x552aea54 1', 'mw.l 0x80000e8 0xaa000003 1',
                      "setenv bootargs '" + BOOTARGS + "'", 'printenv bootargs', 'bootm 0x82408000']
        self.plan_index = 0

    def boot_inventory(self):
        self.phase = 'bootloader_neutral_loader_staging'
        self.stage_end = min(self.cap_m, self.origin + 1500 * 10**9)
        self.command('sf probe 0:0 20000000 0', b'anyka spi flash. bus_width:1\n\nanyka$')
        self.command('mw.b 0x82408000 0 0x60000')
        self.crc_command(DEST, 0x60000, f'{self.crc(bytes(0x60000)):08x}')
        state, data = self._text('sf read 0x82408000 0x0 0x60000', 20)
        require(re.fullmatch(rb'\nSF: 393216 bytes @ 0x0 Read: OK\n\nanyka\$', data.replace(b'\r', b'')))
        self.accept(state)
        self.crc_command(DEST, 0x60000, '452a21fe')
        for address, want, mask in NEUTRAL:
            state, data = self._text(f'md.l {address:#x} 1')
            rows = re.findall(rb'(?m)^0?' + f'{address:x}'.encode() + rb':\s+([0-9a-fA-F]{8})(?:[ \t]+[^\r\n]*)?\r?$', data)
            require(len(rows) == 1 and int(rows[0], 16) & mask == want)
            require(len(data.replace(b'\r', b'').strip().splitlines()) == 3)
            self.accept(state)
        for off in range(0, 1872, 4):
            self.command(f'mw.l {0x82001000 + off:#x} {struct.unpack_from("<I", self.loader, off)[0]:#010x} 1')
        self.crc_command(0x82001000, 1872, 'dd0572e1')
        for obj in self.retained:
            self.step(obj.check, cost=3)
        # The initial catch is CONSUMED. Explicit one future-reset catch, no raw reset.
        require(self.state()['abort_armed'] is False)
        self.arm()
        self.reset_arm = True
        require(self.state()['abort_armed'] is True)
        self.phase = 'one_QCL_transfer'
        self.stage_end = min(self.cap_m, self.origin + 4800 * 10**9)
        # Nominal .06/frame +60s, not measured timing/prevention proof.
        planned = 60 + ((len(self.data) + 95) // 96) * .06
        self.clock(planned)
        state, _ = self._text('go 0x82001000', 10, kind='ready')
        self.accept(state)
        self._transmit(b'QCL1' + struct.pack('<II', len(self.data), int(self.image_crc, 16)))
        self.exact(f'QCL_BEGIN {len(self.data):08x}\r\n'.encode(), 3)
        for seq, off in enumerate(range(0, len(self.data), 96)):
            chunk = self.data[off:off + 96]
            self._transmit(struct.pack('<III', seq, len(chunk), self.crc(chunk)) + chunk)
            self.delay(.015)
            expected = f'QCL_ACK {seq:08x}\r\n'.encode()
            if off + len(chunk) == len(self.data):
                expected += f'QCL_DONE {len(self.data):08x} {self.image_crc}\r\n'.encode()
            self.exact(expected, 3 if off + len(chunk) == len(self.data) else 1)
        self._transmit(b'QEND')
        # U-Boot prints the application's return code before the held prompt.
        state, data = self.response(3)
        require(re.fullmatch(rb'QCL_COMMIT_OK\r\n(?:## Application terminated, rc = 0x0\r\n)?(?:\r\n)?anyka\$', data))
        self.accept(state, data)
        self.phase = 'independent_RAM_CRC_and_boot'
        self.stage_end = min(self.cap_m, self.origin + 5100 * 10**9)
        self.crc_command(DEST, len(self.data), self.image_crc)
        for obj in self.retained:
            self.step(obj.check, cost=3)
        self.command('mw.l 0x80000e4 0x552aea54 1')
        self.command('mw.l 0x80000e8 0xaa000003 1')
        self.command("setenv bootargs '" + BOOTARGS + "'")
        state, data = self._text('printenv bootargs')
        require(data.replace(b'\r', b'').strip() == ('bootargs=' + BOOTARGS + '\n\nanyka$').encode())
        self.accept(state)
        state, bootlog = self._text('bootm 0x82408000', 45, kind='boot')
        self.accept(state)
        require(self.plan_index == len(self.plan))
        self.h['new_json'](self.out / 'boot.private.json', {'rx_end': self.cursor,
                             'boot_log': bootlog.decode('ascii'), 'startup_message_is_not_state_ACK': True})
        self.phase = 'fixed_RO_inventory'
        rows = {}
        for key, command in QUERIES:
            wire = command.encode() + b'\r'
            require(len(wire) <= 64)
            self._transmit(wire)
            state, data = self.response(6, PROMPT)
            raw = data[:-len(PROMPT)].replace(b'\r', b'')
            lines = raw.split(b'\n')
            while lines and lines[0] == b'':
                lines.pop(0)
            if lines and lines[0] == command.encode():
                lines.pop(0)  # Exact optional TTY echo, never a completion witness.
            while lines and lines[-1] == b'':
                lines.pop()
            rows[key] = b'\n'.join(lines).decode('ascii')
            self.validate_row(key, rows[key], rows)
            self.accept(state)
        self.h['new_json'](self.out / 'inventory.private.json', {'rows': rows,
                           'complete_process_FD_inventory': False, 'native_DMA_exclusivity_proved': False,
                           'next_phase_admission': False, 'PTZ_OFF_physical_proof': False})
        _, extra = self.fresh()
        require(not extra)
        self.observed = True

    def validate_row(self, key, value, rows):
        require(not re.search(r'(?i)unknown command|open failed|read failed|not found|no such file|permission denied', value))
        if key in ('boot_id', 'boot_id_after'):
            require(re.fullmatch(r'[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}', value))
            if key == 'boot_id_after':
                require(value == rows['boot_id'])
        elif key == 'pid1':
            require(value == 'init')
        elif key == 'identity':
            require(re.findall(r'(?m)^Uid:[ \t]+(\d+)[ \t]+(\d+)[ \t]+(\d+)[ \t]+(\d+)[ \t]*$', value) == [('0', '0', '0', '0')] and
                    re.findall(r'(?m)^Pid:[ \t]+(\d+)[ \t]*$', value) == ['1'])
            cap = re.findall(r'(?m)^CapEff:[ \t]+([0-9a-f]{16})[ \t]*$', value)
            require(len(cap) == 1 and int(cap[0], 16) & (1 << 17))
        elif key == 'release':
            require(value == RELEASE)
        elif key == 'cmdline':
            require(value == BOOTARGS)
        elif key == 'modules':
            require(value == '')
        elif key == 'mounts':
            mounts = {}
            for line in value.splitlines():
                p = line.split()
                require(len(p) == 6 and p[1] not in mounts and p[4:] == ['0', '0'] and
                        'rw' in p[3].split(',') and 'ro' not in p[3].split(','))
                mounts[p[1]] = (p[0], p[2])
            require(set(mounts) == {'/', '/dev', '/proc', '/sys'} and
                    mounts['/'][0] in {'rootfs', 'ramfs', 'tmpfs', 'none'} and
                    mounts['/'][1] in {'rootfs', 'ramfs', 'tmpfs'} and
                    mounts['/dev'] == ('devtmpfs', 'devtmpfs') and
                    mounts['/proc'] == ('proc', 'proc') and mounts['/sys'] == ('sysfs', 'sysfs'))
        elif key == 'memory':
            pairs = re.findall(r'(?m)^(MemTotal|MemFree|CmaTotal|CmaFree):[ \t]+(\d+) kB$', value)
            require(len(pairs) == 4 and len(dict(pairs)) == 4)
            mem = {k: int(v) for k, v in pairs}
            require(24000 < mem['MemTotal'] <= 40960 and mem['CmaTotal'] == 16384 and
                    mem['MemFree'] <= mem['MemTotal'] and mem['CmaFree'] <= mem['CmaTotal'])
        elif key == 'iomem':
            require(re.search(r'(?m)^81800000-83ffffff : System RAM$', value))
        elif key == 'ISP':
            pairs = re.findall(r'(?:^|[\s;])([A-Za-z_][A-Za-z0-9_]*)=([^\s;]+)', value)
            fields = dict(pairs)
            require(len(pairs) == len(fields))
            for k, v in {'result': '-61', 'readable': '0', 'guard_errors': '0', 'irq_mode': '0',
                         'irq_frames': '0', 'queue_mode': '0', 'queue_started': '0',
                         'queue_held': '0', 'wb_owner_active': '0'}.items():
                require(fields.get(k) == v)
        elif key == 'watchdog':
            require(value == 'inactive')
        elif key == 'nowayout':
            require(value == '0')
        elif key == 'UART':
            require(re.fullmatch(r'rx_error_events=0 mem_busy_ticks=0 last_status=[0-9a-f]{8} rxpos=\d+; cached diagnostic, lost bytes cannot be reconstructed', value))
        elif key in ('ptz0', 'ptz1'):
            # At probe both verification booleans are FALSE: never invent OFF ACK.
            axis = key[-1]
            require(value == f'axis={axis} issued_steps=0 phase=0 result=0 coil_readback_ok=0 coils_off_verified=0 max_pulse_steps=4 period_ms=20 physical_position_known=0 bank_hold=0 hold_axis=0 hold_attempted=0 hold_initialized=0 hold_result=0 hold_issued_steps=0 hold_phase=0')
        else:
            self.fail('Unsupported RO schema')

    def preflight(self):
        a, h = self.a, self.h
        require(ROOT == Path('/home/gadorach/Desktop/AnykaOpenCam') and ROOT.resolve() == ROOT and
                sys.flags.isolated and os.getuid() == os.geteuid() and os.getgid() == os.getegid())
        require(all(str(Path(v)) == v for v in (a.private_parent, a.power_config, a.serial_init, a.selection)))
        self.keep(Path(__file__).absolute(), source=True)
        v4 = self.keep(V4, source=True)
        require(v4.digest == V4_SHA)
        public = {path: self.keep(path, source=True) for path in h['PINS']}
        require(all(public[path].digest == digest for path, digest in h['PINS'].items()))
        boot_ast = ast.parse(public[BOOT].data)
        constants = {n.targets[0].id: ast.literal_eval(n.value) for n in boot_ast.body
                     if isinstance(n, ast.Assign) and len(n.targets) == 1 and isinstance(n.targets[0], ast.Name)
                     and isinstance(n.value, ast.Constant)}
        require(a.port == constants['ADAPTER'] and re.fullmatch(r'foundation-20261024[a-z]', a.part))
        # Only two PURE CRC definitions from the exact boot source, no Boot/main.
        crc_nodes = [n for n in boot_ast.body if isinstance(n, ast.FunctionDef) and n.name in {'crc_bzip2', 'crc_verified'}]
        require(len(crc_nodes) == 2 and public[BOOT].digest == BOOT_SHA)
        crc_ns = {'re': re}
        exec(compile(ast.Module(body=crc_nodes, type_ignores=[]), str(BOOT), 'exec'), crc_ns)
        self.crc, self.crc_verified = crc_ns['crc_bzip2'], crc_ns['crc_verified']
        loader = self.keep(LOADER)
        require(loader.digest == LOADER_SHA and len(loader.data) == 1872 and self.crc(loader.data) == 0xdd0572e1)
        self.loader = loader.data
        self.select_image()
        self.prepare_plan()
        prior_names = {n.value for n in ast.walk(ast.parse(public[h['RECORDER']].data))
                       if isinstance(n, ast.Constant) and isinstance(n.value, str) and
                       n.value.startswith('raw/uart-rx-rx-fixed-') and n.value.endswith('.bin')}
        require(len(prior_names) == 1)
        self.prior_path = h['RECORDER'].parent / prior_names.pop()
        self.prior_chain = h['protected'](self.prior_path.parent, outside_git=False)
        prior = self.keep(self.prior_path, limit=854)
        require(prior.meta[2] == os.getuid() and not prior.meta[4] & 0o222 and
                len(prior.data) == 854 and b'\r\nanyka$' in prior.data)
        self.private_chain = h['protected'](Path(a.private_parent))
        config_path = Path(a.power_config)
        h['protected'](config_path.parent)
        h['ancestry'](config_path.parent, private=True)
        config = h['strict_json'](self.keep(config_path, 0o600, 4096).data)
        require(type(config) is dict and set(config) == {'utc', 'endpoint', 'actual_user_load_association',
                'initial_OFF', 'one_idempotent_turn_off_POST_HTTP200', 'final_OFF_readback',
                'ON_requested', 'independent_electrical_rail_measurement'} and type(config['utc']) is str)
        require(config['actual_user_load_association'] == 'QC07F_ONLY_NOT_MS42P_OR_OTHER_EQUIPMENT' and
                all(config[k] is True for k in ('initial_OFF', 'one_idempotent_turn_off_POST_HTTP200', 'final_OFF_readback')) and
                config['ON_requested'] is False and config['independent_electrical_rail_measurement'] is False)
        self.endpoint = config['endpoint']
        u = urlsplit(self.endpoint)
        require(u.scheme == 'http' and u.username is None and u.password is None and not u.query and not u.fragment and
                u.path == '/switch/outlet_power' and u.hostname is not None and self.endpoint == 'http://' + u.netloc + u.path)
        ip = ipaddress.IPv4Address(u.hostname)
        require(any(ip in ipaddress.IPv4Network(net) for net in ('10.0.0.0/8', '172.16.0.0/12', '192.168.0.0/16')) and
                u.netloc == str(ip) + ((':' + str(u.port)) if u.port is not None else ''))
        python = self.keep(Path(sys.executable).resolve(), limit=None, source=True)
        serial_source = self.keep(Path(a.serial_init), source=True)
        require(re.fullmatch('[0-9a-f]{64}', a.python_sha256) and python.digest == a.python_sha256 and
                re.fullmatch('[0-9a-f]{64}', a.serial_sha256) and serial_source.digest == a.serial_sha256)
        import serial
        require(Path(serial.__file__) == serial_source.path and Path(serial.__file__).resolve() == serial_source.path)
        self.serial_file = serial.__file__
        spec = importlib.util.spec_from_file_location('_entry24_cold_classifier', h['CLASSIFIER'])
        self.classifier = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = self.classifier
        exec(compile(public[h['CLASSIFIER']].data, str(h['CLASSIFIER']), 'exec'), self.classifier.__dict__)
        require(Path(self.classifier.__file__) == h['CLASSIFIER'])
        self.control_chain = h['ancestry'](h['CONTROL'])
        require(h['CONTROL'].lstat().st_uid == os.getuid() and stat.S_IMODE(h['CONTROL'].lstat().st_mode) == 0o700)
        for name, mode, expected in (
            ('peripheral-gui-operation.lock', 0o644, {'state': 'HOLD', 'operation': 'one_radio_native_load_RUN21a'}),
            ('peripheral-unattended22b-operation.lock', 0o600, {'state': 'HOLD', 'operation': 'one_idle_watchdog_reset22b_stop_at_uboot'}),
            ('peripheral-gui-instance.lock', 0o644, None)):
            marker = self.keep(h['CONTROL'] / name, mode, 4096)
            fcntl.flock(marker.fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            marker.check()
            require(marker.data == b'' if expected is None else h['strict_json'](marker.data) == expected)
        require(not os.path.lexists(h['SOCKET']))
        self.realport = Path(a.port).resolve(strict=True)
        self.port_meta = h['tty_association'](self.realport.stat())
        self.step(h['fuser'], self.realport)
        for rel in ('raw/uart-rx-', 'logs/uart-events-', 'analysis/capture-final-'):
            ext = '.bin' if rel.startswith('raw/') else ('.jsonl' if rel.startswith('logs/') else '.json')
            require(not os.path.lexists(h['RECORDER'].parent / (rel + a.part + ext)))
        self.out = Path(a.private_parent) / ('driver-entry-' + a.part)
        require(not os.path.lexists(self.out))
        self.out.mkdir(mode=0o700)
        self.out_chain = h['protected'](self.out)
        self.capture = self.out / 'capture'
        self.rawpath = self.capture / ('uart-rx-' + a.part + '.bin')
        self.eventpath = self.capture / ('uart-events-' + a.part + '.jsonl')
        self.finalpath = self.capture / ('capture-final-' + a.part + '.json')
        h['new_json'](self.out / 'intent-HOLD.json', {'state': 'HOLD', 'operation': 'one_supervised_driver_boot_RO24',
                      'part': a.part, 'no_automatic_retry': True, 'native_admission': False,
                      'supervised_detection_not_prevention': True, 'original_total_seconds': TOTAL})
        h['new_json'](self.out / 'inputs.private.json', {'retained': [
            {'path': str(o.path), 'metadata': o.meta, 'sha256': o.digest} for o in self.retained],
            'historical_prerequisite': 'trusted retained namespace, not modern current-state authentication',
            'dependency_scope': 'exact interpreter/serial init, ordinary system dependency closure trusted',
            'user_load_and_current_supervision_are_caller_facts': True})

    def startup(self):
        h, a = self.h, self.a
        planned = 60 + ((len(self.data) + 95) // 96) * .06
        self.clock(1300 + planned + 120)  # WHOLE nominal path must fit before ON.
        self.stage_end = min(self.cap_m, self.origin + 300 * 10**9)
        self.phase = 'initial_GET_OFF'
        self.power_call('initial_GET')
        self.initial_off = self.power['initial_GET'] is False
        require(self.initial_off)
        self.off_proved = True
        self.argv = [sys.executable, '-I', '-B', str(h['RECORDER']), '--capture', '--part', a.part,
                     '--port', a.port, '--seconds', str(TOTAL), '--private-capture-dir', str(self.capture)]
        fd = os.open(self.out / 'receiver-console.log', os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
        self.clock(5)
        # Assignment retains the actual Popen even if subsequent identity/write fails.
        with os.fdopen(fd, 'wb') as console:
            self.child = subprocess.Popen(self.argv, cwd=ROOT, stdin=subprocess.DEVNULL,
                                          stdout=console, stderr=console, start_new_session=True)
        self.start = h['ticks'](self.child.pid)
        self.peer = (self.child.pid, os.getuid(), os.getgid())
        self.clock()
        h['new_json'](self.out / 'child.private.json', {'pid': self.child.pid, 'start_ticks': self.start,
                      'argv': self.argv, 'serial_file': self.serial_file, 'global_cap_monotonic_ns': self.cap_m,
                      'conservative_cap_boottime_ns': self.cap_b})
        until = min(time.monotonic_ns() + 5 * 10**9, self.cap_m - 150 * 10**9)
        while not os.path.lexists(h['SOCKET']):
            require(self.child.poll() is None and time.monotonic_ns() < until)
            self.delay(.025)
        state = self.state()
        require(state['rx_bytes'] == state['tx_bytes'] == 0 and state['abort_armed'] is False)
        self.arm()
        for obj in self.retained:
            self.step(obj.check, cost=.1)
        require(h['protected'](self.prior_path.parent, outside_git=False) == self.prior_chain and
                h['ancestry'](h['CONTROL']) == self.control_chain)
        state = self.state()
        require(state['rx_bytes'] == state['tx_bytes'] == 0 and state['abort_armed'] is True)
        startup_events = self.events()
        require([e.get('kind') for e in startup_events] == ['open_intent', 'opened', 'abort_armed'] and
                startup_events[2].get('key_hex') == h['PACKET'].hex() and
                startup_events[2].get('mode') == 'hold-cli' and startup_events[2].get('inspect_read_only') is False)
        h['new_json'](self.out / 'ON-intent.private.json', {'owned_state': state, 'peer': self.peer,
                      'start_ticks': self.start, 'argv': self.argv, 'socket': h['identity'](h['SOCKET'].lstat()),
                      'one_ON_only': True, 'manual_host_loss_fallback_currently_confirmed_by_caller': True})
        state = self.state()
        require(state['rx_bytes'] == state['tx_bytes'] == 0 and state['abort_armed'] is True)
        self.phase = 'ON_POST'
        self.clock(3)
        self.on_attempted, self.off_proved = True, False
        # Even uncertain POST is followed by the ONE settled GET; errors stay sticky.
        on_errors = []
        try:
            self.power_call('ON_POST_ack', 'turn_on')
        except BaseException as exc:
            on_errors.append('ON_POST_UNCERTAIN: ' + repr(exc))
        self.phase = 'ON_settle_GET'
        try:
            if self.hold is None:
                self.delay(3)
            else:
                time.sleep(3)  # Recovery state observation only; no renewed admission.
        except BaseException as exc:
            on_errors.append('ON_SETTLE_FAILED: ' + repr(exc))
        # ONE GET is attempted even after an uncertain POST/settle/clock. No UART
        # can follow any error; actual GET value is retained separately from ACK.
        try:
            if self.hold is None and not on_errors:
                self.power_call('ON_GET')
            else:
                self.power['ON_GET'] = h['http_power'](self.endpoint)
        except BaseException as exc:
            on_errors.append('ON_GET_FAILED: ' + repr(exc))
        if on_errors:
            self.errors.extend(on_errors)
            self.fail(on_errors[0])
        require(self.power['ON_GET'] is True)
        self.phase = 'exact_cold_and_quiet'
        first = self.state()
        raw, events = self.snapshot(self.rawpath, 1024), self.events()
        require(events[:3] == startup_events)
        count = h['observe'](raw, events, first, self.classifier, self.child.pid, self.realport)
        require(count in (1, 3), 'Closed profile permits one or three complete catch packets only')
        self.delay(10)
        second = self.state()
        require((second['rx_bytes'], second['tx_bytes']) == (first['rx_bytes'], first['tx_bytes']) and
                self.snapshot(self.rawpath, 1024) == raw and self.events() == events and
                h['observe'](raw, events, second, self.classifier, self.child.pid, self.realport) == count)
        self.cold_raw, self.cold_events, self.cold_count = raw, events, count
        self.cursor, self.tx, self.reset_arm = len(raw), count * 19, False
        self.classified.update(raw)
        self.raw_fd = os.open(self.rawpath, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK)
        s = os.fstat(self.raw_fd)
        require(stat.S_ISREG(s.st_mode) and s.st_uid == os.getuid() and
                stat.S_IMODE(s.st_mode) == 0o600 and s.st_nlink == 1 and s.st_size == len(raw))
        self.raw_meta = (s.st_dev, s.st_ino, s.st_uid, s.st_gid, s.st_mode, s.st_nlink)

    def audit_final(self, final, controlled=True):
        h = self.h
        require(final.get('argv') == self.argv[3:] and final.get('cwd') == str(ROOT) and
                final.get('private_capture_v1') is True and type(final.get('files')) is list and
                [x.get('path') for x in final['files']] == [str(self.rawpath), str(self.eventpath)])
        for entry in final['files']:
            data = self.snapshot(Path(entry['path']), 64 * 1024 * 1024)
            require(entry.get('bytes') == len(data) and entry.get('sha256') == hashlib.sha256(data).hexdigest())
        raw, events = self.snapshot(self.rawpath, 4 * 1024 * 1024), self.events()
        require(type(final.get('rx_bytes')) is int and len(raw) == final['rx_bytes'])
        require(events and events[-1]['kind'] == 'capture_end' and
                events[-1].get('rx_bytes') == len(raw) and events[-1].get('tx_bytes') == final['tx_bytes'] and
                events[-1].get('private_rx_bytes_intentionally_omitted') == 0)
        require([e['kind'] for e in events].count('stop_requested') == int(controlled))
        if controlled:
            require(len(events) >= 2 and events[-2]['kind'] == 'stop_requested')
        # Full raw RX coverage and exact TX intent/completion pairs. No cold-only
        # observe(final=True) on the extended capture; two distinct arm records.
        allowed = {'open_intent', 'opened', 'abort_armed', 'abort_arm_triggered', 'rx',
                   'tx_protected_intent', 'tx_complete', 'abort_burst_cancelled', 'stop_requested', 'capture_end'}
        require(all(e.get('kind') in allowed for e in events))
        stamps = [e.get('monotonic_ns') for e in events]
        require(all(type(t) is int for t in stamps) and stamps == sorted(stamps))
        offset, pending, completions = 0, None, []
        for event in events:
            if event['kind'] == 'rx':
                require(event.get('offset') == offset and type(event.get('bytes')) is int and event['bytes'] > 0)
                offset += event['bytes']
            elif event['kind'] == 'tx_protected_intent':
                require(pending is None and type(event.get('bytes')) is int and 0 < event['bytes'] <= 128 and
                        event.get('reason') == 'Protected capture transport; no TX payload in events')
                pending = event['bytes']
            elif event['kind'] == 'tx_complete':
                require(pending is not None and event.get('bytes') == pending)
                completions.append(pending)
                pending = None
        require(pending is None and offset == len(raw) and sum(completions) == final['tx_bytes'])
        if self.cold_events is not None:
            require(events[:len(self.cold_events)] == self.cold_events and raw.startswith(self.cold_raw))
            require(completions == [19] * self.cold_count + self.sent)
            later = events[len(self.cold_events):(-2 if controlled else -1)]
            arms = [e for e in later if e['kind'] == 'abort_armed']
            require(len(arms) == int(self.reset_arm))
            if arms:
                require(arms[0].get('key_hex') == h['PACKET'].hex() and arms[0].get('mode') == 'hold-cli' and
                        arms[0].get('inspect_read_only') is False)
            require(all(e['kind'] in {'abort_armed', 'rx', 'tx_protected_intent', 'tx_complete'} for e in later))
            if self.observed:
                require(len(raw) == self.cursor and final['tx_bytes'] == self.tx and
                        hashlib.sha256(raw).digest() == self.classified.digest())
        else:
            # Early failure never earns observation PASS; final counter/hashes still checked.
            require(not self.observed)

    def finish(self):
        h = self.h
        cleanup_phase = 'clock'

        def record(message):
            self.errors.append(message)
            if self.hold is None:
                self.hold, self.failed_phase = message, 'finalize:' + cleanup_phase

        if self.hold is None:
            try:
                self.clock(0, 0)
            except BaseException as exc:
                record('CLEANUP_CLOCK_HOLD: ' + repr(exc))
        if self.on_attempted:
            self.off_attempted = True
            cleanup_phase = 'OFF_POST'
            try:
                h['http_power'](self.endpoint, 'turn_off')
                self.power['OFF_POST_ack'] = True
            except BaseException as exc:
                record('OFF_POST_UNCERTAIN: ' + repr(exc))
            cleanup_phase = 'OFF_settle'
            try:
                time.sleep(.25)
            except BaseException as exc:
                record('OFF_SETTLE_FAILED: ' + repr(exc))
            cleanup_phase = 'OFF_GET'
            try:
                self.power['OFF_GET'] = h['http_power'](self.endpoint)
                require(self.power['OFF_GET'] is False)
                self.off_proved = True
            except BaseException as exc:
                record('OFF_UNCERTAIN_MANUAL_OFF_REQUIRED: ' + repr(exc))
        if self.child is not None and self.off_proved:
            cleanup_phase = 'owned_stop'
            try:
                if self.child.poll() is None:
                    # Partial creation is NOT silently re-admitted. Missing start/peer
                    # refuses mutation, retains the Popen, still observes actual wait.
                    self.identity()
                    require(self._control({'action': 'stop'}, recovery=True).get('stopping') is True)
            except BaseException as exc:
                record('OWNED_STOP_UNPROVED: ' + repr(exc))
            cleanup_phase = 'owned_wait_final'
            try:
                require(self.child.wait(timeout=5) == 0)
            except BaseException as exc:
                record('OWNED_RETIREMENT_UNPROVED: ' + repr(exc))
            if self.child.returncode is not None:
                before = len(self.errors)
                for label, check in (
                    ('owned_PID_absence', lambda: not (Path('/proc') / str(self.child.pid)).exists()),
                    ('socket_absence', lambda: not os.path.lexists(h['SOCKET'])),
                    ('visible_owner_absence', lambda: h['fuser'](self.realport) is None)):
                    try:
                        require(check())
                    except BaseException as exc:
                        record(label + ': ' + repr(exc))
                try:
                    obj = h['Retained'](self.finalpath, 0o600, 100000)
                    try:
                        self.audit_final(h['strict_json'](obj.data))
                        self.final_ok = self.child.returncode == 0 and len(self.errors) == before
                    finally:
                        os.close(obj.fd)
                except BaseException as exc:
                    record('FINAL_RAW_EVENT_ACCOUNTING_UNPROVED: ' + repr(exc))
        elif self.child is not None:
            record('RECORDER_RETAINED_POWER_UNCERTAIN_MANUAL_OFF_REQUIRED')
        if self.child is not None and self.child.poll() is None:
            cleanup_phase = 'passive_actual_owner_retention'
            # Do not return and lose actual Popen/locks while a partial or uncertain
            # child remains. No further UART/RPC/power/status, kill or timer renewal.
            # The ORIGINAL recorder duration remains its only scheduled lifetime.
            if self.out_chain is not None:
                try:
                    h['new_json'](self.out / 'retained-owner-HOLD.private.json', {
                        'state': 'HOLD', 'manual_OFF_required': not self.off_proved,
                        'actual_Popen_retained': True, 'no_more_effects_or_RPC': True,
                        'original_duration_not_extended': TOTAL, 'errors': self.errors})
                except BaseException as exc:
                    record('RETENTION_RECORD_FAILED: ' + repr(exc))
            print('HOLD: manual OFF if uncertain; retaining owned recorder until its original exit; no retry.', flush=True)
            while self.child.returncode is None:
                try:
                    code = self.child.wait()  # Actual child only, never a PID-only kill.
                    record('RETAINED_CHILD_EVENTUAL_WAIT: ' + str(code))
                except KeyboardInterrupt:
                    # Ctrl-C is not proof of OFF/retirement or authority to kill.
                    continue
                except BaseException as exc:
                    record('ACTUAL_WAIT_UNPROVED_OWNER_RETAINED: ' + repr(exc))
                    time.sleep(1)  # Passive custody only; never another target effect.
            # Independently inspect available final accounting even on natural
            # expiry/failed controlled stop; absence/error remains a saved failure.
            for label, check in (
                ('eventual_owned_PID_absence', lambda: not (Path('/proc') / str(self.child.pid)).exists()),
                ('eventual_socket_absence', lambda: not os.path.lexists(h['SOCKET'])),
                ('eventual_visible_owner_absence', lambda: h['fuser'](self.realport) is None)):
                try:
                    require(check())
                except BaseException as exc:
                    record(label + ': ' + repr(exc))
            try:
                obj = h['Retained'](self.finalpath, 0o600, 100000)
                try:
                    self.audit_final(h['strict_json'](obj.data), controlled=False)
                finally:
                    os.close(obj.fd)
            except BaseException as exc:
                record('EVENTUAL_FINAL_ACCOUNTING_FAILED: ' + repr(exc))
            # This is NOT successful controlled stop/finalization or power proof.
            self.final_ok = False
        # No preceding postcheck failure may skip another original input/marker.
        cleanup_phase = 'individual_retained_input_postchecks'
        for obj in self.retained:
            try:
                obj.check()
            except BaseException as exc:
                record('INPUT_OR_MARKER_CHANGED: ' + repr(exc))
        checks = [('private_parent', lambda: self.private_chain is None or h['protected'](Path(self.a.private_parent)) == self.private_chain),
                  ('output', lambda: self.out_chain is None or h['protected'](self.out) == self.out_chain),
                  ('control', lambda: self.control_chain is None or h['ancestry'](h['CONTROL']) == self.control_chain),
                  ('historical_parent', lambda: not hasattr(self, 'prior_chain') or
                   h['protected'](self.prior_path.parent, outside_git=False) == self.prior_chain)]
        for label, check in checks:
            try:
                require(check())
            except BaseException as exc:
                record(label + ': ' + repr(exc))
        if self.hold is None:
            try:
                self.clock(0, 0)
            except BaseException as exc:
                record('FINAL_CLOCK_HOLD: ' + repr(exc))
        success = self.observed and self.final_ok and self.off_proved and self.off_attempted and not self.errors and self.hold is None
        if self.out_chain is not None:
            try:
                h['new_json'](self.out / ('result.private.json' if success else 'failure-HOLD.private.json'),
                              {'state': 'BOOT_RO_OBSERVATION_ONLY' if success else 'HOLD', 'errors': self.errors,
                               'first_error': self.hold, 'failed_phase': self.failed_phase, 'phase': self.phase,
                               'power': self.power, 'initial_OFF': self.initial_off, 'ON_attempted': self.on_attempted,
                               'OFF_attempted': self.off_attempted, 'OFF_readback': self.off_proved,
                               'capture_finalized': self.final_ok, 'child_pid': self.child.pid if self.child else None,
                               'actual_child_returncode': self.child.returncode if self.child else None,
                               'manual_OFF_required': self.on_attempted and not self.off_proved,
                               'ownership_retained_if_unretired': self.child is not None and not self.final_ok,
                               'TX_completed_lengths': self.sent, 'original_cap_monotonic_ns': self.cap_m,
                               'original_cap_boottime_ns': self.cap_b, 'clock_offset_intersections': self.offsets,
                               'strict_C1_proved': False, 'next_phase_admission': False,
                               'complete_process_FD_or_DMA_proof': False, 'physical_OFF_measurement': False})
            except BaseException:
                success = False
        for obj in reversed(self.retained):
            os.close(obj.fd)  # Only OUR read-only descriptions; no marker byte changes.
        if self.raw_fd is not None:
            os.close(self.raw_fd)  # Does not stop/close the recorder's serial owner.
        return success


class Parser(argparse.ArgumentParser):
    def error(self, message):
        self.exit(2, 'Closed driver entry arguments refused; HOLD\n')


def main():
    parser = Parser(description=__doc__, allow_abbrev=False)
    for name in ('private-parent', 'power-config', 'port', 'part', 'python-sha256',
                 'serial-init', 'serial-sha256', 'selection', 'selection-sha256'):
        parser.add_argument('--' + name, required=True)
    for name in ('confirm-current-uart-only-serialized-historical-association',
                 'confirm-current-supervised-manual-host-loss-cutoff'):
        parser.add_argument('--' + name, action='store_true', required=True)
    args = parser.parse_args()
    os.umask(0o077)
    entry = Entry(args, pinned_helpers())
    try:
        entry.preflight()
        entry.startup()
        entry.boot_inventory()
    except BaseException as exc:
        if entry.hold is None:
            entry.hold, entry.failed_phase = repr(exc), entry.phase
        entry.errors.append(repr(exc))
    finally:
        success = entry.finish()
    print('Boot/RO observation only; outlet OFF; recorder finalized; no next phase.' if success else
          'HOLD: inspect private evidence; manual OFF if uncertain; no retry or next phase.')
    return 0 if success else 1


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (Exception, KeyboardInterrupt):
        print('HOLD: prerequisites/ownership uncertain; manual OFF if needed; no retry.', file=sys.stderr)
        raise SystemExit(1)
