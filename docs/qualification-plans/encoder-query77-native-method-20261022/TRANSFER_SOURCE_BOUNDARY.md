# Volatile transfer — additional source boundaries, not a sender

2026-10-22; ENC-02 / PLAT-05/06/07/08. Offline public source reading only.
This supplements, and does not alter, the19-pin investigation. No generated wire
commands, native controller, test, shell/client execution or hardware access.

## Findings that constrain any future implementation

1. **Monitor return is not an exit-status receipt.**
   `bringup/qc07f/initramfs/wifi-monitor.h:23–36` starts ash with fixed environment,
   waits through EINTR, then prints the same return text without checking child
   wait status. A shell exec failure can also reach that monitor return. Neither
   that text nor `qc07f>` proves file transfer or query-client success. A future
   shell channel needs a distinct source-reviewed framed command-result protocol
   and explicit child status plus the two validated query records. No such protocol
   is implemented by the existing monitor.
2. **Boot.command is not a Linux shell transport.**
   `scripts/qc07f_boot.py:437–458` requires the U-Boot prompt, checks exact command
   echo before CR and waits for its marker. Its framing cannot be reused merely
   by changing a marker argument: the held-U-Boot check remains, and ash/TTY echo/
   prompt/printk behavior needs a separately reviewed transaction implementation.
   Never bypass that check with a superclass call to obtain arbitrary shell TX.
3. **Small decoder buffers do not establish a process allocation budget.**
   Exact BusyBox source `libbb/uuencode.c:207–276` decodes using an82-byte local
   buffer. The `baseNUM_main` decode branch is selected at
   `coreutils/uudecode.c:286–297`; it reads until EOF, not until one UART newline.
   Therefore a finite pipe input is required; writing base64 directly to an
   interactive decoder leaves it waiting for EOF and is not a bounded chunk method.
   Ash's compiled no-editing path uses IBUFSIZ1024 and reads up to1023 bytes
   (`shell/ash.c:3004,10828–10911`); that is an input read size, not an AST/string/
   expansion or total memory ceiling. Libc stdio, pipes, processes and all live
   allocation paths still need a budget. No128KiB exception or total RSS claim.
4. **Decoder exit alone is insufficient integrity evidence.**
   `read_base64:262` does not inspect the immediate fwrite return. The applet has
   its own final stdout flush/error handling; pipeline producer/consumer outcomes
   and redirection still need explicit treatment. Regardless of exit status,
   require actual destination length after every chunk and the final whole-file
   length+SHA256. A failed/ambiguous chunk is terminal, not replayable. Reject any
   later client execution when byte accounting or the final digest is missing.
5. **All transfer helpers precede device ownership.**
   Ash may fork for pipes/applets and command substitutions. The client itself
   acquires watchdog/encoder only after exec. No device description may be opened
   in ash and inherited by that chain; no shell watchdog feeder is selected.
   The query client performs normal disable/close before numeric output. Thus a
   later output collection or shell wait can block with no watchdog running.
   Parent must bound output/lifetime and retain manual OFF fallback, rather than
   treating silence after nominal5s as guaranteed recovery.

## Required bounded transfer shape — proposed, not admitted commands

- Select only the exact reviewed30140-byte ARM client, with its existing SHA256.
  Chunk size remains<=48 binary bytes,<=64 encoded characters, exactly628 chunks
  for this ELF (627 full chunks and one44-byte tail). No general payload selector.
- A future reviewed generator must enforce a fixed ASCII grammar and short absolute
  volatile paths. Encoded input uses only standard base64 alphabet/padding, never
  evaluated shell text. One finite producer/decoder transaction per chunk; no
  here-document, input-EOF trick, unbounded loop or continuous stream.
- Root mount must actually be proved volatile before exclusively creating a new
  mode0700 directory. Initial file creation must refuse existing files/symlinks;
  later appends require actual same-file ownership and single serialized writer.
  Shell noclobber alone is not proof of ancestry/mount/file association. A partially
  written file remains local evidence and must never be executable on failure.
- Each transaction must carry a fresh attempt/sequence binding. Accept only the
  unique completed response after the matching TX boundary, never a token found
  in echo, a prompt or an earlier capture window. Expected result grammar and
  echoed command grammar must be disjoint. Any unexpected RX, printk split,
  duplicate/missing result, timeout or partial TX means sticky HOLD, no resend.
- Seal and check complete command byte length, parser/TTY limits,<=64-byte pacing
  and actual recorder pacing capabilities. U-Boot's900-byte command cap is not
  permission for a900-byte ash transaction. Command counts, byte counts, capture
  bounds and one immutable total deadline must be derived from the exact wire
  grammar before code is selected; no invented throughput or native timing PASS.
- File verification, mode change and client launch are distinct gates. Mode change
  cannot repair a bad transfer; an executable bit is not launch authority. Query
  launch requires a separate fresh admitted one-shot trial and sameboot lineage.
  Exact numeric query output plus observed child exit0 and sameboot postflight are
  all necessary; they are not supplied by the monitor's unconditional return text.

## Smallest next source increment

First complete the fresh dual-HOLD/epoch/recovery contract and its independent
review. Only then select a pure, non-sending encoder of this fixed payload/grammar
or a separately reviewed shell transaction implementation. Neither exists here.
An inactive-watchdog Linux staging interval cannot inherit the loader/startup or
query guard. No admitted guarded receiver exists; a future manually recoverable
branch needs actual operator availability and separately reviewed handoff before
any staging. Source work can continue without requesting physical action now.

The frozen images, monitor, client, old controllers and both HOLDs are unchanged.
This document establishes source limitations, not a replacement native recipe or
authority to run an existing shell, uploader, reset or station shortcut.
