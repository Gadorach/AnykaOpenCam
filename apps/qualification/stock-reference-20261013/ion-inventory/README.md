# Stock ION allocation/mapping inventory — no capture

One fixed8MiB allocation from retail heap **id1/mask2/type2**, one exported fd,
one full read-only mapping, bounded metadata-only pagemap inspection, and known
allocator cleanup. No mapped payload byte is read, written, hashed, zeroed or
prefaulted. No camera/ISP/UIO/watchdog, frame queue, register, SD or network API.

## Delivered payload and CLI

`evidence/delegated/20261013-stock-reference/ion-inventory/build01/stock-ion-inventory-armv5`

- SHA256 `023f520f8e43185a9dc96908a6e4b2c747662aaf661cc970205ef714918ed680`
- **39,680 bytes**, static ARMv5 ELF32 little-endian/EABI5 soft-float.
- No interpreter/dynamic segment; non-executable stack. Native never executed here.
- **78 host-MOCK cases passed**. Exact commands, inputs, logs, map, ELF inspection,
  source snapshot and hashes are under build01.

```text
<parent-proven-absolute-RAM-payload-path> --native-stock-ion-inventory
```

No other mode/path/size/mask/flags/address option exists. Absolute argv0, exact argc
and selector, real/effective UID0, non-PID1 child, Linux3.4.35 and page size4096 are
required. The native executable has an ARMv5 compile guard. Run as a child of the
rescue shell, never replacing PID1. Parent's wrapper must prevent inheritance of
its watchdog fd3; the helper neither knows nor closes an inherited watchdog fd.

## Parent eligibility / current table binding

Parent must freshly admit exact original image/kernel/provider pins, sole owner,
no other ION/UIO/video/ISP clients, no prior queue/start/DMA in this epoch, suitable
normal-RAM headroom for this small process, and existing externally owned stock60s
recovery/user power recovery. This unit does not inspect fuser/global proc, change
watchdog policy, or create a recovery mechanism. A past timestamp is not authority.

Already supplied evidence binds the current table; no extra platform probe is
needed by this helper:

- `evidence/qualification/qc07f/stock-reference-20261013/retail-ION-pdata-static-binding01.json`
- `evidence/qualification/qc07f/stock-reference-20261013/actual-stock-ION-public-pdata-readback01.json`

Tuple: one heap, type2, id1, Reserved phys Memory, base0x80000000,
size0x01800000, align4096, private0. **Mask2 is derived from id1, not the heap-type
enum macro whose bit would be4.** The table is not a free-pool/allocation/cache
proof. Exactly one8MiB request is allowed; ENOMEM is a legitimate failure, with
no alternate size/mask, eviction, OOM experiment or retry.

## Exact operations and ABI

1. Arm10-second process alarm/non-restarting ALRM/INT/TERM handlers; validate a
   monotonic clock. lstat `/dev/ion` must show a nonsymlink character node.
2. Open that fixed node with `O_RDONLY|O_CLOEXEC|O_NOFOLLOW`; fstat must match
   device/inode/rdev identity. Parent still supplies exact provider identity.
3. ALLOC`0xc0144900`,20-byte ARM32 packet: len0=`0x00800000`, align4=`4096`,
   heap-mask8=`2`, flags12=`3`, output opaque handle16. No handle is interpreted
   as a physical address or emitted in output.
4. MAP`0xc0084902`,8-byte packet: handle0, signed output fd4 initialized−1. Validate
   handle/canaries, fd>=3 and distinct from ION client. fstat exported fd must match
   old3.4 anon_inode semantics: permission bits, **no file-type bits**. Requiring
   modern S_IFREG here would incorrectly reject the reviewed old provider.
5. `mmap(NULL,8388608,PROT_READ,MAP_SHARED,dmafd,0)` once. No requested virtual or
   physical address, executable/write permission, whole24MiB mapping or payload
   access. Check returned page alignment and nonwrapping ARM32 virtual extent.
6. Open only `/proc/self/pagemap` read-only/CLOEXEC/NOFOLLOW. Its file offsets are
   derived exclusively from that returned mapping. No maps/pagemap sweep or
   physical-base environment override.
7. Read the2048 own-page entries in64 chunks of at most256 bytes. At most two
   pread attempts per chunk,128 calls total. Each offset/count is8-byte aligned;
   partial reads or EINTR have a finite continuation allowance. EOF/misalignment/
   repeated EINTR/error produces HOLD; no touching mapped pages to force validity.

Packets have fixed zero initialization and canaries. An unexpected positive ioctl
return, mutated fixed fields or invalid handle/fd is never treated as success.
The stock ION header was compile-checked on ARM32 against20/8/4-byte packets and
the three exact request numbers; full source/REA references are in ABI.json.

### Old3.4 pagemap semantics

The reviewed `fs/proc/task_mmu.c` macros, not modern soft-dirty assumptions, define:

- bits63..61 status: exactly4 for a present page;
- bits60..55 page shift: require12;
- bits54..0 PFN (55-bit mask).

All2048 entries must be present, old-format, contiguous, and inside PFNs
`[0x80000,0x81800)`, with the whole8MiB extent contained. Starting PFN is not
hardcoded to0x80000. The mock success starts at0x80400 to exercise this distinction.
Missing PTE/PFN, swapped/reserved status, unexpected page-shift format, discontinuity
or out-of-reservation PFN is HOLD. No mapped payload is dereferenced. Successful
PFN observation does **not** establish memory cache attributes, exclusive absence
of kernel consumers, later DMA safety or capture retirement.

The source comment's overlapping “0–55” prose is resolved using its actual
PM_PSHIFT_OFFSET/PM_PFRAME_MASK macros. Runtime-format disagreement is reported,
not silently reinterpreted with a newer pagemap scheme. Only page count and first/
last validated PFN scalars are output; no raw pagemap table or other address range.

## Cleanup and explicit uncertain-state policy

On all **known** paths with owned resources, the one-shot order is:

`close pagemap if opened → munmap own8MiB → close exported DMA fd → FREE known
handle once (0xc0044901,4 bytes) → close ION client once`.

No operation is retried in cleanup. This is allocator-only retirement with no
DMA consumer by construction and parent admission. It must never be copied into
a future capture cleanup path merely because this allocation test succeeds.

Known ALLOC failures ENOMEM/EINVAL/ENODEV/ENOTTY close the client without freeing
an unreturned handle. Exact retail ALLOC EFAULT rolls back a successful allocation
before return; even a plausible partial output handle is **not freed again**.
MAP ENOMEM/EINVAL/ENODEV/ENOTTY with no reported live fd allows known-handle cleanup.
MAP EFAULT/EINTR is ambiguous because export may precede failed copyout; do not
scan for the leaked fd or guess its number. Unknown ALLOC result, invalid/mutated
packets, fd identity disagreement and uncertain FREE/munmap/close enter quarantine.

Quarantine emits `UNCERTAIN_STOP`, `known_allocator_cleanup:false` and
`remaining_ownership:retained_or_unknown`, then sends **SIGSTOP to itself**. It
retains remaining owners for parent's already bounded recovery/reset. There is
no listener, command loop, retry, watchdog feeder or autonomous recovery. If
externally continued, it remains inert in pause and does not guess cleanup. Parent
must recognize stopped status and resolve it within its admitted bound; do not
wait for a normal exit or interpret timeout as release. Host MOCK substitutes
exit125 for quarantine; it never SIGSTOPs a real host/target device process.

Signal handlers only set a flag. Once an interrupted syscall returns with known
ownership, ordinary ordered cleanup runs; ALRM/INT/TERM report124/130/143. An
ambiguous ioctl or cleanup result still quarantines. Signals cannot preempt an
uninterruptible kernel operation; the process alarm is not a retirement token.

## Output/exit interpretation

- Exit0/PASS: allocation/mapping acknowledged, all2048 PFNs meet this exact
  old-format/range/contiguity test, known allocator cleanup calls completed.
- Exit2/HOLD: allocation/mapping may have succeeded but pagemap cannot establish
  the requested proof; known cleanup completed. No prefault/retry allocation.
- Exit1/ERROR or signal-derived nonzero: entry/clock/syscall failure; inspect
  explicit stage/flags, never promote to PASS.
- UNCERTAIN_STOP: native child is retained/stopped, not exited; parent's failure
  handling/reset is required. It intentionally reports no guessed allocation state.

Parent accepts a complete JSON record with corresponding observed exit/stopped
status, not a prefix or successful file transfer. Output contains only fixed ABI
inputs, Boolean operation results, old-pagemap scalar counts/PFNs and stage labels.
No payload/credentials/configuration/identifiers/footage or their hashes.

## Build/resource/runtime/license closure

```text
PYTHONDONTWRITEBYTECODE=1 python3 apps/qualification/stock-reference-20261013/ion-inventory/build_mock.py buildNN
```

Fresh output directory required; compiler TMPDIR is inside this unit. Native is
built/inspected only. Host binary must use `--mock-stock-ion-inventory`; every
UID/kernel/open/stat/ioctl/mmap/pread/cleanup/signal adapter is MOCK. Its mapping is
a deliberately unmapped fake pointer, so accidental payload access would fail.
78 cases cover exact inputs, single allocation/export, FD/identity/handle faults,
allocation rollback, ambiguous export, fixed-range old pagemap/endian parsing,
partial/EINTR read bounds, missing/noncontiguous/out-of-range/bad-format PFNs,
signal cleanup, once-only cleanup prefixes/quarantine and CLI/UID/kernel refusal.

Toolchain: pinned Bootlin armv5-eabi-musl2025.08-1, GCC14.3.0, binutils2.43.1,
musl1.2.5, sysroot headers5.4.296. No dynamic uClibc mixing. Exact compiler/subtool,
header, source, CRT/libc/libgcc and parent-public-receipt hashes are in report.json.
Native/static metadata and runtime inspection are sealed separately. No native
clock/pagemap/allocator acceptance is claimed by host mocks or source equivalence.

Application main frame1424 bytes; conservative sum of all application frames3144;
writable ELF LOAD432 bytes (combined3576). These bound small application metadata
storage only, not libc/signal/kernel memory or the separate8MiB reserved-heap
mapping. No application heap allocation. Old pagemap itself allocates kernel
walk metadata; a256-byte user read is not a256-byte total-kernel-memory claim.
No replacement CMA40/16, security2MiB limit or default memory budget is changed.

Original license notices remain untouched. Stock/public ION header/source carries
its original GPLv2 notices; musl/GCC runtime licensing and source/license locations
are preserved from the pinned toolchain summary/README. No workspace license or
publication grant is invented. The separate sensor/init correction and capture
cache/retirement contracts remain outside this no-capture unit; full-stream
ownership must still remain alive through admitted reset if normal retirement
cannot be proved.
