# RP-08 external failed-work retention — bounded SOURCE result

2026-10-22. **BLOCKED before production patch; contract and exact source freeze
delivered. No code, compilation or runtime result.** CORE-03/04, CAP-05/07 and
STREAM-01/03/04/07 remain prerequisites, not completed tasks. No full app/native GO.

## Decision and smallest technical dependency

There is no installed device-free media-process bootstrap with a continuing
external owner and separately admitted recovery policy. The selected RTSP server
also owns verifier startup inside its monolithic serve call. Consequently:

1. Acquiring/adopting native/provider/watchdog descriptions before serve would
   fork them into the verifier child. CLOEXEC does not prevent that fork-time
   duplication, nor undo pre-exec open-description/refcount/close effects.
2. Calling quarantine on failed NEXT can preserve the **same live process**, but
   neither creates its external owner nor proves its caller/config/backing lifetime
   or hardware recovery. Returning error afterward would still violate RTSP's
   required internal partial-work retirement. A pidfd or copied hub/frame metadata
   does not repair either condition.

**The smallest source dependency is an explicit device-free server/backend
lifecycle boundary:** perform the existing actual verifier init once, keep that
same stable backend alive, and later enter service without reinitializing/forking
again. Partial-init/destroy/retain-verifier ownership must be specified, not hidden
behind ready callbacks. This can be a future fresh derivative for parent selection;
it is not implemented/selected automatically here. A real B/O/M bootstrap and
independent recovery policy are still required afterward; the split alone is not
external process retention or native readiness.

`RETENTION_CONTRACT.md` was written before any production code (none followed).
It specifies actors, actual FD custody, fork/setup order, stable caller/server-stack
lifetimes, failed return versus nonreturning fail-stop, partial transfers, terminal
records, unsupported shutdown, unexpected exit/OOM and recovery/reset authority.
Initial pre-implementation contract SHA256 (later clarified to permit device-free
public verifier-startup config before native userdata setup; final hash in OUTPUTS):
`88d78d6bbd7563944e7f5467d4c6861a0787d9d1c0cf7fad6e4aa05f71a6574e`.

## Exact source anchors

Path abbreviations below are workspace-relative:

- **G/** = `docs/qualification-plans/qc07f-provider-return-20261022/runs/build01/source/`
- **S/** = `apps/rtsp-auth/isolated-server-host-tail/`
- **W/** = `apps/rtsp-auth/qc07f-runtime-candidate/`
- **N/** = `apps/web-media/native-producer/`

| Finding | Actual source / consequence |
|---|---|
| Public error-return boundary | `framework/include/camera/rtsp_server.h:36–42`: non-OK NEXT owns no returned packet **and retires its partial work internally**. Common frame end/STOP bookkeeping is not DMA retirement. No public contract edit is proposed. |
| Native uncertainty survives zero outward outputs | G/`rtspd-native-import.c:177–247`: capture/encode can fail after backing/capture/job effects; error retains produced/resident backing and FDs, clears returned packet, does not prove DMA idle. G/`native_adapter.c:127–140` can fail in lifetime/cancel/time checks after acquisition. G/`native-watchdog.inc:62–85` zeroes valid outputs but fences and retains actual work on those failures or failed disable. |
| Existing quarantine is process-local and nonreturning | G/`native_adapter.c:185–202`: actual fence, scalar active/staged FD/backing/identity diagnostics, fflush, repeated SIGSTOP. No external media owner registration/observation or recovery is installed. SIGSTOP is not device/DMA stop. Diagnostics/flush cannot establish finite-latency recovery. |
| RTSP failed NEXT goes toward returning cleanup | S/`rtsp_server_candidate.c:535–551`: non-OK NEXT is not released; exact common END/failure/close and `clients_done` follow. There is no native partial-work custody transfer. The frame-retention notifications at :527/:541/:573 cover particular other boundaries, not a new unconditional failed-NEXT handoff. |
| Cleanup can invalidate live callback context | S/`rtsp_server_candidate.c:431,438–443`: backend, hub, clients and capture gate are serve-stack objects. :595–626 closes clients/listener, stops the hub, may retire if eligible, unbinds guard, finishes backend and returns. Failure guards avoid some duplicate detach/retire, but do not preserve that stack after return. S/`candidate_auth.h:25–35` sinks are synchronous callbacks, not owners of M. |
| Actual callback sinks have only fixture custody | S/`fixture.c:127–137` copies hub/demand/frame or stores rv_client in fixture records. They are synthetic HOST instrumentation, not native FD/process ownership. No fixture is adopted or executed. |
| Verifier retention/shutdown is a different owner | S/`candidate_auth.c:105–117` calls actual rv_stop/rv_destroy and requires retain_verifier on unknown retirement. S/`verifier_adapter_overlay.c:24–39,287–306` owns its own verifier child/pidfd/IPC/client; stop requests TERM then KILL and requires own wait. This establishes verifier CPU exit only, not native DMA retirement or M survival. |
| Fork is inside serve startup | S/`rtsp_server_candidate.c:431–445` calls backend init before binding source guard/listener; S/`candidate_auth.c:97–103` calls rv_start; S/`verifier_adapter_overlay.c:164–222` duplicates image/material and forks at :179 before pidfd/release/READY. Child closes/reassigns inherited descriptors only later at :119–121. No prestarted-backend serve API exists in the inspected entry/header. |
| Frozen glue cannot simply wrap the latest native owner | W/`rtsp_native_source.c:42–62` directly calls produce_h264_frame and its postchecks, not the newer guarded nh_native_next wrapper. :94–97 still refers to old watchdog_hold, whereas G/`rtspd-native-import.c:41–50` embeds qw_owner watchdog. Old glue compilation is not compilation/adoption of the newer derivative. Rebase/guard integration would be a separate change, not a safe retention-only shortcut. |
| Actual healthy retirement is narrower than error cleanup | G/`rtspd-native-import.c:65–103` requires keyed receipt/sensor-off/END/RETIRED/ACK then sequential close-once. G/`native-watchdog.inc:96–129` requires successful logical disable before CPU free. No uncertainty END/reset/close retry or regular failed shutdown is supported. |
| Strict healthy return does not solve this failure | G/`provider-return.inc:22–44,58–89` excludes memory/session/activation/history/loan/HOLD, moves roles sequentially and returns UNKNOWN watchdog last. Populated destinations own earlier actual moves even on later error; native retains only remaining roles. It is not post-memory/failed-state handback or an external owner. |
| Watchdog HOLD does not guarantee a coming reset | G/`watchdog_owner.c:77–111` enables nominal5s and can fail after disable took effect. :128–145 returns UNKNOWN with no service/control, possibly already running at unknown timeout. Source checks cannot preempt a blocking ioctl or prove timer headroom. No feeding failed work, rearm, END, reset, kill or resume is permitted by retention. |
| Original finite native caller is not an installed RTSP bootstrap | N/`handoff.c:150–180` retains/fences on unknown failed NEXT. N/`main.c:69–72` calls actual quarantine before free when report.quarantined. It is a finite qualification caller with parent recovery requirement, not the newer provider-return integration or an external process owner. Its old early-exit/setup paths are not adopted. |

### Existing process supervision inspected, not reused

| Existing implementation | Why not the missing media owner |
|---|---|
| S/`host_process.py:28–41,73–104` | Owns a HOST fixture child/pidfd; courtesy TERM/wait and pipe close. No descendant/DMA retention or native entry. |
| S/`host_guard.py:34–37,68–83,87–120,172–243` | Dedicated nonroot x86_64 HOST subreaper/session owner. Its bounded cleanup sends TERM/KILL to the fixture group and reaps, then parks only if exits remain unknown. This is appropriate HOST cleanup, not a policy to preserve uncertain native FD/CPU/DMA owners. Environment/hash acknowledgment is no native ownership grant. |
| `apps/auth-web/native-launch/launch.c:193–215,302–354,364–391` | Real pinned web-service bootstrap with own fork/pidfd, child PDEATHSIG KILL, finite service TERM/KILL/wait and returning cleanup. Header explicitly excludes camera/driver/recovery protocol (:2–3). No media/provider/backing lifetime or reset proof; cannot reuse its termination policy for M. No private inputs were read. |
| `apps/rtsp-auth/isolated-verifier/authority/credential_process.c:392–434` | Credential-child bounded CPU retirement/kill/wait, not media/DMA custody. |
| `apps/auth-web/ui-candidate/parent_campaign/owned_client.py:17–25,57–71` | Parent-only PC client pidfd with TERM/KILL/wait/dispose. Does not own a camera media process or descriptions. |

Bounded static searches of apps/framework/bringup found these relevant owned
process mechanisms, not an installed media bootstrap. This is not a claim to have
audited all repository history, private runtime or every possible platform.

## Source freeze / evidence strength

Read the standing index/current execution, baseline, subsystem02/05/08, current
resume, full latest strict-return report/contract/independent review/parent
disposition, full owned-provider report/contract, watchdog-v2 report and frozen
runtime-glue report/disposition. Dated statements are not current hardware facts.

The parent-selected latest20-file authored source remains pinned by
`qc07f-provider-return-20261022/SOURCE_SEAL01.json` at aggregate
`9ac7f363d7395fe944f2d068a4c964a5b20c77e9410e84633c07ce7f8b8a169f`.
Its individual20 SHA256 values were rehashed and directly compared with the seal's
source_files entries; they match. Independent review-seal SHA256 matches the
parent-supplied
`644c052ad930a2b9d132f3ff38d2c85c2d9b76fcc7e9a53423fc0adbee80dbe5`.
This increment does **not** repeat the previous5,108-input/full build-custody review
or claim a newly reconstructed aggregate using the prior raw-digest algorithm.

`INPUTS.sha256` freezes91 selected public files:20 latest app inputs,40 actual
generated source/header files,19 callback/supervision/interface sources,12 reports/
contracts/seals. All91 match a fresh `sha256sum --check --quiet` (exit0). It is a
current static pin/preservation check, not behavioral validation, independent
acceptance or full-workspace integrity proof. Manifest SHA256:
`9fb6864139b63ecc3cf337a3e340167e774bbc6e90bcfadcd92032ca4e37084d`.

Only two assigned NEW directories were written: this documentation directory and
`apps/rtsp-auth/qc07f-retention-owner-candidate/` (README only). No source snapshots
were modified or regenerated. Existing SPDX, provenance and runtime licenses stay
unchanged; no copied code or global relicensing. Exact static method is in
`METHOD.md`; final output hashes are in `OUTPUTS.sha256` (excluding itself).

## Honest functional checklist — all NOT RUN / no queue

- [ ] Real device-free B/O/M launch, sole wait ownership and pre-effect boundary.
- [ ] One real verifier startup before all native description/backing setup, no
  second fork/init and no inherited native/guardian/provider descriptions/userdata.
- [ ] Actual stable source/config/bridge/hub/caller lifetime and owned backing.
- [ ] Successful guarded native callback acquisition/release/keyed normal retirement.
- [ ] Nonreturning uncertain-owner quarantine preserving real M and its FD table,
  native backing, stable objects and server stack; no outward success/error cleanup.
- [ ] External observed terminal identity record, distinct from mere self-report,
  with no credentials/payload/media or private payload digest.
- [ ] Positive independently admitted recovery/reset-domain disposition before any
  reuse; unexpected exit/OOM/lost owner remains diagnosed failure, not safe hold.
- [ ] Healthy pre-memory nonatomic returned-destination custody and separate
  UNKNOWN-watchdog policy; post-memory/failed-state shutdown remains unsupported.

No candidate/source/helper/script/generator/test/listener/app execution, no HOST/
ARM compilation/archive/link, no runtime/ordinary functional/adversarial/fuzz/
mutation tests, no target/UART/RPC/status/network/private IO or hardware observation.
Only static tools read public bytes. No Git/index/central-plan/AGENTS/P1 changes,
provider/model/safeguard changes, policy rejection/retry/reroute, query77 source/build
inspection or kernel/module/image build/install. Parent alone hardware. The last
reported held original U-Boot at12:12UTC is historical; receiver22b coverage expired,
catch consumed, no current Linux/runtime/recovery inherited. Both original radio
and reset-failure HOLDs remain untouched.

Singleton custody, actual FD adoption, keyed retirement, strict pre-memory return,
UNKNOWN policy, fixed10s/<2s/5s,128KiB/40MiB/CMA, native2MiB/HOST4MiB/loopback and
strict verifier pins remain intact. No backing exclusions/credits, memory-fit,
full-provider/app/G2/G3/G8 or native acceptance claim. Query77 offline completion
is separate; old76 is immutable and its station pairing unsupported for query77.

**One bounded handback:** parent review/disposition of this technical block and
pre-code contract. No automatic implementation, review worker, build, test or
native queue follows.
