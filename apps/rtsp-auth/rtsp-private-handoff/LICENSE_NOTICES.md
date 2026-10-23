# License/provenance notices

New `private_handoff.c`, `private_handoff.h`, `positive_publicdummy.c` and the
bounded compilation recipe are GPL-2.0-only, consistent with the camera sources
and original interfaces they use. No global license grant is made.

Unmodified dependencies, never copied/relicensed here:

- `apps/rtsp-auth/rtsp-startup-material/startup_material.{c,h}` and
  `apps/rtsp-auth/qc07f-owned-runtime/server/candidate_auth.h`: GPL-2.0-only.
- `framework/src/{backend,rtsp_digest,rtsp_digest_wire,rtsp_input,rtsp_request}.c`
  and `framework/include/camera/`: retain their original GPL-2.0-only notices.
- `apps/rtsp-auth/rtsp_crypto_config.h` and actual verifier interfaces:
  GPL-2.0-only; RTSP-only build, never the HTTPS configuration.
- `apps/auth-web/service/vendor/mbedtls/library/{sha256,md5,constant_time,platform_util}.c`
  and consumed Mbed TLS headers: Copyright The Mbed TLS Contributors,
  **Apache-2.0 OR GPL-2.0-or-later**, as their own headers and
  `apps/auth-web/service/vendor/mbedtls/LICENSE` specify. Preserve those full
  license texts with any future distribution. This local combination can use
  their GPL-2.0 option; no upstream license text was changed.

`native-qualification/{paths.c,material_tool.c}` was reference-only. No old main,
random qualification-password generator or builder was invoked/adopted. The
backend and verifier checks were read as actual interface constraints. Retained
musl/ARM compiler and libc keep their own licenses; compile artifacts are not
release packaging or redistribution approval.
