# Clean-exec provisioner — independent ordinary source review

2026-10-24 · SEC-01/03/11, RP-08; SEC-05 dependency.

**Narrow conditional SOURCE/API/order/custody/lifetime/security-boundary sufficiency.**
No blocking ordinary source defect identified within the stated trusted-caller
contract. This covers the inseparable real worker entry, pointer-free public codec
and separately typed exec/owned-wait/bind owner. It is **NOT parent adoption,
worker-image eligibility, runtime/native/cap acceptance, API5 integration, product
main or product GO**. Parent disposition must precede use, correction or follow-on.

## Scope and exact evidence

Read all ten files under `apps/rtsp-auth/rtsp-provisioner-exec-candidate/`, including
complete C/headers, ABI constants, recipe and notice; complete author CONTRACT,
HANDBACK, BUILD_RESULT, feasibility and design draft. Compared actual old process,
private handoff, startup constructor, config/prestarted/graph/routes/rate/Digest/
Mbed TLS/zeroizer APIs and four prior parent dispositions. Actual verifier adapter,
verifier isolation and authority launch/credential-process/parent-sandbox were
comparison inputs only, not a demand to apply their incompatible nonroot filters.
Additional comparison/input pins are in [REVIEW_INPUTS.sha256](REVIEW_INPUTS.sha256).

Author directory: `docs/qualification-plans/rtsp-provisioner-exec-20261024/`.
Its unchanged `DELIVERY.sha256` is
`87e64883489a30a0e88266b12a68414b2a0425828686d397ae266d891e044721`.
All270 delivery records matched; exact app/docs membership is those270 files plus
that manifest,271 originals total. Important source SHA256 pins:

| Candidate file | SHA256 |
|---|---|
| `exec_owner.c` | `dd39a64f055bfea4a062e5c946d431f1c944db356852d7e962e386f95b6368f0` |
| `launch_security.c` | `0782ca800bd6802c5d6de48136e89f94cbd3dbc3eec3cadc42863f4ad7a4f989` |
| `public_codec.c` | `0c997e204b71601dbc2c586ffdb55e11023cdd45ebaeb308975d08d2ebf0e2af` |
| `worker_entry.c` | `462130a8c62f1b7326aa3280abbd3ad129640888641f55aa1d6b4a69828e41b9` |
| `compile_recipe.sh` | `7651e21619927871bc00012e2d817d8331905ed160c65ec71dc7d591c94d75fc` |

Read-only public hash/diff/log/raw-ELF/archive inspection independently checked:

*127 commands/success names/exact corresponding logs;32 actual C compilations
 (16 HOST/16 ARM), four rooted REL links and one static ARM worker link, cited
 only. No build02/FAILED marker; all compiler/link logs and16 source diffs empty.
*16 consumed C copies equal their actual sources;22 overlapping selected-input
 pins and17 frozen before/after records match.1957 dependency records match live
 public inputs, `.d` multiset and exact recorded hash-command argv order;106
 artifacts, ten compiler-component/tool records, six metadata-tool records and
 nine supplementary static-input pins match. Counts overlap;603 distinct lexical
 paths were hashed in that verification, not a complete compiler closure.
*32 production object headers, all four raw REL symbol/relocation tables against
 saved tables, both ABI data sections, all five maps and final raw ELF metadata.
 Static map has14 selected project objects +five CRT objects +three archives;
 selected archive metadata includes10 libgcc and92 libc bodies (91 libc names,
 including duplicate-named members). No extraction or object execution occurred.

## Actual source findings

**Parent admission/launch (`exec_owner.c:58–112`).** One process-local attempt and
self-stable owner are latched; canonical ingress moves before clock/preflight.
Actual root IDs/groups0, no inherited seccomp, fresh real unused owner/default
SIGCHLD/sole reaper class, original public configuration, FIFO/directory and
dedicated borrowed worker-image identity precede launch. `launch_security.c:68–115`
checks readonly/CLOEXEC root nonwritable/non-setid nlink1 regular image, exact bytes,
full public SHA256, ELF32 ARM ET_EXEC/EABI5/softfloat, bounded nonoverlapping LOAD
extents/entry, no INTERP/DYNAMIC/RWX/executable stack and stable metadata. These are
structural/hash checks, not worker-role/ancestor provenance or instruction proof.
The independent worker policy is supplied externally; no default/self-pin/verifier
image substitution. Borrowed image/directory survive failure.

Actual private nonblocking seqpacket endpoints and each of four collision-free
`F_DUPFD_CLOEXEC >=16` results are recorded before later failures. Config/routes/
graph pointer structure/unused-owner/image policy snapshots plus full graph value
serialization are retained. Actual ONE fork result is recorded as own unreaped
child/possible file before postfork close/send. Parent ingress, child socket and
stage slots clear before each ONE close; separate sticky uncertainty bits prohibit
numeric retry. Only known closes/current checks permit ONE whole public send.
Parent neither reads ingress credentials nor material payload.

**Child exec transition (`launch_security.c:144–205`).** Real IDs/groups0 and
no inherited seccomp; adequate inherited memlock/AS allowance; only reductions to
2MiB memlock/AS and core0. Nondump/NNP, ambient clear, locked NOROOT/no-setuid-fixup/
no-ambient-raise/KEEP_CAPS-off precede dropping every supported bounding cap and
capset ZERO. Verification checks real outcomes before exec and again afterwards.
Dropping CAP_SETPCAP from the bounding set does not itself remove the current
effective setup capability before the remaining drops/capset. Needed setup
authority and supported APIs are prerequisites, not implied by UID0. Unknown/new
caps or incompatible locked states refuse; no retained IPC_LOCK/SYS_RESOURCE/KILL
or root-regain fallback. See the concrete unsupported ARM pairing below.

The four staged sources map to3 control/4 ingress/5 directory/6 image; only6 is
CLOEXEC. Checked close_range>=7 and stdio closure/absence, actual getppid and
security revalidation precede ONE direct SYS_execveat/AT_EMPTY_PATH/fixed argv/
empty env. Failure exits nonzero, never parent logic/PATH/interpreter/fexecve retry.

**Codec/public equivalence (`public_codec.c:17–162`).** Explicit LE1374 prefix +
144/128/204/72 records, counts<=16/16/8/8, max7934<=8192. Exact lengths/reserved0,
bounded enums, canonical NUL/zero tails, nonzero generations, identifiers, full
graph bindings/selected720p-H264 profile, actual normalized routes and all rates
are validated. Original deadline-start is1..10000ms; no reset budget. No pointer,
stat/size_t/padding/native C ABI, function/context, verifier FD or secret/credential
length/HA1/hash serialization. Decoder builds actual local arrays/graph/config,
ABI3/RV_ABI1/native/logical projection, image/material=-1 and NULL callbacks.
That projection is not a verified image. Actual constructor consumes those fields;
actual local CANDIDATE_PRESTARTED_INIT permits legitimate local handoff move,
without pretending to be the parent's separately checked owner. Explicit1392-byte
semantic reply (including six zero tail bytes) is not an old wire-ABI claim or
permission token. Full original graph/request re-encoding is checked at receive
AND bind, besides original pointer/callback/context/owner identity snapshots.

**Worker before/after secrets (`worker_entry.c:75–120`).** Real main validates
fixed args/empty env, post-exec caps/IDs/limits/NNP/securebits/no inherited seccomp,
stdio/image6 absence and CLOEXEC3..5. Tail absence depends on the actual trusted
pre-exec close_range, not a new arbitrary-FD scanner. Actual socket peer is the
socketpair CREATOR parent, not the child PID; getppid and bound role inodes are
checked. Bounded recvmsg records ALL installed unexpected rights before any
ancillary/count/truncation/length refusal, then fails without credential reads.
Actual public validation, inherited-role checks, post-exec real hardener/mlock/
nondump, repeated security verification and post-cost clock precede FIRST secret
byte. Pre-exec mlock/dump state is not assumed to survive exec.

Original private16B framing,1..64 username/realm,1..128 password, exact EOF and
shared2048 recv/read/poll calls precede ingress close-once. Actual unchanged
handoff repeats hardener → actual constructor → HA1 → maintained SHA256/explicit
legacy MD5 → exclusive fixed0600 file →1536-byte bounded writes/fsync → ONE writer
close → readonly/NONBLOCK/CLOEXEC/offset0/inode checks. Payload/header clear on
constructor return, including failure. Actual local handoff move precedes ONE
SCM_RIGHTS reply; sender/control/directory close-once and full owned secret wipe
precede exit0. File opens returning0..2 are handled through actual owned slots.
Controlled failure wipes owned scratch/exits nonzero; no failure unlink, repair,
resend, readback or secret logging/hash. Crash/SIGKILL/physical wipe/fsync-hang
guarantees are expressly absent.

**Parent completion/HOLD (`exec_owner.c:114–168`).** Receiver records ALL delivered
rights before malformed reply/truncation/count refusal. Exact public bindings,
readonly NONBLOCK/CLOEXEC root0600/nlink1/1536B/offset0, reply dev/inode, fixed-name
same-directory association and image/verifier nonalias checks are necessary but
not sufficient. Actual OWN child's WNOHANG wait0, unchanged original inputs and
all known closes admit READY; explicit bind repeats checks and moves only the
received FD into ORIGINAL cfg.material_fd. No old-owner fill/cast, child-success
Boolean, forged handoff or automatic prepare/start. Unknown wait/ECHILD, nonzero
exit, clock/close/send/receive uncertainty permanently HOLD; only still-known
unreaped child observation remains. Queue/rights/objects/references/possible file/
borrowed directory/image/external root persist; no kill/close/unlink/reset/
destructor/other-PID/retry escape. Keep nlink1 for the future backend. Child exit
does not authorize material/native/DMA/backing retirement or whole-process exit.

## Saved link evidence and concrete limitations

Raw/saved relocation counts HOST parent/worker375/469, ARM466/533; symbol tables
191/230 and309/366. Parent roots are real and exclude main/constructor/handoff;
worker retains real main/constructor/handoff and excludes parent exec owner.
ARM main328 → hardener332/create357/move342/zeroizer316; create → constructor326;
constructor → HA1 index322/section97 → LOCAL hash110/section95 → actual SHA256/
MD5/free/zeroizer definitions. HOST main193/create222/constructor191/HA1 index187
section98 → SECTION-symbol61/.text.hash section96 containing LOCAL hash134.
Parent begin301 ARM/184 HOST reaches actual child-exec282/167 and real image SHA.
All retained referenced project APIs are defined, not stubs. Three parent LOCAL
UND names issue/close/verify (ARM223/225/226, HOST118/120/121) have ZERO retained
relocations; maps discard their only wire callers. Preserved nm UND output is
truthful, not missing required crypto. Normal libc/compiler/syscall dependencies
remain in RELs; no final parent ELF exists. Final worker has no named UND symbols
and real main/hardener/handoff/constructor/HA1/hash/crypto/zeroizer definitions.

**N1 — concrete native incompatibility, not a boundary escape.** Retained ARM
compiler header fixes CAP_LAST_CAP=37; HOST header=40. The inspected retained
kernel75 source header also declares40. Hence this saved ARM policy rejects that
kernel class at caps_supported(PR_CAPBSET_READ,38), BEFORE exec/secret input; older
capability extents also fail the exhaustive checks. Available close_range/execveat/
ambient/securebits/setup authority and matching reviewed capability extent are
joint source prerequisites, not a demonstrated eligible native normal path.
Do not claim this artifact can run on kernel75 or silently extend/skip the bound.
Native eligibility requires a separately reviewed compatible image/policy/pairing;
no correction, rebuild or use is selected here.

**N2 — unadmitted instructions/image.** Actual68080B static worker SHA256
`1c38f3ce45ccdb564c9cdfdc7cc134054b97bac88167a3ed5b49a835da941b0a`:
ELF32 ARM ET_EXEC/EABI5 softfloat, entry0x10750, R-E/RW LOADs, nonexec stack,
no interpreter/dynamic/RWX. Saved user-owned gadorach0755 metadata is NOT eligible
under the root/nonwritable guard. CPU5TEJ AND Thumb-2 attributes are a real unresolved
ARMv5 compatibility risk, not cleared by compile flags or absence of UND symbols.
The source guard does NOT inspect instruction sets/attributes; even repaired
permissions alone would not qualify this image. Independent correct-role image/
hash/ancestor/inode provenance AND instruction/OS qualification remain necessary.
This limits the unadmitted artifact, not the narrower structural/source claim.

New source has no heap requests; ABI metadata matches HOST/ARM local sizes, not
wire ABI. Individual largest recorded stack frames include unchanged8432/8392B
and worker main1664/1560B; neither these nor62240B LOAD memsz sum proves transitive
stack, libc/startup allocations, mlock/process2MiB,128KiB whole-load or memory fit.
Original nominal clock admission/call counts do not prevent late effects across
arbitrary syscall/hardener/constructor/fsync latency, suspension or host loss;
strict C1/whole-interval authority is NOT claimed or accepted.

**Review-reader history:** three read-only metadata inspections initially stopped
on reviewer assumptions: Python vs shell dependency ordering (same1957 multiset),
unnamed raw SECTION symbols displayed by readelf using section names, and ARM map
`LOAD linker stubs` pseudo-entry parsed as a file. Corrected reader interpretation
matched exact recorded dependency argv, all symbols/relocations and actual map
file inputs. These were not candidate/build failures; no sealed evidence changed.

## Conditions and stop

Trusted final-address disjoint immutable config/graph/routes/callback/context/
unused-owner/image objects and serialized FD ownership/no aliases, reentry,
signals, threads, later forks or competing reapers remain mandatory. Authorized
independent writer/exclusive ingress/private namespace and stable root/ancestors
are external facts. UID0 with zero caps retains owner DAC: NOT hostile-code
filesystem/syscall isolation, nor proof of secret-free media-parent history/global
FD/env/cap launch. No old verifier/authority sandbox was weakened or newly applied.

Native caps/securebits/exec/close_range/syscalls/libc/stack/mlock2MiB/instructions,
entropy, authorized writer, durable generations/storage/rotation, main/release,
external lifetime/backing/recovery and C1 remain OPEN. Old fork route/API5 bootstrap
are not adopted by this type; any future integration must inspect actual new owner
and preserve separate finish/normal-only explicit material retirement semantics.

Only this fresh review directory was written. No Git/index/source/evidence/AGENTS/
central-plan/policy changes; no build/compiler/readelf/nm rerun, recipe import,
candidate/helper/test/worker/main/function/fork/exec/hardening/credential/material/
retirement or private/proc/device/network/hardware operation, including HOST.
Normal/negative/adversarial/mutation/fuzz remain NOT RUN.23h OFF is historical,
not fresh state; RAM75 recovery/C1 blocked, Query78 parked, all old failures/HOLDs
unchanged. No self-GO or automatic next queue; stop for parent disposition.
