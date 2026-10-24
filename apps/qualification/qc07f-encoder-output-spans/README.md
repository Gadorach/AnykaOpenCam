# QC07F encoded-output spans — SOURCE candidate

B07 / RP-04/05/09; CAP-01/05/06/07 + ENC-03/05 dependency only.
Contract: `docs/qualification-plans/qc07f-encoder-output-spans-20261024/CONTRACT.md`.

Actual driver derivative, not an application, standalone scatter mock or diagnostic
query. New exact `QC_HWENC_H264_STREAM_SPANS` (`0xc0805167`) feeds the same H.264
hardware body, then CPU-copies the retired valid output into eight131072-byte user
ranges. Old contiguous commands and query0x66 remain supported by this source.
No source here is adopted, staged, installed or run. Independent review/parent
disposition is required before use. Old kernels return ENOTTY, no fallback.

The two derivative driver files replace the corresponding baseline source files;
new `encoder-output-spans.h` / `.inc` accompany them. Five unchanged support files
come from the sealed query baseline, snapshotted by the fresh recipe. Do not link
two providers or compile this source against invented headers/stubs.

Only selected offline command (fresh output required):

```text
python3 -B apps/qualification/qc07f-encoder-output-spans/build.py build01
```

This new fixed recipe reads, never imports/invokes, old recipe/evidence inputs.
It compiles declaration-only ABI objects with HOST/retained musl ARM and the WHOLE
actual driver with explicit NEW Clang23.1.3 and retained genuine Kbuild flags/
headers/config, without make, configuration/header generation, linking or image
creation. It records dependency preprocessing/compile pins and ELF metadata; no
output/candidate/test/ioctl is executed. Failure stops and preserves its output.
Clang23.1.3 is NOT the retained generated23.1.1 compiler/config or native ABI pair.
No complete compiler/toolchain closure claim. No automatic next build/run queue.

On any error, all partial output/metadata is unusable; a failed actual encode loses
its logical P-reference ownership. Discard output; do not repair/retry and do not
equate EFAULT or ownerNULL with DMA retirement. A future admitted job needs IDR.
Preflight errors preserve existing references. Stable ordinary userspace RAM and
mapping/alias serialization remain caller obligations: access_ok is not a pin or
alias/provenance proof. No physical/DMA/user-MMIO import interface.

Unchanged RTSP native producer still requires contiguous1MiB output; no producer/
hub/provider/controller adoption here. Existing2MiB guarded DMA allocations, raw/
process/CMA/combined-load/native-fit/overall128KiB cap remain unqualified. Query78
is PARKED. No hardware, test, fuzz/mutation/adversarial, Git or central-plan work.
