# QC07F first-frame DMA contract

This is a **bounded diagnostic**, not yet a V4L2 streaming driver or proven image capture. `isp.c` is new Linux6 code; it never calls stock firmware. Numeric register/calibration facts come from the owned retail device and REA evidence.

## Inputs and provenance

- Linux6.18.55 pinned at `725bd2f3c81d54edccb66fead01d4c0c222e2231`.
- Stock kernel analysis ELF SHA256 `790646be95cfde5996d6c32ec4ad0b959e1503c212db415c93d8a383cbadf800`.
- Live11808-byte table snapshot SHA256 `7d5ef72eb46a0c4b318c778e4ee0ce34f3345d5e6b17b30d4b0d0700cfdb0e40`, acquired from stock System RAM83b9c000..83b9ee20, hash-matched over UART. It is not atomic across frame updates.
- REA results are retained in `work/lightbulb-camera/implementation-20261008T205404Z/analysis/capture-*.json` and `raw-capture-*.json`. Original pseudocode remains evidence, not compiled source.

## Register-table layout

REA `isp2_module_init` describes register banks at table offsets0,ec,1bc,3bc,5bc,7bc,868,aa8,ba8,c50,cdc. The register configuration occupiesd60 bytes. SDPC/stat areas atd60/1d60 are separate, within the allocated table.

- Buffer0 Y/U/V + sub Y/U/V: reg10 offsets64..78 => tabled40..d54; stat0 =>d58.
- Buffers1..3: reg0 main Y7c/80/84, U88/8c/90, V94/98/9c; sub Ya0/a4/a8, Uac/b0/b4, Vb8/bc/c0; stats c4/c8/cc.
- 3DNR reference address fields d4..e8 carry30-bit DDR offsets.
- OSD addresses: reg9+60/6c =>cb0/cbc. Its useok bits22/23 in reg10+80 =>d5c are disabled.
- Frame-control mask: reg9+88 =>cd8, disabled in the generated template and explicitly enabled only for capture.

`generate_isp_reference.py` strips **all those DMA fields** before generating the header. Runtime replaces them with coherent allocator handles, never stock addresses. The driver refuses DMA handles outside the measured40MiB System RAM. This whitelist is based on recovered address setters; production implementation still needs broader concurrency/cache/error qualification.

## Bounded execution

Probe allocates owned16KiB table,2MiB frame and4MiB reference storage, but does not start DMA. Main1280x720 planar YUV420 occupies1382400 bytes; sub640x360 occupies345600 bytes. The rest of the2MiB frame allocation is a sentinel guard. Source23's explicit command starts the initialized/ID-checked sensor, guards the operation with the tested5s watchdog, resets ISP19, programs sanitized reference/timing data, enables only buffer0/single mode and polls pending status for at most2s. It then disables capture and resets ISP19 before inspecting/exporting bytes, stops the sensor, and magic-closes the watchdog. The CPU ISP IRQ remains masked.

Changed bytes or pending status alone are **not** proof of a valid frame. A candidate must have intact guard bytes, complete exported data, plausible planes/geometry and an actual sensor scene before being presented as the requested confirmation picture. No success is inferred from a compiler result.

Source23 boots/allocates successfully but its hardware test times out with **zero changed frame bytes** and intact guards. Source24 adds the omitted retail `ak_camera_add_device` PERIPLL1 bit25 clear (not a PLL-divider retune), records direct control/table/order/clock readbacks, and computes target CRC32/BZIP2 after stopped DMA. The host extractor requires complete numbered UART data and checks that CRC when present. This is an experiment, not yet a positive frame result.

## Remaining production work

V4L2/media graph, vb2 queue ownership, IRQ scheduling, CCF/reset/pinctrl consumers, continuous-frame stress, formats/controls/AE/AWB, coherent register-table updates, stopped-DMA lifetime guarantees and encoder/RTSP integration remain incomplete. The diagnostic deliberately does not expose a production `/dev/video0` claim.
