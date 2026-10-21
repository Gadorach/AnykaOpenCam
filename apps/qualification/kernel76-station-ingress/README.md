# Station76 A2a ingress SOURCE/HOST derivative

Exact 68-file derivative of reviewed A1; only seven internal source/header files
changed. LICENSE and existing notices are retained. No original source, module or
image is changed. See the candidate report and `station-ingress-a2a-host01/` under
`docs/qualification-plans/kernel76-transport-20261021/`.

Strict qualification policy: successful USB completion, original backing/range,
and one exact unprefixed CMD/CMD2 with declared length equal to the untrimmed
actual completion length. Zero-length completion, failed status, short header,
partial/surplus control (including zero padding) refuse and poison A1's receipt
latch. This deliberately does not claim compatibility with undocumented RX
padding. TX padding rules are not RX evidence.

Only the new native HOST runner may be executed in this lane:

```
python3 -B apps/qualification/kernel76-station-ingress/run_host_tests.py --run runNN
```

It refuses existing output directories. Real changed functions are extracted with
sealed byte extents and compiled with synthetic USB/kernel primitives. The A1
semantic regression test body is copied byte-exact; no old collector is invoked.
Vendor Makefiles are preserved as source evidence, **not execution recipes**.

No ARM/module/kernel build, staging or hardware action is authorized. Event
consumer, data/aggregate grammar, concurrency, real drain, autonomous recovery,
and S1/S2/S3/S5/S6 remain unqualified; full S4/native attach remain NO-GO.
