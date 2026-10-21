# 03 — Platform drivers, reproducible builds, recovery and runtime supervision

Entry: [index](00-INDEX.md). Prefix: **PLAT**. Depends: G0; coordinates CORE.

## Tasks

- [ ] **PLAT-01 — Reproducible build baseline.** Keep Linux6.18.55 pin initially;
  record compiler/linker/libc/config/DT/initramfs/module/source hashes and exact
  commands. Split active overlays from archived evidence; make a clean build
  reproduce the checked artifacts. Audit build scripts and retain licenses and
  local patches. Source upgrades require separate regression, not incidental
  rebasing while debugging imaging.
- [ ] **PLAT-02 — Eliminate stale embedded modules.** Generate one release
  manifest for kernel/config/symvers/initramfs/modules/firmware. Build radio and
  codec against that exact output; refuse inconsistent packaging/loading. Remove
  the active-image stale `wifi attach` path only through a deliberate source
  change with rollback; historical initramfs and proofs remain preserved.
- [ ] **PLAT-03 — Production UART path.** Use applicable Anyka GPL source and
  upstream serial-core patterns for RX/TX, interrupt ownership, buffering,
  termios, error reporting and close/hangup behavior. Retain kernel69 pacing
  workaround and error counters. Verify error-bit semantics before writes;
  bounded overrun/error tests must not expose secret payloads or erase evidence.
  Never spin indefinitely waiting on read-only MEM_RDY. Qualify RX while codec,
  USB and logging are loaded; monitor dropped bytes, not just prompt presence.
- [ ] **PLAT-04 — Remaining base-driver audit.** Clock/reset/IRQ/timer/watchdog,
  pinctrl/GPIO/I2C, USB and future MMC/audio resources must have DT-described
  ownership and managed probe/remove failure paths. Preserve independently used
  clock bits. Replace arbitrary diagnostic register interfaces with constrained
  standard interfaces where possible; retain explicit lab diagnostics separately.
- [ ] **PLAT-05 — Userspace boot/runtime.** Replace the qualification monitor as
  product PID1 with a small supervised service layout. Verify required syscall,
  script/interpreter and BusyBox support; current ELF DHCP bridge is a proved
  workaround, not a reason to assume shell scripts execute. Preserve serial
  rescue mode, read-only base filesystem, writable runtime dirs and deterministic
  startup dependencies. Do not put WLAN credentials in images.
- [ ] **PLAT-06 — Watchdog and fault policy.** One supervisor owns watchdog
  servicing based on meaningful progress, not an unconditional timer. Define
  service restart budgets, backoff, source recovery, motor-safe state and a
  bounded reboot-loop escape to recovery/setup. Driver errors must not free DMA
  that cannot be proven retired. Keep startup and runtime watchdog roles distinct.
- [ ] **PLAT-07 — Memory/log/service budget.** Measure boot/free/CMA high-water,
  heap/stack/thread/fd/socket limits and tmpfs/log growth. Fixed-size rotating
  diagnostics, health summaries and controlled crash evidence must fit with live
  streaming/UI/recording. Avoid retry loops that allocate indefinitely or create
  unbounded core/frame dumps. Measure logging's IRQ/latency cost.
- [ ] **PLAT-08 — Recovery and image layout.** Validate RAM staging/decompression/
  CMA ranges from measured40-MiB RAM, not U-Boot's64-MiB string. Maintain checked
  UART loader, stock boot path and exact rollback images. Produce a release-size
  map including8-MiB flash constraints, existing partitions and configuration
  storage choices. If it cannot fit, report the measured constraint and propose
  a reviewed boot/storage solution; never truncate/pad/repartition silently.
- [ ] **PLAT-09 — Update/install mechanism design.** Define trusted image
  manifests, compatibility checks, update keys if updates are enabled, preflight,
  power-loss handling and rollback. Measure whether true A/B fits before claiming
  it. Do not implement an unattended SPI installer as a shortcut to bring-up.
  Persistent installation requires exact ranges and current authorization.

## Tests / gate contribution

Cold RAM boots and software recovery must reproduce; intentional userspace
crash, failed module load, allocation failure and UART disruption leave an
observable recovery route. Verify timer32 compatibility and time arithmetic on
real ARM, not only a64-bit host. Built versus target-booted versus installed are
separate statuses. Platform assumptions are packaged as QC07F facts so later
ports replace them without rewriting common services.
