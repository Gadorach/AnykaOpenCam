# RTSP startup material — independent ordinary source review

2026-10-23 · SEC-01/03/05, STREAM-07 · **Narrow conditional SOURCE/API sufficiency.**
No correction-required defect found within the trusted, serialized, disjoint-buffer
contract. This is construction of actual startup material, not runtime acceptance,
an implemented provisioning process, or authorization to use credentials/devices.

## Scope and evidence

Read all 16 selected inputs, including the complete candidate, actual backend
prefix checks, verifier material/engine, Digest helper, registry/graph validators
and selected headers. Also inspected the request parser and its header, backend,
wire-limit and access-policy headers, and relevant Mbed TLS context-destruction/
zeroization implementations. Original inputs were not edited.

Fresh read/hash checks matched **16/16** selected byte-count/SHA256 pins below.
`SOURCE_PINS.json` itself hashes to
`4f352b12e63d08325e77765528674cde0ddc9266adcba070791e119e3c7f41a4`.
The pinned build report records `HOST_ARM_COMPILE_REL_PASS_NOT_RUN`,
2026-10-23T03:38:19.438860+00:00–03:38:23.342828+00:00:

- **20 C compilations:** ten each HOST/retained ARM; all recorded exits 0.
  Actual constructor, existing Digest/wire/graph/registry/parser and four vendored
  crypto units; explicit RTSP-only crypto config. ARM specifies ARMv5TE, ARM mode,
  soft float. Existing `rtsp_input.c` alone has the recorded warning exceptions.
- **Six further commands:** one `-nostdlib -r` combination, one `nm -u`, one
  `readelf -h -A` per architecture; all recorded exits 0. All **26/26 public log
  pins** freshly match. Saved ELF logs identify HOST ELF64 x86-64 REL and ARM
  ELF32 little-endian EABI5/v5TE REL, not executable applications. Saved undefined
  lists retain libc/compiler support symbols, not missing camera/Digest/Mbed TLS
  entry points. This is not final application linkage or deployment evidence.
- All ten report source pins freshly match. Selected compilation-header pins
  and the eight additional correctness-context source/header pins match report
  dependency entries; backend C and engine C are consumer context, not newly
  compiled units. No full compiler/dependency-closure or binary requalification
  is claimed; artifact hashes are cited metadata, not independently rehashed.

No compiler/build/tool recipe, candidate/helper/library, generated object/program,
test/fixture/generator or hardware operation was run. Only public source/metadata/
log inspection and read-only hashing were performed; no actual/private credential,
capture or target-state material was read, generated or hashed. No adversarial,
fuzz or mutation testing was performed.

## Ordinary findings by location

1. **Whole output and failure clearing:** `startup_material.c:7–12,30–49,76–99`
   uses the actual little-endian `rv_material`, asserting 1536 bytes and offsets
   348/476. A nonnull output is crypto-wiped over its entire size before any input
   validation. All pre-crypto rejection paths therefore leave cleared storage;
   helper failure wipes the entire record again. After helper success only
   nonfailing bounded field stores/copies remain. No uninitialized whole-struct
   copy populates the record: unused username/realm/HA1/URI tails and ABI tail
   padding start cleared, and no caller padding is imported. The explicit clear
   API uses existing Mbed TLS zeroization, not an ordinary removable destructor
   `memset`. Null output returns invalid without dereferencing it.

2. **Bounds and trusted ownership:** `startup_material.h:6–21` expressly requires
   valid borrowed storage, immutable same-bootstrap values, serialized use and a
   fresh output disjoint from every input. These conditions are essential because
   clearing precedes validation; hostile pointers/overlaps are not supported.
   `startup_material.c:14–28,40–64` validates canonical bounded zero-padded IDs,
   nonzero generations, algorithm/TTL and graph counts before graph traversal.
   `backend.c:26–57` establishes valid graph references and unique profiles; exact
   ID selection then requires H.264 1280×720 and a canonical sensor ID. Existing
   wire-limit initialization validates all three explicit rate pairs and window
   (1–1024 attempts, window 1–60000 ms), without adopting running counters/defaults.
   Digest validation limits names to 1–64 bytes and password to 1–128 bytes before
   the output length casts/copies. No allocation or input mutation is introduced.

3. **Actual backend bindings, not fabricated readiness:**
   `startup_material.c:40–56,81–94` matches the material fields consumed by
   `qc07f-owned-runtime/server/candidate_auth.c:23–31,47–105` and
   `isolated-verifier/verifier.h:28–36`: ABI/size, explicit algorithm, epoch, boot
   and credential generations, every TTL/rate field, device/account/sensor/profile,
   RTSP domain and fixed native numeric account **1**. HOST fixture/native-profile
   mismatch is rejected. `engine.c:49–71` consumes those fields into real Digest
   credentials and RTSP viewer/profile-LIVE policy. No UI/admin or PUBLICDUMMY
   identity is substituted. The constructor intentionally does not verify image
   bytes/FDs, callbacks, root/process hardening, entropy or native/provider readiness;
   it does not promise that an otherwise incomplete config will start a backend.

4. **Exact configured routing:** `startup_material.c:66–72,97–98` bounds NUL
   searches to both 513-byte registry arrays before existing string validation.
   `rtsp_input.c:21–42` reconstructs a cleared registry from distinct, bounded
   absolute RTSP URIs; `rtsp_request.c:74–136` excludes URI-authority userinfo and
   malformed targets. The material receives these exact URI bytes, not uninitialized
   caller tails, guessed routes, decoding or canonicalized aliases. `engine.c:54,
   125–129` uses the same registry and exact target matching. Using the same frozen
   registry for serving remains the trusted caller's duty, not an equality proof
   obtainable from `candidate_config`, which does not contain routes.

5. **Real crypto, errors and secret lifetime:** `startup_material.c:74–86` calls
   existing `camera_rtsp_digest_ha1` with the explicit selected algorithm and the
   borrowed password; it copies only username/realm and HA1 into output, never
   plaintext password. `rtsp_digest.c:13–20,54–79` restricts username/realm text;
   password is deliberately bounded opaque octets, not a character-policy input.
   Mbed TLS start/update/finish errors propagate through the helper; context frees
   run on both success and failure and wipe their state. The constructor maps
   invalid credentials to `CAMERA_INVALID`, other helper failures to
   `CAMERA_IO_ERROR`, and never returns successful partial material. MD5 uses 16
   bytes with a cleared 16-byte HA1 tail; SHA256 uses 32. No downgrade/retry exists.
   Mbed TLS `md5.c:32–39`, `sha256.c:233–240`, and
   `platform_util.c:88–136` support the explicit cleanup path. This source review
   does not establish elimination of every compiler/register/stack copy. Borrowed
   password copies remain caller-owned and must be wiped; output HA1 remains
   credential-equivalent SECRET until explicitly cleared after a separate handoff.

6. **Process/domain separation:** `startup_material.h:6–21`, README and contract
   consistently restrict use to a separate trusted provisioning context before
   media startup. `rtsp_crypto_config.h:4–8` enables only RTSP SHA256/explicit legacy
   MD5, not HTTPS/TLS MD5. This candidate has no ingress, writer, IO, clock, random
   generation, fork, verifier/media init or live-update operation. Recorded builds
   form separate relocatable objects, not a media/HTTPS integration. Future linker
   and bootstrap wiring must preserve that separation; this C API cannot enforce
   process identity or prove supplied RTSP credentials were independently chosen.

## Exact selected pins

Paths are workspace-relative. Every row matched both recorded bytes and SHA256.

| Path | Bytes | SHA256 |
| --- | ---: | --- |
| `apps/rtsp-auth/rtsp-startup-material/startup_material.c` | 5019 | `5eb23e473750c0b3ae0daecd472c2091ec1a6e8287001c65b9d1634fdd3aceae` |
| `apps/rtsp-auth/rtsp-startup-material/startup_material.h` | 1779 | `76fb100e06c6b7508b6011400bf35734d273615fd26a3aa8cc57cd2150630206` |
| `apps/rtsp-auth/rtsp-startup-material/README.md` | 2066 | `33d3062824c296a56c3acf635851d49b58e8d5d4591188e46d081befca8743c9` |
| `docs/qualification-plans/rtsp-startup-material-20261023/CONTRACT.md` | 2468 | `36b5754f0302ee72c37d0f2afc58d9259769df22a0bb4f60df19939bd555e1e6` |
| `docs/qualification-plans/rtsp-startup-material-20261023/runs/build01/REPORT.json` | 83833 | `c7e06a428e0306a4daf6dd0a4ff469bdb4355c22765710c322b28f6ffe4ec6e4` |
| `apps/rtsp-auth/qc07f-owned-runtime/server/candidate_auth.h` | 6001 | `94b6ed560677b7db4119eded9726ab01085c2e0eb392bdad29e961ebcd2ed643` |
| `apps/rtsp-auth/qc07f-owned-runtime/server/candidate_auth.c` | 17425 | `cb27b35e2d8730b8d19f231d339e06cbe4c97fbe0aca7826fe3469d8479825a7` |
| `apps/rtsp-auth/isolated-verifier/verifier.h` | 6234 | `3f4a97a12ed9acafe678e720b473c798c75b170648e6a6de076a1017ff0ad900` |
| `apps/rtsp-auth/isolated-verifier/engine.c` | 10852 | `cf298b8096c66859806c6e0c0b449592ac4081f05ad814f21fee35e97262c396` |
| `framework/src/rtsp_digest.c` | 12142 | `24ac70a0e3a4d76e2b651c4ad7570b4e89cd6645b62a5291408fd3b00a14e80f` |
| `framework/src/rtsp_digest_wire.c` | 13918 | `0ecc44fe90618eca56a43d6a95df23ef1e236aac07413b5abf016d1623a7abc9` |
| `framework/src/rtsp_input.c` | 9009 | `f827dff40c7874650872f6a506c28695ef9af90ae7f3dbbe93b7e4fc0976491f` |
| `framework/src/backend.c` | 4344 | `cc5462d995479578c83d5acb38164d2c8f3eae8a1adaf0c6e5a7fe7f929ff9f3` |
| `framework/include/camera/rtsp_digest.h` | 7432 | `26294a9e67f663eb3bfc903c48a63396298a300caebd2ea15dcffa5b87745aae` |
| `framework/include/camera/rtsp_input.h` | 2591 | `bb39c927b8d3da0754da5ed759e420987b700e1e1cfd11d9b78192289f80cac9` |
| `apps/rtsp-auth/rtsp_crypto_config.h` | 379 | `7cfb2b4eacf9379bd945a2faa9a95bc228c131bdf956a07e3953fdba6d7f818f` |

## Disposition and remaining boundaries

Conditionally sufficient **only** as the reviewed native-account1, H.264720p,
RTSP-only startup-material constructor under the stated trusted caller contract.
No correction is requested for this bounded source increment. Source execution,
secret ingress, private FD writer/handoff, storage, provisioning authentication/
authorization, process hardening, bootstrap ordering/external owner/recovery/
backing/main, durable generation authority, live credential change and native/
combined-load qualification remain absent or unqualified. Compilation is cited,
not rerun; no runtime GO, adoption or future work queue follows.

Both native HOLDs remain untouched. Historical Oct22 held U-Boot is not a current
target-state assertion; this review obtained no new target observation. Only this
fresh review directory was written; candidate, central docs, old evidence and Git/
index were left unchanged. Parent retains adoption and isolated-index curation.
