# Private RTSP descriptor custody — parent closeout

2026-10-23; SEC-01/03/05/11, STREAM-07. Accept **narrow conditional SOURCE/API/
custody sufficiency** after independent ordinary review. No correction required
in this bounded implementation. It is not an installed provisioner or a complete
separate-process credential/IPC pipeline, and it does not complete SEC/G3/G8.

Actual constructor → exclusive fixed-name1536-byte regular file → one writer
close → validated readonly CLOEXEC FD → explicit unused-config slot move is
implemented. Partial writes/fsync/metadata failures preserve the file; close
uncertainty is sticky after clearing the writer slot, with no numeric retry.
Readonly ownership is stored before fallible checks; failed move retains it.
No implicit cleanup, credential readback/hash/log or live config change occurs.
Scratch is crypto-wiped; caller still owns borrowed password copies.

Native hardening performs actual root/saved-ID/group checks, core-limit0,
nondumpable/NNP and successful current/future mlock, with terminal partial-failure
semantics. It is NOT executed here and cannot prove separate-process placement,
authenticated ingress, capability/FD/environment policy or authorization. The
HOST PUBLICDUMMY build has separate symbols/class and no runtime native fallback.

## Verified evidence, not repeated execution

- C SHA256 `cd3e08600841502f1373c30ea80b43c736aa07e705bed7ec83f20f19d5f5903c`.
- Header SHA256 `4ad3136dfcf4e8fff5b3a26ab794279a96ee908466bfa547819dd87546fefa44`.
- Independent review SHA256 `f2ab1b0475c9074120b07285a6304e5b567540bcd70184ec67c58bba46528ecd`.
- Build report SHA256 `5c5f4ce8b8a8cb82d3fee8a5faf8b93f63ae0651f2917c35117bddb02e3be1b4`.

Parent read the complete C/header/harness/recipe/contract/handback and review.
Freshly matched32 selected source/context pins,15 delivery pins,1363 consumed
dependency records,75 artifacts and37 command logs. Counts overlap. All24 C
compilations and37 commands succeeded; real original constructor/crypto/validators
were compiled HOST and retained ARMv5TE, not stubbed. Original sources remain
unchanged. ARM REL/stack records are not native ABI/whole-stack/memory admission.

ONE author HOST PUBLICDUMMY normal-path invocation passed: real exclusive write,
known writer close, readonly metadata, explicit move, scratch zeroization and
explicit test-only cleanup. Parent/reviewer did not rerun it. No negative/fault/
adversarial coverage is claimed. No actual private/operator credential was used,
read, generated, persisted or hashed. Native hardening/launch/media remain NOT RUN.

## Actual remaining integration

The C-slot move does not cross a process boundary or prove a secret-free media
address space. Separate authenticated ingress/launch/IPC, inherited-FD/capability/
environment hygiene, private linked-file retirement/startup-failure disposition,
durable generations/storage, rotation/reset and real bootstrap/main remain work.
Writer exclusion and linked-inode stability must continue through actual verifier
consumption/retirement; metadata/fsync do not prove these or crash-safe durability.
External media owner/recovery,128KiB-compliant backing,40MiB/CMA/load and native
authenticated PLAY/decode/STOP remain unqualified. Next feature work must connect
actual separate provisioner/bootstrap lifetimes, not call the current slot API IPC.

No source worker/reviewer remains active. No hardware was operated in this lane;
parent's separate passive23b0RX/0TX observation supplies no runtime admission.
Both original HOLDs remain. Curated local Git may retain reviewed source/notices/
public metadata, never generated binaries/private material; ordinary index stays
unchanged and no push/publication follows.
The curated Git whitespace check reports only the37 preserved trailing spaces
from the recipe's exact argv logger in COMMANDS.txt. Keep that sealed command
record byte-identical; all other selected paths pass the whitespace check.
