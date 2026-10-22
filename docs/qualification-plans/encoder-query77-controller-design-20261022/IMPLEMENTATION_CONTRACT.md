# Query77 parent controller — bounded OFFLINE SOURCE DESIGN

2026-10-22; ENC-02 / PLAT-01/06/08, BASE-01/05/07. **Design candidate only;
not independently accepted, implemented, executed or native-admitted.** This packet
selects only the next *source* boundary: dual historical-HOLD preservation and
nonrenewable epoch custody before boot. Later phases specify interfaces/stop
conditions, not an executable controller, TX recipe or physical-action request.

Last recorded target fact is held original U-Boot at12:12UTC; neither Linux77,
current power, owner, adapter, catch, clock coverage nor operator availability is
known here. Both radio and reset failures remain FAILED/HOLD. Old22b entrypoints,
receipts, parsers and helpers remain immutable historical evidence; do not import,
run, edit, retry or use them as authority for a new epoch. No target, serial,
network, recorder, private capture, host proc/dev or current state was accessed.

## 1. Three different evidence classes

- **Supported source primitives:** original bootloader pin/read, 1872-byte pinned
  word-aligned RAM loader, QCL CRC/framing/commit, exact volatile bootargs/readback,
  original monitor read-only dispatch and console BusyBox shell, conditional
  protected recorder sends, frozen query88 client. See [SOURCE_ANCHORS.md](SOURCE_ANCHORS.md).
  Their existence is not a current owner, timeout, successful syscall or recovery.
- **Proposed new contracts:** every rule below, including new cold classifier,
  dual-HOLD guard, phase receipts, single-attempt transport, shell transfer and
  observer. No parser/controller behavior is newly validated by this document.
- **Missing implementation/admission:** reviewed new controller/profile, complete
  shell/decoder allocation/quoting/exit observer, actual sole-process/FD/no-job
  admission and current physical recovery handoff. Offline design can continue
  without hardware facts; an operation cannot replace them with ready flags.

## 2. Immutable selected-input profile (fail before any live lease)

Future validators must use explicit exceptions/refusals, never `assert` for safety,
and must not call old22b `inputs()` or accept its PASS booleans. Freeze new source,
dependencies, parser grammar, allowed effect sequence and input pins **before** a
live lease. No helper/source/recipe edits, parallel workers or source resnapshot
to rationalize drift during one. Every input is regular, bounded and stable across
read; reject symlink/path substitution and recheck held descriptor/path identity.

| Input | Exact required association |
|---|---|
| Query77 uImage | 3,603,307B; SHA256 `f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e`; full CRC32/BZIP2 `63ea4eac` |
| Legacy header | magic `27051956`, timestamp0, payload3,603,243B, OS5/ARM2/type2/compression0; load `0x82408000`, entry `0x82408040`; name `QC07F encoder-query77 RAM ONLY`; header IEEE `011cd017`, data IEEE `0d55fce6` |
| Package/config | Exact pinned `PACKAGE02.json`, query77 `.config`, build disposition/result and retained provenance; config SHA `d7b2ae8f167c8b56f442a201684eb433cc621bce45f52c98c94a87e48879b5bf`; exact release `6.18.55-qc07f-encoder-query77`, coherent NEW Clang23.1.3, not relabelled76 |
| Payload constituents | New zImage3,600,672B/SHA `65bb107d9a33ca1733bfbdd8439a00b37e7c72f0f634cecfb9ce2cf924290eb8` plus retained DTB2571B/SHA `d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`; unchanged initramfs/monitor association from pinned package/build evidence |
| Loader/capture | Loader1872B/SHA `a1630c90f7f44666e941704778cbf119e5e2e13355113adca52f842bd9046ad1`; recorder source SHA `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00`; actual processes must later match, not just files |
| Client | ARM ELF30,140B/SHA `87a40f291600fee1296d96030726b9db825956e274e0db2a7ee5ea18f6a26ac6`; exact reviewed C SHA `d132a2eeaae170586aa437a2e7a3102027d9ff656077c5102f57dd79d95268b7` |
| Query ABI | Header2455B/SHA `6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37` equal in client/frozen candidate/actual query77 kernel; version1, size88, identity offset32/reserved84, ioctl `0x80585166`; no old76/56-byte fallback |

Reject any mismatched image/header/config/package/release/ELF/UAPI/source pin; a
signed-off report or filename alone cannot substitute for the actual selected
bytes. Future validator must inspect ELF32 little-endian EXEC/ARM/EABI5/soft-float,
ARMv5TE, no INTERP/NEEDED/dynamic dependencies, no WX LOAD or executable stack and
compare exact reviewed static load/stack metadata. Do not execute an ELF to inspect
it. The source-design snapshot hashes selected artifacts only; it does **not**
repeat the parent's250,219-path build verification or ELF/package validation.

Keep strict4MiB transfer ceiling, measured40MiB RAM `[0x81800000,0x84000000)`,
staged77 `[0x82408000,0x82777b6b)`, kernel live `[0x81808000,0x82059bc0)`,
conservative decompressor end `0x82790578`, CMA `[0x82800000,0x83800000)` and
457,352B static gap. No relocation/copy alternative or higher bound. Preserve
128KiB allocation,2MiB child/4MiB HOST limits; snapshots/static ELF figures are
not measured process/combined-load memory admission. Embedded station76 is
unpaired/unsupported and never loaded, including autoload.

## 3. Dual-HOLD and private epoch custody — smallest next source packet

Historical paths are identified **symbolically** by immutable source: original
radio marker is `CONTROL / peripheral-gui-operation.lock`; reset marker is
`CONTROL / peripheral-unattended22b-operation.lock` (Boot's CONTROL definition,
preserved_hold lines25–40). This investigation did not read either actual file.

Future parent guard must:

1. Open both existing files read-only, NOFOLLOW/CLOEXEC/NONBLOCK, never CREATE,
   truncate, append, chmod, replace or clear. Validate canonical protected parents,
   regular singly linked owned0600 files, stable dev/inode/size/mtime/ctime/mode/
   uid/link metadata, descriptor-to-path identity and bounded content. Acquire
   nonblocking exclusive advisory locks in fixed radio-then-reset order. Any
   contention or unexplained metadata/content is refusal, not kill/reclaim.
2. Privately pin raw bytes/metadata as well as parsed meaning: radio HOLD operation
   `one_radio_native_load_RUN21a`; reset HOLD operation
   `one_idle_watchdog_reset22b_stop_at_uboot`. JSON equivalence alone is insufficient.
   No public hashes, raw bytes or private paths. Compare both locked descriptions
   **and** current path metadata/bytes before, at every phase/effect boundary and
   after every operation, including failure. Keep locks held throughout. During a
   transfer, only its sole controller may perform these local integrity checks;
   no parallel helper/polling. Advisory locks do not prevent noncooperating writes:
   future source must fail on replacement/content drift and cannot claim immunity.
3. Create a separate protected new operation marker with O_EXCL/NOFOLLOW,0600,
   fsync plus parent-directory durability before the first effect (including
   recorder open/arm). It records operation/profile pins, current phase and
   `IN_FLIGHT_UNKNOWN/HOLD`, not ready. All outputs are fresh O_EXCL with safe
   ancestors, no symlink/hardlink overwrite; directories0700. Raw, boot UUID,
   adapter/peer/process identity, counters, transcripts and errors stay outside
   **every Git worktree**. Do not copy old envelope/public owner receipt layout.
4. Retain marker on any exception, interruption, partial TX, uncertain syscall,
   stale watermark, clock regression, lost reply, unexpected RX/reset, failed
   postflight or source/HOLD drift. Crash with no durable completion means unknown,
   never resumable or automatically cleared. A restart may only report the pending
   phase; no target effect or adoption of a new status baseline.
5. Use fresh exclusive intent/completion records with sequence and predecessor
   hashes; durable completion only after positive phase proof and dual-HOLD
   comparison. Never erase the marker to indicate success: a new finalizer may
   append a terminal completed receipt while retaining first-failure/unknown
   history. No continuation once any phase failed. Torn/ambiguous ledger is HOLD.

**Packet C1 proposal:** implement only private path/dual-HOLD read-only locking,
stable source/input pin verification, append-only phase ledger and pure epoch/
budget validation against supplied bounded records. No UART/RPC/process launch,
current-state getter, shell, transfer, boot or query entrypoint. Pure validation
acceptance is not admission; live evidence collection remains separate. Require
independent static review of actual callsites/exception/finalizer paths before
selecting C2. No tests/build/implementation worker is queued by this proposal.

## 4. Mandatory admission tuple, not a Boolean

For **each** future phase retain private evidence keyed by capture part, raw/log
file identity, recorder SO_PEERCRED(pid,uid,gid), PID start ticks and exact canonical
argv/cwd/source, selected adapter by-id **and** resolved endpoint, sole serial
ownership, private envelope, controller/profile pins, image CRC, expected cumulative
RX/TX and classified byte intervals, phase start/deadline and predecessor receipt.
After Linux exists add boot UUID/release; U-Boot epoch uses captured banner offset,
not an invented Linux UUID. Unexpected RX cannot be discarded by taking a newer
status. No fresh peer/start tick or catch may silently replace the bound one.

Boot source119–180/204–282/320–359 supplies concrete process/coverage precedents:
canonical complete recorder options, permitted interpreter options, actual adapter
argument, duration120..14400, PID start and peer match, sole adapter owner, source
pin, capability/envelope/raw path, raw-file length versus RX count. Future parent
must establish those current facts; no old PID/receipt or flag suffices. A socket
peer mismatch sends no mutating request; source `rpc` currently drains a status
on mismatch, which is **not** authority to adopt/rebase to the new peer.

Bound coverage to the minimum of current-clock arm expiry and conservative
process lifetime (BOOTTIME minus PID-start/CLK_TCK), then map to one immutable
monotonic epoch deadline and take only earlier minima thereafter. No wall-clock
receipt from12:12, renewed per-command budget, clock rollback or capture rotation
extends it. Before each effect require remaining coverage >= remaining admitted
phase cost + transition reserve30s + recovery observation reserve120s. These
source reserves are host observation margins, **not target reset mechanisms**.
Any suspend/clock discontinuity invalidates admission rather than renewing it.

Fresh unconsumed `hold-cli` catch, no inspect mode, no active burst/inspection/
private-send window or competing transfer, and sufficient remaining capture must
be admitted before possible reset. Distinguish initial cold catch (consumed at
P2) from a separately freshly armed future-reset catch (required before GO/boot/
query as applicable); never infer armed merely because an old ACK exists. Catch
event/TX evidence must show exact source-defined keys and actual transmitted bytes.

Parent alone may send UART. Unknown owners/sockets/processes are preserved for
investigation, not killed, unlinked, stopped or replaced. While a transfer lease
is active there is no second UART/RPC/status/progress poller, recipe edit or helper;
the **single transfer loop's own** conditional-send/RX checks are necessary, not
permission for external polling. Freeze remains in force until operation return.

## 5. Phase machine — only positive completion advances

All phases below are future design, not admitted commands. Each uses the tuple
above and fresh same-epoch receipts. Any unlisted transition goes to F/HOLD.

| Phase | Allowed effect and mandatory positive successor evidence | Recovery association |
|---|---|---|
| P0 SEALED | Offline exact pin/shape/profile validation; private dual-HOLD custody/new marker, no target effect | No current recovery claim |
| P1 CAPTURE_BEFORE_ON | After actual future operator OFF report/readiness, sole recorder start with **no immediate TX**, source pin/peer/start/adapter/private capture verified, RX=TX=0/idle; fresh bounded initial hold-cli arm | Human-controlled OFF; serial open may transiently change modem lines, so reset/boot wiring detached and behavior documented |
| P2 COLD_HELD | Only after actual future ON report with P1 complete; audit source-defined automatic catch; new original banner/board5/DRAM64 and final held prompt, no kernel/flash-read handoff; proposed10s RX/TX-quiet stability | Manual OFF if catch missed/unknown; no rescue TX or second ON; initial catch consumed |
| P3 PIN_NEUTRAL | Pin unchanged bootloader via exact scratch/read/CRC; eight fixed register **reads** with historical neutral schema; no GPIO stores | Held CLI, no claimed active watchdog; manual OFF if hung |
| P4 LOADER_STAGED | 468 exact loader words at `0x82001000..0x82001750`, source/CRC exact; new future-reset catch admitted before GO | Still no watchdog while staging; manual OFF; GO/READY first-byte wait is also unarmed |
| P5 RAM_TRANSFER | One GO, one QCL header,37,535 frames of<=96B (last43B), one QEND after exact DONE/fullCRC, then COMMIT_OK and held prompt; no retry | Loader starts nominal15s guard **only upon first header byte**, refreshes at complete received frames, stops before COMMIT_OK; source association, not calibration |
| P6 RAM_CRC_BOOT | Independent full3,603,307B BZIP2 RAM CRC; only fixed boot-start watchdog pair, exact volatile bootargs/readback, one bootm; actual Starting kernel, new Linux77 release and monitor | Revalidated future catch; fixed startup guard inherited by core then `/init` attempts magic-close; not a continuous guard into Linux idle |
| P7 RO_IDLE | Actual boot UUID stable before/after, UID0/PID1init/release/cmdline/mount/module/ISP/watchdog/UART/memory and complete process/FD inventory; sole console, no device owners/jobs/unknown DMA; no writes | Inactive logical timer => manual-OFF branch, not unattended recovery |
| P8 VOLATILE_STAGE | One shell child, fresh0700 volatile directory, one fresh private client file,628 bounded chunks, exact return/size/hash, chmod only after integrity; no execution | **No admitted watchdog-owning receiver.** Only separately reviewed/manual-OFF-admitted branch may eventually enter |
| P9 QUERY_ONCE | Fresh sameboot re-admission, verified client/output/process association; final child after all setup forks/exec, one frozen client trial; only its exact watchdog and encoder calls | Client-owned nominal5s/whole1.5s phase, no feeder; manual OFF remains mandatory for inactive/unknown/late-disable gaps |
| P10 NORMAL_POSTFLIGHT | Actual two numeric snapshots **and** wait exit0, no signal, shell normal exit then actual monitor return; fresh sameboot boot/idle/UART/FD evidence; both HOLDs unchanged | Timer inactive on normal path, manual OFF availability through completion; no stop/kill/disable cleanup |
| P11 COMPLETE | Durable tuple-bound positive receipts and normal finalizer; no later native queue | Does not imply power OFF, retention, radio reset, G2/G3/G8 or product success |

Proposed P2 quiet10s is a new acceptance condition, not transfer of22b's stability
PASS. Future source must choose immutable finite caps for word staging, monitor
reads, shell launch/chunks/verification, process wait/output and postflight, and
prove they fit the single epoch deadline. Existing Boot `planned = 60 +
ceil(image_bytes/96)*0.06` is2312.1s for77; it excludes earlier468 command staging
and is not a measured timing budget for Linux staging. Existing3s echo,64B text
pieces with25ms gap,<=128B protected frame and15ms postframe pacing are source
facts, not sufficient full admission. Missing bounded phase costs block C2/live
selection, not this offline contract. No throughput assumption extends coverage.

### Explicit new cold classification (not a repair of reset22b)

Proposed parser binds a complete new capture interval **after arm and actual ON**.
Require exactly one genuine original banner, board5/DRAM64 in that interval,
audited catch TX lineage, no Starting kernel/SF boot-read handoff, no repeated or
partial new banner, a line-anchored final `anyka$` prompt, no residual password
challenge at tail, TX idle and the complete quiet interval. Distinguish:

- **PASSWORD_TEXT_SEEN:** occurrence is retained as an observation. It is not by
  itself held-prompt proof or automatic phase failure; source recorder can cancel
  remaining catch burst upon that text. A subsequent complete held-prompt sequence
  can only be accepted under this separately reviewed new parser/grammar.
- **CHALLENGE_OR_UNKNOWN:** password prompt at tail without positive held prompt,
  ambiguous/reordered/partial context, unexpected diagnostics, missing catch proof,
  multiple boots, extra unclassified RX/TX or deadline failure => HOLD, no text/key
  retry, password response, rescue or status-baseline rebase.
- **NEW_COLD_HELD:** all positive evidence above, not substring presence or silence
  alone. Prompt matching uses full line context, not an echoed command containing
  `anyka$`. Exact surrounding original-banner grammar still needs independent
  review; no private historical transcript was inspected or parser tested here.

WATCHDOG_RESET22B_RESULT remains FAILED/HOLD without reinterpretation; its later
receive-only reconciliation remains separate recovered-to-held evidence. New
parser design is neither retry nor retroactive acceptance of that controller.

## 6. Closed effect profile; no arbitrary command escape

The future new profile may review/encapsulate individual Boot transports, but
must not inherit broad `Boot.command`/`PrivateBoot` bypasses as authority. Boot
`send` retries a stale-watermark refusal up to5 times and `upload` retries ACK
timeouts up to4 attempts; **neither loop is admitted here**. Even not_sent =>
stop/HOLD, not a rebase/retry. No old22b imports/entrypoints, dynamic command input,
assertions-only checks or legacy `stock-shell`/ROOT/private_payload path.

Allowed boot-side extents/effects are exclusively:

- SPI probe/read (no write) of original boot `[0,0x60000)` to scratch
  `[0x82408000,0x82468000)`, preceded by zero scratch+zero CRC, fresh exact full read
  and original BZIP2 CRC `452a21fe`. Never stock KERNEL/ROOT reads as substitute.
- Reads at `0x08000074/78/7c`, `0x20170000/0c/18`, `0x080000c8/d0`, with neutral
  values from cold_gate49–51. U-Boot64MiB string is identity, not physical capacity.
- Loader words derived only from pinned bytes at exact aligned468 addresses,
  CRC, one GO and selected QCL bytes into `[0x82408000,0x82777b6b)`; loader-owned
  UART/L2/SRAM TX and TOP watchdog effects are those of pinned code, no other MMIO.
- QCL wire selection is one12B `QCL1` header (LE32 length/full CRC),37,535 frames
  (LE32 sequence/length/chunk CRC plus<=96 bytes), one4B QEND:4,053,743B binary TX
  total, excluding echo-checked U-Boot text/catch packets. Each ACK must uniquely
  match the actual fresh frame interval/sequence; NAK, duplicate, partial,
  out-of-order or unclassified record is HOLD, never replay or permissive search.
- Independent image CRC and the exact two **boot-start-only** TOP stores from
  ram76_boot27/77–78 (`0x080000e4` value `0x552aea54`, then `0x080000e8` value
  `0xaa000003`, once each); these are not a Linux staging watchdog route. Fixed bootargs
  are `earlycon console=ttyAK0,115200n8 rdinit=/init loglevel=8 panic=0 cma=16M@0x82800000-0x83800000`.
  Set/readback in volatile environment, no saveenv, one bootm at staging address.

Linux allowlist: fixed RO queries from postcheck24–33 with query77 release/schema,
plus newly reviewed fixed watchdog identity/timeout/node and all-process/task FD,
fdinfo/device-number inventory for aliases/inherited ownership. Do not open
device nodes to discover identity. `isp state` is cached observation, not sensor
command; modules empty and UART errors/busy0 remain mandatory. Complete inventory
must account PID1/ash/each setup child and all survivors, node rdev aliases and
standard-FD console-only lineage; missing/unreadable/unexplained entries => refuse.
Proc snapshots cannot themselves prove no prior DMA: require fresh tracked boot
and the full no-provider/no-device-effect transcript from P6, plus actual process
evidence. A clean query88 or FD listing cannot repair an unknown history.

Volatile allowlist is conditional on actual rootfs/ramfs/tmpfs and mount topology
proof, no bind/persistent substitution: one nonexisting0700 directory under `/tmp`,
one fresh client file, fixed bounded append-only decode and verification, chmod0500,
and a distinct exclusive bounded output sink if selected. No removal/overwrite/
truncate/alternate pathname/repair of partial files, no arbitrary shell text,
stdin program, heredoc, background jobs, network or credential provisioning.
Exact shell quoting, fresh-file noclobber/exclusivity, pipeline **decoder** status
and per-chunk size witnesses must be source-reviewed; a final `echo OK` or prompt
does not prove prior pipeline success.628 chunks cover627*48+44 bytes; encode
only the pinned client,<=64 base64 chars/chunk, no stream replay. Final byte count
30140 and whole SHA256 are mandatory before execution. Full command lines and
per-line/total allocations, canonical-mode echo, stdio buffering, fork peaks and
UART pacing remain missing implementation review, not waived by small chunks.
Parent's later `TRANSFER_SOURCE_BOUNDARY.md` additionally anchors decoder-until-EOF
and monitor's unchecked shell status: each chunk needs a finite producer/decoder
transaction, not a decoder waiting on interactive newline. Distinct attempt/sequence
result grammar must be disjoint from TX echo, unique after the matching TX boundary,
and reject printk splits/duplicate/missing results. Its ash1023B input-read fact is
not a safe command-length or heap limit. Those supplementary source findings are
cited, not fresh whole-ash review or an implemented observer in this packet.

No radio/firmware RUN/station/module, network/link/MTU, sensor/media/ISP capture,
GPIO/peripheral/motor/audio, flash/ENV/card/block/OTP/persistent write, reset or
installation; no retry/reload/automatic recovery boot or image fallback.

## 7. Recovery and observer boundary — unavailable guard, honest manual branch

Inspected `wifi_shell` only forks/execs ash and waits; base64 decode only reads
stdin/writes stdout. `/init` already attempts startup magic-close. Neither owns
a progress-coupled staging watchdog or transfers one to a receiver. Existing
`wdt smoke/reset`, wifi_attach or another provider cannot be repurposed as a
concurrent guard: they have other effects, compete for ownership or are forbidden.
No admitted minimal existing guarded staging primitive was found in this route.
Do not introduce unconditional feeder, raw Linux TOP writes, competing/borrowed
watchdog FD, UNKNOWN disable, or the illusion that host timeout/catch resets Linux.

Therefore **select only a proposed manual-OFF branch for later independent source
review**, not unattended staging. Before its future P1/P7/P8/P9/P10 operations,
parent must obtain actual current operator reachability, physical ability to cut
the exact camera's power without UART/other-device collateral effects, agreed
bounded observation/intervention interval, and fresh affirmative handoff covering
the entire phase+fallback. A CLI flag/old broad steering is insufficient. Loss of
contact/readiness or expired coverage before an effect refuses it; loss during an
effect retains HOLD and requires the independent physical fallback, not further
TX. **No such manual-OFF authorization/readiness is claimed or requested now.**

Client source alone supplies conditional single ownership:1.5s budget starts
immediately before watchdog open, which already arms the existing1..60s timeout;
nominal5s SETTIMEOUT re-arms internally. Only then encoder open/two queries. Budget
checks cannot preempt a blocked syscall. Actual process evidence must tie the
only watchdog description to this one final executable with no inherited device
FD, forks/dup/threads/SCM/competing writers. Catch must be freshly unconsumed and
covered. A5s metadata read is not independent physical/electrical calibration.

Final query launch must have exactly one selected executable/argv and one observed
PID/start lineage from the owned shell wait; no timeout wrapper that kills it,
second client, background supervisor or fork after device acquisition. The existing
monitor does not export shell wait status/PID, so a new bounded shell observer/
exit grammar is genuinely missing, not supplied by `RAM-only shell returned`.
Before launch finish all applet setup forks/exec, validate output sink/backing and
recheck sameboot/idle/FD/current coverage. The same single controller observes
bounded output and **actual wait status** without UART polling from another lane.

Accept two complete17-field numeric lines in frozen order, exact version1/bytes88,
nonzero equal session, NONE/owner/frame0, faults/telemetry/ledger/history0 and full
equality **and exit0 with no signal**. Stdout by itself is not completion; exit0
by itself lacks the observations. Post-retirement output failure is not a successful
query trial. Pre-effect refusal exits2..5 and output failure6 are HOLD, no retry.
Silence/timeout is unknown, never proof failstop remains alive or timer is armed.

On normal source path encoder close precedes one logical disable/watchdog close;
then stdout occurs with inactive timer. Failed/late disable may also leave it off.
No external owner is authorized to disable/close/reopen/feed/release on uncertainty.
No SIGSTOP/kill/timeout signal, terminal cleanup or freeing uncertain DMA; retained
process/FD numbers are not electrical retirement or guaranteed external custody.
CPU reset/catch invalidates Linux/FD epochs, not proof of normal close or cold
radio/rail retirement. Actual OFF/ON facts remain separate from CPU-boot evidence.

## 8. Receipts, failure finalizer and remaining disposition

Every positive receipt binds phase/predecessor, actual capture byte intervals and
source-defined TX intents/completions, exact cumulative counters, boot/release,
source/profile/client/package/config pins, RAM CRC and original monotonic budgets.
Require fresh command result schema and trailing prompt/exit as applicable; no
silent prompt refresh after unexpected data. RO postflight must repeat sameboot
UUID, release/idle/ISP/UART/watchdog/module/mount/process/FD evidence only **after
normal client exit** and shell return. No concurrent postflight while unknown work
owns descriptions. All corresponding raw bytes and identity/error evidence private.
Public receipt may state bounded scalar outcomes, public source/artifact pins and
limitations; never export boot UUID, private capturepart/owner/error/raw/digests.

Finalizer preserves the **first failed phase**, incomplete intent and all later
observations separately. Unexpected reset can create `RECOVERED_TO_HELD_NOT_PASS`
only after separately admitted receive-only classification/stability, while the
operation stays HOLD. It cannot continue P6/P7, relabel earlier PASS, replenish a
consumed catch, retry/upload/reset or clear either old marker. If even failure
recording fails, the pre-effect marker remains authoritative unknown/HOLD.

Completed here: one static design increment, selected public input hashes before/
after documentation. No implementation, compilation/generator/client/test run,
Git/index/AGENTS/central-plan/source change, target/current-state investigation,
native method admission or physical request. Native staging/query stays BLOCKED;
C1 offline source implementation remains eligible after parent selection. Source
availability and recovered CPU boot cannot close PLAT-02/06, DMA/radio-rail,
retention/backing/provisioning/main, combined-load or G2/G3/G8. Preserve upstream
licenses; this documentation introduces no license grant or publication authority.
