# Station76 P2 attach-init — ordinary SOURCE/build-custody review

2026-10-22; RP-02 / NET-01–08 / K76-06, P2 / S3 section4 only.

## Disposition

**CHANGES REQUIRED; no narrow source-sufficiency acceptance for these pins.**
The selected callback table cannot pass Linux6.18.55's hardware allocator. Two
additional ordinary lifecycle/first-error gaps are documented below, conditional
on fixing that first blocker. These are direct source/API findings, **not executed
functional observations**. The candidate's final16-unit HOST syntax evidence is
authentic to its seal but cannot detect this callback-table semantic failure.

P1 remains an unmet, intentionally absent integration dependency. This review
does not read/adopt/retry/complete its blocked source or partial review. P3–P5,
normal functional confirmation, paired ARM/module build, native and product gates
remain unmet. No composite attach, RF-silence, native or product GO follows.

Only this new report and `station-attach-init-p2-review01/` were written. No source,
candidate report, Git/index, central status, helper or boot recipe was edited.
No candidate program, compiler, Kbuild, unit/runtime test, adversarial/fuzz/mutation/
penetration test, target/UART/RPC/network/private input or native job was executed
or accessed. Parent's exclusive RAM76 operation was not queried or disturbed.

## Pins and reproducible custody

`S/` below is `apps/qualification/kernel76-station-attach-init/source/`.
`K/` is the retained public source checkout
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux/`.

- Exact supplied `station-attach-init-p2-source01/candidate-seal.json` SHA256:
  `c54ff57f63bec1180199a97321fbf1ea38e9b448edf0969c9135953c565b1919`.
- **153/153 sealed files**, exact71-file candidate source set and **204/204 accepted
  A1/A2a/A2b source pins** independently read/hash-verified; no compilation rerun.
- Source-map SHA256 (sorted-key compact UTF-8 JSON, as author defined):
  `a18b8aef67c04fc66acf5805794e9fa277c19f1bfbfd078be207fe633bfc2447`.
- 62 unchanged imports / six changed imports / three new files confirmed. LICENSE,
  original core/procfs, A2b rx/event and A1 validation header remain exact imports.
- LICENSE SHA256 `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`:
  retained GPLv2 text. New files carry GPL-2.0-only identifiers; imported source,
  notices and the pinned A1→A2a→A2b copy/preparer chain remain retained. This is
  source/provenance verification, not a publication or global-license decision.

| Reviewed changed/new S/path | SHA256 |
|---|---|
| `hgic_smac/Makefile` | `a14626c1cc4c7a39a54d9b4eeea39fa7c4c54afc097fd9aed8cd1f1ae2a959b9` |
| `hgic_smac/attach_init.c` | `d28d4a59dff7149dcacbb1030d40c098b84334dbf3013f7d53647f4f646ba4c5` |
| `hgic_smac/attach_status.c` | `2ed87029172917352e7f983c6dee4a0199b5fe01cfccfcb3364852164a9b159f` |
| `attach_init.h` | `b87e394ac1268b766a4bffacb271e743b2acff67b2c9b4170a7a4efd23086926` |
| `hgic_smac/hgics.h` | `6701f2c7b07c667e70b939af37967acaf73b08e405fe0519f03c0c1f45a52a6a` |
| `hgic_def.h` | `d4245c831975c635f31f85ef8017826b4e8c0e71ef6db025581a8db4abca3f1c` |
| `utils/fwctrl.c` | `029ac03598c2b44e46ee2d9a1efe970f3bad15d8e1357917873aa89e3d776927` |
| `utils/fwctrl.h` | `7d64e1bbd0c46d234e9acb4f7814a9ccdabbac9653da6e02f6b43bcdd006ca49` |
| `utils/if_usb.c` | `84c44b31e95bee4a71dff54a7d7091a010fddc67ba4ec9a8bd9f9b4c5ff37cdc` |

| Kernel API input K/path | SHA256 |
|---|---|
| `net/mac80211/main.c` | `38047a679e48375f6d8cae18b6b2746d89dac38cde4ee62602ed3e56ca957e74` |
| `include/net/mac80211.h` | `43b4be700a0aa3d5b4e5ac3483db02d2506ba637e2686343b053a1d05949067f` |
| `net/core/dev.c` | `83a1a83bdf7a979b0aa27170272748c5a71070ad52227243a8f391008ebed80c` |
| `net/mac80211/iface.c` | `461e696280b214723762d85aad60641d9792f57d72a8f1f3c08c90f724aaec6f` |

Executed review reader only:

```sh
python3 -B docs/qualification-plans/kernel76-transport-20261021/station-attach-init-p2-review01/read_public_inputs.py
```

Evidence there: `reader-receipt.json` retains argv/UTC/Python/full-file public input
pins; `public-pin-verification.json` retains count/provenance/final-syntax matching;
`public-excerpts.json` retains exact line ranges/text/range hashes including the
kernel APIs used below. `review-seal.json` records final input revalidation and
review-output pins; it excludes itself. Static equality is not a behavioral test.

## Concrete findings

### P2-R1 — blocker: incomplete channel-context callbacks reject the core before init

`S/hgic_smac/attach_init.c:225–248` supplies private add/remove/change channel-context
callbacks but **neither `assign_vif_chanctx` nor `unassign_vif_chanctx`**. They are
not the three mac80211 emulation callbacks.

`K/net/mac80211/main.c:813–832` requires the complete five-callback group for this
non-emulation case. In particular:829–831 warns and returns NULL when either
assignment callback is absent. `S/hgic_smac/attach_init.c:455–458` calls this allocator and maps its NULL result
to `-ENOMEM`;:518–520 caches/returns that error. USB probe:462–466,485–497 propagates
the returned core-probe error. Consequently the selected source cannot reach core
publication, `probe_post`, the one FWINFO or registration on this kernel. The
observed receipt would misleadingly resemble an allocation shortage, although the
source defect is an invalid ops table. This is an API deduction, not a native run.

**Smallest next implementation patch:** add the two correctly typed private
callbacks to this new attach core and wire them into `attach_ops`:

- `int assign(hw, vif, struct ieee80211_bss_conf *link_conf, ctx)` returns the
  existing `attach_refuse()` before effects.
- `void unassign(hw, vif, struct ieee80211_bss_conf *link_conf, ctx)` performs no
  device action: assign can never succeed, as with existing remove callbacks.

Exact declarations: `K/include/net/mac80211.h:4770–4777`. Do not replace the table
with success stubs or enable channel programming to satisfy allocation. This is
the **one smallest next patch**, not implemented by this read-only review and not
authority for a module/native trial. Ordinary syntax confirmation would be useful
after an authorized correction, but cannot itself establish registration behavior.

### P2-R2 — init-success predicate uses a registration notification, not surviving registration

`S/hgic_smac/attach_init.c:535–541` sets the historical `netdev_registered=1` during
NETDEV_REGISTER;:523–549 has no handling of NETDEV_UNREGISTER/PRE_UNINIT.
The success predicate:359–369 treats that milestone as sufficient registration.

The actual API permits a normal registration failure **after this notifier has
run**: `K/net/core/dev.c:11401–11410` runs the entire REGISTER chain and rolls the
device back if a later notifier returns an error. Rollback emits UNREGISTER
(`K/net/core/dev.c:12353–12356`). `K/net/mac80211/iface.c:2376–2379` returns that
failure, but `K/net/mac80211/main.c:1607–1616` only warns if its automatic default
interface creation fails and can still return0 at:1636.

Thus, after R1 is repaired, a later REGISTER notifier's ordinary failure can leave
no default netdev but leave P2's count1/error0. `attach_register_hw` then marks
hardware registered and init can publish SUCCEEDED. An interface removed between
registration and publication likewise is not detected. These are conditional
source paths, not induced failures or reproduced runtime results. The candidate
truthfully calls these receipt fields historical, but a historical milestone alone
does not meet section4's **actual successful netdev registration** success gate.

**Minimal subsequent correction:** keep the historical milestone if desired but
also cache owned-netdev presence/removal under the same receipt lock. An owned
UNREGISTER while init is pending must prevent success and latch a registration
failure; require presence in the final predicate. Distinguish expected core removal
from loss during init, without retaining a dangling netdev pointer. The original
later notifier's exact errno is not exposed to this observer; do not invent it.
No widening of device effects, P4 retirement claims or automatic retry is needed.

### P2-R3 — signed firmware failure is observed before its first-error latch

`S/utils/fwctrl.c:235–247` validates a CMD/CMD2 reply and obtains signed `status`
but does not latch a negative value before publishing the completed response.
Only the waiting caller later copies/returns that negative status and records
it at:125–142. If a USB completion/error occurs after validation but before the
waiter runs, `if_usb.c:273–275` can latch that later USB cause first; the waiter
then returns it at `fwctrl.c:126–127` rather than the earlier firmware failure.

Therefore the new receipt means "first call to the error recorder", not always
"first observed error", for the explicitly included signed firmware-negative
path. Its first-error immutability and fail-closed outcome are not disputed; the
cause/phase chronology is incomplete. This is an ordinary asynchronous ordering
deduction only; no concurrency test was performed.

**Minimal subsequent correction:** on an otherwise valid associated CMD reply,
latch `status < 0` with its command phase while still holding the existing FWCTRL
lock, before completion/wakeup. Keep the negative reply as the first response,
no copy of its payload and no conversion to generic malformed-reply `-EPROTO`.
This preserves the documented lock order FWCTRL→receipt and needs no P3 stack
pointer, cancellation mechanism, wire token or retry.

## Other bounded source observations

These describe ordinary selected source intent, not independently tested passes:

| Section4 concern | Actual selected implementation / disposition |
|---|---|
| Build selection | `S/hgic_smac/Makefile:5–25` selects attach_init/status, not original core/procfs. Its Linux2G+USB units match the author's16 syntax units. SDIO/AH/STABR/RTOS selections are explicitly rejected by `attach_init.c:13–17`. |
| Real init path | Module init:555–587 → USB probe and initialized core publication:434–514 → one `probe_post`:373–390 → worker:328–371 → strict `hgic_fwctrl_attach_fwinfo`:477–501 → register:310–325. R1 currently prevents reaching it; no substitute success path was found. |
| DOWN and UP | Owned PRE_UP returns notifier-encoded `-EPERM` (:523–548); kernel `net/core/dev.c:1682–1705` propagates it before ndo_open/IFF_UP. Start independently refuses at:98–101 before DEV_OPEN/radio/START/queue effects. No native UP attempt is needed or authorized. |
| Other callbacks | Interface/config/AP/key/station/TXQ/aggregation/channel callbacks:111–247 refuse; key refusal makes no key copy. TX frees refused skb, not an ACK. Filter:158–162 only clears local capability bits. Stop/remove no-op device cleanup is consistent with never-successful start/add, not synthetic firmware success. R1's missing assignment pair remains an exception. |
| Sole FWINFO | `fwctrl.c:102–123,305–347,477–533` binds current init caller, one command and allocated skb; only existing GET_FW_INFO/STA/no input/24-byte output is admitted. `control_receipt.h:27–96` retains exact accepted retail validation and signed status/copy rules. USB:293–324 consumes the same skb's permission before transport. No CMD2 TX permission follows from CMD2 RX. |
| Recovery/loader exclusion | Selected core has no download/RUN/boot query/reset/reinit/reopen path. Detect/test works and timers:300–308 only refuse; initialized timers:482–483 are not armed. TX completion:250–261 never clears BUS_ERROR/firsterror. |
| Retained implementation versus reachable init | Makefile still links fwdl/ota/iwpriv/bluetooth/AP objects; their code is **not claimed absent from a binary**. The new core does not initialize loader/OTA/HCI/writers, and direct send_data/USB gates reject non-owned/non-FWINFO TX including BOOT ENTER. No runtime fallback/profile toggle was found in selected init. |
| Overrides | Nonzero test/aggregation/dack/BT, changed txq size and no_bootdl!=1 return-EINVAL before status/notifier/USB setup (:559–562); source defaults are not silently coerced. |
| Firsterror/init state | Receipt lock:31–89 makes first cause immutable and NOT_STARTED/RUNNING terminal FAILED/done1; later success does not clear it. One binding:441–448 and one queue/start:333–343,378–389. Final publication is gated but R2/R3 leave the stated gaps. Init success is not inferred from USB driver registration alone or a firmware event. |
| Work/lock ordering | Core initializes queues/locks/work/timers before hghw matching or publication (:469–513); ctrl readiness is explicit, including missing workqueue failure. FWCTRL→receipt nesting has no reverse acquisition in receipt helpers. Registration, submission and synchronous cancellation are outside receipt lock. No async pointer to stack response was added; local skb identity is not an extra reference. |
| Stage-aware cleanup | Core:392–431 checks primitive/control/hardware stages before cancellation/release/unregister/free. USB:443–497 refuses partial requested counts, respects exact core-probe return, clears actual publication and drops acquired dev ref on probe failure. These narrow improvements do **not** solve the separately acknowledged P4 drain/submitter/immediate-callback lifetime problems. |
| Module setup/status | :563–593 propagates proc/notifier/USB registration errno and unwinds established stages. No-device registration remains NOT_STARTED. `attach_status.c:10–35` uses one locked module scalar snapshot/no device-backed seq private pointer;:37–61 owns/removes only fixed read-only proc entries. Failed module load does not leave a claimed persistent tombstone. |

## What the compile evidence does and does not establish

All153 sealed author files, including compiler logs/dependency listings/receipts,
matched their hashes. The final `syntax03/receipt.json` pins exactly the reviewed
71-file source map and records **16/16 returncode0** commands with `-fsyntax-only`,
implicit-function and incompatible-pointer errors enabled. Its compiler log says
Clang23.1.3, `x86_64-pc-linux-gnu`; recorded installed headers are
Linux6.18.55-2-cachyos-lts. This reviewer did not rerun those commands or rehash the
external compiler/header installation; author-recorded binary/header pins remain
cited provenance, not fresh compiler validation. See the full command summary in
`public-pin-verification.json` and sealed original diagnostics for retained warnings.

No object/module/link/API-behavior evidence exists in those commands. In particular,
valid C function-pointer types do not establish a complete mac80211 ops table.
Root `S/Makefile:30–33,69–81` still contains inherited MIPS SDK/default recipes and
strip/copy operations; it is provenance, not a reviewed paired ARM build recipe.
The selected subdirectory Makefile was read, never invoked.

## Remaining boundaries / handback

- P1 is missing and unaccepted; this exact A2b-derived tree is intentionally not
  integrated with it. No partial blocked-work claims are adopted.
- P3 end-to-end deadline, owned-unsent cancellation and serialized submit/error
  transition remain absent.500ms response wait and1000-jiffy USB token wait remain
  different budgets; local one-shot permission is not retirement or atomic submit.
- P4 exact descriptors, allocation rollback (including the unlisted RX URB leak),
  finite initial RX submission and full worker/submitter/callback/reference/drain
  ordering remain unresolved. Source stage guards do not qualify normal unload.
- P5 reviewed bounded controller/watchdog/generation admission remains absent.
  Receipt reads are not permission to operate/feed hardware.
- Normal one-shot FWINFO/registration/DOWN behavior, truthful terminal/error
  observation and stage unwind still need later explicitly selected ordinary
  functional confirmation. No deferred test coverage is marked passed here.
- Physical RF silence, pairing/ARM/module/native/product gates and original
  radio FAILED/HOLD are unchanged; no hardware state is inferred or observed.

**ONE next source action:** implement R1's two refusal/cleanup callbacks in a fresh
parent-authorized source correction, preserve these candidate pins/evidence, then
review the correction and address R2/R3 before claiming P2 source sufficiency.
No build, test expansion, P1 retry, integration or native action is queued here.
