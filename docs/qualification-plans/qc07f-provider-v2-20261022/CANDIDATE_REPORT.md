# Persistent encoder FD custody — HOST/ARM compile-only v2 candidate

2026-10-22; RP-07/09 dependency source. No complete provider/native/memory adoption.

## Delivered footprint and exact unchanged inputs

`apps/rtsp-auth/qc07f-provider-candidate-v2/` contains6 fresh files,550 lines/
28116 bytes: byte-identical frame C/header imports142 lines; new encoder C/header
150 lines; compile-only builder214 lines; README44 lines. No existing helper,
runtime/query/driver/header, old report/builder/artifact, central plan, Git/index,
P1, boot/recorder or safeguard/provider/model file was changed.

Actual imports, not rewritten counterparts:

| Byte-identical v1 source | SHA256 |
|---|---|
| `frame_fd_custody.c` | `d6c4a65939c2f6bde1498eb6622c9487a92757c0d7170bcc90d3137a7cfcf78d` |
| `frame_fd_custody.h` | `6dce51456b7588fa46c8605d73d222a57eb320abc1583399a78f4a6ae45365ae` |

Original provider v1 source pin:
`bcc141c9e57b791d2670b7a45c55c54a33699d2ceb12f4e91d4e0b6fccf49d82`.
Exact original query SOURCE_SEAL01 source pin:
`ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520`.
New helper uses the existing frozen `encoder-open-query.h` (SHA256
`6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37`)
and its actual frozen stage flags, not a guessed syscall/schema or duplicate query.
Frame receipt header stays byte-exact,56 bytes; encoder query stays88 bytes.

The builder independently verifies source aggregates and all sealed evidence pins
before use, verifies the exact frame import, checks original query-run candidate
header/source pins, and retains original snapshot pins before/after. **668 original
source/evidence snapshot files**, including old compiler-prerequisite failure and
new23.1.3 source object work, remained unchanged. Original query/runtime/compiler
seals and license/provenance copies are retained under `runs/build02/inputs/`.
New code is GPL-2.0-only; all imported SPDX/GCC/musl notices remain their own terms.
No global license grant or private credential/payload input is introduced.

## Real implemented API — not an operational provider flag

New `qc_encoder_fd_take` / `qc_encoder_fd_give_back` own **ONE actual preopened
persistent encoder open description** and its last valid full query snapshot.
They neither merge it with the temporary-job encoder role nor invent a watchdog
alias/lifetime design. The frame helper remains a separate unchanged import, not
upgraded into native acceptance by inclusion in a library.

### Mandatory trusted preconditions, not syscall-proven facts

Caller must establish exact boot/image/provider/source/query association and an
independently observed expected char-device/session identity from that description.
Only a future separately reviewed paired kernel can support the new query. Sole
serialized custody is required: no surviving dup/fork/SCM_RIGHTS aliases, concurrent
I/O/fd-table mutation/close, signal-handler reentry or overlapping API objects/slots.
All records stay stable and unmodified while held. The helper cannot prove these
facts, per-task authority, exclusive aliases, authorization or kernel/compiler pairing.
The expected numeric tuple is not a permission Boolean or a kernel pointer cast.

### Exact take sequence and fail-before-commit behavior

1. Require explicit `QC_ENCODER_FD_CUSTODY_EMPTY` (FD=-1, empty state/identity),
   nonzero actual expected device/session tuple, and input FD>=3. Zero/FD0 defaults
   are never empty owners. This rejects missing local prerequisites before ioctl.
2. GET-only fcntl/fstat must show O_RDWR, FD_CLOEXEC and the expected char-device.
   No device-number guess, F_SET, new open, duplicate, allocation or configuration.
3. Issue **only actual QC_HWENC_OPEN_QUERY**. Require syscall0, version1, bytes88,
   reserved0, nonzero session equal to the independently observed expected identity,
   valid relation/owner/frame relationship, bounded16-bit frame and actual Boolean
   fault/ledger fields. Failed/partial syscall replies are never read.
4. NONE requires ownerID0/frame0. THIS requires ownerID==this session. OTHER
   requires a different nonzero owner ID. These valid THIS/OTHER snapshots are
   distinguishable, but **neither admits a transfer**.
5. Transfer only for current NONE/owner0/frame0 with no observed quarantine,
   DMA uncertainty, guard poison, active ledger, known telemetry error or unknown
   telemetry-flag contradiction. QC_STAGE_F_SATURATED **alone is allowed and
   retained**, not cleared or treated as a fresh/unique ref/job generation.
6. Only then move the actual FD into owner and set bootstrap slot=-1. Any earlier
   refusal leaves bootstrap's FD and output owner unchanged. No cleanup close or
   destructor is called. SYSTEM on a failed ioctl preserves its errno, including
   natural **ENOTTY on the unsupported frozen76 kernel**. No LIFETIME/INFO/global
   session_sequence fallback or fabricated supported flag exists.

`O_RDWR` is checked as descriptor mode, **not granted as operational authority**.
The query observations are refusal conditions for custody transfer, not a claim
of DMA health/retirement, never-used, live-source readiness, backing admission,
normal-close proof or permission to run a job. NONE may follow earlier activity;
historical counters/session data are not erased or converted to leases.

### Give-back and retained uncertainty

Recipient must be explicit-1; invalid caller arguments leave ownership unchanged.
A held owner repeats the actual descriptor/schema/identity query and requires the
original node filesystem/inode plus the same transfer conditions. If a full valid
snapshot was obtained, it replaces last_observed **before** the transfer-refusal
decision, retaining actual THIS/OTHER/fault/history/generation/saturation data.
Malformed/version/identity/partial replies cannot overwrite the last valid snapshot.

An observation/transfer-condition failure latches sticky HOLD, keeping the real
FD, expected identity, original node tuple and last valid complete snapshot in the
owner. No later healthy query clears HOLD. Success only moves that **same FD** back
to recipient and empties the old record, never closes it. Caller must retain the
owner/process on HOLD. There is no close/reset/disarm/destructor/free/recovery API.
This record does not own or retire the camera's failed-next process/DMA/backing;
no metadata sink is substituted for that missing ownership contract.

Existing diagnostic reference generation/operation counters can saturate and codec
frame numbers wrap; all snapshot/history fields remain exact, without unique-ID
claims. A clear observation does not sanitize old history or establish release of
another alias/owner. No payload/key/device label, pointer or physical address export.

## Actual offline build result

```text
python3 -B apps/rtsp-auth/qc07f-provider-candidate-v2/build.py build02 --toolchain /home/gadorach/Desktop/AnykaOpenCam/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1
```

**PASS12:25:51UTC**,18 ordinary compile/archive/relocatable-link/compiler/ELF
commands;2 translation units/platform. HOST GCC16.2.1 and retained pinned ARMv5
EABI/musl GCC14.3.0.120 captured inputs and99 consumed source/header paths retained
with compiler/toolchain pins; all unchanged after build. Both frame files remain
byte-identical to v1. Exact argv, environment, source/UAPI/licenses, original pins,
compiler stack metadata, artifact hashes and diagnostics: `runs/build02/REPORT.json`.

| Actual output | Bytes | Role |
|---|---:|---|
| `libqc07f-provider-custody-v2-host.a` |7882| HOST helpers, not an app |
| `libqc07f-provider-custody-v2-arm.a` |6336| ARMv5 musl helpers, not adopted/run |
| `host-provider-custody-v2-NOT-EXECUTABLE.o` |6040| Rooted ELF REL, no main |
| `arm-provider-custody-v2-NOT-EXECUTABLE.o` |4720| Rooted ELF32 ARM EABI5 REL, no executable |

Only the four actual frame/encoder take/give-back functions are exported. Remaining
undefined dependencies are normal fcntl/fstat/ioctl, stack checks and ARM memcpy/
memset/GOT support—not open/close/dup/malloc, provider stubs or guessed kernel
symbols. No warning/error diagnostic occurred. ARM compiler static stack estimates
for new encoder observe/take/give-back are32/288/272B, not measured live peak.

Initial `build01` compiled/archived/relocatable-linked all helpers, but its evidence
collector then failed converting public `/usr/include/stdc-predef.h` into a
workspace-relative path. Its overall FAILED report/logs/artifacts remain preserved.
The only correction was path-key handling in the new builder for system headers;
no helper/source/API/warning policy changed. Fresh build02 has complete closure
receipts and is the final compile result. No runtime test was used to repair it.

No executable, listener, test/synthetic/adversarial campaign, native query, driver
compile, unchanged UAPI/unrelated suite rerun, full kernel/module/image build,
installation or target/UART/RPC/status/network/private IO occurred.

## Completion boundary and one next actual source increment

This delivers real frame/persistent-encoder **custody pieces**, not a complete
provider/camera app or native FD-adoption proof. Frozen76 still lacks OPEN_QUERY;
offline helper compile success does not install support or certify future pairing.
Complete sensor and temporary encoder roles, admitted backing/extents/placement,
actual failed-next retained process/FD/code/backing, trusted provisioning/current
source readiness and owned listener remain missing. Strict child pin/loopback and
128KiB/40MiB/CMA/native2MiB/HOST4MiB limits, original radio/reset/memory HOLD and
G2/G3/G8 are unchanged. Deferred runtime/adversarial coverage is not passed.

**One next implementation dependency:** define and source-integrate the real single
owned watchdog lifetime across capture and encode, preserving the existing5s
recovery semantics. Actual watchdog open starts hardware and is single-open;
two preopened aliases cannot reproduce the old independent V/close brackets.
Resolve who may ping/disarm/close and retain the actual description on uncertainty
before replacing any original internal open. This is a separately selected source
increment, not permission for a target operation, new guardian, unsafe close or
automatic paired full build/native query. Parent source disposition of this exact
candidate remains required before using even these custody pieces.
