# QC07F span runtime — source replacement, NOT RUN

CAP-01/05/06/07, CORE-03/04, RP-04/09, B07. Read
`docs/qualification-plans/qc07f-span-runtime-20261024/CONTRACT.md` and HANDOFF.md.
Independent ordinary review and parent adoption required before any use.

Replace original owned_runtime.c/header and included native adapter/return bodies
with these units; compile **owned_runtime.c ONLY**, never native_adapter.c alone.
Local complete native_adapter.h selects actual accepted new memory definitions
before frozen nested headers can include the original header under the same
legitimate guard. Recompile all layout consumers; names/guards are NOT binary ABI.

New `qc07f_owned_runtime_memory_span_hub_v1(actual_runtime, actual_v2_hub)` binds
one persistent caller hub to the actual private raw+whole-hub prefault path.
The flat native fixed-buffers API refuses before effects. Paid readiness checks
complete actual range/credit/history association, without banning publication.
Every new field participates in pre-memory return-history refusal. No new native
owner, setter, borrowed-hub free, ready Boolean, source retirement or serve export.
Any borrowed span history keeps final cleanup on existing nonreturning retention;
normal source END does not relinquish caller hub lifetime.

`build.py` is a fixed fresh offline compile/REL metadata recipe. It never invokes
candidate code, an executable, callback, reader, native helper or fixture. It
excludes old runtime/native/memory/server/flat-hub objects; no dependency on any
unaccepted span-server version. Existing original server header declarations and
flat callback *types* remain source ABI for unchanged config, not server adoption.
Runtime source flow, actual external retention/backing/recovery/provisioning/main,
native allocation/process/combined-load/strict-C1 gates remain unqualified.
