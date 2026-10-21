# Camera product acceptance scope

User scope,2026-10-10. These are requirements, not claims of implemented or
hardware-qualified features. Existing three-camera/imaging/PTZ objectives
remain. Finish the QC07F streaming path first, then advance through the
remaining subsystems; preserve working RAM images and recordings throughout.

## Shared driver-pluggable framework

Userclarification: thisframework mustserveallthreeplatforms, includingthetwo
dual-camera targets. Platform/kernel drivers andsmall userspaceadapters own
registers, routing, timing andhardwareconstraints. Sharedstreaming, webUI,
authentication, configuration, discovery/ONVIF, recording/motion/retention and
setup/reconnection services consumeversionedcapabilities andopaquehandles.
Do notduplicate theseservices perSoC orhardcodeQC07Fdevicepaths into them.

Enumerateindependentphysicalcamera IDs/profileinstances; representmain/sub
outputs asprofiles ofonecamera, notfittedsensorcount. PTZassemblies/lights/
sensors/radios/storage haveindependentstable IDs andexplicit associations.
Two cameras may shareanencoder orPTZhead; describeresource/concurrency groups
andactualqualified limits ratherthanassumingoneencoder percamera orsilently
disablingone stream. Unsupportedcapabilities returnexplicitunsupported/
unavailable errors. Optionalstaticlink adapters suit40MiB targets; dynamically
loadedplugins areoptional, not a reason toforcehostSDKbinaries ontoARMtargets.
Kernelmodules remainbuiltagainstexactmatchingconfig/symbols, neverforce-loaded.

Sharedcontrol coordination mustdistinguish a logicalproducer's lifetime from
its shorter kernelbuffer/session lease. An imagingrequest can beaccepted in
a safelease-renewal gap evenwhileanRTSPclient remainsconnected. Applychanges
through theproducer's commandqueue andreportdesired/appliedstate/results.
KernelEBUSYprotects actualheldcaptureleases; itisnot an adminpolicy. Aclient
connection alone isnot evidence that a hardwarelease isheld at thatinstant.

## Delivery order and gates

1. Streaming: hardware H.264, correct colour/day-night, continuous owned-buffer
   capture, useful sustained native frame rate, multiple RTSP/browser/NVR
   consumers, reconnect/slow-client/stop/restart correctness. Current gated
   720p/~1.4fps prototype is a fallback, not this completion gate.
2. PTZ and board peripherals: safe motion/stop/range model, independent white
   and IR lights, actual light-sensor routing/data or explicit absence, reset
   input/debounce/long-press. Do not infer an IR-cut mechanism, speaker, absolute
   motor feedback, zoom or a fitted ambient sensor from family source.
3. Storage: measured MMC/SD ownership, read existing recordings, robust owned
   segment recording/motion detection/masks/retention, browser playback.
4. Configuration/network services: authenticated control/settings, discovery,
   network switching and recoverable setup AP/captive portal.
5. Integration/qualification: ONVIF/NVR controls/discovery, combined workload,
   reset/setup/security tests, recovery and exact persistent-install recipe.

No subsystem is declared complete from host fixtures or build success. Keep
all-required-modules false until directly qualified. Normal frame rate and
free-running V4L2/vb2 remain unqualified at this scope's creation.

## Network identity and discovery

- Stable per-device hostname, DHCP hostname/FQDN request and DHCP-mediated DNS
  registration where the owned network supports it. Do not promise that an
  arbitrary router accepts dynamic DNS, or change router/DNS accounts.
- mDNS hostname and DNS-SD advertisements for the implemented HTTP/HTTPS/RTSP
  services; clean withdrawal/reannouncement after IP/WLAN changes.
- ONVIF WS-Discovery (NetworkVideoTransmitter) for NVR discovery, with stable
  device UUID and truthful service/capability announcements. Actual Frigate
  and Blue Iris discovery/control tests are required, not generic SOAP alone.

## Protected Web UI and APIs

- Password-protected UI: live video, PTZ, independent white/IR controls,
  measured light readings, health/capabilities and unavailable-feature states.
- Settings: independent RTSP and UI credentials, DHCP/static IPv4 settings,
  WLAN selection/credentials, recording/motion masks/retention and admin setup.
- Browsers do not natively play RTSP: use an authenticated compatible browser
  transport/remux path (H.264 MSE/fMP4 or appropriately qualified WebRTC), with
  an explicit JPEG fallback if needed. A native RTSP URL in a video tag is not
  browser integration. SD playback must use a browser-decodable container and
  honour authorization, range requests and recording selection.
- One captured/encoded producer with bounded per-consumer queues: a slow UI
  viewer must not stall the NVR or capture. Reject stale/corrupt frames rather
  than silently switch to software encoding or replay them as live.
- Avoid shared factory admin credentials, plaintext credential logs, URL
  passwords in logs, or recycling the lab root password as UI/RTSP default.
  Store protected password verifiers/secrets; constant-time comparisons,
  session expiry/rotation, CSRF protection, rate limiting and TLS policy must
  be specified and tested. Locally generated certificates do not imply public
  trust or authorization for ACME/DNS-account changes.

## WLAN transition and setup AP

- Handle WLAN changes as recoverable transactions: validate settings, commit
  pending profile, announce connection loss before it occurs, reconnect,
  verify address/reachability and provide results through a resumable operation
  ID after reconnection. UART/recovery control must remain available in tests.
- If the chipset cannot scan while associated, report that limitation. Offer
  cached/manual SSID entry or an explicit disruptive scan with reconnection
  and result retrieval; never pretend a failed scan is an empty network list.
- Setup AP is required after factory setup/reset. Retail radio AP support and
  hostapd operation must be directly qualified; current station-only driver
  does not establish AP or concurrent AP+STA capabilities.
- Captive portal steers HTTP connectivity probes to the local configuration
  page using AP-side DHCP/DNS/redirects and supported captive-portal options.
  Never intercept the normal LAN or impersonate HTTPS destinations. Pair/setup
  authorization and AP password/session lifetime need a safe default policy.
- Switching to station mode must give the client a clear reconnect/hostname
  instruction, withdraw the portal and re-advertise the resulting services.

## Reset-button behaviour

The user now explicitly requires a reset behaviour that restores camera
settings/admin defaults and starts the setup AP. This supersedes the earlier
restriction against treating button support alone as a settings-reset policy.
It does **not** authorize immediate erasure during development or deletion of
recordings. Implement deliberate debounced long-press confirmation, reset only
the camera-owned configuration, revoke sessions, restore safe peripheral state
and open initial setup. Keep firmware/recovery and SD video intact. Short
presses, bouncing/stuck inputs and power loss must not cause accidental reset.
Hardware testing of the destructive settings transition should use a disposable
RAM/test configuration until persistent storage and recovery are qualified.

## Recording and retention

- Constant recording or motion-event recording, selectable by the admin.
  Motion detection operates on real captured frames with configurable ignore
  polygons/areas, sensitivity, debounce and pre/post-event coverage.
- Bounded segments with trustworthy timestamps/metadata; recover incomplete
  segments after power loss and never serve a partial segment as complete.
- Retention: prune oldest closed recorder-owned segments near low free-space
  thresholds (hysteresis/reserve), or actively prune outside an admin-selected
  date window. Never delete an open/leased segment, follow unsafe symlinks,
  traverse outside the recording namespace, or format the card.
- Preserve existing stock recordings/rollback files by default. Import for
  playback read-only; adoption into deletion policy must be explicit, with an
  inventory/preview. New retention authorization is not a reason to erase old
  acquisition data or unrelated SD files now.
- Test full-card behaviour, removal/reinsertion, FS warnings, interrupted
  writes, concurrent browser playback and retention before deployment.

## NVR control interface

- Implement the applicable ONVIF device/media/PTZ services plus WS-Discovery,
  authenticated RTSP and any vendor-compatible controls actually needed by
  Frigate/Blue Iris. Verify current integration requirements in their docs.
- Advertise only measured support: relative/continuous movement and stop if
  qualified; absolute position/presets only with a defensible calibrated
  position model. No invented motor feedback, zoom/focus or unmeasured limits.
- Expose common measured controls (illumination, day/night, imaging, profiles)
  using standard services where defined, otherwise documented authenticated
  extension APIs. Unknown/unavailable values are explicit, not fabricated.
- API actions share the same motor/illumination owner and safety limits as
  the UI. Client disconnect, auth failure and application restart must leave
  motors stopped and avoid unsafe illumination/power toggles.

## Persistence and release

Changes remain RAM-only until a specific image/address map, configuration
storage plan, rollback and recovery method are established. Product behaviour
requirements are not blanket permission to flash SPI or overwrite recordings.
Keep exact source/build/provenance and classify fixture, RAM-hardware and
persistent-install results separately.
