"""Explicit parent-admitted kernel75 transport via the existing private recorder.

No serial open, boot, setup, staging, catch rearm or SSH fallback. A private epoch
is prepared by the parent only AFTER native feed/control qualification. Unknown
completion retains the operation lease and durable HOLD, even after client exit.
"""
import datetime as dt
import fcntl
import json
import os
from pathlib import Path
import re
import sys
import threading
import time

from optical_control import ACTIONS, ALL_ACTIONS, ControlError, NotAdmitted, Epoch, receipt

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'scripts'))
import qc07f_private_root_transfer as private

HELPER = '/tmp/qc07f-optical-cli'
HELPER_PIN = 'eaded5d3827ec9ee073cc09efb0904b06f6f7d073405bb7bfc3e757c5db4f128'
DISPATCH = '/tmp/qc07f-optical-dispatch.sh'
DISPATCH_PIN = 'cfc10dea3fff5a3fb434a98053d5113a3573b5a7bc9ff2b11778853eb8d63df6'
MODULE = '/run/qc-stage-optical75.ko'
KERNEL = '6.18.55-qc07f-prototype-dirty'
KERNEL_PIN = '5e37b09c9dfda7c87efab998d2ce3484dc4ff152d6746563560ba745a7e18ec1'
MODULE_PIN = 'b839958867cfbf89877a17cfc72a448db12b3703f13bf3528faaa720a6a3a61e'
LOCK = private.boot.CONTROL / 'peripheral-gui-operation.lock'
# Native20a ABI/status/neutral setup/OFF and sourced dispatcher were checked.
# This opens schema transport only: a fresh private *bounded trial* epoch is
# still mandatory; pulse/IR timing and optical movement are not product-qualified.
NATIVE_SCHEMA_REVIEWED = True


def completed_shell_context(recent):
    """An idle owned shell need not end in PS1 after asynchronous kernel logs.

    Accept only a complete trusted nonce frame, with no subsequent boot/monitor/
    fatal transition. This is console-context evidence, not effect admission.
    """
    if type(recent) is not str:
        return False
    if recent.rstrip().endswith('#'):
        return True
    text = recent.replace('\r', '')
    ends = list(re.finditer(r'(?m)^(P20_END|OB_DONE|AIM_DONE)_([0-9a-f]{32}):0$', text))
    if not ends:
        return False
    end = ends[-1]
    prefix = {'P20_END': 'P20_BEGIN_', 'OB_DONE': 'OB_BEGIN_', 'AIM_DONE': 'AIM_BEGIN_'}[end[1]]
    if not re.search(r'(?m)^' + prefix + end[2] + '$', text[:end.start()]):
        return False
    suffix = text[end.end():]
    return not any(marker in suffix for marker in
                   ('U-Boot', 'Starting kernel', 'Linux version ', 'qc07f>',
                    'RAM-only shell returned', 'Kernel panic', 'Oops:', 'BUG:'))


def strict_json(data):
    def pairs(items):
        row = {}
        for key, value in items:
            if key in row:
                raise ControlError('Duplicate bridge field')
            row[key] = value
        return row
    def nonfinite(_):
        raise ControlError('Nonfinite bridge field')
    return json.loads(data, object_pairs_hook=pairs, parse_constant=nonfinite)


def profile(path, now=None):
    """Only parent-owned private admission; never creates/upgrades a profile."""
    value = strict_json(private.read_private(path, tuple(range(1, 16385))))
    if not isinstance(value, dict):
        raise ControlError('Invalid optical UART epoch')
    required_true = ('startup_reviewed', 'source_reviewed', 'recovery_reviewed',
                     'owners_reviewed', 'feed_retirement_qualified',
                     'controls_native_inert_qualified', 'bounded_optical_trial_admitted',
                     'uart_connected')
    aiming = value.get('schema') == 'optical-aim-epoch-v2'
    dispatch_pin = DISPATCH_PIN
    aim_sha = None
    if aiming:
        import aim_backend as aim
        if not aim.NATIVE_REVIEWED:
            raise ControlError('Aiming transport not yet admitted')
        required_true += ('white_native_inert_qualified', 'white_OFF_physically_confirmed',
                          'motor_native_metadata_reviewed', 'bounded_aim_trial_admitted')
        expected_pins = dict(aim_sha256=aim.TOOLSET_SHA, white_module_sha256=aim.WHITE_MODULE_PIN,
                             white_helper_sha256=aim.WHITE_HELPER_PIN, ptz_helper_sha256=aim.PTZ_HELPER_PIN)
        if any(value.get(k) != v for k, v in expected_pins.items()):
            raise ControlError('Aiming artifact pins not reviewed')
        dispatch_pin, aim_sha = aim.DISPATCH_PIN, aim.TOOLSET_SHA
    if (value.get('schema') not in ('optical-uart-epoch-v1', 'optical-aim-epoch-v2') or
            value.get('admission') != 'ADMITTED' or
            any(value.get(k) is not True for k in required_true) or
            value.get('kernel_sha256') != KERNEL_PIN or
            value.get('module_sha256') != MODULE_PIN or
            value.get('helper_sha256') != HELPER_PIN or
            value.get('dispatcher_sha256') != dispatch_pin or
            type(value.get('expected_shell_pid')) is not int or
            not 2 <= value['expected_shell_pid'] <= 4194304 or
            value.get('expected_pid1') not in ('sh', 'init') or
            not re.fullmatch(r'foundation-[0-9]{8}[a-z]+', value.get('recorder_part', ''))):
        raise ControlError('Unqualified optical UART epoch')
    epoch = Epoch(**{k: value[k] for k in
                    ('host', 'generation', 'boot_id', 'expires_at', 'kernel_sha256', 'helper_sha256')},
                  aim_sha256=aim_sha)
    now = time.time() if now is None else now
    epoch.check(now)
    if not 75 <= epoch.expires_at - now <= 600:
        raise ControlError('Optical UART epoch expired, near expiry or excessively long')
    private.private_path(value['capture_dir'], directory=True)
    private.private_path(value['diagnostics_dir'], directory=True)
    return value, epoch


def packet(action, request, settings):
    """Fixed actions and validated UUID/hash atoms only; no free-form shell."""
    request.epoch.check(time.time())
    aiming = request.epoch.aim_sha256 is not None
    if action != request.action or action not in ('status',) + (ALL_ACTIONS if aiming else ACTIONS):
        raise ControlError('Invalid optical action')
    if type(request.request_id) is not str or not re.fullmatch(
            r'[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}', request.request_id):
        raise ControlError('Invalid optical nonce')
    nonce = request.request_id.replace('-', '')
    expected = settings['expected_pid1']
    if expected not in ('sh', 'init'):
        raise ControlError('Invalid PID1 admission')
    shell = settings['expected_shell_pid']
    if type(shell) is not int or not 2 <= shell <= 4194304:
        raise ControlError('Invalid persistent shell identity')
    dispatch, dispatch_pin, prefix = DISPATCH, DISPATCH_PIN, 'OB'
    if aiming:
        import aim_backend as aim
        if not aim.NATIVE_REVIEWED or request.epoch.aim_sha256 != aim.TOOLSET_SHA:
            raise ControlError('Aiming request not admitted; no command sent')
        dispatch, dispatch_pin, prefix = aim.DISPATCH, aim.DISPATCH_PIN, 'AIM'
    cmd = (f'p20_dsum=$(/bin/busybox sha256sum {dispatch}); '
           f'if test "${{p20_dsum%% *}}" = {dispatch_pin}; '
           f'then . {dispatch} {action} {nonce} {request.epoch.boot_id} '
           f'{request.epoch.helper_sha256} {expected} {shell}; '
           f'else echo {prefix}_BEGIN_{nonce}; echo {prefix}_DONE_{nonce}:125; fi')
    if len(cmd.encode()) > 950 or any(c in cmd for c in '\r\n\0'):
        raise ControlError('Optical command exceeds reviewed line bound')
    return cmd.encode() + b'\r', nonce


STATE_FIELDS = ('abi setup fault owned_mask pair_latch pair_pad ir_latch ir_pad '
                'direction_mask mux_ok release_verified ir_active ir_remaining_ms '
                'cooldown_remaining_ms pulse_us ir_limit_ms sequence monotonic_ns').split()


def translate(state, request, validate=True):
    """A command's native snapshot, not process exit, supplies all release facts."""
    if (type(state) is not dict or set(state) != set(STATE_FIELDS) or
            any(type(state[k]) is not int or not 0 <= state[k] <=
                (0xffffffffffffffff if k in ('sequence', 'monotonic_ns') else 0xffffffff)
                for k in STATE_FIELDS)):
        raise ControlError('Invalid optical native snapshot')
    fixed = dict(abi=1, setup=1, fault=0, owned_mask=7, pair_latch=0, pair_pad=0,
                 direction_mask=7, mux_ok=1, release_verified=1, pulse_us=10000, ir_limit_ms=4900)
    if any(state[k] != v for k, v in fixed.items()):
        raise ControlError('Native release/setup not verified')
    active = state['ir_active']
    if (active not in (0, 1) or state['ir_latch'] != active or state['ir_pad'] != active or
            (not active and state['ir_remaining_ms'] != 0) or
            (active and not 1 <= state['ir_remaining_ms'] <= 4900)):
        raise ControlError('Native IR release/timer not verified')
    epoch = request.epoch
    row = dict(schema='optical-control-v1', request_id=request.request_id,
               generation=epoch.generation, boot_id=epoch.boot_id, host=epoch.host,
               action=request.action, completion='COMPLETE', admission='ADMITTED',
               kernel='kernel75', kernel_sha256=epoch.kernel_sha256,
               helper_sha256=epoch.helper_sha256, expires_at=epoch.expires_at,
               release='00', pulse_ms=10 if request.action in ('pulse-a', 'pulse-b') else 0,
               ir_state='ON' if active else 'OFF', timer_owner='target' if active else 'none',
               timer_remaining_ms=state['ir_remaining_ms'], timer_bound_ms=5000 if active else 0,
               proof='TARGET_TIMER_ARMED' if active else 'OFF_CONFIRMED')
    raw = json.dumps(row, separators=(',', ':')).encode()
    if validate:
        receipt(raw, request, time.time())
    return raw


def optical_native_state(native, nonce, action):
    errors = ('command_errno', 'snapshot_errno', 'transport_errno', 'keeper_errno',
              'verification_errno', 'close_errno')
    fields = {'action', 'nonce', 'result', 'ioctl_completed', 'keeper_pid',
              'lease_mode', 'state', *errors}
    if type(native) is not dict or set(native) != fields:
        raise ControlError('Camera CLI receipt schema mismatch')
    if (native['action'] != action or native['nonce'] != nonce or
            native['result'] != 'OK' or native['ioctl_completed'] is not True or
            any(type(native[k]) is not int or native[k] != 0 for k in errors) or
            type(native['keeper_pid']) is not int or native['keeper_pid'] != 0 or
            native['lease_mode'] != ('self' if action == 'ir-on' else 'none')):
        raise ControlError('Camera CLI completion/holder not verified')
    return native['state']


class UARTDriver:
    """Explicit sole-recorder bridge; construction does not query/transmit."""
    def __init__(self, path):
        self.path = Path(path)
        self.settings, self.epoch = profile(self.path)
        self._guard = threading.Lock()
        self._held = None
        self._fault = False
        self._clock = time.monotonic

    def status(self, request):
        return self._request('status', request)

    def act(self, action, request):
        return self._request(action, request)

    def _request(self, action, request):
        with self._guard:
            deadline = self._clock() + 7.0  # Retain HOLD before the UI's8s alert.
            if not NATIVE_SCHEMA_REVIEWED:
                raise NotAdmitted('Optical UART transport not yet admitted; no command sent')
            if self._fault:
                raise ControlError('Optical UART completion remains unknown')
            try:
                settings, epoch = profile(self.path)
                if settings != self.settings or epoch != request.epoch or epoch != self.epoch:
                    raise ControlError('Optical UART epoch changed')
                wire, nonce = packet(action, request, settings)
            except (ControlError, OSError, ValueError, KeyError, TypeError):
                raise NotAdmitted('Admission refused before camera TX') from None
            held = LOCK.open('a+')
            try:
                fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
                held.seek(0)
                if held.read(1):
                    raise ControlError('Unretired peripheral operation HOLD')
            except BaseException:
                held.close()
                raise
            self._held = held
            send_attempted = False
            intent_written = False
            b = None
            try:
                output = Path(settings['diagnostics_dir']) / nonce
                b = private.PrivateBoot(output, settings['capture_dir'])
                state = b.owner(margin=75)
                if (state['part'] != settings['recorder_part'] or not state.get('abort_armed') or
                        state.get('tx_busy') or not completed_shell_context(state['recent_rx'])):
                    raise ControlError('No admitted Linux console/recovery')
                if self._clock() >= deadline:
                    raise ControlError('Console admission exceeded completion budget')
                b.epoch_cursor = state['rx_bytes']
                # Durable intent BEFORE first byte. No exception clears it.
                held.write(json.dumps(dict(state='HOLD', request=nonce, action=action,
                                           generation=epoch.generation, boot_id=epoch.boot_id)) + '\n')
                held.flush()
                os.fsync(held.fileno())
                intent_written = True
                start = state['rx_bytes']
                for offset in range(0, len(wire), 64):
                    if self._clock() >= deadline:
                        raise ControlError('Optical request deadline reached; no continuation')
                    # Mark BEFORE RPC: an exception may follow a real write.
                    send_attempted = True
                    b.send(wire[offset:offset + 64], 'Explicit optical GUI kernel75 action')
                    if offset + 64 < len(wire):
                        time.sleep(.025)
                while self._clock() < deadline:
                    data = b.received(start)
                    if len(data) > 16384:
                        raise ControlError('Optical completion exceeded bound')
                    lines = data.replace(b'\r', b'').splitlines()
                    prefix = 'AIM' if request.epoch.aim_sha256 is not None else 'OB'
                    begin = (prefix + '_BEGIN_' + nonce).encode()
                    end_prefix = (prefix + '_DONE_' + nonce + ':').encode()
                    end = end_prefix + b'0'
                    endings = [line for line in lines
                               if line.startswith(end_prefix)]
                    if endings and endings != [end]:
                        raise ControlError('Optical command completion contradicts itself')
                    if end in lines:
                        if lines.count(end) != 1 or lines.count(begin) != 1:
                            raise ControlError('Optical completion markers disagree')
                        first = lines.index(begin)
                        last = lines.index(end)
                        body = lines[first + 1:last]
                        if prefix == 'AIM':
                            from aim_backend import translate_body
                            raw = translate_body(body, nonce, request)
                        else:
                            if len(body) != 1:
                                raise ControlError('Unexpected native optical completion')
                            raw = self._native(strict_json(body[0]), nonce, request)
                        if self._clock() >= deadline:
                            raise ControlError('Late completion retained for parent review')
                        b.log('optical_completion_verified', action=action, nonce=nonce)
                        held.seek(0)
                        held.truncate()
                        held.flush()
                        os.fsync(held.fileno())
                        held.close()
                        self._held = None
                        return raw
                    if any(line.startswith(end_prefix) for line in lines):
                        raise ControlError('Optical command did not prove completion')
                    time.sleep(.02)
                raise ControlError('Optical completion unknown; no retry')
            except BaseException:
                if not send_attempted:
                    # This request made no send RPC, so no effect can be its
                    # unknown outcome. Clear only this owned no-send intent.
                    if intent_written:
                        held.seek(0)
                        own_intent = strict_json(held.read())
                        if own_intent.get('request') != nonce:
                            self._fault = True
                            raise ControlError('Foreign HOLD intent; parent review required') from None
                        held.seek(0)
                        held.truncate()
                        held.flush()
                        os.fsync(held.fileno())
                    try:
                        if b is not None:
                            b.log('optical_preTX_denied_no_command_sent', action=action, nonce=nonce)
                    except Exception:
                        pass  # Logging failure cannot invent a target TX attempt.
                    finally:
                        held.close()
                        self._held = None
                    raise NotAdmitted('No camera command sent; admission denied') from None
                # Keep even an exited helper's ownership uncertain. The durable
                # intent survives process death; no GUI clears or retries it.
                self._fault = True
                raise ControlError('Optical UART HOLD; parent retirement/recovery required') from None

    @staticmethod
    def _native(native, nonce, request):
        return translate(optical_native_state(native, nonce, request.action), request)
