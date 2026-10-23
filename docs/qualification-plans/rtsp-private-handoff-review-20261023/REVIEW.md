# Independent ordinary SOURCE/API/custody review

2026-10-23 · SEC-01/03/05/11, STREAM-07

## Disposition

**Narrow conditional SOURCE/API/custody sufficiency. No correction-required
defect found within the explicit trusted preconditions.** This is a real
constructor → exclusive file → retired writer → readonly FD → explicit config-slot
move implementation, not an implemented separate-process provisioning/IPC pipeline.
No build, candidate import/call, test, credential use or hardware operation was
performed by this reviewer. No further execution is queued by this review.

## Inputs and scope

Read the full delivered HANDOFF/CONTRACT, all new C/header/harness/recipe and README;
the frozen startup constructor/header/contract; current owned-runtime
`server/candidate_auth.{c,h}`; actual verifier ABI/engine, adapter launch/material
path, child consumption and isolation material checks; relevant security plan and
current ledger. Review is ordinary source/API/custody inspection, not additional
adversarial, negative, mutation, fuzz or runtime qualification.

Exact SHA-256 source pins (paths relative to `apps/rtsp-auth/`):

| Path | SHA-256 |
| --- | --- |
| `rtsp-private-handoff/private_handoff.c` | `cd3e08600841502f1373c30ea80b43c736aa07e705bed7ec83f20f19d5f5903c` |
| `rtsp-private-handoff/private_handoff.h` | `4ad3136dfcf4e8fff5b3a26ab794279a96ee908466bfa547819dd87546fefa44` |
| `rtsp-private-handoff/positive_publicdummy.c` | `1624450b6fc374179fc4e81de491a0f1093f7b65f0eb58d9a02005be8a5a94f6` |
| `rtsp-private-handoff/compile_recipe.sh` | `5fdc9aa50d54806a8b7f6b3706b62811515b68b91529e783e44b6e48ce816d95` |
| `rtsp-startup-material/startup_material.c` | `5eb23e473750c0b3ae0daecd472c2091ec1a6e8287001c65b9d1634fdd3aceae` |
| `rtsp-startup-material/startup_material.h` | `76fb100e06c6b7508b6011400bf35734d273615fd26a3aa8cc57cd2150630206` |
| `qc07f-owned-runtime/server/candidate_auth.c` | `cb27b35e2d8730b8d19f231d339e06cbe4c97fbe0aca7826fe3469d8479825a7` |
| `qc07f-owned-runtime/server/candidate_auth.h` | `94b6ed560677b7db4119eded9726ab01085c2e0eb392bdad29e961ebcd2ed643` |
| `isolated-verifier/verifier.h` | `3f4a97a12ed9acafe678e720b473c798c75b170648e6a6de076a1017ff0ad900` |
| `isolated-verifier/engine.c` | `cf298b8096c66859806c6e0c0b449592ac4081f05ad814f21fee35e97262c396` |
| `isolated-verifier/adapter.c` | `4cdf9a301c5603dd3542f07bd96cffd01f79497c39da15a0cc95b84223a4c8a1` |
| `isolated-verifier/child.c` | `b317bac6512a3795fa7b8c82ae339366750dbf39960642d97e53d69d1d3e4ad7` |
| `isolated-verifier/isolation.c` | `6759d15640e17048d331354bce62376a23588689daeea468e5e5b1a4ec54b135` |

## Source findings

Locations below are in `rtsp-private-handoff/private_handoff.c` unless qualified.

- **Write and writer retirement sufficient:** lines128–153 use exclusive
  `O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC`, mode0600, actual regular-inode checks and
  at most64 write calls bounded by the remaining1536 bytes; EINTR consumes budget.
  Exact size/identity/name checks surround mandatory file fsync. All ordinary
  post-create failures reach the one writer close. Lines85–95 clear the writer
  slot before that attempt; any close error, including EINTR, leaves sticky
  WRITER_CLOSE_UNKNOWN and forbids readonly acquisition. No close retry, unlink,
  overwrite or destructor hides partial/uncertain custody. `created`, identity,
  bytes-written, synced and known-close fields distinguish partial outcomes.
  This bounds calls/bytes, not blocking syscall time.
- **Readonly admission sufficient:** lines48–83 and154–161 store the returned FD
  before any fallible validation. Directory identity/owner/exact0700/CLOEXEC,
  fixed named inode, saved writer dev/inode, opened regular inode, owner/exact0600,
  nlink1/exact1536, readonly access/CLOEXEC and offset0 are checked. Failed
  validation retains the FD in READONLY_HOLD; failed readonly open retains the
  closed linked file. O_NONBLOCK is not claimed as namespace-race protection.
- **Explicit final move sufficient under caller ordering:** lines169–182 require
  the same config address, its still-empty material slot, correct native/HOST
  class and actual fresh prestarted-owner fields; recheck custody/metadata and
  exclude image-FD equality. An attempted unsuccessful move retains the FD in
  HOLD. Success clears the custody slot before assigning the config slot, without
  dup/close/backend call. Subsequent startup failure leaves bootstrap ownership.
  The header lines14–20,61–66 and CONTRACT lines22–29,52–59 correctly make stable
  disjoint storage, no aliases/reentry/FD reuse and never-started global ordering
  trusted requirements, not claims proved by these field checks.
- **Errors and secret lifetime sufficient:** lines103–125,162–166 clear nonnull
  scratch before validation and on every return; success is assigned only after
  readonly admission. NULL scratch returns invalid without dereferencing it.
  Frozen `startup_material.c:30–31,74–99` uses crypto-library zeroization and real
  Digest construction, never copies the password into the record, and binds the
  actual routing/generation/policy fields. Borrowed password storage must stay
  valid during construction and is caller-wiped afterward; scratch/input overlap
  is explicitly outside the API contract. No new secret logging/readback/hash.
- **Native hardening sufficient for its stated subset:** lines13–34 perform real
  real/effective/saved root UID/GID and empty-group checks; set zero soft/hard core
  limits, nondumpability, NNP and current/future mlock; then verify limits/prctl
  state. Successful mlock syscall is required, not a supplied permit. Creation
  repeats this at114–116. Pre-ingress invocation and terminal disposition after
  partial irreversible failure remain explicit caller obligations. No claim of
  capability removal, ingress authorization or process isolation follows.
- **Actual ABI and class separation sufficient:** native construction retains
  account1/RTSP/720p and explicit SHA256 or legacy-MD5. This matches current
  `candidate_auth.c:53–94`, `verifier.h:21–36`, `isolation.c:100–112` and
  `engine.c:44–71`; child consumption is after hardening at `child.c:23–38`.
  Header lines34–48 and C lines37–45,113–124 make PUBLICDUMMY different compile-time
  symbols with HOST/nonroot admission and PUBLICDUMMY numeric account. The local
  synthetic constructor shape is not a native runtime fallback. Recipe lines51–63
  link that separate handoff/harness with original constructor/crypto objects,
  never native handoff object0 or backend/child/media launch code. Existing symbol
  log corroborates this. No UI/HTTPS credential or TLS algorithm change appears.

## Existing evidence checked, not reproduced

Read-only SHA-256/length validation matched all32 SOURCE_PINS entries,15 delivery
entries and16 frozen constructor entries; all1363 dependency records,75 artifacts
and37 command logs also matched. These references cover378 distinct resolved
pinned files, with overlap. Author's243 unique dependency *path spellings* resolve
to241 files: two `../` header aliases account for the difference, not changed
inputs or a source defect. No candidate or recipe was imported/executed for this
validation, and no private material was read/hashed.

Recorded argv exactly match COMMANDS.txt. The37 recorded successful commands
contain24 C compilations (11 HOST native-shaped,11 ARM native-shaped,2 separately
defined PUBLICDUMMY), two REL combinations, one HOST harness link and ten tool
inspections; all24 compile diagnostic logs are empty. Existing version/ELF/undefined
logs support GCC16.2.1 HOST, retained GCC14.3.0 ARMv5TE/soft-float flags, ARM EABI5
REL and libc/compiler-support unresolved symbols. No ARM executable/native ABI
or release qualification follows. ARM handoff stack entries216B create/24B move
are per-function compiler records, not total-stack/memory admission.

`positive01/OBSERVATION.json` records exactly ONE fixed-public-fixture call,
04:21:27.215727–04:21:27.217580 UTC, exit0; stdout reports1536-byte write, known
writer close, readonly metadata, explicit move, scratch zero and cleanup PASS;
stderr is empty. Full harness inspection at lines45–80 supports these normal-path
assertions before success-only cleanup, with no payload readback or launch.
Failure handling at83–88 does not retry or clean up. This is cited author HOST
evidence only, not a reviewer run, real credential use, native hardening test or
failure-path coverage. Deferred tests remain deferred, not passed.

Evidence seals, relative to `docs/qualification-plans/rtsp-private-handoff-20261023/`:

| Path | SHA-256 |
| --- | --- |
| `CONTRACT.md` | `6abc0f2ac02b5d7816d7f0c7d2e1cffb14e4ddd138a21cc40ab173863f41c5e2` |
| `HANDOFF.md` | `258e4735f5974ef1579e58613a14ba8f826018a344c89d5180ba1e5d1ceba6d4` |
| `SOURCE_PINS.json` | `cb94c39623a6b466c40882db0bcf286d51590ee8a8fec750fcc914829bedd926` |
| `DELIVERY_PINS.json` | `c7fcefa2f6bd7a96a58fe7ced74c5fd8e122bfa2c5d7f7cc7d392a94e296124c` |
| `runs/build01/REPORT.json` | `5c5f4ce8b8a8cb82d3fee8a5faf8b93f63ae0651f2917c35117bddb02e3be1b4` |
| `runs/positive01/OBSERVATION.json` | `1ca87cee1f0ea17d4698d07f58eda94b6676ccac7eb521436cdca0d865d1a771` |

## Conditions and exclusions retained

Trusted namespace/ancestors, intended directory identity, no alternate writer,
alias/mmap/fork/thread/signal/reentry and stable config/routes must hold throughout.
Writer exclusion and linked-inode stability must continue beyond this move through
verifier consumption/retirement, as HANDOFF lines140–146 already require. Metadata
rechecks cannot establish these whole-interval facts.

A C-slot move alone does not implement IPC, transfer an FD between processes or
prove a secret-free bootstrap/media address space. Actual authenticated secret
ingress, separate process/launch handoff, capability/environment/inherited-FD
hygiene, caller password wiping, durable generation authority, storage durability,
rotation/reset and linked-file/startup-failure retention/cleanup remain missing.
File fsync alone is not crash-safe storage. Native hardening/adoption, actual
bootstrap/main, external recovery/retention/backing,128KiB-compliant media buffers,
combined-memory/load and authenticated PLAY/decode/finite STOP remain unqualified.
This review does not close complete SEC tasks, G3/G8 or product gates.

Only this fresh review directory is written. Frozen sources/evidence/private
failures, central plans, AGENTS, Git/index and hardware HOLDs are unchanged. No
current-state/proc/device/lock/serial/RPC/network/process-controller access occurred;
historical held U-Boot is not asserted as current state. Parent audit/curation is
next; no automatically queued build or native action.
