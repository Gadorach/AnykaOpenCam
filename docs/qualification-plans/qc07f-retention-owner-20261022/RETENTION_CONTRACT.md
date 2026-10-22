# RP-08 failed-NEXT retention — prerequisite contract, before code

2026-10-22. SOURCE investigation only; CORE-03/04, CAP-05/07,
STREAM-01/03/04/07 dependencies. **Not an implemented API or execution admission.**
This contract was written after inspecting the actual native, RTSP cleanup,
verifier and existing process supervisors, before any production patch. None is
made in this increment: the supported integration stops at missing bootstrap and
recovery ownership. Requirements below are requirements, not established facts.

## Actors and actual custody

| Actor | Required lifetime / actual ownership | Available now |
|---|---|---|
| B: trusted bootstrap | Establish exact boot/image/driver, sole descriptions, permitted backing/effects and separately admitted recovery. Create the process topology before native device/lease/backing/session setup. | No installed camera bootstrap in the selected sources. Public config/callbacks are not B. |
| O: external media-process owner | Directly create and keep a stable relationship to M before any media effects; retain its actual unreaped-child/pidfd observation handle and terminal record. Remain alive on uncertainty, without termination/respawn timers. | Existing verifier/HOST/web owners do not provide this media-owner contract. |
| M: media process | The one native singleton, actual FD table, staged/active/returned slots, native CPU mappings/raw/resident/parked backing, caller-owned RTSP objects and server call stack all remain here. | Native can fence and SIGSTOP itself; no external media owner is installed. |
| V: isolated RTSP verifier | Created by M's existing rv_start; exact pinned IMAGE/material/READY15 and policy remain unchanged. M owns rv_client and its verifier-child relationship. | Implemented verifier ownership, **not** ownership of M or native DMA. |
| R: independent recovery authority | Resolve the affected hardware/driver/reset domains and establish positive retirement/reset evidence before any later release/reuse. | No selected runtime bootstrap/recovery association; historical parent hardware observations are not it. |

O does not own M's device descriptions merely by holding a pidfd. Those actual
descriptions and CPU mappings stay owned in M while M remains alive. An unreaped
zombie/pidfd is process-identity evidence, **not** retention of the exited task's
FD table/address space, an OOM guarantee or DMA-retirement proof. O's continued
life is itself a platform lifetime prerequisite; no callback, environment value,
numeric PID, metadata copy or user-provided ready Boolean establishes it.

## Bootstrap / fork / object-lifetime order

1. Establish a real device-free B/O/M launch relationship, with exactly one wait
   owner and a pre-effect release boundary. No existing native, guardian/watchdog,
   sensor/frame/encoder open descriptions, borrowed userdata/backing, leases or
   active session may be duplicated into another process by fork. No fork after
   provider adoption; CLOEXEC only acts at exec and does not fix pre-exec aliases.
2. Complete all remaining fork/exec (including V) before opening/adopting native
   descriptions or setting up media leases/backing/session. V's original startup
   checks/pin/2MiB policy cannot be replaced by an inherited numeric handle.
   Device-free public routing/config required by verifier startup may be prepared
   earlier; it must not contain an already established native/provider/guardian
   userdata or backing owner that fork would duplicate into V.
3. Only then may the single M acquire/adopt the native descriptions, establish
   native/provider userdata, stable producer/bridge/lifecycle ownership metadata
   and admitted backing, and enter media effects. Caller config and public routing
   already used by verifier startup remain stable. Original source order applies: singleton
   init, UNKNOWN watchdog take, storage/exact139 opt-in, provider take, one-shot
   memory setup, original paid backing gates and guarded NEXT. No new memory grant.
4. No later fork/dup/SCM transfer, concurrent FD changes, provider dispatch,
   signal reentry or asynchronous callbacks. All native/server objects are stable,
   disjoint, serialized and nonmoving throughout service and terminal hold.

Current server entry starts V internally, before its own gate/listener. It does
not offer an already-owned initialized backend entry. Therefore a conventional
"prepare native provider, then serve" wrapper would fork after adoption. Merely
starting another verifier before serve would create two owners/children. Lazy
native setup through an arbitrary readiness/listener callback is not an installed
bootstrap or an independently proved object/FD lifetime. This contract grants none
of those workarounds.

## Callback return, failure and no-return boundary

- Successful NEXT must still return the actual native borrowed packet and keyed
  ownership facts, with the complete guarded capture/encode/postcheck/disable
  semantics. RTSP release and exact frame END retain their original meanings.
- A normal non-OK NEXT may return under the unchanged public contract only when
  no packet is returned **and partial work really retired internally**. Zeroing
  outward packet/key fields is not such retirement. In particular the native
  post-encode lifetime/receipt/clock/disable failure can retain actual work.
- For uncertain work, any future same-process integration must fence the actual
  singleton and enter terminal **nonreturning** quarantine before returning to
  RTSP or to any caller that can release/free/close/unload/exit. This is not an
  error return made contract-compliant by moving metadata to another owner.
  It must report neither CAMERA_OK nor retire-OK nor outward service success.
- No-return terminal hold preserves the actual server stack as well as stable
  caller state: backend, hub, clients, capture gate, common producer, bridge,
  config, native context, returned-FD destinations and their backing/code. A copied
  hub/frame/demand/rv_client pointer cannot preserve a returned stack frame.
- On source/release/retire/clock uncertainty, retention callbacks cannot simply
  return and allow an unsafe cleanup path. No STOP/END/ACK, close/free, rearm/feed,
  reset, kill, resume or retry may be inferred from failure notification. Do not
  alter the framework contract to call retained partial work retired.
- V-only uncertainty has distinct semantics: rv_stop already targets only its own
  verifier child. Its mandatory retain_verifier sink must keep the actual client,
  namespace and M alive if retirement remains unknown. That is not permission to
  apply verifier TERM/KILL cleanup to a media/DMA owner.
- Regular shutdown is unsupported when any owner/loan/backing/child retirement is
  uncertain. No returning destructor or normal-exit path exists for that state.

## Partial transfers and terminal identity records

The latest strict return API is healthy **pre-memory** only. It transfers sensor,
frame, encoder, then UNKNOWN watchdog sequentially. Populated destination slots
own their actual earlier transfers even on later error; remaining native/staging
slots own only their remaining actual descriptions. No rollback/close/retry or
"all native/all returned" assumption. The destination records are in M and must
survive alongside its native owner. After all transfers RETURNED stays terminal,
including a final cancellation error. Post-memory/failed-state return is absent.

Any future bounded terminal record must preserve, without exporting private
payload, the actual direct child relationship/owned observation handle and last
known process status, actual FD-role ownership in M (active, staged and returned),
expected source/file/session tuple, keyed generation/state, last valid snapshots,
close-unknown former number marked NOT-owned, backing/loan presence and the reason
and stage of hold. Numeric FDs describe M's table only; copying them is not a
transfer. No credential/material, packet/raw data or payload digest is required.
Existing native scalar diagnostics do not include an installed O/M relationship
or guarantee external record delivery. Logging/flush may itself block; it cannot
be the recovery mechanism or finite-latency proof.

## Watchdog, exit, reset and release

- Preserve the existing fixed10s one-shot session, source phase<2s and nominal5s
  watchdog. A real kernel/hardware watchdog may reset the CPU while M is stopped.
  SIGSTOP stops userspace execution, **not hardware/DMA** and not the timer.
- UNKNOWN at bootstrap/return can already be running with unknown timeout. Failed
  or late disable can already have disabled hardware. Neither a retained FD nor
  QW_HOLD proves known-armed automatic recovery. No feed/enable/disable/END/reset
  operation is added by retention or a supervisor.
- Unexpected M/O exit, OOM, fatal signal, IPC EOF or lost observation is a diagnosed
  retention failure/unknown completion. If M exited its FDs/address space are gone;
  a wait result/zombie/pidfd cannot preserve them or prove DMA stopped. No false
  safe state, automatic restart or regular cleanup follows.
- Only R's separately admitted, positively observed retirement/reset for all
  affected domains can authorize subsequent disposal/reuse. SoC reset is not cold
  radio/rail reset; process exit, nominal timeout elapsed and held U-Boot text from
  a historical campaign are not current all-domain retirement. No recovery
  operation is selected here, nor any HOLD cleared.

## Implementation decision

**STOP before a production patch.** There is no real device-free media bootstrap
owner/recovery contract to integrate with, and the existing monolithic serve entry
forks V internally. Adding a pidfd or no-return callback alone would not establish
these missing lifetimes or recovery. No wrapper, new producer, media RPC, dummy
main/supervisor, background feeder or framework redesign is delivered.

One feasible next SOURCE increment for parent selection is a separately reviewed
device-free server/backend lifecycle split: preserve the existing real verifier
initialization and allow its one actual stable backend to outlive/enter serve
without a second init/fork. Define partial-start/destroy ownership first. It is a
prerequisite only, not external M ownership, native readiness or deployment. The
actual B/O/M launch and R policy still require a concrete platform entry before
this contract can be implemented. No automatic next queue follows this report.
