# Reviewed HTTPS service native launch — parent review HOLD

Complete narrow launcher + PC material generation + private HTTPS campaign.
No native execution, real operator-secret creation, UART/camera/guest access or
shared adoption is performed by this agent. Only explicit PUBLICDUMMY host tests
are authorized here. Actual75 root debug state is not secret-child READY proof.

## Fixed code / privilege / FD contract

`launch.c` uses exact pins generated from a successful canonical service build,
not a hardcoded historical service hash or runtime override:

- Service hash/size come from `--service-build` and its validated public manifest.
- ARM child62284B `8bb3d596065efb35338e2f28bedd60718eac8254d4f569e4412e77567f9c119a`.

Mbed TLS3.6.7 SHA256 + library constant-time comparison verify each opened source
and copy; no homemade crypto, secret-file hash, dynamic pin override or remotely
selected executable/PID/command. Native ELF must be32-bit LE ARM EABI5 softfloat,
ET_EXEC, no interpreter/dynamic segment/executable stack. This inspects ABI, not
proof it ran on ARM926. Host profile uses separately pinned reviewed x86 ELF.

Native CLI (public paths only):

```
auth-web-launch-arm.stripped /tmp/auth-web-qual01 /trusted/RAM/service0500 /trusted/RAM/child0500
```

The private scope must ALREADY be root-owned0700 **tmpfs**, containing root-owned
regular0600 single-link `cert.pem`, `key.pem`, `credentials.bin`, `config.bin`.
All absolute paths are securely walked without symlinks/..; root-owned sticky
`/tmp` is an allowed ancestor, never the private final directory. Input originals
are read-only and unchanged. The launcher makes exclusive exact-image copies,
root-owned0555 in separate sibling `/tmp/auth-web-qual01.public-code/` (0555).
This solves root-staged0500 executability after UID drop without global chmod,
new accounts, chowning existing code or exposing private scope contents.

The service executes the verified OPENED inode via execveat. Its child path is
the verified read-only sibling copy, not a client parameter. Native launch child
clears supplementary groups, sets all real/effective/saved UID/GID1000, zero
capability sets/core limits/dumpability, no-new-privileges and parent-death signal;
the unchanged service/secret child independently enforce their own hardening.
Root supervisor first retains only SETUID/SETGID/KILL for this fork, then only
CAP_KILL for its differing-UID **owned pidfd** cleanup. No IPC_LOCK/SYS_RESOURCE
capability bypass, global root/debug/UART policy or device-permission change.

Private FDs passed read-only at offset0:

| FD | Data |
|---|---|
|3|Matching provisioned P-256/SHA256 PEM cert, <=8192B|
|4|Private PEM key, <=4096B|
|5|112B: dummy56 + account56; schema1/count600000/salt16/verifier32|
|6|768B configv2; finite runtime120 or180s; response35s; trust_mode1 OOB|
|7|Anonymous SOCK_SEQPACKET created by the ROOT bootstrap itself|

The bootstrap remains the service's ACTUAL parent, so FD7 SO_PEERCRED creator PID/
UID match getppid/root correctly. It never execs a shell or intermediary owner.
Its forked child waits for release until the bootstrap acquires its mandatory
pidfd; unrelated FDs close before service exec. Empty exec environment/no secret
argv. The service's original credential-child pidfd/READY/INIT protocol is unchanged.

**Important immutable-binary limit:** child inherits service RLIMIT_MEMLOCK; the
reviewed images have no separate child-limit launch argument. Native conservatively
sets BOTH soft/hard limits to **2MiB**, never auto-raises/unlimits/relaxes locking.
An independent2MiB-child/4MiB-service contract would require a separately reviewed
service launch change; it cannot safely be manufactured after READY with prlimit.
Native2MiB service/child VM/locking feasibility is still a measured gate. Host
glibc exceeds2MiB; a separately compiled **PUBLICDUMMY host-only4MiB** profile
tests actual HTTPS without changing the ARM limit or strict image hardening.

No private key/password/material is read into bootstrap buffers: it delegates
already-checked read-only FDs; the unchanged nonroot service locks before reading
them and wipes its buffers. Only public code/hash/config bytes enter bootstrap
memory, locked/wiped. No private input or payload fingerprint is logged.

## Owned finite supervision, not recovery scaffolding

One service launch only, no asynchronous restart. Service runtime120/180s; owned
supervisor relative timer adds10s, then own pidfd SIGTERM,3s grace, own pidfd
SIGKILL if necessary, bounded actual wait receipts. It touches no camera DMA,
watchdog/guardian/FIFO/bridge/proof protocol. Existing parent recovery protection
is external and unchanged. Unknown CPU retirement remains failure, not success.
Subreaper is scoped to this bootstrap; after service exit it reaps only its own
adopted service lineage and reports when no children remain.

Only recognized scalar source lines are interpreted. Readiness is reported only
after the EXACT pinned service's post-TLS/provisioning/strict-child readiness
line—not a bootstrap fabricated READY or marker from an arbitrary file. Child
retirement is attributed to the pinned service's actual cp_wait receipt; service
exit is the bootstrap's own wait4 receipt. No match counts/raw child output,
password/Authorization/cookie/CSRF/key/certificate/material hashes forwarded.

The trusted owner may signal THIS bootstrap through its own pidfd SIGUSR1 once.
That sends only existing FD7 op1/fixed-account generation+1 revocation and checks
the exact8B ACK. There is no HTTP/remote PID/command selector. SIGTERM similarly
cleans up only its owned service. Root debug signal policy elsewhere is untouched.

## Parent-only private PC provisioning (DO NOT execute as this agent)

`material-gen-PC-PRIVATE-parent-only` is a HOST-only executable using pinned full
Mbed TLS3.6.7 library PBKDF2-HMAC-SHA256 **600000** for both account and dummy.
It requires scoped core0/nodump/mlock; qualified GRND_NONBLOCK with bounded
short/EINTR handling supplies independent random account password, dummy secret
and salts. Library base64 encodes the random operator password. It creates
exclusive0600 `operator.password` and `credentials.bin`; values/derived output/
payload hashes never stdout/argv/env/source/image. Local secret buffers are
library-zeroized. No password is displayed by default or reused from guest/WLAN.

Parent consciously invokes after review, on OWN PC, fresh directory only:

```
python3 apps/auth-web/native-launch/provision_pc.py --create-private \
  --material-tool /owned/build/material-gen-PC-PRIVATE-parent-only \
  --directory /owned/private/qc07f-qual-FRESH \
  --address 192.168.1.73 --port 18443 --runtime 180 \
  --boot-generation NEW_NONZERO_BOOT --child-generation NEW_NONZERO_CHILD \
  --device-id PARENT_CORE_DEVICE --sensor-id PARENT_CORE_SENSOR \
  --profile-id PARENT_CORE_PROFILE --account qualification-operator
```

The uppercase generation placeholders are decimal PUBLIC metadata selected by
the parent, not secrets or fabricated native-runtime evidence; replace with the
actual intended trusted epoch. CORE IDs must be actual selected graph IDs or
explicitly labelled qualification identities, not evaluation-board assumptions.

The recipe uses maintained local OpenSSL P-256/self-signed SHA256 certificate,
SAN192.168.1.73, two-day PC-dated validity; private key writes only0600 file,
tool stdout/stderr never forwarded. It verifies material-tool class/result, creates
768B configv2/35HTTP/40CPU/180runtime. Not permanent onboarding or public-CA trust.
Authenticate the certificate OOB/pinned; PC/client UTC validity, camera cold
entropy, durable key store/rotation and target permissions remain explicit gates.

**All private target transfer is exclusively the PARENT'S EXISTING AUDITED
send_private / echo-off / bounded RX-redaction fallback.** This package supplies
no UART/network transfer routine, secret shell heredoc/hex/argv/env or substituted
privacy transport. Transfer only the exact four server files to the new0700 RAM
scope,0600 via that current parent-owned private path. Never transfer/display
`operator.password`, never log/hash private file contents, never use PUBLICDUMMY
fixtures on target. Public launcher/service/child images may use the existing
parent public staging path, with exact public SHA/image evidence only.

## Future actual75 campaign: parent only after READY gate

1. Parent reviews source/ELF pins, confirms independently observed strict native
   credential READY (UID/caps/core/mlock/seccomp/thread/FD/time64/pidfd/exec ABI),
   actual trusted ownership/finite external recovery protection and fresh IP/MTU.
   Root debug unfiltered is expected, not secret-child readiness.
2. Parent privately provisions unique server files as above; executes launcher
   with current exact trusted staged public source paths. No keys/PW/hash in UART
   command or ordinary RX logs. Native2MiB locking failure is a concrete stop,
   never automatic limit increase. Observe pinned-service readiness before clients.
3. Parent PC runs (password only read from private file, never CLI/env):

```
python3 apps/auth-web/native-launch/private_client.py --private-directory /owned/private/qc07f-qual-FRESH --after-owner-revoke
```

This performs real login/session/protected status/capabilities/media501/logout/
replay/Range401 plus one full600k discarded job and a subsequent actual login.
Three real KDF jobs: target prior idle24.804768586s each;40s CPU bound remains.
140s PC campaign budget within180s service; no lowered count. On its NONSECRET
owner-revoke-window marker, parent sends SIGUSR1 through its OWN bootstrap pidfd
and observes private FD7 ACK/old-cookie401. Stop bootstrap through owned pidfd
SIGTERM, preserve scalar service/child/lineage exit receipts and resource metadata.
No stdout client cookies/CSRF/match values. Python client zeroes its password
bytearray but does not claim audit of every interpreter/TLS temporary/kernel copy.
4. No actual browser media/RTSP auth/FPS/product acceptance from this campaign.
   Media stays501, RTSP independent NOAUTH LAB unchanged; all product flags false,
   later camera ports deferredG8. Native memory/headroom/combined-media load remain
   unqualified, not inferred from host/library/static file sizes.

## Reproduction

From the canonical project root, using only installed read-only toolchain:

```
export AUTH_WEB_ARM_TOOLCHAIN=/path/to/existing/armv5-eabi--musl--stable-2025.08-1
python3 apps/auth-web/service/build.py runSERVICE
python3 apps/auth-web/native-launch/build.py runLAUNCH --service-build local/build/auth-web/service/runSERVICE
python3 apps/auth-web/native-launch/test_host.py runLAUNCH hostTEST
python3 apps/auth-web/native-launch/finalize.py runLAUNCH hostTEST handoffNEW
```

The service build compiles full HOST and ARM Mbed libraries from canonical
vendored source once. The launcher revalidates source/config/ABI/compiler/header/
runtime/library/image hashes and uses ONLY those exact fresh local outputs.
No ignored historical source/archive/prebuilt library input or SDK auto-build.
`--toolchain` may replace the environment variable. `--output-root` may select
another directory under project `local/`; default launcher outputs are
`local/build/auth-web/native-launch/runLAUNCH/`. Test/finalizer run arguments may
also be absolute or workspace-relative local paths. No private material is
created by building the PRIVATE PC executable; only parent consciously invokes it.
All names are fresh-only; no overwrites. Tests include real
reviewed Mongoose/Mbed HTTPS/private owner/own process exits when4MiB host limit
permits, explicit2MiB refusal/no auto-raise, altered ELF pin/config refusal and
source0500/copied0555/private0700/0600 separation. Root UID/groups/tmpfs/native
execution proof is absent; host unprivileged fixture does not pretend otherwise.
Full source/config/tool/header/library/runtime/license closure is retained.
