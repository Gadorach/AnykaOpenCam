# 14 — Network settings, resumable WLAN changes, setup AP and settings reset

Entry: [index](00-INDEX.md). Prefix: **SETUP**. Depends: NET capability result,
CORE events, SEC durable transactions/credentials, WEB and CTRL reset input.
Hardware delivery after G5; primary gate **G6**.

## Baseline

Station association/DHCP works with a privately supplied RAM configuration.
There is no qualified hostapd/setup AP, AP+STA, scan-while-associated, captive
portal, persistent network settings or transactional switching/reset product.
Do not build first-use recovery on an advertised AP feature that has not run.

## Tasks

- [ ] **SETUP-01 — Network state machine.** Enumerate interfaces and explicit
  down/start/scan/associate/addressing/connected/degraded/setup/recovering states,
  link generation and operation ID. Split driver success, association, usable
  address, route and application reachability. Never require public Internet or
  cloud availability to call a usable local camera network connected.
- [ ] **SETUP-02 — IP configuration.** Validated DHCP/static IPv4, prefix,
  gateway/DNS and hostname, conflict detection where supported, DHCP renewal,
  address change and rollback. Define IPv6 enabled/disabled and advertisement
  policy explicitly; do not accidentally expose an unauthenticated service on
  an address family outside the settings model. Avoid modifying router accounts.
- [ ] **SETUP-03 — Scan semantics.** Measured associated/nonassociated scan
  capabilities and limits, cache age, busy/unsupported/error results and manual
  SSID entry. If scan disrupts association, require the explicit disruptive
  operation path, return receipt before disconnection, reconnect to the old
  profile and deliver scan results by operation ID. Failed scan is not an empty
  network list; stale results cannot overwrite a newer requested operation.
- [ ] **SETUP-04 — WLAN switching transaction.** Validate profile privately,
  preserve previous committed profile, persist pending operation/rollback, reply
  with reconnect instructions, quiesce affected services, attempt new association/
  DHCP/address/reachability and commit only on defined success. Timeout/failure
  restores prior profile or enters setup recovery with an explicit result.
  Do not expose SSID/PSK values in command logs, crash data or firmware images.
- [ ] **SETUP-05 — Resumable results.** Survive UI reload, lost HTTP response,
  old-IP disappearance, duplicate requests and reboot midway. Operation receipts
  identify the device and result endpoint without embedding credentials. Define
  pending timeout and final-result retention, idempotent query and rollback
  semantics. Reannounce identity/discovery when the address becomes usable.
- [ ] **SETUP-06 — AP driver/hostapd bring-up.** Implement source-supported
  mac80211/nl80211/firmware AP functions, channel/beacons, key setup, client
  association, DHCP/DNS and regulatory constraints. Qualify one AP client first,
  then required capacity and transitions. Concurrent AP+STA is advertised only
  if tested; mutually exclusive AP/STA switching can satisfy setup with explicit
  reconnect guidance. Station-only mode cannot close this task.
- [ ] **SETUP-07 — First-use setup AP.** Unique device identity and protected
  onboarding policy, local-only setup permissions, per-device provisioning
  secret or equivalent measured scheme, finite enrollment/session lifetime and
  clear recovery. Do not use a shared lab password as the product admin default.
  Handle missing/corrupt config, absent network, repeated failed credentials
  and already-provisioned state without losing recovery access.
- [ ] **SETUP-08 — Captive portal steering.** AP-side DHCP/DNS, HTTP connectivity
  probe handling/redirects and supported captive-portal metadata. Target only
  clients of the camera's setup AP. Do not intercept the normal LAN or imitate
  arbitrary HTTPS sites. Test platform-specific captive assistants and a normal
  browser; provide an explicit local URL when HTTPS cannot be redirected.
  Completing setup withdraws portal/AP as policy dictates and announces STA
  endpoint/hostname rather than leaving a permanent unsecured parallel path.
- [ ] **SETUP-09 — Reset workflow.** Use CTRL's debounced deliberate long press.
  Enter a supervised settings-reset transaction: stop/neutralize owned controls,
  drain new recorder activity safely, revoke sessions, reset only camera-owned
  user/network/admin settings, preserve device identity/physical calibration per
  SEC policy, preserve all footage/firmware/recovery, then advertise setup AP.
  Test using disposable configuration before persistent settings. Restart or
  power loss at each step resumes a clear reset/setup state, not half-old auth.
- [ ] **SETUP-10 — Failure/race tests.** Wrong PSK, disappearing AP, DHCP failure,
  static-IP mistake/conflict, result lost just before disconnect, reboot during
  commit/rollback, two simultaneous administrators, scan cancellation, card/config
  storage unavailable, reset bounce/short/stuck button and repeated setup attempts.
  UART recovery stays available during qualification. No unattended flash erase.

## Gate

G6 requires an actual QC07F setup AP, usable captive steering, recoverable WLAN/IP
changes and result retrieval after disruption, and settings-only reset preserving
footage. If the radio firmware cannot support required AP mode, document a hard
blocker and a technically justified hardware/software resolution for user review;
do not mark reset/setup complete with UART-only recovery.
