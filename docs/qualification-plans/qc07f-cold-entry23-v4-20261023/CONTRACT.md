# Cold-entry23 v4 — dispatch settling and private power evidence

2026-10-23; BASE-01/05/07 / PLAT-06/08. Fresh derivative of frozen, once-invoked
v3. Preserve foundation-20261023d FAILED/HOLD, its source and all earlier failures.
User confirmed the supply path is correct and the camera visibly powered up;
do not repeat those physical questions. V3's arm-to-stop interval was about0.2s,
before its nominal3s wait. Immediate ON GET racing asynchronous dispatch is a
hypothesis, not a proved error location: v3 retained only a generic exception.

Move the EXISTING nominal3s wait from after ON GET to between the successful ON
POST acknowledgement and the SINGLE strict GET ON. Do not extend the ON wait,
retry a request, poll state, synthesize ON or weaken the GET contract. Add one
nominal250ms dispatch settle before the SINGLE OFF GET, even if the one OFF POST
raised; retain its error and still attempt that GET. These are ordinary delays,
not hard deadlines, native timer coverage or independent host-hang recovery.

Keep narrow private result fields for the last power phase, successful POST
acknowledgements and actual strict GET Boolean results separately. Store a GET
value before checking its expected state, so a rejected OFF/ON value is retained.
An absent result remains null, never inferred from the requested state or HTTP200.
No URL/configuration, response body, credential, capture bytes or private digest
enters public metadata. Existing private intent/results remain exclusive0600.

ALL other v3 source behavior remains unchanged: strict HTTP GET/entity/body bounds,
source pins, exact port and private inputs, read-only historical marker roles,
sole owned child/peer/argv, one arm/ON/OFF, exact360-byte cold profile plus EVERY
controlled trailer byte, separate10s quiet, failure OFF, owned stop/wait0/final
hashes and individual retained-input postchecks. Errors remain sticky. No retry,
classifier/old source edit, C1/native-clock waiver or hardware effect beyond the
one future cold observation. No loader/image/boot/motor/NOR/media operation.

AST/syntax and exact public source delta checks only until independent ordinary
review and parent selection. No import/call/test/private state/HTTP/UART/power
operation is authorized merely by creating this candidate. Select one fresh
foundation-20261023e transaction only after current prerequisites are revalidated.
