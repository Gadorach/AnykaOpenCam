"""Fail-closed injectable optical adapter contract. NO remote transport exists here."""
from dataclasses import dataclass
import json
import math
import queue
import re
import threading
import time
import uuid
from typing import Protocol

from optical_video import admitted_host

ACTIONS = ("pulse-a", "pulse-b", "ir-on", "ir-off")
AIM_ACTIONS = ("white-on", "white-off", "motor0-minus", "motor0-plus", "motor1-minus", "motor1-plus")
ALL_ACTIONS = ACTIONS + AIM_ACTIONS
ADAPTER_PENDING = "ADAPTER NOT YET ADMITTED — parent live endpoint integration required"
UNKNOWN = "UNKNOWN completion — sticky fault; no retries. Parent retirement/recovery required."


class ControlError(ValueError):
    pass


class NotAdmitted(ControlError):
    """Trusted transport proof that this request attempted no target TX."""
    pass


def _uuid(value):
    return type(value) is str and bool(re.fullmatch(
        r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}", value))


def _pin(value):
    return type(value) is str and bool(re.fullmatch(r"[0-9a-f]{64}", value))


@dataclass(frozen=True)
class Epoch:
    host: str
    generation: str
    boot_id: str
    expires_at: int
    kernel_sha256: str
    helper_sha256: str
    aim_sha256: str | None = None

    def check(self, now, minimum_seconds=10):
        try:
            host = admitted_host(self.host)  # No loopback control admission.
        except ValueError:
            raise ControlError("Invalid parent epoch host") from None
        if (host != self.host or not _uuid(self.generation) or not _uuid(self.boot_id) or
                type(self.expires_at) is not int or not math.isfinite(now) or
                self.expires_at - now < minimum_seconds or not _pin(self.kernel_sha256) or
                not _pin(self.helper_sha256) or
                (self.aim_sha256 is not None and not _pin(self.aim_sha256))):
            raise ControlError("Parent epoch missing, expired or invalid")


@dataclass(frozen=True)
class Request:
    request_id: str
    epoch: Epoch
    action: str


class Driver(Protocol):
    """Parent-injected only; methods return bounded UTF-8 receipt bytes.

    Both calls must return within eight seconds. act() must use the existing
    shared operation lock/HOLD protocol and parent-admitted pinned endpoint;
    NotAdmitted is reserved for proven pre-TX refusal, never native uncertainty.
    Never retry on exceptions.
    """
    def status(self, request: Request) -> bytes: ...
    def act(self, action: str, request: Request) -> bytes: ...


class UnavailableDriver:
    def status(self, request):
        raise NotAdmitted(ADAPTER_PENDING)

    def act(self, action, request):
        raise NotAdmitted(ADAPTER_PENDING)


def receipt(raw, request, now):
    """Exact keys/types, no duplicate JSON keys, nonfinite or unknown states."""
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ControlError("Duplicate receipt key")
            result[key] = value
        return result

    def nonfinite(_):
        raise ControlError("Nonfinite receipt value")

    if type(raw) is not bytes or not 1 <= len(raw) <= 4096:
        raise ControlError("Receipt byte bound/type invalid")
    try:
        row = json.loads(raw, object_pairs_hook=pairs, parse_constant=nonfinite)
    except (ValueError, UnicodeError, RecursionError):
        raise ControlError("Malformed optical receipt") from None
    fields = {"schema", "request_id", "generation", "boot_id", "host", "action",
              "completion", "admission", "kernel", "kernel_sha256", "helper_sha256",
              "expires_at", "release", "pulse_ms", "ir_state", "timer_owner",
              "timer_remaining_ms", "timer_bound_ms", "proof"}
    extended = type(row) is dict and row.get('schema') == 'optical-control-v2'
    if extended:
        fields |= {'aim_sha256', 'white_state', 'white_timer_owner', 'white_timer_remaining_ms',
                   'white_timer_bound_ms', 'white_proof', 'motors', 'motor_effect'}
    if type(row) is not dict or set(row) != fields:
        raise ControlError("Receipt schema mismatch")
    if extended != (request.epoch.aim_sha256 is not None):
        raise ControlError('Aiming receipt/epoch downgrade or capability mismatch')
    request.epoch.check(now)
    expected = {"schema": "optical-control-v2" if extended else "optical-control-v1", "request_id": request.request_id,
                "generation": request.epoch.generation, "boot_id": request.epoch.boot_id,
                "host": request.epoch.host, "action": request.action,
                "completion": "COMPLETE", "admission": "ADMITTED", "kernel": "kernel75",
                "kernel_sha256": request.epoch.kernel_sha256,
                "helper_sha256": request.epoch.helper_sha256,
                "expires_at": request.epoch.expires_at, "release": "00",
                "pulse_ms": 10 if request.action in ("pulse-a", "pulse-b") else 0}
    if (request.action not in ("status",) + (ALL_ACTIONS if extended else ACTIONS) or not _uuid(request.request_id) or
            any(type(row[key]) is not type(value) or row[key] != value
                for key, value in expected.items())):
        raise ControlError("Receipt request/epoch/action/release mismatch")
    if (type(row["timer_remaining_ms"]) is not int or type(row["timer_bound_ms"]) is not int):
        raise ControlError("Receipt timer must use integer milliseconds")
    off = (row["ir_state"] == "OFF" and row["timer_owner"] == "none" and
           row["timer_remaining_ms"] == 0 and row["timer_bound_ms"] == 0 and
           row["proof"] == "OFF_CONFIRMED")
    on = (row["ir_state"] == "ON" and row["timer_owner"] == "target" and
          1 <= row["timer_remaining_ms"] <= 5000 and row["timer_bound_ms"] == 5000 and
          row["proof"] == "TARGET_TIMER_ARMED")
    if not (off or on) or (request.action == "ir-on" and not on) or (
            request.action == "ir-off" and not off):
        raise ControlError("Contradictory IR state or missing target auto-off proof")
    if extended:
        if row['aim_sha256'] != request.epoch.aim_sha256:
            raise ControlError('Aiming helper/source epoch mismatch')
        if any(type(row[k]) is not int for k in ('white_timer_remaining_ms', 'white_timer_bound_ms')):
            raise ControlError('White timer uses integer milliseconds')
        white_off = (row['white_state'] == 'OFF' and row['white_timer_owner'] == 'none' and
                     row['white_timer_remaining_ms'] == row['white_timer_bound_ms'] == 0 and
                     row['white_proof'] == 'OFF_CONFIRMED')
        white_on = (row['white_state'] == 'ON' and row['white_timer_owner'] == 'target' and
                    1 <= row['white_timer_remaining_ms'] <= 5000 and row['white_timer_bound_ms'] == 5000 and
                    row['white_proof'] == 'TARGET_TIMER_ARMED')
        if (not (white_off or white_on) or (request.action == 'white-on' and not white_on) or
                (request.action == 'white-off' and not white_off)):
            raise ControlError('White release/target timer not verified')
        validate_motors(row['motors'], row['motor_effect'], request.action)
    return row


def validate_motors(motors, effect, action):
    fields = {'axis', 'issued_steps', 'phase', 'result', 'coil_readback_ok',
              'coils_off_verified', 'max_pulse_steps', 'period_ms', 'physical_position_known', 'health'}
    if type(motors) is not list or len(motors) != 2:
        raise ControlError('Both motor metadata snapshots required')
    for axis, motor in enumerate(motors):
        if (type(motor) is not dict or set(motor) != fields or
                any(type(motor[k]) is not int for k in fields - {'health'}) or
                motor['axis'] != axis or not 0 <= motor['issued_steps'] <= 0xffffffff or
                not 0 <= motor['phase'] <= 3 or motor['result'] != 0 or
                motor['max_pulse_steps'] != 4 or motor['period_ms'] != 20 or
                motor['physical_position_known'] != 0):
            raise ControlError('Motor identity/bound/position metadata mismatch')
        cold = (motor['issued_steps'] == motor['phase'] == motor['coil_readback_ok'] ==
                motor['coils_off_verified'] == 0 and motor['health'] == 'UNTESTED')
        ready = (motor['coil_readback_ok'] == motor['coils_off_verified'] == 1 and motor['health'] == 'READY')
        if not (cold or ready):
            raise ControlError('Motor is not cold-untested or positively retired')
    move = re.fullmatch(r'motor([01])-(minus|plus)', action)
    if move is None:
        if effect is not None:
            raise ControlError('Unexpected motor effect')
        return
    axis, sign = int(move[1]), -1 if move[2] == 'minus' else 1
    fields = {'axis', 'requested_steps', 'before_issued_steps', 'after_issued_steps',
              'issued_delta', 'coils_off_verified'}
    if (type(effect) is not dict or set(effect) != fields or
            any(type(effect[k]) is not int for k in fields - {'coils_off_verified'}) or
            effect['coils_off_verified'] is not True or effect['axis'] != axis or
            effect['requested_steps'] != sign * 4 or effect['issued_delta'] != 4 or
            not 0 <= effect['before_issued_steps'] <= 0xffffffff - 4 or
            effect['after_issued_steps'] != effect['before_issued_steps'] + 4 or
            effect['after_issued_steps'] != motors[axis]['issued_steps'] or
            motors[axis]['health'] != 'READY'):
        raise ControlError('One four-step move/retirement proof required')


class Controls:
    """UI-thread state machine; adapter and pure receipt validation run on a worker.

    Preview discontinuities revoke status admission even if a pending status
    later completes. Timeout/uncertain completion is sticky for this instance;
    fresh video/status cannot clear it. No automatic act/status/OFF/kill exists.
    """
    def __init__(self, driver=None, epoch=None, *, clock=time.monotonic, wall=time.time):
        self.driver = driver if driver is not None else UnavailableDriver()
        self.epoch = epoch
        self.clock, self.wall = clock, wall
        self.owner = threading.get_ident()
        self.results = queue.Queue(maxsize=1)
        self.thread = None
        self.busy = False
        self.fault = False
        self.admitted = False
        self.ready = False
        self.session = None
        self.revision = 0
        self.pending = None
        self.deadline = 0.0
        self.cooldown = 0.0
        self.ir_until = 0.0
        self.white_until = 0.0
        self.aim_admitted = False
        self.last_receipt = None
        self.message = (ADAPTER_PENDING if epoch is None or isinstance(self.driver, UnavailableDriver) else
                        "Connect preview, then Read controls status — bounded optical trial only")

    def _ui(self):
        if threading.get_ident() != self.owner:
            raise RuntimeError("Controls state belongs to the UI thread")

    def observe(self, preview, host):
        self._ui()
        valid = preview.ready and self.epoch is not None and host == self.epoch.host
        if not valid or preview.session != self.session:
            if self.ready or self.admitted or preview.session != self.session:
                self.revision += 1
            self.admitted = False
            self.aim_admitted = False
        self.session, self.ready = preview.session, valid
        if self.epoch is not None:
            try:
                self.epoch.check(self.wall(), minimum_seconds=90)
            except ControlError:
                self.admitted = False
                self.ready = False
                if not self.fault:
                    self.message = "Parent epoch expired/near expiry — Close safely and ask for fresh admission"

    def allowed(self, action):
        self._ui()
        if self.busy or self.fault or not self.ready or self.epoch is None:
            return False
        try:
            self.epoch.check(self.wall(), minimum_seconds=90)
        except ControlError:
            return False
        if action == "status":
            return True
        if action in AIM_ACTIONS and not self.aim_admitted:
            return False
        # OFF has independent safety priority over cooldown / active ON timer,
        # but never bypasses serialization, fresh preview or a sticky fault.
        return (action in ALL_ACTIONS and self.admitted and
                (action in ('ir-off', 'white-off') or (self.clock() >= self.cooldown and
                 (action != "ir-on" or not self.ir_until) and
                 (action != "white-on" or not self.white_until))))

    def dispatch(self, action):
        self._ui()
        if self.epoch is None or isinstance(self.driver, UnavailableDriver):
            self.message = ADAPTER_PENDING
            return False
        if not self.allowed(action):
            return False
        request = Request(str(uuid.uuid4()), self.epoch, action)
        self.pending = (request, self.revision, self.session)
        self.busy = True
        self.deadline = self.clock() + 8
        self.message = "Checking controls status" if action == "status" else "One bounded action pending"

        def work():
            try:
                raw = (self.driver.status(request) if action == "status" else
                       self.driver.act(action, request))
                # Validate at actual completion, not when a delayed Tk event
                # loop eventually polls. No GUI state/widget access here.
                row = receipt(raw, request, self.wall())
                result = (True, row, self.clock())
            except NotAdmitted:
                # Trusted adapters use this ONLY before any target TX attempt.
                result = (None, None, self.clock())
            except Exception:
                result = (False, None, self.clock())  # No private diagnostics.
            self.results.put_nowait(result)

        self.thread = threading.Thread(target=work, daemon=True)
        self.thread.start()
        return True

    def poll(self):
        self._ui()
        if not self.busy:
            return
        try:
            ok, row, completed_at = self.results.get_nowait()
        except queue.Empty:
            if not self.fault and self.clock() >= self.deadline:
                self.fault, self.admitted = True, False
                self.message = UNKNOWN
            return
        if ok is None:
            # Proven no-send denial can retire even after an alert; it cannot
            # come from a native/post-TX failure or a fresh healthy status.
            self.busy = self.fault = self.admitted = self.aim_admitted = False
            self.pending = None
            self.message = "Not admitted — no camera command sent. Close safely; request fresh admission."
            return
        if self.fault:
            return  # Actual timeout/unknown stays sticky despite late replies.
        if completed_at >= self.deadline:
            self.fault, self.admitted = True, False
            self.message = UNKNOWN
            return
        request, revision, session = self.pending
        if not ok:
            self.fault, self.admitted = True, False
            self.message = UNKNOWN
            return
        self.busy = False
        self.pending = None
        self.last_receipt = row
        if request.action == "status":
            self.admitted = (self.ready and revision == self.revision and session == self.session)
            self.aim_admitted = self.admitted and row['schema'] == 'optical-control-v2'
        self.ir_until = (self.clock() + row["timer_remaining_ms"] / 1000
                         if row["ir_state"] == "ON" else 0.0)
        self.white_until = (self.clock() + row.get('white_timer_remaining_ms', 0) / 1000
                            if row.get('white_state') == 'ON' else 0.0)
        if request.action != "status":
            self.cooldown = self.clock() + 2
        self.message = ("Receipt verified; optical result still requires observation" if self.admitted else
                         "Preview admission lost; manual controls-status required")
        if self.admitted and row.get('motor_effect') is not None:
            effect = row['motor_effect']
            self.message = (f"Motor{effect['axis']} issued {effect['requested_steps']:+d} raw steps; "
                            "coils-off verified. Confirm scene movement before more; no position/limit proof.")

    def close_allowed(self):
        self._ui()
        # Parent bridge, when implemented, must preserve a cross-process HOLD on
        # unknown completion. UI will not silently abandon a pending adapter.
        return not self.busy and not self.fault
