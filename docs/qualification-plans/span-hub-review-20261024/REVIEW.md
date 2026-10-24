# Span hub — independent ordinary SOURCE/API review

2026-10-24; RP-09/B07, STREAM-01/02/03, CAP-01/05/07.

**Disposition: one minimum ordinary correction required (R1). No overall
SOURCE/API/ownership sufficiency yet.** The actual chunk allocation/copy and
production packetizer connection are present, but lease identity is not bound
to the acquiring consumer. This is not runtime/native/cap/product GO. Parent
must make a separate disposition before any use; this review selects no next
implementation, execution or native queue.

## Scope and exact inputs

Read all six files in `apps/rtsp-auth/span-hub-candidate/`, author CONTRACT,
HANDOFF, AUTHOR_AUDIT, PRECALL_AUDIT, BUILD_INSPECTION, delivery/build pins,
command records, relevant logs/maps/dependencies/stack metadata and sole-HOST
records. Read actual framework packet_hub C/header, backend C/header and RTP
header, accepted rtp-span C/header and parent disposition, and relevant CAP,
STREAM and memory plans. Targeted read-only runtime source search confirms old
allocation/hub/packetizer sites remain; it is not a whole-runtime review.

SHA-256 under `apps/rtsp-auth/`:

| File | SHA-256 |
| --- | --- |
| span-hub-candidate/span_hub.c | `40c6d84c1e78c29a78384bee4dc1f435a53032f31655e122949921275a5c2289` |
| span-hub-candidate/span_hub.h | `8c45f279a09dce01b53d928e2fa024663093f6955c469d0b71bb18ba72947c47` |
| span-hub-candidate/host_normal.c | `7b5ac823f1cad92181f7d870a92d306f855c19cb339052e35f39a5b8390e685f` |
| span-hub-candidate/compile01.py | `6a242e4b21c6610ac93f45ac6da5ebc319d55d7317e20a3533d37e771de3e9dc` |
| span-hub-candidate/host_once.py | `210f9598978b6b0b7abba43cb8e6a88309770e63d7a958326b3d92836e0c2671` |
| span-hub-candidate/LICENSE_NOTICES.md | `380f490c17abc6cde0509bbd31a960e2dd150d5473fabb199d0e5bc33c149180` |
| rtp-span-candidate/rtp_h264_spans.c | `a608fe8ee8a292b1172ce62c62f19c2a61f9969f64d71385f1a742fdd326560e` |
| rtp-span-candidate/rtp_h264_spans.h | `41126a8b9827894d6ecb71788cd1cc71bf3b5b036e44fddf998f4c5740f05f91` |

Evidence anchors under `docs/qualification-plans/span-hub-20261024/`:

- `DELIVERY_PINS.json`: `1edcfbba1de9acfc514b53d2a06580d864c9e5692093c718b6ecf67343795339`.
- `runs/build01/REPORT.json`: `1c6ab0a62b64f21a36ae69cf1b8e67590335811ca094142852286d800b068ce0`.
- `runs/host01/OBSERVATION.json`: `ba7c6624edf3a3a6cf92d5188dad7d8a9bc688b3be4bb65f5de7a4d1fbe046aa`.

Fresh read/hash verification matched **222 overlapping pin records**: delivery18,
selected inputs19, consumed dependencies53, tools/link inputs30, artifacts90,
pre-call12; additionally all33 command-log pin records matched. No mismatches.
These counts are not distinct files or complete compiler closure. Read-only
`readelf -h -A` also matched the retained HOST/ARM REL and HOST executable roles.
No candidate/script import, build, link, test or ELF execution occurred here;
only public reads/hashes/log/ELF metadata inspection. Only this review directory
was written; no Git/index, source, central-plan, private/current-state or hardware
operation, subworker or adversarial matrix.

## R1 — shared publication identity can release another consumer's lease

**Blocking ordinary ownership/API defect.** `span_hub.h:41–48` has no acquiring
consumer identity. `span_hub.c:151–161` gives every acquired view the same owner,
token=generation, metadata and spans. `lease_valid` (`176–192`) checks the
caller-supplied consumer's bit but never binds that consumer to the supplied
view. `release` (`165–173`) similarly accepts the shared generation as token
for whichever acquired consumer the caller names.

Static consequence, not an executed additional case: after consumers0 and1
acquire one publication, their views have identical checked fields. Following
consumer0's proper release, its saved view still passes
`lease_valid(h, 1, &view0)` while consumer1 is held. Likewise, supplying
consumer1 with view0's token/generation to `release` returns OK and clears
consumer1's acquired/pending bits without consumer1 releasing its own lease.
With no remaining acquired bits, publish can overwrite that backing, or
STOP/drained/dispose can free it while consumer1 still retains its view.
The production packet wrapper inherits the wrong-consumer validation.

This requires no hostile pointer, owner mutation, concurrency or wrap: it is an
ordinary wrong-cursor/consumer association among valid stable trusted objects.
Separate acquired bits alone do not establish the requested view/release
identity. The existing framework shares a similar convention, but that does
not meet this candidate's explicit consumer-bound review requirement. The normal
HOST case uses the correct consumer at each call and does not establish this
missing binding.

**Minimum correction:** bind the issued view/release identity to the actual
acquiring consumer, so another consumer's unchanged view/token cannot validate
or release it. Distinct per-consumer lease tokens checked against that consumer's
canonical active lease, or an equivalent consumer-bound release handle, suffice;
merely adding a consumer field to view validation leaves scalar release broken.
Preserve nonzero monotonic identities and checked exhaustion/no reuse, with
refusals before relevant state/output effects. Keep exact once-only release,
STOP-held release admission, chunk retention and the accepted packetizer intact.
No security framework, hostile-pointer defense or expanded test matrix is
requested. Claims of exact owner/consumer/token/generation identity in the
contract/handback must reflect the corrected source, not substitute for it.

## Other source findings — narrow, under the stated caller contract

- **Backing/lifetime (`34–52`, `215–228`):** eight separate direct
  `malloc(131072)` results immediately enter the eight canonical slots. No hidden
  1MiB allocation, realloc, descriptor/DMA/FD adoption or implicit destructor.
  Self/INITIALIZING latch precedes allocation; partial INIT_FAILED retains every
  success and cannot publish. Explicit dispose admits only known unpublished
  INIT_FAILED or STOPPED/drained, latches DISPOSING and clears each canonical slot
  before its one void free. No reinit/history reset. The bit-based drain/free
  gate is otherwise correct, but R1 prevents an overall held-reference guarantee.
- **Publication/bounds (`82–140`):** codec, boolean keyframe, 1..16 nonempty
  valid borrowed spans, each <=128KiB, checked exact sum <=1MiB and first-byte
  forbidden-bit/type1-or5/keyframe agreement precede mutation. Single-VCL input
  is a caller contract, not full bitstream/AU/decoder validation. Generation
  UINT64_MAX refuses before copy or drop changes. Each copy piece consumes a
  positive minimum of source/destination remainder; destination stays below
  total before every access, chunk indices0..7, no unchecked sum or terminal
  dereference. Only actual NAL bytes are copied once, with no retained input
  pointer. No eligible consumer yields NOT_READY, never fake copy success.
- **Backlog/STOP:** an acquired bit prevents copying. BUSY/resync preserve held
  metadata and acquired pending bits while dropping only unacquired backlog and
  marking IDR need; new subscribers require IDR. Held unsubscribe refuses;
  resync safely preserves its held lease. Acquire trims the last extent and
  zeroes unused descriptors. Apart from R1, validation covers phase, subscription,
  acquired bit, owner/token/generation, all scalar metadata and all eight
  pointer/size pairs, including unused ones. STOP denies publish/acquire/packet
  effects, retains old releases, and drained means STOPPED/no acquired bits.
- **Production connection (`195–203`):** actual fresh lease validation precedes
  the real unchanged `camera_rtp_h264_spans_packet_v1`, not a stub/old contiguous
  packetizer. No send, sequence/offset commit, source END/ACK, peer/network or
  external retirement authority is introduced. This connection is real but
  its consumer identity guard needs R1.
- **Caller boundary:** one true-fresh, one-use stable private trusted owner;
  valid stable disjoint input spans/metadata, output views, session, packet,
  scalar outputs and hub/chunks; serialization, no writer aliases, moves,
  reentry/signals/concurrency/fork or post-disposal use. Metadata/self checks
  are not hostile-pointer security or native ownership proof.

## Retained build/HOST evidence and integration limits

Recorded five HOST/ARM warning-as-error C compiles, two actual hub+accepted-span
REL links and all33 tool commands passed; five compile logs are empty. Symbols,
dependency paths and maps show the hub's real builder reference resolved to the
accepted object, production without main/old packetizer, and HOST harness linked
to the actual REL. ARM is ELF32 REL/EABI5/v5TE only, not an executable/native ABI
acceptance. Local stack reports are not transitive/native stack qualification.

The sole retained normal HOST observation exited0/empty stderr: one174080B
numeric synthetic IDR, three separate source objects, actual eight allocations,
one publish followed by volatile source wipe, four views sharing the same two
trimmed chunks, 126 packets/client (504 total), complete incremental header/body
formula checks before cursor/sequence commit, four releases, STOP/drain/dispose.
No whole-NAL reconstruction/per-client payload copy or payload logging/hashing.
This is not decodable media or a new reviewer run. Partial init/failure, backlog,
stalls, refusals, wrap and retirement matrices remain unexecuted; none is passed
by this review.

Allocation instrumentation is ordinary production counters adjacent to the
actual malloc path, not malloc/free replacements or a stubbed production build;
symbols retain real malloc/free references. Eight calls, 1048576 successful
requested bytes and131072 maximum request truthfully describe the normal case.
`disposed=1` reports successful explicit disposal, not independent free-wrapper
measurement. These counters/sizeof do not measure allocator overhead, residency,
prefault/locking, process2MiB or native combined40MiB/CMA/TLS/headroom budgets.

Frozen APIs and actual runtime remain unchanged: native output malloc/prefault
1MiB, contiguous driver output-copy contract, raw/DMA ownership, old hub/cursors,
memory ledger and producer release/END/drain/external retention/recovery still
need real integration. Generic multi-VCL/parameter sets/RTCP, recorder/audio/
snapshot policy, main/provider wiring and native combined-load acceptance remain
open. CPU chunk disposal grants none of those authorities. Parent's finalized
23h/controller-OFF context and blocked RAM75/C1 recovery remain unchanged, not
fresh observations. Preserve originals and this candidate/evidence; parent
disposition is required and no automatic continuation is authorized here.
