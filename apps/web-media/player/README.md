# WEB-05 / WEB-10 standalone browser candidate

**Source + deterministic host fixtures only. Not deployed, browser-renderer-tested,
mobile-qualified, hardware-tested or product-accepted.** Current production media
remains 501 and RTSP remains independently unauthenticated lab evidence. No service
assets, C sources, shared frameworks, target state or central plans are changed.

## Current API contract (read-only inspection)

`apps/auth-web/service/web_service.c` is authoritative, particularly lines230–459.
There are **no** `/api/session/login` or `/api/session/logout` routes:

| Request | Contract |
| --- | --- |
| GET `/api/session` | schema1; authenticated boolean; in-memory CSRF64 lowercase hex; server sets/rotates cookie |
| POST `/api/login` | raw password UTF-8 bytes12–128, `application/octet-stream`, `X-Account` ASCII stable ID1–63, `X-CSRF-Token`; one-use challenge, cookie/CSRF rotation |
| POST `/api/logout` | empty body, CSRF; schema1 logged_out receipt; cookie revocation |
| GET `/api/status` | browser_media unsupported_unqualified, settings unbound/revision null, product_accepted false |
| GET `/api/capabilities` | browser_video/control/settings_commit false, producer_binding unqualified |
| GET `/api/media/live` and `/api/media/snapshot` | authenticated admission only, currently501; never used as fallback |

Exact Host and POST Origin must match service configuration. Browser-managed
Origin/Host must not be forged in JS. Deploy at the service's **dedicated HTTPS
origin**, without unrelated cookies: server accepts only the dedicated cookie.
Cookie is `__Host-camera`, Secure, HttpOnly, SameSite=Strict, Path=/; JS neither
reads nor sets it. Requests use same-origin credentials/mode, no-store, error on
redirect, fixed paths, no URL secrets. CSRF is private class state, not returned
to UI/DOM/logs/storage/channel. No Authorization, password logging, localStorage,
URL userinfo, eval, untrusted HTML insertion, external fetches or CDN dependencies.
Request failures are generic/scalar; no protected response JSON is dumped to UI.

V3 corrects the browser WebIDL fetch receiver in both Session and Player:
stored native/injected fetch functions are invoked with `.call(globalThis, ...)`,
never the class instance as receiver. The app may still pass raw window.fetch;
no wrappers, request options, routes, Origin/CSRF/cookie or cancellation semantics
change. Strict receiver mocks cover explicit injection and raw global defaults
for refresh/login/logout/media; real app mocks also reject incorrect receivers.
Parent observed v2's actual Chrome152/Electron44 `Illegal invocation`; that failure
is retained, not dismissed as policy or erased by the earlier70-test success.
V3 actual browser rerender remains a parent review dependency, not a host claim.

Login fetch deadline36s accommodates existing35s response bound without altering
native HTTP/CPU/KDF limits. Other API operations have8s deadlines and4096B response
limits. Password input clears immediately and the owned encoded byte array is
wiped on every outcome. JavaScript strings/browser networking internals cannot be
reliably zeroized; this is not a secure-memory guarantee. No sample login is a
default account. Tests use only `PUBLICDUMMY-user` / `PUBLICDUMMY-nondefault-01`.

## Proposed media contract — NOT IMPLEMENTED on camera

Current schema1 capability response cannot activate the player. Proposed explicit
extension, to be independently reviewed by parent/backend owner:

```json
{"schema":1,"browser_video":true,"browser_player_protocol":"camera-fmp4-v1",
 "media_resources":[{"camera_id":"PUBLICDUMMY-QC1","profile_id":"PUBLICDUMMY-main",
 "resource_id":"PUBLICDUMMY-resource","qualified":true,
 "metadata_clock":"source-monotonic-age","codec":"avc1.42e01f"}]}
```

Example IDs are **not actual camera IDs or qualifications**. Backend must provide
exact stable graph resource IDs and qualified physical-camera/profile bindings.
Only the single QC physical sensor is currently qualified; profiles are not extra
sensors. Front-end does not invent sensor tabs or trust arbitrary endpoint URLs.
At most16 resources, unique camera/profile pairs **and globally unique resource
IDs**, ASCII IDs1–63 and explicit AVC codec accepted. Start resolves the exact
current camera/profile/resource tuple from a current-revision selected option,
never the first matching resource ID. Catalogue refresh invalidates old options
before waiting for the API, and late older results cannot replace current bindings.
Missing metadata contract or qualification leaves Start disabled.

Proposed GET `/api/media/v1/live/{resource_id}` is same-origin authenticated TLS,
content-type exactly `application/vnd.camera.fmp4-v1`, no query/token. This is a
framed encoded stream, **not** a bare video/mp4 response and not WebSocket/SSE:

1. Four-byte unsigned big-endian UTF-8 JSON metadata length1–1024.
2. Exactly that many JSON bytes, followed by exactly `bytes` fMP4 bytes.
3. Repeat; clean EOF alone is a fault. Explicit `end` required.

Every record has `v:1`, exact `resource_id`, stable ASCII `generation`, `type`,
and integer `bytes`0–2097152. Generations bind boot/source/profile/codec changes;
record metadata and payload must belong to one authorized source retirement.

* `init`: first/only init,1–8192B, exact `codec`, integer `clock_us` sampled from
  trusted device monotonic clock **while processing this fresh media GET**, same
  generation as capture. Live fMP4 init with accurate SPS/PPS, no nominal cadence.
* `media`: one whole muxed fragment per metadata record, first IDR true; strict
  increasing source `pts_us`; `media_time_us` starts0 and matches actual normalized
  MSE PTS, constant source-PTS offset, non-overlapping presentation intervals;
  measured `duration_us`1–2000000, not25fps defaults. `capture_mono_us` is actual
  capture-associated monotonic time in the same clock domain as init.clock_us.
  Required `age_upper_ms`0–3000 is backend's conservative age bound; required
  `measured_fps` is actual measured value or null. No invented wall-clock timestamp.
* `fault`, `revoke`, `end`: zero payload. Revoke ends local authority. End clears
  video immediately, not a promise that all previously decoded frames rendered.

Client also derives a conservative capture-age bound from browser request start
and the init monotonic sample: `now-requestStart-(captureMono-initClock)/1000`.
Request start precedes the fresh server sample, so this includes network/append
delay without assuming synchronized wall clocks. Future/old clock mappings fail
closed. Clock association, calibration/drift bound, boot generation and actual
capture timestamp reliability are **backend qualification dependencies**; server
age-at-send alone is insufficient. Neither can be fabricated from nominal FPS or
the time the browser received a chunk. Safe-integer microseconds are required.
Generation mismatch aborts and clears all buffers before a manual fresh
GET/init/IDR; never append a new generation to the old SourceBuffer.

Server must independently verify session/expiry/revocation/owner/health/lease
before each effect/chunk, cap sending/retention, and release producer/CPU resources
on cancellation. JS abort is **not proof** of native release. Existing bounded
shared init/header/8192B spans are reused; this proposal does not authorize native
source changes, lifting heap ceilings or full-AU TLS copies. A wire envelope can
be emitted as small headers plus existing borrowed fragment spans.

## Lifecycle, boundedness and truthful display

One reader and one awaited SourceBuffer operation; no speculative reads or append
queue. Fetch chunks over256KiB fail closed. fMP4 payload passes as borrowed
subarray views no larger than8192B, including split NAL/box payloads. Only4B framing
and1024B metadata are coalesced; no full-AU concatenation, re-encoding or tint.

Encoded media accounting caps4MiB plus at most64 outstanding fragment ledger
entries, and MSE time ranges cap6s/8 ranges. Retain2s of played history; credit
bytes only after completed removal of complete records. Caps abort rather than
grow or silently skip to an unqualified frame. The accounting is a conservative
**client admission bound**, not a measured browser heap/GPU/demux RSS bound; MSE
may retain decode dependencies internally. Browser memory still needs measurement.
Read/source-open/header/append/remove operations have8s deadlines. Metadata
history caps64 scalar entries; missing display lookup is waiting, not live.

Stop, logout, hide, pagehide/dispose, profile/camera switch,401, in-stream revoke,
source fault, truncation, quota errors and generation discontinuity abort/cancel,
clear visible video, revoke blob URLs and clear metadata. Epoch guards prevent
late fetch/session/append completions from reinstalling authority or tearing down
a replacement source. Source retry is manual. Visibility resume only rechecks the
session; it never automatically resumes media. BroadcastChannel contains only
literal logout/revoked notifications for same-origin tabs; backend is authoritative
across users/clients. No lab RTSP, JPEG bridge or static-video fallback exists.

`live` requires a current-source requestVideoFrameCallback matching a bounded
trusted PTS interval with fresh **presented-frame** metadata, not newest-appended
metadata. Fresh accepted media may render while explicitly labelled waiting so
compositor callbacks are not blocked by display:none; this is not live proof.
V2 registers no callback against idle/pre-Start video. `FrameProof` owns at most
one current-source callback and one freshness timer; cancellation tickets plus
source epoch/blob checks make already-queued obsolete callbacks complete no-ops
(including no hiding, relabelling or rearming). Missing frame lookup/current
waiting events leave only fresh current media renderable and labelled waiting,
so later compositor callbacks are not stranded by display:none. The250ms monitor
can rearm only an existing fresh current source, never invent a live observation.
Stop/replacement/hide/dispose cancel the callback; visible resume does not restart
media. These lifecycle properties are app-module mocks, not browser compositor proof.
A250ms monitor expires held frames after3s. With no callback support,
display freshness remains unqualified/waiting and video stays hidden. Waiting,
reconnecting, paused, unsupported_unqualified, session_expired, source_fault and
ended are distinct; a previous frame is never deliberately retained as live.
No all-frames-presented, minimum FPS,15fps/headroom or mobile-family claim follows.
Horizontal **and** vertical preview corrections remain `scale(-1,-1)`; no CPU
pixel manipulation. Backend must later document orientation ownership to avoid
double correction; masking/physical-control coordinate transforms are not qualified.

Physical PTZ/light/audio controls always disabled with an explanation. SEC-09
requires desired/applied revisions, resource binding, operation receipts and
recoverable/idempotent results before controls may be enabled. This candidate
implements no transaction engine or physical command, and never equates an
accepted request, link loss, missing SD or network reboot with applied success.

## Assets and deployment constraints

Local external JS modules + CSS only. Required response CSP:

```text
default-src 'none'; script-src 'self'; style-src 'self'; connect-src 'self';
media-src blob:; form-action 'self'; base-uri 'none'; frame-ancestors 'none'
```

Also no-store, nosniff, no-referrer and correct HTML/JS/CSS types. Existing service
CSP lacks style-src/media-src and routes only `/`/`/app.js`; these files are **not
drop-in service assets**. V1's16548B `core.mjs` is preserved under the evidence
snapshot. V2 preserves that public facade/API but splits Session, contracts,
reader/event/buffer operations, sample validation and Player into external modules.
`frame-proof.mjs` is the app's separate compositor lifecycle owner. The explicit
allowlist is `assets.mjs`: ten browser assets, each at most7168B, checked by the
runner. Each body leaves at least1024B for response framing/headers under
the unchanged8192B native full-reply budget. No gzip serving is required or claimed.
Native adoption still must review fixed routes/MIME/header sizes/no-store/CSP and
total connection/resource costs; local size checks alone do not prove native fit.
**Do not raise that budget.** No framework, bundler, installer, cloud or license
grant was introduced. Exact raw/gzip sizes and SHA256 pins are in manifests.

## Reproduce without installing tools

From workspace root, use fresh, never-reused output names:

```sh
node apps/web-media/player/run-tests.mjs local/validation/browser-player/FRESH-fixture
node apps/web-media/player/fixture-server.mjs local/validation/browser-player/FRESH-ui
node apps/web-media/player/loopback-test.mjs local/validation/browser-player/FRESH-ui
```

The Node fixture listens only127.0.0.1 on an owned ephemeral port,4 connections,
5s HTTP request/header bounds and180s total deadline. It serves only these local
assets and PUBLICDUMMY API responses: no camera access, footage, TLS/cookie
security equivalence or media implementation. It refuses overwriting the output
directory. No protected values are written. Browser UI validation must use a
new explicitly owned tab and the fresh URL, then close that tab and fixture.

The v1 browser-tool attempt was blocked: `browser.disconnected`, no connected
desktop browser, zero fixture requests. No tab/screenshot was obtained or parent
tab accessed. Fixture closed normally after exact owned SIGTERM. Tests establish
mock/API lifecycle behavior only. A separate finite PUBLICDUMMY loopback campaign
passed9 HTTP/API/MIME/CSP scalar checks; it did not execute/render the UI or MSE.
Keyboard/touch/accessibility rendering, real
Fetch-to-MSE parsing, browser memory, multiple real users/tabs, source reboot,
network switch, SD removal, desktop/mobile families and on-camera service remain
**NOT qualified**. V2 adds real app-module DOM/Fetch/MSE/callback mocks and a
separate split-asset loopback check; no parent browser tab is accessed here.
V1 source/evidence remains byte-exact under
`evidence/delegated/20261012-browser-player/v1-snapshot/`.
Three new parent-review regressions failed against v1 and are retained under
`evidence/delegated/20261012-browser-player/v2-review/`.
V2 source/evidence is preserved byte-exact under `v2-snapshot/`; v3 receiver
regressions, parent browser failure and fresh results are under `v3-review/`.
The runner reuses historical contract input hashes from that snapshot instead
of rereading other lanes during this scoped correction; these are not new native
source measurements. See `v3-review/REPORT.md` for current counts/pins; earlier
reports stay historical and are not overwritten.
