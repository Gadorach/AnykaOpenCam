# Isolated C HTTPS UI adoption candidate — WEB-03 / WEB-05

Parent review only. The canonical service, native launcher, dependencies, player,
framework, plans and existing ELFs are read-only. Only fixed static UI lookup,
headers and associated source/build tooling change. All media remains501; no
media-v1/producer/control/driver implementation is enabled. Old warm native HTTPS
qualification does not transfer to this image; G3/G8/product gates remain open.

## Small auditable source overlay

`prepare.py` refuses an existing snapshot and pins the actual canonical public
source/scripts/configs plus ten actual v3 player assets against its78-test receipt.
Canonical C SPDX notices remain intact. `snapshot/generated/web_service.c` differs
only by its static asset include and the old `/`/`app.js` public branch replacement
from `static_dispatch.inc`. Reversing those replacements reproduces canonical
service bytes. The exact diff is `snapshot/generated/canonical-to-ui.patch`.
All API/session/auth/privateFD/worker/filter/isolation code is unchanged.

`ui_assets.h` embeds bounded constant bytes and a fixed eleven-route table:
`/`, `/index.html`, and the nine named JS/CSS assets (app/core/contract/session/
wire/sample/player/frame-proof modules and player.css). Bodies are byte-identical
to the v3 assets. No path decoding, filesystem/directory API, query/userinfo,
untrusted filename, gzip or network dependency. Missing routes retain the original
401/501 or strict framing400 behavior. `/app.js` is no longer a public candidate
route; the original production ELF/router remains unchanged.

All raw bodies≤7168B, response including exact Mongoose status/header/Content-Length
framing≤8192B, checked before send and afterward. Largest actual reply7491B.
Header buffer512B unchanged. No raised heap128KiB single allocation/1MiB total,
receive/body/peer/time/CPU/source ceilings. Assets are constant image data, not a
per-peer cache. Additional ARM rodata/image cost needs actual2MiB native qualification.

Static CSP: default-none, self script/style/connect, blob media, self form-action,
base-none, frame-ancestors-none; no inline/eval/wildcards/CDN. Correct UTF-8 MIME,
no-store, nosniff, no-referrer, connection-close; no static Set-Cookie. API CSP and
all exact Host/Origin/CSRF/__Host-cookie/KDF behavior are canonical.

## Reproduce (existing tools only, fresh outputs)

```sh
python3 -B apps/auth-web/ui-candidate/prepare.py
python3 -B apps/auth-web/ui-candidate/build.py runNEW --toolchain /path/to/pinned/existing/toolchain
python3 -B apps/auth-web/ui-candidate/availability.py runNEW hostAvailabilityNEW
python3 -B apps/auth-web/ui-candidate/asset_tls_test.py runNEW hostAssetsNEW
python3 -B apps/auth-web/ui-candidate/finalize.py runNEW hostAvailabilityNEW hostAssetsNEW
```

Do not rerun prepare over the preserved snapshot. Builds/tests resolve only under
`local/build/auth-web/ui-candidate/`; fresh directories refuse overwrites.
Builder executes the pinned copied canonical builder with a small explicit source/
vendor-path overlay, retaining full HOST+ARM TLS libraries, compound sources,
fault variants, actual old-semantics HOST regressions, availability/sandbox/media
tests and compiler-derived ARM filter test. No vendor tree duplication; libraries
compile from canonical pinned MbedTLS3.6.7/Mongoose7.23 read-only source. New inputs
are copied/pinned; header/compiler/link/runtime closure is in the build manifest.

`availability.py` reuses the exact canonical491 TLS/real600k campaign without
changing protocol fixtures, counts/cost/time/limits. `asset_tls_test.py` reuses its
exact PUBLICDUMMY fixture/helper prefix for separate real TLS static checks:
exact asset bytes/CSP/MIME/full-response counts, no token/gzip, strict Host/query/
escape/userinfo/traversal/unknown routes, real600k login, status/capabilities,
challenge replay/logout/old-cookie refusal and continued public assets.
No test mock KDF or reduced iterations. Known/unknown account uniformity, idle
expiry, owner revoke/replace, race/abort/discard/drain/failclosed and same-instance
36-cycle reuse remain covered by491. Only fresh HOSTPUBLICDUMMY fixtures are
generated; no operator/camera/private configuration is read or created. Credentials,
cookie/CSRF/key bodies and material hashes are not in public manifests/logs.

Finite180s campaign budgets and actual owned loopback service/child teardown remain.
No proc/fuser, target/UART/SD/ROM/photo/footage, install, Git, publish or parent
tab/PID actions. No browser is accessed here; parent performs its own actual
HTTPS/browser test. Ten-module parallel loading with the unchanged two-peer/one-
handshake policy is a browser qualification dependency, not proven by serial TLS
asset checks. Host exit RSS includes pre-exec inherited accounting; it does not
prove native resident/locked/combined-load headroom. Native mandatory2MiB limit,
UID1000/caps0/core0/nodump/seccomp/privateFDv2 remain unchanged.

## Pins, licences and parent adoption

Snapshot/build/handoff manifests contain exact SHA256 input/source/asset/ELF pins,
commands/timestamps/compiler/header/library/runtime closure and copied upstream
licences. The service C/helpers preserve GPL-2.0-only notices; Mongoose/Mbed/runtime
licences remain authoritative. Player asset copyright/licence was not invented:
no redistribution/workspace-wide grant is asserted. Candidate and HOSTPUBLICDUMMY
fixtures remain local; no publication authorization follows.

Stripped ARM strict child remains byte-exact8bb3d596... . New service hash requires
a newly matched launcher: parent can use the existing canonical launcher builder's
`--service-run` input after reviewing this candidate. Never override the old runtime
service pin or use an old launcher as if it matched. No launcher is built/adopted here.

**One next parent native step:** fresh ownership/runtime/image/recovery boundary,
then exact candidate plus matched launcher at unchanged2MiB, a bounded HTTPS UI
asset/browser/login/session/status/logout/replay and owned exit campaign. Leave
media501/RTSPNOAUTH and all native/browser-media/product gates open.
