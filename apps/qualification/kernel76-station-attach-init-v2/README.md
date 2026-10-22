# Station76 P2 v2 — bounded R1/R2/R3 source correction

Fresh byte copy of all71 frozen P2 v1 source files. Exactly four source files are
changed: `attach_init.h`, `hgic_smac/attach_init.c`, `hgic_smac/attach_status.c`,
and `utils/fwctrl.c`. The other67 files, LICENSE/notices, accepted A2b RX/event,
original core/procfs, command formats, queues/deadlines and build selection remain
byte-exact v1. V1 and its CHANGES REQUIRED review/evidence are preserved.

- **R1:** complete the required private channel-context callback group. Assignment
  returns the existing refusal; unassignment has no device action.
- **R2:** preserve historical `netdev_registered`, append presence/removal/teardown
  scalars in schema2, and require surviving owned presence for init success.
  RTNL serializes final publication with register/rollback/removal notifications.
  Unexpected loss records a local missing-device `-ENODEV`/NETDEV_LOST condition,
  not an invented original notifier errno. Expected core cleanup is distinguished;
  a later cleanup cannot reclassify an earlier unexpected removal.
- **R3:** record an associated validated negative CMD status under the existing
  FWCTRL lock, before response publication/completion; retain first response and
  earlier immutable cause, without malformed-reply conversion or payload copy.

Changed-unit HOST syntax only:

```sh
python3 -B apps/qualification/kernel76-station-attach-init-v2/check_syntax.py syntaxNN
```

Fresh output name required. This runs Clang `-fsyntax-only` on the three changed
C units against installed x86 headers; no Kbuild/object/module/ARM build, linking,
test/app execution, network/private input or hardware IO. Header changes are
covered through those units. No unchanged suite or deferred test campaign is run.

Report: `docs/qualification-plans/kernel76-transport-20261021/STATION76_ATTACH_INIT_P2_V2_CANDIDATE.md`.
Pins/commands/diagnostics/provenance: `station-attach-init-p2-v2-source01/` beside
the report. No source/native/product GO: independent review and ordinary functional
confirmation remain, as do unmet P1 and P3–P5 dependencies. No integration,
staging, hardware or publication authorization follows from this tree.
