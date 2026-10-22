# Device-free prestarted backend — contract before implementation

2026-10-22. RP-08 prerequisite only; CORE-03/04, STREAM-04/07. SOURCE/API candidate,
not media retention, bootstrap/recovery, backing or native/product acceptance.

## Chosen scope and original boundaries

Derive only from frozen `apps/rtsp-auth/isolated-server-host-tail/`. Keep actual
candidate backend validation, RNG, routing/material/image predicates, rv_start
fork/pidfd/release/READY15/provision ACK, policy exchanges and verifier stop/wait.
Split the real server entry, not a dummy launcher/helper. No legacy monolithic
serve entry is exported by this new sibling; frozen originals stay untouched.
No listener API work: original internal loopback socket/bind/listen path remains.
The parallel owned-listener sibling is not inspected or composed here.

## Actor, storage and trusted order

A single caller-owned `candidate_prestarted` object, explicitly initialized with
the source macro, embeds ONE actual backend and four stable actual auth peer
records. A process-local one-shot claim and self-address reject ordinary repeated
start/copy mistakes before legacy init's memset. This is public C source lifecycle,
not attacker-memory protection, a binary plugin ABI or native authorization.
Caller may neither edit/copy/move/reset the object nor call private raw init/
destroy routines. All calls/callbacks are serialized, nonreentrant, stable and
disjoint, without concurrent FD changes or signal reentry.

Actual start must run before ALL native/provider/watchdog/guardian descriptions,
media leases/backing/session and provider userdata exist. All other fork/exec
must likewise precede those owners; CLOEXEC is no fork-alias proof. This ordering
is a trusted bootstrap requirement, not an fstat/PID/ready-Boolean check. No native
setup is performed or authorized by this API.

The public config object/identity, graph/routing, context address and callback
addresses remain immutable and nonmoving from start through finish/HOLD. Device-
free public startup state/RNG/auth image/material are real original inputs. After
the final fork, native userdata may be prepared *within that existing stable
context*, without changing the config, graph or callback bindings, fabricating
readiness or bypassing the server-bound source guard. Such preparation/lifetime
is uninstalled/unproved here. Preopened image/material stay the caller's; startup
retains only its real verifier-owned client/child/IPC relationship, not new aliases
of native descriptors. No credentials/payload or their digests enter reports.

## State transitions and failed start

EMPTY -> STARTING (before real init) -> READY or START_FAILED or HOLD. Start is
one attempt per object/process, including invalid config/clock/start failures.
It calls the real validated backend init/rv_start exactly once. Success requires
actual READY15/profile and current owned-child observation, not a passed ready
callback or copied rv_client. An additive scoped observer may inspect the existing
owned wait relationship; no signals/protocol/child policy or permission are added.

Record the actual rv_start result, so RV_UNCERTAIN from its own failed-start stop
attempt is not lost in backend CAMERA_INVALID. On that outcome retain the real
client/backend/config/process namespace, enter terminal HOLD and invoke the real
mandatory retain_verifier sink; do not repeat stop through finish. Failed start
with known/no child retirement stays START_FAILED until one explicit finish. Any
unretired child pointer always lives in the stable caller object, never a returning
startup stack temporary. A readiness observer failure does not mean retired.

## Actual serve and peer/stack lifetimes

READY -> SERVING -> SERVED or HOLD. Serve attempt is consumed even on option or
entry refusal; no second serve, startup, reset or retry. The serve entry uses only
the embedded actual backend and its original config; never copy/move/memset/init
it and never call rv_start/fork/exec or implicit backend finish. Observe real child
and READY/profile and perform actual backend polling; every protected effect
retains the existing exact live policy CHECKs, limits and source guard.

The existing server's network clients/hub/capture gate remain serve-stack objects
until actual return. Each client's auth pointer refers FROM FIRST OPEN to the
corresponding stable embedded peer record, not a stack peer copied out on failure.
Thus backend peers/pending_peer never refer to expired serve-stack storage, even
when pending crypto prevents close from retiring its record. Existing bound guard
is unbound before any returning path after binding; no asynchronous callback may
retain gate/client/hub stack addresses. Source-retention callbacks retain their
original copy-only contracts; a returned source pointer is not external retention.

After actual serve return, failed serve/backend/pending/nonquiescent source state
is HOLD, not normal successful shutdown. Caller keeps the full owner/config and
source/backing/process lifetimes. No framework failed-NEXT/internal-retirement
contract is changed. Native failed NEXT incompatibility remains BLOCKED: the API
does not make its cleanup safe or supply the required nonreturning media hold.

## Explicit finish, destruction, HOLD and terminal status

Only READY (never served), START_FAILED, or clean SERVED may enter FINISHING once.
Caller independently retains/adjudicates native/provider owners; finish controls
ONLY the actual verifier child and is never native free/close/exit permission.
No finish during serve/callback, no hidden serve destruction and no HOLD retry.
Original rv_stop then owned retirement/rv_destroy checks remain: second rv_stop
inside destroy is a no-effect already-retired observation, not a second signal
attempt. Normal initialized success requires actual BYE wipe plus own exit0.

Known retirement frees/wipes only the actual backend/client per the original
destructor and leaves the outer owner terminal FINISHED, retaining its result/
history. Unknown retirement invokes mandatory retain_verifier, keeps the actual
client/backend/config/namespace alive and leaves terminal HOLD. Repeated finish
refuses without another destructor/stop. No stack-only owner, numeric-PID fallback,
respawn, fake retirement or status-code-as-proof. Regular native shutdown with
uncertain owners is unsupported; unexpected process/OOM exit retains no FDs and
must be diagnosed, not called safe. pidfd observation cannot prevent exit/DMA.

## Evidence and open gates

HOST/ARM real changed-unit compilation/archive/REL only; no executable, test,
generator payload, runtime, network/listener, private IO or hardware operation.
Retain source before/after, pins, licenses, exact toolchain/dependency closure and
all failures. Preserve loopback, strict verifier pin/native2MiB/HOST4MiB, original
auth/source limits and128KiB/40MiB/CMA constraints. No source-native glue/provider
adoption, allocator/backing, process supervisor, media RPC/reset/feed/END/ACK.
This sibling and the owned-listener sibling require later explicit composition/
review; neither is a composite. External B/O/M/R lifetime, media retention,
backing, provisioning, native timing/driver pairing and product gates remain open.
