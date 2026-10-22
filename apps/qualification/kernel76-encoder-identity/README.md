# Observational per-open encoder identity query — SOURCE/COMPILE candidate

New encoder-local `QC_HWENC_OPEN_QUERY`, output-only version1/88 bytes. It reports
the actual open description's existing boot-lifetime numeric session ID, actual
reference-owner numeric session and NONE/THIS_OPEN/OTHER_OPEN relation, existing
observational reference generation/frame number and serialized job ledger/history.
No addresses, payload/device labels, key material, grants or authorization flag.
Queries do not allocate, start capture, reset, clear refs/quarantine, change owners
or counters, or invoke hardware. The only output is the fixed userspace copy.

`encoder-open-query.inc` is included into the real retained provider, not a mocked
query implementation. The fresh exact source overlay is generated in
`docs/qualification-plans/encoder-identity-20261022/runs/<fresh>/candidate/`;
`original/` and `applied-source.patch` retain exact provenance and changes. Original
bringup/mainline/full76 source, config, object, image, old RTSP/provider candidates
and helpers are read-only. No automatic application to a kernel tree is provided.

Per-open session IDs already refuse new opens at u64 exhaustion (no wrap/reuse).
Existing telemetry generations/operation counters saturate, expose their existing
flags and are NOT leases. Codec frame number wraps16 bits. A query takes the same
mutex as jobs/open/final release, so it waits behind synchronous work; job history
is not a current executing-job oracle, readiness or DMA retirement. dup/fork shares
the same identity, not independent ownership. Actual closed-file/history behavior,
lock/init lifetime and future paired build/native requirements are in the report.

Old ioctl definitions and handlers are byte-preserved apart from the additive
include/dispatch. New command is `_IOR('Q',0x66,struct qc_hwenc_open_query)`; the
frozen kernel76 does NOT support it and returns ENOTTY. No old-kernel fallback to
global lifetime/session_sequence or fabricated supported flag is implemented.

## Compile only

```sh
python3 -B apps/qualification/kernel76-encoder-identity/build.py fresh-name --toolchain /path/to/retained/armv5-eabi--musl--stable-2025.08-1
```

UAPI declarations compile HOST GCC/retained ARM GCC. The actual whole encoder
translation unit compiles ARM Clang using retained final76 config/generated
headers/Kbuild flags, rebased to fresh output paths, then links ELF REL only.
No make/full kernel/module/image build, executable/main, runtime/synthetic/adversarial
test, listener, installation or native query. HOST kernel code is NOT simulated:
its original ARM/static/built-in configuration guards remain unchanged.

GPL-2.0-only for new code; original GPL/UAPI/kernel license notices preserved.
No memory/guard/policy limit, child image pin, runtime dependency or native provider
adoption follows.40MiB/CMA/128KiB and native2MiB/HOST4MiB, failed-next/backing/
watchdog/provisioning/listener/radio HOLD and G2/G3/G8 remain unchanged.
