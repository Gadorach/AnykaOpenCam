# RTSP-only startup credential construction

SEC-01/03/05 / STREAM-07 source candidate. This is actual provisioning-side C,
not another diagnostic/build framework, a credential generator run or a new app.

`qc07f_rtsp_startup_material_build_v1` accepts an explicitly supplied independent
RTSP username/realm/password and constructs the existing verifier's1536-byte
startup record. Public bindings come from the **same** current owned-runtime
config/graph and exact resource registry; the real existing Digest/Mbed TLS
helper computes HA1. No default password, UI-password import, random qualification
account, network input, filesystem path, logging or secret evidence hash exists.
The constructor allocates nothing and does not initialize media or a verifier.

Use only in a separately hardened, authenticated provisioning context before
starting media. Do not link this credential-derivation object/crypto configuration
into the media process or UI/HTTPS. `candidate_config` is a public binding here,
not proof of image/FD/entropy/boot/generation authority. Current fixed native
account1, H.264720p profile and explicit SHA256/legacy-MD5 choice are preserved;
no silent compatibility downgrade or TLS MD5 enablement occurs.

The returned record is **SECRET** (HA1 is password-equivalent). Caller must wipe
borrowed password copies and the output via the explicit clear API after a
separately admitted private FD handoff. This module does not implement that
handoff, storage, process hardening, account authorization, durable generations,
live credential rotation/revocation, reset, entropy or startup main. Never call it
with live config/output aliases. Failure clears fresh output; it never writes
to input spans or modifies an active backend.

HOST/ARM compilation and ordinary source review are separate from real credential
use. No operator password/material/secret fixture is read/generated or executed
by this work. No executable/test/listener/camera operation is selected. Original
owned-runtime/provider/verifier/framework/crypto sources remain unchanged.
