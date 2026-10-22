# Station76 owned-control P3 — SOURCE candidate only

Fresh byte copy of exact reviewed P4a. One bounded heap FWINFO request replaces
the selected stack receipt and generic TX skb queue. It has explicit waiter,
registry, pending, queue/worker, submitter and actual-callback refs; an immutable
monotonic500ms deadline sealed before command grant/allocation; and one nonblocking
USB token admission. The real immutable first-error lock serializes queued-UNSENT
cancellation with final submit claim. A claim that won remains potentially submitted.

No async stack response pointer, unrelated-skb eviction, capacity retry, credit
poll, timer or new producer is introduced. Positive output requires both the real
A1 reply and successful USB submission return. Callback reconciliation is not
callback-return proof: registry/request final free still waits for exact P4a full
producer joins and all-slot poison, not refs/list emptiness. P4a reservation bits
remain conservative owner reservations, not request refs or active counts.

Original RX/event, LICENSE/notices, wire/A1 helper and unselected legacy files are
unchanged. P1 remains absent/unmet. No full P4/P5, native, timing/recovery, paired
build or product acceptance follows. Init/RTNL/USB/poison scheduling can still
block; this is not a2s/5s completion guarantee or physical atomic TX cancellation.

Changed-unit HOST syntax only, fresh output name:

```sh
python3 -B apps/qualification/kernel76-station-control-owned/check_syntax.py syntaxNN
```

No tests/shims/app, Kbuild/object/link/ARM/module build, staging or target execution.
Syntax01's missing netdevice declaration include failure is retained; final
syntax04 returned0 for all five changed C units, not functional proof.

Report and frozen contract/provenance/delta/syntax evidence:
`docs/qualification-plans/kernel76-transport-20261021/STATION76_CONTROL_OWNED_P3_CANDIDATE.md`
and `station-control-owned-p3-source01/` beside it. Independent exact-pin review
and ordinary functional confirmation remain required. No next queue is authorized.
