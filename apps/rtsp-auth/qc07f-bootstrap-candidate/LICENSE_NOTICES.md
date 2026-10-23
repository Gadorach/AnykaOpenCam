# License and dependency notices

New `bootstrap.c`, `bootstrap_v1.h` and `compile_recipe.sh` are GPL-2.0-only.
This does not grant a license to unrelated source, stock material or acquisitions.

Frozen dependencies are reused, not copied or relicensed. Preserve notices in:

- `apps/rtsp-auth/qc07f-owned-runtime/licenses/` and its original source headers;
- `apps/rtsp-auth/rtsp-provisioner-process/LICENSE_NOTICES.md`;
- `apps/rtsp-auth/rtsp-private-handoff/LICENSE_NOTICES.md`;
- `apps/rtsp-auth/rtsp-startup-material/LICENSE_NOTICES.md`;
- `apps/auth-web/service/vendor/mbedtls/LICENSE` (original dual-license terms).

Retained ARM toolchain includes GCC runtime-exception and musl notices, preserved
in the owned-runtime license directory. Compile-only objects/REL and manifests
are local evidence, not a release package or approval to publish/stage binaries.
