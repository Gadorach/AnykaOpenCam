# QC07F fixed PTZ sysfs transport v1

Own source, SPDX GPL-2.0-or-later. CTRL-02/03 bounded source increment only;
not product PTZ, calibrated position, native timing, or physical movement proof.
No old kernel, driver, GPIO, optical, white-light, GUI or sealed tree was changed.

## Fixed ABI and invocation

Stage only after parent admission as `/tmp/qc07f-ptz-cli`. Exact argv:

```
/tmp/qc07f-ptz-cli status LOWERCASE_32_HEX_NONCE
/tmp/qc07f-ptz-cli motor0-minus LOWERCASE_32_HEX_NONCE
/tmp/qc07f-ptz-cli motor0-plus LOWERCASE_32_HEX_NONCE
/tmp/qc07f-ptz-cli motor1-minus LOWERCASE_32_HEX_NONCE
/tmp/qc07f-ptz-cli motor1-plus LOWERCASE_32_HEX_NONCE
```

No options, caller paths, counts, rates or GPIO numbers. No startup/status action
writes any target file. Move payload is exactly `-4\n` or `4\n`, written exactly
once to one `pulse` fd. Never split/retry/chain writes, even EINTR, short, zero or
errno. No homing, STOP/OFF, continuous motion, absolute coordinates or sweep API.
GUI labels: **Motor0/pan raw -4/+4**, **Motor1/tilt raw -4/+4**. Retail physical
axis/sign history is not proof of the current own-kernel mapping/mechanics.

Fixed paths (source-pinned; subsequently confirmed read-only by parent):
`/sys/bus/platform/devices/soc:ptz-axis0` and `soc:ptz-axis1` under the same directory.
Both identities are validated before even opening `pulse`: SYSFS_MAGIC, root-owned
directories/regular leaves without group/other write bits, driver link basename
`qc07f-ptz-diagnostic`, exact NUL-terminated compatible
`anyka,qc07f-ptz-diagnostic`, BE32 axis 0/1 and retail-board-version 5.
Platform device and `of_node` links intentionally follow sysfs links; `driver` is
readlink-only. Every leaf uses O_NOFOLLOW|O_CLOEXEC; no environment/path override.
No private inputs. No capabilities are raised; EUID must already be zero.
Parent must review actual privilege/runtime identity; EUID/identity checks are
not cryptographic kernel/image authentication or boot-generation admission.

`state` parsing accepts only the exact current ordered fields and terminal LF:

```
axis=N issued_steps=U32 phase=0..3 result=I32 coil_readback_ok=0|1 coils_off_verified=0|1 max_pulse_steps=4 period_ms=20 physical_position_known=0
```

Every state leaf is bounded to 256 bytes/64 reads with checked numeric ranges.
No missing/extra/reordered fields, trailing bytes, embedded NUL or count wrapping.
All-zero cold state is `UNTESTED` (allowed first move), not a failure. Subsequent
result nonzero or either flag false is `HOLD`, which blocks **both** axes.
`READY` requires result=0 and both flags=1. Neither READY nor the flags establish
physical position, motion, electrical pulse width or mechanically safe limits.
Moves reject issued_steps > UINT32_MAX-4 on the selected axis. Four complete steps
return the same phase (mod4), increase issued_steps exactly4, set both flags1 and
result0; every field of the other axis must be unchanged. Both before/after axis
snapshots are returned. Reads are not atomic across axes; parent exclusivity is
mandatory, and snapshots cannot detect every possible external concurrent pulse.

## JSON pin

One LF-terminated JSON object, strictly <4096 bytes, exact keys:

* `schema`: `qc07f.ptz.sysfs.v1`
* `action`: fixed action; rejected argv uses `invalid`
* `nonce`: validated lowercase32hex; rejected argv uses empty string
* `result`: `OK`, `FAILED` or `UNKNOWN`
* `stage`: fixed string (`arguments`, `euid`, `identity`, `before`, `status`,
  `admission`, `pulse-open`, `pulse-write`, `verify`, `complete`, `close`)
* `requested_steps`: 0 for status/invalid; otherwise -4 or4
* `write_attempted`, `write_completed`, `hold`: booleans
* `errno`: primary positive errno or0; short/zero writes report EIO
* `diagnostic_errno`: first post-snapshot errno or0
* `before`, `after`: arrays [axis0, axis1]; absent/unparsed state is null

State object exact keys are the nine sysfs fields plus `health`:
`axis`, `issued_steps`, `phase`, `result`, `coil_readback_ok`,
`coils_off_verified`, `max_pulse_steps`, `period_ms`,
`physical_position_known`, `health` (`UNTESTED`/`READY`/`HOLD`). All numeric fields
are JSON integers, not booleans. Root result `OK` for status means readable facts,
even when `hold=true`; it is **not** authority to clear a previous global HOLD.
Status has before snapshots and after=[null,null]. Exit0=OK, 2=FAILED (no pulse
write attempted), 3=UNKNOWN (write attempted, completion or verification uncertain),
4=receipt output failed/incomplete. stdout-only partial/EINTR handling is bounded
to64 calls; it can never replay a sysfs write. `write_completed=true` means only
that write returned its full byte count, never mechanical movement or acceptance
of a subsequent failed check. Unknown can coexist with completed=true.

## Mandatory parent admission / failure semantics

The CLI is stateless across invocations. **The parent GUI must latch global HOLD**
for UNKNOWN, missing/partial/late/unmatched output, signal/death, exit4, failed
post-checks or unknown job ownership, and refuse all subsequent controls. A later
successful read/status MUST NOT clear that latch. Even an UNKNOWN followed by
healthy kernel state is not permission to try again. CLI rejects kernel-visible
bad state; it cannot persist an unknown outcome after its own process is gone.
GUI must retain instance/operation locks, one owned command, no autorepeat,
no click queues, fresh nonce and exact receipt/action matching. The kernel mutex
serializes both axes but does not authenticate callers or protect CLI snapshots
against other writers. No fake boot/key admission is reported here.

On any attempted write, exactly one read-only snapshot per axis is tried (if
ordinary syscall return permits), with no repair, OFF, remux or further pulse.
The existing kernel **frees descriptors even when all-off readback fails**;
driver quarantine/exclusive ownership after such a fault is NOT guaranteed.
Global HOLD and actual physical OFF recovery are required, not merely closing
the GUI. Do not claim this transport fixes that driver limitation.

IRQ-enabled kernel msleep currently bounds the requested primitive to four steps,
20ms dwell plus 1..1.5ms break each, then off/readback/free. Scheduling delays,
driver stalls and hardware faults mean this is not a guaranteed physical duration
or hang-proof cutoff. Killing userspace does not cancel an ongoing sysfs callback.
No PID guardian/watchdog wrapper is added: live RTSP opens the watchdog per frame
and competing wrapper EBUSY would break controls. Native admission is conditional
on parent's reviewed finite primitive and manual OFF recovery, never a new guard.

## Offline qualification / remaining native gates

`python3 bringup/qc07f/ptz-console-candidate/build_cli.py runNN` refuses reused
outputs, hashes all source/reference inputs and pinned toolchain, compiles actual
C body with fully mocked syscalls optimized and ASan/UBSan, validates receipts
and syscall traces, then builds static ARMv5TE CLI without executing it. All build
commands run bwrap read-only root/no network with only fresh owned run output
writable. Report records argv/tool versions/timestamps/hashes; two final fresh
builds must match binary bytes.

Parent remaining gates: inspect exact source/build/ELF/schema; prove actual
paths/property/driver/permissions, bounded read-only status (including cold
UNTESTED), privilege/image/runtime identity, no conflicting motor owner and
exclusive GUI bridge locks, current recovery/manual-OFF availability and real
feed lifecycle. Then, only user-driven **one first4-step** trial with no automatic
movement, confirm both axis snapshots/coil retirement and observe actual scene.
Physical mapping, signs, limits, current/temperature, timing, repeat behavior and
media interference remain unproven. Increasing the cap requires a new properly
reviewed primitive; never emulate32 by eight4-step writes. No native claim is
made by this source unit; no hardware, UART/network or private inputs accessed.
