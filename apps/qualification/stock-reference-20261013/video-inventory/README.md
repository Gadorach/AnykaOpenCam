# Fixed stock video metadata inventory

One paired-fd metadata probe for QC07F stock Linux3.4.35. It does not capture an
image, initialize a sensor register table, change illumination, or finish the IR
test. Parent alone admits and executes the native payload.

## Payload / exact CLI

Artifact:
`evidence/delegated/20261013-stock-reference/video-inventory/build02/stock-video-inventory-armv5`

SHA256: `f4cfea4f3657aadcc8e310a95e3869d2081def77a04421676650d188626875c7`

Size: **35,652 bytes**. Static ELF32 LE ARM EABI5, soft-float, no interpreter or
dynamic segment, non-executable stack. ARM payload was never executed by worker.

```text
<parent-proven-absolute-RAM-payload-path> --native-stock-video-inventory
```

There are no other arguments, modes, device paths or values. The staging path is
chosen/proved by parent, not an invented existing camera path. Run as a child of
the rescue shell; replacing PID1 is refused. Real/effective UID must both be0,
uname must be Linux/3.4.35, argv0 must be absolute and argc/selector exact.
These checks do not authenticate the image or driver; parent must bind exact pins.

## Fixed operation order

1. Set handlers for SIGALRM/SIGINT/SIGTERM without SA_RESTART; unblock those
   signals, arm10-second process alarm, obtain valid monotonic start time.
2. lstat **both** `/dev/isp_char` and `/dev/video0`; require existing character
   nodes, no symlinks. No create/chmod/mknod/module load.
3. Open `/dev/isp_char` with `O_RDONLY|O_CLOEXEC|O_NOFOLLOW`, retain fd; verify
   fstat device/inode/rdev identity against lstat.
4. Open `/dev/video0` with `O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW`; verify identity.
   O_NONBLOCK does not make driver's open noninitializing or preemptible.
5. On video fd only: QUERYCAP, G_FMT VIDEO_CAPTURE/type1, ENUM_FMT type1/index0.
   Exactly one of each in that order; stop on first error. No enumeration loop.
6. Attempt video close **before** ISP close on every normal/error/interrupted
   branch with successfully obtained fds. Never retry close. Retain ISP while video
   is open or being closed. Close return is not a DMA-retirement acknowledgement.
7. Validate elapsed time<10s and emit one complete metadata JSON line after
   successful query and ordered close. Parent accepts complete JSON plus exit0.

No S_FMT/S_PARM/S_CTRL, REQBUFS/QUERYBUF/QBUF/DQBUF, STREAMON/STREAMOFF, poll/read
frames, allocator/DMA imports, MMIO, audio, motor, GPIO, application or network
operation is present. ISP receives **no ioctl** from this helper.

## Hard parent admission and readback

Parent must establish the exact current stock image/kernel/modules and actual
node providers, resident selected SC1346 callback, no app or competing camera/ISP
owner, empty/unstarted queue and **capture IRQ/frame counts zero before entry**.
The helper's read-only flags do not eliminate first-open effects:

- ISP first open invokes private ISP initialization; retaining this fd supplies
  context needed by the video's implicit format/scaler path.
- Video first open can reset/power/clock the sensor and implicitly apply format.
  Those effects require explicit parent admission even though no S_FMT is issued.
- Close video before last ISP close, which can finalize/unmap ISP context.

After exit, parent must record zero capture IRQ/frame counts, actual provider/
owner state, both child descriptors retired and no queued buffers/services, using
its existing admitted observation mechanism. This program does not open proc/sysfs
or add further driver-status queries. If counts change, the metadata result is not
proof of an idle/harmless campaign; preserve it and stop further entry.

Keep RO root/usr/JFFS2 and card isolation, no normal-app homing/autolight/recording/
cloud startup, sole hardware owner and existing external recovery/user power
recovery. The stock watchdog's reported60-second/kernel-idle-feeder configuration
is unchanged. The helper never opens watchdog and supplies no guardian.

The signal handler sets a flag and returns so normal cleanup can close video
before ISP. On returning from an interrupted syscall, remaining queries are
skipped and ordered cleanup runs; exit124/130/143 for ALRM/INT/TERM. A signal
cannot preempt uninterruptible open/ioctl/close, nor guarantee a cleanup deadline.
Parent's independently owned bound remains required. Abrupt SIGKILL/reset cannot
be represented as the normal ordered-close result.

## Verified wire ABI

`stock_wire.h` uses fixed-width fields; native source does not include modern
videodev2 structures. `abi_compare.c` is compiled **only**, never executed, against
both original3.4.35 header and pinned5.4.296 sysroot header on ARM32.

| Operation | Request | Structure extent | Meaningful input |
|---|---:|---:|---|
| QUERYCAP | `0x80685600` | 104 | zeroed buffer |
| G_FMT | `0xc0cc5604` | 204 | type1 at offset0;200-byte union starts+4 |
| ENUM_FMT | `0xc0405602` | 64 | index0 at+0, type1 at+4 |

Both headers agree on these extents, requests and offsets. Old pixel-format
structure is **32 bytes**; new header extends it to48. Only the verified old eight
32-bit words at union+0..28 are parsed: width, height, fourcc, field, bytesperline,
sizeimage, colorspace, priv. No interpretation of new fields is inherited.
ENUM_FMT flags are+8 and fourcc+44. Capability version/capabilities/device_caps
are+80/+84/+88; device_caps is reported raw, not assumed valid unless capabilities
advertises the corresponding standard bit.

All packet bytes are zero-initialized before setting fixed type/index; canaries
protect both ends. Returned type/index must remain the requested values. The
public stock-source usercopy reads only G_FMT's4-byte type and ENUM_FMT's8-byte
index/type input, zeroes remaining kernel temporary storage, then copies the
full `_IOC_SIZE` back; partial copies return EFAULT. G_FMT's204-byte kernel temporary
allocation is metadata, not a frame buffer. Retail host querycap fills the known
card/capability fields; source/REA citations and hashes are in ABI.json/seal.

`expected_wire_bytes` describes the ABI, **not a returned byte-count measurement**.
ioctl success is0, not bytes copied. A faulty provider silently returning0 after
partial copy cannot be detected solely by initialized zeros. Exact provider
binding remains essential. Negative/positive unexpected results, EFAULT, canary
damage, changed node identity, type/index mismatch and close failure yield no
success JSON. ENUM_FMT ENOTTY is an explicit unsupported error, no fallback scan.

## Output/privacy and meaning

Output is a local stock-reference record, not a product schema/API. It includes
numeric capability words, cached format fields, old32-byte pixel metadata hex,
enum0 numeric flags/fourcc, expected ABI sizes, elapsed time and close order.

- `bus_info` is **never output or hashed**. No raw104-byte capability dump.
- Driver/card strings are emitted only on exact bounded match to the public
  labels `ak_camera`, `AK_Camera`, `soc-camera`; all others become
  `unrecognized_omitted`. Arbitrary descriptions/reserved/opaque bytes are omitted.
- No credential, configuration, footage or image content is read/output.
- Unknown fourcc and zero cached dimensions are preserved, not normalized into a
  valid frame claim. `sizeimage` is not complete main+sub+metadata slot proof.
- Successful G_FMT reports cached bookkeeping, not configured physical frame
  delivery, actual YUV order/stride, capture-safe backing or IR sensitivity.
- The source-level `capture_started_by_helper:false` states that this helper
  issues no start/queue/frame operation; the required parent IRQ check determines
  whether the actual kernel nevertheless had activity during open/close.

## Build and verification closure

```text
PYTHONDONTWRITEBYTECODE=1 python3 apps/qualification/stock-reference-20261013/video-inventory/build_mock.py buildNN
```

Fresh outputs only; compiler temporary files stay in this unit's evidence subtree.
`build01` preserves the initial old-header compile refusal because unexported
kernel headers warn under Werror. `build02` adds the header's intended
`__EXPORTED_HEADERS__` marker for the compile-only old-header comparison; original
headers were not edited and no diagnostic/size assertion was removed.

**65 host-MOCK cases passed.** Host binary accepts only
`--mock-stock-video-inventory`, never forwards device operations, and verifies
UID/PID/kernel/entry rejection; clock/signal setup; missing/symlink/noncharacter
nodes; first/second open and fstat/identity failures; golden zero-initialized
104/204/64-byte packets/endian parsing; each query's negative/positive/EFAULT and
overrun/underrun failure; wrong type/index; enum ENOTTY/EINVAL; both close errors;
close order during alarm/interrupt/termination, open and close; unknown/zero
format preservation; suppression of synthetic private bus/label/description data.
No native/firmware binary was run on host or camera.

Pinned compiler: Bootlin armv5-eabi-musl2025.08-1, GCC14.3.0, binutils2.43.1,
musl1.2.5, sysroot headers5.4.296. Source-only original3.4 header comparison and
used headers/tools/CRT/libc/libgcc archive hashes, command lines, source snapshot,
link map, ELF/disassembly/stack metadata are retained in build02/report.json.
No dynamic uClibc/musl mixing. Modern libc time/stat fallback compatibility remains
the pinned runtime, not proof from uname; this payload awaits parent native entry.

Resource evidence: main stack3000 bytes; sum of all application's compiler-reported
stack frames3344 bytes; writable ELF LOAD464 bytes. Fixed application storage and
these bounds stay below4KiB; combined3344+464=3808. This does **not** bound musl
startup/callee/signal stack, kernel open allocations or total process memory to4KiB.
No application heap allocation/imports. File size35,652 bytes is a separate metric.
ELF retains the toolchain's aggregate v5TEJ/Thumb-2 attributes and runtime-selected
old-kuser/v6/v7 atomic routines; no unsupported instruction path is newly patched.

Original source/license notices remain untouched. Toolchain summary identifies
musl MIT/GCC runtime exception and includes source/license locations in its README;
copies are sealed with the handoff. No new workspace license or redistribution
authorization is inferred. This unit stops at metadata admission, not optical IR
acceptance or capture implementation.
