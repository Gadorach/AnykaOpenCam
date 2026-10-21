# Nonsecret local rescue-code bundle; not a publication licence

- BusyBox 1.37.0 static is reused unchanged from the retained build. GPLv2-only
  text and original build config/applet list are included. Corresponding retained
  source: work/lightbulb-camera/implementation-20261008T205404Z/sources/busybox-1.37.0/busybox-1.37.0/.
  Build recipe: the same implementation directory's build_wifi_userland.py;
  toolchain: sources/toolchain-armv5-musl-202508/. No rebuild was performed here.
- Stock BusyBox identifies as 1.24.1; GPLv2-only. The included GPL text applies;
  exact vendor corresponding source/build configuration has NOT been established.
- Stock uClibc 0.9.33.2 loader/libraries are copied unchanged code-only from ROOT;
  LGPL-2.1 text is included as a reference, not a replacement for any component's
  original notices. Exact vendor corresponding source/options and complete
  per-file notices have NOT been acquired by this unit.
- Dropbear, LibTomCrypt, LibTomMath, musl and GCC notices are copied byte-exact
  from reviewed ssh-control02/release01/licenses/. That musl/GCC material also
  documents the retained ARMv5-musl toolchain family, not proof of every static
  BusyBox link input. The original static binary/source/build records remain.
- No stock WPA executable/libnl, camera application, configuration, private key,
  authorized key, WLAN profile or device identity is in this bundle.

This is an owner-authorized LOCAL, nonsecret lab artifact. Do not publish it or
claim complete redistribution/corresponding-source compliance merely because
licence texts are present. Preserve original licences; no workspace-wide licence
or new redistribution grant is asserted. Parent must finish licence/source
closure before any external redistribution.
