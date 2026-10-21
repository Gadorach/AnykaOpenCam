# Profile handles ↔ network transactions — offline source integration

**HOST_PUBLICDUMMY/SOURCE_SYNTHETIC only; parent review pending.** Real reviewed
ppv2, ntxv2/network_transition, settings_store and access_policy sources are
read-only dependencies. No actual driver/FD/interface/AP/DHCP/route/wpa/root broker/
transport operations. No SEC/G6/G8 acceptance; T34/Fulhan deferred.

**V2 parent-review correction:** v1 could treat a failed metadata query as close
permission. Close now requires fresh successful public-metadata inspection,
matching owner-observed store revision, no pending/blocked/uncertain/reconciliation
state, and a matching published source-known terminal receipt. Error/HOLD retains
adapter/provider references, initialization and actual pin. Before edits, full
v1 source/API/docs/1,151-check reports/failure build/log evidence was preserved
byte-exact in `evidence/delegated/20261012-profile-transition/v1/`; fixture payloads
were not read/hashed/copied for preservation and originals remain untouched.

System RAM remains **40MiB `[0x81800000,0x84000000)`**, explicit CMA16MiB/default
compiled0. Mandatory native2MiB security budgets, buffers/CRC/deadlines/locks are
unchanged. HOST actual owned-region mlock is not full native isolation. Parent
alone owns the active kernel75 RAM restoration/UART/hardware.

## API and actual authorization

`profile_transition.h/.c` embed an actual ntx context and borrow real pp session
and policy. Config contains exactly two source-owner-minted nonsecret pp_handle
references: scope/owner/key epochs/profile ID/revision. Admission uses actual
pp_store_handle GCM/HMAC/strict-codec/live-authority checks, not cached flags.
Both old/proposed profiles must remain usable before/after fsync. Private class
key FD reads occur only after real pp HOST nonroot/cap/core/nodump/NNP/SYS_mlock
checks; Native pp still HOLD before key read.

`pt_open/seed_and_bind/prepare/receipt_ack/dispatch/continue/cancel/confirm/query`
use actual `camera_access_check`: **ADMIN, CONFIGURATOR, DEVICE_CONFIG** grant
on the canonical device. Exact owner principal snapshot and local lease/work are
required for continuation; another admin, viewer, expired/revoked/mutated snapshot
or stale lease cannot stop unrelated work. Tests' trusted PUBLICDUMMY issuer calls
real policy issue; its verified facts are **not wire authentication**. Principals,
leases and source IDs are local bindings, not serialized bearer/auth grants.

Malformed requests/unknown IDs and wrong scope/key/profile revision are refused.
Only opaque IDs enter public metadata; no SSID/PSK/plaintext/profile/secret hash
in metadata, session cookies, public callbacks, ARGV, images or logs. Query/replay
are scalar/read-only/idempotent; no new crypto payload or fresh fsync claim.

## Private source and lifetime

The INTERNAL trusted synthetic radio owner receives an immutable pointer to the
adapter's **actual locked**200-byte private copy, not the pp callback's temporary
buffer. `begin` may retain it (even on failure); `step` models only owned associate/
address continuation. Each requires policy/live old+new profiles and fresh clock/
exact work/deadline after crypto. Cancel/discard/revoke disables ordinary later
steps but **never wipes/frees** the source copy or live command.

`pt_source_abort/observe/retire/rollback/continue_rollback/pending` are trusted C
owner routes, MUST NOT be exposed through admin/public RPC based on an ID/flag.
Source-ID equality only detects wrong local binding. Revoke cleanup is allowed
only for the exact owned tuple and old-profile rollback, while actual policy is
still evaluated; it is not authenticated authority to apply another profile.

Actual `retired(work)` proof must establish matching command/private consumer
done before completion. Wrong/stale rollback proof retains logical work/copy.
Latest source observation is sampled at retirement too, so queued loss2 cannot
be ignored by old positive1; latest fresh proof or explicit failure is required.
Stale private namespace prevents restoration success. Held data is zeroized only
after actual source retirement plus core acceptance removes live work, including
when the subsequent metadata sync fails (publication stays uncertain). No DMA is
modeled; there is no DMA retirement or unknown-hardware free claim.

Contexts/providers/callback code are nonmovable, caller-owned and serialized
across all policy/source authority/files/effects. Fresh storage is zero-initialized;
no live reinit/copy/alias/reentry (open rejects a retained live context before memset);
borrowed provider storage remains valid until actual private callbacks are done.
Authority failure may wipe pp key state while the still-owned effect copy remains
until retirement. `pt_close` is BUSY/HOLD for held/live/unresolved metadata; no repair,
automatic unseal, restart rehydration, defaults or fallback/open AP. Exactly one
owned episode per adapter context; no replacement/GC/forget extension.

### Complete close /partial-open lifecycle contract

| Context state | Permitted lifecycle operation | Required actual evidence |
|---|---|---|
| Held copy/logical work exists | `pt_close` BUSY | Nothing is wiped/unlocked or declared retired |
| Active episode, query failure/missing receipt | `pt_close` HOLD | Error is NOT a terminal receipt; context/provider/code/lock retained |
| Active episode, pending/unpublished/future FINAL/blocked/uncertain/reconciliation | `pt_close` HOLD/BUSY | Actual source command retirement alone cannot clear disk-fence uncertainty |
| Active episode, normal terminal | `pt_close` success | Fresh inspect OK/current-valid/no-pending/active-index NONE, exact observed revision+operation stamp/scope; query success/runtime-bound/no reconciliation/no possible live work; CONFIRMED/RESTORED/FAILED/CANCELLED == published and volatile state; exact disk receipt matches all fields |
| Source-bound empty history after own initialization | `pt_close` success | Fresh valid matching zero-count store and actual ready STALE-on-unknown-token query, never reopened/blocked history |
| EMPTY-before-any-episode | Only explicit `pt_abandon_empty` | No metadata mutation attempted/adopted history/lease/source/private references; actual EMPTY inspection/settings OK/no current/pending/stamps |
| Metadata validation failed before backend initialization | Only explicit `pt_abandon_empty` | Original constructor INVALID +actual inspection INVALID, no adopted history or metadata mutation/reference |
| Clock-failed partial startup | `pt_close` HOLD; explicit abandon only if actually empty | Fresh EMPTY inspection, no adopted/attempted namespace/episode; no flags interpreted as native proof |
| Open adopts nonempty/foreign/corrupt/reconciliation history | Both close/abandon HOLD | Never discard/reset old namespace/history to obtain empty status |

`metadata_started` records a mutation **attempt**, even if sync failed before a
lease was returned. `pt_abandon_empty` cannot be called as active forced-forget or
history repair. Close/abandon read **public metadata only** through existing ntx/
settings APIs and pure public-ID allowlist; no private-profile/policy/source
callback, key read or unseal. They do not close borrowed provider/policy/dirfds.
Only after all checks succeed does actual SYS_munlock occur; failure preserves
initialized/locked state. Query/readback/corruption error and competing current
revision never silently refresh owner publication to close. Sticky core ambiguity
requires proper qualified reconciliation, not byte restoration or flag clearing.

## Policy/profile/clock/fence control table

| Boundary | Real controls | Failure/lifetime |
|---|---|---|
| Prepare | CONFIGURE → exact handles/IDs → actual old+new crypto/live authority → fresh policy → durable ntx prepare | No receipt/effect from malformed/unauthorized request |
| Delivered receipt ACK | Exact owner/lease +policy/private readiness → actual ACK fence | Lost/unacknowledged receipt gives no dispatch |
| Metadata dispatch | ntx pre/post-fsync real policy+both profiles → post-callback clock/state/tuple/deadline | No unfenced replacement/effect; references/fence retained |
| Private begin | Actual allocated core work → pp authenticated private borrow → fresh policy/clock/exact live tuple after crypto → locked copy → source begin | No begin if late/stale; not-issued allocated work still needs trusted retirement |
| Later owned step | Exact work/lease → policy+live profiles → fresh clock/deadline → synthetic step → checks again | No ordinary step after abort/revoke/expiry; held copy remains |
| Observe/retire | Source binding +exact generation/ordered observations; policy/private checks; actual consumer done; core v2 latest-proof rules | Wrong callback cannot cancel unrelated work; old positive never manufactures restore |
| Commit | Policy/old+new → ntx COMMIT_FENCE → latest source +policy/private rechecks → fresh core proof/clock → durable FINAL | No published confirm from scalar/typed flags/fsync failure |
| Safety cleanup | Actual policy evaluated, SOURCE_ONLY owning tuple may abort/rollback after revoke with real old/new readiness/time | Never grants frontend authority or releases unknown source state |
| Scope close | Actual fresh public inspection +ready/published matching terminal query/revision; no pending/uncertainty | Error/unknown preserves context/provider/pin; no fake logical retirement from command ACK alone |
| Partial constructor abandon | Explicit empty-before-episode path, original failure origin +actual no adopted metadata/reference | No active/history/reset/forget bypass |

Largest/capacity constraints unchanged: ntx896/1024/1872 bytes/eight receipts,
pp384/848/80/two retained profiles. No prune/reset/format/mount/firmware/footage
deletion; all identity/calibration/recovery/footage protections preserved.

## Fresh tests, exclusions and truthful teardown

```sh
python3 apps/network/profile-transition-candidate/qualify.py
```

**V2: 30 process-isolated scenarios,34 contexts /2,036 checks each optimized and
ASAN/UBSAN PASS**,34 retained private directories per mode. Actual hardened process +pp/crypto/source CAS +policy +POSIX
metadata store, not replacement backends. Full prepare→delivery ACK→private begin/
steps→real synthetic retirement→ordered association/address→durable confirm/final;
authenticated old-profile rollback; queued/delivered loss versus stale proof;
revocation/expiry/other-admin/lease/generation/request/epoch/revision/ID denials;
frontend loss with held copy; authority change/torn private files; fsync/clock/
crypto delay before begin; time consumed inside begin; final/retirement sync
uncertainty; zeroization and read-only results. C++→C PASS; ARM static link/readelf
PASS **NOT RUN**. Standalone finished core suites were not retaken.
New cases inject query RECONCILE/storage/corrupt/HOLD/missing receipt, actual clock/
readback-close/storage/torn-CRC errors, real competing public-store CAS revision,
unresolved rollback then real restored terminal, cancelled terminal, invalid/empty/
clock-failed partial open and adopted-history refusal. A safe Linux test wrapper
counts actual region SYS_munlock calls (exact used syscall arities, real kernel
forwarding), verifies none on failed close and exactly one on successful close;
borrowed key provider ownership/initialization remains, no secret reads at close.

Earlier errors retained: real policy rejected empty viewer grant (fixture fixed
to valid EVENTS-only synthetic sensor grant, no hardware claim); refreshed
rollback sequence retained loss facts (fixture fixed to explicit fresh positive,
not weakened ordering). Only final source/build pins qualify this increment.

Each scenario has a fresh isolated PUBLICDUMMY process. On HOLD it keeps the
adapter/provider/code/private-region pin and key session intact until process exit;
**no manual munlock, provider wipe, fake close, reinitialization or repaired flags**
to run another scenario. 21 retained-HOLD processes per mode exit with unresolved
metadata unchanged, not native reconciliation/physical resource retirement proof.
Successful terminal cases use real close. Native key unseal/entropy/backing-store/
radio/client and full2MiB isolation remain unqualified.

All fixture metadata/key/profile/plaintext/ciphertext/nonces/tags/checkpoints/
session payloads are excluded from content/hash inventories, including publicdummy
bytes. Only declared source/config/header/compiler/library/ELF/object/scalar-log
artifacts are hashed; no recursive scan. Separate fresh Mbed3.6.7 minimal config/
objects, NEVER cross-linked/reused with TLS/RTSP allocator objects. Source/licence/
tool/header/runtime closure is recorded. No operator PC private files/credentials,
ROM/photo/audio/footage, target/UART/proc/fuser/network/SD/SPI/GPIO/mount/install/
Git/publication effects. Completed reviewed dependencies remain unchanged.

Evidence: `evidence/delegated/20261012-profile-transition/v2/MANIFEST.json`.
Root MANIFEST/20-case1,151-check handoff are preserved historical v1, not current
close-lifetime acceptance.

**ONE next parent dependency:** source-owned device-key/bootstrap/live checkpoint
and owning-radio/latest-observer qualification on a measured dedicated physical
store before protected native transition (read-only readiness first). MMC75
disabled/probe refusal is not card absence/failure. SEC/G6/G8 stay open; main
parent-serialized video/UI restoration is independent.
