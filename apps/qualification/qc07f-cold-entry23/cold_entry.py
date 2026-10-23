#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""ONE parent-only cold observation. Read CONTRACT.md before selecting execution."""
import argparse
import ast
import datetime
import fcntl
import hashlib
import http.client
import importlib.util
import ipaddress
import json
import os
from pathlib import Path
import re
import socket
import stat
import struct
import subprocess
import sys
import time
from urllib.parse import urlsplit

ROOT = Path(__file__).absolute().parents[3]
RECORDER = ROOT / 'work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py'
BOOT = ROOT / 'scripts/qc07f_boot.py'
CLASSIFIER = ROOT / 'apps/qualification/encoder-query77-cold-context-v2/cold_context.py'
PINS = {
    RECORDER: '135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00',
    BOOT: '6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985',
    CLASSIFIER: '17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558',
}
CONTROL = ROOT / 'work/qc-uart-live'
SOCKET = CONTROL / 'uart.sock'
PACKET = b'U-C!FItA\r\x11U-C!FItA\r'


def require(ok):
    if not ok:
        raise ValueError('Cold observation prerequisite or evidence refused')


def strict_json(data):
    def pairs(items):
        value = {}
        for key, item in items:
            require(key not in value)
            value[key] = item
        return value
    def invalid_constant(value):
        raise ValueError('Non-JSON constant refused')
    return json.loads(data, object_pairs_hook=pairs, parse_constant=invalid_constant)


def identity(s):
    return (s.st_dev, s.st_ino, s.st_uid, s.st_gid, s.st_mode, s.st_nlink,
            s.st_size, s.st_mtime_ns, s.st_ctime_ns)


def ancestry(path, private=False):
    require(path.is_absolute() and str(path) == os.path.normpath(str(path)) and path.resolve(strict=True) == path)
    result = []
    for p in (*reversed(path.parents), path):
        s = p.lstat()
        require(stat.S_ISDIR(s.st_mode) and s.st_uid in (0, os.getuid()) and not s.st_mode & 0o022)
        if private:
            require(not os.path.lexists(p / '.git') and p != ROOT)
        result.append((str(p), s.st_dev, s.st_ino, s.st_uid, s.st_gid, s.st_mode))
    return result


def protected(path, outside_git=True):
    chain = ancestry(path, private=outside_git)
    s = path.lstat()
    require(s.st_uid == os.getuid() and stat.S_IMODE(s.st_mode) == 0o700)
    return chain


class Retained:
    """Read-only object retained solely through this transaction's final postcheck."""
    def __init__(self, path, mode=None, limit=100000):
        self.path, self.fd, self.limit = path, None, limit
        self.chain = ancestry(path.parent)
        before = path.lstat()
        require(stat.S_ISREG(before.st_mode) and before.st_uid in (0, os.getuid()) and
                before.st_nlink == 1 and not before.st_mode & 0o022)
        if mode is not None:
            require(before.st_uid == os.getuid() and stat.S_IMODE(before.st_mode) == mode)
        self.fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK)
        try:
            self.meta = identity(os.fstat(self.fd))
            require(self.meta == identity(before))
            self.digest, self.data = self.read()
            self.check()
        except BaseException:
            os.close(self.fd)
            self.fd = None
            raise

    def read(self):
        os.lseek(self.fd, 0, os.SEEK_SET)
        h, chunks, size = hashlib.sha256(), [], 0
        while chunk := os.read(self.fd, 65536):
            size += len(chunk)
            require(size <= (self.limit if self.limit is not None else 32 * 1024 * 1024))
            h.update(chunk)
            if self.limit is not None:
                chunks.append(chunk)
        return h.hexdigest(), b''.join(chunks)

    def check(self):
        require(ancestry(self.path.parent) == self.chain and
                identity(self.path.lstat()) == self.meta and
                identity(os.fstat(self.fd)) == self.meta and self.read()[0] == self.digest)
        require(identity(os.fstat(self.fd)) == self.meta)


def new_json(path, value):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
    with os.fdopen(fd, 'w') as f:
        json.dump(value, f, indent=2)
        f.write('\n')
        f.flush()
        os.fsync(f.fileno())
    d = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY | os.O_CLOEXEC)
    try:
        os.fsync(d)
    finally:
        os.close(d)


def rpc(request, peer):
    metadata = SOCKET.lstat()
    require(stat.S_ISSOCK(metadata.st_mode) and metadata.st_uid == os.getuid() and
            stat.S_IMODE(metadata.st_mode) == 0o600 and metadata.st_nlink == 1)
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.settimeout(3)
        s.connect(str(SOCKET))
        actual = struct.unpack('3i', s.getsockopt(socket.SOL_SOCKET, socket.SO_PEERCRED, 12))
        mismatch = actual != peer
        # An empty disconnect can break this immutable recorder's error reply.
        s.sendall(json.dumps({'action': 'status'} if mismatch else request).encode() + b'\n')
        data = bytearray()
        while chunk := s.recv(4096):
            data.extend(chunk)
            require(len(data) <= 65536)
    require(not mismatch)
    value = strict_json(data)
    require(type(value) is dict and 'error' not in value)
    return value


def ticks(pid):
    return int((Path('/proc') / str(pid) / 'stat').read_text().rsplit(')', 1)[1].split()[19])


def fuser(port, pid=None):
    q = subprocess.run(['/usr/bin/fuser', str(port)], capture_output=True, timeout=3)
    require((q.returncode, q.stdout.split()) == ((1, []) if pid is None else (0, [str(pid).encode()])))


def http_power(endpoint, action=None):
    # endpoint is already strictly numeric/validated: no resolver discovery or redirects.
    u = urlsplit(endpoint)
    c = http.client.HTTPConnection(u.hostname, u.port or 80, timeout=3)
    try:
        c.request('GET' if action is None else 'POST', u.path + ('' if action is None else '/' + action),
                  body=None if action is None else b'', headers={'Connection': 'close'})
        response = c.getresponse()
        require(response.status == 200 and response.getheader('Location') is None)
        require(response.getheader('Content-Type', '').split(';')[0].strip().lower() == 'application/json')
        body = response.read(4097)
        require(len(body) <= 4096)
        value = strict_json(body)
        require(type(value) is dict and value.get('id') == 'switch-outlet_power' and
                type(value.get('value')) is bool and
                value.get('state') == ('ON' if value['value'] else 'OFF'))
        return value['value']
    finally:
        c.close()


def observe(raw, events, state, classifier, pid, realport, final=False):
    require(type(state.get('rx_bytes')) is int and type(state.get('tx_bytes')) is int and
            state['abort_armed'] is False and state['tx_busy'] is False and state['read_only_inspection_active'] is False)
    require(len(raw) == state['rx_bytes'] and 360 <= len(raw) <= 596)
    facts = classifier.classify_cold_context(raw[:360])
    require(facts.classification is classifier.Classification.EXACT_OBSERVED_PROFILE_NOT_ADMISSION and
            facts.matched_records == 19 and facts.matched_record_bytes == 360 and facts.challenge_record_matched)
    allowed = {'open_intent', 'opened', 'abort_armed', 'abort_arm_triggered', 'rx',
               'tx_protected_intent', 'tx_complete', 'abort_burst_cancelled'}
    if final:
        allowed |= {'stop_requested', 'capture_end'}
    require(all(type(e) is dict and e.get('kind') in allowed for e in events))
    def selected(kind):
        return [e for e in events if e['kind'] == kind]
    require(len(selected('open_intent')) == len(selected('opened')) ==
            len(selected('abort_armed')) == len(selected('abort_arm_triggered')) == 1)
    opened, opening = selected('opened')[0], selected('open_intent')[0]
    require(events[:2] == [opening, opened] and opened.get('pid') == pid and
            opening.get('port') == state['adapter'] and opening.get('realpath') == str(realport) and
            opening.get('dtr_requested') is False and opening.get('rts_requested') is False)
    stamps = [e.get('monotonic_ns') for e in events]
    require(all(type(t) is int for t in stamps) and stamps == sorted(stamps))
    arm, trigger = selected('abort_armed')[0], selected('abort_arm_triggered')[0]
    require(arm.get('key_hex') == PACKET.hex() and arm.get('mode') == 'hold-cli' and
            arm.get('inspect_read_only') is False and trigger.get('payload_hex') == PACKET.hex() and
            trigger.get('prequeued_commands') is True and trigger.get('volatile_bootargs') is False and
            type(trigger.get('attempts')) is int and trigger['attempts'] in (1, 3))
    require((trigger['marker'], trigger['attempts']) in (
        ('early U-Boot banner/DRAM for bounded key prequeue', 3), ('complete Net line from new boot', 1)))
    require(events.index(arm) == 2 and events.index(arm) < events.index(trigger))
    intents, completed = selected('tx_protected_intent'), selected('tx_complete')
    count = len(completed)
    require(1 <= count <= trigger['attempts'] and count == len(intents) and
            state['tx_bytes'] == count * 19 and len(selected('abort_burst_cancelled')) <= 1)
    pending, sent, offset = False, 0, 0
    for e in events:
        k = e['kind']
        if k in ('tx_protected_intent', 'tx_complete'):
            require(events.index(e) > events.index(trigger) and e.get('bytes') == 19)
            if k == 'tx_protected_intent':
                require(not pending and e.get('reason') == 'Protected capture transport; no TX payload in events')
                pending = True
            else:
                require(pending)
                pending, sent = False, sent + 1
        if k == 'rx':
            require(e.get('offset') == offset and type(e.get('bytes')) is int and e['bytes'] > 0)
            offset += e['bytes']
    require(not pending and sent == count and offset == len(raw))
    def echo(command):
        return command + b'\n' + b"Unknown command '" + command + b"' - try 'help'" + b'\r\n\r\nanyka$'
    group = echo(b'U-C!FItA') + echo(b'\x11U-C!FItA')
    require(len(group) == 118 and raw[360:] == group * (count - 1))
    if final:
        require(len(selected('stop_requested')) == len(selected('capture_end')) == 1 and
                events[-1]['kind'] == 'capture_end' and
                selected('stop_requested')[0] == events[-2] and
                events[-1].get('rx_bytes') == len(raw) and events[-1].get('tx_bytes') == count * 19 and
                events[-1].get('private_rx_bytes_intentionally_omitted') == 0)
    return count


class Parser(argparse.ArgumentParser):
    def error(self, message):
        self.exit(2, 'Cold observation arguments refused; HOLD\n')


def main():
    p = Parser(description=__doc__, allow_abbrev=False)
    for name in ('private-parent', 'power-config', 'port', 'part', 'python-sha256', 'serial-init', 'serial-sha256'):
        p.add_argument('--' + name, required=True)
    p.add_argument('--confirm-current-uart-only-serialized-historical-association', action='store_true', required=True)
    a = p.parse_args()
    os.umask(0o077)
    retained, errors, child, start, peer = [], [], None, None, None
    out = endpoint = None
    on_attempted = off_attempted = initial_off = off_proved = False
    observed = final_ok = False
    argv = realport = port_meta = out_chain = private_chain = None

    def keep(path, mode=None, limit=100000):
        obj = Retained(path, mode, limit)
        retained.append(obj)
        return obj

    def child_identity():
        require(child is not None and child.poll() is None and start is not None and ticks(child.pid) == start)
        proc = Path('/proc') / str(child.pid)
        require((proc / 'cmdline').read_bytes().split(b'\0') == [os.fsencode(x) for x in argv] + [b''] and
                (proc / 'exe').resolve() == Path(sys.executable).resolve() and
                (proc / 'cwd').resolve() == ROOT)
        require(Path(a.port).resolve(strict=True) == realport and identity(realport.stat()) == port_meta)

    def status():
        child_identity()
        s = rpc({'action': 'status'}, peer)
        require(s.get('private_capture_v1') is True and s.get('protected_send_v1') is True and
                s.get('conditional_send_v1') is True and s.get('capture_source_sha256') == PINS[RECORDER] and
                s.get('part') == a.part and s.get('adapter') == a.port and s.get('session') == str(capture) and
                s.get('private_capture_dir') == str(capture) and s.get('raw_path') == str(rawpath) and s.get('recent_rx') == '')
        fuser(realport, child.pid)
        return s

    def capture_bytes(path, limit):
        obj = Retained(path, 0o600, limit)
        try:
            require(protected(capture))
            return obj.data
        finally:
            os.close(obj.fd)

    def capture_events():
        return [strict_json(line) for line in capture_bytes(eventpath, 100000).splitlines()]

    try:
        require(ROOT == Path('/home/gadorach/Desktop/AnykaOpenCam') and ROOT.resolve() == ROOT and sys.flags.isolated and
                os.getuid() == os.geteuid() and os.getgid() == os.getegid())
        require(all(str(Path(v)) == v for v in (a.private_parent, a.power_config, a.serial_init)))
        own_source = keep(Path(__file__).absolute())
        public = {path: keep(path) for path in PINS}
        require(all(public[path].digest == digest for path, digest in PINS.items()))
        boot_ast = ast.parse(public[BOOT].data)
        constants = {n.targets[0].id: ast.literal_eval(n.value) for n in boot_ast.body
                     if isinstance(n, ast.Assign) and len(n.targets) == 1 and isinstance(n.targets[0], ast.Name)
                     and isinstance(n.value, ast.Constant)}
        require(a.port == constants['ADAPTER'] and re.fullmatch(r'foundation-20261023[d-z]', a.part))
        # Derive the immutable prerequisite path from pinned source; never export its selector.
        prior_names = {n.value for n in ast.walk(ast.parse(public[RECORDER].data))
                       if isinstance(n, ast.Constant) and isinstance(n.value, str)
                       and n.value.startswith('raw/uart-rx-rx-fixed-') and n.value.endswith('.bin')}
        require(len(prior_names) == 1)
        prior_path = RECORDER.parent / prior_names.pop()
        # ONLY this exact source-bound historical read has an in-workspace role.
        prior_chain = protected(prior_path.parent, outside_git=False)
        prior = keep(prior_path, limit=854)
        require(prior.meta[2] == os.getuid() and not prior.meta[4] & 0o222 and
                len(prior.data) == 854 and b'\r\nanyka$' in prior.data)
        private_parent = Path(a.private_parent)
        private_chain = protected(private_parent)
        config_path = Path(a.power_config)
        protected(config_path.parent)
        ancestry(config_path.parent, private=True)
        config = strict_json(keep(config_path, 0o600, 4096).data)
        require(type(config) is dict and set(config) == {'utc', 'endpoint', 'actual_user_load_association',
                'initial_OFF', 'one_idempotent_turn_off_POST_HTTP200', 'final_OFF_readback',
                'ON_requested', 'independent_electrical_rail_measurement'} and type(config['utc']) is str)
        require(config.get('actual_user_load_association') == 'QC07F_ONLY_NOT_MS42P_OR_OTHER_EQUIPMENT' and
                all(config.get(k) is True for k in ('initial_OFF', 'one_idempotent_turn_off_POST_HTTP200', 'final_OFF_readback')) and
                config.get('ON_requested') is False and config.get('independent_electrical_rail_measurement') is False)
        endpoint = config['endpoint']
        u = urlsplit(endpoint)
        require(u.scheme == 'http' and u.username is None and u.password is None and not u.query and not u.fragment and
                u.path == '/switch/outlet_power' and u.hostname is not None and
                endpoint == 'http://' + u.netloc + u.path)
        ip = ipaddress.IPv4Address(u.hostname)
        require(any(ip in ipaddress.IPv4Network(net) for net in ('10.0.0.0/8', '172.16.0.0/12', '192.168.0.0/16')) and
                u.netloc == str(ip) + ((':' + str(u.port)) if u.port is not None else ''))
        python = keep(Path(sys.executable).resolve(), limit=None)
        serial_source = keep(Path(a.serial_init))
        require(re.fullmatch('[0-9a-f]{64}', a.python_sha256) and python.digest == a.python_sha256 and
                re.fullmatch('[0-9a-f]{64}', a.serial_sha256) and serial_source.digest == a.serial_sha256)
        import serial
        require(Path(serial.__file__) == serial_source.path and Path(serial.__file__).resolve() == serial_source.path)
        spec = importlib.util.spec_from_file_location('_cold_entry23_profile', CLASSIFIER)
        classifier = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = classifier
        # Execute the exact retained public bytes, never a timestamp-matching .pyc.
        exec(compile(public[CLASSIFIER].data, str(CLASSIFIER), 'exec'), classifier.__dict__)
        require(Path(classifier.__file__) == CLASSIFIER)
        control_chain = ancestry(CONTROL)
        require(CONTROL.lstat().st_uid == os.getuid() and stat.S_IMODE(CONTROL.lstat().st_mode) == 0o700)
        for name, mode, expected in (
            ('peripheral-gui-operation.lock', 0o644, {'state': 'HOLD', 'operation': 'one_radio_native_load_RUN21a'}),
            ('peripheral-unattended22b-operation.lock', 0o600, {'state': 'HOLD', 'operation': 'one_idle_watchdog_reset22b_stop_at_uboot'}),
            ('peripheral-gui-instance.lock', 0o644, None),
        ):
            marker = keep(CONTROL / name, mode, 4096)
            fcntl.flock(marker.fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            marker.check()
            require((marker.data == b'') if expected is None else strict_json(marker.data) == expected)
        require(not os.path.lexists(SOCKET))
        realport = Path(a.port).resolve(strict=True)
        require(stat.S_ISCHR(realport.stat().st_mode))
        port_meta = identity(realport.stat())
        fuser(realport)
        for rel in ('raw/uart-rx-', 'logs/uart-events-', 'analysis/capture-final-'):
            ext = '.bin' if rel.startswith('raw/') else ('.jsonl' if rel.startswith('logs/') else '.json')
            require(not os.path.lexists(RECORDER.parent / (rel + a.part + ext)))
        out = private_parent / ('cold-entry-' + a.part)
        require(not os.path.lexists(out))
        out.mkdir(mode=0o700)
        out_chain = protected(out)
        capture = out / 'capture'
        rawpath = capture / ('uart-rx-' + a.part + '.bin')
        eventpath = capture / ('uart-events-' + a.part + '.jsonl')
        finalpath = capture / ('capture-final-' + a.part + '.json')
        new_json(out / 'intent-HOLD.json', {'state': 'HOLD', 'operation': 'one_cold_observation23',
                 'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'part': a.part,
                 'power_config': str(config_path), 'no_automatic_retry': True, 'native_admission': False})
        new_json(out / 'private-prerequisites.json', {
            'retained': [{'path': str(obj.path), 'metadata': obj.meta, 'sha256': obj.digest} for obj in retained],
            'historical_prerequisite': 'trusted retained namespace; no modern manifest; NOT current-state authentication',
            'dependency_scope': 'isolated same interpreter/serial import; ordinary system dependency closure trusted',
            'user_load_association_is_caller_fact_not_measurement': True,
            'caller_source_sha256': own_source.digest})
        initial_off = http_power(endpoint) is False
        require(initial_off)
        off_proved = True
        argv = [sys.executable, '-I', '-B', str(RECORDER), '--capture', '--part', a.part,
                '--port', a.port, '--seconds', '120', '--private-capture-dir', str(capture)]
        fd = os.open(out / 'receiver-console.log', os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
        with os.fdopen(fd, 'wb') as console:
            child = subprocess.Popen(argv, cwd=ROOT, stdin=subprocess.DEVNULL, stdout=console,
                                     stderr=console, start_new_session=True)
        start = ticks(child.pid)
        peer = (child.pid, os.getuid(), os.getgid())
        new_json(out / 'child.json', {'pid': child.pid, 'start_ticks': start, 'argv': argv,
                 'source_sha256': PINS[RECORDER], 'serial_file': serial.__file__})
        until = time.monotonic() + 5
        while not os.path.lexists(SOCKET):
            require(child.poll() is None and time.monotonic() < until)
            time.sleep(.025)
        s = status()
        require(s.get('rx_bytes') == s.get('tx_bytes') == 0 and s.get('abort_armed') is False and
                s.get('tx_busy') is False and s.get('read_only_inspection_active') is False)
        ack = rpc({'action': 'arm_abort', 'mode': 'hold-cli', 'volatile_cli_hold': True, 'inspect': False}, peer)
        require(ack.get('armed') is True and ack.get('mode') == 'hold-cli' and
                ack.get('persistent_write') is False and ack.get('volatile_bootargs') is False and ack.get('read_only_commands') == [])
        new_json(out / 'arm.json', ack)
        for obj in retained:
            obj.check()
        require(protected(prior_path.parent, outside_git=False) == prior_chain and ancestry(CONTROL) == control_chain)
        s = status()
        require(s.get('rx_bytes') == s.get('tx_bytes') == 0 and s.get('abort_armed') is True and
                s.get('tx_busy') is False and s.get('read_only_inspection_active') is False)
        startup_events = capture_events()
        require([e.get('kind') for e in startup_events] == ['open_intent', 'opened', 'abort_armed'] and
                startup_events[2].get('key_hex') == PACKET.hex() and startup_events[2].get('mode') == 'hold-cli' and
                startup_events[2].get('inspect_read_only') is False)
        new_json(out / 'power-ON-intent.json', {'one_attempt_only': True, 'actual_fresh_armed_status': s,
                 'actual_socket_peer_pid_uid_gid': peer, 'start_ticks': start, 'argv': argv,
                 'socket_metadata': identity(SOCKET.lstat()), 'visible_fuser_exact_owned_PID': True})
        # Last point-check AFTER durable intent, directly before the single ON.
        s = status()
        require(s.get('rx_bytes') == s.get('tx_bytes') == 0 and s.get('abort_armed') is True and
                s.get('tx_busy') is False and s.get('read_only_inspection_active') is False)
        on_attempted, off_proved = True, False  # BEFORE a possibly successful-but-unacknowledged ON.
        require(http_power(endpoint, 'turn_on') is True)
        time.sleep(3)
        first = status()
        raw = capture_bytes(rawpath, 1024)
        first_events = capture_events()
        require(first_events[:3] == startup_events)
        count = observe(raw, first_events, first, classifier, child.pid, realport)
        time.sleep(10)
        second = status()
        require((second['rx_bytes'], second['tx_bytes']) == (first['rx_bytes'], first['tx_bytes']) and
                capture_bytes(rawpath, 1024) == raw)
        quiet_events = capture_events()
        require(quiet_events == first_events and
                observe(raw, quiet_events, second, classifier, child.pid, realport) == count)
        observed = True
    except BaseException as exc:
        errors.append(repr(exc))
    finally:
        if on_attempted:
            off_attempted = True  # Exactly one attempt, even if POST/GET raises.
            try:
                require(http_power(endpoint, 'turn_off') is False)
            except BaseException as exc:
                errors.append('OFF_POST_UNCERTAIN: ' + repr(exc))
            # Read back even after an uncertain POST; NEVER issue a second OFF.
            try:
                require(http_power(endpoint) is False)
                off_proved = True
            except BaseException as exc:
                errors.append('OFF_UNCERTAIN_MANUAL_OFF_REQUIRED: ' + repr(exc))
        if child is not None and off_proved:
            try:
                if child.poll() is None:
                    child_identity()
                    require(rpc({'action': 'stop'}, peer).get('stopping') is True)
            except BaseException as exc:
                errors.append('OWNED_STOP_UNPROVED: ' + repr(exc))
            # Keep the actual Popen and observe its wait result even if stop ACK was lost.
            try:
                require(child.wait(timeout=5) == 0)
                require(not os.path.lexists(SOCKET))
                fuser(realport)
                final_obj = Retained(finalpath, 0o600, 100000)
                try:
                    final = strict_json(final_obj.data)
                    # Recorder stores sys.argv, not /proc's interpreter/-I/-B prefix.
                    require(final.get('argv') == argv[3:] and final.get('cwd') == str(ROOT) and final.get('private_capture_v1') is True)
                    require(type(final.get('files')) is list and len(final['files']) == 2)
                    require([x.get('path') for x in final['files']] == [str(rawpath), str(eventpath)])
                    for entry in final['files']:
                        data = capture_bytes(Path(entry['path']), 100000)
                        require(entry.get('bytes') == len(data) and entry.get('sha256') == hashlib.sha256(data).hexdigest())
                    if observed:
                        require(capture_bytes(rawpath, 1024) == raw and final.get('rx_bytes') == len(raw) and
                                final.get('tx_bytes') == count * 19)
                        final_events = capture_events()
                        require(len(final_events) == len(quiet_events) + 2 and final_events[:len(quiet_events)] == quiet_events and
                                observe(raw, final_events, second, classifier, child.pid, realport, final=True) == count)
                    final_ok = True
                finally:
                    os.close(final_obj.fd)
            except BaseException as exc:
                errors.append('OWNED_RETIREMENT_UNPROVED: ' + repr(exc))
        elif child is not None:
            errors.append('RECORDER_PRESERVED_POWER_UNCERTAIN_MANUAL_OFF_REQUIRED')
        # EACH retained object gets its own failure-path postcheck before ANY unlock.
        for obj in retained:
            try:
                obj.check()
            except BaseException as exc:
                errors.append('INPUT_OR_MARKER_CHANGED: ' + repr(exc))
        for label, check in (
            ('private_parent', lambda: protected(Path(a.private_parent)) == private_chain),
            ('output', lambda: out is None or protected(out) == out_chain),
        ):
            try:
                require(check())
            except BaseException as exc:
                errors.append(label + ': ' + repr(exc))
        success = observed and final_ok and off_proved and off_attempted and not errors
        if out is not None and out_chain is not None:
            try:
                new_json(out / ('result.json' if success else 'failure-HOLD.json'),
                         {'state': 'PASS_OBSERVATION_ONLY' if success else 'HOLD', 'errors': errors,
                          'initial_OFF': initial_off, 'ON_attempted': on_attempted, 'OFF_attempted': off_attempted,
                          'OFF_readback_proved': off_proved, 'capture_finalized': final_ok,
                          'owned_child_pid': child.pid if child else None,
                          'owned_child_returncode': child.returncode if child else None,
                          'manual_OFF_required': on_attempted and not off_proved,
                          'native_admission': False, 'independent_rail_measurement': False})
            except BaseException:
                success = False
        for obj in reversed(retained):
            os.close(obj.fd)  # Releases only OUR read-only descriptions/locks.
    print('PASS: cold observation only; outlet OFF; recorder finalized; no next operation.' if success else
          'HOLD: inspect private evidence; if power uncertain, manual OFF required. No retry.')
    return 0 if success else 1


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (Exception, KeyboardInterrupt):
        print('HOLD: private prerequisites/cleanup uncertain; manual OFF if needed. No retry.', file=sys.stderr)
        raise SystemExit(1)
