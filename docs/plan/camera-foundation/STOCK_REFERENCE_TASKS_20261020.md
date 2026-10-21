# Active stock-reference closeout tasks

**ACTIVE — user authorized execution on 2026-10-20.** This is the execution
reference, not a proposal. Scope/methods remain in
[the closeout plan](STOCK_REFERENCE_CLOSEOUT_PLAN_20261020.md).

**Latest checkpoint:** all unblocked source/decision rows executed and parent-
reviewed. Native resource/cadence rows are explicitly BLOCKED. One fresh20b stock
boot reached read-only root but not a verified shell; no retry. User then confirmed
**OFF, leave OFF/UART connected**. Both private captures finalized/hash-verified;
no UART owner/socket/recovery arm or worker remains. This is not a project-wide
halt or native qualification pass. Reviewed inputs:
[SRX-08 result](../../qualification-plans/stock-reference-20261013/closeout-parent01/NEXT_BUILD_INPUTS.md).

**Subsequent user steering:** transitioned to modern-kernel work. The proposed
USB increment is now implemented and candidate76 built/packaged with offline
verification; follow [KERNEL76_TASKS_20261020.md](KERNEL76_TASKS_20261020.md), not
a repeated stock campaign. The native/physical BLOCKED rows below remain honest.

## Rules and order

Dependency-first priority: **SRX-02 SD/L2 → SRX-01 Wi-Fi → SRX-03 video →
SRX-04 motor zeroing → SRX-05 audio → SRX-06 AP decision → SRX-07 NOR decision**.
Independent read-only source work may overlap in at most two disjoint lanes;
hardware work is parent-only and serialized. A physical/entry blocker is recorded
and does not stall independent source tasks. No task is native-passed by a source
report or mock alone. Work remains local/uncommitted/unpublished.

User approved the investigation plan, not its separately excluded effects:
**no live homing/full travel, limit bypass, card writes/removal fault injection,
AP broadcasting/new SSID, speaker/GPIO1 effects with UART connected, firmware/
ENV/MAC/OTP writes, factory reset or full retail-app/cloud/recording startup.**
Original footage/recovery media and credentials stay protected. No disabling a
native refusal gate, read-clear register polling, guessed ioctl or broad legacy
ABI/instrumentation project to force a result.

Statuses: `QUEUED`, `ACTIVE-SOURCE`, `ACTIVE-NATIVE`, `SOURCE-CLOSED`,
`NATIVE-PASS`, `BLOCKED`, `DECISION-CLOSED`, `EXCLUDED`.

## Task ledger

| Order | ID / task | Status | Required result / dependency | Evidence |
|---|---|---|---|---|
| 0 | SRX-00 Freeze rollback and current admission | BLOCKED | Host pins verified; fresh20b catch/original kernel handoff attempted, shell admission failed; now user OFF/capture retired | `closeout-parent01/NATIVE_ATTEMPT.md`; all rollback pins match; no current native/recovery epoch |
| 1 | SRX-02A Actual MCI board branch + L2 contract | SOURCE-CLOSED | Board5 GPIO47 pulse, exact group13 mux, dynamic L2/UART/ADC/DAC ownership and unsafe stop/free paths documented; physical/pdata/clock gaps explicit | `closeout-sd-audio01/RESULTS.md` + `closeout-parent01/MCI_MUX_ADDENDUM.md`;87 input rehashes/independent parent checks |
| 1 | SRX-02B Source-safe stock resource receipt | BLOCKED | Whitelist reviewed; no reader/provider/resource snapshot attempted without verified shell; no new host bind/load | One20b native entry failed; no register/card command; user OFF resolved epoch |
| 1 | SRX-02C Optional expendable-card RO reads | BLOCKED | Specifically identified expendable test card and reviewed host initialization; original footage/recovery card excluded | No test card admitted |
| 2 | SRX-01A USB/radio transport delta | SOURCE-CLOSED | Stock low-byte OUT kick preserves MODE; modern next-packet0x00a7/high-first write clears it. One Anyka-only continuation-kick hypothesis; not a proven fix | `closeout-wifi01/REPORT.md`, parent04 reproduction:42 pins/4 kernel+7 station seals/256 arithmetic cases |
| 2 | SRX-01B Targeted public-pattern comparison | DECISION-CLOSED | No new stock repeat justified: existing1500/2048 successes + exact compiled delta sufficient; discriminating future test belongs on modern patch | Parent-reviewed bounded stop; modern MTU1500/native transport acceptance still open |
| 3 | SRX-03A Minimal media entry + pipeline audit | SOURCE-CLOSED | Exact current executable HOLD identified; continuous4-slot/reference/timing contracts reused | `closeout-parent01/VIDEO_NOR.md`; current native main and adapter both refuse before effects |
| 3 | SRX-03B Uncapped stock cadence measurement | BLOCKED | Missing admitted minimal encoded/uncapped consumer route + instance/clock/reset seals; not resolved merely by stock boot | Stored15/cached30 remain unmeasured; no broad ABI detour or refusal removal |
| 4 | SRX-04A Zero/phase/STOP/origin source closure | SOURCE-CLOSED | Exact8-phase recovered; seek-like reset conflicts with software-limit branch; NO-GO copying homing, manual origin first | `closeout-parent01/MOTOR.md`; motor-run03 has fresh disassembly/relocations and8 decision cases; physical/runtime policy unresolved |
| 4 | SRX-04B Live zeroing | EXCLUDED | Separate reviewed physical proposal only if source findings justify it; no automatic motion | No movement queued |
| 5 | SRX-05 Audio provider facts | SOURCE-CLOSED | Exact retail ADC8k OSR256/divisor1..256, calculated8012 at400MHz/div195; shared ADC/DAC clocks/close/L2 hazards documented; ALSA/native work remains | `closeout-sd-audio01/SRX-05.md`; existing scalar mic/audible5s reused, no new sample/effect |
| 6 | SRX-06 AP capability decision | DECISION-CLOSED | Exact firmware capability UNRESOLVED; selected station75/beacon-only prototype NO-GO as protected AP. Host role/client/key evidence is not firmware/RF acceptance | `closeout-wifi01/AP_DECISION.md`; mandatory setup feature still open, no broadcast/guard removal |
| 7 | SRX-07 NOR/boot necessity decision | DECISION-CLOSED | No new stock controller fact blocks next RAM image; skip NOR experiment; persistent packaging/install separate | `closeout-parent01/VIDEO_NOR.md`; RAM uImage exceeds current KERNEL partition, not an install candidate |
| Close | SRX-08 Next-build inputs and open flags | SOURCE-CLOSED | Consolidated reviewed driver inputs, one next MUSB patch/test and unresolved native/physical gates; no image/install performed | `closeout-parent01/NEXT_BUILD_INPUTS.md`; native BLOCKED rows preserved |

## Current execution checkpoint

- **Superseding actual:** user OFF/leave OFF, UART connected.20a and20b private
  capture files sealed/full-hash verified;20b PID/socket retired, adapter unowned,
  failed-boot HOLD resolved only after user power OFF. No recovery arm or GUI/workers.
  Do not inherit any power-on/RAM/network/IP/recorder statement below.
- 03:50UTC read-only **host** discovery found the intended adapter owned only by
  recorded20a recorder PID3651532; socket exists, operation HOLD empty. No target
  byte was transmitted by this discovery.
- Last actual target result remains the03:01UTC RAM75 neutral/idle postcheck; no
  claim of a current stock runtime. GUI is stopped; retired GUI epochs not reused.
- The historical20a capture ends04:12UTC. Revalidate actual process/source/catch/
  remaining time; do not use that dated deadline as future authority. A new
  user-coordinated OFF/fresh-capture/ON handoff may be needed for the stock window.
- First action is bounded source closure, not a reboot or module load.

## Progress log / result requirements

Append a dated entry for each completed increment: exact question, source/input
and tool pins, new observations versus inference, artifact paths, modern-driver
decision, remaining blocker and one next step. Keep private UART/config/image/
identity/digest data outside Git; use public code/evidence references appropriately.

### 2026-10-20 — execution opened

User instructed creation of an actual active reference and execution by priority.
SRX-02A opened first; disjoint Wi-Fi source lane may begin without waiting on a
physical SD prerequisite. No optional motion/card/RF/write effects are queued.

### 2026-10-20 — parent host/source increments

- `freeze_host.py` verified retained kernel75 build/config/symvers/radio pins,
  optical/white modules and all three control helpers; DT/initramfs rehashed.
  Host-only status matched sole20a recorder identity/source, protected/redacted
  capture, no busy TX and catch armed, with817s conservative lifetime at the
  receipt. This does **not** renew it or validate target/runtime; fresh physical
  epoch is still needed for a stock session. No target byte sent.
- SRX-03A/03B: inspected current capture-reference native source; refusal remains
  independently in main and adapter. It is not a full encoded cadence benchmark.
  Exact HOLD and useful modern continuous-queue/profile decisions recorded rather
  than reopening the forbidden broad ABI/trace project.
- SRX-04A: direct ELF/relocation extraction recovered phase8, and fresh selected
  app/kernel assembly bound worker→self-check→both RESET ioctls. The seek-like
  reset's first negative leg is intercepted by software limits for normal margins;
  eight pure branch-model cases pass. This is a meaningful contradiction, not
  evidence of successful physical hard-stop homing. Live RESET remains excluded.
- SRX-07: decision closed as unnecessary now; preserve all flash and focus next
  persistent-design work on actual image/layout fit. No new flash acquisition.
- Source lanes SD/L2→audio and Wi-Fi→AP remain isolated; parent reviews before
  final ledger closure. Reports live under the campaign's `closeout-*01/` folders.

### 2026-10-20 — reviewed source closeout and failed native entry

- Both source lanes completed; parent reproduced the Wi-Fi source/ELF/256-case
  collector in fresh `closeout-wifi01/parent04`, independently rehashed all87
  SD/audio inputs and checked GPIO47, request masks and exact ADC assembly/math.
- Parent additionally recovered retail group13 table (one named40-byte row) and
  its attribute branch: SYSdc mask3fc0/value15c0; pullup API calls31..36 with1.
  This closes the table gap, not physical voltage, initialized pdata or clock parent.
- Exact Wi-Fi delta supports one modern Anyka-only continuation-kick experiment;
  another stockMTU1500 run was closed as unnecessary, not falsely native-passed.
- One user-coordinated OFF/fresh protected20b/ON handoff passed cold catch and
  original BOOT/kernel pins. Volatile init=/bin/sh/ro stock boot reached read-only
  root and garbled console bytes, not a verified shell. No module/resource/card/
  network/media command followed; no automatic boot retry. An initial password-
  prompt interpretation was corrected as stale pre-kernel text; no credential sent.
- User then confirmed OFF/leave OFF. Both captures preserved/hash-verified and
  owned recorder/socket retired. All details/failure boundaries are in
  `closeout-parent01/NATIVE_ATTEMPT.md`; raw/CRC/config data remain private.
- No native15/30fps, MMC provider, secure AP or homing claim. No new image build,
  flash/ENV/OTP/card write, full retail app or live actuator/audio/RF trial.
- **Next engineering input:** isolated Anyka USB continuation-kick patch and finite
  modern discriminating test; not a queued hardware operation. Hardware rows stay
  blocked on a fresh physical/console admission and their specific reader/media gates.
- Final04:27UTC host-only check: adapter unowned/socket absent/HOLD empty,20b
  retirement verified; failed native entry recorded as FAILED, not a pass. All85
  immutable SD/code/report inputs still match; only central AGENTS/ledger were
  intentionally updated after the earlier87-input review. Receipts:
  `closeout-parent01/{STOCK_BOOT20B_FAILED01,FINAL_CHECK01}.json` under campaign evidence.
