# 07 — USB host, radio transport and networking capability

Entry: [index](00-INDEX.md). Prefix: **NET**. Depends: BASE/PLAT; feeds G2 and
SETUP. Hardware work here is QC07F only until the deferred ports.

## Baseline

**Oct20 increment:** NET-02 source/build candidate76 implements Anyka PIO-only
low-byte TXCSR kick at continuation and NAK retry. Independent source and ARM
object review,262144 optimized+262144 sanitized helper cases and3 runtime mutants
passed; final paired RAM image built/verified. This is **not native full-size USB
or MTU1500 acceptance**, and these tasks remain unchecked. Keep MTU384 rollback.
See [candidate76 results](../../qualification-plans/kernel76-integration-20261020/RESULTS.md)
and [task ledger](KERNEL76_TASKS_20261020.md); target is OFF, no test queued.

Measured USBa012:8400, retained firmware RAM/RUN, station70 mac80211 soft-MAC,
WPA2/CCMP and DHCP work. Earlier TCP goodput~7.22/6.44Mb/s is finite evidence.
MTU384 avoids unqualified native2048-payload/2052-wire USB failures, including
hostTX4 FIFO flush errors. Slow RTSP sends show TCP RTO2.17–3.37s; RF/USB/CPU
cause is not yet isolated. Current radio advertises station-only support.

## Tasks

- [ ] **NET-01 — Layered diagnostic baseline.** Preserve current known-good
  MTU384 profile. Measure USB transactions, HCD/FIFO/DATA-toggle state,
  short/zero-length packets, endpoint sizes, radio queue/credits, MAC retries,
  RF/link statistics, TCP_INFO and host observations with common timestamps.
  Distinguish a peer not reading from packet loss/retransmission or local IRQ
  starvation. Do not classify every application cutoff as radio corruption.
- [ ] **NET-02 — Full-size MUSB PIO qualification.** Audit upstream MUSB indexed
  FIFO and applicable Anyka glue against measured controller behavior. Test
  transfers around64/512/maxpacket boundaries and multi-packet lengths, exact
  payload framing, buffer capacity, flush/NAK/timeout and endpoint reset. Apply
  the smallest verified fix; no guessed clock/GPIO/reset values or hidden forced
  transfer profiles. Preserve each failed large-transfer attempt.
- [ ] **NET-03 — Radio driver hardening.** Correct control/data queue ownership,
  firmware credits, completion/error paths, disconnect/reconnect, RX bounds and
  aggregation only where supported. Keep RAM firmware verification and command
  sequencing. No OTA/flash operations as part of driver load. Normal module
  unload/reload must retire URBs and callbacks without stale DMA or leaks.
- [ ] **NET-04 — MTU restoration.** Incrementally qualify normal Ethernet MTU1500,
  TCP both directions, DHCP, DNS, multicast, long transfers and real streaming.
  Test packet sizes independently from encoded AU sizes. Remove the workaround
  from the default only after actual goodput/reliability and combined workload
  pass; retain a named recovery profile. Do not invent jumbo-frame requirements.
- [ ] **NET-05 — Latency/retransmission policy.** Correlate stalls with source
  processing, TX queue occupancy, TCP RTO and loss. Test bounded socket budgets,
  fair scheduling, packet coalescing and link power policy based on measurements.
  Do not simply increase every timeout to hide problems or globally tune TCP
  without evaluating all services. A dead consumer must still retire boundedly.
- [ ] **NET-06 — Capability truthfulness and AP feasibility.** Audit radio
  firmware/driver support for AP, beaconing, keys, station tables, nl80211,
  channel selection/regulatory data, scanning and concurrent AP+STA. Read-only
  feasibility research happens early so SETUP is not built on false assumptions.
  Implement and qualify required AP support in SETUP; do not advertise it from a
  family feature list. If infeasible, report the blocker explicitly; station-only
  does not satisfy the user's setup requirement.
- [ ] **NET-07 — Robust network lifecycle.** Association failure, AP unavailable,
  WPA rekey, weak/intermittent link, DHCP renewal/address change, USB disconnect,
  suspend if supported and radio reload produce observable state transitions.
  Coordinate with SETUP transactions and discovery reannouncement. Preserve
  credentials privately and avoid disruptive scans during unrelated tests.
- [ ] **NET-08 — Acceptance matrix.** Real bidirectional data integrity, control
  latency while streaming/recording, repeated link recovery and normal MTU at the
  intended bitrate/concurrency. Monitor CPU, DMA, retry/drop counters and heap
  growth. Repeat with UART enabled to catch console/IRQ interactions.

## Gate / output

G2 transport contribution is a stable measured link for the accepted stream
workload, with full-size USB/MTU1500 qualified or an explicitly reviewed product
constraint. Deliver capability matrix, transport traces, exact firmware/driver
pins and an evidence-supported cause/fix for persistent stalls. NET-06 must have
a concrete AP feasibility result before SETUP begins hardware implementation.
