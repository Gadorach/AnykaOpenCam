# Producer hub feature handback — SOURCE / NOT RUN

2026-10-24. B07 / RP-04/09; CAP-01/05/06/07 + STREAM-01/02/03.
**Delivered candidate; independent ordinary review and parent adoption pending.**
No automatic next queue or candidate invocation. Only the two assigned fresh
app/docs directories were written; no Git/index/central plan/AGENTS edits.

## Actual connection delivered

`apps/rtsp-auth/span-hub-producer-candidate/span_hub.c/.h` are a complete exclusive
version3 hub source replacement, not a wrapper around copied publish:

* Exclusive reserve (no pointers) → arm (actual existing eight canonical writable
  chunks) → validated zero-copy commit (logical NAL slice) → distinct consumer-
  bound const views → actual unchanged accepted span packetizer.
* Fixed eight malloc131072 requests at init remain the ONLY backing allocation;
  no second1MiB source/copy/memmove, caller-pointer owner or per-consumer NAL buffer.
* Canonical writer phase/token/next generation/exposure/error are audited in
  actual hub paths. One lifetime issuance, no wrap/reuse/reset, refusal before
  reserve effects. Exact unexposed RESERVED cancellation only. ARMED failures and
  STOP become sticky terminal HOLD; no abort/free/reset or native retirement claim.
* Writer custody excludes pending/acquired consumers and all subscription/resync/
  copy/reader/packet/dispose effects. STOP denies future effects immediately;
  reserved STOP may exact-cancel, armed STOP intentionally cannot drain/free.
* Commit checks all eight exact pointers/capacities/count and actual bounded owned
  prefix/header bytes, declared0/3/4 prefix, H264 VCL type1/5 and keyframe agreement.
  It installs offset/length and increments generation once; writer retires LAST.
  No local refusal after publication begins. While writer-held, old bytes/offset/
  metadata are historical, NOT readable publication authority.
* Logical spans account for prefix-shortened first physical chunk, every later
  physical boundary and NULL/0 tails; exact bounded sum. No concatenation. Each
  actual packet build freshly revalidates consumer owner/tag/token/generation and
  every canonical descriptor, then calls the accepted packetizer. Per-send lease
  validation and whole-packet partial-write commit remain caller obligations.
* V2 R1 consumer identity/scalar-release semantics remain: each acquisition issues
  its own monotonic token, canonical per-consumer active slot, exact release only.
  Separate typed writer/reservation APIs cannot accept a consumer token as such.
  Tokens are CPU identities, not permissions or source/DMA/ACK receipts.

Contract: `CONTRACT.md` (written before code). Detailed author-only source audit:
`AUTHOR_AUDIT.md`. This is source reasoning, NOT independent review/runtime proof.

## Essential external boundary and missing integration

Future real caller MUST establish actual supported kernel syscall0 plus complete
valid metadata, actual successful producer release/exact frame END, native/clock/
auth guards and quiescent writable aliases BEFORE commit. This hub accepts no
success Boolean/callback/fake receipt and cannot itself prove these external facts.
Any uncertain ioctl/write/metadata/source/clock/auth outcome retains armed CPU
backing AND actual external owners; wrong identity does not mutate another writer.
Explicit exact hold needs only intact identity, not possibly corrupted descriptors.
No repair/retry/ENOTTY fallback or complete-looking partial-copy acceptance.
HOLD does not permit further writes; caller quiesces aliases and retains storage.
Real armed retirement/recovery connection is deliberately missing, not a fake ACK.

The accepted encoder UAPI/source at `qc07f-encoder-output-spans` (930afde) supplies
the eight fixed131072 bounds and128B0xc0805167 context only. It remains uninvoked;
no kernel/client/provider implementation is copied or adopted here.

Current composed sender/runtime still use native malloc1048576/contiguous ioctl
and copied publish BEFORE source release/END. Existing paid memory, prefault,
pristine, ready/return-history, front-server and normal CPU borrow-end checks all
know the OLD layout. They MUST NOT cast/dereference/reuse this replacement or old
RELs automatically. Recompile only after separate actual derivative integration;
never link both hubs/old combined RELs. No memory-ready Boolean was added.

Overall128KiB cap/raw/process2MiB, kernel2MiB buffers,40MiB/CMA/TLS combined load,
external recovery/backing/provisioning/main/native ABI/entropy and strict C1 remain
open. Camera last23h OFF/finalized only; no fresh physical observation. RAM75
independent recovery/C1 blocked. V1 hub remains NOT ADOPTED and all original failed
observations/HOLDs remain unchanged. No new clock/receipt/diagnostic framework.

## Exact source and deltas

* New C SHA256 `72e06177ae86c8659972ba8a59c81208302d44e0cb78f29b6114f8a4d5631f93`.
* New header SHA256 `460320064f85d1f1f8ada1a9afd1e282c287583fdff34787ba63c7354adeed8e`.
* Accepted baseline C/header hashes remain as in CONTRACT and run SOURCE_DELTA.

`runs/build01/SOURCE_DELTA.json` pins new/old source and all four literal forward/
inverse C/header diffs. Both directions were reconstructed exactly in text without
patching originals. Function text `consumer_valid`, `lose_backlog`, `input_valid`,
`copy_input`, `camera_span_hub_packet_v1` is byte-identical. Other existing paths
change only to integrate writer exclusion/history, logical slicing or stronger
explicit no-writer drain/dispose admission, plus version initialization/comments.
Consumer structs, ordinary declarations and legacy prefix-free input policy keep
their meanings; old binary/layout reuse is NOT claimed. GPL-2.0-only provenance
is scoped in app LICENSE_NOTICES; no upstream/license/acquisition changes.

## Compile-only evidence (no rerun)

Command: `/usr/bin/python3 -B apps/rtsp-auth/span-hub-producer-candidate/compile01.py`.
Saved `runs/build01/REPORT.json`: **COMPILE_ROOTED_REL_METADATA_PASS_NOT_RUN**,
2026-10-24T11:07:46.945017+00:00 (build timestamp, not C1 admission).

* Four actual production C compiles: new hub + unchanged accepted span packetizer,
  each HOST O2 and retained musl ARMv5te ARM soft-float Os. Wall/Wextra/Werror/
  Wpedantic, C11, no fixtures/stubs/main or source substitutions.
* Two real direct ld `-r --gc-sections` links, all17 public hub functions rooted
  explicitly. Both RELs contain all17 definitions plus actual accepted packetizer;
  no unresolved camera symbols. ELF metadata/relocations/symbols inspected only.
  ARM ELF32 EABI5 REL, entry0/no program headers. Neither object was executed.
* 22 successful fixed tool commands; zero failures/retries. Exact argv/cwd/env,
  start/end/return codes and22 complete logs pinned in REPORT/command records.
* 42 distinct consumed dependency paths,12 retained-matching tool records; scoped
  compiler/tool/input pins and before/after checks retained. Not a complete
  compiler-execution closure, final ABI/loadability or full libc/allocator link.
* 519 frozen files across12 relevant source/evidence directories preserve exact
  bytes AND membership; see FROZEN_INPUTS. Includes v1 negative history, accepted
  v2, packetizer, new encoder and current composed runtime/server. No edits there.
* ARM hub individual `.su` frames max168B (acquire), arm136B; these are compiler
  per-function observations only, not transitive stack/native/memory acceptance.
  malloc/free/memcpy/memset/compiler/GOT symbols may remain normally unresolved.

No candidate/library/helper/hub/malloc/packetizer function, normal/negative case,
test/harness, native ioctl or hardware call was performed, including HOST. The
Python recipe only orchestrated compiler/link/ELF metadata and filesystem evidence.
Further negative/adversarial/mutation/fuzz coverage is deferred, NOT passed.

Only next step: parent-arranged independent ordinary SOURCE review of this exact
delivery. No source/hardware follow-on selected; author stops at this handback.
