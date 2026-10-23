# One-use device-free bootstrap — design before code

2026-10-23 · CORE-03/04, SEC-01/03/05/11, STREAM-07. New SOURCE candidate only.
Frozen provisioner/handoff/constructor/runtime/backend code is not changed.

## Actual composition and stable addresses

One persistent `qc07f_bootstrap_v1`, initialized once with
`QC07F_BOOTSTRAP_INIT`, contains the actual zero runtime, actual
`CANDIDATE_PRESTARTED_INIT` owner and actual `RTSP_PROVISIONER_PROCESS_INIT`
provisioner. Its fields are public only to allocate static storage: PRIVATE after
begin, including embedded runtime/owner, with no mutable-handle getter or activation/
serve export. No copy/reset/destructor/reinitialization or second attempt.

Trusted caller supplies the real config, graph/routes and stable public bootstrap
context before begin. `cfg.context` already addresses the embedded future runtime;
callbacks (especially real random_ready and retain_verifier) accept that runtime
address and, when called after prepare, use `runtime.bootstrap`. Before prepare
runtime is zero and callbacks must not run. No native userdata is placed there.
Config, graph, routes, callbacks and all object/context addresses remain stable.
Only the documented library mutations below are allowed. No shallow snapshot is
claimed to protect arbitrary graph mutations or malicious trusted C access.

1. Begin calls actual `rtsp_provisioner_begin`, with the canonical caller ingress
   slot, same config/routes and actual embedded fresh prestarted owner. Its real
   preflight requires `material_fd == -1`; ingress moves before fallible preflight.
   Parent never reads/encodes credentials. Only the provisioner module forks here.
2. Observe delegates to actual provisioner observe; READY requires receive checks,
   actual own-child wait0 and known parent closes. Receive alone is not READY.
   Observe never binds/prepares/starts. HOLD still delegates when the provisioner
   is HOLD, allowing its still-known child collection without clearing HOLD.
3. Explicit start requires actual provisioner READY, then calls **bind → actual
   `qc07f_owned_runtime_prepare_v1` → actual `candidate_prestarted_start`** in that
   order. Never prepare in PENDING/READY before bind: prepare changes context,
   source_ready, guard and retention callbacks and would break unchanged-config
   checks. Bind moves the readonly FD into the original config. Prepare creates
   addresses/references only, not native init or backing. Start uses that exact
   owner/config and actual verifier fork/exec/checks. No fabricated ready object.
4. This slice stops at actual prestarted READY or failure. No postfork bind,
   nh_native_init, provider/backing/listener setup, media or serve is reachable
   through this API. Public phases describe source progress, not native permission.

## Failure lifetime and the one explicit verifier finish

Ordinary admitted failures latch the first error; no auto retry/start/cleanup.
Invalid identity and active reentry refuse without touching another live owner.
Before provisioner fork, moved ingress remains in its actual owner on failure.
Provisioner HOLD retains FDs/queued rights, possible linked file and known child;
observe remains available to collect that child without admitting startup.
After bind, config owns material_fd even on prepare/start/finish failure.
No existing backend path closes that original slot: rv_start duplicates it and
closes its own duplicates. No bootstrap close/unlink or numeric close retry.
Borrowed directory and linked nlink1 file remain live/exclusive through all states,
including verifier use/finish and HOLD; later file retirement policy is absent.

The actual backend start can return START_FAILED with retained child state, or
HOLD after UNKNOWN retirement (already calls real caller retain_verifier).
Only actual READY/START_FAILED from this composition admit ONE explicit finish
call. It invokes actual `candidate_prestarted_finish`, never raw destroy/reaping.
No source/backend field is interpreted as global native-ownership proof. The
trusted no-earlier-native/media-anywhere precondition and this closed API's lack
of activation/serve are essential; callers cannot borrow handles, activate native,
then reuse this finish as a pre-native permission flag.

Actual finish0 AND actual PS_FINISHED produce FINISHED_DEVICE_FREE. Nonzero finish
is sticky HOLD even when PS_FINISHED reports known retirement (the actual API can
return a historical start error or missing normal wipe/wait0). UNKNOWN/HOLD retains
the real owner/child namespace and caller retain_verifier sink; no finish retry.
Failed start error remains recorded even while explicit finish is eligible.
FINISHED_DEVICE_FREE is verifier-only, not FD/file/native retirement or exit policy.

## Real prerequisites, not booleans or metadata proof

Trusted authorized external RTSP writer, canonical exclusive preopened ingress,
genuinely secret-free/device-free parent before fork, no previous prepare/start
ANYWHERE, native root/no-groups class and inherited-FD/environment/capability policy
remain caller prerequisites. So do real qualified public randomness, root image
identity/pinning, parent hardening required by actual rv_start, exclusive0700
directory/ancestors/name/writer exclusion and whole-lifetime process retention.
Single serialized thread, no reentry/signals/external reapers/FD aliases/extra fork,
no scope exit through HOLD; callbacks preserve these conditions and do not acquire
native/media resources or expose mutable owner handles. No fake RNG, success/noop
retention callback, authorization/native bool, default credentials or fixture.

Finite loops/nominal budgets in dependencies do not bound stalled kernel calls;
no all-domain recovery, C1/native-clock waiver or watchdog guarantee follows.
No heap/media buffer added; no128KiB exception or inherited1MiB-site fit claim.
External ingress/launch, durable generation/storage/rotation, linked-file failure
policy, external media owner/recovery/backing, native/main/packaging remain gaps.

## Selected evidence

HOST and retained ARMv5 changed-unit compilation with real headers; audit frozen
build02 runtime archives/members/flags and provisioner native-shaped objects before
any read-only reuse for a new relocatable closure. Exclude PUBLICDUMMY, HOST native
fallback/stubs and duplicate graph/crypto objects. No old builder/generator, test,
fixture, executable, child/verifier, secret ingress or native/hardware execution.
Compile-only source handback requires independent ordinary review and parent
adoption before any use. No SEC/G3/G8/product or deployment acceptance.
