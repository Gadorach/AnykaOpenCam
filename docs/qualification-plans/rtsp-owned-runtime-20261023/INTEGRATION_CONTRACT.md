# Owned QC07F → authenticated RTSP integration (before code)

2026-10-23. CORE-03/04, CAP-05/07, STREAM-01/04/07, RP-08/09. SOURCE and
HOST/ARM compilation only; no task/gate/runtime acceptance.

Use direct fresh copies of the **actual** provider-return build01 generated
source, not the old v6 glue or an overlay generator. Keep one native singleton,
one current prestarted-owned-listener backend/child, real common producer/hub/
RTSP/auth dependencies and all original licenses. Old source/seals stay immutable.

## Feature API and order

1. Trusted caller supplies fresh, stable, disjoint runtime/config/graph/prestarted
   storage and genuine RNG, verifier-retention, optional owner/listener callbacks.
   Device-free `prepare_v1` installs source/guard/retention callback **addresses**
   and stable context/graph references before `candidate_prestarted_start`.
   It does not initialize native userdata, adopt/open FDs, create backing, sample
   device state, allocate, or claim readiness. Caller callbacks take this runtime
   context and may use its caller-owned bootstrap pointer.
2. One real device-free prestarted start completes the verifier fork. All other
   fork/exec must also finish. Config/backend/peer/graph/context addresses and
   callbacks never change afterward. `bind_postfork_v1` checks this same actual
   READY owner and calls the real one-time `nh_native_init` in its owning TU;
   no caller Boolean stands in for its actual private initialization state.
3. Independently admitted caller performs original UNKNOWN watchdog take,
   storage/exact139 opt-in, actual provider adoption, one-shot memory and paid
   backing preparation. No such operation is implied by wiring, source-ready,
   listener notification or bind. Real listener creation follows the last fork.
   Source-ready is a local refusal predicate, never lazy setup or permission.
4. Serve uses that same prestarted owner and moved listener, the real common
   producer/lifecycle and actual native borrowed packet. Keyframe demand goes
   through a small new guarded NEXT variant: force0/1 reaches the existing encode
   job **inside** unchanged guard-begin → legacy checks → disable order.
   Original NEXT remains a force0 wrapper. Retain actual packet/key until release;
   common END uses copied scalar facts, never released payload or invented receipt.

## Failure boundary

Public failed-NEXT internal-retirement semantics remain unchanged. Native error
with retained/uncertain work must fence and call the actual nonreturning native
SIGSTOP quarantine **before** returning to RTSP. Release, END, retirement and
clock uncertainty obey the same rule. Fresh server cleanup hooks must catch a
clock failure before closing client/listener FDs or unwinding stack, and catch
unretired native work before final unbind/return. No copied metadata owner,
outward success, END/reset/feed/close retry, resume/kill/respawn policy or new
watchdog credit. Known pre-effect configuration refusals remain ordinary errors;
actual initialized private ownership/history, not a generic permission Boolean,
distinguishes retention. Normal native retirement success is not fabricated.

This retention is **conditional on process survival**. The caller must already
provide a real external media-process owner, stable backing/caller lifetimes and
independent all-domain recovery. None is installed here. SIGSTOP, exit, pidfd or
copied numeric FDs cannot retire DMA or retain an exited task's FD table. Missing
bootstrap/provisioning/backing/main/packaging and native qualification remain
explicit blockers to calling this source candidate. Both native HOLDs stay sticky.

All API/callback/FD-table use is trusted serialized, nonreentrant, signal/thread/
alias-free stable C use; no stronger concurrency or hostile-memory claim. Preserve
native <2s/10s/nominal5s, 128KiB allocation cap, 40MiB/CMA/backing gates, one1MiB
hub, verifier pins/MEMLOCK/domain/rate/MD5 policy and exact loopback policy.

Compile actual C/dependencies HOST + retained musl ARMv5 GCC14.3.0 into archive/
ELF REL only, with compact commands/logs/source/header/tool pins. No main/stubs,
generator/ELF/test/listener/device execution, private reads, diagnostic image/build
lane or adversarial testing. Parent reviews source; no self-acceptance or native queue.
