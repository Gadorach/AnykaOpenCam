# Driver entry24 — independent ordinary SOURCE/METHOD/custody review

2026-10-24 · BASE-01/05/07, PLAT-06/08; preparation for RP-02/11/12.

**REQUIRES CORRECTION — v1 not adopted.** One ordinary failure-path custody defect
prevents narrow sufficiency for the complete advertised transaction. Freeze the
reviewed v1; any fresh bounded correction is for the parent to select. No candidate
invocation, execution admission, image pairing or native/product GO follows.

## Exact scope and pins

Read **all 1,166 lines** of `apps/qualification/qc07f-driver-entry24/driver_entry.py`,
its README and all six author qualification files; full original v4, recorder,
`scripts/qc07f_boot.py`, fixed cold classifier, loader C, monitor `init.c` and
`wifi-monitor.h`, PTZ source, private-transfer source and the three historical22b
source references. Read the supervised policy, relevant index/ledger, prior v4
review/contract,23h public result, RAM75 findings/disposition and controller source
anchors. Historical scripts were text references only, never imported or replayed.

Independently read/hashed **all26 records** in
`docs/qualification-plans/driver-supervised-entry-20261024/PINS.tsv`: all byte counts
and SHA256 values match. That manifest's SHA256 is
`e68d2c55490e5043d53984873b8879a1fb0c226986ee932057e991ba5d44627d`.
This is a selected public-input check, not dependency/build/native evidence.

| Principal reviewed input | SHA256 |
|---|---|
| `apps/qualification/qc07f-driver-entry24/driver_entry.py`,65982B | `262ec18013905e2774eb197c4d509061bf928724188c77337c8a1ddb030bf765` |
| Original `qc07f-cold-entry23-v4/cold_entry.py`,30486B | `40794530f6ef7bb9fec15104fee5f3c06e841ba6a08947794041bac99b3cc8b5` |
| `scripts/qc07f_boot.py`,39829B | `6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985` |
| Original `uart_capture.py`,25426B | `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00` |
| Fixed `cold_context.py`,4019B | `17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558` |
| Actual retained loader bytes,1872B | `a1630c90f7f44666e941704778cbf119e5e2e13355113adca52f842bd9046ad1` |
| `bringup/qc07f/initramfs/init.c`,24349B | `b3dd4ee9689bde4bead4c7a3c862284b198b879d4470b920366c1503814598aa` |
| Reviewed PTZ source,7551B | `a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87` |

Supplemental public reads/hashes outside those26 records:

| Path | Bytes / SHA256 |
|---|---|
| `docs/qualification-plans/execution-20261023/COLD23H_RESULT.md` |2875 / `bc26189ff858d5a392213cf14661c7946269d5a1d5ebbab842a6b78cf86a9c1a` |
| `bringup/qc07f/diagnostics/mmu_uart.h` |1083 / `f8936403da09fa0ec3f6fba14eec871b1503304f1ed90716a17e3d265692786d` |
| `bringup/qc07f/variants/security75/mach-ak3918ev200/isp.c` (schema slice473–495) |67624 / `7d0ef80561df4ef058be1deb6698001c35066fab5533f492dec97d2b447bbd50` |
| `bringup/qc07f/variants/security75/mach-ak3918ev200/uart.c` (schema slice189–193) |8337 / `175e30cffdf173a9f631ea369540c3d96ae9bf7100edf32a8cc73a763dc1efe0` |

The schema slices are retained-source corroboration, not the future image's build
association. One attempted historical `drivers/watchdog/watchdog_sysfs.c` read was
absent; no implementation proof is attributed to it. No active builder/output was
polled or read. No new image size/digest/selection exists in this review.

## R1 — already-exited child bypasses failure final accounting

**Blocking custody/failure-method defect**, `driver_entry.py:983–1082`.

After ON, the OFF POST/settled GET may fail and leave `off_proved=False`. If the
actual owned recorder has **already exited** by cleanup's `child.poll()` at1039:

1. The known-OFF branch at1003 is skipped.
2. The `elif` at1037 records manual-OFF/retention uncertainty.
3. The passive-retention branch at1039 is also skipped because the child is no
   longer running.
4. Neither available-manifest audit (`audit_final` at1031 or1076), nor the
   PID/socket/visible-owner absence checks, runs. Cleanup proceeds to original
   input checks/result writing/descriptor release at1083 onward.

This is an ordinary ordering possibility: an original-duration expiry or recorder
exit can precede or occur during an uncertain cutoff. It does not require hostile
input, mutation, a retry or an outside actor. Both audit sites are guarded by the
wrong combined power/liveness branch for this case. Merely saving the child's
return code and `capture_finalized=False` is not the promised available final-file
hash/RX/event/TX reconciliation. HANDBACK52–57 and README35–41 expressly promise
available eventual accounting even when power or retirement is uncertain.

This does **not** create a false overall PASS: the uncertainty remains sticky and
success1105 refuses. It is nevertheless a real omission in the closed custody
contract, not a missing runtime test.

**Minimum correction required:** available final evidence/actual child retirement
must be accounted for after termination regardless of whether OFF was proved or
the child was still alive on entry to cleanup. Preserve manual-OFF requirement,
original errors and failed controlled-stop disposition; unknown OFF must still
forbid a stop RPC, kill, takeover or renewed target operation. Missing/unverifiable
final evidence must remain an explicit failed check. No corrected source, test
matrix or implementation queue is supplied here.

## Other source observations (not independent adoption)

- **Actual helper use:**89–106 selects exactly13 named definitions plus specified
  constants/imports from SHA-pinned v4 bytes with v4's real `__file__`; no v4 main,
  old Boot instance or global PrivateBoot bypass.730–740 selects only the two pure
  CRC definitions from the exact full Boot source. Classifier783 executes retained
  pinned bytes in its registered distinct module, not a cached classifier pyc.
  Interpreter/serial-init hashes and actual serial `__file__` are checked; the
  ordinary system/serial dependency loader closure remains explicitly trusted.
- **Preflight/ownership:** Retained ancestry/metadata/read checks and exclusive
  read-only radio0644/reset0600/empty-instance0644 marker locks are preserved.
  Historical854B prerequisite is retained and source-derived, not freshly read by
  this reviewer. Numeric RFC1918 QC07F-only endpoint/config validation, initial
  strict GET OFF before capture, durable intents and final fresh pre-ON status are
  real code. Actual Popen is stored before ticks/identity failures; exact argv,
  start/peer/interpreter/cwd/adapter/fuser and protected fresh outputs are checked.
  No source/controller serial open, blind kill, socket unlink or owner adoption.
- **Selection:**283–314 requires explicit selection path+SHA, duplicate-free fixed
  keys, fixed profile/release/DTB/PTZ, real retained file records and exact separate
  association JSON. Actual uImage size/load/entry/type/IEEE CRCs, suffix DTB,
  whole-image BZIP2 CRC and strict staging ceiling are checked. This cannot prove
  build provenance, decompression fit/config/compiler pairing or authorize an
  image merely by hashing arbitrary reports. Parent-reviewed actual association
  and public image selection remain external mandatory prerequisites.
- **Timing/order:** the first5400s cap precedes preflight/child creation and only
  shortens from actual BOOTTIME start ticks. Actual bracketed clock intervals are
  intersected, not tolerance/fabricated triples. Point checks retain origin-based
  phase caps/reserves; socket actions clamp to one nonrenewing RPC/local bound.
  Source hash/fuser/blocking costs are checked before further controlled TX.
  Response/echo loops retain their local ends; no timeout retry or deadline
  reanchor was found. Per-frame interpreter hashing/fuser work is substantial and
  nominal `.06/frame` is **not measured throughput or progress proof**. HTTP's
  inherited per-receive timeout, filesystem calls and scheduling are not hard
  preemption. The supervised policy permits detection/abort, not strict C1.
- **Cold and transport:** exact v4 whole19-record360B prefix plus every counted118B
  trailer, one/three completed19B catch packets and separate10s unchanged raw/
  events/counters precede U-Boot work. Original offset0 remains; only classified
  complete responses advance the logical cursor. Each controlled send uses the
  actual watermark CAS and refuses stale/partial results without retry. Extended
  events are audited at finalization, not by misapplying cold-only final observe.
  The recorder's inherent serial-open flush is disclosed, not new prefix trimming.
- **Closed plan:** BOOT zero/read/CRC, neutral8+TOP-bit read checks, exact468 loader
  words/CRC, one future catch, GO/READY/QCL header/96B frames/unique ACKs, last
  ACK+DONE together, one QEND/COMMIT/held prompt and independent whole-RAM CRC are
  actual ordered calls. Text chunks require echo before execution CR. No old
  five-CAS/four-ACK loop is used. Loader source arms15s only after first byte and
  stops before COMMIT; READY/staging/commit/idle gaps need real human fallback.
- **Boot/RO:** fixed TOP pair/volatile args/readback precede one bootm. New release,
  Starting-kernel/monitor receipt and source-defined ordered help are checked;
  help tokens are received text, not a TX allowlist. U-Boot full envelopes are
  new strict schemas; historical helpers' looser matching is not proof that every
  new envelope will accept measured vendor output. No measured full boot transcript
  was opened here. Unknown/unframed/duplicate/asynchronous RO output fails closed;
  no echo/prompt repair or successful normal-path runtime claim is warranted.
  All16 queries are fixed monitor reads; UUID/PID1/UID/capability/release/args/
  mounts/modules/memory/CMA/ISP/UART/watchdog/PTZ schemas are independently checked.
  PTZ zero verification flags are truthful probe state, not physical coils-OFF.
- **Cleanup otherwise:** first HOLD is sticky; uncertain ON still gets one settled
  GET, then one OFF POST/.25s/GET independently of failed clock admission. Known
  OFF gates same-peer stop; partial identity refuses stop and retains actual Popen.
  Failed stop/wait remains failed after passive original-duration exit. Every
  retained input/marker postcheck is separately attempted before unlock. R1 above
  is the uncovered already-exited/unknown-OFF branch, not permission to weaken
  these boundaries.

## Conditions and disposition

Supervised bench policy supersedes strict whole-interval prevention **only at its
stated scope**. Same-host smart switch is normal cutoff; genuinely present manual
host-loss fallback, immutable trusted sources/config during the lease, sole current
ownership and parent admission remain necessary. No unattended power failsafe,
physical OFF/STOP/thermal/zeroing, complete process-FD/DMA, native memory/capacity,
Wi-Fi/MTU1500, microphone or product qualification is established. Baseline sensor
reset/MCLK, GPIO42 radio power/USB enumeration and ISP allocation are disclosed
boot effects. Speaker tests remain DEFERRED; backend expansion remains PARKED.

Only this fresh review directory was authored. Public reads/hashes/AST/text only;
no candidate/helper import, compilation, call/test/main/help, command replay/build,
private/proc/dev/current-state access, network/UART/RPC/outlet/GPIO operation,
active artifact access, source/marker/image/Git/index/AGENTS/plan/settings change,
subagent, adversarial case or provider/safeguard change. Parent disposition,
correction review, actual independently reviewed paired-image selection and fresh
supervised admission are still required. **No self-adoption or native queue.**
