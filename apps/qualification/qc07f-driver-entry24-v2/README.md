# Driver entry24 v2 — bounded R1 source correction

Read [CONTRACT.md](../../../docs/qualification-plans/driver-supervised-entry-v2-20261024/CONTRACT.md)
and [HANDBACK.md](../../../docs/qualification-plans/driver-supervised-entry-v2-20261024/HANDBACK.md).
V1 remains frozen NOT ADOPTED. This copy changes ONLY `Entry.finish` body:
unknown OFF plus already-observed child exit now attempts independent absence
checks and available final accounting with `controlled=False`.

No stop RPC, further power request, kill/unlink, takeover or retry is added. Known
exit0 is NOT power proof or controlled stop; `final_ok` remains false and manual
OFF remains required. The existing known-OFF and live-retention paths are unchanged.
All other source bytes and the closed startup/boot/RO policy/CLI are identical.

AST/text/hash inspection only, no candidate/helper import/call/test or hardware
operation. Independent ordinary correction review, parent disposition, actual
reviewed paired image selection and fresh supervised admission precede any use.
No filled invocation/image selection is supplied. No native readiness or next
phase is claimed. Speaker tests DEFERRED; backend expansion PARKED.
