# Owned loopback listener — SOURCE/compile candidate only

RP-09 / STREAM-04/07. Read
`docs/qualification-plans/rtsp-owned-listener-20261022/CONTRACT.md` first.

`camera_rtsp_owned_listener_serve_v1` is the actual frozen server derivative with
mandatory real preopened-listener custody. It is not an unused adoption helper,
launcher, platform bootstrap, network admission policy or new main. The original
loopback/strict authentication/source/provider/lifecycle gates remain. It accepts
only a nonblocking/CLOEXEC listening IPv4 TCP socket bound to exact127.0.0.1 and
the already validated options port. Validation is query-only and repeated after
real verifier READY/engine ACK, immediately before moving the actual caller int
slot to -1. Poll/accept use the moved descriptor. Close clears the private slot
first, occurs once, and returns explicit consumed/close-attempt/error disposition.

`frozen/` is a byte-exact41-file read-only reference/import, including original
source/license notices. Its guarded builder/harness/tests are **never executed**.
The original internal-bind server is reference-only and absent from compilation
and exported artifacts. Its old header declaration is inert. The derivative uses
the original actual auth/backend/adapter/framework/crypto implementations; no
verifier core, pin, native2MiB/HOST4MiB budget or readiness guard is changed. Do
not link this archive alongside another copy of those implementations.

Compile only (no application/test/generator/child/listener executable produced):

```text
python3 -B apps/rtsp-auth/owned-listener-candidate/build.py build01
```

The fresh builder reads the actual prior `local/build/rtsp-app/build02` compiler
command receipt and dependency/tool pins, snapshots public inputs/licenses,
compiles the real changed server and custody unit plus unchanged real dependency
units for HOST and retained ARMv5 musl, and emits archive/ELF REL artifacts only
inside the assigned docs/runs directory. It imports/executes no old builder.
Every output name must be fresh. Failed builds are preserved.

No runtime/native/deployment/finished-app acceptance. No real trusted bootstrap,
provisioning, endpoint, entropy/source callbacks or backing is supplied. Original
failed-next external retention, admitted memory/backing, query77 pairing and
ordinary-operation/native/product gates remain. No integration of the separate
runtime/provider derivatives, no new network mode, no native queue or HOLD change.
Deferred testing is not passed. Exact caller/source/FD-table/lifetime prerequisites
need independent review before any use; metadata does not establish them.
