# Station76 A2b event suppression candidate — SOURCE/HOST ONLY

Fresh 68-file derivative of accepted `kernel76-station-ingress/source`, preserving
66 files byte-exact and changing only `hgic_smac/rx.c` and `hgic_smac/event.c`.
LICENSE and existing notices remain unchanged. No firmware binary is included.

EVENT/EVENT2 require exact original extent before trim. The real consumer validates
its envelope and silently frees ID18 without accessing payload bytes. Every other
ID denies through the existing locked receipt-error latch and frees the owned skb.
There is no event queue/export/RTOS forwarding or structured event payload ABI.

Fresh exclusive HOST runs (never a source Makefile):

```sh
python3 -B apps/qualification/kernel76-station-events/run_host_tests.py --run runNN
```

The runner requires a nonexistent `runNN` evidence directory. Final author run is
`run08` in `station-event-a2b-host01/`. `test_control.c`, `test_ingress.c`, `shim.h`
and `ingress_shim.h` are byte-exact A2a regression inputs; their event stubs are
**not A2b consumer coverage**. `test_events.c` and `event_shim.h` exercise the real
consumer, actual USB callback, ingress, worker and A1 copy path. All primitives,
scheduling and transport effects are deterministic HOST fakes, not hardware proof.

See `docs/qualification-plans/kernel76-transport-20261021/STATION76_EVENT_A2B_CANDIDATE.md`.
Independent exact-pin review is required before any later build/use. Full S4,
S1/S2/S3/S5/S6, non-event grammars, ARM build, staging and native attach remain
OPEN/NO-GO. No target operation or effect is queued.
