# Shared camera framework baseline

`include/camera/backend.h` defines versioned, opaque driver contracts and a
capability/device graph. `src/backend.c` validates it. The services layer will
own streaming fan-out, authentication/UI/ONVIF/discovery, configuration/WLAN
transactions, setup/reset and recording/retention; drivers own hardware.

Physical sensors and their main/sub profiles are distinct concepts. Two
physical cameras may share an encoder or PTZ head; resource groups and graph
associations represent that without fabricating concurrency or motor feedback.
An unqualified or absent feature is not advertised as working. Missing control
operations return unsupported. Frame/packet leases have generations and no
physical-address API; adapters must enforce lifetime and DMA ownership.

Prefer small statically linked adapters on constrained targets. Optional
dynamic modules require ABI/size validation and exact platform builds. Linux
V4L2/DMA-BUF, nl80211, GPIO/IIO/input and block/filesystem interfaces should be
reused where qualified. Existing QC07F diagnostic character APIs belong in
its adapter, not in shared web/NVR services.

This directory is currently **contract/fixture code, not a deployed framework**.

Latest WEB-02/STREAM-08: bounded C init/profile parsing, actual RTP timeline,8192B
borrowed spans and exact CPU-hub lease validation are fixture-passed. A handoff
model reuses existing producer/hub buffers; actual scheduling/driver/TLS integration
is not adopted.26 C +2 Python +76 receipt cases pass with sanitizers, plus C++→C
link/run and ARM static links. C-remuxed retained source decodes pixel-identically
and in MSE; one reported renderer drop prevents zero-drop display qualification.
See `evidence/qualification/qc07f/web02-init-cadence-handoff-20261012/README.md`.

2026-10-12 WEB-02/STREAM-08 increment: `camera/fmp4.h` / `src/fmp4.c` provide a
bounded112-byte fragment header plus borrowed NAL span, no allocation/I/O/auth
decision. All11 headers match the actual retained-H264 browser MSE fixture;
sanitized22 C +2 Python +76 receipt cases pass, C++ header/ARM static link pass.
Init/profile/cadence/lease/TLS-producer integration remains open; media stays501.
See `evidence/qualification/qc07f/web02-fmp4-fragment-20261012/README.md`.
The existing QC07F bring-up server still has board-specific paths and requires
an adapter migration. Other two cameras and their dual sensor paths remain
unqualified. `tests/backend_contract_test.c` tests ABI/topology/unsupported
behaviour only; it does not qualify real PTZ, shared encoders or dual sensors.

Product acceptance: `../docs/CAMERA_PRODUCT_ACCEPTANCE.md`.

Classvtable ABI/sizes arechecked independently for capture, encoder, controls,
radio andstorage. WLANoperations areasynchronous/resumable anddo notpromise
scan-while-associated/AP+STA withoutqualifieddrivercapabilities. Platform
storageexports anownedrecordingdirectory, not arbitrarypaths/formatting.
`dma_integrity.h` is a sharedRAM-only kernelhelper: exactbyteCRC/guard semantics
withalignedwordreads. HostsanitizedandARMCPUoraclefixtures pass; codecwordguards
also haveactualQC07F180-framehardwaredecode qualification. Thatdoesnotqualify
theothercamera platforms orthenewkernelISPwordCRC beforeitboots/tests.

`lease_pool.h`/`src/lease_pool.c` add a caller-serialized ownership/generation
model for smallbuffer pools. Device-owned orconsumer-leased slots cannot be
requeued; stale completions/releases arerejected. Stoprequires actualDMA
retirement andpreservesoutstandingleases/storage. Initialize onlyfreshpools;
do notreinitializewhileoldleases/referencesexist. Thisis model/fixturecode,
not a substituteforvb2/IRQ synchronization orproof ofhardwareDMAretirement.
Per-camera pools areindependent; actual sharedengines stillneed scheduling.

`memory_budget.h` / `src/memory_budget.c` add checked per-physical-pool/engine
DMA and user-copy accounting with no board defaults. Host fixtures reject the
current20.125-MiB QC four-slot+codec plan against16MiB, and keep the proposed
two-slot/import14.125-MiB arithmetic distinct from unimplemented/untested
hardware import and masks. Allocator, socket, recorder and sink obligations
must be included; arithmetic fit is not allocation/lifetime qualification.

The product/UI controlbroker mustown logicalproducer commands aswellasdevice
leases. An idle-onlykernelimaging operation may safelysucceed during a renewal
gap whileanRTSPclient isconnected. Desired/appliedstate andtimedresults belong
to thesharedservice; don't useclientpresenceas a hardwarelease predicate.

## Shared streaming implementations

- `packet_hub.h` / `src/packet_hub.c`: bounded, caller-serialized encoded packet
  fan-out. Consumers join on a keyframe. Slow pending consumers lose their
  backlog and wait for another keyframe; held leases are never overwritten.
  Stop preserves held data until release. Allocate four equal-sized slots per
  physical stream; the caller must keep storage alive and prohibit publisher
  input from overlapping hub storage. This is not a hardware buffer queue.
- `rtp_h264.h` / `src/rtp_h264.c`: shared TCP-interleaved RFC6184 single-NAL and
  FU-A packetization, with per-consumer sequence/channel/SSRC. It does not know
  device paths or SoCs. The caller supplies SPS/PPS before IDR, monotonic capture
  timestamps, a synchronous send callback, and bounded backpressure handling.
  QC07F RTSP a13 builds against this code; live validation is pending.

Sanitized host fixtures cover fan-out, slow consumers, four simultaneously held
slots, keyframe recovery, stale leases, stop, RTP reassembly and sequence wrap.
The packet hub is not yet wired to the RTSP client scheduler or recorder. These
tests are not evidence of live multi-client streaming or dual-sensor support.

Run all shared C fixtures in a **new** output directory:

```sh
python3 framework/tests/run_tests.py --output /tmp/opencode/camera-framework-tests-01
```

The runner uses Clang with ASan/UBSan and records commands, compiler version,
timestamps, input hashes and output hashes. It refuses an existing directory.
Fixtures have no UART, network, camera, flash or SD access.

## Experimental RTSP scheduler

Current2026-10-10 increment: fair nonblocking per-peer RTP cursors replace
head-of-line serialized socket writes while retaining one producer/one hub
AU at a time. `camera_rtp_h264_packet` permits partial-write retention without
premature RTP sequence/offset commits. Strict wire capture/correlation tools
exclude truncated terminal AUs from decoder input. See
`../docs/QC07F_SEND_CUTOFF_AND_WHITEBALANCE.md` for actual target failures,
the explicit2-s test policy, and finite a20 qualification scope. The historical
lab descriptions below are not claims of completed product streaming.

`rtsp_server.h` / `src/rtsp_server.c` now wire the packet hub and RTP packetizer
to a shared TCP RTSP scheduler. A producer callback supplies one owned H.264
NAL per capture/encode; up to four consumers receive it with independent RTP
state. Joining clients request a fresh IDR. A stalled peer has a **100ms whole
access-unit deadline** and is disconnected without blocking other consumers
indefinitely. This remains serialized capture/encode/send, not DMA overlap.

The shared API now accepts an explicit10..2000ms whole-AU send budget (zero
selects100ms lab default). QC a17 selects500ms after actual healthy target sends
exceeded100ms; host fixtures also test500ms. That is bounded backpressure policy,
not a claim that healthy peers or normal FPS already pass on the target.

A18's serialized sender now explicitly uses one packet slot with1MiB capacity,
matching the codec's verified output limit. A17's128KiB lab limit rejected a
valid153KiB IDR; that failure is preserved, not interpreted as codec corruption.
`camera_hub_init_count` supports1..4slots; the original init still selects4.
One-slot held-reference/backpressure/stop fixtures pass. A18 target60-frame
decode passes with168–170KiBNALs, while renewed multi-client tests are pending.
This bounds packet storage at1MiB, not four worst-case NALs. It does not budget
future asynchronous recording/network workers or solve hardware rate control.

The scheduler has no hardware paths. The QC07F bring-up adapter can select it
explicitly with `--fanout`; RTSP a16 is built but not hardware-qualified. Its
lab packet limit is128KiB, with oversized frames rejected/logged (never clipped
or silently encoded in software). Other platforms still need their adapters
and measured capability/resource graph. Four clients do not mean four sensors.

**No authentication is implemented in this experimental scheduler.** It is not
the protected product/NVR endpoint and does not complete streaming acceptance.
ONVIF, browser bridging, rate control and true queued capture remain separate
pending work.

`tests/rtsp_fanout_test.py` runs an explicit loopback-only recorded-IDR fixture:
three FFmpeg clients each decode24 frames while a fourth consumer stalls and
is disconnected. Sanitized fixtures pass; recorded repeated pixels are not
fresh camera frames or proof of on-target multi-client streaming. Unlike the
five C unit fixtures, this integration test deliberately uses loopback sockets.
