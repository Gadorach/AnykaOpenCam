# QC07F PLAT-04 / STORE-01 / STORE-02 source audit

2026-10-12. **Offline source candidate, not an operational Linux MMC driver.**
Only `bringup/qc07f/mmc-candidate/` and
`evidence/delegated/20261012-mmc-candidate/` were written. Parent review pending.
No target observation, UART, network, card, block, mount, register, GPIO, driver
load, power or persistent write was performed by this lane.

## Authorities and exact source boundaries

1. Parent-supplied current blocker: cached read-only `/sys/block` was empty;
   running IKCONFIG byte-equal to config75
   `22acc5e770af2d6d4804ca8f64cd7e9ffe71f69f2e5c8a0fa455a23dc9217bf8` has MMC
   and EXT4 disabled. This lane independently checked that **source/build config**,
   not live IKCONFIG or current runtime. No inferred card absence or card fault.
2. Parent-supplied measured camera evidence: U-Boot read of QCLDR, 1872 bytes,
   native CRC `dd0572e1`; masked volatile SDMUX update at `0x080000dc`,
   mask `0x3fc0`, value `0x15c0`, preserving other fields. This proves that
   historical U-Boot path, not Linux MMC, CID/UUID, filesystem or writes.
3. Public GPL Anyka sources read only under
   `work/lightbulb-camera/deep-bringup-20261008T055753Z/sources/ak3918ev200/vendor/linux-3.4.35/`.
   Container checkout pin `5fafbded3f4fa2552890d78674d88aacd5cac86a` does not by
   itself prove the vendor subtree pristine; every used source has an input hash
   and unchanged original copy in the build's `upstream/` directory.
4. Retail REA contracts read only under
   `work/lightbulb-camera/driver-re-20261008T110053Z/contracts/pseudocode/`.
   These are provider-produced pseudocode, **not licensed implementation source**.
   Their stock module input identity is
   `625004c2de9f81a7f3967393a5601960aaea3eafc4c78b2034085f4760032c3d`.
   No module/ROM bytes were read or executed here; only existing public contracts.
5. Independent full Linux source input:
   `work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-security75-source01/linux/`.
   Git base `725bd2f3c81d54edccb66fead01d4c0c222e2231`, Linux 6.18.55, with the
   existing public QC07F port. Existing source, config75, builds and helpers were
   not modified. New copies are byte-inventoried; internal upstream symlinks were
   inspected and preserved, external/absolute links rejected.

Exact hashes/commands/versions are in the final build's `REPORT.json`, copied
source inventory, Kbuild `.cmd` files and final `seal-*/REPORT.json` (README).

## Controller identity and resources

| Resource | Source/contract fact | Retail qualification / candidate action |
|---|---|---|
| Controller | Anyka proprietary MCI, **not SDHCI**. `devices.c:40–59`: physical `0x20100000`, 0x44 bytes. Separate SDIO `0x20108000`. | SDIO controller is not substituted. Disabled review node describes only MCI. No ioremap or resource claim. |
| Main IRQ | `irqs.h:20`: MCI hwirq4; L2 main hwirq12 at line28. Linux75 `irq.c` has one-cell domains; Linux virtual IRQ is not hwirq4. | IRQ domain binding and L2 callback ownership missing. No IRQ requested/unmasked. GPIO/L2 subinterrupt support is not supplied by current port. |
| Clock | `clock.h:20,41`: shared SYSCTRL gate `0x0800001c`, MCI bit1; L2 bit9. `clock.c:555–559`: MCI parent ASIC. | Actual retained parent rate and shared masked clock provider missing. Never replace gate register or change PLL/CPU/video clocks. |
| Reset | `reset.c:45–51`: module bit1, read-modify-write of shared reset control, old code delays 5ms. | No reset provider/retirement guarantee ported. Never global-reset L2/UART/video or pulse candidate reset. |
| Pins | `ak39-gpio.c:36`: eval MCI mask/value `0x3c0`; 8-line variant line37 is different again. | Those defaults do **not** equal measured retail mask `0x3fc0` / value `0x15c0`; don't apply them. No pinctrl or mux activation exists. |
| GPIO47 power | Retail stock probe REA includes board-version branches toggling 42 or 47, with sleeps. | **Candidate ONLY**, not measured QC07F power routing/polarity. No `power-gpios`, output configuration or toggling. Eval GPIO29 card detect is also not adopted. |
| Voltage / WP / CD | GPL host OCR defaults 3.2–3.4V. Eval board specifies active-low GPIO29 CD and missing WP. Retail `akmci_get_ro` contract returns `-ENOSYS`. | Actual supply voltage, switched/fixed-power policy, WP/CD wiring/polarity and removal behavior unknown. `get_ro=1` is a software policy, not a physical WP observation. |
| L2 control / SRAM | Machine `map.h` maps L2 control `0x20140000` and SRAM `0x4800c000`. Unused generic `akmci.h` literals `0x2002c000` / `0x48000000` disagree. | Use neither generic literal nor a hard-coded buffer. Need variant-correct channel/SRAM/assignment evidence and arbitration first. |
| L2 channel | `plat/l2.h:125–135`: ADDR_MMC_SD enum4, dynamically assigned common buffers0–7, 512 bytes each. Fraction DMA is a **shared engine**, bit9. | Enum4 is NOT physical buffer4. No allocators/shared-fraction arbitration provider or retirement proof available on Linux75. |
| UART collision | Retained Linux75 UART uses `0x20140000` L2 registers; RMW at `+0x8c` bit16/17, `+0x84` bit29 and SRAM `0x4800d000`. | A private MMC lock cannot serialize these existing UART updates. Any L2-wide reset/clear or uncoordinated RMW can break recovery/UART. Must preserve current owner and independently used bits. |
| USB/video | Retained USB source explicitly uses PIO, not L2 DMA. Video/codec ownership and CRC guards stay unchanged. | USB PIO doesn't prove all global L2/DMA aliases idle. No source/memory/IRQ/video policy changes were made. Combined media/WLAN/storage load remains unknown. |

No unknown speaker, PTZ, ALS, motor, sensor or audio feature is inferred.

V2 software-only correction: generation namespaces are nonwrapping. Idle
`UINT32_MAX` exhaustion refuses before claim/MMIO or Linux work scheduling;
the last valid operation at `UINT32_MAX` still receives its own events and
normal retirement. Active/poisoned state retains precedence. Null IRQ helper
arguments and stale generation1 events fail closed. No resource fact, register
constant, provider, host probe, DT/config or native readiness changed. Exact
v1→v2 diff and focused tests/build closure are linked in `HANDOFF-v2.json` in
`evidence/delegated/20261012-mmc-candidate/v2-20261012T180922.619999Z/`.

## Register protocol evidence (no register access performed)

Original GPL `akmci.h:13–105` and retail command/data contracts agree on:

- CLOCK `+0x04`, ARG `+0x08`, CMD `+0x0c`, responses `+0x14..0x20`.
- CMD: enable bit0, opcode bits1–6, response bit7, long response bit8,
  with-data bit11. No SDHCI command/descriptor layout is used.
- TIMER `+0x24`, LENGTH `+0x28`, DATACTRL `+0x2c`, remaining count `+0x30`.
  DATACTRL: enable0, read-direction1, bus mode3–4, literal block length16–27.
- STATUS `+0x34`, MASK `+0x38`, DMACTRL `+0x3c`, FIFO `+0x40`.
  Completion/error masks include command CRC/timeout/response/end, data
  CRC/timeout/end/start-bit. Candidate doesn't ignore start-bit failures.
- GPL/REA IRQ handlers **read STATUS**, without a W1C status write. That does
  not prove every bit's read-clear/level semantics. An owned IRQ snapshot and
  drain protocol must be source/board verified. The provider must not invent
  W1C acknowledgement, and a parent passive observer must avoid this register.
- `TRANS_DATA_TIMEOUT=0x800000` is a source timer value, not a portable 419ms
  wall-clock guarantee. New core additionally bounds polling by wrapping
  monotonic 500ms deadline **and 4096 samples** if the clock stops.
- Clock source computes f_min/f_max using `(divL+1)+(divH+1)`, but its cached
  bus_clock uses a product. Candidate uses the sum, rounded down to requested
  <=400kHz. **Hardware divisor/frequency still unqualified**, not activated.

## Why copying the old driver is unsafe

1. `akmci_start_data` maps SG without checking `dma_map_sg`'s returned count;
   later walks original SG using `sg_dma_address`/`sg_virt`. A map/symbol/vermagic
   match would not make these ARM926 cache or retirement assumptions valid.
2. `l2_combuf_cpu` contains unbounded `while(l2_get_status(...))` waits.
   `l2_wait_dma_finish` uses CPU-loop iteration limits (50 million), may clear
   state/addresses on timeout, and doesn't establish no future DMA writes.
3. Old stop unmaps caller SG after controller disable; request end frees L2
   buffer. Controller-stop alone is not proof that L2/fraction DMA/callbacks
   are retired. The failure state must retain/quarantine memory and ownership.
4. Shared pin and CPU-frequency locks, board-specific power pulses, obsolete
   MMC `.enable/.disable` callbacks, timers and notifier lifetime can't be
   transplanted blindly into Linux6.18.55.
5. GPL and retail REA differ in SG batching, DMA alignment, transfer scheduling,
   IRQ masks, probe power fields and callback details. GPL code is licensed
   guidance; matching target contracts are comparison evidence, not an excuse
   to treat the eval platform as measured retail wiring.

## What the candidate actually implements

`host-core.c/h` implements a **provider-dependent** command/data controller core:
SD-only initialization/query/single-block reads, one 512-byte maximum descriptor,
one-bit bus, no writes/erase/lock/vendor/SDIO/eMMC commands, no forced profile,
no CMD18/multiblock/SBC/stop request. CMD55 context is consumed once and requires
APP_CMD with no card-error status. CMD6 allows exactly the Linux6 mode0 support
query `0x00fffff0`, not mode1 switching. ACMD6 permits one-bit only; ACMD41 rejects
S18R/1.8V switching. Bounds precede multiplication in Linux glue.

The transaction has claim/start/owned-event accumulation, command/data/CRC/
timeout/busy/cancel paths, stale generation rejection, mask/stop, mandatory
retirement proof, then cache-synchronized publish/release. Failure to retire
poisons the engine and **never copies or releases the provider lease/mapping**;
subsequent requests refuse. A private provider bounce prevents timeout DMA from
writing a returned caller SG. No whole-AU buffer or memory ceiling is increased.

Synthetic callbacks test policy/lifetime; they **do not implement hardware DMA**.
Required real provider: bounded `DMA_FROM_DEVICE` mapping/error checks, ARM926
cache synchronization, per-channel L2 and fraction engine arbitration, IRQ
generation/drain fences, DAT0 busy/CD observation and proved bounded retirement.
All callbacks require bounded execution; the core cannot rescue a provider
that violates this contract. 500ms core policy isn't native latency proof.

`qc07f-mci.c` type-checks dormant Linux6 MMC request/work glue, retains it in the
ARM object, and returns `-EOPNOTSUPP` unconditionally from probe. It does **not**
register `mmc_host`, start discovery, request resources, or provide `set_ios`,
CD, power, IRQ/DMA/retirement, ordered-queue teardown or quarantined removal.
These are **unfinished native implementation**, not silently working callbacks.
The disabled DT has no pinctrl, clock, GPIO, regulator or reset defaults.
Changing a DT status/boolean cannot enable the core. Do not add a default
pinctrl state: Linux driver core can apply that before a refusing probe.

No native Linux MMC block path, physical features, G5/G8 or either later port is
accepted. Full kernel/runtime linkage, footprint and combined-load qualification
remain open. Config75, 40MiB/CMA16, built-in owned codec, CRC guards, CPU/L2,
root/debug and rollback/recovery were not changed.

## Missing facts and one parent native step

Missing physical facts: fitted CD/WP/power routing and polarity; measured SD
supply/IO levels; which mux fields/lines are actually used; whether power can
remain on across the intended Linux boot; safe removal/power-off behavior.
GPIO47 is not a substitute for these measurements.

Missing kernel facts: clock/reset/pinctrl providers with shared masked ownership;
MCI IRQ translation/status acknowledgement; existing UART/L2 arbitration;
MMC SRAM/channel/fraction-DMA ownership; ARM926 cache/DMA addressing/retirement;
managed probe failure/removal and quarantine lifetime. A native backend and
usable Linux MMC host are blocked on these, not just Kconfig.

**One next parent read-only native qualification step:** after fresh sole UART
ownership, exact runtime/image and bounded rollback/recovery validation, obtain
one source-reviewed passive **retail resource receipt** of SDMUX/shared clock
gate, already-defined GPIO direction/input levels and existing UART/video/L2
owners. Exclude controller STATUS/FIFO and any read-clear register; no clock,
reset, power/mux write, driver bind, discovery/CMD, block open, mount or card IO.
Compare only with the measured retail mux and pinned sources above. This can
qualify a resource observation, **not** SD reads or GPIO47 voltage/polarity.
Passive electrical routing/voltage measurement and provider implementation may
still block subsequent host review. Parent alone selects/seals a later new RAM
image and first bounded card metadata reads once those blockers are resolved.
