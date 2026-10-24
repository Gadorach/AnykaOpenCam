# License and dependency notices

Derivative `bootstrap.c`, `bootstrap_v1.h` and fixed `compile_recipe.sh` remain
GPL-2.0-only, derived from the frozen `qc07f-bootstrap-candidate` source. No global
license grant covers unrelated source, stock firmware or private acquisitions.

Dependencies are reused, not copied or relicensed. Preserve original headers and:

- `apps/rtsp-auth/qc07f-bootstrap-candidate/LICENSE_NOTICES.md`;
- `apps/rtsp-auth/rtsp-os-random/LICENSE_NOTICES.md`;
- `apps/rtsp-auth/qc07f-owned-runtime/licenses/` (including GCC runtime/musl);
- provisioner-process/private-handoff/startup-material license notices;
- `apps/auth-web/service/vendor/mbedtls/LICENSE` (original dual-license terms).

Local objects/REL are compile evidence, not release packaging or publication
approval. Explicit parent review/adoption is required; no deployable app is claimed.
