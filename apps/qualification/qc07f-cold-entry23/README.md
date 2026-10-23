# QC07F cold-entry23 — parent-only transaction candidate

Read [CONTRACT.md](../../../docs/qualification-plans/qc07f-cold-entry23-20261023/CONTRACT.md).
**Not independently accepted or run.** This is one foreground cold observation,
not a boot tool, reusable controller service, image build or native admission.

`cold_entry.py` directly retains the original recorder's `subprocess.Popen`.
It does not call/import old boot controllers, `start_receiver`, `PrivateBoot`,
old22b logic, or the original main. Boot source supplies only AST-read adapter
association; recorder source supplies the historical prerequisite path. Neither
selector is duplicated here. The unchanged pure classifier is loaded at parent
runtime from its exact pinned public bytes and canonical `__file__`, without
using a cached classifier bytecode file. All source/marker objects stay read-only.

## Parent selection, not worker execution

Parent must first establish current serialized bench custody, current RX/TX/GND-
only wiring and the exact historical adapter association. The required combined
confirmation flag records these caller facts; it does not measure or discover
them. Parent independently reviews the entire source and chooses a fresh part.
Do not infer present power/runtime or automatic retry authority from this README.

Use a trusted **isolated** Python interpreter (`-I -B`), the same system dependency
environment used by the child. Supply its independently reviewed executable SHA256,
the canonical actual system `serial.__file__` and its independently reviewed SHA256.
The script verifies those sources and import identity, but trusts the ordinary
system Python/stdlib/pyserial dependency closure and trusted serialized namespace;
this is not a hermetic dependency audit. User-site-only pyserial will refuse under
isolated mode; no installation/environment workaround is performed automatically.

Parent-only argument shape (placeholders are deliberate; not executed by author):

```text
<trusted-python> -I -B <canonical-workspace>/apps/qualification/qc07f-cold-entry23/cold_entry.py
  --private-parent <existing-owned-0700-outside-all-Git-directory>
  --power-config <explicit-existing-private-0600-camera-power-config>
  --port <explicit-exact-historical-by-id-port>
  --part foundation-20261023d
  --python-sha256 <reviewed-current-interpreter-sha256>
  --serial-init <canonical-system-serial-init-source>
  --serial-sha256 <reviewed-serial-init-sha256>
  --confirm-current-uart-only-serialized-historical-association
```

No default/discovery/fallback URL or adapter. Configuration has exactly the public
schema supplied by parent: `utc`, `endpoint`, `actual_user_load_association`,
`initial_OFF`, `one_idempotent_turn_off_POST_HTTP200`, `final_OFF_readback`,
`ON_requested`, `independent_electrical_rail_measurement`. The explicit HTTP URL
must have numeric RFC1918 IPv4 host, optional numeric port, and the exact
`/switch/outlet_power` path, without credentials/query/fragment. No remote is
contacted until runtime configuration/association checks and private intent exist.
HTTP requires200, no redirect, JSON content type, unique JSON keys, exact entity
`switch-outlet_power`, Boolean `value`, and consistent `state` string ON/OFF.
Bodies cap at4096 bytes and socket operations use3s timeouts. Neither timeout nor
the fixed120s recorder loop establishes a hard whole-transaction host deadline.

## Actual bounded order

1. Retain public sources, self/interpreter/serial source, the source-bound original
   read-only854-byte historical prerequisite, explicit private configuration, and
   read-only radio/reset/instance locks. No historical final manifest is invented.
   Exact historical marker modes are0644/0600/0644; instance is empty. No competing
   socket or visible fuser owner is adopted, removed, killed or retried.
2. Create fresh `cold-entry-<part>/` and private immutable HOLD intent. GET must
   already report OFF: there is no preflight OFF write to an unconfirmed load.
   Recorder alone creates the absent `capture/` leaf. All new files0600, dirs0700,
   outside every Git tree. All private identities/hashes/errors remain there.
3. Direct Popen of the pinned original recorder, explicit port/part/private capture,
   fixed120s duration. Retain actual Popen, PID/start/argv and same peer UID/GID.
   Validate zero RX/TX/inspection/busy and sole visible fuser ownership. Request
   exactly one original volatile hold-cli arm without automatic inspection.
4. Recheck sources/owner/armed zero counters; persist private ON intent; final
   owner/status point-check; one ON POST. Nominal3s settle, one status, full private
   capture explanation; separate nominal10s quiet, one status and full-byte/event
   comparison. No UART/RPC polling during automatic catch transmission.
5. Exact first360 bytes must match all19 original records. Every later byte must
   equal the118-byte fixed catch echo group × (actual complete19-byte packets - 1),
   with1..3 packets and exact arm/trigger/TX event lineage. No arbitrary slicing,
   second-banner search, normalization, prefix repair, fallback profile or command
   reply acceptance. Unsupported legitimate variants stay HOLD. Failed23c stays
   failed; the old classifier and originals remain untouched.
6. After any attempted ON, exactly one OFF POST, then independent GET OFF even if
   POST failed. A failed POST remains sticky HOLD even when GET proves OFF. Only
   proven OFF permits same-peer original stop and actual owned Popen.wait(5) with
   exit0. No kill or stop retry; lost stop ACK remains HOLD even if child exits.
   Unknown OFF preserves recorder and demands manual OFF. Pre-ON failures may
   retire only the known own child while initial OFF remains proven.
7. Verify actual private final raw/event manifest hashes, exact final event trail,
   socket gone/fuser none, retained bytes/FD/path metadata/ancestry, then emit only
   public scalar PASS/HOLD. Every retained object gets an independent final check
   before any owned descriptor/lock is released. Immutable intent is never cleared;
   a separate fresh result/failure file records the outcome. No next action queues.

## Explicit limits and gaps

No worker tests/imports/calls/private reads or hardware access. Only public-source
AST/syntax compilation is recorded in the source report; parent review and actual
functional operation are outstanding. The source is longer than a minimal power
script because startup failure, OFF uncertainty, original read-only custody and
owned-child finalization are kept in this one transaction rather than delegated to
a new framework. No runtime claim is made from that structure.

Visible fuser plus cooperative locks is conditional trusted-bench exclusion, not
global privileged-process proof. Recorded DTR/RTS requests and user wiring reports
are not electrical waveform measurements. Initial serial flush loses earlier bytes.
The unchanged recorder permits additional RPCs; caller must exclude other clients.
Stored arm ACK expiry is not a native clock/recovery lease. Host hang/suspend/power
loss can prevent OFF; outlet state is not independently measured camera voltage.
On unknown power, manual OFF is required; on unknown child, preserve its evidence
and investigate rather than take over. Strict C1v2/native clock, staging, bootstrap,
backing, watchdog, media, motor, flash and product gates remain unchanged.

License: new source declares GPL-2.0-only, consistent with the reused pure classifier;
all original source/license notices remain in place and no upstream relicense or
publication permission is inferred.
