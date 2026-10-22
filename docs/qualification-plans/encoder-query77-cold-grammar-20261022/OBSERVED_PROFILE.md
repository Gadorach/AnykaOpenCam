# Original QC07F cold context — narrow observed software profile

2026-10-22; BASE-01/05/07 / PLAT-06/08. **Parent-prepared source grammar input,
not an implemented parser, replay fixture, live capture or admission receipt.**
It supplies the missing layout for one exact historical software/profile shape;
it does not claim the full language of every possible original U-Boot boot.

## Evidence and privacy boundary

Parent separately selected read-only offline inspection of the saved successful
October22 original cold-catch evidence. Only the initial bounded raw prefix through
the first held prompt and its corresponding historical event prefix were examined.
The original files were read without execution/modification; protected derivative
analysis and provenance remain outside Git. No serial/RPC/current recorder/process/
device/status operation, new capture, catch arm, user power action or reset occurred.

This specification contains a symbolic layout and short software/status literals,
not raw captures, event records, their paths/digests, boot UUIDs, device labels,
credentials or identifiers. Three initially withheld records were privately checked
to be generic stdin/stdout/stderr routing to `serial`, **not serial numbers**.
There are no variable or identity-bearing fields in this selected grammar.
Software build date/time below identifies a bootloader build, not a device.

Public historical anchors: `execution-20261022/ram76-resume22b/COLD22B01.json`
records the successful original bootloader/neutral gate, before loader/Linux;
`WATCHDOG_RESET22B_RESULT.md` preserves the distinct later reset-controller failure
and receive-only reconciliation. Parent's private prefix association supports
layout provenance only; it is not new source/ownership/catch/quiet qualification.
The original radio and reset HOLDs remain FAILED/HOLD.

## Exact lexical rules — no redacted wildcard fields

- Encoding: the listed ASCII bytes only; no decoding with replacement, escape
  interpretation, case folding, trimming or whitespace normalization.
- `EOL` is exactly CR LF (`0x0d 0x0a`). No bare CR/LF or interchangeable terminator.
- Quoted strings below are byte literals. Spaces inside them are exact.
- Final `PROMPT` has **no** trailing space, newline or other byte in this profile.
- The complete selected interval starts at its first byte. No last-banner slicing,
  prefix removal, caller-provided skip offset or accepting a matching subsequence.
- Additional bytes, alternative dates, optional records, variable spacing, command
  echo, ANSI/control sequences and another boot/profile are unsupported, not
  automatically normalized into this profile. Unknown context must refuse.

| Token | Exact short software/status literal |
| --- | --- |
| BANNER | `U-Boot 2013.10.0-AK_V2.0.03 (Mar 15 2023 - 16:15:35)` |
| DRAM | `DRAM:  64 MiB` |
| SPI | `anyka spi flash. bus_width:1` |
| FLASH | `8 MiB` |
| MMC_INIT | `mmc_initialize: ak_sdhsmmc_init OK!` |
| MMC_HOST | `ANYKA SDHC/MMC4.0: 0` |
| CONSOLE_IN | `In:    serial` |
| CONSOLE_OUT | `Out:   serial` |
| CONSOLE_ERR | `Err:   serial` |
| LOAD | `load_addr = 0x81808000!` |
| BOARD | `board_version: 5` |
| NET | `Net:   eth_initialize board_version:5` |
| CHALLENGE_TEXT | `Please enter password:` |
| PROMPT | `anyka$` |

The grammar is a single exact ordered production, expressed by token names rather
than an emitted transcript/fixture:

```text
COLD_PROFILE = EOL EOL BANNER EOL EOL DRAM EOL SPI EOL FLASH EOL
               MMC_INIT EOL MMC_HOST EOL SPI EOL
               CONSOLE_IN EOL CONSOLE_OUT EOL CONSOLE_ERR EOL
               LOAD EOL BOARD EOL NET EOL CHALLENGE_TEXT EOL EOL PROMPT EOF
```

Consequences: one banner; DRAM precedes board; SPI status occurs twice in these
positions; console routes occur once each; password text occurs after NET and
before a blank separator/final prompt. No password value or response is specified
or accepted. The software's64MiB DRAM string is identity text, never measured RAM
capacity; retain the40MiB map. FLASH/MMC status does not establish installation,
card contents, absence of earlier hardware effects or authority for card/storage
operations. LOAD is diagnostic text, not a command or permission to use that range.

## Classifier contract this packet can support

A future **pure bounded source library** may consume one immutable bytes argument
of at most64KiB. It must inspect the entire argument against this exact production
without opening a capture or constructing a live epoch. It may return only fixed
offline context facts: exact-profile match, proper-prefix/incomplete, unsupported/
contradictory bytes, bounded matched-token count and whether the complete expected
challenge record was observed. No raw token/byte dump, digest or private identity.

A proper prefix is not a match, including a partial banner/prompt or the complete
challenge without the final prompt. Bytes past EOF are not a match. Unexpected
kernel/flash-read/second-banner/echo/control context cannot be accepted by searching
for later positive tokens. A mismatch must not be repaired by stripping or retry.
No broad alternate date, arbitrary-line rule or optional prompt-tail branch.

Password **occurrence** remains distinct from positive complete context. For this
profile it is expected before the final prompt; rejecting that occurrence alone
would reproduce the old substring-classification defect. Conversely, the password
record alone never proves held context and triggers no response/automatic key TX.
This is new source reasoning, not a relabelled PASS for the old failed controller.

## Missing live requirements — never supplied by a byte match

This historical first-prompt grammar does **not** prove that an interval selected
by a caller is authentic, begins after a fresh arm/actual ON, includes all earlier
bytes, or belongs to the current boot/owner. Whole authenticated capture selection,
source/profile pins, private RX/TX event lineage, no competing transfer, actual
future catch, immutable coverage and10s RX/TX quiet remain separate parent gates.
The grammar's first prompt is not evidence of that later quiet interval. A caller
must not discard extra later bytes to obtain an exact match and claim stable held.

No new grammar branch is inferred from the later watchdog-reset transcript. A
different future interval/profile stays unsupported until separately reviewed;
there is no fallback to last-banner parsing, old controller or trial retry.
Logical CPU recovery does not establish cold radio/rail/DMA retirement.

## Next source selection, not execution

Parent may separately select a fresh pure classifier candidate from this narrow
grammar, with exact token/order/EOF/limit checks and ordinary independent review.
No candidate/helper/generator/test or classifier invocation is selected here.
No raw/synthetic fixtures, full-controller stub, serial/RPC/send/capture reader,
clock/catch/status getter, boot/staging/query/reset or physical request follows.
Actual method/recovery and unguarded staging remain blocked independently.
