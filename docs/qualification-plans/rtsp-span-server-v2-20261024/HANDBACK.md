# Authenticated span server v2 — R1/R2 source handback, NOT RUN

2026-10-24. RP-09/B07; bounded STREAM-01/02/03/07, CAP-01/05/07.
**Candidate correction delivered; independent ordinary review and parent disposition
required BEFORE ANY use. No self-adoption, runtime/native/cap acceptance or next queue.**

Only `apps/rtsp-auth/rtsp-span-server-v2/` and this qualification directory were
written. Contract precedes the derivative: [CONTRACT.md](CONTRACT.md). No Git/index,
AGENTS/central plans, frozen runtime/helper/source/review, private or hardware edit.
No subworkers. The disjoint memory candidate was neither read nor modified/adopted.

## Exact identity and replacement rule

SHA-256:

| New file | Identity |
| --- | --- |
| `rtsp_span_server.c` | `8bebe38b59a2690921074bb1fd348c2504503716433b0d416925a7e2592cc020` |
| `rtsp_span_server_v1.h` | `ae1fcbe656a5ded4fba5b06c7d5114ee9652f9287187df66c65c3b2b8333a7b1` |
| `build.py` | `52f010b9ce4a67d634ed13e7877923458af5c0ee37ff50965ec6f19664e78e18` |
| `LICENSE_NOTICES.md` | `2ff6da57865eea15d4dd52825094f30ab51d9174f084e9ff221574dec414bfba` |
| `build01/REPORT.json` | `a4fb08f2c98e0c24eefe50361079fd90f1ad415aaed8b6f00c69edc0dd188d1a` |

SAME exports, types and include guard as v1: compile consumers with this
replacement C/header ONLY, accepted span-hub-v2 C/header and accepted span builder.
Never link old server units, old hubs or old combined RELs alongside this unit.
Original GPL-2.0-only notices/licenses remain referenced and unchanged.

Exact v1 C/header pins are respectively
`529fd4902931467dc896b6faa12d502a20846cc0ba4cb76060044b453a018f97` /
`8f6488595576c4205b9e9548f18f5e63109b41398cfab6319ce702261b980a07`.
Full independent v1 REVIEW was read and its expected SHA matched
`5ae6708f00f3e5e6465730b9b47c6726278964a4bc3e294cba25bae8cf4b9871`.
V1 stays frozen **NOT ADOPTED**: its R1/R2 findings and earlier build failures are
not erased, reclassified, fixed in place or counted as v2 evidence.

## Only three C function boundaries changed

### R1 — retain before observed-uncertain cleanup

* `finish_au:356–386`: preexisting sticky clock/lifecycle_error/lease_uncertain
  enters the original nonreturning `retain_span` before logging or local retirement.
  Its initial actual clock MAX also retains immediately. After possibly blocking
  TCP/AU diagnostics, another real clock read gates exact hub release and active /
  media_lease_active clear. Failed exact release retains before those flags clear,
  unchanged. After `pending_control`, sticky ownership checks and another fresh
  clock read precede packet erasure or failure logging/connection cleanup.
* `close_client:199–228`: preexisting uncertainty retains BEFORE native cleanup,
  fd<0 return, pause/unsubscribe/detach, socket/input cleanup or local clear. Fresh
  clock reads guard entry, post-pause cleanup and post-blocking peer retirement.
  `pause_client:175–198` has the same preexisting/fresh-clock entry boundary before
  unsubscribe/playing clear. Thus a returning original runtime cleanup is never
  mistaken for permission to retire clock-uncertain local ownership.
* One necessary same-function pointer preservation: clean close's memset now
  restores its ACTUAL `span` binding. Otherwise a later required fd<0 uncertainty
  retention could receive NULL instead of this live whole-server destination.
  No new ownership state, field, generic guard/helper or clock framework was added.

`guarded`, `control`, pending control and failure notification can discover faults
between earlier checks and cleanup; finish/close/pause now check at the applicable
mutation boundary. A fault newly detected AFTER an earlier known-safe release or
close does not retroactively invalidate/reconstitute that earlier state. These
checks do **not** establish whole-interval clock prevention or independent recovery.

Ordinary socket/send/auth refusal/whole-AU timeout remains distinct: absent clock
or ownership uncertainty, release the exact safe CPU lease and close that TCP
connection under the original partial-AU policy. No retain-every-socket-error rule,
deadline renewal, stop/auth policy or packet-send change was introduced.

### R2 — one detach, ownership clear only after acknowledged success

`pause_client:184–195` keeps `demand_attached` true while the ONE real detach is
called and the actual post-callback clock is checked. Non-OK remains the actual
detach error even if the post-clock also fails; OK plus MAX becomes IO error.
The error is latched before original `failure_client` notification and preserved
afterward, then immediate existing nonreturning retention precedes flag clear,
return or any later cleanup attempt. Notification cannot grant ACK/retry/STOP/END.
`failure_client` itself is unchanged (including its sticky-clock refusal). Only
OK plus valid post-clock reaches demand_attached=0. Actual demand scalars stay in
the live client record delivered to the sink; no attempted-as-detached flag.
Original unsubscribe/playing-clear order otherwise remains; an unsuccessful
detach never pretends those earlier local changes are a demand acknowledgment.

The trusted lifecycle contract is synchronous, metadata notification only, stable
tokens/context, no reentry/asynchronous completion. Actual common detach first
checks bridge time then calls actual producer detach, which can drain/notify;
that is why the post-clock is required. These are source-order findings only.

## Unchanged implementation and proofs

[AUTHOR_SOURCE_PROOF.json](AUTHOR_SOURCE_PROOF.json) verifies ALL C bytes outside
pause_client, close_client and finish_au, not merely selected function names.
Header noncomment bytes are identical, independently checked in
[SOURCE_DIFF_PROOF.json](SOURCE_DIFF_PROOF.json). That file records exact in-memory
forward AND reverse application of both saved C/header unified diffs back to the
original bytes; no candidate C execution. Diffs are `build01/SERVER_{DELTA,INVERSE}.patch`
and `build01/HEADER_{DELTA,INVERSE}.patch`. Recipe/notice diffs are at this directory
root. Consumed source/header/recipe/notice copies are in `build01/consumed-*`.

Consequently actual auth/listener/backend/start/finish, source guard and callback
semantics, producer release/END, borrowed pristine-hub checks, per-consumer v2 hub
ownership, packetization/partial-send cursor, whole-AU deadline and STOP/drain
tails remain unchanged. `retain_span:54–61` is byte-identical: owner source_hold /
HOLD, actual typed sink, original runtime cleanup if it returns, repeated SIGSTOP.
It retains whole live process/stack/cursor array and external-owner obligation,
not copied tokens; no exit/clear/free/FD close/END retry/unwind on uncertainty.
The original native cleanup may quarantine first; no external sink implementation
or survival/all-domain recovery is supplied by this source correction.

Accepted hub C/header remain
`bcf50041bb1fe1b20352d92d1ec9ba61d9e70156fa2cf4c96b06378d837322b1` /
`310160cf0b3bf753daa199193a5674e9e223e036af467e320cc5905e415b3454`;
accepted span C/header remain
`a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e` /
`41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91`.
[FROZEN_INPUTS.json](FROZEN_INPUTS.json) records217 original files across v1 app,
v1 evidence, v1 review and accepted hub/span directories. Exact membership AND
all bytes matched before and after the fresh build. No frozen file was rewritten.

## Single fresh compile-only evidence

Command: `python3 apps/rtsp-auth/rtsp-span-server-v2/build.py`.
Fixed exclusive output `build01/`; refused overwrite/no retry loop. **PASS**:
6 real production C compilations (new server, accepted v2 hub, accepted span C,
each HOST + retained musl ARMv5TE) /2 actual rooted REL links /26 exit0 recorded
tool commands. Six compile logs and two link logs are empty. No earlier v2 failed
attempt. Exact argv/cwd/environment/timestamps/tools/support pins and logs are in
`build01/REPORT.json`; toolchain remains retained GCC14.3.0/marm/soft-float ARM.
No original build helper was imported or invoked.

Reused the SAME21 of23 reviewed real runtime archive bodies per target, checking
roles, production defines, hashes, actual ar membership/extraction and body bytes.
Old server member9 and old packet_hub member17 stay excluded; no whole-archive,
old combined REL, HOST fixture define, stub or generated substitute implementation.
Rooted build03 closure policy is preserved:18 actual API roots, actual retained
project relocations checked. Raw three Digest UND names are explicitly kept in
nm logs/report; readelf symbol rows prove LOCAL/NOTYPE and relocation logs prove
zero retained references. The recipe adds this explicit proof, not a weaker check.
All rooted APIs and actual cleanup/publish/span packet APIs have nonempty real
GLOBAL FUNC definitions; libc/compiler/linker references appropriately remain.

| REL (NOT executable) | Bytes | SHA-256 |
| --- | ---: | --- |
| HOST |259064| `f6863373166954b320af723c21f400c7e4cb49c790c42a62973d9546476484a0` |
| ARM |172324| `1ca1a251e327180bf8e44340f18d07c2e6296d085452852c8a45167a0f2ff30d` |

[AUTHOR_FINAL_VERIFICATION.json](AUTHOR_FINAL_VERIFICATION.json) verifies203 input
pins,251 new dependency paths,363 reused dependencies,98 artifacts,26 command logs,
exact new dependency memberships, frozen217 membership/bytes and actual ELF metadata:
HOST ELF64/x86-64, ARM ELF32/ARM, type REL, entry0/no program headers. Counts overlap;
not a full compiler-execution/transitive-stack closure or final ELF/native ABI proof.
This is author evidence, not independent review/adoption.

## Explicit remaining blockers and stop boundary

No C/program/test/harness/fixture/negative/adversarial/mutation/fuzz case, listener,
socket, auth/backend/verifier, callback, provisioner/RNG/credential, encoder/DMA/
native/hardware function was called, even HOST. Source text/hash/diff/ELF metadata
inspection is not a runtime observation. Further deferred coverage is NOT passed.
V1 build01/02 remain failed and v1 build03 compile evidence remains its own history.

Native use remains **BLOCKED**: original native_adapter still prefaults a flat
1048576B hub through old kind1/paid[1]; owned-runtime readiness still requires it.
No passing a chunk to that old void* API or automatic memory-candidate adoption.
Real native chunk accounting/adapter/ready/return-history connection, producer1MiB
output/raw backing,128KiB allocation cap,2MiB process/40MiB/CMA/TLS/headroom/combined
load, native ABI/image fit, external whole-process retention/recovery, provisioning
and main remain open. Trusted stable/disjoint serialized addresses, no aliases/
reentry/signals/fork/FD mutation, fresh hub AFTER last fork and actual admitted
backing/lifetime remain prerequisites, not proved by metadata or callbacks.
One-VCL serialized sender remains not complete AU/asynchronous product support.

Parent-provided camera OFF/finalized23h/no live lease and RAM75 recovery/C1 block
remain inherited context only. No physical state was observed or operation queued.
**Stop here for independent ordinary v2 review, then parent disposition.** No use,
automatic correction/review delegation, execution, adoption or next work selected.
