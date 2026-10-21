# Fixed C optical transport — OFFLINE candidate

This unit supplies a tiny static ARMv5 CLI because kernel75's initramfs has no
Python. It does **not** supply target admission, a UART bridge, a network service,
or native qualification. Existing module, UAPI, policy, Python transport, GUI,
tests and sealed kernel sources are unchanged. No target access/staging/boot/pulse.

## Parent bridge contract

After independent review, the intended RAM path is `/tmp/qc07f-optical-cli`.
The only accepted invocations are:

```text
/tmp/qc07f-optical-cli status 0123456789abcdef0123456789abcdef
/tmp/qc07f-optical-cli pulse-a 0123456789abcdef0123456789abcdef
/tmp/qc07f-optical-cli pulse-b 0123456789abcdef0123456789abcdef
/tmp/qc07f-optical-cli ir-on 0123456789abcdef0123456789abcdef
/tmp/qc07f-optical-cli --ir-on-hold 0123456789abcdef0123456789abcdef
/tmp/qc07f-optical-cli ir-off 0123456789abcdef0123456789abcdef
/tmp/qc07f-optical-cli --setup 0123456789abcdef0123456789abcdef
```

Examples are syntax, **not permission to run**. Generate a fresh lowercase32hex
nonce per request; it is echoed for correlation, not stored/deduplicated and not
an authorization or anti-replay token. Setup is an explicit **parent-only** action,
never GUI setup-on-connect. There is no arbitrary pin, width, path, command, shell,
network, CLOSE, rearm, retry or long-lived daemon interface.

Parent alone must pin helper/module/UAPI identities, current kernel75 boot/epoch,
`/dev/qc07f-optical` sysfs/device identity, sole private UART owner, bounded recovery,
media readiness/receipt renewal, source readiness and a shared serialized operation
lease/HOLD. The CLI checks effective UID0, opens the one fixed device with
`O_RDWR|O_NOFOLLOW|O_CLOEXEC`, and requires `fstat` character-device mode. That is
**not** proof that this is the correct module/provider/boot. Low FDs are refused
so missing standard streams cannot redirect receipt writes to the optical device.
Kernel capability/namespace checks remain authoritative.

Parent must not enable controls merely because this builds or a snapshot exists.
Frame decoding does not admit effects. No automatic button repeat or fallback
transport. Missing/partial/failed receipts mean HOLD and reconcile, never replay.
Parent must not signal a process group or kill unrelated target owners.

**GUI/native bridge IR_ON must use `--ir-on-hold`, not historical `ir-on`.**
Exact initramfs `wifi-monitor.h:23..36` forks BusyBox ash and `wait4`s that
specific shell PID, not generic orphan children. Old fork mode has no qualified
reaper here. Do not modify PID1/kernel or add a daemon/subreaper. The new holder
must be a direct job of already-owned persistent interactive ash, which must stay
alive to reap/wait it. The existing4900ms kernel timer remains primary.

## Receipt and error semantics

One bounded JSON line (under2048 bytes), stdout only. Normal return is0 for a
complete OK receipt,1 otherwise. Self mode emits before holding, then `_exit(0)`;
post-receipt cleanup errors cannot revise that receipt or exit status. Output
failure can leave partial/no JSON and returns1 without holding. Fields:

- `action`: allowlisted name (`setup` for `--setup`), otherwise null.
- `nonce`: validated32hex request value, otherwise null. Invalid input is not echoed.
- `result`: `OK` or `FAILED`; neither asserts physical movement or optical position.
- `ioctl_completed`: true only for known zero command-ioctl return (status uses its
  single status ioctl). This is separate from snapshot/keeper/receipt success.
  An errno or unexpected positive return is **not** permission to repeat an effect.
- `command_errno`, `snapshot_errno`, `transport_errno`, `keeper_errno`,
  `verification_errno`, `close_errno`: numeric errors, zero if absent. `EPROTO`
  also covers unexpected positive ioctl returns/unknown ABI or failed ON-state
  verification; no arbitrary error strings, raw logs or private identities.
- `keeper_pid`: positive direct fork child PID only for successful IR_ON command
  with successful fork; **zero for self mode**, never a fabricated PID lease.
  Informational only, not a future PID lease.
- `lease_mode`: exactly `none` for non-IR actions/unrecognized input, `fork` for
  recognized historical `ir-on`, `self` for recognized `--ir-on-hold`. Both IR
  invocations report `action:"ir-on"`, including failures. Mode selects the path,
  not proof of a surviving holder. Early argc rejection has action=null/mode=none.
  GUI native envelopes must reject `fork`: require self/keeper_pid0 for IR_ON,
  none/keeper_pid0 for other actions, alongside existing success/admission checks.
- `state`: exact numeric `struct qo_status`, or null when no successful snapshot.
  An unknown ABI's actual copied fields are preserved but rejected, never admitted.

State is the actual **80-byte**,16u32+2u64 layout, no invented boot/kernel admission:

```text
abi setup fault owned_mask pair_latch pair_pad ir_latch ir_pad
direction_mask mux_ok release_verified ir_active ir_remaining_ms
cooldown_remaining_ms pulse_us ir_limit_ms sequence monotonic_ns
```

Non-status actions get one command ioctl and a separate diagnostic STATUS, even
after command errno/unexpected result. Status gets just one STATUS. No command or
snapshot is replayed. Historical `ir-on` forks **before** STATUS. That snapshot
must show ABI1, setup1, fault0, owned7, IRactive1/pad1/latch1, positive remaining
time<=4900ms, fixed limit4900ms, neutral pair/pads, direction7/mux1 and verified
pair release. A failed snapshot/verification emits FAILED; the holder may still
exist until the original deadline. Completion/IR state can therefore be uncertain;
the parent must not infer OFF from FAILED. Fork failure is different: immediately
close the sole optical FD, emit FAILED with keeper error, state=null and
snapshot_errno=ECANCELED, never a misleading pre-close ON receipt or hidden retry.

`--ir-on-hold` issues IR_ON once and STATUS once **without any fork**. The same
positive ON-state checks above must pass before emitting one OK receipt with the
optical FD still open. Only complete successful emission enters `keeper()`, which
closes all other FDs (including stdout) before sleeping until the original
pre-command CLOCK_MONOTONIC+5s deadline, closes the optical FD and `_exit`s.
Command/status/verification/fstat failures close immediately before FAILED output;
partial/failed OK output closes immediately with no second receipt/no hold.
Close is never retried; no error path repeats ON, rearms or raises IR again.
OK describes a snapshot and completed write, not future isolation/sleep/close
success. Late close errors are unreportable: close_errno0 is not a future-close
claim. Kernel timer remains primary even if output blocks or keeper exits early.

Separate ioctl/snapshot/fork/output are not atomic. Another privileged writer can
change state between them; the parent must exclude foreign root writers. An ON
snapshot may already have expired or been switched OFF when the desktop reads it.
`ioctl_completed` is not evidence of measured width, coil movement, emitter output,
hardware release timing or a surviving child. A missing output can follow an effect.

## Exactly one bounded FD holder

The IR owner is the kernel's open file description. Historical fork shares it;
closing the CLI parent's reference does not release it while the child holds it.
Non-IR actions and `--ir-on-hold` do not fork. Only historical mode returns from
the CLI before its keeper exits; self mode remains the direct ash job itself.

Before IR_ON command entry the CLI records CLOCK_MONOTONIC+5s. The keeper keeps
only the original optical FD: two kernel75 `close_range` calls close all other
FDs, including stdin/stdout/stderr, inherited FD3 watchdog/UART handles, and FDs
above the current open-file soft limit. No FD enumeration guesses or state files.
If close_range fails the child exits immediately, closing everything through
`_exit`; no unsafe fallback. It makes no ioctl, stream access, signal, output,
fork, command replay or timer rearm. `clock_nanosleep` uses the **same absolute
monotonic deadline** on EINTR. At most64 attempts; signal storms/other sleep errors
close early. It closes the optical FD once and `_exit`s, never flushing stdio.
Earlier child death/close is acceptable and can release IR early.
Historical mode orphans its child to PID1, whose monitor does not provide a generic
reaper. **Do not use historical mode from this GUI/native path.** Self mode avoids
that orphan only as a direct persistent-ash job. No reaper/daemon is supplied.

The existing **kernel4900ms hrtimer release is primary**, independent of child or
desktop. The keeper5s close is only a backup, not a physical<=5s guarantee: scheduling,
IRQ starvation/kernel failure and transport delays remain unqualified. The CLI
never extends the kernel timer. Closing the parent's reference before its JSON
output cannot revoke the child's shared owner. Other status/OFF/pulse CLI exits do
not close another owner's open file description. A child exiting before receipt
delivery, including close_range failure, is possible; postfork status is a snapshot,
not a child-readiness handshake. No parent/child guardian protocol is added here.

JSON construction and write attempts are bounded (16 calls maximum, partial writes
and EINTR handled). A blocking UART write still depends on parent finite transport
supervision; no CLI-side scheduler/timeout framework or process-group signaling.
No actual target ioctl, pulse, file, fork keeper or ARM execution is used by tests.

## Parent-owned persistent-ash ready-file recipe (review only)

Feasible handoff, **not an implemented dispatcher or permission to run**: parent
must source/review its fixed RAM dispatcher in the **already-owned persistent
interactive ash**. Do not launch via an exiting `sh -c`, subshell, pipeline,
command substitution, nested helper process, `nohup` or double-fork: those can
orphan the holder and recreate this problem. Keep ash alive through completion
and native reaping qualification. No networking is added.

After parent validation/reservation of fresh lowercase32hex nonce and new private
RAM paths (private umask and no-clobber policy in that same ash), the essential
shape is a **direct simple background command**:

```text
/tmp/qc07f-optical-cli --ir-on-hold 0123456789abcdef0123456789abcdef </dev/null >/tmp/optical-0123456789abcdef0123456789abcdef.json 2>/tmp/optical-0123456789abcdef0123456789abcdef.err &
optical_job=$!
```

Never reuse these example paths. Parent owns safe path creation, substitution,
launch framing and shell variable ownership; this unit does not implement them.
The nonce file is a receipt/ready-file, not a daemon socket or persistent state.
Parent reads it through its separately reviewed finite UART protocol: accept one
newline-terminated JSON object under2048 bytes, exact schema/action/nonce,
lease_mode=self/keeper_pid0, all success checks and positive ON status. File
existence, partial lines, job PID or shell prompt is not readiness. No rename
handshake: the complete validated line is the receipt. Enforce the parent's short
fixed receipt deadline; expiry/unknown state means HOLD, never replay. File
completion cannot extend entry+5s and may be observed after expiry; freshness
and media renewal must still pass.

After original5s plus the parent's bounded observation allowance, the same ash
must account for/reap its direct job (`wait "$optical_job"` only under reviewed
completion/transport supervision), collect status and retire nonce files. Job
messages, `$!`, `jobs`/status and builtin `wait` behavior on exact BusyBox ash need
**native qualification**, including no zombie or surviving inherited FDs. A
blocking wait/hung kernel is not made safe by this recipe: retain independent
recovery, never claim a hard userspace bound. No GUI admission from this recipe.

The libc ioctl request cast preserves the exact low32 UAPI command bits: glibc's
prototype accepts unsigned long, while the pinned musl prototype accepts int.
The actual status command is compile-time checked as0x80505160 with size80.

## Reproduce offline

```text
python3 -B bringup/qc07f/optical-control-candidate/build_cli.py runNN
```

Choose an unused numeric run; no overwrites. Outputs stay exclusively under
`evidence/delegated/20261016-optical-cli/runNN/`. Commands execute with root inputs
read-only, output-only writable, private `/dev` and no network via bubblewrap.
No modules/header preparation/full-kernel build occurs. The Bootlin ARMv5 musl
202508 toolchain under `work/lightbulb-camera/implementation-20261008T205404Z/`
is read-only and pinned (wrapper, real GCC14.3.0, libc.a and libgcc.a).

Each run copies own sources and unchanged endpoint inputs with their SPDX notices,
records SHA256/tool versions/argv/timeouts/timestamps/log hashes, and verifies all
source inputs unchanged. Host optimized and ASan+UBSan builds include the **actual
CLI** with every OS/device/process/clock/stream call mocked. JSON parsing validates
all18 actual UAPI fields,64-bit exactness and failure receipts. Holder tests model
close_range across standard/foreign/high FDs, fixed deadline/EINTR/early exits,
no ioctl/output/rearm, and historical parent-return/shared-FD ownership. Actual
self CLI tests check no fork, positive receipt before isolation/sleep/last close,
entry deadline unchanged by snapshot/output delays, standard/high FD retirement,
failed status/verification/fstat/close/output paths, partial writes, no hold on
failure and no replay. The test runner
refuses anything except x86-64 host ELF; ARM is built/static-inspected only.

The ARM artifact is **not executed on the host**, staged or native-qualified.
Own source is compiled separately with ARM-mode/v5TE/soft-float flags and its
object attributes inspected. The pinned runtime's final merged attributes also
advertise v5TEJ/Thumb-2; that metadata is retained, not relabelled or silently
treated as proof of native ISA/ABI compatibility.
Exact host-case/JSON-receipt counts are in each run's test logs; both optimized
and ASan+UBSan must pass. Failures and intermediate source snapshots are
retained under earlier run directories, not overwritten.
`BUILD_AND_CLI_MODEL_PASS_NOT_NATIVE` proves the offline unit only. The parent's
native feed, renewal, under-load pair release, device/module lifetime and optical
acceptance gates remain open.
