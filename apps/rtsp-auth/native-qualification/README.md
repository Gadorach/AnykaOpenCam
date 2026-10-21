# First native RTSP verifier entry SOURCE pack — OFFLINE / HOST PUBLICDUMMY

SEC-05/06/11, parent review required. **No native execution, private material/
password generation, native positive RTSP verification or native media/per-effect
IPC acceptance by this lane.** Media501/RTSPNOAUTH/G3/G8 unchanged, T34/Fulhan
deferred. Existing verifierV2/serverV3/framework/UI/crypto/vendors/launcher/
private-client/kernel/pins/runner/plans are read-only. No hardware/watchdog/UART/
radio/SD/proc/fuser/status/real private inputs/ROM/photo/audio/footage/networkcamera/
install/Git/publication. No listener/producer/media/FIFO/daemon/guardian added.

## Exact reviewed inputs / native invocation (PARENT ONLY)

`PROVENANCE.json`, `native_image_pin.h`, `host_image_pin.h` come from the exact
successful reviewed `local/validation/rtsp-verifier/v2verifier01/REPORT.json`.
They pin SOURCE/IMAGE bytes, not credential/material/password/nonce payloads.
Helper links readonly V2 `rv_start`/owned adapter and codec; it does not overwrite
the child or infer authority from HOST crypto flags. Existing V2 source pins must
still match before build. Original upstream/runtime license notices retained.

Parent pre-stages:

- `<private-scope>`: absolute safe-walk root-owned0700 directory, no symlink /..
  or writable untrusted ancestors (root sticky /tmp ancestor narrowly allowed).
- `<public-code-sibling>`: root-owned0555 directory outside the private0700 scope.
- `<public-code-sibling>/verifier`: exact pinned reviewed native V2 ARM image,
  root-owned0555 readonly regular nlink1; no runtime chmod/chown/restaging/hash
  override or root-only0500 exec workaround. V2 additionally validates ARM ELF32
  EABI5 softfloat/static/no interpreter/executable-stack and immutable IMAGE pin.
- `<private-scope>/rtsp-material.bin`: fixed **1536-byte** zero-padding SOURCE_ABI
  record, root-owned0600 readonly regular nlink1/offset0, quiesced other writers.
  Readonly descriptor metadata does not prove global immutability. Root helper
  explicitly reads ONLY the48-byte public version/domain/algorithm/epoch/boot/gen/account
  prefix; reviewed `rv_start` also reads its348-byte nonsecret binding prefix,
  stopping BEFORE username/realm/HA1. No payload hash. Actual child reads material
  only after native root-origin/groups0/2MiB caps0/core0/nodump/NNP/lock/seccomp/
  FD/PDEATH READY15 and supplies actual provisioning ACK.

Actual ROOT-only native CLI, compiled ARM helper to be invoked ONLY by parent:

```text
ARM-ROOT-native-entry-SOURCE <private-scope> <public-code-sibling>
```

No secret args/env/arbitrary PID/path read selector. Filenames are fixed trusted
bootstrap names, not public RPC. Already-root UID/GID0 helper empties OWN startup
supplementary groups, reduces OWN caps to SETUID/SETGID/KILL before spawn (never
IPC_LOCK/SYS_RESOURCE or privilege acquisition), core0/nodump; V2 child drops
UID/GID1000/groups0 before exec/material, with immutable owned pidfd before barrier
release. It rejects HOST4MiB child/image and PUBLICDUMMY account partition before
qualification. Source flag `PRIVATE_ACCOUNT=1` is a private qualification class
discriminator, not authentication/default account provisioning.

Parent invokes this one finite FIRST-entry operation only after handoff/actual
source readiness with its existing independent qualified5s/≤45s recovery policy.
The helper **never opens, arms, renews or resets watchdog** (single-open conflict
avoided), UART or kernel source. Normal operation makes bounded rv_start/control/
retirement calls (~seconds; each existing control ≤1s, startup ≤2s, stop bounded),
not a new timeout/sourceCPU-budget phase. No positive VERIFY/KDF/password reader.

## Actual finite protocol / ownership receipts

One child startup → exact native flags15/profile1/provisioning ACK → OPEN one peer
→ CHALLENGE syscall/engine validation (nonce response immediately wiped, not
inspected/logged/hashed/forwarded) → bad opaque grant query denial → fixed known
account/profile owner REVOKE/current-generation ACK → normal owned SIGTERM →
actual BYE/self-wipe + wait exit0 + no owned child lineage. Only fixed scalar
receipts print. `functional_RTSP_verify_native_NOTtested=1` is always explicit;
HOST grant/crypto proof is NOT inherited as native authentication.

Actual V2 waitpid calls are observed by a tiny linker wrapper delegating to the
real kernel; no fabricated exit/READY/root FD. It reports actual owned exit0,
not merely a signal request. On error, actual rv_stop/wait outcome controls use.
If retirement is UNCERTAIN, the allocated client and owner process are retained,
fixed HOLD receipt printed and no free/respawn/automatic retry performed. This
exception deliberately awaits parent-owned external bounded recovery; it is not
a successfully finite STOP/quiescence claim, watchdog/guardian or background
service. Parent must never deploy without the external ≤45s protection boundary.

No root RTSP whole-producer3.8MiB assumption: this helper/verifier are separate
minimal entry processes, native child limit remains fixed2MiB. No HTTPS600k/2MiB
change. No source/CPUlease/media operation is possible via this helper interface.

## PC-only PRIVATE material TOOL SOURCE (DO NOT EXECUTE BY AGENT)

Private tool is compiled for later explicit parent PC invocation only:

```text
PRIVATE-PC-material-tool-SOURCE --PRIVATE-PC-ONLY <PC-private-0700-dir> SHA-256|MD5 <public-device-ID> <public-account-ID> <sensor-ID> <profile-ID> <child-epoch> <boot-generation> <credential-generation> <public-live-RTSP-URI>
```

Public IDs/algorithms/epochs are finite reviewed nonsecret args. Password is never
an input/arg/env/UI/admin/guest key. Tool's PRIVATE source uses actual OS CSPRNG
**8 realm bytes +32 password bytes**, separate real MbedSHA256/explicit legacy
MD5 to construct independent RTSP HA1. Realm is random qualification-salted and
fixed for that record—not product default, MAC/UID derivation or PBKDF2 reuse.
PRIVATE tool source hardens its own PC process core0/nodump/caps0/NNP/mlock before
private generation; failure produces no secret or fallback. No native target
private generation/provisioning/positive login occurs in this lane.

Only two EXCL0600 fixed PC files:

- `rtsp-material.bin`: exact1536-byte **private same-source** record, zeroed padding.
- `rtsp-password.pc`: PC-only password retention for a later independent client;
  **NEVER transfer this password file to target or read it in root helper**.

No contents/realm/username/nonce/HA1/password/key/verifier/ciphertext/payload file
hash inventory/logging/dumping. All material/hash-context/scratch copies wiped
by actual Mbed zeroization; caller owns quiescence/writer close and private file
retention. Existing files refuse overwrite; no disk persistence format/product
credential store is claimed. `source_abi.h` compile asserts actual reviewed
sizeof1536/alignment8 and offsets HOST+ARM; private native endian/padding is not
a public wire/persistent ABI. Do not guess1536 from this README independently.

## Executed HOST class / offline reproducibility

Separately compiled PUBLICDUMMY tool accepts **only** `--PUBLICDUMMY`; HOST helper
requires actual nonroot caller and DUMMY account partition. PRIVATE mode never
executed by builder/agent; strict root helper on HOST only tested for pre-input
nonroot refusal, never borrowed root flags from a mock. HOST helper uses exact
pinned V2 PUBLICDUMMY4MiB child for real entry/isolation/signal/wipe/exit testing,
with no native origin/groups0/2MiB claims. ASan/UBSan instrument helper/tool paths;
actual hardened child remains the reviewed optimized image, not unlocked shadow VM.

```sh
python3 -B apps/rtsp-auth/native-qualification/build.py entry01 --toolchain /path/to/existing/armv5-eabi--musl--stable-2025.08-1
```

Use a fresh name under `evidence/delegated/20261012-native-rtsp-verifier/runs/<new>`. Inputs,
pin generation provenance, exact compile/link/run commands/versions, SOURCE /
headers/runtime/library/image hashes, licenses/adaptation diffs and scalar
results are recorded; no generated material/password files or hashes enter
manifests. PRIVATE executable is SOURCE-only, ARM helper static-link/readelf
only—**NEVER executed**. Root entry/provisioning2MiB actual measurement is HOLD.

HOST PUBLICDUMMY tests: SHA256/MD5 generation source ABI/modes/stdout discipline,
fixed native positive-crypto-not-tested receipts, real child START/OPEN/CHALLENGE/
badquery/REVOKE/normal exit0, overwrite and PRIVATE-class rejection, own input/
scope symlink/../hardlink/mode/image pin/class negatives. Existing V2 low-level
hardfault/PDEATH/entry/seccaps checks remain source authorities, not inherited
native pass; no source-frame production or child READY fabrication.

Additional separately compiled HOST hardfault helper signals ONLY its actual
owned pidfd child after real READY/provisioning; output is HOLD, never normal
exit0/self-wipe success. Own PUBLICDUMMY source-length corruption tests actual
provisioning ACK failure while parent still reads only the public48-byte prefix.
Neither fault hook is present in the ARM/root helper or PRIVATE material tool.

Final review corrections: bad-token entry requires exactly `RV_BAD` from the
reviewed child's mismatched-handle ACK (`RV_DENIED` is for expired/revoked grants; transport
failure is not a passed negative); fixed-file opens reject metadata/type before
open, use O_NONBLOCK/O_NOFOLLOW and recheck inode identity/type afterward (FIFO/
directory negatives included). Unknown-retirement owner pointer is volatile so
the allocated context is not optimized away. Build records actual HOST execution
separately from PRIVATE/ARM nonexecution and nonroot strict-helper refusal.
Earlier entry01–04 source/reports/failures are preserved; those historical builds
wrote `local/validation/native-rtsp-verifier/` outside the two write-only paths.
That was a scope error, not added authorization. They are left untouched; new
output stays exclusively in the authorized evidence lane. PUBLIC SOURCE/report/
build-log copies are retained in `prior-entry04/`; generated payloads were NOT
read, hashed or copied into that inventory. Native output must be collected by
the parent separately, never inferred from these HOST results.

Entry05 preserved a review-test expectation failure: it demanded `RV_DENIED`
where engine.c explicitly returns `RV_BAD` for a mismatched token. The final
entry uses exactly `RV_BAD` and additionally tests same-size public image
content tampering, so neither arbitrary error nor file-size refusal substitutes
for the child's ACK or adapter IMAGE-pin check. Scope/type/metadata tests remain
HOST-only; no native root authority is fabricated to test them.

**ONE parent operation:** independently review/rerun this offline source pack,
then explicitly stage and invoke the exact pinned ARM root helper once with
parent-created PRIVATE material under fresh root-origin/2MiB/READY15/owned-exit
qualification and the existing independent protection, without watchdog open.
Native positive auth/per-effect IPC/media/NVR and all product gates remain open.
