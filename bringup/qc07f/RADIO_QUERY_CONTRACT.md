# TXW801 transport qualification

Basic replacement USB endpoint0 enumeration is directly observed: owned fitted radioVID/PIDa012:8400,480Mb/s, configuration1, bulk01OUT/81IN512 bytes. The descriptor requests400mA. Stock normal runtime already supported that fitted device; replacement's initial100mA software cap rejected configuration. Source33 uses400mA and disables external hubs. **Neither descriptor declaration nor policy is an electrical-current measurement.** No new external VBUS circuit/device is operated.

PHY bootstrap uses the retail REA operation58 mask/value `(old & ffffc038)|17c6`, controller POWER21, clock/reset15 and source-backed radio powerGPIO42. Older BSP17c0 was insufficient. User_gpio entryc041e8f0, board5 list/pdata and applicationGPIO_WIFI_SetValue confirm42; other candidate tables or the tempting stockGPIO1 high level were not substituted.

Source34 adds only an explicit **read-only firmware-info query**, not a network driver:

- Require configured owneda012:8400 device before opening `/dev/bus/usb/001/002`.
- Claim interface0; ARM32 USBDEVFS bulk UAPI16-byte layout is asserted.
- Send exactly12 bytes: `2b1a03010c0001002b000000` (TX magic1a2b,type3,STA1,len12,cookie1,command43,status0).
- This layout is directly retained from REA `hgic_fwctrl_send_cmd` and getter43. `hgic_fwctrl_get_fwinfo` expects up to24 returned payload bytes; the newer public comparison driver also labels command43 as shared fmac/smac.
- Read at most four512-byte packets with500ms deadlines under a5s watchdog; match RX magic2b1a/cookie1/command43 before interpreting status. Release interface and magic-close guard afterward.
- No firmware download, OTA, OTP/eFuse, calibration write, channel/key/SSID/association command is emitted. Received bytes are evidence, not instructions.

The public comparison tree listsUSB8400 in FMAC but does **not** include the matching SMAC source directory. USB ID/header similarity alone does not establish host/firmware ABI compatibility. Never substitute HaLow firmware or declare networking from a queried version. Preserve retail firmware, exact host contracts and unresolved firmware/security-event semantics before attempting association with the user-authorized guest network. Credentials are not copied here.

## Direct state and same-image RAM-loader experiment

Source35 keyed type7 ENTER query directly returns checked BOOT state (cookie2,command0,status0,checkfd). REA's actual stock `hgic_bus_usb` template confirms checksum mode2 and2048-byte fragments; stock allocation/sender confirms20-byte command layout and8-byte key at11..18. This is no longer an inference from the short application-type3 reply.

Candidate37 embeds only the exact retail chip8400 payload (151056 bytes, payloadSHA94f4c48d7ad8716514398ea9d9081c1ba3271ddb328a828c3945b9b2aaccaefb). Full file SHA4ed2c2f48e30262f0f7acf362f4791e1198e2aca493de9113078a8df6c0f097e remains unchanged. Raw reflected CRC residue0 matches local header0; AES is off. Firmware licensing is not reassigned by the local GPL loader and it is not published.

An explicit `radio load` command refuses a changed image/VID/PID/configuration, requires BOOT plus accepted GET_SC/security query, and writes only radio RAM20000000..20024e10 in74 unpadded fragments. Retail USB commands declare each fragment+1024 (upper declared envelope20025210); actual transmitted firmware bytes remain exact. REA `hgics_tx_work` strips virtual type15's20-byte header before USB, so data goes raw, with a zero-length USB packet after512-byte multiples. This is not a main-CPU `mw` operation: main20000000 is ISP MMIO and is never used by this loader.

Command ACKs require cookie/command/checkfd; raw-data ACKs require cookie/status/checkfd, matching the separate stock send_fw behavior. Every transfer is bounded; a5s camera watchdog is fed after each accepted fragment. RUN uses exact address/size and retained CRC/SPI-config flags0x60000000. No boot register-write, speed/protection change, OTA, OTP/eFuse, main/radio SPI erase/program command is present. GPIO42 cold reset and unchanged stock-host firmware loading provide recovery. RAM acceptance/RUN ACK is not proof of firmware readiness or association; verify subsequent state/FWINFO and host/security-event ABI first.
