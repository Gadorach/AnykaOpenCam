# Bounded luma-tile motion + ignore-mask candidate — STORE-08

**Offline source/HOST only, parent review pending.** No real frames/photos/H.264/
footage, source driver/SD/network/writer/scheduler/retention/public endpoint used.
STORE/G5/G8/native accuracy/recording/NVR remain OPEN. Cheap native luma source,
actual tuning/night/EMI/15fps and shared-resource headroom are still unqualified.

## Actual candidate and versioned coordinates

`motion.c/.h`, `geometry.c`: fixed32×18=576 caller-produced luma statistics,
8 ignore polygons×16 vertices,4KiB compile-time static engine cap, two internal
event slots, no heap/threads/timers/full-frame copy/720p CPU lookahead. Per-cell
record is4B: raw8-bit luma mean/noise and exact16-bit native pixel coverage.
**This unit does not generate statistics by scanning a frame**. Provider must
qualify its actual cheap full-ROI source/cache/digest/lease; declaration/CRC match
is not physical data evidence. Native-qualified remains0.

Secret-free setting key **`motion.tile-luma.v1`**, schema1; reject unknown/old
versions, no silent migration of legacy masks. Client polygons use closed pixel-
edge DISPLAY coordinates of the bound measured profile, after independent X/Y
orientation and crop/scale. Source statistics stay in SENSOR-CROP tile order.
Transforms use exact integer rationals, not rounded points: sensorX=cropX+
(`flipX ? displayW-x : x`)*cropW/displayW, likewise Y. Inverse is explicit.
No postdecode colour/RGB/tint compensation or image transform is performed.
Graph binding proves supplied sensor/profile relation and explicit shared-head
association in the trusted actual graph; fixtures do not invent a retail sensor.

Polygons: bounded/simple, canonical positive signed-area winding, no zero area,
duplicate vertices, consecutive collinear vertices, self-crossing/nonadjacent
edge contact, out-of-bounds or nonzero unused-tail vertices. Overlapping valid
polygons combine by union. Rasterization uses exact scaled sensor rectangles,
point containment and edge intersection. **Conservative whole-cell ignore**:
any overlap OR boundary contact ignores the whole cell; not subpixel pixel-perfect
masking. UI must display that coarse policy. Every cell's pixel count must equal
floor-grid native coverage; total576 cells represent100% of crop, no arbitrary
vote weights/partial zero-filled source. Native dimensions≤4096×2160 ensure all
geometry/cross products fit64-bit; no GPIO route/capability is implied.

## Privacy/config/source lifetime authority

### V2 post-callback authority and acquisition/return timing correction

The parent found V1 checked policy BEFORE the last `valid` callback, so a real
revocation in that callback could still allow a tile read/commit. V1 source,
run03/HANDOFF/failures/fixtures were preserved byte-exact before editing in the
own-lane `v1-snapshot/`. New regression links the actual V1 API/source to real
policy and proves cell0 staging108 after revocation while committed bg100 remains.

V2 performs **final direct actual policy proof AFTER every external callback**,
including final `valid` and clock calls. No callback intervenes before effect.
Statistics/staged-copy/background always require exact PROFILE LIVE even when
an EVENTS check succeeds; an EVENTS-only rotated owner cannot commit private
statistics. Internal event query/take separately requires sensor EVENTS; a LIVE
token alone cannot publish notifications. Config application retains its existing
final dual ADMIN/observer proof. Grant/credential/profile changes and expiry deny
ordinary effects; exact own-loan cleanup still works after observer revocation.
`tiles_read` increments immediately after real protected cell loads, enabling
meaningful phased read-count assertions without a fabricated source getter.

| Callback outcome/timing | V2 retained/returned state |
|---|---|
| `acquire=0`, normal≤1ms | Known immutable loan, then current policy/source check before any tile |
| `acquire=0`, late/regressing time | Known loan still held, CLOCK fault/no reads; explicit checked cancel only |
| `acquire=1`, late | Known **no loan**, CLOCK fault; no fake DMA quarantine/return |
| `acquire<0` | Unknown ownership/context retained; missing ticket cannot be forced returned |
| `release=0`, normal≤1ms | Actual return ACK exactly once, owned0 |
| `release=0`, late/regressing time | Preserve actual return ACK/owned0, CLOCK fault/no new acquire; never call it uncertain DMA or repeat return |
| `release!=0` | Retain/quarantine exact loan, return-attempt recorded once; no retry/free |

Pre/post actual monotonic observations explicitly surround acquisition and return,
not merely later tile checks. Equality at1ms is admitted; >1ms faults/rejects as
specified. Source/event expiry equality still denies; geometry/gap/threshold/
4096B/32×18/8×16 limits unchanged. The callback bound is software observation,
**not preemption** of a blocking provider/physical timer guarantee. All APIs remain
trusted serialized owner entry points, not new wire grants/actors or auth-lock
holding. Final direct policy reads execute without external callbacks afterward.
Unknown source/exposure/clock does not become a zero-filled no-motion report.

Actual shared `camera_access_check` is compiled/read-only, not a wire
`verified=1` assertion. Trusted initialized observer owns actual verified principal
with explicit PROFILE LIVE read grant and SENSOR EVENTS grant. A verified UI-
domain recorder-owner fixture is only a synthetic authenticated principal, not
unrestricted UI access; RTSP's viewing-only domain does not confer EVENTS.
Protected recorder/public notification/endpoint policy remains separate later.

`mm_stage_config` requires actual **ADMIN + RECORDING_CONFIG(sensor) /
RECORDING_MANAGER** before staging masks/settings. Public malformed/alias/wrong-
role requests are inert. One pending config; application only between source
loans, rechecks its saved principal/expiry/revocation before effect. Uncommitted
revoked config is discarded, current committed config stays unchanged. Swaps
terminate existing event with explicit CONFIG_CHANGE gap, reset baseline eligibility
and preserve both display axes; no file/config executor or broad device grants.

Provider functions are owner-installed cached-source callbacks, never wire user
selectors. Actual profile read policy + resource admission checked **before
acquire**, each tile read/staging effect, final background/event effect and every
event query/take. Clock sampled around callback, fresh actual policy check and
expiry after callback,1ms software callback cap (not blocked-callback preemption).
Known ticket/epoch/track/profile/crop/dimensions/complete coverage/digest and source
validity/expiry checked for every raw effect. Source callback `valid` must validate
its actual ledger/immutable producer statistics, not turn a typed flag into proof.

Raw stats span is borrowed and immutable; no owned span is fabricated before
acquire. Copies only576 luma bytes into staging, not a frame/AU. Background
commits atomically after complete digest/coverage + current policy/source checks;
interrupted/denied frame never partly updates it. Loan return occurs only after
the actual bounded read/copy finishes or explicit owner cancel. Denial mid-frame
retains own loan; exact cleanup is still allowed after observer revocation.
Unknown acquire/return quarantines context/backing; missing ticket cannot be
invented and release is attempted at most once. Consumer return is **not DMA STOP,
unload/free or EOF retirement**; upstream actual capture class governs backing.
No source stop/reset/driver operation exists in this ABI.

## Declared algorithm/control table — engineering params, not measured accuracy

| Control/state | Candidate policy / units |
|---|---|
| Sensitivity | Raw mean-code absolute delta≥`delta_min + noise×noise_multiplier`, bounded255; pixel-weighted changed permille of unignored area |
| Noise | Trusted per-cell raw noise code, multiplier0..4; high-noise cells reject salt/pepper delta, no inferred calibration |
| Background | Quiet-small-delta EWMA1/8; changed cells freeze to avoid immediate absorption; startup/config/inhibit requires baseline |
| Motion debounce | Proposed200ms consecutive positive valid source time; BEGIN only after threshold |
| Minimum duration/clear | Proposed500ms event duration AND200ms valid quiet debounce before END |
| Nearly global brightness | Proposed same-sign≥900permille + weighted mean delta≥8 raw codes: explicit GLOBAL_LUMA inhibit/rebaseline; could suppress large real-scene movement, not hidden “no motion” |
| Exposure/PTZ/day-night | Explicit inhibit reason/gap and rebaseline; head/sensor/profile/epoch association exact, no global any-camera PTZ flag |
| Gaps/unknown/EOF/clock/digest | Explicit UNKNOWN/INHIBIT/gap termination, no missing-source→zero/no-motion assumption; frame sequence continuity and max gap500ms (equality allowed), source/event expiry at equality denied |
| Events | Two static BEGIN/END/GAP messages, lifetime namespace/nonwrap sequence/config/source generation, TTL proposed2s, peek/take exact-once; full queue backpressures frame acquisition instead of overwriting/replaying |

Stats mean and thresholds are raw units, not lux/RGB/absolute physical motion.
Source availability/crop/head/exposure receipts are software evidence, not native
capabilities. Pause/failure/gap is **not continuous recording accepted**. Event
queue expiry retains gap semantics; metadata alone cannot validate footage.
No existing video2s/10s/CRC/lease/allocation/128KiB/CMA16/40MiB/security limits change.

## Evidence/build and one next parent dependency

`build.py RUN --toolchain EXISTING_PIN` creates fresh own-prefix snapshots/logs;
optimized+ASan/UBSan, independent rectangle/edge orientation oracle + synthetic
motion/noise/flicker/masked patches/privacy/config/lifetime faults, C++ C API
and ARM static link (**never executed**). Exact source/licensing/header/compiler/
runtime/commands/failures/stack costs are recorded; old source/tests are read-only.
HOST costs are not ARM15fps/headroom or actual-night/detection precision.

**One parent next dependency:** qualify an actual low-cost complete576-cell luma
statistics producer for the measured profile/crop/orientation, with actual valid
lease/cache/digest/frame/exposure/shared-head timing receipts under fresh protected
ownership/runtime/recovery. No real-frame/native tuning or recording qualification
from these synthetic arrays, no SD/footage read/write or other bench operation.
