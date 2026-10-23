# Next hardware boundary — operator-assisted cold catch only

2026-10-23. BASE-01/05/07 / PLAT-06/08. Proposed bounded method for independent
ordinary review, **not yet selected for execution**. Passive23b finalized with
0RX/0TX and no current runtime/power proof. User confirmed current RX/TX/GND-only
wiring, no modem-control/reset/boot/VCC leads. No qualified remote camera power
actuator is known; parent must obtain actual operator OFF/readiness, not another
general permission. Keep both failures and all originals unchanged.

## Source reuse, no new controller framework

- `scripts/qc07f_boot.py` SHA256 `6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985`.
- `scripts/qc07f_private_root_transfer.py` SHA256 `cb224d3cd8b635774b7e364b07b259d90640b30de269ea07351e767c3f97fa3c`.
- Original recorder SHA256 `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00`.
- Pure cold-context-v2 SHA256 `17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558`.

Read the actual original `start_receiver` and protected startup paths. Reuse only
`PrivateBoot` construction and `start_receiver(..., power_off_confirmed=True,
private_capture_dir=...)` via an explicitly source-pinned parent call, not the CLI
that opens the original marker append-capable. No old22b controller/epoch/reset,
`caught/owner/process_remaining/arm/upload/pin_bootloader/command` or ROOT payload
function may be called. Do not edit imported sources/constants/source pins.

## Proposed phases

1. Obtain actual camera OFF and operator readiness to control power during the
   catch, including leave-OFF on failure. No remote reset/automatic power cycle.
   Approval alone is not OFF/readiness. GPIO0 is not a reset method.
2. Freshly verify exact source/adapter association, no active recorder/socket/GUI/
   transfer, trusted canonical control ancestry and unchanged historical marker
   objects. Acquire radio/reset/instance locks read-only as in the reviewed passive
   role (radio/instance0644; reset0600). Retain byte/mode/inode failure postchecks.
3. Create fresh0700 private output/capture directories outside Git. Start only the
   exact original recorder with explicit historical port, new foundation-20261023c
   part and14400s duration. No source edits/parallel UART while startup/catch is
   active. Existing output/socket/receipt refuses, no takeover or retry.
4. Original start_receiver positively checks actual new PID/peer/UID/part/source/
   envelope/fuser sole owner, zero TX/RX/busy while confirmed OFF, then requests
   exactly one original hold-cli catch (volatile_cli_hold=True, no inspection).
   It uses only status/arm RPCs, no UART command or image payload. Its identity-only
   fresh23c receipt is fixed under existing evidence path and stays uncommitted;
   all capture/raw/digests/errors remain outside Git. No new private-output policy
   exemption or native timer/coverage guarantee is claimed by that receipt.
5. After actual arm ACK and parent source/owner association, ask operator ON now
   while remaining available for OFF on failure. Do not give an ON instruction
   before an admitted arm or after expiry/unknown ownership; stale instructions
   are not renewable authority. No second arm or boot/catch retry.
6. After the actual ON report, the parent examines only this capture's new raw/
   events and current same-owner status, without concurrent transfer. Classify
   the entire first cold interval through the first prompt with the pinned pure
   exact19-record classifier. Require genuine new boot/arm lineage and only the
   recorded automatic catch packets; password text alone neither passes nor
   fails the profile. Extra/contradictory context refuses; no prefix repair.
7. Separately observe10s no RX/TX/repeat-boot changes on that same capture; this is
   observation evidence only, not whole-interval clock prevention/recovery proof.
   End/finalize the owned recorder explicitly before any new helper/source work.
   Preserve all errors/partial captures. Missed catch/unknown state requires OFF
   and leave OFF, not rescue TX, reset, arm again or automatic reload.

This selects no readback command, loader/image/RAM write, kernel handoff, flash,
motor/peripheral/card/radio/media action. Operator assistance is an actual external
power fallback for this cold observation, not an installed unattended recovery
owner. C1v2, strict whole-interval native clock policy and boot/staging/query/app/
DMA/backing/NOR gates remain blocked unchanged; do not use this catch as native
effect admission. A normal physical power ON can boot stock software if the catch
is missed; actual operator readiness and immediate OFF fallback are indispensable.
