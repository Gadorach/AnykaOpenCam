# Bounded G.711 + borrowed-span RTP audio candidate — AUDIO-04/05/06

**Source/HOST unit for parent review. No native microphone/track/transport or
AUDIO/G4/G7/G8/A/V150ms acceptance.** Existing media stays501/NOAUTH; no SDP,
browser track or capture driver/config/policy/framework/mux is enabled/edited.

## Codec and provenance

`g711.c/.h`: bounded S16LE byte encoder plus µ/A-law sample encoder/decoder.
The µ-law portion adapts the full16-bit Sun/ALSA convention (bias132, biased
magnitude saturated32767, zero trap disabled), from the existing pinned Linux
`sound/core/oss/mulaw.c` LGPL-2.0-or-later source. Original Kysela/Bizjak/Sun
attribution, complete source/notice and license are preserved; the HOST reference
uses unchanged selected leaves, not our segment-search implementation. A-law is
a newly implemented standard sign/segment/mantissa/toggle arithmetic routine under
the same LGPL license; no ITU document or software redistribution grant is
invented. Packet/application sources are new GPL-2.0-only code; no workspace-wide
license is assigned. Full notices/dependency pins are in each evidence run.

Independent Python **quantizer interval inverse** / decoder oracle covers all
65536 S16 values and all256 law codes. µ-law also matches the original licensed
ALSA reference. Legitimate companding is lossy (µ peaks±32124; A peaks±32256;
A zero decodes+8, µ positive/negative zero both decode0). This is not camera
clipping/entropy fallback or a claim original PCM is recovered pixel-perfectly.
Existing FFmpeg, if installed, independently decodes all256 words per law with
bitexact comparison to C/Python decoded S16LE, file-only/no network/device use.

Standards mapping: ITU-T G.711 integer companding conventions; RFC3551 §4.5.14
and static profile table PT0=PCMU/PT8=PCMA,8000Hz mono; RFC3550 RTP header/wrap/
random-source requirements. These are citations, not newly fetched certification
or claimed ITU software licensing. Encoder convention is explicitly validated,
not an assumed16k→8k conversion.

## Implemented resource/timing contract

### V2 authority correction — grant BEFORE source subscription/borrow

Parent found V1 called `ac_borrow` before admission. A revoked/unauthenticated
consumer could change READY→BORROWED and starve the source despite DENIED.
V1 source/HANDOFF/run03/all originals were preserved byte-exact before editing
under the own-lane `v1-snapshot/`; original passes did not prove this boundary.

V2 requires a distinct trusted `am_preborrow_fn` **before any ac_borrow/helper
counter/key mutation**. `am_borrow_identity` contains known stream namespace/
generation and current source owner/clock/track/format context, **no capture
ticket, slot/data pointer, sample index or fabricated owned span**. Actual profile/
principal/resource admission belongs to the callback context. Guest, revoked,
expired, wrong-role/domain, stale-profile or resource denial is inert: helper/
capture counters, keys, READY/pending/borrowed and output remain unchanged.
Malformed/null/known-storage-aliased begin outputs are inert before callbacks.
Repeated bad begins cannot claim the four source slots or cause an XRUN.

Trusted monotonic `am_clock_fn` is sampled before/after callbacks; no UTC/wire
timestamp is promoted into source time. Each pre-borrow and owned-effect check
performs real policy/admission again at fresh post-callback time, rechecks current
source context/span, and requires actual minimum principal/admission expiry
(`valid_until_ns`) still in the future after final return. Equality, clock
regression, source/generation/mute mutation and callback overrun deny before the
effect. Callback pair≤1ms is a software admission cap, **not preemption or a
physical timer guarantee**. The final trusted proof callback must check current
policy without mutating authority afterward; a lying adapter cannot be made
authentic by metadata. Callback windows are trusted owner outputs, never wire
`verified=1` or fabricated leases. Caller still serializes checks with effects.

Only after this successful pre-grant/source revalidation does the helper acquire
a real loan. Policy/resource races **after** acquisition still run actual span/
ticket/track checks before encode and every header/payload/queued continuation;
denial retains the correct loan until explicit done/discard/cancel. No source
STOP/reset/free or DMA retirement follows from client errors. Codec/rate/packet/
storage/seed-entropy requirements are unchanged. Parent independent review and
genuine8k/native/client/150ms A/V acceptance remain open.

`audio_packet.c/.h` uses the **actual reviewed capture_core.c/.h and ac_span_valid**,
pinned/copied read-only for builds. It acquires one real consumer borrow via
`ac_borrow`, never a peer-supplied pointer/`verified` flag.8k MONO/S16,512B/256
frames and exact rational rate are mandatory.16/24/48k/unknown formats refuse
without resampling/decimation or silent fallback. Future resampling needs a
separate native quality/CPU/ownership qualification.

- One period: raw512B stays borrowed; encoded256B + RTP12B fixed storage.
  Entire helper≤1024B static, enforced by compilation; one≤64B offered chunk,
  no heap/queues/lookahead/full-AU/TLS copy or ceiling changes.
- RTP timestamp is random caller seed + relative actual64-bit source sample
  index; low32 wire wrap, extended64 ticks/sequence and monotonic source epoch
  tracked. SourcePTS uses exact8k sample period125000ns; no dequeue/UTC/FPS
  guess. Gaps, source/resource/track/capture-generation changes, clock regression
  and overflow invalidate unsent content. SSRC/timestamp/sequence seed must come
  from parent-qualified entropy/unique-session owner; no verified-entropy flag,
  PID/clock/MAC pseudo-entropy or RNG implementation exists here. PUBLICDUMMY
  constants are fixtures, not secure native provisioning.
- Caller must serialize all methods, capture state and actual policy/admission
  with effects. Trusted `am_admit_fn` runs actual protocol/session/resource policy
  and admission **before encode, every header/payload copy, every queued/partial
  continuation and progress**. It never accepts wire roles or constructs auth
  principals. Callback result1 is a trusted adapter result, not authentication
  from a typed flag. HOST tests use real copied shared policy core with trusted
  PUBLICDUMMY issued principals; a synthetic camera0 live-profile grant is not
  a registered/advertised physical microphone or second camera.
- `am_offer` is a guarded bounded copy, **not socket/TLS I/O**. Immediately before
  actual queued/partial transport emission caller MUST `am_recheck` the exact
  current chunk and atomically serialize check→effect→`am_advance`. Past success
  is not a lease; partial or zero-write retries must recheck. Helper cannot
  control an external writer that ignores this contract or a lying adapter.
  Rechecks use exact capture ticket/current span/track/bounds, before and after
  admission callback. Source mutation during callback denies old bytes; callback
  must itself verify actual current policy before returning, not cached authority.
- One chunk/work serial is nonwrapping and namespaces stream/generation/capture/
  track identity. Malformed/stale/aliased offers/receipts cannot advance current
  work. Complete accessible typed objects are required, not arbitrary pointer
  dereference protection. No SDP/XML/string/profile injection parser exists;
  format descriptor accepts only the two law enums and fixed PT/clock/channels.
- Mute/revoke/expiry/source fault denies future effects, scrubs helper's unsent
  header/codes, keeps the raw loan until caller confirms cancellation/done.
  Trusted transport must discard all external queued copies; previously sent
  bytes cannot be recalled. No implicit return on denial or socket EOF.
  `am_done` after all real byte progress, or explicit consumer `am_cancel`, returns
  the exact borrow once. It **never** calls capture STOP/source-fault/free/reset.
  Capture's already-known STOP+RELEASE+held-return class alone governs DMA/code
  retirement; last return can trigger its RELEASE only after known source STOP.
  Unknown/duplicate-external return quarantines helper; no automatic retry/free.
  Cleanup with regressed source time is refused to avoid manufacturing capture
  clock-fault STOP from a media error. Caller retains all backing/context until
  actual coordinated loan/retirement proof.

## Container/client choices — compatibility, NOT product enablement

| Candidate | Offline result / honest limit | Actual qualification still needed |
|---|---|---|
| G.711 PCMU/PCMA over RTSP/RTP | Standard8k PT0/PT8 bytes/header helpers, synthetic decode only | Real8k mic/source, protected RTSP account/transport, owned Frigate/Blue Iris formats/controls/playback/load |
| PCM/S16/WAV fixture | Simple file/container/host decoder fixture option; no capture/recording implementation | Native resource cost, source formats, STORE container/playback and actual browser support |
| Browser MSE audio | **Not implemented/advertised.** Canonical retained MSE proof is H.264 video-only; G.711 RTP/raw/WAV decode does not imply MP4/MSE audio | Actual protected audio codec/container, browser negotiation/timing/CPU/load tests |
| AAC/Opus | Hardware support unknown; no library install/transcode/product selection | Actual supported native/codec/licensing/CPU/memory evidence; no heavy software fallback by assumption |

No fake silent tracks/SDP/time, audio sample file from a device, NVR client contact,
speaker/playback activation or second camera. Combined A/V drift, cold entropy,
mute transport effects and all real client/native gates remain OPEN.40MiB/CMA16,
2MiB credential/TLS budget, video2s/10s/CRC/locking and service501 remain unchanged.

## Reproduce / one next real dependency

`python3 apps/audio/media-candidate/build.py RUN --toolchain EXISTING_PIN` uses
fresh own-lane outputs, copies exact reviewed capture/policy source headers (no
edits or old suite reruns), runs optimized/sanitized/C++ and ARM static link
(NEVER EXECUTED), preserves licenses/reference leaves/oracles and full source/
compiler/header/runtime/tool closure. HOST process-CPU costs are labelled HOST,
not ARM FPS/headroom; vector/synthetic WAV files stay inside evidence only.

**One parent actual dependency:** protected/read-only-admitted native mic format
qualification proving a genuine MONO/S16 **exact8k** clock/rate/first-sample epoch
and safe resource lifecycle. The capture probe still refuses until routing/bias/
L2/cache/IRQ are measured/reviewed. No native PCM or150ms A/V claim from this unit.
