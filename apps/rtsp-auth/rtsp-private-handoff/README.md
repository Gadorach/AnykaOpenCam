# Actual private RTSP descriptor handoff (candidate)

SEC-01/03/05/11, STREAM-07. `private_handoff.c`/header implement real exclusive
file construction and readonly FD custody around the frozen reviewed startup
constructor. No old tool/main was copied or invoked. No media/HTTPS adoption.
See `docs/qualification-plans/rtsp-private-handoff-20261023/CONTRACT.md` before use.

Native order in the separate provisioning context:

1. Call actual `qc07f_rtsp_private_provisioner_harden_v1()` BEFORE authenticated
   secret ingress. Failure is terminal; no fake hardening permission argument.
2. Supply a trusted exclusively preopened private directory FD, exact immutable
   config/registry and borrowed independent RTSP credential spans to
   `qc07f_rtsp_private_handoff_create_v1()`, using fresh custody + disjoint material
   scratch. Creation repeats hardening, constructs via the real helper, writes
   the actual1536-byte record, closes its only writer once, opens/checks a readonly
   FD and wipes scratch. Caller wipes the borrowed password separately.
3. Inspect both result AND phase. No destructor/automatic unlink/FD cleanup.
   Created partial file and filled readonly slot survive failures; writer close
   uncertainty permanently blocks reopen/handoff. Never reset/copy/retry object.
4. As the explicit last step immediately before owned-runtime prepare/start,
   move the readonly FD with `qc07f_rtsp_private_handoff_move_v1()` into the SAME
   still-unused config/material slot -1, with the actual fresh prestarted owner.
   No duplicate FD ownership: custody slot becomes -1. Preserve config/FD after
   later startup failures until separate reviewed retirement/cleanup.

Native requires root IDs/no groups, exact root-owned0700 directory, linked
root-owned0600 regular file, exact1536 bytes and readonly CLOEXEC offset0. These
checks do not establish namespace trust, no other writers, process separation,
authenticated ingress, durable storage/generations or native readiness. File
fsync is required; this is not a directory-fsync-backed durable credential store.
Caller must maintain the whole-interval quiescent namespace/no-alias/lifetime
contract, never link/call construction in media or mutate a live config.

`positive_publicdummy.c` is an explicit separate HOST-only normal functional
harness with no input selectors and no backend/image/media linkage. Its build
must define `RTSP_HANDOFF_HOST_PUBLICDUMMY`, which exports different symbols and
produces PUBLICDUMMY account material from a fixed public fixture, not native
account1. That build cannot move into native config or satisfy native hardening.
Only one normal fixture invocation is selected; no negative/adversarial suite.

All new C/header code is GPL-2.0-only, matching the original camera interfaces.
Frozen camera sources/notices remain intact. Vendored Mbed TLS retains its own
Apache-2.0 OR GPL-2.0-or-later notices; see LICENSE_NOTICES.md in this directory.
Build logs/pins and honest remaining gaps are under the qualification directory.
