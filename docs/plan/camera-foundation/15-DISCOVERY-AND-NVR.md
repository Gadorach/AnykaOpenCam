# 15 — Network identity, DNS advertising, ONVIF and real NVR integration

Entry: [index](00-INDEX.md). Prefix: **NVR**. Depends: CORE/SEC/STREAM/CTRL,
WEB and SETUP; audio/recording capabilities only after their qualification.
Primary gate **G7**. Most services are not implemented yet.

## Tasks

- [ ] **NVR-01 — Stable identity.** Device UUID, human hostname/display name,
  model/firmware version and graph IDs independent of DHCP address. Protect
  private identifiers in exported evidence. Multiple physical sensors/profiles
  have stable distinct IDs; sharing an encoder/head does not merge sensors.
- [ ] **NVR-02 — DHCP/DNS integration.** Send qualified hostname/FQDN options,
  allow local hostname/DNS settings, and observe DHCP-mediated registration on
  an owned server that supports it. Router behavior is external: report whether
  registration occurred, not a blanket promise of dynamic DNS. Do not modify
  DNS/router accounts or create external DNS records as an implied task.
- [ ] **NVR-03 — mDNS/DNS-SD.** Advertise hostname and only implemented services
  with supported record types/TXT fields/ports. Resolve collisions, validate
  names, withdraw on shutdown/address/interface changes and reannounce after
  SETUP transitions. Prevent setup-only services leaking to unintended interfaces.
  Bound multicast traffic/cache/resource use; test more than one camera identity
  with fixtures before deferred hardware ports.
- [ ] **NVR-04 — ONVIF capability selection.** Check current Frigate/Blue Iris
  documentation and actual clients for required services/auth/profile/transport.
  Select the implemented ONVIF feature subset explicitly. Do not claim formal
  ProfileS/T/G conformance or discovery in a client that does not provide that
  workflow merely because generic SOAP responses work.
- [ ] **NVR-05 — WS-Discovery and device service.** Stable endpoint UUID, scopes,
  Hello/Bye, Probe/Resolve, correct reachable XAddr and bounded XML/SOAP parsing.
  Implement relevant device/time/network/capability information with real data.
  Handle interfaces/address changes and repeated probes without unbounded memory
  or unauthenticated state-changing controls.
- [ ] **NVR-06 — Media and imaging services.** Enumerate actual physical sources,
  stream profiles/configuration/options, authenticated stream/snapshot URIs and
  real supported imaging ranges/modes. Correct native/sub-profile relationships,
  hardware concurrency limits and restart/discontinuity behavior. Reject unsupported
  combinations instead of silently changing another client's profile.
- [ ] **NVR-07 — PTZ and control integration.** Relative/continuous movement,
  STOP and truthful status/options use CTRL's single owner and safety leases.
  Absolute positions/presets only when calibrated. Map orientation and shared-head
  associations correctly. Standard illumination/day-night controls where defined;
  otherwise documented authenticated extension endpoints, not invented ONVIF data.
- [ ] **NVR-08 — Events and optional recording services.** Expose measured motion/
  health events with bounded subscriptions, renewal and disconnect behavior.
  Add ONVIF recording/playback services only if selected as a real requirement;
  browser SD playback does not automatically imply ProfileG conformance. Keep
  supported service claims synchronized with STORE/AUDIO implementation.
- [ ] **NVR-09 — Actual Frigate qualification.** Run the documented supported
  setup/discovery or explicit configuration workflow, verify authenticated RTSP,
  video/audio formats, detection/recording ingestion and supported PTZ controls.
  Record NVR version/config without credentials, source profile, reconnect,
  timestamps and concurrent browser/camera recording. Host-only test software
  is useful but is not this integration result.
- [ ] **NVR-10 — Actual Blue Iris qualification.** Discover/configure the camera,
  verify ONVIF media/profile/PTZ and authenticated RTSP/audio according to that
  installed version, recording/reconnect and changes to camera IP/settings.
  If the licensed application/test host is unavailable, state the specific
  blocker and keep its gate pending; no simulated checkmark.
- [ ] **NVR-11 — Interoperability fault matrix.** Bad auth, expired nonce, clock
  unset/skew, duplicate client request, changing address/hostname, NVR restart,
  camera restart, malformed XML, unsupported methods and two NVRs competing for
  controls. Resource/rate limits preserve streaming and safe STOP behavior.

## Gate

G7 requires truthful discovery/DNS service behavior and recorded real-client
acceptance for both requested NVRs, including each supported control path.
Publish a local compatibility matrix and known unsupported optional features.
Never equate RTSP decode success with discovery/control integration success.
