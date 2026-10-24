# Local offline recipe and retained-source notices

`build.py` is GPL-2.0-only; its small format readers/layout method are adapted from
the project's `apps/qualification/encoder-query77-build/{build,package,package02}.py`
(read as text, never imported or invoked). The kernel is the retained pinned
Linux6.18.55 checkout plus existing project patches and the separately reviewed
PTZ OFF-retention source. Its original COPYING, LICENSES and per-file SPDX notices
are preserved unchanged in the fresh full source tree; this notice grants no
global license and does not replace upstream licenses.

Initramfs is a derivative of the accepted local qualification source package,
removing only hgics.ko and bin/rtspd. Remaining monitor/BusyBox/WPA/DHCP binaries
retain their original provenance and source-package/license obligations; no opaque
binary is executed. Generated images, archives, binaries and retained stock material
are ignored LOCAL evidence only, not Git/publication/redistribution candidates.
No remote contact, download, account, host policy or hardware operation is needed.
