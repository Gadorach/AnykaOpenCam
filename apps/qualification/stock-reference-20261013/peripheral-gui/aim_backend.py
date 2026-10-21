"""Strict typed composition of existing CIR/IR, white and bounded PTZ receipts.

No transport, processes, file/device access or effect on import. v2 pins add to,
not replace, the working optical v1 contract. Parent native admission is separate.
"""
import hashlib
import json
import re
import time

from optical_control import (ACTIONS, ALL_ACTIONS, ControlError, receipt, validate_motors)

DISPATCH = '/tmp/qc07f-aim-dispatch.sh'
DISPATCH_PIN = 'd6ccd758c0708ee4fd1758493400b0fd158bf914a4cd812a37b0d1dcd47dc2c5'
WHITE_MODULE_PIN = '195c05ad2421894d8abc796d471c88ede64082717658cc58603f120965d21110'
WHITE_HELPER_PIN = 'd08cb896298cd646c16ddcf9df86802b30d6f00dc5261a60750c94df8d694697'
PTZ_HELPER_PIN = '6ee85b38354f8c9a58e963857cb0f6d3a5cf60ea412587200be8e904e9e7fdd4'
PINS = dict(dispatcher=DISPATCH_PIN, white_module=WHITE_MODULE_PIN,
            white_helper=WHITE_HELPER_PIN, ptz_helper=PTZ_HELPER_PIN,
            optical_dispatcher='cfc10dea3fff5a3fb434a98053d5113a3573b5a7bc9ff2b11778853eb8d63df6',
            optical_module='b839958867cfbf89877a17cfc72a448db12b3703f13bf3528faaa720a6a3a61e',
            optical_helper='eaded5d3827ec9ee073cc09efb0904b06f6f7d073405bb7bfc3e757c5db4f128')
TOOLSET_SHA = hashlib.sha256(json.dumps(PINS, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
# Native20a inert/status/white-OFF and exact sourced composition were verified.
# A fresh v2 epoch still admits only bounded manual trials, not motor mechanics,
# travel limits, timed white-ON electrical bounds or product acceptance.
NATIVE_REVIEWED = True


def white_state(native, nonce, action):
    errors = ('command_errno', 'snapshot_errno', 'transport_errno', 'verification_errno', 'close_errno')
    keys = {'action', 'nonce', 'result', 'ioctl_completed', 'lease_mode', 'state', *errors}
    if (type(native) is not dict or set(native) != keys or native['action'] != action or
            native['nonce'] != nonce or native['result'] != 'OK' or native['ioctl_completed'] is not True or
            native['lease_mode'] != ('self' if action == 'white-on' else 'none') or
            any(type(native[k]) is not int or native[k] != 0 for k in errors)):
        raise ControlError('White command/receipt completion not verified')
    state = native['state']
    fields = {'abi', 'flags', 'fault', 'owned', 'white_latch', 'white_pad', 'direction', 'mux_ok',
              'remaining_ms', 'limit_ms', 'prior_pad', 'prior_direction', 'sequence', 'monotonic_ns'}
    if (type(state) is not dict or set(state) != fields or
            any(type(state[k]) is not int or not 0 <= state[k] <=
                (0xffffffffffffffff if k in ('sequence', 'monotonic_ns') else 0xffffffff) for k in fields)):
        raise ControlError('White status ABI/types invalid')
    if (state['abi'] != 1 or state['fault'] != 0 or state['owned'] != 1 or state['direction'] != 1 or
            state['mux_ok'] != 1 or state['limit_ms'] != 4900 or
            state['prior_pad'] not in (0, 1) or state['prior_direction'] not in (0, 1)):
        raise ControlError('White ownership/config/fault not admitted')
    off = state['flags'] == 13 and state['white_latch'] == state['white_pad'] == state['remaining_ms'] == 0
    on = (state['flags'] == 7 and state['white_latch'] == state['white_pad'] == 1 and
          1 <= state['remaining_ms'] <= 4900)
    if not (off or on) or (action == 'white-on' and not on) or (action == 'white-off' and not off):
        raise ControlError('White OFF/readback or target timer not verified')
    return state, on


def motor_state(native, nonce, action):
    fields = {'schema', 'action', 'nonce', 'result', 'stage', 'requested_steps',
              'write_attempted', 'write_completed', 'hold', 'errno', 'diagnostic_errno', 'before', 'after'}
    if (type(native) is not dict or set(native) != fields or native['schema'] != 'qc07f.ptz.sysfs.v1' or
            native['nonce'] != nonce or native['action'] != action or native['result'] != 'OK' or
            any(type(native[k]) is not bool for k in ('write_attempted', 'write_completed', 'hold')) or
            native['hold'] or any(type(native[k]) is not int or native[k] != 0 for k in ('errno', 'diagnostic_errno')) or
            type(native['requested_steps']) is not int):
        raise ControlError('Motor completion/identity/hold not verified')
    before, after = native['before'], native['after']
    validate_motors(before, None, 'status')
    if action == 'status':
        if (native['stage'] != 'status' or native['requested_steps'] != 0 or
                native['write_attempted'] or native['write_completed'] or after != [None, None]):
            raise ControlError('Motor status must be purely read-only')
        return before, None
    match = re.fullmatch(r'motor([01])-(minus|plus)', action)
    if match is None:
        raise ControlError('Fixed motor action required')
    axis, sign = int(match[1]), -1 if match[2] == 'minus' else 1
    if (native['stage'] != 'complete' or native['requested_steps'] != sign * 4 or
            not native['write_attempted'] or not native['write_completed']):
        raise ControlError('Exactly one four-step write must complete')
    validate_motors(after, None, 'status')
    if (after[1 - axis] != before[1 - axis] or after[axis]['phase'] != before[axis]['phase'] or
            after[axis]['health'] != 'READY' or before[axis]['issued_steps'] > 0xffffffff - 4 or
            after[axis]['issued_steps'] != before[axis]['issued_steps'] + 4):
        raise ControlError('Motor counter/phase/other-axis/retirement changed unexpectedly')
    effect = dict(axis=axis, requested_steps=sign * 4,
                  before_issued_steps=before[axis]['issued_steps'],
                  after_issued_steps=after[axis]['issued_steps'], issued_delta=4, coils_off_verified=True)
    validate_motors(after, effect, action)
    return after, effect


def translate_body(body, nonce, request):
    from optical_uart import strict_json, optical_native_state, translate
    if request.epoch.aim_sha256 != TOOLSET_SHA or request.action not in ('status',) + ALL_ACTIONS:
        raise ControlError('Aiming toolset/request epoch not bound')
    expected = ('OB_BEGIN_' + nonce, 'OB_DONE_' + nonce + ':0',
                'WHITE_BEGIN_' + nonce, 'WHITE_DONE_' + nonce + ':0',
                'PTZ_BEGIN_' + nonce, 'PTZ_DONE_' + nonce + ':0')
    if len(body) != 9 or any(body[index] != value.encode() for index, value in zip((0, 2, 3, 5, 6, 8), expected)):
        raise ControlError('Complete exact three-component aiming frame required')
    optical, white, ptz = (strict_json(body[i]) for i in (1, 4, 7))
    action = request.action
    opt_action = action if action in ACTIONS else 'status'
    w_action = action if action in ('white-on', 'white-off') else 'status'
    m_action = action if action.startswith('motor') else 'status'
    opt = optical_native_state(optical, nonce, opt_action)
    ws, white_on = white_state(white, nonce, w_action)
    motors, effect = motor_state(ptz, nonce, m_action)
    normalized = json.loads(translate(opt, request, validate=False))
    normalized.update(schema='optical-control-v2', aim_sha256=TOOLSET_SHA,
                      white_state='ON' if white_on else 'OFF',
                      white_timer_owner='target' if white_on else 'none',
                      white_timer_remaining_ms=ws['remaining_ms'], white_timer_bound_ms=5000 if white_on else 0,
                      white_proof='TARGET_TIMER_ARMED' if white_on else 'OFF_CONFIRMED',
                      motors=motors, motor_effect=effect)
    raw = json.dumps(normalized, separators=(',', ':')).encode()
    receipt(raw, request, time.time())
    return raw
