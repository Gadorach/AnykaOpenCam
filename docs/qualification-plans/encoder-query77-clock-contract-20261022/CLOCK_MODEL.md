# Raw clock roles, conservative bounds and the remaining logical block

**Design deductions, not measurements or executable pseudocode.** All equations
are symbolic; no sample values, calibration, synthetic histories or tests were
constructed/run. This does not amend the frozen C1v2 API in place.

## 1. Why an adapter cannot truthfully satisfy frozen C1v2

Let M be the selected host MONOTONIC clock, B BOOTTIME and W REALTIME/UTC.
C1 requires, against original and previous:

`B_i - B_anchor = M_i - M_anchor = W_i - W_anchor >= 0`.

Equivalently it demands constant observed B-M and W-M offsets. Each scalar is
read at a different instant in an ordinary sequential collector. Different call
spacing, preemption, quantization and conversion can change those observed
offsets even with ideal continuous clocks. The API provides neither simultaneous
three-clock reads nor a source-proved zero-error guarantee. Accidental equality
can occur; it is not a general sampling method or proof of physical continuity.

Deriving `B_i=B_0+(M_i-M_0)` and `W_i=W_0+(M_i-M_0)` would make arithmetic pass
by definition, while deleting the actual evidence for suspend/clock jumps. It is
forbidden, as are rounding all samples into agreement, retrying until agreement,
choosing an arbitrary tolerance or labeling a derived estimate a raw reading.
The existing function must continue refusing nonconforming records. No admitted
sampler can be delivered by merely wrapping it or using BOOKKEEPING_ONLY.

## 2. Minimum proposed record amendment — not a selected implementation

A **fresh, separately selected and reviewed** source revision would need these
distinctions, rather than changing three field names or passing normalized values:

| Record role | Necessary fields and invariants |
|---|---|
| Immutable original host association | host boot association AND time-namespace/clock-domain association, selected clock IDs/implementation semantics, recorder PID/start ticks/peer/source/capture/adapter/raw/catch association; separately retain target epoch/runtime/boot identity. Never reuse target boot_id as host boot identity. |
| Raw single-clock sample | one explicit clock ID, actual returned integer value, actual M-before and M-after readings, ordered collection slot/sequence, success/refusal status. For B or W, M-before occurs before that raw read and M-after after it. All three fields are real calls, not projections. |
| Sampling contract | source-derived error/quantization/float-conversion enclosures where needed; explicit finite max collection/bracket age/width and inclusion in phase costs. **No numeric window selected here.** Resolution alone is not an error/latency bound. Missing reviewed bounds prevent use, not imply zero. |
| Native coverage limits | recorder's original immutable M timer/arm deadline or safe lower enclosure; original process duration/ticks/tick-rate basis and immutable B expiry bound; original arm wall limit with its source precision/provenance. No fresh arm/receipt may replace them. |
| Derived accounting | explicitly marked offset intervals, conservative deadline candidates and earlier-only global/phase deadlines. No derived value may occupy a raw sample field or become a fake equal-clock triple. |
| Continuity evidence | separate, source-backed gap-free prevention/observation evidence spanning original anchor through effect/postflight; unavailable/ambiguous is UNKNOWN, never a `no_suspend=True` flag from interval consistency. No such evidence source is established by this packet. |

Keep exact built-in int-not-bool/range/schema/sequence checks; fixed clock IDs,
fixed named sample slots/count, no arbitrary clock handles, getters or callbacks
inside the pure validator. Keep original and previous associations immutable and
validate each previous record against original before use. Same epoch/counters/
catch rules, fixed source pins, no rearm/rotation/restart/rebase. Raw records and
their authentic collection lineage belong in future private bounded evidence,
not this public report. Type/frozen/hash checks cannot authenticate them.

The smallest C1-facing change is a versioned interval-observation/coverage record
and derived-budget validator with a distinct result for continuity-unproved
(which is refusal, not a usable Budget). It also changes Epoch initial clock/
coverage representation, constructor initial validation, previous/original
checks, observe/complete callsites, `_observation_fields`, marker/ledger encoding
and exact readback schema. Retain all existing record/ledger/count/memory caps;
if required raw evidence cannot fit, stop rather than raise a ceiling or drop it.
This is not an authorized patch, native collector or complete source acceptance.

## 3. What brackets can actually prove

For a raw C sample x with actual monotonic bracket [l,u], l<=u, define the
instantaneous offset at that sample as C-M. Under independently proved clock
semantics and a combined error enclosure e, it lies in:

`I_C = [x-u-e, x-l+e]`.

Here e must cover relevant read/representation errors; it is not an empirical
tolerance chosen to make data overlap. Even without suspend, finite bracket
width is genuine uncertainty about M at the C read. Do not replace it by a
midpoint. Ordered source calls, same host clock domain and safe enclosures are
prerequisites; a user-supplied tuple alone does not establish them.

For original interval [L_0,U_0] and current [L_i,U_i], offset change is enclosed
by `[L_i-U_0, U_i-L_0]`; check against previous as well, never just the latest
pair. Maintaining an intersection can expose inconsistency with a constant
offset, **not** prove that the offset was constant throughout the gaps.

- For documented ideal Linux B/M semantics, B-M accumulates suspend. A proved
  positive lower change establishes a suspend/semantics violation => HOLD.
  A proved negative change violates that model/association => HOLD, not rebase.
  If zero and positive change are both possible, suspend is **unknown**. Overlap
  is not no-suspend. Even point-like returned values lack a source proof of
  zero physical uncertainty and coverage of the intervals before/after reads.
- W-M endpoint changes can expose incompatibility with the selected continuous
  rate model, but need not uniquely identify a clock step (slew/model error also
  matters). Overlapping intervals do not exclude a step inside the uncertainty;
  even perfect endpoint equality cannot exclude compensating steps between reads.
- Finite reads cannot certify the interval from last precheck through an effect
  and its postcheck. Post hoc detection is useful failure evidence, not proof
  that the effect was admitted under uninterrupted coverage when it happened.

Thus interval consistency must not replace the existing no-suspend/no-discontinuity
policy with "no change detected". No precision-based numeric skew choice solves
the missing event/continuity evidence. MONOTONIC_RAW, max(elapsed clocks) or shorter
poll spacing does not close that logical gap. No polling is selected here.

## 4. Process lifetime: conservative and suspend-aware, not guaranteed liveness

Retain B-based upper expiry from the source-associated process start, not a renewed
duration when status is read. Let N=10^9, T=actual PID start ticks, H=actual positive
SC_CLK_TCK, d=canonical recorder duration. Future collector must authenticate those
values and their host boot/namespace/start identity; none was obtained here.

If exact applicable host source proves `T=floor(S_B*H/N)`, then
`S_low=floor(T*N/H)` is a safe lower B start bound. More generally require a
source-proved bound q_start on conversion uncertainty and round outward:
`S_low=max(0, floor(T*N/H)-q_start)`. A one-tick subtraction can be justified only
if source establishes an error of at most that tick; it is not a chosen tolerance.
The local manpage specifies units, not the required rounding/namespace mapping.
Without that mapping/q_start, a tight process expiry is **unproved**. Do not use
target kernel6.18.55's conversion as evidence for the unidentified host kernel.

`E_B = S_low + d*N` is fixed. Recorder timer starts after process start in the
pinned source; B includes suspended time while that timer's M excludes it. This
can only make the process-based authorization end earlier under those semantics.
At a B sample with conservative upper reading B_hi, `R_B=E_B-B_hi`; nonpositive
means expired. Integer floor/upward rounding must reduce allowance, never round
remaining upward. Source float handling is not automatically exact nanoseconds.

This is a conservative authorization cap, NOT a guarantee the process will remain
alive, recording or ready until E_B. Early exit/source change/lost peer/catch is
independent failure; time arithmetic cannot replace actual source/PID association.

## 5. Recorder and arm expiry: preserve original native timer

R102/136 uses one float monotonic start and duration. R245 has no native arm
deadline; R247 projects remaining timer duration into an `ends_by` UTC string
using **separate** W and M calls. Status does not expose timer start/deadline.
The ACK is not a simultaneous raw reading, nor a separately measured BOOTTIME
lease. Timedelta/float precision also needs outward error treatment.

A minimal future observational source-interface amendment must expose the same
timer's immutable original M deadline/lower bound, with clock ID, original source/
start/peer/capture/catch and same-host-boot/namespace association. No new timer,
arm renewal, sleep, key TX or automatic restart. If float internals are retained,
their proven rounding enclosure must make the advertised bound earlier, never
later. Native integer timing would itself require a separately reviewed source
change; converting old float text into asserted exact raw data is not equivalent.

Original coverage remains the minimum of original arm expiry and original
conservative source-process lifetime, with any stricter original profile cap.
Retain the original wall ACK as an additional earlier-only limit wherever it was
part of the original epoch; a later native-deadline report cannot delete/extend it.
If that ACK cannot be safely interpreted, refuse rather than bypass it. No actual
ACK/timer value is read here, and the existing recorder is not retrofitted.

## 6. Single M budget, but retain native-domain expiry checks

Under **proved continuity/rate and error assumptions**, a remaining lower allowance
R at a C sample bracket [l,u] maps conservatively to `D_candidate=l+R`, not `u+R`
or a still-later now+R. At the completed bracket, remaining is at most
`D_candidate-u`: sampling/processing time is spent, never reimbursed. For a wall
limit E_W, use an early-rounded expiry and upper raw wall reading to form R.
W is metadata/jump-refusal evidence and an additional conservative upper limit,
never a renewable budget authority. Changing its current C1 expiry role to pure
metadata alone would require a separate policy change; not selected here.

Important limit: mapping a B expiry to one M deadline is **not globally safe
across unknown future suspend**, since B can advance while M is stopped. A later
B sample can expose elapsed suspend/expiry, but cannot make an earlier effect
retroactively safe. Keep the immutable E_B and original native recorder/arm
deadlines and check them at every admitted boundary. For an unbounded unknown
future suspend, no positive M duration alone can guarantee completion before
E_B. The equations here do not return a usable admission budget in that case.
Similarly an unseen forward W step can invalidate a W-based cap before the next
read. No single-M remapping removes the continuity block from section3.

With continuity established separately, global accounting is:

`D_0 = earliest valid original M coverage bound`;
`D_i = min(D_0, D_previous, all new conservative candidates, all earlier caps)`.

No original or prior cap may move later. Equivalently a remaining cap at a later
M reference cannot exceed the prior remaining minus actual elapsed M; validating
previous against original remains mandatory. A larger later ACK/cost estimate/
coverage value can never grant time. Earliest immutable cap is retained even when
fresh records show more time by another clock. Missing input is not infinity.

At the final pre-effect reference M_gate (conservative latest bound, including
all admission/read/fsync/communication age), require:

`D_i - M_gate >= remaining_admitted_phase_cost + 30s + 120s`.

Also require each native-domain limit still valid and proven continuity through
the intended bounded operation. Per-phase cap is fixed from its first admitted
intent, bounded by D_i, not renewed per chunk/command/revalidation. Selected cost
must cover all still-required work and checks, including collection/postflight;
unset phase caps/unknown throughput/aging block use. Reserves remain observation
margins, never a reset mechanism. Post-effect uses the same D_0 and phase caps,
not a new epoch. A failed or late postcheck cannot turn an effect into PASS.

Taking intersections or max of actual elapsed clock bounds can only shrink a
properly derived recorded allowance, but this is **lifetime nonextension**, not
proof of no suspend/steps or actual catch/owner/liveness. Even earlier-only minima
cannot repair an originally overestimated mapping. In particular B333–341 adds
remaining to a later M read: minimum-with-old does not by itself account for the
first sample's latency/error. The proposed before-bound treatment avoids that
specific projection error only under its stated assumptions, not by changing B.

## 7. Decision boundary

No available sampled scalar API/documentation proves the strict any-suspend/
no-discontinuity property for this controller interval. **STOP at that logical
and operational dependency; no positive sample validator or implementation is
selected.** A fresh interval schema would fix truthful representation and bound
aging, but not supply missing continuity evidence or actual owner/catch facts.
No amount of deriving equal triples or picking a tolerated window is permitted.

Smallest next eligible SOURCE packet, only if separately selected: a bounded
docs-only feasibility contract for a source-backed host continuity prevention/
event-evidence mechanism spanning original epoch through final effect/postchecks,
plus host tick/namespace and recorder deadline semantics. It must specify gaps,
loss/overrun/privilege/race failure and actual source association, and either prove
the strict requirement or remain blocked. No guessed monitor Boolean. If strict
proof remains unavailable, parent must explicitly decide whether to keep the block
or separately review a different policy; this report does not relax it or choose
a numeric uncertainty window. No user physical action or automatic next queue.
