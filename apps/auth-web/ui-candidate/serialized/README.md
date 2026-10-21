# Serialized local UI entry candidate

WEB-03/05 parent review correction only. V1 source, runUI01 images/receipts and
exact v3 player modules remain read-only; preserved byte-exact under
`evidence/delegated/20261012-native-ui/v1-snapshot/`. No canonical service/player,
peer/handshake/backlog/heap/body/response/memlock/time/KDF limit is modified.

## Loader contract

Derived HTML replaces the eager CSS + module script tags with one external deferred
`bootstrap.js`. The login fieldset starts disabled in HTML, not just after JS begins;
unsupported/failed/timed-out entry cannot accidentally submit a form without app
handlers. Only that fixed HTML and a new bootstrap differ; copied v3 JS/CSS bytes
remain identical. No media/physical-control/credential behavior is changed.

The loader waits for stylesheet load, then waits for one modulepreload load at a
time in this exact dependency order:

1. `/player.css` (stylesheet)
2. `/contract.mjs`
3. `/session.mjs`
4. `/wire.mjs`
5. `/sample.mjs`
6. `/player.mjs`
7. `/core.mjs`
8. `/frame-proof.mjs`
9. `/app.mjs`
10. external module script `/app.mjs`, reusing the document module map

Every static import/export dependency is already loaded before its parent; no
dynamic import graphs, huge concatenation, URL queries/cachebusting, inline/eval/
blob scripts, fetch shims, service workers or persistent secret cache. Per-step
deadline8s; CSS/module/script error or pagehide is terminal with no automatic retry
and no later requests/unlock. Late events cannot advance the disposed/failed loader.
`modulepreload` plus module-script support are required explicitly; absence displays
unsupported and keeps login disabled. This support check is **not browser-family
qualification**. Login unlocks only after the module script load event, after app
event handlers are bound. App/API initialization begins only after the complete
dependency graph; bootstrap never reads/logs CSRF, cookies or credentials.

HTML/root + bootstrap initial document fetching is not controlled by modulepreload;
the bounded claim is the CSS/dependency loader phase, not every browser-generated
request (favicon, developer tools, extensions etc.). V3 API code remains byte-exact,
including its later status/capabilities Promise.all. This loader does **not**
serialize or alter authenticated API races; their behavior with actual TLS admission
is a separate full browser/native qualification dependency. Nothing is labelled
live or all-presented by this loader, and no footage is read or embedded.

## Reproduce with existing tools and fresh outputs

```sh
python3 -B apps/auth-web/ui-candidate/serialized/prepare.py
node apps/auth-web/ui-candidate/serialized/run_model.mjs evidence/delegated/20261012-native-ui/serialized02/FRESH-model
python3 -B apps/auth-web/ui-candidate/serialized/build.py runSerializedNEW --toolchain /path/to/pinned/existing/toolchain
python3 -B apps/auth-web/ui-candidate/serialized/availability.py runSerializedNEW hostSerializedAvailabilityNEW
python3 -B apps/auth-web/ui-candidate/serialized/asset_tls_test.py runSerializedNEW hostSerializedAssetsNEW
python3 -B apps/auth-web/ui-candidate/serialized/finalize.py runSerializedNEW hostSerializedAvailabilityNEW hostSerializedAssetsNEW
```

Prepare refuses the existing snapshot. Wrappers pin/reuse the reviewed v1 recipes;
only isolated snapshot path, added asset/derived HTML provenance and expected
additional asset-check counts change. Canonical491 protocol/cost/negative/race
campaign remains byte-exact. Full HOST/ARM libraries use canonical read-only
MbedTLS3.6.7/Mongoose7.23 pins; compiler/header/link/runtime/licence closure and all
original security/process tests remain. Child ARM must remain byte-exact8bb3... .
Do not rebuild a launcher with an old runtime override; parent reviews the new
service image and generates matched pins afterward. Native mandatory2MiB unchanged.

`loader_test.mjs` checks the actual import/export edges and models one network
request maximum, dependency-cache reuse, all load failures, unsupported detection,
deadline/late event, pagehide and script error. It executes the **actual unchanged
app module** in mocks and checks UI API starts only after graph readiness. These
are19 deterministic model tests, not actual browser/TLS/security/headroom proof.

`http_fixture.mjs` is optional finite owned127.0.0.1 PUBLICDUMMY HTTP, not an
unauthenticated product bridge: fixed public assets, anonymous dummy session only,
two connections/backlog2,5s HTTP bounds/180s total deadline. No certificate or auth
security equivalence, persistent account, video or camera access. A new own tab was
attempted; tool reported browser.disconnected. No tab or parent browser was read;
fixture closed normally. TLS browser was not attempted without explicitly scoped
legitimate trust; no self-signed warning bypass or global trust change occurred.

Current evidence: `evidence/delegated/20261012-native-ui/serialized02/`. All twelve
static routes (root/index alias + ten other files) retain body≤7168/full reply≤8192
and exact reviewed CSP/MIME/no-store/nosniff/no-referrer. Gzip is not required. No
headers/API/IPC/isolation change beyond reviewed fixed static assets. Licence
notices remain authoritative; no player redistribution grant is invented.

**One next parent step:** protected exact new candidate/matched-launcher at2MiB
native TLS UI campaign, including actual browser stylesheet/module preload order,
login/status/capabilities/logout and observed owned exits. Media remains501,
RTSPNOAUTH and all G3/G8/media/product gates remain open.
