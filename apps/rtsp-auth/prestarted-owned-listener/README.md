# Prestarted backend + mandatory owned listener — SOURCE composition candidate

ONE actual v2 candidate_prestarted backend/child/config/peer owner. Original real
device-free start and R1 sticky IPC close/history/stop/wait/destroy are unchanged.
Versioned actual entry:
`camera_rtsp_prestarted_owned_listener_serve_v1(owner, options, producer, &fd, &outcome)`.
No init/fork/exec/hidden finish or internal socket/bind/listen fallback in serve.
Original explicit verifier-only finish remains one-shot, never allowed on HOLD.

The selected listener must be created AFTER all fork/exec, independently admitted
by bootstrap. All native/provider/watchdog/backing/session/userdata setup also
follows last fork. Stable config/graph/context ADDRESS/callbacks are unchanged;
later actual userdata may only be prepared inside that stable context. None is
installed or authorized here; no fake source-ready/RNG/endpoint/provisioning.

Active nested entry returns BUSY FIRST without owner/FD/outcome writes. Outside
reentry, invalid owner/required NULL storage or non-READY phase refuses without
touching stale outcome. A valid READY owner plus required slots consumes the one
serve attempt; later backend/options/clock/listener errors retain backend on HOLD.
Listener preflight and post-poll requery use actual metadata/identity; queries are
not authority/no-alias proof. Move sets actual caller=-1/consumed1 before guard
binding; poll/accept only moved FD. Every postcommit return has consumed1, guard
unbound while stack-live and private slot cleared before one close. Close failure
is non-OK/HOLD; no retry or former-number ownership claim. Final outcome/owner
state are published before reentry release. Full serialized/disjoint C contract:
`docs/qualification-plans/rtsp-prestarted-owned-listener-20261022/COMPOSITION_CONTRACT.md`.

Derived from frozen prestarted-v2 source5a42cdc9… and owned-listener sourcefebab864…;
one combined source implementation, never two sibling libraries/backends. Listener
custody C is byte-identical; internal include/new versioned API is explicit. No
old monolithic/internal-bind entry is exported. All source/rate/auth/strict pin/
native2MiB/HOST4MiB/loopback/budgets/memory policies remain.

This is NOT a deployable camera app, whole backend qualification, external media
retention/bootstrap/recovery/backing or native acceptance. Stable backend peers do
not retain returned source stack or cure native failed-NEXT partial cleanup. No
provider/native glue/query client adoption, main, allocator, media RPC/supervisor,
END/reset/feed or automatic queue. Original licenses and failure history retained.

Compile/archive/ELF REL only, actual combined components + normal dependencies:

```sh
python3 -B apps/rtsp-auth/prestarted-owned-listener/build.py build01 --toolchain /path/to/retained/armv5-eabi--musl--stable-2025.08-1
```

No executable/output execution, runtime/test/fixture/payload generator/listener,
network/private/target/Git operations. Independent composition review is required.
