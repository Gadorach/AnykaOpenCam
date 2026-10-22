# C2a selected query77 inputs — unreviewed offline SOURCE candidate

`selected_inputs.py` is GPL-2.0-only, self-contained and has no CLI/main/package
bootstrap, example launcher, authored import-time IO, old-controller/C1 imports,
output file or effect interface. Its one operational API,
`validate_selected_inputs()`, has no arguments: only25 explicitly pinned public
files at exact canonical workspace-relative paths can be selected. It is **not
called/imported during this delivery**. Parent review must precede any later
explicit public-byte-inspection call. Read the pre-code contract and source/
syntax/method receipts in `docs/qualification-plans/encoder-query77-inputs-20261022/`.

## Source behavior, not an observed runtime result

- Fixed SHA256/exact sizes for image/package/config/zImage/DTB/initramfs reference,
  generated release/compiler metadata, client/C/three UAPI copies, loader/recorder/
  primitive/C1v2 source and selected build/client/C1 review receipts. Paths come
  from the retained build/config/packager receipts; never interpret returned
  firmware/source/JSON argv or paths as instructions/selectors.
- Anchored directory-FD traversal from `/`, NOFOLLOW/CLOEXEC/NONBLOCK, safe uid/mode
  ancestry, prechecked regular single-link file, exact path/FD metadata before/
  after read. Public sources may be600/644/755/444 as appropriate; no private0700
  policy applied to source trees. No metadata repair/write/Git call.
- Every large file hashed in4096B preads, never retained whole. Image CRC32/BZIP2
  and IEEE header/payload checks stream; separate zImage/DTB span hashes bind
  their exact byte concatenation to independently pinned constituent files.
- Only exact small selected JSON/config/ELF/UAPI/release/compiler files<=64KiB
  may be retained. Client ELF header/program/section bounds, ARM EABI5 soft-float,
  executable-backed entry and no INTERP/PT_DYNAMIC/SHT_DYNAMIC/WX LOAD/exec-stack
  checked. No generic ELF loader, execution, section/ARM-attribute disassembler.
  ARMv5TE attribute meaning is tied to the exact previously reviewed30140B pin.
- Three UAPI copies independently hash to the frozen2455B pin and actual retained
  small byte strings compare equal. Version1/88B/identity32/reserved84/ioctl80585166
  semantics come from that reviewed header pin, not a new C compile or native ABI.
- Package field associations must match selected image/zImage/DTB/initramfs pins,
  exact legacy header/release/CRC and static ranges, not generic PASS/G2/G3 flags.
  C1 review seal is associated with its actual pinned REVIEW.md bytes; no approval
  flag grants execution or marker custody. Config five identity lines plus exact
  initramfs selection, generated release and compiler metadata are pinned, not
  an independently repeated76 comparison/full250219-path build.

## Returns and errors

Returns frozen `OfflineManifest(scope="OFFLINE_NOT_ADMISSION", files=..., limitations=...)`
with frozen `InputFact` records containing only public label/size/SHA and a narrow
offline inspection name. No ready/GO/native phase PASS or owner/clock/catch/HOLD/
recovery/live-state field. It contains no raw artifact/OS metadata or private data.
All descriptors have already been closed before return; results are not future
input immutability, loaded/running-source or actual controller/capture association.

`InputsError` exposes only fixed `Reason` codes: primary, metadata-postcheck,
cleanup uncertainty and overall code. OS/path/raw exception chaining is suppressed;
no error strings/logs/output files reveal selected paths or host/environment data.
On failed reads/hash/format, available root/file metadata postchecks are attempted;
the original primary is preserved. All owned descriptors clear/pop before one
close, remaining closes attempted, no retry/falsely retained numeric FD. Cleanup
uncertainty is non-OK and no result is returned. There is no destructor or resume.

## Conditional limits

This is a serialized trusted host namespace contract, not immunity to same-UID
replacement, privileged bind/mount/namespace changes, hostile monkeypatching or
signal/thread/async-allocation interruptions. Linux has no general atomic
regular-file-only-open guarantee here: precheck/open/postcompare refuses observed
drift, not a claim that no substituted special file could ever be opened by a
concurrent same-UID actor. Exact fixed source/manifest constants are not an
authentication boundary. No arbitrary/root-selector/private file probes exist.

Measured40MiB and static kernel/staging/decompressor/CMA ranges are only checked
against pinned packaging/parent metadata. No fresh decompression/ELF-kernel memory
proof, native457352B headroom, total Python4MiB RSS or128KiB/native combined-load
claim. No allocation/transfer/RAM/CMA ceiling changed. Original initramfs reference
is streamed/hash-pinned; no claim of independently decompressing/locating it in
zImage here. Embedded station76 remains unpaired/unsupported, forbidden to load/
autoload; absence of loading code is not an observed no-autoload result.

Source pins do not establish old76 compiler/ABI pairing, watchdog guard/recovery,
DMA/job/global idle or media retention. Query77 remains never booted. Both actual
HOLDs and historical12:12UTC U-Boot/consumed-catch fact remain untouched; no fresh
runtime/owner/expiry/manual readiness. No source adoption or automatic next queue.
HOST AST/syntax compilation and static public hashes only; no validator/helper/
client/generator/compiler/test call, private/current-state or target access.

Retain upstream GPL notices and original Linux GPL-2.0 text referenced by the
query-client report. Python/stdlib/toolchain/retained payload licenses remain
separate; no new global license or artifact/publication grant. No generated image,
ELF, initramfs payload, credentials/capture or wire/controller artifact is copied
into this directory or selected for Git/publication.
