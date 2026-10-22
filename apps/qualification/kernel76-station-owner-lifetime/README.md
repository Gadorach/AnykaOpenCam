# Station76 P4a shared-owner lifetime — SOURCE candidate only

Fresh byte copy of71 frozen P2 v2 files. Six imports change and one new selected
USB unit is added. Original `utils/if_usb.c`, accepted A2b RX/event, LICENSE,
wire declarations, original core/procfs and all old candidates/evidence stay exact.

The selected `utils/owner_usb.c` uses immutable16TX/32RX slots,2560-byte RX backing,
one permanent bus STOP/work admission fence, split core producer-stop/final-free,
and permanent poison of every allocated URB outside callback locks. Producers join
before the final poison sweep; callback/private owners remain live until all
kernel poison calls have returned. Slot lease counters are conservative reservations,
**not active/inflight counts**; only external join/poison return proof clears them.
Uncertain retirement blocks without freeing or returning from disconnect/unload.

Partial allocation cleanup and finite32-entry initial RX enumeration are included
because stable owner enumeration requires them. No request heap/cancellation/
deadline implementation, full descriptor/tombstone/status expansion, P5 controller,
data/UP/radio or native qualification is added. P1 stays unmet. Inherited500ms
reply and1000-jiffy token waits remain; no5s guard or timing proof is supplied.

Changed-unit HOST syntax only, fresh output name:

```sh
python3 -B apps/qualification/kernel76-station-owner-lifetime/check_syntax.py syntaxNN
```

No Kbuild, objects/link/module/ARM build, tests/app/native/hardware execution.
Run01's missing skb include failure is retained; corrected run02 returned0 for
all four changed C units. This is not functional/source/native/product GO.

Read the contract written before implementation, final scope/limits, source/API
anchors and pending ordinary confirmation in:
`docs/qualification-plans/kernel76-transport-20261021/STATION76_OWNER_LIFETIME_P4A_CANDIDATE.md`.
Pins/provenance/delta/compile metadata are in `station-owner-lifetime-p4a-source01/`
beside the report. Parent independent review is required before further selection.
