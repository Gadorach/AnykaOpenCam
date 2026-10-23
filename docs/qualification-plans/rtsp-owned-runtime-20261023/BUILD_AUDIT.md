# Compile-only recipe audit

Read all200 lines of the fresh `apps/rtsp-auth/qc07f-owned-runtime/build.py` before
first invocation. It imports only Python standard-library bookkeeping, copies
known public source/headers/notices, checks retained public/toolchain/vendor pins,
and writes exclusive fresh run directories under this qualification directory.
No old build/overlay/helper import or invocation. No credentials/private inputs.

Subprocess allowlist as authored: selected cc `--version`/`-dumpmachine`/`-c`, ar
`rcsD`, cc `-nostdlib -r`, nm symbol tables and readelf header/attributes/sections/
relocations. No executable link/run, default/help app run, test, listener, generator,
network/device/process control, kernel/image build or Git/index operation. Compile
inputs are actual native/custody/watchdog/server/auth/common dependencies, no stub.
HOST uses strict native auth defines just like ARM (no IS_HOST_PUBLIC_ONLY).
Inherited warning handling is limited to existing indentation diagnostics and
rtsp_input's inherited nonnull diagnostic; no guard/auth/ownership suppression.

Fixed original 2s/10s/5s,1MiB hub,128KiB/40MiB/CMA/MEMLOCK/pins/policy are unchanged.
Compiler/tool timestamps are build evidence, never admission clocks. Fresh failed
compile outputs are retained; ordinary bounded corrections are permitted, not
adversarial/mutation tests. This is author inspection, not independent acceptance.
