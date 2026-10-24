# Driver foundation01 — offline paired image contract

2026-10-24 · RP-11/CTRL-02, PLAT-01/02/04/08. Written before recipe/source/build.
Author owns only this directory, `bringup/qc07f/driver-foundation-image/` and
fresh `work/lightbulb-camera/driver-foundation-20261024/`. Parent alone owns hardware.

## Exact selected change

Start from the independently accepted coherent Linux6.18.55/NEW Clang23.1.3
query77 full source/build baseline; see its PARENT_DISPOSITION and
BUILD_MANIFEST01 under `encoder-query77-build-20261022`. Read its builder and
successful package02 as TEXT only; do not import/invoke old helpers. Copy the full
source into a separate fresh tree after auditing paths/symlinks/absence of SCM.
No original source, generated header, build, toolchain, image or seal is writable.

Replace ONLY `arch/arm/mach-ak3918ev200/ptz.c` with the complete independently
reviewed `bringup/qc07f/ptz-off-retain-candidate/ptz.c`, SHA256
`a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`
(local33ec694). No loose object reuse. The source includes the prior counter guard,
terminal bank-wide actual descriptor/device/code retention on uncertain OFF or
partial direction effects, original +/-4/20ms/phases3,6,12,9, no homing/position.
Its probe requests/writes no GPIO. Retention may leave coils energized indefinitely;
it is not STOP, current/thermal protection, provider-binding or recovery proof.

Exactly TWO configuration symbols may differ from accepted query77:
`CONFIG_LOCALVERSION="-qc07f-driver-foundation01"` and
`CONFIG_INITRAMFS_SOURCE` naming the fresh archive. Preserve all compiler flags,
compiler23.1.3 identities, SND/audio-disabled selection, memory/CMA, watchdog,
console, recovery and boot policy. No media/backend/query78/new station source.
Existing query77 observational source remains byte-identical as baseline only.

Retain actual `attempt45.dtb` byte-identical (2571B, SHA256
`d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5`).
Its actual active nodes (not disabled prototype source DTS) are board5 axis0:
GPIO10,11,19,20; axis1:15,14,13,24. Both have status okay, flags0, direct GPIO
provider phandle3: active-HIGH/no open-drain or open-source flags, unchanged
push-pull lookup. No DTS/probe changes, GPIO sweep, guessed remux or native mux proof.

## Narrow initramfs cleanup

Inspect newc paths, headers, types, hardlink groups and symlink graph in memory
before copying any bodies; never extract/execute. Exact observed regular payload
set is init, hgics.ko, bin/busybox, bin/wpa_supplicant, bin/wpa_cli, bin/rtspd,
bin/udhcpc.script, bin/qc07f-dhcp-script-bridge. No credential/config payload is
selected: etc is empty. Refuse unexpected payload paths/types/links.
Remove exactly `hgics.ko` and `bin/rtspd` from the NEW archive only, after
establishing both are standalone regular files. Preserve every other raw record
(including header, type/mode/uid/gid/inode/nlink/mtime/body/padding), order and
trailer. Only terminal zero padding may be realigned to the new512B archive length.
No renaming, fallback module, new userland or autolaunch change. Original archive
stays immutable. Monitor init/help/wifi-attach code remains byte-identical, but
its module file is physically absent: wifi attach remains unsupported.
This is narrow PLAT-02 cleanup, not complete platform/module adoption.

## ONE full build and package

Use the baseline clean Kbuild arguments/environment (neutral build identity),
fresh output and existing pinned host tools only. One olddefconfig, then ONE
`make ... -j4 zImage vmlinux`; no modules/install, target ELF/helper execution,
tests/adversarial/mutation/fuzz or native action. Kbuild's necessary native HOST
generators may run; no target/userland content is executed. Stop/preserve on first
actual recipe/compiler/link/package failure: no automatic correction or retry.
Record real UTC/argv/logs, tool versions/pins, complete source association/exact
delta, config/generated headers, consumed .cmd dependencies, PTZ built-in/no
MODULE command, real vmlinux symbols/code, DTB and embedded archive association.
Keep licenses, original COPYING/notices and local-only retained binary provenance.
Reuse baseline closure references where appropriate rather than inventing another
250k dependency framework. Final-only discovered inputs are labelled as such.

Legacy UIM: 64B IEEE-CRC header, new ARM zImage + unchanged appended DTB,
load0x82408000/entry0x82408040, timestamp0/Linux5/ARM2/kernel2/outer compression0.
Check XZ+LE32 Image, real ELF page table/kernel BSS, compressed BSS/stack,
head.S DTB-growth and64KiB malloc bounds against measured40MiB
[0x81800000,0x84000000), existing staging/CMA split0x82800000 and CMA
[0x82800000,0x83800000). No padding/truncation/ceiling/config repair to fit.
Calculate real header/data IEEE CRCs and full-image CRC32/BZIP2. New filename and
pins identify the driver image, never old75/query77 pins as new evidence.

## Existing boot effects and remaining gates

Unchanged active baseline boot includes sensor GPIO41 reset/MCLK setup, radio
GPIO42 LOW2s/HIGH then USB-host enumeration and ISP coherent allocations. It is
NOT an electrically inert boot. No automatic sensor streaming, firmware load/RUN,
station/network, motor pulse, microphone/audio or server starts are added.
No active audio fake provider/SND enable. Speaker testing DEFERRED, Wi-Fi P1 review
restriction and incomplete native ADC provider unchanged. No hardware-ready claim.

RAM-only package is larger than1.5MiB NOR KERNEL: no flash/install/repartition.
Static fit is not native ABI, live DMA/stack/combined-load or allocation-cap proof.
Original75/76/query77 rollback remains immutable. Supervised detect/abort/manual
fallback policy permits later reviewed entry preparation, not an invocation here;
strict C1 stays unproved. Independent ordinary review and parent disposition of
all new code/proof are mandatory BEFORE use. Stop after handback; no native queue.
