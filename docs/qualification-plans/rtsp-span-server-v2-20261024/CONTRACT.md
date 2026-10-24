# Authenticated span server v2 — contract BEFORE source correction

2026-10-24. RP-09/B07; bounded STREAM-01/02/03/07, CAP-01/05/07.
Only fresh `apps/rtsp-auth/rtsp-span-server-v2/` and this directory are writable.
V1 app/evidence/review remain frozen, NOT ADOPTED; its build01/02 failures and
build03 compile success are historical, not v2 observations. Independent ordinary
review and parent disposition are required before any use. No automatic next work.

## Exact bounded replacement

Derive C/header from frozen rtsp-span-server-candidate, preserving SAME exports,
type spellings and include guard as an EXCLUSIVE source replacement. Never link
either old server unit/combined REL with this unit; use accepted span-hub-v2 and
rtp-span-candidate unchanged. Read full v1 C/header/CONTRACT/HANDBACK and independent
REVIEW (`5ae6708f00f3e5e6465730b9b47c6726278964a4bc3e294cba25bae8cf4b9871`),
actual sticky-MAX clock, common detach/lifecycle and owned-runtime cleanup contracts.

Only three cleanup/ownership function boundaries change:

* R1 `finish_au`: preexisting clock/lifecycle/lease uncertainty and a fresh MAX
  enter EXISTING nonreturning retain_span BEFORE logging/release/flag retirement.
  Re-read actual clock after possibly blocking diagnostics, immediately before
  exact safe CPU release; retain on uncertainty. Recheck after pending control
  before packet wiping/failure logging/cleanup. No whole-interval clock prevention
  claim; a later newly observed failure does not undo earlier known-safe actions.
* R1 `close_client` / direct `pause_client`: retain preexisting sticky clock,
  lifecycle_error or lease_uncertain BEFORE native cleanup, fd<0 return,
  unsubscribe, playing clear, detach or other retirement. Fresh boundary reads
  additionally detect newly observed clock failure. Native cleanup returning is
  NOT a retirement receipt. Keep the real span binding across a clean client
  memset so an already-closed slot still has its actual whole-process retention
  destination. Recheck after blocking peer retirement before final local clear.
* R2 `pause_client`: demand_attached stays true through ONE actual detach call
  and a successful post-call actual clock read. Non-OK/clock uncertainty latches
  the error, preserves original notification (metadata-only, synchronous, no
  reentry under the trusted contract), then nonreturns through retain_span before
  flag clear, return, retry, STOP or END. Notification is never acknowledgment.
  Only known detach OK plus valid post-clock permits demand_attached=0.

No new clock/guard/state/helper framework. Ordinary auth refusal, socket errors
and whole-AU timeouts retain the original exact-safe-CPU-release/connection-close
policy; do not convert them all into uncertainty. No stop/auth/deadline/source
callback, producer release/END, packet cursor, per-consumer v2 lease, packetizer,
borrowed lifetime or listener/backend semantics change outside these boundaries.
The same actual sink receives the actual demand and whole live cursor array/
stack/process; existing runtime fallback then repeated SIGSTOP remain unchanged.
No exits, FD close, packet wipe or retry on an uncertain path.

## Evidence, prohibitions and remaining gates

Fresh exclusive output, fixed small copied/adapted recipe: six production C
compiles (new server/accepted v2 hub/span builder, HOST and retained musl ARMv5TE),
two rooted real REL links using the SAME 21 reviewed runtime archive bodies per
target, excluding old server and old packet_hub. Preserve build03 actual retained
relocation closure check and actual API/entry roots; three raw nm Digest UND names
are permissible only with retained zero-reference relocation proof (STB_LOCAL).
Record exact argv/tools/input/dependencies/logs/artifacts and precise C/header
forward/inverse diff plus unchanged-region/reverse proof. Refuse overwrite; a
failure remains fresh FAILED_PRESERVED, no functional test/retry loop.

No original helper import/invocation, C function/program/test/harness/fixture/
negative/adversarial/mutation/fuzz execution, final ELF, socket/listener/backend/
verifier/provisioner/RNG/credential/encoder/DMA/native/hardware action. No Git/index,
AGENTS/central-plan, private or disjoint memory-candidate edit/read/adoption.
Policy refusal stops work without retry/reroute/safeguard changes.

V1 trusted serialization/stable-disjoint storage/no aliases/reentry/signals/fork/
FD mutation, true-fresh hub AFTER last fork and real external lifetime/order
requirements remain mandatory. Native use remains BLOCKED: old flat 1MiB hub
prefault/paid[1] contract is incompatible with eight chunks; real chunk accounting/
adapter/ready integration, producer 1MiB output/raw backing, 128KiB allocation cap,
2MiB process/40MiB/CMA/TLS/combined load, native ABI/image fit, external whole-process
retention/recovery, provisioning and main remain open. No cap waiver/ready Boolean.
Camera OFF/final23h, RAM75 recovery/C1 blocked are inherited parent context only;
no physical observation or hardware queue. No upgrade of v1 or deferred coverage.
