# Result: useful detection/prevention pieces, no strict-continuity admission proof

**Retain the C2c block.** No explored interface alone or composed here proves
unchanged no-suspend/no-intervening-wall-step coverage from original anchor through
effects/postflight. This is a bounded SOURCE-design feasibility result, not an
API/policy amendment, positive validator or lifecycle GO. Exact reference source
anchors and host-applicability limits are in SOURCE_NOTES.md.

## Feasibility matrix

| Mechanism | Useful source-supported role | Why it cannot currently discharge strict continuity |
|---|---|---|
| REALTIME timerfd, absolute+CANCEL_ON_SET, retained same open description | Nonpreventive cancellation watch. An observed cancellation can be latched permanently as failure; one sentinel survives ordinary compensating offset updates after it is set, until consumed/reset. | Poll readiness is not cancellation identity, and lack of readiness is not a clock-write completion barrier. Reference notification samples an offset after publication; concurrent compensating writes may restore it before sampling. Read/setup/ioctl can consume/reset evidence. It prevents neither a step nor an effect racing notification. |
| REALTIME_ALARM variant | Same cancellation class, with wake capability and extra CAP_WAKE_ALARM requirement. | Waking after sleep is not prevention of sleep; not a stronger all-step audit trail. No reason to infer continuous admission from using an ALARM clock. |
| suspend_stats success/fail/step counters | Cheap cumulative return-path diagnostics under their exact implementation/configuration. Positive change can invalidate an epoch; conservative refusal on a failed attempt is possible. | Resumed tasks can run before increment. Read equality is not absence, an in-progress barrier or universal sleep coverage. Finite counters/rotating history require wrap/reset/boot/source bounds; unaudited sleep paths and PM tests differ. |
| power:suspend_resume via perf/ring transport | Source events include pre-freeze markers; transport can provide event identity and loss counters. A delivered relevant event or loss can latch HOLD. | No global event sequence in the trace payload; activation/per-CPU scope/config/filtering/disabled intervals/CPU hotplug must be proved. Lost records can be delayed until a future successful write. No records isn't proof of no producer loss or missing subscription. Postevent delivery is not preventive interlocking. |
| PrepareForSleep / ordinary service event notifications | Cooperative manager notification of requested sleep and resume. Useful to fail/stop future work. | No gap-free replay/sequence/loss guarantee or all-kernel-path coverage proved. Reconnect/restart/missed signals cannot be repaired by a new baseline. Notification does not prevent direct sleep or guarantee consumer scheduling before an effect. |
| Held strong logind sleep inhibitor | Conditional prevention of cooperating manager-routed sleep while actually active, under proved service/policy/lifetime assumptions. More useful for pre-effect exclusion than a postresume counter. | Does not exclude authorized bypass, direct kernel/unit/firmware paths, manager failure/replacement, power events or untrusted privileged mutation. Delay/weak is weaker. No wall-step prevention. No actual host inhibitor/policy/service facts were acquired. |
| B-M brackets, endpoint agreement, max elapsed, intersecting deadlines | Conservative lifetime accounting and detection of some changes. | Still no event/continuity history, prevention, all-step/no-suspend proof or future guarantee. C2c's distinction remains unchanged. |

## Timerfd findings that must not be overstated

The manual's current-or-future ECANCELED contract makes a retained watch useful,
but it is not an application-owned permanent event log. In the reference source,
read consumes the sentinel and settime can rearm while returning ECANCELED.
Consequently any observed cancellation/error must latch failure before subsequent
processing; no retry, "drain then clean", rearm, dup-to-new-object or recreation
may silently restore the epoch. Normal expiration is not clock continuity and
must not trigger a new full lease. Setup deadline must be bounded consistently
with the immutable epoch; no expiry extension or alarm/device action is selected.

The reference synchronous wall setter notifies before returning to that setter,
but other tasks can observe the changed clock and act before timerfd processing.
Offsets are sampled after releasing the timekeeping lock; this is not a source
proof that every compensating mutation is captured. Deferred work also exists,
including resume notification. Eventual readability after an event is neither
"no event yet" nor prevention while a parent effect is executing.

REALTIME is global in the documented time-namespace model, while M/B are offset
domains. That does not bind the watch to the recorder/controller M domain, boot,
PID-start basis or actual executing source. FD integer equality doesn't identify
the same open description; aliases, inheritance, exec/last-close and destructive
reads must be excluded/authenticated by future ownership design. No FD is opened
or inspected here. Ordinary rate slew/leap/clocksource paths would also need an
applicable-source scope decision; timerfd cancellation is not a detector of all
possible forms of clock instability. No exact equal-clock rule is weakened.

## Loss awareness is necessary but not sufficient

A prospective event contract would have to cover every relevant producer path,
all CPUs/contexts, subscriptions active BEFORE anchor, no silent filtering/pause/
disable/multiplex gap, stable event identities and correctly ordered head/tail
consumption. Generic ring byte offsets and timestamps are not gap-detecting event
sequence numbers. A per-task subscription alone does not cover sleep initiated
elsewhere. No tracepoint/perf configuration is selected or queried.

An empty ring with no LOST record is insufficient: reference LOST emission can
await the next successful reservation. A final cumulative loss observation can
help reveal trailing overflow only under an applicable API/schema, but it is
not an atomic frontier across all producers or proof that an event stayed enabled.
Each unproved field, overrun, lost sample, bad framing, unavailable/disabled source,
CPU-scope change or inconsistent identity must be UNKNOWN/HOLD. No subscription
recreation, counter reset, empty-ring baseline or later clean snapshot heals it.

Even a truly loss-free event history would normally establish a **retrospective**
property at its covered frontier. It cannot ensure that a prohibited event will
not occur immediately after a precheck or during an operation. Detection then
means failed attempt/HOLD; it cannot retrospectively establish preventive
admission for that effect. A source-backed atomic interlock or a proven prevention
contract is additionally required if strict uninterrupted operation is mandatory.
This packet supplies neither, and does not invent a generic supervisor to do so.

## Can a trusted cooperative-host contract ever support the unchanged property?

**Conditionally in principle, not established here.** Restricting possible event
sources by proved environmental assumptions need not logically change the desired
no-event property. But those assumptions themselves must be explicitly accepted
and evidenced; calling the host "trusted" is not evidence. At minimum:

- All admitted sleep entry paths must be mediated by the proved held strong
  inhibitor, with no already-committed operation, bypass authorizations, direct
  kernel sleep/alternate manager path or untracked service replacement.
- All wall-step sources must be separately excluded or interlocked throughout
  the interval: administrative/time-service setters, applicable kernel resume/
  RTC/leap/virtualization paths and any other applicable discontinuity source.
  A sleep inhibitor does not accomplish this. Disabling a setter in this task
  is forbidden and no such host enforcement is claimed.
- Trusted kernel/manager/hardware and privilege boundaries must be specified.
  Unconstrained root/kernel/hypervisor/firmware or power loss is not controlled
  by an application FD. No finite userspace watcher proves immunity to arbitrary
  privileged source/descriptor/clock mutation. Existing serialized namespace/FD
  assumptions do not automatically waive this host-continuity proof.
- Acquisition, lifetime, absence of in-flight transitions and release must be
  covered by source semantics plus authentic current evidence; no Boolean,
  receipt digest or typed record substitutes. Failure/loss remains fail closed.

This is a proposed assumption list, **not accepted host policy**, actual facts,
proof that an inhibitor is installed, or authorization to change privileges,
services, clock sync, DBus, tracing or system sleep settings. A policy that accepts
postevent detection as if it were prevention, or accepts unproved privileged
exclusions, would be a substantive change and is not selected here.

## Smallest remaining decision / packet

Keep the block. Parent must separately decide whether to pursue proof of an
explicit limited cooperative-host contract under the unchanged property, retain
the operational block indefinitely, or separately review a different policy.
No operator/settings/power request is made now. There is no adequate basis here
to select an implementation, clock sampler, watcher or controller.

If separately selected, the smallest useful SOURCE packet is a **host-contract
applicability/evidence specification**, not another generic clock framework:
identify admissible running-source/configuration/namespace/privilege associations,
enumerate all sleep and wall-step authorities and their enforced exclusions,
and establish pre-anchor activation/lifetime/no-in-flight barriers with failure
coverage. If those cannot be proved, it must stop at the same block. Actual host
facts would require a separately authorized parent-only read-only boundary, not
queries from this author. LIFECYCLE.md lists the needed evidence, not a live recipe.

The PID-start tick-lower-bound/time-namespace mapping and recorder's own immutable
M-deadline observation interface remain **separate explicit source blocks** from
C2c CLOCK_MODEL sections4–5. This packet does not infer host rounding from K or
manufacture a deadline from UTC. Original min(arm expiry, source lifetime), earlier
caps,30s+120s+phase cost, source freeze and all custody/HOLD protections stand.
No automatic next packet, test, native/physical action, query, boot or reset queue.
