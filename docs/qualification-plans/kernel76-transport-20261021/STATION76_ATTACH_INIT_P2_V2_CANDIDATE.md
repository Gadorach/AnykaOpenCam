# Station76 P2 v2 — R1/R2/R3 SOURCE correction candidate

2026-10-22. RP-02 / NET-01–08 / K76-06, P2 / S3 only. **Best-effort SOURCE
candidate, not source-sufficiency, integrated attach, native or product GO.**

## Outcome / immutable inputs

Fresh owned paths only:

- `apps/qualification/kernel76-station-attach-init-v2/`
- this report
- `station-attach-init-p2-v2-source01/` beside this report

All **71 v1 source files were copied as bytes before editing**. Final v2 has the
same71-file source set: **67 byte-exact v1 files and four corrected files**:

```text
attach_init.h
hgic_smac/attach_init.c
hgic_smac/attach_status.c
utils/fwctrl.c
```

No additional production change was made. Build selection, USB implementation,
FWCTRL ownership declarations, command formats, accepted A1 validation/copy helper,
original core/procfs and accepted A2b `rx.c`/`event.c` remain exact v1 bytes. LICENSE
remains `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`;
existing notices/provenance are retained, with no source-license rewriting or
global license grant. All **204 accepted A1/A2a/A2b source pins** were reverified.

V1's report/tree/evidence remain frozen. Its seal is still:

```text
c54ff57f63bec1180199a97321fbf1ea38e9b448edf0969c9135953c565b1919
```

The ordinary independent review's **CHANGES REQUIRED** disposition remains valid
for v1, not silently replaced by this author's correction. Its exact report pin:
`1b9707d01381c0c89331751813b3d73b0c9a1ec951824a57348f1fa79d8e4a7b`.
The153 v1 sealed files, six review sealed files and both seals were read/hash-
verified without executing their tools or modifying their evidence. No P1 source,
partial review, retry, adoption, completion, substitute review, safeguard or model
change was used. P1 remains an unmet integration dependency.

Final v2 source-map SHA256:

```text
a23ec7ce1e9794762e086853655e286b3567db5ddb4e449c88bab25390b9c75f
```

Format: SHA256 of compact sorted-key UTF-8 JSON `source` map in
`station-attach-init-p2-v2-source01/source-inventory.json`, retaining exact file
sizes/hashes. `copy-baseline.json` pins the byte-copy baseline and immutable v1/
review/accepted-source custody. `R1_R2_R3_DELTA.patch` contains the complete four-
file delta. `candidate-seal.json` freezes v2 code/report/metadata/diagnostics and
excludes itself. V1's own pinned copy chain remains retained through those inputs.

## Exact kernel anchors read before correction

Public retained kernel source root:
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.
All four full-file hashes match the independent review. Exact line excerpts and
their range/full-file hashes are retained in `kernel-anchors.json`.

| Kernel file | Relevant ordinary API contract |
|---|---|
| `net/mac80211/main.c:813–832` | Non-emulation channel-context implementations require all five callbacks. |
| `include/net/mac80211.h:4763–4777` | Exact add/remove/change and assign/unassign callback types, including `struct ieee80211_bss_conf *`. |
| `net/core/dev.c:11401–11410,11422–11430` | Later REGISTER notifier failure can roll registration back; earlier init failure can emit PRE_UNINIT. |
| `net/core/dev.c:12280–12290,12328–12376` | Removal requires RTNL, unlists before UNREGISTER and subsequently emits PRE_UNINIT. |
| `net/mac80211/iface.c:2376–2390,2393–2407,2422–2427` | Interface registration error propagates locally; removal requires RTNL. |
| `net/mac80211/main.c:1595–1636` | Automatic default interface creation failure can be warned while hardware registration ultimately returns0. |

These are source deductions, not induced or observed runtime failures.

## R1 — complete the required callback group without device effects

**V1 defect:** private add/remove/change channel-context callbacks lacked
assign/unassign. Linux6.18.55's `ieee80211_alloc_hw` therefore rejects the table
before publication/FWINFO/init. Syntax-only success did not detect this semantic
ops-table requirement; v1's `-ENOMEM` would not prove a real allocation shortage.

**V2 correction:** `attach_assign_vif_chanctx(hw, vif, link_conf, ctx)` has the
exact required typed signature and returns existing `attach_refuse()` before any
effect. The identically typed void unassign callback performs no device/channel
action, since assignment cannot succeed. Both are wired into `attach_ops` next to
the existing private channel callbacks. No emulation fallback, success stub,
channel programming or additional opcode was introduced.

## R2 — surviving presence is not a historical REGISTER milestone

**V1 defect:** `netdev_registered=1` meant that one REGISTER notification was seen.
Rollback/unregistration could remove that device while this historical value
remained1, allowing the old final init-success predicate to accept no netdev.

**V2 correction:** schema2 preserves `netdev_registered` as the historical
milestone and appends four bounded scalars, copied coherently with all original
receipt fields under the existing receipt lock:

| New field | Meaning |
|---|---|
| `netdev_present` | Latest owned-lifecycle observation: set on accepted REGISTER, cleared on UNREGISTER/PRE_UNINIT. Not a held reference or retirement proof. |
| `netdev_removed` | Sticky first removal observation; repeated UNREGISTER/PRE_UNINIT is idempotent. |
| `netdev_teardown` | Existing core-remove entry has begun; classification marker only, not P4 drain completion. |
| `netdev_removal_expected` | First removal was observed after core-remove entry; a later teardown cannot reclassify an earlier unexpected loss. |

Ownership continues to use the existing one-wiphy/one-registration lifetime rule.
No netdev address/pointer/reference is retained or exported. Duplicate REGISTER
still fails rather than creating another registration generation or resurrection.

Unexpected owned UNREGISTER/PRE_UNINIT clears presence, records removal and latches
**local `-ENODEV` / `HGIC_ATTACH_NETDEV_LOST`**. The new phase is appended, leaving
old phase numbers unchanged. This denotes the observed missing owned device:
**the original later notifier/default-interface failure errno is unavailable and
is not guessed or claimed exact**. Removal returns NOTIFY_DONE, not a fabricated
failure from the ongoing unregister operation. Earlier immutable cause wins.

Final init publication now reacquires **RTNL before the receipt lock** after
`ieee80211_register_hw` returns. This closes the interval where removal could
unlist a netdev before reaching its notifier while publication read stale cached
presence. Its predicate requires current presence, no removal, no teardown/REMOVE,
and all previous strict FWINFO/registration/DOWN/error-free conditions. A missing
device gets the same local absence condition rather than a guessed registration
errno. Source-requested cancellation remains `-ECANCELED`, not a successful init.

Core-remove marks expected cleanup under the receipt lock before the existing
stage cleanup. Pending init is still terminally canceled by the existing logic;
healthy terminal init's expected cleanup does not create a spurious registration
loss error. Unexpected removal following init success records an error while
preserving the historical SUCCEEDED milestone. No late success repairs failure.

Status now explicitly labels **`policy=attach-init-p2-v2-source01 schema=2`**, keeps
all old field meanings and prints the four appended scalars. It remains a fixed
read-only module-owned snapshot, not a device query, pointer export, progress feed
or platform ABI. There is no new teardown/refcount/cancellation implementation.
RTNL acquisition is an ordinary blocking lock, **not a new finite-deadline proof**.
P3–P5 still must account for waits and guard/retirement budgets before native use.

## R3 — capture the validated negative reply before completion publication

**V1 defect:** the RX response path obtained a valid associated signed CMD status
but only the waiting command caller later recorded it as first-error. A later
USB error could be latched during that wakeup delay and replace the earlier
observed firmware failure as the apparent first cause.

**V2 correction:** within `hgic_fwctrl_rx_response`, after successful existing
association/status validation and while its **existing FWCTRL lock remains held**,
`status < 0` calls `hgic_attach_error(status, HGIC_ATTACH_COMMAND)` **before**
`resp->skb`, `resp->done` and completion/wakeup publication. Existing receipt-lock
nesting remains FWCTRL → receipt; no reverse lock acquisition was added.

This does not invoke malformed-reply handling, set a generic `-EPROTO`, replace
the first response or copy negative-response payload. The signed response remains
the first owned response; the waiter sees the already-latched cause (or an earlier
immutable cause) and does not copy payload. No stack waiter pointer/reference,
request cancellation, wire token, retry or P3 mechanism was added. This removes
the identified deferred-until-waiter gap; syntax-only evidence is not a general
concurrency or total event-order proof.

## Compile-only evidence / commands

Executed only source-copy/integrity bookkeeping and ordinary changed-unit HOST
syntax checking:

```sh
python3 -B apps/qualification/kernel76-station-attach-init-v2/prepare_source.py
python3 -B apps/qualification/kernel76-station-attach-init-v2/check_syntax.py syntax01
python3 -B apps/qualification/kernel76-station-attach-init-v2/freeze_candidate.py inventory
python3 -B apps/qualification/kernel76-station-attach-init-v2/freeze_candidate.py seal
```

`syntax01`: **3/3 changed C units returncode0** (`attach_init.c`, `attach_status.c`,
`fwctrl.c`), including the changed receipt header. Actual Clang `-fsyntax-only`,
implicit-function and incompatible-pointer errors enabled; installed HOST x86
Linux6.18.55-2-cachyos-lts headers, Clang23.1.3. Compiler/version, full argv/UTC,
71-file source, header dependencies, inputs and diagnostics are pinned in its
receipt. Installed x86 header nocf/address-space warnings remain retained; this
is not a warning-free or paired ARM build claim.

No unchanged suite or16-unit re-run, test/app executable, adversarial/penetration/
mutation/fuzz campaign, Kbuild/object/link/ARM/module build or staging occurred.
No target/UART/RPC/status/network/private IO, parent operation polling, frozen
boot/helper edit, Git/index/AGENTS/central-ledger modification occurred. The parent
owns its active native operation; this source lane did not query or disturb it.
Hardware state, original radio FAILED/HOLD and native completion are not inferred.

## Remaining gates / next action

- V2 is an **author correction candidate awaiting independent exact-pin review**;
  R1–R3 are source changes, not executed functional results. Missing ordinary
  functional confirmation includes actual allocation/one-shot FWINFO/registration/
  DOWN, registration rollback/removal receipt behavior, expected cleanup and
  signed command failure observation before waiter completion. No deferred test
  requirement is marked passed, and no such run is queued here.
- **P1 remains unmet and intentionally absent**; no review/retry/adoption or
  composite attach claim follows. Accepted A2b RX/event bytes are unchanged.
- **P3 remains absent:** finite end-to-end deadlines, owned-unsent cancellation,
  request/queue/ref accounting and serialized error-versus-submit transition.
  Existing500ms command ceiling,1000-jiffy USB token wait and queue design remain.
- **P4 remains absent:** exact descriptors, complete allocation rollback including
  the unlisted RX URB leak, finite initial RX submission, callback/submitter/ref/
  worker drain ordering and normal-unload/final-retirement proof. New R2 scalars
  and the expected-removal marker must not be presented as that proof.
- **P5 remains absent:** reviewed bounded watchdog/controller/source-generation/
  admission/progress/retirement contract. A status snapshot does not authorize
  feeding a watchdog or selecting the old fixed guard.
- No physical RF-silence, native station, network/media/memory, paired ABI,
  persistent install, product or other-camera gate is advanced. No thresholds,
  recovery budgets, allocation ceiling or original accepted evidence changed.

**ONE next eligible action:** parent ordinary SOURCE review of this exact frozen
v2 correction/metadata, without touching blocked P1 or selecting new tests, build,
integration, staging or native work. No source/native/product GO is asserted.
