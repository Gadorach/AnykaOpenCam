# Exact public anchors and reading limits

Abbreviations: C = `apps/qualification/encoder-query77-custody-v2/custody.py`;
B = `scripts/qc07f_boot.py`; R = pinned
`work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py`;
D = `docs/qualification-plans/encoder-query77-controller-design-20261022/IMPLEMENTATION_CONTRACT.md`.
Every file/document in this table has an exact row in INPUT_PINS01.tsv. Line
references are to those bytes, not to current processes or referenced evidence.

| Anchor | Actual source fact / limit |
|---|---|
| C62–65,123–206 | Strict built-in int bounds; immutable Epoch/Observation/Budget types. Epoch binds target runtime/boot_id, not a separately identified host clock boot/time namespace. Remaining lifetime is supplied, not measured. |
| C209–220,223–262 | Previous checked against original; current checked against original AND previous. Equal MONOTONIC/BOOTTIME/wall deltas, nonregressing RX/TX and no catch revival; original natural and previous decayed caps; reserve150s+cost. No clock API call. |
| C237,252–262 | Initial min(wall expiry difference, supplied process remaining), natural remaining from original, earlier previous deadline; Budget adds cap to observation mono. Valid record arithmetic does not authenticate measurements. |
| C445–472,512–524 | Constructor makes initial record from supplied Epoch, stores clock fields in immutable marker. New sampling representation would also require changed serialization, not just a validator wrapper. |
| C418–424,616–622,763–784 | Exact observation ledger field shape, record/total/sequence limits and one-shot append. Adding raw intervals needs a new reviewed schema within existing bounds. |
| C548–565,677–753,755–810,813–831,926–962 | Read-only HOLD/root/source checks, independent available failure/close checks, sticky primary, unknown-write poison, no retries. Preserve these through any future clock amendment. |
| C854–861,884–892 | Actual observe/complete callsites use internal original/prior state and REQUIRED catch. No proposed native collector is present. |
| custody-v2 README18–29,70–79; review38–46; parent disposition16–32 | Narrow R1/R2/path acceptance; exact clock deltas explicitly have no admitted sampler. Supplied record equality is not clock/owner/catch authenticity or native permission. |
| D116–155 | Full native identity/lineage tuple; min arm/PID lifetime mapped to immutable mono deadline; only earlier minima;30s transition +120s recovery +remaining phase cost; any suspend/discontinuity invalidates. |
| D164–185 | Phase caps still missing; proposed10s quiet is new;2312.1s old transfer plan omits staging/etc and is not measured throughput. Reserve is host observation margin, not target reset. |
| B119–180 | Source-only PID-start/argv/duration parsing and BOOTTIME minus start ticks/CLK_TCK lifetime. Uses floating-point reads/division, not nanosecond interval arithmetic or a proof of host starttime rounding. No proc/sysconf call made here. |
| B320–341 | Source peer/start/adapter ownership association; remaining=min(ends_by minus current UTC, process remaining). Adds it to a later monotonic read then keeps minimum. No simultaneous sample or bound on intervening latency. |
| R95,102,136 | Event UTC and monotonic_ns are separate calls; timer start is float monotonic and loop compares elapsed against duration. Timer checked at loop boundary, not an atomic kill/coverage guarantee. |
| R204–247 | Status does not expose original monotonic start/deadline. Arm record has no native expiry field; ACK ends_by uses UTC-now plus duration minus a later monotonic elapsed read, with float/timedelta precision. It is a derived projection, not an atomic wall/boot/mono sample. |
| R158–181,245–247; D144–149 | Catch trigger consumption/TX source and arm expiry are distinct requirements. Cannot infer current unconsumed catch from an old ACK or bound deadline. Prior raw read at R217 remains a separate protected-lineage dependency; not opened here. |
| WATCHDOG_RESET22B_RESULT5–32,42–52 | Failed reset and separate reconciliation; consumed catch, historical12:12 held prompt. Neither failure reset nor live coverage is replaced by this clock packet. |

## Public host API documentation, not live host identification

| Public document / lines | Scope actually supported |
|---|---|
| `/usr/share/man/man2/clock_gettime.2.gz`:1 | Alias to clock_getres.2; raw gzip hashed. |
| decompressed `clock_getres.2.gz`:41–68,98–113,168–215,265–273 | Single clock per gettime; resolution implementation-dependent; REALTIME settable/adjustable; MONOTONIC excludes suspend and can slew/return identical consecutive values; BOOTTIME includes suspend; failures exist. No multi-clock atomic call, event history or zero-error theorem. |
| decompressed `proc_pid_stat.5.gz`:227–233 | Field22 is time after system boot in clock ticks, divide by SC_CLK_TCK. Does NOT specify the exact host conversion rounding/time-namespace implementation needed for a tight lower start bound. No actual proc file read. |
| `/usr/include/time.h`:293–298 | C declaration returns one chosen clock value. Does not state atomic cross-clock semantics. |
| `/usr/include/python3.14/cpython/pytime.h`:10–21 | int64 PyTime_t and separate monotonic/time APIs. Declaration presence is not proof of executing interpreter/kernel or raw sample accuracy. |
| Python v3.14.0 `Doc/library/time.rst`:170–190,227–250,287–322 | `_ns` avoids float loss; monotonic is shared across processes under documented platform semantics; underlying clock implementation varies. clock_info/resolution not a simultaneity, bounded-call-latency or absolute-error guarantee. No clock_info call made. |
| same doc694–736,864–909,990–997 | Wall time may decrease; BOOTTIME includes suspend; MONOTONIC_RAW is a different rate domain, not a substitute for the recorder's timer. Nanosecond units do not guarantee nanosecond measurement accuracy. |

Python document URL:
`https://raw.githubusercontent.com/python/cpython/v3.14.0/Doc/library/time.rst`,
SHA256 `949d9e5b2fef70559990e41a9a1609ebc8ca21d3aaf8c83b37058de175a76813`.
This is a pinned3.14 API-family document, **not** the installed interpreter3.14.8
source/build closure or identification of the host kernel. Target kernel6.18.55
cannot supply host timekeeping/PID conversion semantics. No host uname, proc,
clock resolution, namespace, service or suspend configuration was queried.

## Pin and evidence discipline

24 public inputs:23 local regular files plus the explicit Python documentation
URL. First snapshot followed source reading, before authoring this packet. No new
clock read/time stamp; order is BEFORE/AFTER, task date is supplied. Aggregate is
sorted local path UTF-8+NUL+raw SHA256 digest, followed by the URL in the same form.
Frozen prior manifests are not rewritten; context drift is recorded separately.
No claim of transitive build closure or complete requalification of older seals.

No source implementation or documentation example was executed. Source SPDX and
all Python/Linux/manpage notices remain untouched; this packet paraphrases APIs
and cites short identifiers, not redistributed full source/docs or a new global
license grant. No private paths/digests, event records or raw samples are exported.
