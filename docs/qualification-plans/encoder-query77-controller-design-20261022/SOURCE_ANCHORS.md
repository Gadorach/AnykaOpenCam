# Static source anchors and input-custody limits

2026-10-22. Companion to [IMPLEMENTATION_CONTRACT.md](IMPLEMENTATION_CONTRACT.md).
All line references are to frozen bytes in [INPUT_PINS.tsv](INPUT_PINS.tsv).
Source existence is distinct from proposed controller behavior and native proof.

Abbreviations (workspace-relative):

- **B** `scripts/qc07f_boot.py`; **P** `scripts/qc07f_private_root_transfer.py`.
- **R** `work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py`
  (the exact source reached by B.IMPL, read as text, never imported).
- **H** `evidence/qualification/qc07f/execution-20261022/ram76-resume22b/`.
- **I** `bringup/qc07f/initramfs/`; **D** `bringup/qc07f/diagnostics/`.
- **K** `work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux/`.
- **C** `apps/qualification/encoder-query77-client/`.
- **Q** `docs/qualification-plans/`; **F** `work/lightbulb-camera/foundation-20261011T041148Z/`.

| Source anchor | Actual fact / reuse limit |
|---|---|
| B23–40,61–95 | IMPL/recorder/loader/adapter pins, DEST/LIMIT, bounded payload and legacy header checks. Generic payload accepts more than exact77; new validator must narrow it. Old BOOTARGS are stock-root, **not** selected77 args. |
| B98–180 | SO_PEERCRED; mismatch drains status but never mutating request; canonical actual argv, start ticks, BOOTTIME lifetime. Read as source only, no current proc/socket access. |
| B204–282 | Refuse existing receipt/socket/adapter owner; pin source; spawned PID versus peer/start/sole-owner evidence; zero TX and fresh catch before operator ON. Power-off flag is not physical proof. |
| B320–398 | Receipt/process/peer/adapter coverage, monotonic minimum, raw/counter reconciliation, sticky reset/QCL rejection. Mutable self.state/epoch cursors cannot authorize rebase after unknown RX. |
| B415–470 | <=128B sends, compare-before-send; text echo before CR,64B text pieces/25ms gap, handoff checks. Five not_sent retries are **not** admitted by new one-shot contract. |
| B482–587 | Vendor-simple exact SPI read/scratch baseline/CRC and bootloader pin; volatile bootargs readback/catch and positive cold banner/prompt. Current caught parser is not a reviewed new password-context parser. |
| B598–650 | Loader1872B pin/468 words, CRC, cover/reserves before GO, QCL96B framing/DONE/QEND/COMMIT. Four ACK attempts/duplicate-frame support cannot be imported as new retry permission. |
| P23–71,105–151 | Protected outside-Git canonical paths/0600/0700, metadata read locks, source/capture/envelope. ROOT payload validation is wrong for77. |
| P115–128,163–177 | RPC restriction and exact ROOT/RAM command allowlist; profile cannot simply inherit generic Boot.command escapes for77/Linux. No source changed. |
| R24–64,71–110 | Conditional stale-RX protection, private status, O_EXCL outputs,1152008N1/exclusive adapter, DTR/RTS requested false; open warning/flush can lose preceding RX. Recorder defaults differ from selected B adapter: actual argv mandatory. |
| R111–132,138–181 | TX intention/completion accounting; early banner+DRAM catch up to three19B key packets at100ms; cancellation upon SF/kernel/prompt/password text. Count actual completions; never assume three sent. |
| R204–247,261–268 | Actual capability/status fields, arm consumes next new boot window, expiry ACK, protected frame<=128B/conditional RX and no pending observer. Cannot infer catch remains unconsumed from historical ACK. |
| R215–219 | Existing hold-cli arm also reads a historical raw CLI capture. That capture was not opened here; source pin is not permission to drop this dependency or replace it with a ready flag. Future parent/profile review must explicitly admit its protected lineage/read or separately review a new catch implementation, not silently bypass a safeguard. |
| R289–306 | Capture end/error/final file hashes. Private capture finalizer not a query-result/retirement finalizer. No stop/reclaim selected. |
| H/preserved_hold.py15–54 | Original radio HOLD comparison; subordinate rejects nonempty (now reset HOLD). Existing implementation writes/clears subordinate and uses r+ original: must **not** reuse for dual read-only locked failures. |
| H/cold_gate.py21–51 | Eight fixed reads and neutral values: s74=02000006,s78/s7c=0,dir/out=0,input &8000c0=0,t3/t4=0. Assertions are historical checks, not adopted safety validation. |
| H/ram76_boot.py20–51,73–93 | Historical76 pins/config wrong for77; exact fixed bootargs, boot-start TOP pair, full independent RAM CRC/startup message/monitor/checkpoint pattern only. |
| H/postcheck.py24–91,94–153 | RO list, independent schema, mount/memory/ISP/UART/boot UUID and cumulative TX. Does not provide complete all-process FD inventory; optional refresh cannot excuse unexpected RX. |
| D/uart_ram_loader.c43–57,58–93 | READY first-byte wait unarmed; first byte starts15s guard; frame receipt feeds; duplicate/NAK paths exist but no new retries allowed; stop precedes COMMIT_OK. |
| D/mmu_uart.h8–27 | Loader UART/L2/SRAM register effects, bounded print waits and halt loops; explains why RAM-only loader still has exact UART/watchdog MMIO, not GPIO/SPI writes. |
| I/init.c66–105,291–300,465–535 | Stoppable check; smoke/reset are other operations, not stage guards. Startup magic-close print does not validate write/close returns. Monitor mount/console/dispatch and no88 query. |
| I/wifi-monitor.h23–37 | Fork/setsid/TIOCSCTTY/exec BusyBox ash and real wait; no watchdog receiver or exported child exit status/PID. Lines2–22 wifi_attach are forbidden, not bootstrap recovery. |
| retained BusyBox coreutils/uudecode.c279–297 | Actual baseNUM -d stdin decoder path, no progress watchdog ownership. Config/applet/ELF association inherited from native-method findings, not requalified here. |
| K/arch/arm/mach-ak3918ev200/qc07f_hwenc.c361–385,431–459,480–506 | Exact query dispatch, release/open CPU session and telemetry init, misc node; no supported remove. Query-only/no-other-work conditional effect distinction. |
| K/arch/arm/mach-ak3918ev200/encoder-open-query.inc5–45 | Actual file-private identity/ref/telemetry under interruptible mutex, copy_to_user, no hardware/counter mutation; waits/copy not preempted by client budget. |
| K/arch/arm/mach-ak3918ev200/watchdog.c39–95,103–129 | Actual TOP start/stop/set-timeout effects and support; active5s rearm; heartbeat fields remain zero. No stage receiver. |
| K/drivers/watchdog/watchdog_dev.c76–96,248–319,784–825,864–919,942–989 | Conditional core feeder, start/stop, timeout re-ping, single-open starts watchdog, inactive release versus failed-stop feed. Distinguish inherited startup from client-owned watchdog. |
| C/encoder-query77-qualification.c19–28,40–71,133–153,186–267 | Frozen88/ioctl; no-service nonreturn; original1.5s; actual only device opens/ioctls/close/disable then two stdout writes. Late disable/close may leave timer inactive. No argv readiness authority. |
| C/encoder-open-query.h12–45 | NONE/identity not exclusivity/DMA retirement; mutex/ledger/history limits; full size/direction command prevents old76 fallback. |
| Q/encoder-query77-client-20261022/{SOURCE_CONTRACT,PARENT_DISPOSITION,CANDIDATE_REPORT}.md | Conditional SOURCE/API scope; exact ARM ELF/static bounds, real watchdog caveats and actual native prerequisites. Compile logs cited, not rerun. |
| Q/encoder-query77-build-20261022/{PARENT_DISPOSITION,BUILD_RESULT}.md; F/analysis/encoder-query77-build02/PACKAGE02.json | Exact image/config/package/compiler/static layout and unchanged DTB/initramfs. New offline baseline, no old76 reproduction, station pairing or native use. |
| Q/kernel76-transport-20261021/WATCHDOG_RESET22B_RESULT.md5–32,42–52 | Failed substring classifier and separate actual stable held-U-Boot reconciliation; no retry or false PASS; neither HOLD is cleared. |
| Q/encoder-query77-native-method-20261022/{METHOD_FINDINGS.md,INVESTIGATION_INPUTS01.json} |19 prior public inputs and proposed628-chunk volatile route, missing staging guard/observer and fresh admission. |

## Pin/provenance method and scope

`INPUT_PINS.tsv` records46 public source/document/artifact reads; all19 prior
investigation pins matched. First snapshot17:04:18UTC is **after source reading,
before authoring this design**, not a pre-use/build seal. A final static stdlib
read/hash comparison is recorded separately in `PIN_CHECK01.json`. Aggregate:
sorted full workspace-relative UTF-8 path + NUL + raw SHA256 digest bytes.
Only local public regular files were hashed; binary artifacts were data, not run.
No candidates/helpers/scripts/compiler/generator/client/tests imported or executed.
Inline stdlib hashing only; no source was emitted as executable code.

The final comparison has **43 unchanged inputs and three changed contextual
documents**, not all46 unchanged. During authoring the harness reported parent
AGENTS updates; bounded rereads of AGENTS114–142, index75–90 and ledger16–53 show
the parent's active design-lane/supplement/commit checkpoint. Original manifest
pins remain intact, not rebased. Their before/after hashes are recorded in
`PIN_CHECK01.json`; no helper/controller/client/kernel/artifact pin changed. Full
byte-diff attribution of the three documents was not performed. Future native
admission must freeze its own then-current reviewed profile; this record cannot
authorize operation against changing source inputs.

Parent's later native-method `TRANSFER_SOURCE_BOUNDARY.md` and
`TRANSFER_INPUTS02.json` were separately read after the first46-input snapshot.
`SUPPLEMENT_PINS.tsv` records their later pins; the nine-input source supplement
is cited for EOF/ash/stdio/result framing limits, not claimed freshly reproduced
or incorporated into the earlier46-file snapshot. No ash source was imported/run.

The inventory includes some prior method artifacts rehashed but not freshly
ELF-parsed/content-audited here (BusyBox/config/applet table, initramfs inventory,
client/image binaries). Do not elevate pin equality into fresh implementation/
allocation/ELF/native acceptance. Narrow source slices, not whole kernel/BusyBox
audits, support this design. Both actual HOLD files, private evidence and current
process/adapter state were intentionally **not** read or hashed.

Two optional guessed-path reads returned missing-file/directory errors (a proposed
package directory and client BUILD_RESULT); no policy refusal occurred and neither
missing path was retried. Authoritative discovered PACKAGE02/CANDIDATE_REPORT paths
were read instead. No executable or network/device fallback was used.

Upstream SPDX/licenses remain untouched; paths/hashes here concern public code and
reviewed artifact metadata, not firmware acquisition/private digests. Generated
ELFs/images remain local and are not publication candidates. No Git/index,
AGENTS, central plan, sibling/source/controller or safeguard change was made.
