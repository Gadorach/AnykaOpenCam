# WEB-01 retained hardware H.264 / fMP4 browser feasibility

Latest shared C source now includes bounded init/SPS/PPS parsing and RTP timeline;
`c_mux_fixture.py` builds a host library from canonical framework sources and
repackages unchanged retained AUs using those actual C functions. Its live-init
duration is0, actual fragments/timestamps are source-derived, and all11 decoded
pixel hashes match. MSE decoded11 but reported1 presentation drop/no callbacks;
not zero-drop/all-presented, native producer or authenticated media proof.
See `evidence/qualification/qc07f/web02-init-cadence-handoff-20261012/`.

This is an **offline finite fixture**, not a live camera endpoint, authentication
integration, general muxer or native memory/FPS qualification. Target media
remains501 until protected producer integration is actually qualified.

`mux_fixture.py` verifies the retained complete Annex-B capture and every VCL
NAL against acquisition metadata. It uses actual90kHz RTP timestamps, AVC
constrained-baseline42e01f/no B frames and the source's one-VCL-per-AU contract.
The last complete AU supplies the next timestamp; no guessed last-frame duration.
It is kept intact, not partially emitted. No re-encode/tint/payload clipping.

The fixture writes643B initialization and112B per-fragment headers followed by
borrowed original NAL spans. Actual137–139kB AUs exceed the service's128kB single
allocation cap. A future camera mux must stream bounded chunks from owned encoded
storage, rechecking authorization/freshness and source lifetime on every effect.
Do not feed a whole AU to `mg_send`, enlarge limits, or mistake this PC prototype
for measured camera heap/socket/DMA/combined-load behavior.

Finite loopback-only `serve_fixture.py` serves just the generated manifest/init/
fragment files and these player assets, no directory/camera/raw/credential tree.
It closes automatically. The MSE player labels the video as retained, not live,
preserves both-axis preview corrections and uses no decoded-pixel filter.

Observed desktop OpenCode/Electron Chrome152 decoded11 native720p frames with
duration5.008s and no reported media error. FFmpeg independently decoded all11 to
the same source pixel hashes. Video-frame callbacks observed only9 timestamps
and max presented count10: **not proof every decoded frame was presented**.
Other desktop/mobile browser families, live protected on-device service, dynamic
SPS/GOP changes, multiple VCLs, authenticated leases and combined load remain open.

Evidence: `evidence/qualification/qc07f/web01-retained-h264-mse-20261012/`.
Original captures are unchanged; generated footage stays local/untracked.

```sh
python3 -B apps/web-media/feasibility/mux_fixture_test.py
python3 -B apps/web-media/feasibility/mux_fixture.py --capture /path/to/qualified/capture --output local/web-media/fresh-fixture
python3 -B apps/web-media/feasibility/serve_fixture.py --fixture local/web-media/fresh-fixture --seconds 120
```

Next: implement the bounded reusable header/span mux contract in shared C, test
truncation/time/profile/IDR/overflow/lease behavior, and integrate WEB-02 admission
with the single qualified producer. Independent RTSP credentials/authentication
remain required for G3; do not route through the unauthenticated host lab bridge.
