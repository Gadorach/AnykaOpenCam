# Closed supervised driver RAM-entry source selection

2026-10-24 · BASE-01/05/07, PLAT-06/08; supports RP-02/11/12.
Select one isolated source implementation at
`apps/qualification/qc07f-driver-entry24/` and this qualification directory.
Parent alone executes target operations and integrates central status. No live
operation, power request, recorder open, helper invocation or test is selected.

Reuse the frozen cold-entry23-v4 source-method guards and original recorder plus
the already audited fixed U-Boot/QCL/monitor primitives as source references.
Create a closed driver-boot profile, not a generic command/controller framework:
one fresh OFF/capture/cold catch → exact bootloader/neutral pin → pinned loader
word staging → one no-retry QCL upload → independent whole RAM CRC → exact volatile
bootargs/bootm → fixed read-only monitor inventory → OFF/owned finalization.
No native radio, station, networking/MTU change, motor/mic/speaker, storage or
flash effect. Driver image is being built separately; exact reviewed image pin/
size/CRC/config association must be supplied and validated before execution.

The explicit supervised user policy permits clock-offset/discontinuity detection
and abort/cutoff, not a whole-interval guarantee. Keep fixed deadlines/reserves,
all original source/privacy/cold-prefix/peer/owner/HOLD guards, no retries or
watermark repair. Do not import or invoke old22b controllers, widen PrivateBoot
globally, alter old recorder/markers, or use a success flag as admission.

Mandatory independent ordinary source review and parent selection precede any
invocation. If input/transport/observer facts cannot support the closed scope,
report the concrete blocker; do not fabricate a ready driver or relaxed parser.
