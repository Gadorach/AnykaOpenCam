# Query77 future boot/volatile-client/query — offline method findings

2026-10-22; ENC-02 / PLAT-01/06/08. **Investigation only. No executable native
recipe, capture start, UART/RPC/status/device access, staging, boot or query selected.**
No current physical/runtime/owner/recovery facts are inferred from12:12UTC U-Boot.
Original radio and reset-controller failure HOLDs remain independently sticky.

## Frozen inputs and actual missing bridge

- Accepted offline query77 image:3,603,307B, SHA256
  `f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e`.
  Release `6.18.55-qc07f-encoder-query77`; unchanged DTB/initramfs/monitor. Never booted.
- Source-reviewed conditional query client: ARM30140B, SHA256
  `87a40f291600fee1296d96030726b9db825956e274e0db2a7ee5ea18f6a26ac6`.
  The executable is not embedded or staged; no monitor command invokes query88.
- Full native effect/recovery/ownership admission is still absent. The old22b
  boot/postcheck/smoke/reset controllers cannot name this image/runtime or a new
  epoch, and must not be rerun or edited to masquerade as a new trial.

## Source-supported reuse, not current execution authority

`scripts/qc07f_boot.py:61–95,525–651` defines the original checked legacy image
format, pinned bootloader/word-aligned RAM loader, bounded CRC/ACK transfer,
volatile bootargs and boot handoff. Query77 retains its layout/addresses and
static bounds, not an observed boot result. Existing helper bytes stay immutable.
Only a fresh separately reviewed controller may reuse those exact transport
primitives with the new input pins and a freshly established cold-boot epoch.

`qc07f_private_root_transfer.py` supplies the protected capture envelope and RPC
capabilities; its stock ROOT payload path is **not** a query77 input validator.
Its narrow command profile cannot simply be bypassed by calling old CLI commands.
A new explicit profile must bind only the needed pin/RAM/CRC/boot/read-only/volatile
file operations. Preserve raw UART, boot UUIDs, private owner errors and captures
outside Git, even though the selected image/client hashes are public-code pins.

`ram76-resume22b/preserved_hold.py:25–40` refuses a nonempty22b subordinate HOLD.
That is now the failed reset marker, so it is not a renewable operation adapter.
A fresh owner must lock/compare **both** original failure files without changing
their bytes, then own a separate sticky new-operation marker. No automatic clear,
reset retry, rescue TX or assumption that elapsed time retired uncertain owners.
Unknown socket/process/serial owners must be preserved/investigated, never killed
or overwritten to obtain exclusivity.

## Volatile client delivery is feasible at source scope

The image need not be changed to provide a possible delivery mechanism:

1. `bringup/qc07f/initramfs/wifi-monitor.h:23–37` forks/execs the actual BusyBox ash
   child on the console and returns to the monitor after its real wait. It opens
   no sensor/frame/encoder/watchdog and performs no network operation. All helper
   fork/exec must precede the qualification client's device acquisition.
2. Embedded `/bin/busybox`378104B/SHA256
   `3f7e15eae5f1c03a0af78ce45a341aaa48ad0eb0a00850a0914512e830b7c3a1`
   matches the retained `build/wifi-userland3/busybox/busybox` byte-for-byte.
   Independent ELF parsing found its `.text`329004B, `.rodata`44741B, `.data`501B
   and `.ARM.exidx`8B identical to the same build's symbol-bearing ELF. Real
   `baseNUM_main`, `ash_main`, `md5_sha1_sum_main`, `wc_main`, `chmod_main`,
   `mkdir_main` symbols are present there. Neither ARM ELF was executed.
3. That build's config/generated applet table enables ash, base64, printf, wc,
   sha256sum, mkdir and chmod. Exact source `coreutils/uudecode.c:279–297` supports
   base64 `-d` from stdin. This is stronger than assuming an applet from a name;
   actual shell/option/transfer behavior is still unqualified on query77.
4. The initramfs has `/tmp`; PID1 mounts only devtmpfs/proc/sysfs. A future admission
   must actually verify `/` is volatile rootfs/ramfs/tmpfs and has no persistent
   backing/mount substitution, then exclusively create a fresh mode0700 directory
   and nonexisting file. No writes to flash/card/block devices or original files.
5. Candidate wire design: at most628 independently bounded chunks of<=48 binary
   bytes, each encoded to<=64 base64 characters. Only the immutable client bytes
   may be encoded. Decoder output appends to that one owned volatile file; each
   step needs actual return/size accounting and final30140B + whole SHA256 proof.
   No shell/file/stdin stream is permission to execute unverified bytes. No chunk
   repair/retry, overwrite, fallback path or execution after an incomplete transfer.
6. A future controller must inspect the exact ash/decoder/stdio allocation and
   quoting paths, seal every wire command/grammar and bound UART pacing/line size/
   total time. No unbounded here-document, full-AU copy or allocation-cap increase
   is admitted by this investigation. Source availability alone is not a reviewed
   Linux transfer controller or proof of a target memory/latency budget.

The generic BusyBox delivery above is a **proposed source method**, not a command
queue, tested transfer or native-ready assertion. No target generator ran.

## Required new packet order and recovery limits

The next controller design must separate these positively completed phases:

1. Fresh physical OFF/ON handoff if a current cold held boot cannot independently
   be established; exact sole adapter/recorder and bounded protected capture/catch
   admitted **before ON**, no competing operation. A source flag is not a user's
   physical report. Old22b's consumed catch/deadline/PID/boot UUID are not reusable.
2. Actual new original-U-Boot catch/identity/stability, pin/neutral readbacks,
   original loader source/CRC, exact query77 upload/full independent RAM CRC and
   volatile bootargs/handoff. No flash write/module/station/radio/GPIO/media effect.
3. Actual new release/boot/UID/PID1/mount/module/ISP/watchdog/UART/memory read-only
   idle evidence and console-only FD/proc ownership before any volatile write.
   Revalidate own capture/catch/time budget; snapshots are not a media load budget.
4. Exclusively bounded volatile client transfer/integrity and admission of the
   actual process/output/lifetime/recovery association. CPU-only staging has no
   existing admitted watchdog-owning receiver; **do not add a feeder, steal an FD
   or claim watchdog coverage**. A separately reviewed manual-OFF fallback or a
   genuinely guarded transfer method remains a design/admission requirement.
5. One future explicit query trial: exact image/client, no other provider/jobs/
   unknown DMA/inherited or competing descriptions; actual single watchdog5s,
   immutable client1.5s phase, two query88 snapshots and observed stdout+exit.
   Source/client conditional sufficiency is not proof these preconditions hold.
6. Fresh sameboot normal retirement/postflight and monitor return, or sticky HOLD.
   An unexpected reset may be caught but never automatically relabelled trial
   PASS, reload/retry or clear either original failure. Failed/late disable may
   remove the timer; manual OFF can be genuinely required. No deliberate reset.

Neither host deadlines nor a future catch can reset an inactive/hung Linux target.
Fresh physical/sole-owner/recovery evidence and sufficient bounded coverage must
be checked for **each** effect. No controller may relax the failed22b password-text
classification in place or hide its failure; a new cold-boot parser needs its own
source review of actual original banner/held-prompt context.

## Disposition / next source boundary

Offline investigation establishes reusable immutable inputs and an applet-backed
possible delivery route. **Native execution remains BLOCKED** on a fresh complete
controller/profile/HOLD guard, reviewed bounded Linux transfer/observer method,
recovery/manual handoff and actual current admission. No executable recipe or user
physical action is requested yet: source/method work remains eligible first.

Keep original75/76/77 images/recipes intact. Embedded station76 remains unpaired
and forbidden. This packet cannot adopt app/provider/failed-NEXT/backing/listener
composition, claim full PLAT-01/02/G2/G3/G8, change memory/CMA limits or select
persistent installation. `INVESTIGATION_INPUTS01.json` records only local public
source/artifact input pins, not private/current target state or operation results.
