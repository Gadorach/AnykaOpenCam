# Existing app build — static artifact-role confirmation only

2026-10-22; PLAT-01, RP-08/09, SEC-05/11, STREAM-04/07. Parent selected one
read-only inspection of the existing `local/build/rtsp-app/build02/` outputs.
`python3 -B docs/qualification-plans/rtsp-artifact-roles-20261022/inspect01.py`
returned normally. Fresh exclusive receipt: `RESULT01.json`; its inspection source
pin is recorded there. No application, test, verifier, listener, compiler, builder,
generator or camera program was launched. This is not new runtime qualification.

## Observed static separation

| Existing artifact | Observed ELF/object role | Release disposition |
| --- | --- | --- |
| `libcamera-rtsp-integration-host.a` |14 ELF64 LE x86-64 relocatable members|Strict integration compilation, not a camera library|
| `libcamera-rtsp-integration-arm.a` |14 ELF32 LE ARM EABI5 relocatable members|ARMv5TE/ARM-mode/soft-float compile arguments recorded, not adopted|
| `camera-rtsp-verifier.host` |ELF64 LE x86-64|Strict companion build, not a HOST-fixture policy substitute|
| `camera-rtsp-verifier.arm` |ELF32 LE ARM EABI5, soft-float flag, no hard-float flag|New companion not installed into old image pins|
| `camera-rtsp-bench.host` |ELF64 LE x86-64|Synthetic HOST-only bench, forbidden in camera release|
| `host-integration-linkcheck-NOT-RUN` |ELF64 LE x86-64|Dummy link main, not a service|
| `arm-integration-linkcheck-NOT-RUN` |ELF32 LE ARM EABI5, soft-float flag, no hard-float flag|Dummy link main, never deploy|

All73 recorded compile objects have the expected class, endian, machine and
relocatable type. Both archive bodies match the corresponding14 actual compile
objects byte-for-byte by length/SHA256 multiset (28 members total). This check does
not claim archive filenames alone establish provenance. ELF machine/ABI flag
inspection does not inspect every instruction or prove target-kernel/libc ABI.

The exact recorded define sets separate these five groups:

- HOST/ARM integration (14 each): `server_crypto_config.h`, **no** HOST/public or
  synthetic-source define. The source `candidate_auth.c:14–22,52–60` therefore
  selects the strict native-policy branch even for the HOST integration archive.
  HOST architecture is not permission to substitute HOST policy or pins.
- HOST/ARM verifier (13 each): `rtsp_crypto_config.h`, no fixture defines.
- HOST bench (19): `IS_HOST_PUBLIC_ONLY`, `IS_SYNTHETIC_SOURCE_ONLY`, and
  `server_crypto_config.h`. Only this group includes the five fixture/receipt
  translation units. No ARM bench artifact exists in this build's seven outputs.

Each ARM compile command records `-march=armv5te -marm -mfloat-abi=soft`. Existing
ARM final files carry EABI5/soft-float flags. Source branches/pin checks and nominal
native2MiB/HOST4MiB policies remain unchanged. Defines are evidence from sealed
command receipts and source, not a new execution/preprocessor or memory test.

## Association and limits

The inspection rehashed every recorded authored-source and332 source/header-closure
entry, all88 successful command logs and all seven artifacts against the original
build report. It checked report/artifact pins again after inspection. The report
hash remains `7c995a0a23e6dcc9e7e2a4405eb1c192b309316cc78ad1da564c0afaf3eb334d`.
This is read-time association, not reproduction, authentic execution of a compiler,
future source freeze or a full transitive toolchain proof. Original build01 failure
and all build02 logs/outputs remain unchanged. Existing ELF/symbol reports retain
their separate narrower claims; no new undefined-symbol stubs or relinking occurred.

The seven outputs belong to the older build02 integration selection, **not** the
later reviewed prestarted-owned-listener/provider composition or a finished main.
No camera release manifest/packager exists here, so this does not prove an actual
release excludes HOST objects. Checklist A's architecture/configuration portion
has bounded static evidence; native ABI and actual release exclusion remain open.
Real bootstrap/provider/provisioning, media-owner recovery, packaging and functional/
native combined-load qualification remain missing. No G2/G3/G8 acceptance follows.

Only the inspector and safe public metadata are eligible for local Git. No binary,
image, private capture/digest/configuration or generated target package is added.
No current target/recorder/HOLD state was queried or changed, and no physical action
is requested. The strict clock-continuity block remains independent and unchanged.
