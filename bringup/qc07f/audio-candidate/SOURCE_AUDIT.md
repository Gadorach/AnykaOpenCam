# ADC2/capture-only source audit — AUDIO-01/02

This increment implements source/HOST/kernel-object candidate work, not mic
capture. User-reported mic is **required**. Retail resource/bias/routing/clock/
cache/IRQ/L2 provider and all AUDIO/G4/G7/G8 native acceptance remain open.

## Pinned evidence and adaptation

Public family source container is recorded as OpenIPC/ak3918ev200 revision
`5fafbded3f4fa2552890d78674d88aacd5cac86a` by the retained routing report, not
freshly Git-verified here. Selected files from
`work/lightbulb-camera/deep-bringup-20261008T055753Z/sources/ak3918ev200/vendor/linux-3.4.35-ak3918-diff/`:

| Source / exact lines | Supported software fact | Candidate adaptation / still missing |
|---|---|---|
| `sound/arm/ak_pcm.c:1–19,127–151,966–983` | Anyka copyright/GPLv2-or-later; mono, U8/S16_LE, continuous5500–96000Hz family constraints,512B capture periods,64KiB family ring ceiling | Narrow finite8/16/24/48k **candidate subset**, MONO/S16 only,512B×4=2048B ring; no current retail clock/rate claim |
| `sound/arm/ak_pcm.c:615–672,772–805,890–916` | BUF2MEM L2 capture, completion callback, byte/sample accounting, clock/config/init ordering | Tokenized cache-device→DMA→cache-CPU→READY; provider first-sample epoch/rational PTS, no do_gettimeofday/default nominal rate or300ms delayed work copied |
| `sound/arm/ak_pcm.c:1015–1029,1070–1089` | ADC common-buffer allocation, wait-for-DMA then free | Owned exact STOP and RELEASE fences; held borrows retain backing/code; no indefinite unbounded wait/free-on-marker assumption |
| `sound/arm/ak39_codec.c:317–324,608–716,1191–1340` | Gain raw0–7; measured-parent PLL input; high-speed divider1..64; ADC2 OSR512 for≤24k,256 above; ADC2 L2 enable/close and shared analogue/reference writes | Exact integer parent/(OSR×divider)==requested required; no shared PLL retune/nominal rounding or GPIO/DAC/HP/speaker route. Gain metadata read-only/raw, not dB |
| `arch/arm/plat-anyka/include/plat/l2.h:75–164,219–260`; `l2.c:1–67,238–285,592–645` | Eight shared512B common buffers,64B DMA granularity; ADC2 device ADDR_ADC; UART/USB dedicated lanes and shared registers; global fraction-DMA state | Full512B periods avoid requesting residual fraction transfers, **not** an exemption from global L2/UART/MCI arbitration. No private reset/init/clear-global registers or unowned cache/IRQ callback |

Original complete source notices/files and GPL COPYING are preserved under each
fresh build's `upstream/`. `ak39_codec.c` has MODULE_LICENSE("GPL") but no initial
per-file copyright block; that absence is preserved, not silently replaced with
a workspace license. The kernel/L2 notice applies per source. The implementation
uses GPL-2.0-only compatible adaptation; `adaptation.diff` compares original
`ak_pcm.c` to the actual capture-only glue and documents omitted paths. Stock
firmware binaries/modules are not executed or used as ABI-compatible drivers.

Retained retail REA annotations inspected, not binary/ROM contents:
`contracts/pseudocode/kernel-filtered/286-ak39_codec_adc2_open.c.txt`:
`ev_ceaac9552d189179acc317f51a0ee696f1093acc4fdf3b3fc11f3e7540d94064`;
`288-ak39_codec_set_adc2_samplerate.c.txt`:
`ev_230a372f59c8201fa0950d4ab7cf1d415b633d1a2e96324890b9b3f94af90fc6`.
Both bind subject hash
`790646be95cfde5996d6c32ec4ad0b959e1503c212db415c93d8a383cbadf800` (quoted,
not rehashed/opened). Retail open includes `ak39_adc2_dac_share_open`; rate path
uses actual ASIC PLL and bounded register wait/failure paths. These corroborate
shared resources, **not measured mic wiring/bias or known-safe programming**.

## Ownership and resource blockers

ADC1 ambient AIN0 conversion is separate from audio ADC2. Shared analogue clock,
references/reset/control words must be reconciled before either owner samples.
No ambient/lux/speaker/control feature is inferred. L2 global fraction/request/
IRQ/cache state overlaps UART/USB/MMC ownership. Existing MMC source candidate
also refuses probe and has no qualified L2 provider; this audio unit cannot
quietly reset/bypass it or claim its buffer. Source defaults are not retail nets.

Kernel glue is a real MONO/S16 ALSA ops table (no playback/AEC/codec/transcode),
but its probe **always returns -EOPNOTSUPP before any allocation/card/clock/GPIO/
bias/L2/IRQ/DMA claim**. Kconfig defaults n, DT disabled with no guessed resources.
No active image/config or CMA/IRQ/USB/video/CRC/TLS/credential budget is changed.
The glue's explicit attach/completion seam needs a reviewed native resource
broker owning coherent ring, provider code/module and ALSA card/substream lifetime.
Completion dispatch must be serialized/deferred (not hard-IRQ mutex execution);
period notifications need a live ALSA stream reference and must retire before
STOP ACK. No native broker/IRQ/notification path is provided or claimed here.
`sync_stop`/`hw_free` return EAGAIN until exact fences/borrows retire; no ALSA
automatic-managed/free-pages path may reclaim quarantined backing. Native ALSA
close/removal/provider/module retention, standard period notification scheduling
and common clock routing are **explicit integration blockers**, not ABI proof.

## Evidence classes and next actual requirement

Public licensed family source + retail static REA annotations support this
candidate. Synthetic PCM lifecycle fixtures/ARM static linking/kernel object
compile/relocatable link are separate classes. Sealed Linux6.18.55 MMC prepared
headers are reused **read-only via audited saved Kbuild compiler argv**; no new
header/config preparation, kernel tree copy or image rebuild. Additional actual
ALSA headers are pinned against its sealed Linux input inventory; actual kernel
symbol/runtime linkage (SND not active in baseline) remains unproved. Any failed
compile/header verification is preserved as a blocker, not labelled ABI pass.

One next parent read-only task: fresh sole owner/runtime/recovery followed by a
source-audited **retail mic/ADC2 resource receipt** correlating input/bias net,
clock/reference/mux and shared L2/cache/IRQ owners. Use only already-qualified
passive metadata/readers; no ADC/clock/bias init, GPIO request, L2 reset, card/audio
operation or waveform acquisition. Missing safe net/bias measurement requires
user-coordinated isolation; it does not license an unmeasured output or remove
the microphone requirement. Real sound/timestamps/A/V tolerance/audio tracks and
NVR support require later native qualification, not synthetic data or silent SDP.
