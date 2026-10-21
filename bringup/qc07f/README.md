# QC07F initial upstream-Linux bring-up work

This directory contains **experimental, unboot-tested** source for an initial UART/timer/IRQ milestone. It is not an installable or functional camera firmware. Stock backup paths and all private extracted evidence remain under `work/lightbulb-camera/`.

The selected baseline is **Linux 6.18.55 LTS**. The overlay retains ARM926/ARMv5 and adds a small DT machine, primary/system interrupt domains, timer1 clockevents, timer2 clocksource, bounded early UART output and a deliberately limited polling `ttyAK0`. Register protocols derive from the pinned GPL Anyka 3.4.35 `mach-ak39` / `ak39_uart.c` sources. They are family-source-supported assumptions, not measured QC07F clock/RAM/mux state.

`initramfs/init.c` is a tiny freestanding ARM EABI PID 1 monitor, built without firmware libraries. It can read CPU, memory, uptime, interrupt and mapping information; it cannot flash, drive GPIO/PTZ, initialize a sensor, connect to cloud services or stream video.

## Reproduction

Use a **fresh** session directory with `logs/`, `analysis/`, `build/`, `artifacts/`, `sources/linux-lts/` and the session's `run_logged.py`. Check out the pinned stable kernel revision recorded in the work session. Then, from the project root:

```sh
python3 bringup/qc07f/build_prototype.py --session work/lightbulb-camera/NEW_SESSION
```

The recipe applies only this overlay to its dedicated source checkout, selects ARMv5 explicitly, uses Clang/LLD, builds a deterministic newc initramfs and DTB, and retains commands/config/artifact hashes. `--resume` is only for the known initial failed setup in this session; it is not a general permission to overwrite unrelated work.

## Deliberate limits

- No target boot or usable UART result is claimed from compilation.
- The candidate DT declares 64 MiB from stock boot arguments; actual available RAM, media reservations and safe load/decompression regions require a stock boot log/`bdinfo`/`/proc/iomem` before use.
- UART retains the bootloader clock/divisor/mux. Its polling implementation is not a production serial driver and requires real RX/TX testing.
- The legacy board's watchdog, clock/reset tree, pinctrl, GPIO, I2C, SD/SPI, USB host, ISP, sensor, encoder and PTZ drivers still need qualification/porting.
- Timer2 is repurposed as a clocksource only in this peripherals-disabled prototype; later motor/other timer allocations must avoid conflicts.
- Do not flash these artifacts or use the stock kernel load address as an assumed safe address. No boot/flash command is supplied before actual bootloader/RAM evidence is available.

## Imaging and PTZ findings driving the next work

The recovered configuration selects sensor ID **`0x1346`**, firmware model **`C2L-Y-AB`**, version **`3.8.54.0`**. Fitted sensor identity remains runtime-unconfirmed. Stock media binaries are ARMv5TEJ/EABI5 soft-float, dynamically using `/lib/ld-uClibc.so.0`; the kernel modules target 3.4.35 and cannot be assumed compatible with 6.18.

REA confirms that the retail motor module uses `/dev/motor0` / `/dev/motor1`, an AW9523 route for board-version 3, hardware timers and a vendor ioctl table that differs from the public BSP's older `/dev/ak-motorN` interface. Mainline 6.18 has an AW9523 pinctrl/GPIO driver, but the board's I2C address, actual expander output mapping and mechanical calibration must be corroborated before enabling coils. Stock saved limits (352°/120°, 2000/700 steps) are configuration values, not measured safe limits.

## Required-driver REA evidence

See [`DRIVER_RE_REPORT.md`](../../work/lightbulb-camera/driver-re-20261008T110053Z/DRIVER_RE_REPORT.md): 2,263 unique procedures, 24 driver/dependency groups, physically identified TXW801-100 USB soft-MAC candidate path, retained radio firmware/CRC, two identical139-register SC1346 initialization tables, compiled motor branch candidates and retail ION/UIO/ISP ABI notes. Reference-only headers are under `stock_abi/`; no new device driver or boot-tested image is supplied.

Retail kernel REA uses timer5 as its12-MHz clocksource; this prototype intentionally uses timer2. Reconcile that choice with measured clocks and future timer users rather than silently transferring stock allocations. Compiled phase pins and AW9523 address are now recovered as **branch candidates**, not confirmed retail wiring.

## Open-source port increment after live access

The retail-register `gpio.c`/generated `gpio-data.h` port implements79 GPIO lines, direction/value and active-low bias controls with locking and GPIO49 input-only handling. It writes no output/mux at probe and rejects GPIO requests when their shared selector is still owned by an alternate function. GPIO IRQ/wakeup/full pinctrl integration remains missing; the DTS node stays **disabled**. A fresh pinned6.18.55 build plus six register-contract fixtures passed; this is not target GPIO qualification.

Running stock evidence now confirms40MiB kernel System RAM at0x81800000..0x84000000,24MiB media reserve at0x80000000, board5/sensor0x1346 and TXW801 USBa012:8400. UART SRAM0x4800d000 is corroborated by the retail kernel port table (U-Boot uses another SRAM view). Do not change the finalized attempt4 or assume the new build's full-DRAM layout is already safe to boot. See `work/lightbulb-camera/opensource-port-20261008T185631Z/` for new sources/build/evidence.
