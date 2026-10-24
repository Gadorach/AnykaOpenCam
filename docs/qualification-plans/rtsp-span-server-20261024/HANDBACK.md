# Real authenticated span server — source handback, NOT RUN

2026-10-24. RP-09/B07, bounded STREAM-01/02/03/07 and CAP-01/05/07.
**Candidate delivered for independent ordinary review; no self-adoption/GO.**
The actual authenticated server body now connects the accepted v2 chunk hub to
the accepted span RTP packetizer. This is source integration, not a deployable
app/native admission, allocation-cap acceptance or completion of these tasks.

## Delivery and exact identity

Only `apps/rtsp-auth/rtsp-span-server-candidate/` and this qualification directory
were written. No Git/index, AGENTS, central plans, original runtime/framework/
headers/hubs/seals/builders, private configuration or hardware was touched.
No subworkers. No target/program/test/harness/callback/backend/verifier/socket/
provisioner/OS-random/media function was called, even on HOST. No negative,
adversarial, mutation or fuzz coverage was run or marked passed.

The app directory has exactly four files: one changed real server C, one new
source header, scoped license notice and fixed offline compile/REL recipe.
No copied runtime tree. Original GPL-2.0-only notices and licenses are retained.
Contract was written BEFORE the source derivative: [CONTRACT.md](CONTRACT.md).

| File | SHA-256 |
| --- | --- |
| `rtsp_span_server.c` | `529fd4902931467dc896b6faa12d502a20846cc0ba4cb76060044b453a018f97` |
| `rtsp_span_server_v1.h` | `8f6488595576c4205b9e9548f18f5e63109b41398cfab6319ce702261b980a07` |
| `build.py` | `2568bcf4ef6712a712981a99a6adb4ed5e98452933855cebffa3fc97de242951` |

Exact forward/inverse diffs: `build03/SERVER_DELTA.patch` and
`build03/SERVER_INVERSE.patch`. Consumed new C/header/recipe/notice copies are
under `build03/consumed-*`; originals remain referenced, not overwritten.

## Actual callgraph and types

New export only:
`camera_rtsp_prestarted_span_listener_serve_v1(actual candidate_prestarted*,
original options*, original producer*, borrowed actual camera_span_hub_v1*,
explicit typed retention*, actual listener slot*, original outcome*)`.
The header includes the ORIGINAL prestarted/listener types and the actual **v2**
hub header. V2 retains v1 spelling but has different ownership layout; no v1 hub
implementation/old combined REL is used. This is not a binary replacement;
compile callers against the new entry and never link the old server unit too.

```
new entry -> original READY/one-attempt phase consumption
 -> original backend READY/profile/known child/empty peer checks
 -> options/source/pristine hub checks (legacy hub_storage must be NULL)
 -> original owned_listener_observe -> backend poll -> fresh checks
 -> original owned_listener_move (caller FD=-1; consumed=1)
 -> original source_guard_bind(capture_admitted, live gate)
 -> original accept / candidate_peer_open / bounded input policy
 -> original candidate_request_verify -> real verifier grant
 -> guarded -> poll / source_ready / real effect CHECK / final fresh CHECK
 -> v2 subscribe + actual lifecycle attach
 -> capture_admitted -> actual BEGIN -> capture_admitted AGAIN -> actual next
 -> source_packet_validate -> fresh gate -> publish_source
    -> <=8 borrowed <=128KiB input spans -> actual v2 publish -> owned chunk copy
 -> actual producer release -> exact actual END
 -> send_access_unit -> v2 acquire -> actual consumer-tagged view/token
    -> phase0 SPS, phase1 PPS (bounded original contiguous packetizer)
    -> phase2 VCL (actual v2 hub packet wrapper -> accepted span packetizer)
    -> original nonblocking partial TCP send -> whole-packet cursor commit
    -> exact v2 scalar release -> pending control / detach
 -> original private runtime cleanup -> close clients -> v2 STOP + drained
 -> zero active tokens -> actual retire/source STOPPED/no demands/no active work
 -> original final runtime cleanup -> guard unbind -> listener close_once
 -> original SERVED only if result and unresolved ownership are both clean
```

Start/finish/backend/known verifier/listener implementations are unchanged.
No serve-time fork, backend init/destroy, second verifier or internal bind path.
The original retain_verifier requirement is unchanged. Existing candidate_config
still has its mandatory old retention field for original startup validation;
this derivative NEVER invokes it, with NULL, a fake old hub or a new-hub cast.

## Ownership and uncertainty

Caller must actually initialize the persistent v2 owner after the last verifier
fork/required native ordering and admit its real backing BEFORE serve. These
prerequisites are NOT implemented here or represented by a permission Boolean.
The pristine checks inspect self/ACTIVE/zero publication and token history,
all consumer bits, eight canonical nonnull nonoverlapping chunk ranges and the
real hub's allocation-history shape. They run before listener consumption and
again after owner callbacks. They are not malloc provenance, order or residency
proof. True-fresh storage/no aliases/serialization are trusted C requirements.

No hub allocation/init/dispose/reinit/free occurs in the serve body, including
failure cleanup. Caller owns the whole hub and external source/media process
through every refusal, success and HOLD. A pre-move refusal leaves that hub
untouched and listener caller-owned, but the original admitted READY serve
attempt is consumed. A clean consumed return leaves a stopped/drained borrowed
hub for explicit caller disposition, never an implicit disposal receipt.

The typed sink/context are explicit stable caller inputs, checked before move.
The sink must retain the whole process/stack/cursors/native owner and external
media owner, not just copy demand/view pointers. Consumer calls provide the
exact stored demand record; source/global calls use NULL/index4 and retain ALL
records. No real sink or external owner implementation is supplied in this lane.

`retain_span` first latches actual owner source_hold/HOLD, invokes that typed
sink, calls existing owned-runtime final cleanup if it returns (which may itself
quarantine native work), then repeatedly raises SIGSTOP. It never returns,
exits/aborts, closes FDs, frees backing, retries END or clears a held cursor.
`retain_frame` preserves the original real frame notification then enters that
nonreturning path. A failed exact v2 release retains BEFORE clearing active/media
flags. Failed unsubscribe/detach, clock/global-retire/source uncertainty cannot
unwind into normal stack/FD cleanup. Any non-OK END is conservatively retained,
even if a particular callback closed its bracket while reporting a source error;
there is no attempted classification into a false safe cleanup permission.
Original native cleanup can itself retain sooner; no survival/recovery guarantee
or independent power/host-loss coverage is inferred from either loop.

## Ordinary author audit (static only)

- [AUTHOR_STATIC_CHECKS.json](AUTHOR_STATIC_CHECKS.json): 15 original function
  bodies remain exactly byte-identical: guarded, capture_admitted, both clock
  wrappers, write_counted/write_all, reply/challenge, failure/input notifications,
  begin/end/retire/idle checked wrappers and source_packet_validate. Not an
  independent review or semantic proof. Other changed regions are in the diff.
- Real Digest/expiry/rate/policy/request paths, source guard after BEGIN and
  immediately before copy, and actual producer release/END order remain present.
  Producer token/generation remain opaque, distinct from hub tokens. No payload
  pointer is used after release; only independent scalar facts and owned copies.
- Cursors contain actual small v2 views, two bounded parameter pointers, scalars
  and ONE1404B pending packet each. There is no lease.data, full-NAL concat,
  per-client NAL copy or new1MiB temporary. Parameter bytes are additionally
  bounded to256, matching original native parameters[256]. No VCL contiguous
  packetizer call: phase2 calls camera_span_hub_packet_v1.
- Each build and each partial-write continuation rechecks real auth/policy,
  exact consumer-tagged lease and fresh whole-AU deadline. Pending control does
  not split packets or renew the deadline. Sequence/offset advance only after
  complete writes. Failed/partial AUs close the connection, never skip ahead.
  Original SSRC/channel/coalescing/interleaved behavior remains unchanged.
- Exact per-consumer v2 token/generation release remains before next source call;
  failed release cannot clear the cursor or unwind. STOP + actual drained + all
  four zero canonical active tokens precede a clean return. Native cleanup stays
  before stack/FD cleanup; listener still has one clear-before-close attempt.

## Offline evidence and preserved failures

Command invoked: `python3 apps/rtsp-auth/rtsp-span-server-candidate/build.py`.
Final fixed fresh output: [build03/REPORT.json](build03/REPORT.json).
6 production C compiles (server/v2 hub/span packetizer × HOST/ARM), 2 actual
rooted REL links, **26 successful recorded tool commands**, no runtime calls.
HOST compiler is retained `/usr/bin/cc`; ARM is retained Buildroot GCC14.3.0,
musl/armv5te/marm/soft-float. Exact tools/argv/environment/dependencies/log pins
are in the report, including retained compiler wrapper/real binary/support pins.

Before link the recipe checked original runtime build02 report/object/archive
pins, literal compile defines and `ar t` membership; extracted only selected
members with `ar x` in fresh directories; byte-matched each body; inspected all
selected ELF machine/attributes. Each target reuses21 of23 real archive bodies:
old server member9 and old contiguous packet_hub member17 are EXCLUDED.
No whole old archive/old combined REL, fixture defines, stubs, synthetic provider,
old builder/generator or new crypto/provisioner code. Original production defines
are FORTIFY_SOURCE=2 and real server_crypto_config; no IS_HOST_PUBLIC_ONLY or
NH_MEMORY_PUBLIC_FIXTURE is added. Reused start contains the real pre-serve fork;
link presence is not invocation or native permission.

Final report records197 input pins,251 new dependency paths,363 reused dependency
paths,46 original object-role records (including4 excluded),96 artifact records,
26 logs. Counts overlap and are not a full transitive tool-execution closure.
The author's final read-only check matched current inputs/new/reused dependency
pins. Fifteen unchanged-function comparisons and delivery pins are separate.

| Final REL | Bytes | SHA-256 |
| --- | ---: | --- |
| HOST | 257176 | `ef00f535eacc3014b71882128751c110e899fa9ab3f00c5a662b98354ab55e54` |
| ARM | 171968 | `e6e4a2daa882df087b7081c4023ab31ef94596dae923d08eebadb6a1085d379f` |

Both are REL with zero entry/no program headers, NOT runnable final ELFs. Actual
project relocation closure is satisfied; libc/compiler/linker symbols remain
unresolved. Raw nm retains three camera_rtsp_digest_* UND names with NO retained
relocations after rooted GC; they are explicitly retained in the report, not
silently removed. This is partial-link evidence, not final native ABI/image fit.

`build01` remains FAILED_PRESERVED:3 HOST compiles, one unrooted REL tool command
succeeded but post-link project-symbol check refused unselected Digest wrappers.
`build02` remains FAILED_PRESERVED:3 HOST compiles, one rooted REL tool command
succeeded, then the overly broad raw-nm check refused those now-unreferenced
names. Build03 uses the original runtime's actual-relocation check, not a stub
or suppressed unresolved-reference check. Each failed recipe has12 successful
tool commands and its own consumed source/recipe; neither failure is relabelled.
Across all three recipes:12 C compiles,4 actual REL tool invocations,50 recorded
successful tool commands,2 failed recipe dispositions. No C/header correction
was made between these attempts; only offline recipe/link inspection changed.

## Explicit blockers / next task

Native use is BLOCKED: original `native_adapter.c:83–94` still prefaults ONE
contiguous1048576-byte hub through nh_native_memory_fixed_buffers(void*).
`memory_reserve.h:21–24` and credit_owned retain one begin/end/paid[1] extent;
`owned_runtime.c:68–73` still requires it. No new chunk accounting/prefault call
is supplied or admitted. Passing chunk metadata or one chunk to that void* API
would be invalid, not a solution. Native output still has malloc1048576 at
native_adapter.c:28, and raw backing/cap/2MiB process/protected image/combined
load/CMA/TLS/headroom/external owner/recovery/provisioning/main remain open.
One VCL NAL policy, serialized next-to-AU delivery, generic AU/recorder/RTCP/audio
work remain unchanged, not completion of STREAM-01/02.

Next task: independent ordinary review of THIS source/API/lifetime connection,
then parent disposition. No self-GO, caller/main/bootstrap integration, source
adoption, function call, negative test or native/hardware queue. Parent alone
owns hardware; parent-provided last camera boundary stays OFF/final23h/no lease,
RAM75 recovery/C1 blocked. Nothing here establishes fresh physical state.
