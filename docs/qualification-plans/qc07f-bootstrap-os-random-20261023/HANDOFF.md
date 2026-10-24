# Actual bootstrap OS-random selection — author handback

CORE-03/04 · SEC-06 · STREAM-07. **Replacement SOURCE candidate delivered;
2 HOST/retained ARMv5 changed-unit compiles and 2 real REL links PASS. NOT RUN.**
Independent parent review and explicit adoption remain required; no worker GO.

## Minimal actual feature delta

`apps/rtsp-auth/qc07f-bootstrap-os-random/bootstrap.c` is the frozen111-line
bootstrap's120-line derivative. CONTRACT.md was created before source. The only C
changes are the genuine OS-callback header/type assertion, a strict prepopulated
callback refusal, one assignment and the necessary pre-bind comment exception:

1. Preserve original active/reentry, null, process-attempt, self, phase and
   config/routes preflight. Then require `config->random_ready == NULL`, refusing
   **any** preexisting pointer, even the genuine callback. This is a policy check,
   not proof of otherwise valid config or global freshness. Before admission,
   refusal changes neither object/config nor canonical ingress slot.
2. Preserve the original single-attempt latch and stable self/config/routes/context
   identities. Only then assign actual `qc07f_rtsp_os_random_ready_v1`, immediately
   before actual `rtsp_provisioner_begin` takes its config snapshot. No callback is
   called here; no setter exists after begin/READY/bind. No snapshot mutation.
3. On underlying begin failure, leave the selected callback and process-attempt
   consumed. Preserve canonical ingress/HOLD ownership exactly as the provisioner
   actually admitted it; no restore, close/unlink, cleanup or automatic retry.
4. Observe/start/finish tails are **byte-identical**. Actual known child wait0 and
   bind still precede actual device-free prepare and same embedded owner's actual
   prestarted start. HOLD observation, unknown-child retention, START_FAILED and
   one exact explicit verifier-only finish retain all original semantics.

`bootstrap.c.diff`, `bootstrap_v1.h.diff` and DELTA_PROOF.json provide the precise
delta. Reversing the four documented C edits reproduces original C byte-for-byte,
SHA256 `d67d1f4c752b3809ed11f2f5862b9296ba7c57098df62f3cbe29c46dabf79eb4`.
Header changes are comments only; declarations/enums/layout/initializer and API
tokens remain identical. The original header hash also matches the supplied pin.

**Replacement, not coexistence:** choose THIS C unit plus the actual production
OS-random object instead of the old bootstrap unit. Export names and initializer
are unchanged deliberately. Do not link both bootstrap definitions or the old
combined REL; a second context/unit is not another process attempt. The original
single process-local guard is retained, not weakened or replaced by a bool permit.

## Actual unchanged dependencies and caller contract

The provisioner's actual memcpy snapshot occurs after selection; its unchanged
config/route/envelope checks remain in force through READY and bind. Actual bind
moves the readonly material FD before prepare's existing config mutations. Prepare
still requires random_ready/retain_verifier; actual startup32 probe and session32
consumers remain untouched. No fake config, callback, entropy, provider or main.

Caller must still supply fresh immutable public config/graph/routes, material_fd=-1,
context already addressing the future embedded runtime, real retain_verifier and
device-free callbacks, stable private whole-object/namespace lifetime and native
image/hardening contracts. No prior prepare/start/native/media ANYWHERE, aliases,
external reapers, signals, threads, extra fork or reentry. NULL/function equality
does not prove these. No mutable owner getter/native activation/serve export was
added. All original linked-file and FD custody survives failure and verifier finish.

Actual accepted OS callback/source is reused unchanged: one getrandom32 with
GRND_NONBLOCK, strict full wipe/refusal on short/error, no retry/cache/fallback.
Its context is ignored, not used to invent runtime readiness. Verifier CSPRNG,
credentials, TLS/HTTPS/keys and actual runtime/provisioner/graph APIs are unchanged.

## Compile and real closure evidence only

Fixed invocation (already completed once; refuses existing output):

```sh
env -i PATH=/usr/bin:/bin LANG=C LC_ALL=C TZ=UTC /bin/bash apps/rtsp-auth/qc07f-bootstrap-os-random/compile_recipe.sh
```

INVOCATION.json records host UTC **2026-10-24 02:26:03.612705–02:26:04.718145**,
exit0. The assigned lane/date remains20261023; these are actual recorded host
timestamps, not target/clock admission or a claimed corrected clock.

- 2 strict changed-unit C compilations, 2 real REL links, **24 successful tool
  commands**; both compile logs empty. HOST GCC16.2.1; retained musl GCC14.3.0
  ARMv5TE ARM/soft-float. No old builder/generator was invoked.
- Before linking, checked **902 input paths**, original callback/C/header pins,
  runtime364 source/input pins and378 dependency paths;88 live-source/header
  matches to frozen runtime snapshots. Verified all46 current runtime archive
  member bodies against their retained object pins and actual production flags.
- Real additions are provisioner objects **0,1,2,3,9** and production OS-random
  `{host,arm}-random.o`, not its combined REL/zeroizer/harness. Audited all selected
  compile argv/source/object roles and **871 reused addition dependency records**.
  No PUBLICDUMMY/HOST fixture/fallback/test defines on selected production units.
  Runtime archive supplies real graph/wire/parser/SHA/constant-time/zeroize once;
  additions provide real process/handoff/constructor/Digest/MD5/randomness.
- Read-only ar/readelf/nm checks precede link in the fixed recipe. New actual
  consumed dependencies:182 records/146 unique paths, all matched existing input
  pins. All902 inputs matched again after build; counts overlap, not independent
  qualification counts. BUILD_AUDIT.json and REUSED_DEPENDENCIES.json retain detail.
- `runs/build01/REPORT.json` pins37 generated artifact/log/manifest files and exact
  argv/timing/exit/log records. HOST332832B and ARM227060B outputs are **REL, no
  program headers**, not executables or native ABI/whole-program memory-fit proof.
- Each closure defines all11 integration APIs and actual zeroizer **once**, no
  duplicate global definitions, main or PUBLICDUMMY/fixture symbols. No unresolved
  project/crypto API. Only enumerated libc/compiler/final-link symbols (including
  getrandom) remain unresolved for eventual final platform linking.

One inline post-build metadata assertion initially used the source macro names
instead of actual production provisioner export names; METADATA_NOTE.md preserves
the correction. No compiler/linker repeat or source/object change followed. This
was not a candidate invocation, negative test or functional failure/success.

## Honest nonexecution and remaining gates

**Zero** callback/getrandom/bootstrap/provisioner/verifier/test/helper/generated
program invocations in this lane, including HOST. No credentials/private material,
runtime/native/listener/media, network/UART/proc/dev/hardware, Git/index/central
status or subworker action. Original source/builders/seals and failed observations
remain untouched. Inherited23h is finalized cold-observation PASS ending OFF, not
a live owner/native admission; all earlier radio/reset/23c/23d/23e/23g failures stay.

Native Linux6.18.55 ARM getrandom linkage/syscall/cold CRNG/entropy health remain
unqualified; the earlier separate HOST callback observation is not repeated or
upgraded. No SEC-06/G3/G8/product or deployment acceptance. External authorized
ingress/secret-free launch/global FD-env-cap hygiene, native image/hardening,
external owner/recovery/backing, main, durable generation/storage/rotation and
linked-file retirement remain missing. Existing128KiB/combined-load/1MiB-site and
strict C1/recovery gates are not waived. No memory/native STOP/restart/boot claim.

GPL-2.0-only remains derivative-scoped; original dependency notices/licenses are
preserved. Binaries/REL are local evidence, not curated Git/release inputs.
**One next task: independent parent source/order/custody review of this replacement
and its pinned real closure; explicit adoption before any use. No native queue.**
