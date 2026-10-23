# Fresh cold-entry23 v2 — R1 only

2026-10-23; BASE-01/05/07 / PLAT-06/08. Retain v1 source/contract/negative review
unchanged and uninvoked. Parent's real idempotent OFF interface observation:
HTTP200, no Content-Type, zero response bytes; separate GET returned JSON/OFF.

Separate the actual acknowledgement and state paths. POST accepts the reported
empty acknowledgement only with HTTP200/no Location and existing bounded read/
timeout/explicit endpoint safeguards. It returns no state. Only a strict separate
GET can establish the outlet's reported ON/OFF; do not return a requested value
as observation or weaken GET's JSON/entity/type/duplicate-key checks.

One successful ON acknowledgement must be followed by an actual GET ON before
accepting any cold/quiet observation. Keep ON uncertainty marking before POST.
OFF remains one attempt followed by GET OFF even on an error; errors remain
sticky. No retries, default endpoint, discovery, redirects or fake state.

All other534-line v1 behavior/requirements/limits are inherited byte-for-byte,
including trusted actual user wiring/load association, private0700/0600 outputs,
unchanged readonly historical marker roles, original source pins, real child/peer/
wait custody, whole capture/trailer accounting, quiet observation, OFF/finalize
and independent success/failure postchecks. No native-clock/C1, loader/image/boot,
GPIO/motor/NOR/media/adoption or all-domain recovery guarantee is introduced.

AST/syntax compilation only; no import/call/help/default/test/fixture/private-state/
HTTP/UART/capture/power execution is selected by creating this revision. A focused
independent ordinary R1 re-review and parent disposition must precede invocation.
