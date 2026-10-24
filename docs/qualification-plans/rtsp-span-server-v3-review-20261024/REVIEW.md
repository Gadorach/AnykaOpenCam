# Authenticated span server v3 — independent ordinary SOURCE/API/order review

2026-10-24; RP-09/B07, bounded STREAM-01/02/03/07 and CAP-01/05/07.

**Disposition: narrow conditional SOURCE/API/order/lifetime sufficiency for the
focused residual-R1 correction, with R2 preserved. No correction-required ordinary
defect found in this bounded review.** The real direct rollback and producer-release
paths now retain already-observed uncertainty before the selected destructive
effects. This is NOT adoption, runtime/clock-prevention/native/cap/full-RTSP GO.
Parent disposition remains separate; frozen v1/v2 remain NOT ADOPTED.

## Findings and actual caller audit

Locations are in `apps/rtsp-auth/rtsp-span-server-v3/rtsp_span_server.c`.

- **Residual R1 corrected, PLAY 290–302:** both guard-false and actual failed-attach
  rollback inspect sticky `server_clock.failed`, client `lifecycle_error` /
  `lease_uncertain`, and fresh MAX BEFORE actual unsubscribe. The latter fence is
  after the real attach callback. Uncertainty nonreturns through `retain_span`
  with the actual binding/demand/index; ordinary safe denial keeps unsubscribe /
  error-return policy. Failed attach still requires the existing no-admitted-demand
  callback contract (`framework/include/camera/rtsp_server.h:36–42`); no new detach
  acknowledgment or generic callback ownership proof is manufactured.
- **Residual R1 corrected, open frame 685–691:** a clock-failing `guarded()` cannot
  reach unsubscribe, `playing=0` or `deferred_close=1`. The uncertainty branch calls
  actual `retain_frame(&span,source,...)` while the packet, native owner, source
  bracket and process stack are live. It does not route through pause/detach/STOP,
  producer release or END. Known-safe guard refusal alone retains the original
  unsubscribe/deferred-close policy. Hub unsubscribe itself has no callbacks and
  refuses acquired consumers (`span-hub-v2/span_hub.c:73–79`).
- **Adjacent producer ordering corrected, 666–718:** post-next client flags and
  sticky/fresh clock checks retain before any producer release or failed-next END.
  The old release-on-MAX is gone. After actual precopy admission, 703–705 fence all
  client flags and sticky/fresh clock before publish; 709–711 repeat that fence
  before the **sole** ordinary producer release, including skipped-copy/denial
  paths. Safe success stays validated shared copy → release → exact END. Only
  independent scalar facts and hub-owned copies are used after release; no packet
  metadata/payload pointer read survives it. Safe ordinary auth/admission denials
  keep their original release/result/END policy, not unconditional retention.
- **All four unsubscribes inspected:** pause181, PLAY294/301, open-frame690. The
  unchanged pause entry fences177–179 cover direct PAUSE, pending control and
  close callers. No other direct unsubscribe bypass exists in this unit.
- **Exact CPU release/cursor retirement, 366–395:** unchanged v2 fences precede
  diagnostics and release384, and recheck after potentially blocking diagnostics.
  Failed consumer/token/generation release nonreturns before active flags387.
  Pending-control/auth/notification faults are checked389–392 before partial packet
  wipe393 or failure close. Every sender guard/control/deadline/send failure routes
  through this function or fenced close; there is no second CPU-release site.
  Ordinary socket/deadline failure without ownership/clock fault still releases
  its exact CPU lease and closes the partial connection, never skips/repairs it.
- **R2 preserved exactly, 184–195:** `demand_attached` stays true through ONE detach
  and valid post-clock; only detach OK plus that clock acknowledgment clears it.
  Non-OK remains the actual error (OK+MAX becomes IO error); it is latched before
  metadata-only notification, restored afterward, and immediately retained. No
  return into another cleanup/detach attempt. Actual common detach checks bridge
  time then calls producer detach; last-demand drain can notify (`rtsp_server.h:
  123–128`, `framework/src/producer.c:108–115`). Notification is not acknowledgment.
- **Close/local clear, 199–228:** unchanged v2 entry fences precede native cleanup,
  fd<0 return, pause, socket/input/peer cleanup. Post-pause and post-peer fresh
  checks precede later local retirement/memset; clean memset preserves the actual
  span binding. The pending-backend branch does not acknowledge source retirement.
  The close callers after guarded/control/parser/failure notifications were traced.
  Rejecting a newly accepted, unbound socket at600/602 does not release a source,
  demand or CPU cursor. Header/receive/parser zeroization is control/secret handling,
  not an ownership receipt or part of a claim that every scalar stays unchanged.
- **END/global retirement/tail:** all END sites656/676/718 use fresh-clock checked
  END495–500 and immediately retain on non-OK. Post-BEGIN denial has no acquired
  producer packet to release. Retire/idle wrappers477–488 check pre/post time;
  callers625/628/681/724/733/754/755 either retain directly or latch failed retirement
  before `clients_done`. Mandatory lifecycle admission makes legacy no-life idle
  branches unreachable here. At737–760, clock/global-retirement failure retains
  before the client loop; each client close checks its own uncertainty. Actual
  STOP/drained/zero canonical tokens and source STOPPED/no demands/no active work
  precede guard unbind/listener close-once. Failed retirement is not retried.
- **Real nonreturning retention, 54–68:** source HOLD is latched, actual frame
  notification / typed whole-process sink are called, with original runtime cleanup
  then repeated SIGSTOP if needed. Actual runtime cleanup (`owned_runtime.c:54–60`)
  either quarantines or returns without mutating the server clock; it is not an
  ACK. Its fence/quarantine path (`native/native_adapter.c:185–202`) does not END,
  close, free or retry. No fake old-hub cast/NULL old callback, metadata-only owner,
  unwind or exit substitutes for retaining the actual stack/cursors/process.

A fault first observed **after** an earlier safely completed effect retains current
state; these checks do not reconstruct a released packet, an acknowledged CPU
lease, or prior detach state. Callback-internal ownership remains its own actual
contract. No whole-interval C1 prevention follows from a fresh read or sticky latch.

## Identity and read-only evidence

Read full new C/header/recipe/notices, frozen v2 C/header, full v2 and v1 R1/R2
reviews, contracts/handback/control-flow audit/proofs/inspection notes, saved
report/pins/diffs, actual clock/common producer/detach, owned-runtime retention,
auth/start/finish/listener and accepted hub-v2/packetizer C/headers as relevant.

SHA-256. App prefix `apps/rtsp-auth/rtsp-span-server-v3/`; evidence prefix
`docs/qualification-plans/rtsp-span-server-v3-20261024/`.

| File | SHA-256 |
| --- | --- |
| app `rtsp_span_server.c` | `73c0f5886a54ca8fc064b27c72b7c877180730502673e8a6ee106f085a7304d0` |
| app `rtsp_span_server_v1.h` | `3b81af3ff9e7478ec19b3ab7f91414bdffa74cea231c8fc9d6b180a5f63f5333` |
| app `build.py` | `ec057cb3c0d0bd955ef14d6636e6a68b8ca2bc59253f89876e6c87d576eaf20c` |
| app `LICENSE_NOTICES.md` | `bb24cf0278b76124b00fc71d0a79e759f7cfc3ff06bd375396a6430c12c87c00` |
| `CONTRACT.md` | `166a96aeb885fc9ae939702d255f124d98d9b5ca881e26ff3fa425d500e1b3fe` |
| `HANDBACK.md` | `37b934e6f802f4f6ef4bd805d03c3787ba846623a1cd7ee56241d4e13ac9b724` |
| `FROZEN_INPUTS.json` | `26a64600c64a0e810e26305d924b40e616ea19ea5fb0e21d74373192d8265340` |
| `DELIVERY_PINS.json` | `fbbba3c572862dc0458a948f9240e63b1776eecddc95e5e454cd3dbd4d2431e0` |
| `AUTHOR_SOURCE_PROOF.json` | `872e849da2d7c1816e81ac248177d138cc7b78a4ace790f23251912365bfda2d` |
| `SOURCE_DIFF_PROOF.json` | `fb5bc326f06d1b2cf2620039c1406c350f59887c88edfeb599cfb67171d3c215` |
| `AUTHOR_FINAL_VERIFICATION.json` | `5458caecd74c4a772084875de268824b56d1afbdfc9b27cb95bca4f2072265a9` |
| `build01/REPORT.json` | `b5abd45f47ce98596d4f9005ad6536e32e9906c840e6e2f3951337ee3df53e51` |

Independent read-only stdlib checks matched **332 exact frozen members/bytes**,
117 delivery pins,203 build inputs,251 new and363 reused dependencies,98 artifacts,
26 hashed exit0 command logs;1131 distinct public paths were hashed in that pass.
Counts overlap. Both dependency memberships matched. All consumed app copies equal
canonical delivery. All eight saved forward/inverse diffs regenerate and apply
exactly in memory. ALL C bytes outside process_input/serve_prestarted_body match v2;
pause/close/finish_au and selected guard/retention bodies are exact; header
noncomment bytes match. Same exports/types/guard: exclusive SOURCE replacement,
ONLY new server + accepted hub v2/span, never old units/hubs/combined RELs.

Author evidence remains six HOST/retained-musl-ARM production compiles, two rooted
REL links,26 commands PASS, **zero candidate runtime calls**; not rerun. Six compile
and two link logs are empty. Independent actual archive-byte inspection matched
all46 bodies/production role records and the SAME21 selected of23 per target
against retained objects, extraction and v2; old server9/old hub17 excluded. No
fixtures/stubs/old builder/new crypto/whole-archive substitution.

Independent actual ELF-byte/symbol/relocation parsing confirms18 same roots,
nonempty real GLOBAL FUNC roots/live APIs, no main/old-server/old-hub export,
entry0/no program headers. HOST ELF64/x86-64 REL259576B,2786 relocations, SHA256
`a2bfabd0b9ae9a36f25648b7463ee5a387ae95721cb1e85a04dcfd0681f61bab`;
ARM ELF32/ARM REL172948B,2093 relocations, SHA256
`a3853b7b491067f5ea7773242d88b198d92f58dd1de92f6c5799f27d731314db`.
Actual retained project relocation closure matches saved evidence. All three raw
Digest UND names remain explicitly LOCAL/NOTYPE/size0 with zero retained references;
not hidden or treated as missing APIs. Libc/compiler/linker references remain.
Not final ELF/native ABI/full tool-execution/transitive-stack closure evidence.
Two documented author metadata-reader failures remain failures, not changed
candidate/compiler retries. Old v1/v2 findings/build failures remain frozen.

## Conditions and stop boundary

Valid stable disjoint caller storage, no mutable aliases/reentry/concurrency/signals/
fork/FD mutation, synchronous metadata-only notifications, actual callback ownership,
true-fresh v2 hub after the last fork, admitted backing, and a preconfigured stable
external whole-process/stack/media-owner retention sink remain essential conditions,
not facts proved by C pointer/shape checks. No sink implementation or survival/
external-recovery guarantee is supplied. Auth/readiness/listener/post-BEGIN/precopy,
copy/packetizer/partial-send deadlines, SPS/PPS<=256, producer copy-input and STOP/
drain semantics otherwise remain unchanged, not newly observed functional results.

**Native remains unadmitted:** inherited flat kind1/paid[1],1MiB output/raw backing,
128KiB allocation cap,process2MiB/combined-load, external owner/recovery/provisioning/
main and ABI/image fit. The disjoint span-runtime author and accepted memory source
are NOT this source's integrated dependency or adoption. Last OFF/finalized23h and
RAM75 C1/recovery block are inherited parent context, not fresh hardware evidence.

Only this fresh REVIEW.md was written. No candidate/helper import, build/link/tool
rerun, program/function/test/callback/client/producer/auth/backend/verifier/provider/
runtime/network/private/proc/dev/hardware operation, negative/adversarial/mutation/
fuzz case, subworker, source/Git/index/AGENTS/central-plan/settings/safeguard change.
Deferred coverage is NOT RUN, not passed. Stop for separate parent disposition;
no automatic adoption, correction, next work or hardware queue.
