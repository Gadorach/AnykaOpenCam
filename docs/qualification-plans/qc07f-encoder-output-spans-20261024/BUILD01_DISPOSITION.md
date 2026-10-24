# build01 — FAILED_PRESERVED

2026-10-24 10:02:05UTC. Four successful compiler/version/dependency commands;
one failed HOST declaration compile. ARM UAPI/whole-driver object were not reached.
No object/candidate/test/function/ioctl execution. No retry into this directory.

Author incorrectly counted five instead of four u64 allocation counters in the
unchanged `qc_stage_query` declaration. It contains four counters at offsets216,
224,232,240; four live/request/freed counters at248..272, retire_failures280 and
owner_generation288. Its exact size is296 (`0x128`), so the inherited stage ioctl
is `0xc1285165`, not the author's asserted304/`0xc1305165`.

The compiler rejected precisely those two incorrect assertions. All new spans
size/alignment/offset/full-command assertions and other inherited ABI assertions
produced no diagnostic. That is not a successful object compile or runtime test.

Correct only the two assertion constants in the authored declarations and select
one explicit fresh `build02`. Do not alter the driver, UAPI, baseline header,
compiler/flags/config guards or this failed run's input copies/logs/report. Exact
failed argv, source/input/log pins and before/after custody remain in
`runs/build01/REPORT.json`. This is an author declaration mistake, not an image,
kernel ABI, hardware or negative-test observation.
