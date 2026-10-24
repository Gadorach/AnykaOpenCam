# Bootstrap selection of actual public RTSP OS randomness — before code

2026-10-23 · CORE-03/04, SEC-06, STREAM-07. Isolated source replacement
candidate, requiring independent parent review and explicit adoption before use.
Only this directory and `apps/rtsp-auth/qc07f-bootstrap-os-random/` are owned.

## Minimal real connection

Derive the frozen bootstrap C/header, preserving its ABI, same exported names,
embedded actual runtime/provisioner/prestarted owners and initializer. Choose this
bootstrap translation unit **instead of**, never alongside, the original. Its
process-local single-attempt/reentry guard replaces the original guard; two units
or contexts do not grant independent attempts. No new main or configuration API.

Before mutation, preserve the existing `active`, null, process-attempt, self and
EMPTY/config/routes preflight, then additionally require `config->random_ready ==
NULL`. Even the genuine already-populated callback is refused, not silently
overridden. Before admission, refusal leaves config, ingress and object unchanged.
Only after the existing local preflight admits and latches the one process attempt,
select `qc07f_rtsp_os_random_ready_v1` in the actual caller config immediately before
the actual `rtsp_provisioner_begin`. Include the real callback header and assert its
type matches the real config field. Do not call it here.

The actual provisioner snapshots this selected config. No setter after begin,
READY or bind; no live/snapshotted config mutation. On underlying begin failure,
the selection remains, as do the actual canonical ingress move/HOLD lifetime and
one-attempt consumption. No restoration, cleanup, close/unlink or retry. The old
comment forbidding all pre-bind config changes must narrowly except this initial
selection; all other C flow stays byte-identical: HOLD observer, known wait/bind →
device-free prepare → same owner's prestarted start, and exact explicit finish.

## Unchanged prerequisites and limits

Read the original bootstrap CONTRACT/PARENT_DISPOSITION, runtime and provisioner
contracts, and OS-random CONTRACT/PARENT_DISPOSITION. Caller still supplies fresh
immutable public config/graph/routes, `material_fd=-1`, context already pointing
at the future embedded runtime, real retain_verifier and device-free callbacks,
native image/hardening and private stable lifetime. NULL/function equality is not
proof of freshness, authorized ingress, secret-free parent, global hygiene,
external recovery, native ownership, entropy qualification or permission.

The accepted callback is unchanged: one real getrandom32/GRND_NONBLOCK per valid
request, strict wipe/refusal, no retry/cache/fallback; context ignored. The actual
startup probe and session callsites are unchanged. No dummy entropy, bool permit,
synthetic callback, alternate provider, fixture define or native fallback. No
runtime/provisioner/crypto/config-graph API changes, postfork bind/native activation,
listener/media/serve, backing, retention implementation or new main.

Underlying begin failure can retain ingress before snapshot/fork; do not infer
that selection or outer HOLD proves underlying ownership/retirement. Existing
namespace/object/config/linked-file lifetime and uncertain-child retention remain
mandatory, including failure and explicit verifier-only finish. No scope exit,
aliases, reentry, signals, external reapers, threads, reinitialization or second
attempt. Original failures/holds and finalized23h OFF boundary remain untouched.

## Bounded compile-only evidence

Compile this changed unit HOST and retained musl ARMv5 with real headers. Reuse
only audited frozen runtime build02 archives, actual provisioner build01 objects
0,1,2,3,9 and actual OS-random build01 `{host,arm}-random.o`. Verify source/flags/
pins and current archive/member bodies/roles before linking. No old bootstrap
object/combined REL, random zeroizer object, duplicate crypto or fake providers;
the real runtime archive already defines the real zeroizer once.

A short fixed refuse-overwrite recipe records exact argv, logs, tool/input/
dependency/artifact pins and HOST/ARM relocatable closures. Inspect APIs defined
once, no main/PUBLICDUMMY or unresolved project/crypto symbols. Compiler/linker/
readelf/nm/ar/public-metadata inspection only: **no generated program, callback,
bootstrap, helper/test, getrandom, provisioner/verifier fork or credential call**,
even HOST. No old builder/generator, network/private/proc/dev/hardware/Git/index/
central-plan action or subworker. No negative/adversarial/mutation/fuzz tests.

Source and compile/link evidence do not qualify ARM syscall/cold CRNG/entropy,
native ABI, timing or memory fit. External ingress/launch/hygiene, main, storage/
generation/rotation, linked-file retirement, external owner/recovery/backing and
128KiB/combined-load gates remain open. No autonomous native queue or worker GO.
GPL-2.0-only stays scoped to derivative source, not a global license grant.
