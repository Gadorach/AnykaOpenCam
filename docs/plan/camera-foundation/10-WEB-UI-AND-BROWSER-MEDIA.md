# 10 — On-device browser media, administration UI and common control API

Entry: [index](00-INDEX.md). Prefix: **WEB**. Depends: CORE, STREAM, SEC-01–05.
First milestone G3: live media and authenticated shell; remaining pages close
with CTRL/AUDIO/STORE/SETUP/NVR, not with placeholder buttons.

## Baseline

Current loopback host bridge decodes RTSP to JPEG and demonstrates freshness,
incomplete-AU refusal and both-axis display orientation. It is not on-device,
authenticated, browser H.264 muxing, storage playback or a completed Web UI.

## Tasks

- [ ] **WEB-01 — Browser/media feasibility.** Test qualified hardware H.264
  profile/SPS/PPS in current desktop/mobile browsers. Default candidate is small
  on-device fMP4/MSE muxing with encoded-AU reuse, no decode/re-encode on ARM926.
  Implement appropriately qualified WebRTC only if measurements justify its
  network/security/resource cost. An explicit JPEG mode may serve compatibility,
  but must not silently replace the required normal hardware stream.
- [ ] **WEB-02 — Authenticated live-media endpoint.** Generate initialization
  and bounded fragments with IDR boundaries, accurate timescale/DTS/PTS, codec
  reconfiguration and discontinuity generations. Handle slow readers, expired
  sessions, errors and disconnect; no source corruption or stale buffered frame
  presented as live. Do not expose a secret-bearing RTSP URL to browser logs.
- [ ] **WEB-03 — Small on-device server and assets.** Pin dependency versions,
  serve local compressed/static assets with correct caching and content types;
  no mandatory CDN/cloud. Bound connections, request bodies, fragment caches and
  event streams within PLAT's memory budget. Measure TLS/login/asset costs while
  video is encoding and recording.
- [ ] **WEB-04 — Common API.** Versioned JSON/resource schema using stable graph
  IDs, capability states, profile info, health and operation results. Expose
  desired versus applied settings and revisions. All state-changing requests
  go through SEC authorization and CORE's owner; no shell-command interpolation,
  raw paths, arbitrary device ioctl proxy or fabricated unsupported values.
- [ ] **WEB-05 — Live page.** Per-physical-camera selection, real profile/FPS/
  timestamp/age/connection/error display, image-fit/fullscreen and orientation
  indicators. Distinguish waiting, reconnecting, paused, source fault and live.
  Stop requests on hidden/disposed pages as policy requires; disconnect must
  release all leases. Multiple sensors must not be faked as main/sub tabs.
- [ ] **WEB-06 — Control page.** PTZ press/hold/release with bounded lease renewal
  and STOP on pointer-up/cancel/blur/disconnect; truthful position/limits.
  Independent white/IR controls, manual/auto day-night and real light reading
  units. Imaging/manual/auto controls show applied status and pending errors.
  Disable unavailable controls with explanation rather than simulating success.
- [ ] **WEB-07 — Settings/admin page.** Separate RTSP/UI credentials, DHCP/static
  IP/DNS/hostname, WLAN selection and explicit disruptive scan, recording/motion
  masks/retention, audio and device health. Display validation failures, rollback
  and operation IDs. Protect all controls with sessions/CSRF as applicable.
- [ ] **WEB-08 — Network transition UX.** SETUP's pending operation returns a
  receipt before disconnection, expected reconnect method/hostname, resumable
  result retrieval and clear success/rollback. Browser reload or old IP failure
  must not imply failure was an empty scan result or invite repeated commits.
- [ ] **WEB-09 — Recording browser.** STORE supplies recording IDs/catalog,
  timestamps, valid duration, import/owned status and playable renditions.
  Implement authorization/range/seek, concurrent playback and retention leases,
  missing/corrupt/incomplete states. Do not read a user-provided arbitrary path.
- [ ] **WEB-10 — Functional/browser tests.** Login/logout/session expiry,
  multiple users/tabs, responsive controls, keyboard/touch, orientation/mask
  consistency, abrupt media close, source reboot, network switch and SD removal.
  Test the on-camera service with real media; host mocks establish only UI/API
  behavior. Validate at least one desktop and one mobile browser per declared
  supported family. Capture browser errors and memory/latency evidence.

## Gate

At G3 a browser connects directly to QC07F, authenticates and displays real
hardware video without the host bridge, alongside authenticated NVR/RTSP use.
At G8 every required UI action works against its qualified backend. A mockup,
static video, hidden host relay or accepted request with no applied result does
not count as working implementation. Keep browser transport choice and measured
compatibility/resource limits documented.
