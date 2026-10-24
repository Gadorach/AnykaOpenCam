# Authenticated span server — independent ordinary SOURCE/API/order review

2026-10-24; RP-09/B07, bounded STREAM-01/02/03/07 and CAP-01/05/07.

**Disposition: CORRECTION REQUIRED for R1/R2 below.** The real span data path,
typed retention mechanism and retained compile/REL evidence are present, but
uncertainty is not consistently retained before local ownership/cursor cleanup.
No narrow lifetime/order sufficiency for this exact revision. This is neither
runtime/native/cap/full-RTSP acceptance nor a parent adoption decision.

## Ordinary correction-required findings

Locations below refer to `apps/rtsp-auth/rtsp-span-server-candidate/rtsp_span_server.c`.

### R1 — clock-fault AU cleanup retires the cursor before retention

`finish_au:342–360` detects `milliseconds()==UINT64_MAX` at line346, but merely
sets error fields. It then calls the real v2 release (354), clears `a->active`
and `c->media_lease_active` (357), and can erase the pending partial packet (359)
before reaching `close_client`. That function's native cleanup is only at198;
if it returns, `pause_client` can unsubscribe/clear playing before its failed
clock read finally causes the typed retention path (178–189, 203–205).

This path is reachable from the explicit clock-error branches at375/399, as well
as a clock failure detected inside `finish_au` itself. The sticky clock wrapper
returns MAX on all later reads (`framework/include/camera/rtsp_clock.h:11–22`).
Thus successful scalar CPU release is being used to proceed with local retirement
after a global clock fault; the retained sink no longer receives the original
active cursor/token/partial-packet state. Actual native cleanup may quarantine,
but it is reached **after** those mutations here. The eventual nonreturning sink
prevents stack unwind; it does not make the preceding order satisfy the selected
retain-before-cleanup contract. No actual free or native failure was observed.

**Minimum correction:** before AU release/active-flag changes/packet erasure,
route any sticky/just-observed clock fault to the existing nonreturning retention
path, retaining the whole live cursor array/process. Also make `close_client` /
direct `pause_client` entry retain a preexisting clock/ownership error before
unsubscribe/detach/local retirement, even when original runtime cleanup returns.
Keep ordinary authenticated socket/deadline failure cleanup distinct; this does
not require retaining every ordinary send timeout or changing the packetizer.

### R2 — demand ownership is cleared before detach is acknowledged

`pause_client:181–190` sets `demand_attached=0` at184 **before** the actual detach
callback and its post-callback clock check. On non-OK detach or a MAX post-clock,
it only reports failure and returns. For example, direct PAUSE at288–291 then
returns an error to the loop, whose `close_client` calls `pause_client` again;
the already-zero flag skips detach, and only then does205 enter typed retention.
The exact demand scalar remains stored, but the local ownership flag already
claims it is detached without an accepted receipt. The failure notification is
not a detach acknowledgment. This is inherited code, not corrected by replacing
the old retention call with a nonreturning typed sink later in `close_client`.

**Minimum correction:** keep `demand_attached` set through the actual callback
and successful post-clock check. On any failed/uncertain result, latch the error
and enter the existing nonreturning retention path before clearing that flag or
returning into another cleanup pass. Clear it only on acknowledged success.
Preserve exactly-one callback attempt; immediate nonreturn on uncertainty avoids
needing to disguise an attempted detach as successful ownership retirement.
Any retained failure notification must not authorize retry/STOP/unwind.

These are static ordinary control-flow findings, not executed negative cases.
They do not reject the already-accepted v2 hub or span packetizer.

## Other inspected source paths

- Full new C/header/recipe/notices, CONTRACT/HANDBACK/author checks and forward /
  inverse diff were read. Compared the actual old owned-runtime server, full
  auth/start/finish and verifier adapter, listener custody, owned-runtime cleanup
  and common producer hooks; read accepted v2 C/header/review/parent and accepted
  span packetizer C/header/parent. CAP/STREAM plans and current ledger were read.
- New typed entry preserves BUSY-first/no-outcome-touch refusals, consumes a valid
  READY attempt before option/backend refusals, uses the same actual backend /
  verifier / owner and original observe→poll→fresh checks→listener move. Pristine
  hub is checked again after owner callbacks and before listener consumption.
  `options.hub_storage` must be NULL. No hub init/allocate/free/dispose/reinit call
  exists in this server. Fresh-after-last-fork backing/order remains caller work.
- `retain_span:54–61` does latch source_hold/HOLD, calls the mandatory typed sink,
  invokes actual runtime cleanup if it returns, then repeatedly SIGSTOPs. No
  close/free/exit/abort/END retry/unwind on that path. `retain_frame` preserves the
  original frame notification first; it can itself nonreturn natively. Neither
  a function pointer nor fallback process suspension implements external ownership
  or guarantees survival/recovery. No old-hub cast/fake NULL old callback is used;
  original startup still requires the old config retention field.
- Actual path is validated contiguous producer input → at most eight borrowed
  <=128KiB copy-input spans → v2 owned shared copy → original producer release →
  exact END → four consumer-tagged/token-bound views → real v2 packet wrapper →
  accepted span builder → real nonblocking partial TCP sends. No source payload
  pointer is read after release, lease.data, full-NAL concatenation or per-client
  NAL copy. SPS/PPS alone use the original contiguous builder, bounded to256B.
- Actual request/Digest/expiry/readiness checks, fresh post-BEGIN/precopy gate,
  source namespace/generation validation, lease/auth checks before each build and
  partial-send continuation, unchanged whole-AU deadline and complete-packet-only
  sequence/offset commit remain. Pending control cannot renew that deadline or
  insert a reply into an already partially written RTP packet. Ordinary partial
  send errors close that connection rather than repair/skip the packet.
- Failed exact v2 release retains before clearing active flags (354–357). Failed
  unsubscribe retains at179/276/278/651. Non-OK END retains the frame immediately;
  failed global retire/sticky clock at final cleanup retains before the client
  loop (690–694). STOP + actual drained + all four zero canonical tokens precede
  a clean return; source STOPPED/no demands/no active work precede guard unbind /
  listener close-once. Borrowed backing is not disposed. R1/R2 identify earlier
  paths not covered by those otherwise-correct final checks. Returned HOLD/source
  hold and unresolved backend state remain incompatible with original finish
  admission (`candidate_auth.c:242–257`).

## Exact identity and read-only evidence checks

SHA-256; paths are workspace-relative. New app prefix is
`apps/rtsp-auth/rtsp-span-server-candidate/`; evidence prefix is
`docs/qualification-plans/rtsp-span-server-20261024/`.

| File | SHA-256 |
| --- | --- |
| new `rtsp_span_server.c` | `529fd4902931467dc896b6faa12d502a20846cc0ba4cb76060044b453a018f97` |
| new `rtsp_span_server_v1.h` | `8f6488595576c4205b9e9548f18f5e63109b41398cfab6319ce702261b980a07` |
| new `build.py` | `2568bcf4ef6712a712981a99a6adb4ed5e98452933855cebffa3fc97de242951` |
| new `LICENSE_NOTICES.md` | `c0c0f6af4d5f5ac23a3c6e094b60279605e8ae0f9767d1fd2b7cdc9dd05e22ad` |
| `DELIVERY_PINS.json` | `8190b6b062de0a7b4f6f3e7837e4fa3786b8f097495fe9916c11474250bbc603` |
| `CONTRACT.md` | `fe06ef5512529366bced1c5bbda7095b87ef167adba4297d6e9afd6401387ea6` |
| `HANDBACK.md` | `666cb889ae41280dfb239f64609e1ce6bbcf932bd60901aa03c4e0df2b5c40ca` |
| `AUTHOR_STATIC_CHECKS.json` | `fb4c9aa877d7148970f23fd15e430d677723115fc9df31fcd056caf8bf226400` |
| `build03/REPORT.json` | `ed3d1b1ceb2574f3a7d35e909a962b490c28e7c8fd0350572552deeab2a31ec5` |
| old owned-runtime server C | `fb47ac06be587b8bac793c272c804ece0220601e4a1d0810164295b0c4dd5ce1` |
| `apps/rtsp-auth/span-hub-v2/span_hub.c` | `bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1` |
| its `span_hub.h` | `310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454` |
| `apps/rtsp-auth/rtp-span-candidate/rtp_h264_spans.c` | `a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e` |
| its header | `41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91` |

Read-only stdlib metadata/hash inspection matched final197 inputs,251 new and363
reused dependency paths,96 artifact records,26 hashed exit0 command logs and205
delivery pins, plus earlier artifact/log pins;778 distinct public files were
hashed in that verification pass, no pin mismatch. Counts overlap, not whole
tool-execution closure. Both dependency-file memberships matched exactly. Saved
forward/inverse diffs regenerate exactly;15 claimed unchanged function bodies
also match actual old/new source, not blanket semantic proof. C/header consumed
copies are identical across all three builds; consumed recipes differ and remain.

Final author evidence is6 production HOST/ARM C compiles and2 rooted REL links,
26 commands PASS, NOT RUN; all six compile logs and both link logs empty. Actual
archive bytes contain23 bodies each; all46 match retained objects, selected21 per
target exactly match extracted bodies. Excluded are old server member9 and old
packet_hub member17. All46 role records/defines were checked, including exclusions;
real production FORTIFY/crypto configuration, no HOST fixture or whole-archive.
Fresh inputs are the real new server, v2 hub and accepted span packetizer only.

Actual ELF-byte/symbol/relocation inspection matches saved logs/report: HOST
ELF64/x86-64 REL257176B, ARM ELF32/ARM EABI5 REL171968B, entry0/no program headers.
Both18-root links retain nonempty actual new entry/start/finish/prepare/bind and
live auth/listener/hub/span functions; no main/old serve/old hub export. Actual
project relocation closure is satisfied; libc/compiler/linker references remain.
Raw nm's three `camera_rtsp_digest_{close,issue,verify}` UND names are preserved,
STB_LOCAL/STT_NOTYPE with zero referencing retained relocations. A reviewer parser
initially asserted by counting only global UND names; including these actual
local UND entries reconciled the metadata, without any candidate/build change.

Final REL hashes: HOST `ef00f535eacc3014b71882128751c110e899fa9ab3f00c5a662b98354ab55e54`;
ARM `e6e4a2daa882df087b7081c4023ab31ef94596dae923d08eebadb6a1085d379f`.
Build01 remains FAILED_PRESERVED (report `06a4e68f844260db4997c1ddac7f6cb6e0026fe2ca110e5a84ea90590393b5da`):
its unrooted REL has actual unresolved Digest relocations. Build02 remains
FAILED_PRESERVED (`616243bddc2d0e65de74e4cc0393f758dd7d528b4965a7b5937fad224ac37418`):
rooted GC removes those references, but its raw-nm check refuses the remaining
names. Across all attempts12 C compiles/4 actual REL link tools/50 successful
recorded commands, two failed recipe dispositions. None was rerun or relabelled.

## Remaining conditions and boundary

Trusted valid stable disjoint/private addresses, true-fresh v2 initialization
after the last fork, no aliases/reentry/signals/concurrency/FD mutation and actual
native ordering/backing admission remain mandatory; shape cannot prove them.
Actual typed whole-process/stack/external-owner retention and future caller/main
integration remain source gaps. Use only v2 hub/new server, never old combined RELs.

**Native call admission remains BLOCKED.** Actual old
`nh_native_memory_fixed_buffers` still prefaults flat1048576B; ledger paid[1] is
one extent and owned-runtime readiness requires it. Chunk shape supplies no such
credit. Producer1MiB output/raw backing, real chunk prefault/accounting, process
2MiB/40MiB/CMA/TLS/combined-load, provisioning/external recovery/main and native
ABI/image fit remain open. This is one-VCL serial delivery, not STREAM-01/02 done.

Only this fresh review file was written. No candidate/helper import, build/link,
test/fixture/program/callback/RNG/auth/backend/verifier/provider/credential/socket
or hardware operation; no adversarial/mutation/fuzz matrix, subworker, source/Git/
index/AGENTS/plan/settings change. Parent's last OFF/finalized23h and RAM75 C1 /
recovery block are inherited context, not fresh observations; old failures stand.
Parent disposition is separate. No automatic correction, adoption or next native
queue is selected by this review.
