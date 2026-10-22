# C2a independent review — narrow conditional SOURCE/API and byte-custody sufficiency

2026-10-22; PLAT-01/08 / ENC-02. **No correction-required ordinary defect found
in the selected fixed-public-input validator and its read/error/resource closure.**
Conditional SOURCE/API and bounded read-time byte-custody sufficiency only, under
the stated serialized host namespace/descriptor contract. This is not an observed
validator success, public-filesystem runtime qualification, source adoption,
composite input freeze, C2 controller, method/recovery or native admission.

## Exact source and review method

- `apps/qualification/encoder-query77-inputs/selected_inputs.py`:376 lines,
  18,643 bytes, SHA256
  `5fbcc656882075c44c1e4f4e5df8a896fe83433a33e770deec1fd50c3792d3f0`.
- Two-file source aggregate:
  `9456c227e5f4d66d2e55fbbb44694157c7c48e6785dd1b3b5a6f906b6d8b8565`.
- Read full contract, BUILD_RESULT, README, actual module and author inspection;
  inspected all definitions/callers and method/catalog/discovery/frozen/syntax
  receipts. Independently read actual PACKAGE02, compiler/release text, six exact
  config lines, public UAPI and relevant public build-path source slices. Build
  source/JSON argv are evidence, never execution instructions.
- Public reads, bounded streaming SHA256, JSON/AST/literal-schema inspection and
  direct client ELF header/table reads only. No candidate/helper import or call,
  compile/syntax rerun, eval/exec, CRC/function trace, generator, test, synthetic
  observation, CLI/controller/client/output execution or private filesystem exercise.
  No actual HOLD/control/capture/current-state/proc/dev/UART/RPC/network/device or
  target process operation. Inline stdlib inspection never executed candidate code.
- Only this new review directory is written. Prior v1/v2 sources/reviews/design/
  failures, candidate source, Git/index, AGENTS/central plans, helpers/model/provider/
  safeguards/licenses remain untouched. This is a focused ordinary correctness
  review, not a broad filesystem/security/concurrency audit or adversarial campaign.

## Closed API, selection and read-time byte custody

Source anchors below refer to the exact376-line module.

| Area / anchors | Independent ordinary finding |
| --- | --- |
| Declarations1–61; API351–376 | Sole operational API takes no arguments; exact canonical root and25 fixed public pins. AST-decoded strings/integers/concatenations match the selected manifest in order, with unique labels/paths. No environment/root/path/digest selector, CLI/main, authored import-time IO, callback, old-module import or output writer. Standard declaration dataclasses/enums are not authored filesystem acquisition. |
| `_Read.open`145–169 | Only a manifest member is used. Absolute directory-FD traversal from `/`, bounded components/path, no empty/dot/dotdot/.git path-component aliases, DIRECTORY/RDONLY/NOFOLLOW/CLOEXEC/NONBLOCK. Ancestry trusted uid/root, no022, held-descriptor/path associations checked during descent. Public regular file precheck requires trusted owner, single link, exact size, no special or022 mode bits; RDONLY/NOFOLLOW open then full metadata comparison. Safe600/644/755 public files are not subjected to private0700 policy. |
| `_dirs_check`, `_file_check`132–143 | Directory associations pin inode/type/mode/uid/gid; regular metadata also pins nlink/size/mtime/ctime and current path-to-FD association. No atime pin; directory timestamps/link counts deliberately not frozen. Reads never repair metadata. No atomic regular-file-only-open claim against concurrent same-UID substitution or privileged mounts. |
| `_Read` partial failure128–198; `_observe`331–348 | Ownership slots exist before acquisition. Each directory FD is retained before metadata capture; an ordinary fstat failure leaves a closable slot even if its saved metadata is unset. File baseline/name are assigned before file open, and an acquired file slot is retained before its immediate check. Finally always attempts directory postcheck and independently the available file check, even after read/hash/format/open failure. Missing file FD is not opened to fill context, and absent baseline does not become a new pin. |
| Close184–198; errors76–90,338–348,373–376 | File slot clears and directory slots pop before one close; all remaining owned closes are attempted after an error. No numeric-FD retry, destructor or false retention claim. Fixed primary/postcheck/cleanup codes survive public rethrow with original chain display suppressed; no path/raw/OS error formatting. Cleanup uncertainty is non-OK even without a prior read failure. No result escapes `_observe` until all that file's owned descriptors have closed successfully. |
| `_stream`294–328; API358–372 | Exact4096B-or-smaller reads, full SHA, one-byte EOF check, then metadata postcheck/close. Image/zImage/initramfs are never retained whole. Known small inspection buffers are<=65536B; largest selected retained file is58,859B config. UAPI byte copies are only2455B each. API collects fixed facts and compares retained UAPI bytes after per-file teardown. OFFLINE_NOT_ADMISSION has no live owner/catch/clock/HOLD/GO or effect field. |

The custody claim is bounded to one guarded public read and its retained bytes/
fact. Descriptors close sequentially; later input changes, executing-code identity
and a future controller's use of those bytes are **not frozen** by the result.
The code and README explicitly disclaim that stronger association.

## Image, package/config/UAPI and client-format reasoning

### Streamed image and CRC/spans

`_bzip_update`201–207 is the nonreflected bitwise recurrence: byte enters bits31..24,
top bit selects polynomial04c11db7, result masked32-bit per bit. `_stream` starts
whole-image CRC atffffffff and applies final xorffffffff. This is not zlib's
reflected CRC. No CRC helper, trace or test vector was run by this reviewer.

At307–325 the image is separately SHA-pinned before format acceptance. Header is
the first64 bytes only; payload IEEE skips exactly those64 bytes in the first
chunk and includes all later chunks. Header IEEE zeroes just bytes4..7. Big-endian
`>7I4B32s` compares all legacy words/OS5/ARM2/type2/compression0 and the32-byte padded
name, timestamp0, payload3603243/load82408000/entry82408040. Expected header/payload/
full CRCs are011cd017/0d55fce6/63ea4eac.

Intersection arithmetic312–315 correctly bounds the two spans:
`[64,3600736)` is3,600,672B zImage; `[3600736,3603307)` is2571B DTB. Chunks outside
a span do not update it; crossing chunks contribute only their intersection.
Both span hashes compare to independently selected constituent-file pins. Whole
image is3,603,307B, below the unchanged4MiB transfer ceiling. No full image,
zImage, initramfs or megabyte-region buffer is formed.

### Exact selected metadata, not fresh build or memory proof

- `_package`222–243 matches the actual4011B PACKAGE02 pin dictionaries, selected
  image path/release/header/whole CRC, static RAM/kernel/staged/decompressor/CMA
  associations and `station76_paired is False`. It does not follow package argv or
  accept a generic PASS flag. Static addresses agree with the pinned package;
  neither this source check nor the reviewer redoes decompression/DTB/ELF-kernel
  extents or establishes live CMA/headroom.
- `_small`276–286 checks five exact Clang23.1.3/localversion identity lines once,
  plus the actual retained OLD native-usb76-source01 initramfs path. The latter is
  intentional and matches actual config143, not an erroneous source02 rewrite.
  Generated release/compiler text matches the selected bytes. The2,082,816B
  original initramfs reference is stream-hashed only. No old76 compiler/ABI pairing,
  fresh five-difference comparison or250219-path reproduction is inferred.
- Three exact2455B UAPI pins and byte comparison361–365 preserve reviewed header
  identity. Version1/88B/identity32/reserved84/ioctl80585166 meanings remain tied to
  that header/review, not a new C compile or syscall. No old76/56-byte fallback.
- Review association289–291 checks the C1v2 seal's output entry against the actual
  pinned REVIEW path/length/SHA. It is receipt association, not approval of this
  new validator, actual custody or a generic permission to execute anything.
  Loader/recorder/Boot/protected/C1v2 sources are pins only, never imported/run.

### Client ELF basic structure

`_elf`246–270 receives only the exact30,140B SHA-checked client, not arbitrary
caller bytes. It checks ELF32 little-endian EXEC/EM_ARM, header/table sizes and
bounds, EABI5 soft-float and no hard-float flag, bounded program/section counts,
file/address extents, LOAD filesz<=memsz/alignment, no WX LOAD, an entry inside a
file-backed executable LOAD, exactly one RW/non-exec GNU_STACK, no PT_INTERP/
PT_DYNAMIC/SHT_DYNAMIC. With no dynamic table/section, there is no NEEDED table
to follow. Bitwise/comparison precedence at251,259–261 is correct (`&` binds before
the comparisons; address-size arithmetic is unbounded Python and explicitly
limited to32-bit ranges). No ELF load/exec/disassembly or ARM-attribute parser.

Independent direct public header/table reads—not `_elf` calls—found ident
`7f454c46010101000000000000000000`, type2/machine40, flags05000200, entry10698,
52-byte header, PH52/32*5, SH29140/40*25 (end30140), string index24. Program
headers contain RX LOAD at10000 with file size14028, RW LOAD at14fbc with
file96/memory440, one RW GNU_STACK, ARM_EXIDX and GNU_RELRO; no INTERP/DYNAMIC.
Section types contain no SHT_DYNAMIC. These selected values satisfy the source's
basic bounds/flags/entry conditions. ARMv5TE remains tied to the exact reviewed
file pin, not newly parsed/proved here. This client is not the host verifier, and
neither client nor verifier timing/memory budgets were changed.

## Fresh receipts, context drift and inspection error

`INPUT_PINS01.tsv` seals75 unique explicit public reader inputs. Groups overlap:
25 selected,5 discovery,35 inherited review inputs, current source/docs and prior
review receipt files are not additive unique counts. Snapshot18:41:42UTC follows
source reading and the corrected static checker, before this report; it is not a
pre-use/build seal. Final comparison/output pins are in FINAL_SEAL01.

- All25 selected pins and five discovery pins match. AST catalog matches all
  selected paths/lengths/SHA without importing the module, using eval or invoking
  `_Pin`. All nine author METHOD output pins, exact source/syntax pin and authored
  IO-callsite receipt match. Author HOST AST/syntax PASS is cited, **not rerun**.
- Inherited C1v2 review35 inputs:32 frozen match; the same three contextual
  checkpoint documents changed. FROZEN_CHECK retains the author's then-current
  pins; STATIC_CHECK records original/author/reviewer context pins and the further
  parent delivered/active-review checkpoint drift. No manifest is rebased; no full
  historical diff attribution or transitive250219-path build qualification.
  All four prior C1v2 review output/seal pins match, including seal b58ec94b... .
- One initial reviewer static receipt checker failed with a **KeyError** because
  its own map omitted prior INPUT_PINS01.tsv (and STATIC_CHECK01.json). No files
  were written and no candidate ran. Those two explicit public receipt inputs
  were added to the checker inventory; corrected static comparison completed.
  This is retained as a reviewer bookkeeping error, not hidden or attributed to
  the candidate. No policy refusal, policy retry or tool/provider reroute occurred.

## Limits and handoff

GPL-2.0-only source notice retained; no generated ELF/image/stock payload/license
grant or publication selection. Bounded authored buffers are not total Python/
stdlib4MiB RSS, native128KiB allocation/headroom, latency or combined-load proof.
No ceiling increased. Serialized namespace/FD assumptions are conditional, not
special-file-open race immunity, privileged mount/alias proof or signal/thread/
async-allocation atomicity. No actual permission/path/cleanup/durability run.

No validator invocation, source adoption, C2 controller/parser/staging/boot/query/
watchdog/retention/recovery/native/main/product or physical action is selected.
No current owner/expiry/catch/manual readiness follows; historical12:12UTC U-Boot
and consumed catch remain historical, both actual HOLDs untouched. Parent alone
owns hardware. No automatic call/implementation/retry/build/test/worker/native queue.
