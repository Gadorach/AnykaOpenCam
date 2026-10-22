# Station76 P4b — bounded cached-descriptor SOURCE/API review

2026-10-22. RP-02 / NET-01–08 / K76-06. Explicit parent-selected subset on
the exact reviewed P3/P4a owner contract, not full topology/native admission.

## Disposition

**Narrow SOURCE/API sufficiency for the selected cached-descriptor subset.**
No concrete correction-required cache/API/alias/lifetime/acceptance defect was
identified in the actual two-file delta. No patch is requested against this
frozen candidate. **Full P4b remains BLOCKED** on the explicitly missing expected
configuration/interface/alternate counts and active alternate value. This finding
does not invent those facts or approve a composite/native attach candidate.

Static public source/hash/diff/metadata only. Only this new report and
`station-descriptor-admission-p4b-review01/` were written. No stored script main,
candidate/helper/metadata import or function, compiler/generator/test/shim/runtime,
target/UART/RPC/status/network/private IO, original source/helper, Git/index,
central-plan/AGENTS, settings/model/provider or safeguard operation occurred.
No penetration/adversarial/fuzz/mutation campaign was expanded or run. No task or
policy rejection occurred. P1 source and partial review remain intentionally
uninspected, unretried, unreviewed and unadopted.

Parent alone owns hardware. Last supplied observation is held original U-Boot,
not current Linux; both radio and reset-failure HOLDs remain unchanged. This lane
neither queries hardware nor queues a test, build, implementation or native action.

## Inputs, exact delta and custody

`S/` = `apps/qualification/kernel76-station-descriptor-admission/source/`.
`B/` = `apps/qualification/kernel76-station-control-owned/source/`.
`K/` = retained public kernel tree at
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.

- Supplied **90-file** author seal verified:
  `0388936a8782c1e5c1f74c794b78040b47849dd506e6002b70f24dd839ff6653`.
  Exact candidate/evidence/report file set matched, not only listed hashes.
- Exact74-file source-map SHA256:
  `2516c416c20b621414ebb7e0b9b34be67076ca3c657602f7e1d2f2cd66787e0b`.
  Format: compact sorted-key UTF-8 JSON relative source size/hash map.
  Baseline is all74 sealed P3 files; final source is **72 unchanged, two changed,
  no additions/removals**. Independently reconstructed universal-newline diff
  equals the author's `CACHED_DESCRIPTOR_DELTA.patch`; raw pins retain line endings.
- **595 prior custody entries** match the independently reconstructed exact union
  of nine earlier candidate/review seals, including those seals. The P3 author seal
  remains `5d8a96a78e1b6d99f7a16f259d36f0e077644f7e7378c8ecd88b58109add66eb`;
  P3 review seal remains
  `c449dee74290c8bdd5d318f71816b83576d31a84a407ab05705092094f94db38`.
- **817 P3 public-reader inputs** match its actual input seal and the P4b custody
  map. **204 accepted A1/A2a/A2b source pins** match their accepted seals.
  These maps overlap:90/595/817/204 are **not summed as unique files**.
- LICENSE remains
  `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`.
  Existing notices/provenance and GPL-2.0-only file notice are retained; no
  relicensing/global grant/publication authorization is inferred.

| Only changed S/path | SHA256 / change |
|---|---|
| `utils/owner_usb.c` | `9940453566eb688849a451c306a3dafbbda9a468d43e27754bf2e780b1329938` — cached helper, preallocation call, admitted endpoint assignment |
| `hgic_smac/attach_status.c` | `06e27d8d284c236a782b5eb9c14a9be11e4d4241f63c24d9b86b40d473875e0d` — only fixed policy label |

The status file equals B/ after exactly the fixed string substitution at:14.
Schema2, fields and logic are unchanged. This is not full transport accounting,
request progress, a tombstone or final retirement summary. Build selection,
request/FWCTRL/receipt/ordered-producer code, A2b RX/event, wire/schema headers,
legacy/unselected source and licensing are whole-file unchanged from P3.

## Actual preallocation admission and refusal

`S/utils/owner_usb.c:421–478` reads the valid USB-core probe objects, not arbitrary
descriptor pointers or raw buffers. Its actual order is:

1. `:423–434` reads current pointers without dereferencing their targets, then
   requires bus1/devpath`"1"`, cached VID:PID a012:8400, high speed, nonnull active
   configuration/cache and nonzero cached configuration count. Constants are
   actual `hgic_def.h:25,28`, not new VID/PID guesses.
2. `:437–444` equality-walks `udev->config[]`, requiring active configuration
   membership exactly once **before config dereference**. It checks CONFIG type,
   minimum9B header, total length covering that header, value1, nonzero interface
   count bounded by the actual fixed interface array. No assumption of array slot0.
3. `:445–454` finds this probe `intf` exactly once in active configuration slots,
   selects its same-index cache, and requires nonnull live/cache alternate base,
   nonzero/equal counts and identical array base. It does not assume interface
   number equals array index or impose an unproved interface cardinality.
4. `:456–462` equality-walks that owned alternate array and requires current-pointer
   membership exactly once **before current descriptor dereference**. It then checks
   INTERFACE type/minimum9B, interface number0, exactly two endpoints and nonnull
   endpoint storage. It does not guess alternate0 or an alternate count of1.
5. `:464–477` walks those two cached endpoint objects in either order. Each must
   have ENDPOINT type/minimum7B, **full attributes02**, **full LE packet field512**,
   and full address exactly01 or81, once each. Any other address/duplicate fails;
   success requires both directions. No masked address/type acceptance or last match.

No relational pointer comparison/subtraction, raw-buffer traversal, USB query,
reset, set-configuration, alternate switch, fallback or new parameter is added.
Counts/objects are trusted only under the real core allocation/probe contract
below; this is not a general pointer-corruption/memory-safety validator.

Probe `:487–491` records rejection via the unchanged immutable
`hgic_attach_error(error, HGIC_ATTACH_PROBE)` **before returning**, with no driver
allocation or owner publication. It does not jump to allocated-owner cleanup when
no owner exists. The recorder `attach_init.c:35–73` preserves any earlier cause.
`kzalloc` is first reached at:492 only after admission. Slot/backing allocation,
core probe and submission follow at:514,521,533 respectively.

`:509–513` derives endpoint numbers from the **admitted known pair** and sets both
sizes to512. No later scan can replace them; blk_size and TX padding at:281–283
therefore do not use a zero/unknown divisor or arbitrary non-power-of-two size.
The original allocation failure and initialized-owner rollback paths are unchanged.

## Actual retained kernel ownership, bounds and probe lifetime

The helper's loops use **core-owned parsed allocations and coherent core counts**,
not a new trust boundary over supplied raw descriptor lengths:

| Actual K/ source | Contract used by this subset |
|---|---|
| `include/linux/usb.h:84–97,245–252,340–425,684–686` | Endpoint arrays follow parsed bNumEndpoints; live alternate base/count/current pointer; persistent interface cache; fixed32 interface/cache slots; device configuration/active pointers. Ordering is explicitly not guaranteed. |
| `drivers/usb/core/config.c:18–20,943–961,972–982` | Configuration count is capped at8 before allocation of that many host configurations; a later read failure may shorten the count, not enlarge the allocation. |
| `config.c:756–838,576–605` | Parsed interfaces are counted within32 slots; alternate allocations are capped at128; each alternate is counted only within its allocated cache capacity. |
| `config.c:619–647,331–367` | Endpoint allocation has bounded capacity and bNumEndpoints is reset/recounted as endpoints are retained; the helper's exactly2 walk fits this array contract. |
| `drivers/usb/core/message.c:2165–2185,2226–2275` | Same-index interface/cache association, direct live base/count assignment with cache kref, current alternate chosen from that cache, active configuration installed before interface registration/probe. |

Thus the selected equality walks are bounded by8/32/128/2, with small unsigned
match counters; those implementation bounds are **not expected camera counts**.
No selected-cache alias or array-order mismatch was found. The parser's ordinary
duplicate-endpoint check (`config.c:245–291`) also checks other interfaces and the
same alternate, while allowing different alternates of one interface to reuse
addresses. That is normalized core-cache behavior, not a new wire-exclusivity proof.

Actual selected probe serialization was checked, not assumed from a generic API
name: USB bus `driver.c:2082–2087` requires parent locking; driver registration's
`drivers/base/dd.c:1295–1297` and manual attach:1202–1208 use
`__device_driver_lock:1162–1192` for parent then interface. The async driver-attach
helper:1219–1229 uses the same locks. Initial interface registration occurs inside
the device-locked configuration setter (`message.c:2008–2053,2247–2275`). USB-core
`driver.c:319–399` reaches the driver probe with its live interface; this helper
adds no separate asynchronous cache reader or redundant/reentrant device lock.
Configuration destruction/replacement is not a concurrent unowned helper operation.

The core may autoresume or perform a deferred alternate0 switch **before** calling
the driver (`driver.c:354–399`); the driver adds no such operation and does not
claim core enumeration/probe has zero device effects. Its helper only observes
the current cache. Core default-alt behavior (`message.c:2172–2184`, including
fallback to the first array entry) is **not a measurement of this camera's active
alternate**. This review does not extend to arbitrary callers or establish
post-probe/all-context/native continuity.

## Supported historical projection versus restrictive policy

`apps/qualification/kernel76-transport/metadata21a.py:27–28,60–92` was read only as
public text. Its fixed historical query/parser expects cached a012/8400, config1,
speed480 and endpoint paths `1-1:1.0/ep_01`, `ep_81`, with address01/81,
attributes02 and displayed maxpacket0200. It does **not enumerate** other endpoints,
configurations, interfaces or alternates. The historical metadata review:120–134
corroborates source semantics, not current native state or new qualification.

- **Interface0:** actual `message.c:2182,2203–2204` formats interface names as
  bus-devpath:configuration.interface-number. Thus the historical `1-1:1.0` path
  supports interface number0, **not** alternate0 or one interface.
- **Full attributes02:** `endpoint.c:37–50` displays the full cached byte, so the
  restrictive exact-byte comparison matches the historical projection.
- **Maxpacket:** `endpoint.c:52–58` displays `usb_endpoint_maxp`; actual
  `include/uapi/linux/usb/ch9.h:649–658` masks to bits[10:0]. Displayed0200 therefore
  supports512 in those bits, **not the full16-bit field**. The new full-field512
  equality is additional restrictive policy, not a newly measured wire fact.
- **Exactly two/exclusivity:** observing/querying two named endpoints is not proof
  of no extras. The count2 and uniqueness checks are deliberate restrictive policy
  for the selected cached alternate, not measured device cardinality.
- **Minimum sizes/types:** ch9 CONFIG/INTERFACE/ENDPOINT definitions support structural
  minimum9/9/7 and type constants. They do not establish exact camera extension
  lengths, class/subclass/protocol, interval, configuration/interface/alternate
  counts or active alternate value. None is invented by the implementation.

**Cache exactness is not raw-wire exactness.** Actual `config.c:307–367` can skip
zero/duplicate/extra endpoints and normalize reserved address bits;:457–501 can
clamp packet fields;:619–647 recounts endpoints;:800–809 rewrites total length and
interface count. Consequently an accepted exact cached pair can follow core
normalization/skipping. No raw-descriptor parsing, parser change, USB transaction
or malformed-input campaign is performed or recommended to manufacture closure.

The subset can admit multiple cached configurations/interfaces/alternates if the
selected objects pass. Parent explicitly selected that bounded subset. This does
**not** make the full exact-topology gate optional: it remains BLOCKED outside
this narrow source disposition, including future build/native integration.

## P3/P4a owner-return invariants retained

Independent raw-byte comparisons reproduced the author's three preserved ranges:

| Preserved range | Bytes / SHA256 |
|---|---|
| All USB code before probe/helper |14640 / `82ac78c241bc20236a0a311d197e10d7a4fdeb99ce97b9e538f6f03d74e15ac1` |
| Probe allocation/owner initialization |612 / `c62fadeef38b46a422ccea9e8e4139d154d82acd474881bfa9e49a8b00ad8b01` |
| Slot allocation onward through rollback/disconnect/module return |2111 / `878f577f363ffbd1611b42afcaa9da11a6f78a039e8f58383a0eacac76014efb` |

Four critical stop/core-remove, all-slot-dispose, RX callback and reservation
ranges were also compared directly against the reviewed P4a tree, byte-exact.
P3 request/queue/worker/FWCTRL code is unchanged as whole files. No new producer,
callback, worker or timer exists. Heap lifetime, immutable500ms deadline,
first-error/submit CLAIM serialization, immediate callback versus local failure,
negative-status chronology and terminal-failure retention are not changed here.

Fixed16TX/32RX/2560B backing and actual producer joins followed by **all indexed
slot poison before final request/core/USB free** remain the owner-return proof.
Conservative reservations/list emptiness/request callback reconciliation are not
converted into active counts or callback-return proof. Quarantine retains callers/
owners/module text rather than leak-and-return. This increment does not prove
finite joins/RTNL/poison, normal unload success or5s recovery; no forced free/unload,
synthetic callback, capacity increase or watchdog extension is introduced.

## Evidence closeout and still-open gates

Fresh static evidence verifies **927 unique input pins**. It retains the exact
actual diff, source/custody maps,59 source/API excerpts (33 author excerpts checked
against actual full-file bytes, plus26 reader ranges), additional cache excerpts,
input receipt/seal and final report/output seal. Inline static collection commands
are retained as text; no stored script main was invoked. A harmless reviewer
metadata-key exploration failure is disclosed in `METHOD.md`, not hidden as PASS.

Author **HOST syntax01:2/2 returncode0** was verified through exact source, argv,
UTC and diagnostic metadata, **not rerun**. Actual changed units were owner_usb
and attach_status; Clang23.1.3/x86 installed6.18.55-2-cachyos-lts headers,
`-fsyntax-only`, implicit-function/incompatible-pointer errors enabled. Known
installed-header nocf/address-space warnings remain. External compiler/header
installation was not freshly rehashed. This is not object/link/ARM/module/ABI or
behavioral confirmation. No old suite or fixture result is promoted by this review.

**Concrete missing dependency for full P4b:** supported exact configuration count,
active-configuration interface count, interface alternate count and active alternate
value. Kernel bounds/defaults and two endpoint queries cannot supply these facts.
No query or other operation to obtain them is selected here.

Also open: raw-wire exactness and current-native descriptor continuity; full P4
accounting/status/tombstone/final-summary and normal all-context retirement;
P1 independent review (intentionally untouched); P5 bounded progress/recovery;
ordinary functional/SMP/IRQ/HCD execution; paired ARM/module/image/build admission;
composite attach, station/UP/network/media/combined-memory, persistent installation,
G8/product and other-camera qualification.128KiB cap and500ms/5s policies remain.
Parent may disposition only this frozen narrow review. **No next test, build,
implementation or native queue follows.**
