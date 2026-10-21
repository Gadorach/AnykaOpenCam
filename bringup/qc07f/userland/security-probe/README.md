# SEC-02a: bounded entropy/resource primitives

Offline fixture implementation, 2026-10-11. **No authentication service or
network-security acceptance. No target execution occurred.** Parent owns all
hardware and the active kernel70 upload. These files do not access UART, camera,
network settings, key/config files, MMIO or flash. Do not add them to a product
listener or claim acceptance without the remaining integration/target gates.

## Corrected budget and frozen qualification limits

Parent correction: usable SystemRAM `[0x81800000,0x84000000)` is **40 MiB**;
**16 MiB CMA is within it**, leaving **24 MiB nominal non-CMA**, not free RAM.
The old 48/32-MiB premise is retained as historical error evidence, not used here.

| Enforced primitive bound | Limit |
|---|---:|
| Security allocator payload live total | <=1 MiB; caller may choose less |
| One allocation | <=128 KiB; caller may choose less |
| Simultaneous tracked allocations | 256 |
| Admitted HTTPS peers | 2 |
| In-progress TLS handshakes | 1 |
| Queued send bytes, including application framing | 16 KiB/peer, 32 KiB total |
| Accept-to-TLS-complete deadline | 5000 monotonic ms; never extended on progress |
| One RNG output | 1..4096 bytes |
| OS RNG call / attempts per output | <=256 bytes / <=32 calls |

The <=2-MiB incremental security-service userspace peak and <=64-KiB KDF
worker/queue reservation remain **target qualification ceilings**, not measured
use. KDF storage comes from the same allocator budget, not another MiB. Mapped
code, BSS, these fixed tracking tables, thread stacks, libc allocator overhead,
kernel socket memory, media/recording and rootfs are not counted as allocator
payload. No borrowing from CMA or assuming the 24 MiB is available. Per-peer
128-KiB heap reservation is a sizing proposal, **not** allocator attribution.

## RNG and explicit startup contract

`security_rng.c` only adapts Linux `getrandom`: cryptography remains in Linux
and the pinned Mbed TLS library. There is no local PRNG or time/MAC/frame/random
file fallback. Every call uses `GRND_NONBLOCK`. EAGAIN means not ready, immediately;
only EINTR retries, and short reads accumulate within the shared 32-call limit.
Zero/overlong/invalid syscall results and other errors fail closed. Any failure
latches the context; all valid output spans are wiped in full, including partially
filled buffers. A later call after failure makes no syscall and yields no bytes.

The injectable syscall seam permits fault tests without reading host entropy.
Its buffer must be a valid writable span, and the seam itself must honor that
span. NULL, zero or >4096-byte requests are caller-contract errors, not permission
to wipe an unbounded or invalid pointer. Never pass unchecked remote sizes.
`security_rng_zero` uses volatile stores, including success-output retirement.
No bytes or entropy-payload hashes are logged.

`security_rng_start` requires a successful 32-byte readiness read **and a checked
PSA init callback result** before `security_rng_can_serve()` is true. A failing
init or RNG failure during an init that erroneously returns success prevents
service readiness. The callback is injectable solely for fixtures; production
uses `security_rng_library_start()` and actual `psa_crypto_init()`. Do not replace
that with a success stub. Bind the allocator before PSA or any library allocation.
Before starting listeners, call the explicit startup function and check its return
and `security_rng_can_serve()`. Mongoose's own unchecked PSA init call is not proof.

The optional `SECURITY_WITH_MBEDTLS=1` adapter exports the public
`mbedtls_psa_external_get_random()` callback and Mongoose's `bool mg_random()`.
Failed callbacks return failure, publish zero output length where available, wipe
valid output and leave a **persistent fatal service gate**. Some Mongoose helpers
ignore `mg_random()`'s bool result: therefore return values alone are insufficient.
Never use its dashboard, `mg_random_str()` session generation or fallback RNG.

**Mongoose `mg_tls_init()` returns void and can free TLS state on failure without
making the connection TLS-enabled. The owner must not then serve plain HTTP.**
After that call require the RNG/memory gates, non-NULL `connection.tls`,
`connection.is_tls`, `connection.is_tls_hs` and `!connection.is_closing`; otherwise
close the peer and retire admission without processing a request. On the real
TLS-complete event require `is_tls && !is_tls_hs` and the same gates before
`security_admission_handshake_finish(..., 1)`. An HTTP event is not a TLS-complete
event. No request handler or send may run before completion.

Check gates before and after library operations/event processing. A latched RNG
failure or allocator-integrity failure must disable listeners, stop application
sends, discard pending output and close **all** actual peers through the owner's
teardown path. `security_admission_halt()` retires logical state; it does not own
or close sockets. Do not merely change to HTTP, retry with weak entropy, continue
an existing authenticated session, or wait for another client to notice.

## Allocator and admission ownership

Caller-owned contexts have stable addresses and external serialization. No live
reinitialization, copy/move, concurrent mutation, reentrant seam, or library hook
rebinding is supported. The library callback signatures lack a context argument,
so optional binding is deliberately **once per process**; contexts outlive all
library calls. Threads require an independently qualified locking/lifecycle
adapter; these are not thread-safe primitives.

The allocator checks multiplication overflow, total and single-allocation caps
before calling the injected malloc seam; it zeroes successful calloc output.
It accounts only successfully allocated live spans and retains byte/object high
water marks. Backend OOM/quota rejection leaves existing allocations unchanged;
there is no pressure reclamation. All owned frees wipe before deallocation. A
fixed ownership table rejects unknown/already-freed pointers without reading
freed memory or passing them to libc. Duplicate live pointers returned by a
broken allocation seam are rejected without zeroing/freeing the original object.
Integrity failures latch fatal; valid owned cleanup remains permitted.

Like the library's `free(void *)` contract, address-only ownership cannot
distinguish a stale pointer from a **new live allocation at the same reused
address**. Do not claim arbitrary UAF/ABA detection, corruption resistance or
safe hostile free pointers beyond membership checks. No tests dereference freed
memory. Accounting fields/slots are read-only to callers.

`security_memory_bind_library()` installs public Mbed TLS platform calloc/free
hooks and supplies Mongoose `mg_calloc/mg_free` under its custom-calloc define.
Bare malloc/strdup in optional upstream facilities, libc buffers and kernel
sockets are outside that hook. Do not enable unreviewed upload/filesystem/OTA/
built-in TCP-IP facilities or assume all process allocations are charged.

Admission tokens are owner/id/slot handles, not credentials or RTSP sessions.
Reject foreign/stale handles; IDs do not wrap into reusable live identities.
`open` requires successful startup and a sound allocator. `handshake_begin`
reserves the one handshake; unsuccessful/expired completion retires the peer.
Clock regression halts admission. `expire` retires all incomplete peers at their
original deadline. Each send must reserve before calling an upstream enqueue/
format/response API; release only actual drained bytes, and retire on close.
Overflow/over-release rejection does not change counters. Stalled output is not
allowed to accumulate a complete frame/file. Normal producer/control code is
not called by these primitives.

Limits are meaningful **only when integrated around every relevant owner path**.
Mongoose may allocate the initial accepted connection before its callback;
admission does not itself cap kernel FDs or remove that transient allocation.
Reject peers before TLS allocation, close rejected sockets promptly, sweep
deadlines, and correlate actual send-buffer growth/drain and close with accounting.
The public-library fixtures prove startup/adapter failure handling, not a running
server, timer scheduling, client fairness or complete network-limit enforcement.

## Offline fixtures and reproducible build

Dedicated tests: `framework/tests/security_rng_test.c` and
`framework/tests/security_memory_test.c`. The existing framework runner is
unchanged. Host ASan/UBSan checks cover full/short reads, EINTR exhaustion, EAGAIN,
other syscall errors, invalid seam results, full output wiping, failure latching,
PSA-init failure, allocator quotas/overflow/OOM/slot exhaustion/high-water,
foreign/double free, duplicate allocator pointers, two-peer/one-handshake limits,
send overflow/over-release, stale tokens, pre-TLS timeout and clock regression.
Additional host fixtures use the actual pinned Mbed TLS/Mongoose public APIs,
with fake entropy and allocation failure: PSA random failure, void TLS-init
allocation/certificate failure, and fatal admission retirement. No sockets or
real passwords/certificates are used.

Build recipe and attempt manifests:

`work/lightbulb-camera/foundation-20261011T041148Z/analysis/security-probe01/`

From the workspace, choose a **new unused attempt name** (no overwrites):

```sh
python3 work/lightbulb-camera/foundation-20261011T041148Z/analysis/security-probe01/build.py run03
```

The script verifies historical pinned source/compiler hashes, snapshots only the
owned new source files, sets TMPDIR/TMP/TEMP to a fresh workspace-private attempt
directory, records every command/compiler/version/input/output hash, uses
GNU11/strict warnings for new code, builds ASan/UBSan host fixtures and statically
links ARMv5TE/soft-float/musl ELF files. It never executes ARM output. The inherited
TLS1.2/1.3 config adds only public Mbed TLS platform-allocation hooks in a **new**
header; original pins/configs/archives/binaries/manifests remain untouched.
`run01`'s strict C11/POSIX-feature compile failure is retained, not overwritten.

## Parent's next actual experiments (not performed here)

After the active upload and fresh parent-owned runtime/recovery/ownership/resource
preflight, review hashes and transfer **only the exact public entropy probe** to
an owned volatile location. Run once under an external 5-second deadline, first
idle, then under the established media load. Probe output is one scalar-only line:
ready/status/bytes/calls/errno, timing/resource metadata, corrected memory constants
and `psa_ready=0`. It always wipes the 32-byte read before printing. Exit0 means
OS readiness observed, exit2 EAGAIN, exit3 other/error/measurement failure. It
does not initialize PSA, generate keys/sessions, contact any endpoint, open files,
measure entropy quality, or prove cold-boot readiness. A separate earliest-boot
run must be scheduled by the parent, not by rebooting from this scope.

Then run the **unchanged earlier** `security-stack01/build/kdf-cost-armv5.stripped`
public-input fixture under the existing proposed 35-second external deadline,
idle then under qualified media load. Keep exact executable/config hashes,
elapsed/CPU/memory observations, load context, deadlines and failures. Do not
select a password work factor from link sizes or extrapolation alone. The new
`security-library-link-armv5` is **never-run link evidence**, not the readiness
probe or a deployable HTTPS service. Target allocator/TLS handshake/throughput,
clock/trust/persistence, authentication and NVR acceptance remain open.
