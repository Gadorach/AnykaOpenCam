# Station76 P4b — bounded cached descriptor admission SOURCE candidate

2026-10-22. RP-02 / NET-01–08 / K76-06; explicitly selected P4b subset on the
exact reviewed P3/P4a derivative. Not full P4, composite attach, P5 or native work.

## Result and limits

**Implemented the selected cached-interface admission subset before the first
driver allocation. HOST syntax01:2/2 changed C units returncode0.** This is an
author candidate, not independent SOURCE/API acceptance, runtime confirmation,
complete P4b/metadata GO, paired build or native/product GO.

The implementation validates the existing bus1/devpath`"1"`, VID:PID`a012:8400`,
active config1/high-speed projection, coherent core-owned selected-interface cache,
historically supported **interface number0**, and a restrictive exclusive pair
OUT01/IN81, full attributes02 and full512-byte packet field. It rejects mismatching
cached state before `kzalloc`, URB/backing allocation, core probe, submission or
any use of the TX padding modulus/alignment.

**Still BLOCKED:** expected exact device configuration count, active-configuration
interface count, interface alternate count and active `bAlternateSetting` value.
No value1 for these counts or alternate0 is inferred. Presence of two historically
observed endpoints is not a measurement of absence of extras. Exactly two cached
endpoints is a deliberate restrictive admission policy, not measured cardinality.
The subset can admit multiple configurations/interfaces/alternates if its selected
cache passes; it does not qualify them or close the missing exact-topology gate.

## Scope, provenance and immutable custody

Only the following fresh owned paths were written:

- `apps/qualification/kernel76-station-descriptor-admission/`
- this report;
- adjacent `station-descriptor-admission-p4b-source01/`.

All74 selected P3 source files were initially copied byte-for-byte. Final source
still has74 files: **72 unchanged, two changed, no additions/removals**:

| Relative source path | Entire allowed change |
|---|---|
| `utils/owner_usb.c` | Fixed local cached-admission helper/constants, preallocation probe call, replacement of last-match endpoint selection with admitted fixed endpoint numbers/sizes. |
| `hgic_smac/attach_status.c` | Only fixed policy string becomes `descriptor-admission-p4b-source01`; schema2/fields/logic unchanged. |

No control/status framework, configuration surface, parameter override, new
producer/worker/timer, fallback, trace or unrelated cleanup was added.

Verified frozen P3 candidate seal:
`5d8a96a78e1b6d99f7a16f259d36f0e077644f7e7378c8ecd88b58109add66eb`.
Verified independent P3 review seal:
`c449dee74290c8bdd5d318f71816b83576d31a84a407ab05705092094f94db38`.
The copy custody union contains **595 unchanged files**, retaining the451 older
custody entries, P3 source/evidence and P3 review/seals. In addition, **all817
public P3 review-reader inputs** and **204 accepted A1/A2a/A2b source pins** were
revalidated. These maps overlap; their counts must not be summed as unique files.
The full reader map is retained in `p3-review-reader-custody.json`.

Prior stopped P3, all older candidates/reviews and failed observations remain
frozen. P1/partial-review source was not inspected or integrated. All72 unchanged
files include `rx.c`, `event.c`, `utils/{fwctrl.c,fwctrl.h,owned_control.c,
owned_control.h,control_receipt.h}`, `hgic_smac/attach_init.c`, wire/schema headers,
build selection, legacy/unselected files and original license/notices.
LICENSE SHA256 remains
`8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
This copy does not relicense or authorize publication.

Final source-map SHA256 (compact sorted-key UTF-8 JSON file-size/hash map):

```text
2516c416c20b621414ebb7e0b9b34be67076ca3c657602f7e1d2f2cd66787e0b
```

## Public facts versus policy and unknowns

Read the selected P3 probe, remaining-source plan section6, full independent P3
review, `METADATA_RECEIPT_REVIEW.md` and `metadata21a.py` as public text only.
**Never imported/executed** the metadata script, its imports, parser, main or private
inputs. The metadata review's dated NO-GO findings/test results are history, not
this lane's execution or a fresh acceptance claim. No private/current receipts
were used to manufacture missing descriptor facts.

`public-fact-api-anchors.json` retains exact public file hashes and line excerpts:

| Evidence | What is actually supported |
|---|---|
| `metadata21a.py:27–28,60–92` | The fixed query/parser expects cached VID/PID a012/8400, configuration1, speed480, ep_01/ep_81 addresses01/81, full attributes02 and displayed maxpacket0200 under `1-1:1.0`. It does not enumerate other endpoints/configurations/interfaces/alternates. |
| `METADATA_RECEIPT_REVIEW.md:120–134` | Independent public source corroboration of cached query semantics and endpoint/interface paths; not fresh live admission. |
| Retained `drivers/usb/core/message.c:2165–2204` | Interface name is `%d-%s:%d.%d`: bus, devpath, configuration, **interface number**. Thus `1-1:1.0` supports interface0, not alternate0 or an interface count. |
| `drivers/usb/core/sysfs.c:25–43,145–178,690–704`; `endpoint.c:37–58,150–170` | Cached active configuration, high-speed→480, VID/PID and endpoint scalar/name source. No new USB transaction is needed by this validator. |
| `include/uapi/linux/usb/ch9.h:396–449` | Structural descriptor types/minimum sizes, bulk type2 and endpoint number/direction layout. Protocol constants are not camera measurements. |

**Packet-field nuance:** retained `endpoint.c:52–57` displays `usb_endpoint_maxp`,
which `ch9.h:649–658` masks to bits[10:0]. Displayed0200 supports512-byte maxpacket;
it does **not independently measure the entire unmasked16-bit field**. The new
full `le16_to_cpu(wMaxPacketSize) == 512` check additionally rejects upper
transaction/reserved bits as restrictive policy. Likewise endpoint count2/no
extras/uniqueness is policy over the current cached interface, not proof of the
complete measured topology. Full attributes02 is an unmasked displayed scalar.

No expected class/subclass/protocol, descriptor extension length, bInterval,
configuration/interface/alternate cardinality or alternate value is invented.
Minimum descriptor lengths/types are structural checks, not measured byte-exact
device descriptors. No source-supported number is generalized to another device.

## Cached API contract and actual admission order

The helper receives the USB core's valid probe `intf` and its existing `udev`;
it is not a raw-descriptor parser or an arbitrary-pointer memory-safety API.
It performs only cached reads, bounded array walks, comparisons and returns.

1. Require bus/device identity/topology/high speed, nonnull active configuration,
   configuration cache and nonzero cached configuration count. Preserve the
   original device-ID table and additionally check cached VID/PID directly.
2. Find the active configuration exactly once in `udev->config[]`, without pointer
   ordering/subtraction or assuming array slot0. Check configuration descriptor
   type/minimum size, total length covering its header, value1 and a nonzero
   interface count fitting the actual fixed interface array.
3. Find this `intf` exactly once in that active configuration's declared interface
   slots, not interface-array slot0. Require the corresponding cache, nonnull
   alternate array/current pointer, nonzero/equal live/cache alternate counts and
   identical live/cache alternate-array base.
4. Find the current descriptor pointer exactly once in that owned alternate array
   **before dereferencing it**. No alternate-number constant/order is assumed.
   Check interface descriptor type/minimum size, interface number0, exactly two
   cached endpoints and nonnull endpoint storage.
5. Walk those two endpoints, in either order. Require endpoint type/minimum size,
   full attributes exactly02, full LE packet field exactly512 and each address
   exactly01 or81 once. Missing/count mismatch/duplicates/extra/wrong address,
   endpoint0, nonbulk, reserved attributes, zero or other cached packet fields
   refuse with `-ENODEV` before allocation/use. No masked type/address/packet
   match silently accepts incompatible cached bits.
6. Probe records that failure through unchanged immutable
   `hgic_attach_error(error, HGIC_ATTACH_PROBE)` and returns immediately. There
   is no allocated USB owner to dispose on refusal and no guessed cleanup.
7. Only after success does original driver allocation/initialization proceed.
   Endpoint numbers derive from the two admitted addresses and sizes from512;
   there is no last-match-wins scan, capacity fallback or zero packet divisor.

Actual retained API ownership: `include/linux/usb.h:84–97,182–252,340–425` defines
endpoint/current-alternate/configuration arrays and their counts; active interface
entries are valid only while that configuration is installed. `message.c:2165–2168`
assigns the interface's alternate base/count directly from its persistent cache.
The configuration parser allocates/counts these core-owned arrays, including
endpoint count normalization. Iterations use those owned counts, not device-supplied
buffer lengths. Retained `config.c` limits configurations to8, interfaces to32 and
alternates per interface to128; these are **kernel implementation bounds, not
measured camera counts or new policy constants**. The endpoint walk is exactly2.

The existing serialized USB core probe contract is retained: USB bus
`need_parent_lock=true` and driver-core device/parent locking are pinned in
`driver.c:2082–2087`, `drivers/base/dd.c:1162–1192`. This helper adds no locks or
independent asynchronous cache user. It neither probes a different lifetime nor
claims to withstand arbitrary corruption of kernel-owned pointers/counts. No
post-probe descriptor continuity or all-context/native concurrency proof follows.

**Cache, not original wire:** `config.c:307–367,457–507,619–640,800–809` can skip
malformed/duplicate/extra endpoints, clear reserved address bits, clamp packet
sizes and rewrite descriptor counts/lengths before this callback. Therefore
rejecting a bad **cached** field cannot prove that the original device never
offered it. This lane does not parse rawdescriptors, query USB, switch alternate/
configuration, reset endpoints/device, or change the upstream parser to recover
that missing proof. USB core enumeration before driver probe is not created or
qualified by this source increment.

## P3/P4a invariants preserved, not re-proved by compilation

`source-inventory.json` includes exact equal-block hashes against selected P3:

- **14640 bytes:** everything in owner_usb.c before the old probe, including
  fixed16TX/32RX/2560B allocation and partial-slot rollback bookkeeping, STOP,
  RX submit/callback, one-token TX, actual submit/callback reconciliation,
  conservative reservations, producer joins, all-slot poison and deferred free.
- **612 bytes:** the probe's original USB allocation and owner initialization.
- **2111 bytes:** slot allocation call onward, including exact16/32 checks, core
  probe return handling, device reference/publication tracking, initial32-index
  RX loop, rollback, disconnect and synchronous module-return route.

All actual P3 request/queue/worker/FWCTRL/receipt code is wholly unchanged outside
this USB unit. Its absolute500ms deadline, explicit refs/states, first-error-versus-
submit serialization, negative-status observation chronology and late-result
failure retention remain intact. Reservation bits are not converted to active
counts/refs. Callback reconciliation/list emptiness remains insufficient return
proof; exact P4a joins/poison still precede request/core/USB free. No forced free,
synthetic callback, guessed owner repair or weakened failed-state policy is added.
Existing128KiB cap and5s policy remain; no new timing/finite-wait guarantee.

## Compile/provenance evidence

Executed only the owned offline byte-copy, changed-unit syntax and static
inventory/seal bookkeeping:

```sh
python3 -B apps/qualification/kernel76-station-descriptor-admission/prepare_source.py
python3 -B apps/qualification/kernel76-station-descriptor-admission/check_syntax.py syntax01
python3 -B apps/qualification/kernel76-station-descriptor-admission/freeze_candidate.py inventory
python3 -B apps/qualification/kernel76-station-descriptor-admission/freeze_candidate.py seal
```

Syntax01 compiled only `utils/owner_usb.c` and `hgic_smac/attach_status.c` with
Clang23.1.3, x86 HOST installed `6.18.55-2-cachyos-lts` headers, `-fsyntax-only`,
implicit-function/incompatible-pointer errors enabled, and the retained P3 setup.
Both returned0. No compiler failure occurred in this lane; known installed-header
nocf/address-space warnings remain. Not warning-free, a link or paired ARM ABI
confirmation. `syntax01/receipt.json` retains exact argv, UTC, compiler/header/
dependency/source hashes and diagnostic logs. No unchanged translation unit or
unchanged suite was run. No app/test/shim/fake-USB execution, runtime/unit/security/
adversarial/penetration/fuzz/mutation test, ARM/object/module/Kbuild/image build,
staging or target action occurred.

Evidence also retains `copy-baseline.json`, `CACHED_DESCRIPTOR_DELTA.patch`,
`public-fact-api-anchors.json`, `p3-review-reader-custody.json`, `source-inventory.json`
and a final `candidate-seal.json` excluding itself. No UART/RPC/status/network/
private IO, device effects, Git/index/central-plan/P1/model/provider/safeguard work
or old source/helper edit occurred. Hardware state/HOLDs were not queried or
changed. This source lane grants no native authority.

## Unexecuted ordinary confirmation checklist and blocked gates

The following are **not run, not PASS and not queued**:

- Independent ordinary exact-pin source/API review of this subset, policy-versus-
  measured-fact distinction and unchanged P3/P4a blocks.
- Under separately selected ordinary functional qualification: valid cached
  projection/pair in either order; preallocation refusal for incoherent selected
  cache membership, absent/current-outside-array state, missing/duplicate/extra/
  wrong/nonbulk/zero/wrong packet endpoints; no allocation/core-probe/submission
  on refusal; accepted-path original allocation/rollback/retirement preservation.
  This list does not authorize a shim, malformed-input/mutation campaign or runtime.
- Exact configuration/interface/alternate counts and active alternate value need
  properly scoped supported facts before the complete topology gate can close.
  Policy values may not be relabeled measured; no further query is selected here.
- Raw-wire exactness and post-probe/current-native descriptor continuity remain
  unproved by these normalized cached fields.
- **P1 remains absent/unmet; full P4 transport accounting/status/tombstone/final
  summary and normal/native retirement remain separate and incomplete. P5
  source/runtime selection is ABSENT.** P3/P4a narrow source reviews do not waive
  those gates or supply finite init/RTNL/poison completion/recovery proof.
- Paired ARM/module/image build, composite attach, radio/association, combined
  memory/media, persistent installation, other-camera and product acceptance
  remain absent. No SOURCE/native/product GO or automatic next queue follows.

Parent may select independent review of this frozen bounded candidate; full P4b
cannot be claimed complete while the explicit topology facts remain missing.
