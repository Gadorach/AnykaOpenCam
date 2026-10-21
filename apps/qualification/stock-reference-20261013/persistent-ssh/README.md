# QC07F SSH-only trial ROOT proposal — OFFLINE ONLY

One source/host implementation unit for the user's explicitly selected SSH-only
trial boot. **Not installation authority, a flash command recipe, a booted image,
SSH readiness, native acceptance or an entropy claim.** Parent alone owns current
UART/recovery, geometry/backup/readback, private credentials, final image and device
operations. GUI/audio7 sources and all other partitions are outside this unit.

## Boot changes

Only the original `/etc/inittab` sysinit target changes from `/etc/init.d/rcS` to
`/bench-ssh/bootstrap`; one `/bench-ssh/serve` respawn entry is appended. Existing
getty/restart/shutdown entries remain byte-exact. Original rcS, rc.local, camera,
network and application scripts remain unchanged but are not called automatically.
BusyBox init stays PID1 and reaps the foreground Dropbear listener and orphans.

The bootstrap checks root/3.4.35/ROOT-readonly, mounts proc/sysfs, devtmpfs if needed,
devpts, private-mode RAM tmp/var and **USR squashfs read-only**. It does not mount
JFFS/SD, load MMC/sensor/camera/motor/AW, run stock app/cloud/recording/homing, open
PCM/ADC/GPIO, or alter kernel builtin pin defaults. It does not open/feed/change a
watchdog or add a guardian. Only the exact original WD, otg-hs and hgics modules are
hash-checked then loaded. Original mdev.conf/radio firmware/wpa_supplicant and the
reviewed Dropbear are pinned too. `/proc/sys/kernel/hotplug` becomes `/sbin/mdev`;
no stock hotplug/wifi loader or module-autoload script is run. The original radio's
2048-byte firmware download behavior is unchanged, not patched by this overlay.

ROOT's `/bench-ssh/private/` inputs are copied to root-owned0700
`/tmp/bench-ssh`/`keys`, with files0600 and a newly mounted root-owned0755 `/tmp`.
No broad chmod on `/etc`, config, JFFS or any persistent user data. The host CLIENT
PRIVATE key is never an accepted camera input. Account/passwd/shadow/home and
kernel/bootloader/ENV/MAC/USR/JFFS/SD remain unchanged.

Only VertexHotspot WPA2/CCMP is admitted. Original wpa_supplicant uses
`-qq -B -Dnl80211 -iwlan0 -c/tmp/bench-ssh/wpa.conf -P/tmp/bench-ssh/wpa.pid`.
DHCP is the reviewed local-only hook adaptation: wlan0/255.255.255.0 and valid
192.168.1.2..254 only; router/DNS options are ignored. No default route, DNS,
persistent setting write or IP73/reservation assumption. DHCP's successful actual
ifconfig action atomically writes the RAM address marker; initial deconfig clears
it. udhcpc uses bounded initial attempts, default daemonization after acquisition,
and normal renewal. No universal WLAN/DHCP liveness promise is made.

`serve` reads/validates the actual address marker and execs the fixed reviewed
Dropbear in foreground with `-F -s -r/-D/-P`, explicit `IP:22222`, `-I90 -M300 -T3`.
No `-E` (unsupported), `-R`, generated key, password auth, forwarding, agent or
all-address listener. Key-only policy comes from the pinned build, not `-s` alone.
Use restricted public authorization and explicit finite non-login client commands;
do not assume listener TERM/session timeout kills arbitrary authenticated children.

Any boot/setup/hash failure returns to init/getty. The SSH entry without a successful
RAM marker sleeps idle rather than falling through to stock startup. Normal listener
exit can be respawned by init; no new service controller is used. An address change
on DHCP renewal can leave the listener bound to the old address: the hook updates
the marker but **does not kill sessions or auto-restart anything**. Parent must
qualify lease behavior or recover manually; do not call this network failover/soak
acceptance. The existing serial getty and U-Boot `init=/bin/sh` rescue policy remain.

## Private builder

`build.py` requires explicit `--original-flash`, `--private-inputs` and a **fresh**
`--private-output`. It only accepts the immutable measured8MiB original acquisition
hash in `public-pins.json`. ROOT alone is `[0x200000,0x300000)` (1MiB); live geometry
is a separate parent gate, not inferred from that offline map.

### Oct15 parent correction: preserve the actual live baseline

Live backups proved the Oct08 authorized ROOT password update is still present.
An acquisition-only build would revert it. Such earlier candidates are retained
but explicitly **DO NOT INSTALL**. Real parent composition now uses the additional
`--reviewed-live-flash PRIVATE_FRESH_BACKUP` option, never a guessed old snapshot.
It is now mandatory for every non-dummy composition; omission stops before inputs
are opened. Private paths refuse `..`, symlinks and file hardlink aliases; validated
inputs/parents are canonicalized before outside-Git containment checks. A separate
review found the former lexical-only `..` bypass using public fixtures; the actual
private c/d paths did not use that spelling and were separately checked canonical.

The immutable acquisition remains the code/metadata reference. The fresh protected
8MiB backup must be outside Git, regular0600 and under safe owned ancestors.
BOOT/MAC/KERNEL/USR must match the acquisition; existing ENV/JFFS bytes are preserved
without interpreting or resetting them. Both ROOT archives are audited before
extraction. Their path set, metadata, links and every file must match except the
already authorized `/etc/passwd` delta. Any other difference is HOLD. The candidate
preserves that live passwd byte-exact and changes only the documented startup/
new overlay. Whole composition outside ROOT is compared to the **fresh backup**.
The fresh input digest lives only in the private plan; mutation during build is
refused. Live-baseline source/fixture logic was independently reviewed; path-fix
regressions and exact-build admission remain separately recorded parent evidence.
Neither host checks nor that review approve any programming operation.

Inputs are a protected0700 directory containing exactly three regular0600 files:

* `hostkey`: parent's privately generated/reviewed Ed25519 Dropbear hostkey.
* `authorized_keys`: one `restrict ssh-ed25519 PUBLIC_BASE64 [comment]` line.
* `wpa.conf`: exactly the following fixed policy, with the private PSK injected by
  the parent. No other SSID, include, dynamic writer or arbitrary option is accepted:

```text
ctrl_interface=/tmp/bench-ssh/wpa
update_config=0
network={
ssid="VertexHotspot"
psk="PARENT_PRIVATE_VALUE_NOT_SUPPLIED_HERE"
key_mgmt=WPA-PSK
proto=RSN
pairwise=CCMP
group=CCMP
}
```

No real key/profile was created, read or supplied by this worker. Real input/output
directories must be outside Git. Parent must validate actual hostkey format/trust,
WPA encoding and safe private storage/retention; the builder does not execute a key
parser or listener. Credentials/leaf credential digests and factory account contents/
digests are never logged. All candidate images/trees contain immutable original
private factory account data, so **even dummy builds are PRIVATE, not public firmware**.
Protect their0700 envelopes; generated image/log files are0600. Never upload them.

One fakeroot session audits archive names/symlink topology **before** unsquashfs,
extracts fresh original/candidate trees, adds only the allowlisted overlay and changes
only inittab contents. It preserves original UID/GID, modes, hardlinks, symlink text,
device types/numbers and timestamps. No `-all-root`, forced owner/mode normalization,
original-source edits, sudo/install or firmware-binary execution. Original mixed IDs
0/985/65534 and all four SquashFS ID entries are retained. Directory timestamps
changed by additions are explicitly restored. Original extracted modes exist only
inside private0700 envelopes; top-level host trees are chmod0700 **after** the single
fakeroot verification session ends, without changing already verified image metadata.
Never repack those post-fakeroot host trees directly.

Packing: normal SquashFS4.0/XZ,128KiB blocks/dictionary, original mkfs timestamp,
one processor/reader, original exports/duplicates/fragments policy, no xattrs (original
has none), no BCJ/new filter. XZ headers are inspected and decoder/check requirements
must not exceed the stock shape. SOURCE_DATE_EPOCH/SQFS_CMDLINE are removed to avoid
silent timestamp clamps or command-side files. New files use deterministic original
mkfs time. A second extraction verifies every unchanged file's metadata and bytes,
hardlink topology, plus all additions/inittab. Only booleans/counts are reported for
private input/file comparisons. Images must fit1MiB including SquashFS padding; excess
is a **HOLD**, never a USR/config fallback. ROOT is padded to1MiB with0xff and a private
8MiB composition proves all non-ROOT bytes equal. Whole composed hash stays only in
the private ledger; public reports never publish a composed-image hash.

## Dummy-only offline reproduction

The worker validator uses literal public NOTFLASHABLE fixtures, not actual keys,
and adds a boot-blocking `/bench-ssh/NOTFLASHABLE` marker. It runs mocks and two
independent builds, compares full images privately for byte equality, and emits a
strict nonsecret report. It never opens a hardware device, touches an interface,
starts a host listener, executes ARM firmware, or runs any flash-writing command.

Existing tools only: Python3, fakeroot, squashfs-tools, tar, xz, readelf. No installs.
Use an owned workspace TMPDIR because host `/tmp` is full. In this worker's existing
private build envelope, a fresh validation output name is mandatory:

```sh
TMPDIR="$PWD/work/persistent-ssh01-20261014-host01/tmp" \
 PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 -B \
 evidence/delegated/stock-reference-20261013/persistent-ssh01/validate_offline.py \
 NEW_VALIDATION_NAME
```

Exact argv/timestamps/tool versions/public source hashes are retained in the owned
evidence directory; private per-build ledgers/logs/trees/images stay in the fresh
work envelope. Early failed validation attempts are preserved, not overwritten.
Synthetic fakeroot fixtures explicitly cover nonroot IDs, hardlinks, symlinks and
device metadata because the original ROOT has no device-node/hardlink examples.

Public Dropbear artifact/notices are copied unchanged from admitted release01;
all upstream notices remain. This proposes no workspace-wide license or authority
to redistribute vendor firmware, private images, credentials or photographs.

Parent gates and disposition: `docs/qualification-plans/stock-reference-20261013/
persistent-ssh01/WORKER_REPORT.md`. **Stop after this one offline unit; no automatic
flash, GUI integration, boot or hardware campaign follows.**
