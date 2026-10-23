# QC07F device-free bootstrap — independent ordinary review

2026-10-23 · CORE-03/04, SEC-01/03/05/11, STREAM-07.

**Conditional narrow SOURCE/API/order/custody sufficiency. No required correction
identified in this bounded connection.** Parent independently disposes before
adoption. This is not runtime/native permission, full task completion or product GO.

## Exact reviewed candidate

Paths below are workspace-relative. `B` means
`apps/rtsp-auth/qc07f-bootstrap-candidate/`; `D` means
`docs/qualification-plans/qc07f-bootstrap-20261023/`.

| File | SHA256 |
| --- | --- |
| B/bootstrap.c (111 lines,4877B) | `d67d1f4c752b3809ed11f2f5862b9296ba7c57098df62f3cbe29c46dabf79eb4` |
| B/bootstrap_v1.h | `4bbd0bbc39fec24e0acc3d2146bf05e86c29be155bc462b6fc6eecb48306c863` |
| B/compile_recipe.sh | `63b22a3208f377cf9658dcba7fd6dc4769e83ebd0e10c0c341afbfa3bc2708fd` |
| B/LICENSE_NOTICES.md | `b04e0cc6abf2c43ace6bb3943759d5bdf31ab8afbf03e24770698a06c679da94` |
| D/CONTRACT.md | `01649509b32638297501168c79a54013e0e4b41b4bcadd62520dc1f6a9413564` |
| D/HANDOFF.md | `e52ceb807bf2ae7d0189c5c5fcc66f39ca7a9c7c465e398f1fac6ffc3bf6fe60` |
| D/BUILD_AUDIT.json | `36648d659f036e965e2951c91d05e78a221e22a2dc32b7e43730d43744456bb5` |
| D/BUILD_INPUTS.json | `93cd1705d9881257e3d08c543b1689157f5461757e07b9e56a87c7d67887843e` |
| D/DELIVERY_PINS.json | `af30831a26348d47fbd593e8b2cc6182c9374a3c2ebc5458a401ad18baea464d` |
| D/runs/build01/REPORT.json | `86ed8b9404388550593df89b2e86fee76473675eb0474011beebe4fd5098181f` |

Four-file source aggregate independently reconstructed using the delivery's
sorted-path/NUL/raw-digest method:
`98cfbcebc49e0e8592588c274bac3373cc314961f466f2c0b51dc4b76f8e5b2d`.

Actual dependency bodies/headers read, not just declarations or prior opinions:

| Under apps/rtsp-auth/ | SHA256 |
| --- | --- |
| rtsp-provisioner-process/provisioner_process.c | `d929b392110d6563b818aa00993301e07d5542afb3a2f4c37cafebe6f28b3724` |
| rtsp-provisioner-process/provisioner_process.h | `7208455b184a4a2ed9384b16cff1a05c17217789189ef3fa4812c397ff817bc6` |
| qc07f-owned-runtime/owned_runtime.c | `5e21fcb2559911b79ffbd7732b951b8d8931a84e2e33f03a52f39d09de0dbd43` |
| qc07f-owned-runtime/owned_runtime_v1.h | `024ff9282840c97208644674601181fcbe97befd4e7222c34627e75d5c8b3621` |
| qc07f-owned-runtime/server/candidate_auth.c | `cb27b35e2d8730b8d19f231d339e06cbe4c97fbe0aca7826fe3469d8479825a7` |
| qc07f-owned-runtime/server/candidate_auth.h | `94b6ed560677b7db4119eded9726ab01085c2e0eb392bdad29e961ebcd2ed643` |
| qc07f-owned-runtime/server/verifier_adapter_overlay.c | `03413c00c829f4a94c8630ace490927be4b718bbac9b48b771997a0905c8880a` |

Also inspected actual private-handoff/constructor bodies, runtime native adapter,
`native-watchdog.inc` initialization and imported source-context initialization,
common hook construction, and inherited prestarted/listener API contracts.

## Ordinary findings

1. **Actual owner and provisioner admission.** Header46–47 initializes the actual
   embedded prestarted/provisioner objects and zero runtime. C19–35 calls actual
   provisioner begin with that owner and canonical ingress slot, without prepare.
   Provisioner C232–270 moves ingress before fallible preflight/fork and retains
   failure custody. Its child alone reads credentials, calls real hardening/
   constructor/handoff and sends the readonly reference; no config callback runs
   there. No fabricated ready object, callback or parent credential encoder.
2. **Receive is not readiness.** C38–59 delegates actual observation. Provisioner
   C312–344 requires its own observed wait0, unchanged config/routes/public envelope,
   readonly inode checks and known socket close before READY. Outer HOLD still
   delegates PENDING/HOLD observation, preserving collection of the still-known
   child without clearing either HOLD or retrying unknown wait/ECHILD.
3. **Correct actual mutation order.** C62–81 calls bind → prepare → same-owner
   start, explicitly, only after actual provisioner READY. Bind C347–358 rechecks
   identical config/owner, snapshot, wait0 and known parent closes before moving
   its received slot into config.material_fd. Runtime prepare C204–228 then installs
   real context/source/guard/retention addresses; its common-hook helper constructs
   references only. Preparing before bind would invalidate the actual snapshot;
   this candidate does not do that. Config remains unmodified through bind except
   its documented FD move, then stable after prepare. cfg.context must already
   address the embedded future runtime before begin; runtime.bootstrap becomes
   usable only after prepare and before actual start calls random_ready.
4. **Failed start is retained, not silently destroyed.** C83–90 keeps actual
   START_FAILED finish-eligible and latches the first public error while retaining
   exact start_result. Actual prestarted C227–233 already calls retain_verifier
   after rv_start's uncertain stop; outer HOLD performs no second stop/finish.
   Other failures retain the persistent config/runtime/owner and namespace.
5. **One explicit verifier-only finish.** C93–111 admits only matching actual
   READY/START_FAILED, sets its one-shot flag before actual finish, and never calls
   raw backend destroy or manual wait/reap. Actual prestarted C242–258 checks
   pending/peers/source HOLD and invokes its real destroy policy. Backend C107–116
   and adapter C302–336 preserve uncertain retirement/IPC-close outcomes and real
   retention. Any nonzero finish stays outer HOLD even if PS_FINISHED, including
   historical start errors or missing normal wipe/wait0. Only finish0 plus actual
   PS_FINISHED yields FINISHED_DEVICE_FREE; sticky start error is never cleared.
6. **Original material FD survives finish.** Adapter C187–195 duplicates the
   config FD, closes its temporary duplicates and leaves the original slot owned.
   Its later reads are public-prefix reads. Neither backend destroy nor this glue
   closes the config slot or unlinks the nlink1 file. FINISHED_DEVICE_FREE therefore
   does not establish file/FD retirement, native retirement or safe process exit.
7. **Closed new call surface, not hostile-C encapsulation.** The four new APIs
   export no mutable getter, postfork/native-init/serve operation. Actual runtime
   postfork C230–254 separately calls nh_native_init; that initializer's real
   one-time body is not called here. Whole linked archives still contain inherited
   activation/serve APIs. Public allocation fields are contract-private, not an
   access-control boundary. Active reentry refuses before record access; wrong
   identity refuses without modifying another owner. Admitted bad order can
   permanently HOLD, not retry or discard custody; observe is not post-start poll.

## Frozen build evidence checked read-only

Independently matched863 input pins,48 delivery pins,37 build artifact/log/manifest
pins,180 new dependency records,724 selected original provisioner dependency records,
seven reported tool pins and all24 command/log/START/PASS associations. Counts overlap.
Both pre/post logs contain the same863 OK paths. Author's two HOST/ARM changed-unit
compiles and two actual REL closures PASS are cited, **not rerun**.

Parsed both actual GNU archives including long-name offsets:23 members each,
all46 bodies equal their recorded objects. Each maps to one successful recorded
native-shaped compilation/pinned log, without fixture defines. Selected provisioner
objects0,1,2,3,9 supply only the intended extra dependencies. Actual ELF metadata/
symbol tables match HOST x86-64 and ARM32 EABI5 REL roles and the recorded undefined
sets: all ten selected APIs defined once, no main/PUBLICDUMMY/project/crypto
undefined symbol. Libc/compiler/final-link support remains undefined, including ARM
__ehdr_start. Existing ARM attribute evidence records v5TE; no native ABI inference.
The earlier inline GNU-longname audit mistake remains preserved, not a candidate
failure or a successful first audit. Notices remain scoped to their own sources.

## Essential conditions and real gaps

This sufficiency requires once-initialized, stable, writable, disjoint trusted
storage/slots and the documented embedded references; immutable graph/routes/
config/callback addresses; serialized calls without signals, threads, reentry,
aliases, extra fork or competing reapers; and whole object/process namespace,
borrowed directory, linked file and writer-exclusion lifetime through every HOLD
and verifier use/finish. Public structs, metadata and singleton/phase fields do
not prove these obligations. No earlier native/media ownership or prepare/start
ANYWHERE, and device-free real callbacks, are essential, not a Boolean permission
or fresh-backend inference. Do not borrow private handles for native use and then
reuse this finish as a cleanup authorization.

Authorized external ingress/writer and launch, genuinely secret-free fork/global
inherited-FD/environment/capability hygiene, qualified entropy, parent hardening,
qualified native image, private-file lifetime/retirement/failure policy, durable
generation/storage/rotation, external media owner/recovery/backing, main and release
packaging remain gaps. No whole compiler closure, native ABI/timing,128KiB cap
exception/inherited1MiB-site qualification, combined memory fit, finite native STOP,
watchdog/kernel-stall guarantee, native authenticated stream or SEC/G3/G8 GO.

Review used public source/hash/archive/ELF/log metadata only; no compiler/linker,
builder/helper, candidate/test/fixture/provisioner/verifier/native/credential or
hardware execution. No Git/index, central-plan/AGENTS, private/proc/dev, network,
UART/RPC, lock or actual HOLD access. Only this fresh REVIEW.md was written.
Further adversarial/negative/fuzz/mutation testing remains deferred. Parent's23g
unexplained-prefix FAILED/HOLD and reported camera OFF/no recorder remain untouched,
not freshly checked here. This review queues no test, native operation or next work.
