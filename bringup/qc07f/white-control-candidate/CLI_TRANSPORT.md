# GPIO5-only white transport, ABI 1

Offline candidate, **not hardware-admitted**. Parent alone owns target/runtime,
UART, private inputs, effect admission and GUI integration. No networking,
stock ABI/service/guardian or motor control is included. Existing optical source,
module, CLI, dispatcher, policy and GUI are untouched.

## Fixed interface

Parent staging paths (not staged by this unit): module `qc07f_white.ko`, helper
`/tmp/qc07f-white-cli`. Module registers `/dev/qc07f-white`, mode **0600**.
Open and every ioctl require effective UID0 plus CAP_SYS_RAWIO in init_user_ns.
Only these exact ioctl constants are supported; effect argument must be zero:

| ioctl | value | argument |
|---|---:|---|
| QW_STATUS | `0x80405170` | pointer to 64-byte `struct qw_status` |
| QW_OFF | `0x00005171` | zero |
| QW_ON | `0x00005172` | zero |

`white_uapi.h` is canonical. All fields are native-endian fixed width, with no
pointers/padding. Twelve u32 fields in this exact order:
`abi, flags, fault, owned, white_latch, white_pad, direction, mux_ok,
remaining_ms, limit_ms, prior_pad, prior_direction`; then u64 `sequence` (offset48),
u64 `monotonic_ns` (offset56). Size/offset/ioctl compile-time assertions exist.

Flags: SETUP=1, ACTIVE=2, PRIOR_VALID=4, OFF_VERIFIED=8. `owned` is boolean GPIO5
reservation, **not a bank mask**. Latch/pad/direction are GPIO5-only boolean
readback; direction1=output. Mux_ok means `(SYS74 & 0x60)==0`.
`limit_ms` always4900; `remaining_ms` is ceil(max(deadline-now,0)/1ms).
ACTIVE may remain1 with remaining0 if a due callback has not run. Do not infer
OFF from zero remaining. `prior_pad/prior_direction` describe the immediately
pre-effect sample of the most recent admitted explicit OFF, only when PRIOR_VALID.
They report initial HIGH/input without requiring it to be LOW first.
Sequence increments for admitted OFF, executed ON (including failed verification),
active expiry or ON-owner close. Refused commands/status do not increment it.
Fault bits: CONFIG=1 (mux/direction), READBACK=2, GPIO=4 (API error); sticky until
module replacement. OFF_VERIFIED is current configured-low readback, not a
fault-clear or optical/current measurement: it can coexist with a sticky fault.

### CLI argv and JSON

Exactly three argv entries including executable:

```text
/tmp/qc07f-white-cli status LOWERCASE32HEX_NONCE
/tmp/qc07f-white-cli white-off LOWERCASE32HEX_NONCE
/tmp/qc07f-white-cli --white-on-hold LOWERCASE32HEX_NONCE
```

There is **no** direct `white-on`, setup, close, arbitrary GPIO, duration or
fork mode. Actions in JSON are `status`, `white-off`, `white-on`, or null for an
unrecognized argument. Bad nonce becomes null; unsafe strings are never echoed.
Every complete receipt is one LF-terminated JSON object, <1536 bytes, exact keys:

```json
{"action":"white-off","nonce":"0123456789abcdef0123456789abcdef","result":"OK","ioctl_completed":true,"command_errno":0,"snapshot_errno":0,"transport_errno":0,"verification_errno":0,"close_errno":0,"lease_mode":"none","state":{"abi":1,"flags":13,"fault":0,"owned":1,"white_latch":0,"white_pad":0,"direction":1,"mux_ok":1,"remaining_ms":0,"limit_ms":4900,"prior_pad":1,"prior_direction":0,"sequence":1,"monotonic_ns":123456789}}
```

This is a **schema example, not native evidence**. `state` is null if no complete
successful STATUS ioctl was obtained; otherwise it contains **all fourteen** UAPI
fields, including unknown ABI values for diagnostics. Integers are unsigned UAPI
integers; preserve u64 exactly (no JSON floating-point roundtrip).
`ioctl_completed` is boolean: true means effect ioctl returned0, or STATUS returned0
even if its ABI was unknown. It is **not** proof of electrical/optical completion.
All errno keys are nonnegative integers. Positive unexpected syscall results map
to EPROTO. No error/unknown/EINTR effect is ever replayed; at most one diagnostic
STATUS follows an effect. `result` is `OK` or `FAILED`; process return normally0/1.
Status `OK` means transport/ABI success only: read actual flags/fault/mux/ownership.

Successful OFF additionally requires flags13, fault0, owned1, low latch+pad,
direction1, mux_ok1, remaining0, limit4900 and valid boolean prior values.
Successful ON requires flags7 and the corresponding HIGH latch+pad,
0<remaining<=4900. Failed command/snapshot/verification closes the fd immediately.

`lease_mode` is `self` only for parsed `--white-on-hold` (even a later failure),
otherwise `none`. No PID or keeper process/lease exists. On verified ON the helper
emits the receipt **before** isolation/holding, then issues two `close_range`
calls excluding only the same open device fd, and absolute CLOCK_MONOTONIC sleep
until original **pre-IOCTL** monotonic time+5s. No fork, exec, re-open, ioctl,
signal or output occurs in the holder. At most64 EINTR sleep attempts, one close,
then `_exit`; close_range/sleep failure exits early. Kernel75 close_range is
required; there is no guessed FD limit fallback. No standard FD may alias device
(fd<3 is refused). Device is fixed O_NOFOLLOW|O_CLOEXEC, fstat S_ISCHR.
Output has a16-syscall budget; partial/failed ON output closes early with no replay
or second receipt. Blocking output/scheduling is not bounded by userspace alone.

Parent must launch ON as a **direct job of the owned persistent shell**, retain
and reap it; do not orphan it or use a PID as readiness/lease proof. Positive JSON
describes its pre-hold snapshot, not future safety. An isolation/sleep/close failure
after that receipt is not retroactively reported. Parent readback is still needed.

## GPIO and lifecycle contract

Exact provider checks follow the existing optical consumer: singleton compatible
`anyka,ak3918ev200-gpio`, available OF node, reg `0x20170000/0x100`, linked sysctrl
compatible `anyka,ak3918ev200-intc`+`syscon`, reg `0x08000000/0x100`, driver
`ak3918ev200-gpio`, ngpio79, non-cansleep and unique lookup label; acquired
descriptor's provider must match. Only GPIO offset5, ACTIVE_HIGH/PERSISTENT,
normal exclusive ASIS gpiolib request. Root consumer has no default pinctrl state.

Aliases read only SYS74 mux mask0x60/value0; bank0 DIR+0x00, OUT+0x0c and IN+0x18
bit5. **No raw MMIO stores.** No remux/pulls/clocks or GPIO0/1/2/6/7/23/41/42
access. Exact provider source uses a shared raw-spinlocked per-bit OUT/DIR RMW;
neighbour bits are preserved alongside capture and optical activity. Its request
refuses wrong mux, direction_output writes LOW latch before enabling OE.
Normal lookup flags request no bias/drive changes; unsupported push-pull config
is ignored by gpiolib and exact driver's set_config rejects it before any store.

Load and STATUS are electrically inert: no reservation, output or active timer.
Explicit OFF may reserve and establish output LOW even if initial input/pad HIGH.
Foreign owner/wrong mux refuse before output writes; no forced claim or remux.
After effects all reservations remain until **parent-reviewed unload**, including
OFF, owner close, API/readback fault and uncertain pad. Sticky fault blocks ON;
OFF continues trying LOW only on the owned line. Owned wrong-mux OFF clears its
latch but does not change direction or mux. It reports failure, never repair.
STATUS can latch a diagnostic sticky fault after setup but performs no repair.
OFF also checks its pre-effect sample after a prior verified setup, preserving
unexpected config/readback drift as a sticky fault even if LOW is re-established.
This does not reject initial input/HIGH as a fault before first setup.

ON requires a previously verified OFF/setup, no source fault, neutral current
readback and no active interval. A CLOCK_MONOTONIC **hard hrtimer is armed BEFORE
HIGH**, nominal deadline4900ms. No ON extension/rearm while active, even past a
deadline whose callback is delayed. Callback only takes effect raw spinlock,
clears owned GPIO5, verifies pad/latch/direction/mux and retires owner/deadline.
Synchronous cancellation is outside effect_lock. Callback never takes command
mutex. Closing an unrelated/status fd is electrically inert; closing **white ON
owner fd** clears/verifies white and cancels its timer. The separate optical IR
owner and all existing optical semantics are untouched.

Unload: fops.owner blocks ordinary unload with open fds. Forced unload is NOT
supported. Exit cancels, clears/verifies owned GPIO5, logs UNKNOWN on source fault,
then releases its own line/metadata only. Void module_exit cannot refuse failure:
**reservation quarantine does not survive a parent-reviewed unload**. Do not
unload on uncertain state as an automatic repair/rearm. No restoration of initial
HIGH/input is attempted (white OFF is the intentional final state).

Exact75 config is HZ100, HIGH_RES_TIMERS disabled, PREEMPT_NONE. **Neither nominal
4900ms kernel expiry nor5s user deadline proves electrical <=5s**, pad voltage,
optical light, arbitrary-hang protection, current/thermal safety or native capture
coexistence. Software cannot guarantee shutoff with IRQ/bus/CPU hangs. Native
inert/OFF/ON/expiry/close/fault/coexistence and physical observations remain OPEN.

## Offline reproduction

```text
python3 -B bringup/qc07f/white-control-candidate/build_offline.py runNN
```

Fresh `evidence/delegated/20261020-white/runNN` only; no overwrite. Builds original
source as M=external module with copied metadata O= and MO=own output. Whole host,
including sealed kernel/build/source, is read-only under bubblewrap; only fresh
evidence output is writable, network namespace unshared. No modules_prepare,
configuration/full-kernel target or ARM execution. Bootlin pinned static compiler
path comes from the optical CLI builder. Reports retain argv/tool versions/hashes,
all source/prepared metadata hashes, artifact hashes and failures. Shared-policy
tests explicitly simulate GPIO; actual CLI implementation has all external
syscalls mocked. Optimized and ASan/UBSan variants run only x86-64 host test ELF.
