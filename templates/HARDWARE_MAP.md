# Retail-board hardware map

- Target/session: unknown
- Board photographs or annotated physical map: unknown
- Board markings: photograph indicates AV100D / V430P4 / DGH_4G
- Processor marking: photograph indicates ANYKA KM01
- Exact variant from runtime/documented identification: unknown
- Flash marking, JEDEC, main-array capacity: unknown
- Installed RAM and authoritative evidence: unknown
- Stock kernel/U-Boot versions: unknown

## Main UART — already identified by user

| Signal or property | Observed value | Evidence / measurement |
| --- | --- | --- |
| Camera TX pad | unknown | user has identified; record exact pad |
| Camera RX pad | unknown | user has identified; record exact pad |
| Common ground | unknown | unknown |
| UART idle/high voltage | unknown | unknown |
| Host adapter and input-level support | unknown | unknown |
| Host port | unknown | unknown |
| Baud / framing | unknown | unknown |
| First passive capture | unknown | unknown |

## Power and peripherals

| Function | Component/interface | GPIO/register/pad | Active level / sequence | Stock evidence | Verified locally? |
| --- | --- | --- | --- | --- | --- |
| Camera input power | unknown | unknown | unknown | unknown | no observation supplied |
| SD socket | unknown | unknown | unknown | unknown | no observation supplied |
| Cellular modem | Fibocom LE270-CN, photograph | unknown | unknown | unknown | label only |
| Sensor | unknown | unknown | unknown | unknown | no observation supplied |
| Sensor reset/power/clock | unknown | unknown | unknown | unknown | no observation supplied |
| IR cut / illumination | unknown | unknown | unknown | unknown | no observation supplied |
| Pan/tilt motors, if fitted | unknown | unknown | unknown | unknown | no observation supplied |
| Audio input/output | unknown | unknown | unknown | unknown | no observation supplied |
| Reset / watchdog | unknown | unknown | unknown | unknown | no observation supplied |
| Buttons / status indicators | unknown | unknown | unknown | unknown | no observation supplied |

## Runtime memory and image loading

| Range or address | Value | Source | Notes |
| --- | --- | --- | --- |
| Physical RAM base and extent | unknown | unknown | do not use media reservation as total RAM |
| Reserved media/DMA regions | unknown | unknown | compare stock DTB and runtime |
| Bootloader-reserved/relocated area | unknown | unknown | unknown |
| Stock kernel load / entry | unknown | unknown | observed values are not automatically safe for a different image |
| DTB location and extent | unknown | unknown | unknown |
| Initramfs location and extent | unknown | unknown | unknown |
| Kernel decompression workspace | unknown | unknown | allow for expanded size and overlap constraints |

## SDK comparison

- Closest candidate KM01D/N DTS and reason: unknown
- Matches supported by stock evidence: unknown
- Differences requiring a retail-board patch: unknown
- Unresolved proprietary driver/library dependencies: unknown
