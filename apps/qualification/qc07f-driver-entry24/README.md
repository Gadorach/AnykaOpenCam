# QC07F driver entry24 — uninvoked closed source candidate

Read the [contract](../../../docs/qualification-plans/driver-supervised-entry-20261024/CONTRACT.md)
and [handback](../../../docs/qualification-plans/driver-supervised-entry-20261024/HANDBACK.md).
Independent ordinary review, actual reviewed paired image selection and parent
disposition are mandatory BEFORE invocation. Creation/AST parsing is not admission.

`driver_entry.py` owns one foreground cold start → fixed BOOT/neutral checks →
one-shot QCL → independent RAM CRC → fixed volatile Linux boot → fixed RO monitor
inventory → OFF → actual owned recorder stop/wait/final accounting. No importable
command API, alternate action, dry-run, image default or runtime test is selected.
Source-private transports are only called by that closed sequence; U-Boot text
must match the next precomputed command, including each exact loader word.

The eventual parent-only CLI has mandatory `--selection` AND `--selection-sha256`,
`--private-parent`, `--power-config`, `--port`, `--part`, `--python-sha256`,
`--serial-init`, `--serial-sha256` and both current-custody/supervision confirmations
listed in `main`. Use isolated Python/no bytecode, a fresh foundation-20261024[a-z]
part, the exact source-pinned adapter association and outside-Git protected paths.
There is deliberately NO filled invocation or image selection here. Confirmation
flags attest caller facts, not independent recovery proof or a permission token.

The selection schema is in CONTRACT.md and implemented by `select_image`:
explicit public file records, actual whole-image SHA/size/BZIP2/header/data CRCs,
fixed release/DTB/PTZ source and separately pinned association/review/context.
The other author’s future image is not polled and no pin is invented here.

One5400s first cap includes staging and transfer. Actual child BOOTTIME/argv can
only shorten it. Clock interval discontinuity latches HOLD; no C1 prevention claim.
RPC receives and fuser waits clamp to original global/local bounds. Ordinary file
IO, Python scheduling and inherited HTTP socket timeouts are NOT a preemptive
whole-interval guarantee; checks detect overruns after return. Smart switch is
normal cutoff while host alive; actual present human is host-loss fallback.

Unknown OFF/partial child retirement never triggers kill, unlink, another request
or a replacement owner. The foreground controller retains its actual Popen/locks
and passively awaits the ORIGINAL recorder exit. This may block if it stalls;
it does not renew capture/timing authority or infer OFF from natural exit. Do not
wrap the controller in a timeout that kills ownership/cleanup. Manual OFF is still
required on uncertain power. Available eventual final accounting is checked as
failed evidence, never promoted to controlled-stop PASS.

Strict unsupported console envelopes, asynchronous RO printk, stale CAS, unexpected
RX/reset, NAK/duplicate/incomplete records or clock/source drift stop the scope;
no echo repair, retransmit, prompt refresh or new epoch. Boot printk is accepted
only as timestamp-framed observational data, not healthy-driver evidence. The
monitor has no complete process/FD inventory; boot-only facts admit no next phase.
PTZ probe reports verification booleans0 truthfully, NOT a measured coils-OFF ACK.

Baseline boot has sensor reset/MCLK, GPIO42 power/USB enumeration and ISP allocation.
No radio load/query/RUN/station/MTU/network, motion/homing, mic/audio/speaker, sensor
stream/media, flash/card or backend work. Speaker tests remain DEFERRED. No calls,
tests, process launches or hardware activity were performed by this author.
