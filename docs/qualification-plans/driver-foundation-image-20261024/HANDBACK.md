# Isolated driver-image lane handback — STOPPED, no image

2026-10-24. **ONE full Kbuild PASS, then author verifier FAILED; no UIM produced.**
No rerun, recipe/source/flag repair, alternate packaging or target action.

Owned paths only:

- `bringup/qc07f/driver-foundation-image/`: fixed `build.py` and NOTICES.md.
- `docs/qualification-plans/driver-foundation-image-20261024/`: pre-source
  CONTRACT, BUILD_RESULT, BUILD_MANIFEST, this handback and DELIVERY pins.
- `work/lightbulb-camera/driver-foundation-20261024/`: fresh full source/build,
  narrow archive derivative, inputs/logs/.cmd/dependencies, original FAILURE.json
  and fresh read-only `failure-handback/` metadata. Generated content stays local.

## Precise blocking result

`build.py:429` incorrectly requires unsuffixed `ak_ptz_hold`; actual saved
Clang23.1.3 object/vmlinux use `.0/.1/.3/.4/.5/.6/.7/.8` split symbols. The kernel
compile/link exited0 once;16 top-level commands exited0, then Python exited1.
Name-only dictionary handling also conflates six local `state_show` definitions.
No failing assertion or output was edited/relabelled. Recipe remains SHA256
`952ae8c2a0d8dd0a47b704956179892bd2fbede4830131891c9ca8288d78f52f`.

`work/lightbulb-camera/driver-foundation-20261024/images/` is EMPTY.
Image path/hash/size/CRCs/actual header are **absent**, not old query77 evidence.
Whole-package decompressor/DTB/stack/workspace/staging checks were NOT REACHED.
The built zImage3400736B is preserved, not a packaged/approved RAM artifact.

Reviewed full PTZ source is actual C SHA
`a0c7d152b8a2e163ed20419e2bfbd6a7fe765056870dd939b3e5b78511a6ce87`.
Fresh actual ptz.o equals the prior reviewed8580B built-in object byte-for-byte;
new actual .cmd and linked code/symbol logs are retained. Unique release is
`6.18.55-qc07f-driver-foundation01`; only selected LOCALVERSION/archive-path config
delta and exact full PTZ replacement occurred. DTB and all other source/config
policy stayed unchanged. Fresh archive removes exactly hgics.ko and bin/rtspd,
preserves every remaining raw record and is embedded exactly in the new kernel.

Read BUILD_RESULT.md and BUILD_MANIFEST.json for exact output/metadata pins,
timings, original-source/build/rollback before/after checks and evidence limits.
Read-only postfailure collection is NOT a continuation/retry of the failed recipe.

## Parent boundary

Next eligible action is **independent ordinary review and parent disposition of
the preserved failure/outputs**, not automatic repair or an executable boot task.
No reviewer/subagent was spawned by this lane. No source author work remains
active. Parent alone owns any later source/packaging selection and hardware.

No UART/network/outlet/device/private-capture/Git/index/central-plan/AGENTS change
was performed here. No target ELF/helper/test, module install/load, motor/mic/radio,
flash or boot action. No query78 invocation or station/P1 adoption/reroute.
Speaker remains deferred; backend expansion parked; strict C1 unproved. Supervised
detect/abort/manual-fallback policy does not create a current lease or native GO.
No physical STOP/current/thermal/position/provider-binding or driver-ready claim.
