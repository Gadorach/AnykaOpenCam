# QC07F GPIO0 passive button observer

Fixed input-only qualification utility, not a reset daemon or generic GPIO tool.
See `docs/qualification-plans/stock-reference-20261013/reset-button01/ACTUAL_RESULT01.md`.

`--status` validates stock3.4.35/root, fixed nodes `/dev/mem`1:1 and original misc
`/dev/akgpio`10:61, maps only two fixed pages read-only, requires GPIO0 already
GPIO/input, and compares pure GET with input MMIO. `--sample-ms 1000..60000` samples
only bit0 with config guards, finite sample/deadline/edge budgets and read-only
cleanup. No SET/mux/pull/direction/output/IRQ/reset/erase paths exist.

GET is literal`0xc001662b` with12-byte wire/value offset6, despite its encoded size1.
Every call initializes an ff value sentinel and all other fields/canaries. Source
and native node/kernel/ownership admission are mandatory; O_RDONLY is not a driver
sandbox. Physical pressing may have independent electrical effects beyond this
software. No button press is automatically requested by running the binary.

`build.py buildNN` uses fresh output, optimized/sanitized pure fixtures and fully
mocked native control flow, then static ARMv5 compilation. ARM firmware is never
host-executed. Build01 is retained/unadmitted; use the reviewed corrected build02.
`collect_press.py` is a dated parent harness with pinned SSH, native READY+heartbeat
barrier, regular private diagnostics, no fallback, and uncertain-owner lease hold.
It is not a reusable authorization or permission to kill an unknown remote helper.

Observed physical GPIO0 active-low taps do not establish factory-reset behavior,
debounce completeness, startup-held handling or a hardware-reset recovery path.
