# Cold catch — independent ordinary SOURCE/METHOD review

2026-10-23; BASE-01/05/07 / PLAT-06/08. **Conditional method sufficiency for ONE
operator-assisted cold observation, subject to the minimum corrections/conditions
below; not execution admission or unattended recovery.** Parent disposes separately.
No new controller or modification of the original catch is needed.

## Basis and pins

Read COLD_CATCH_PLAN, current passive plan/refusal/review/disposition/result;
complete original boot/private-helper/recorder sources; cold-context-v2 source,
contract/README/disposition and exact OBSERVED_PROFILE grammar. Only public source
and documents inspected; no private evidence or current-state verification.
`sha256sum` of exactly the four referenced public sources matched:

| Source | SHA256 |
|---|---|
| `scripts/qc07f_boot.py` | `6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985` |
| `scripts/qc07f_private_root_transfer.py` | `cb224d3cd8b635774b7e364b07b259d90640b30de269ea07351e767c3f97fa3c` |
| `work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py` | `135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00` |
| `apps/qualification/encoder-query77-cold-context-v2/cold_context.py` | `17986670452b756b16c7bc5c3b1ec3989447ded3369f7e23008b8ab0743fb558` |

No actual parent caller was supplied for line-by-line review. Physical wiring,
passive23b completion and historical marker metadata are parent/user reports, not
independently reproduced observations.

## Findings and minimum conditions

1. **Correct startup leaf creation order.** Plan phase3 must not precreate both
   directories. PrivateBoot validates fresh output then Boot.__init__ creates it
   (private helper:108–113; boot:287–290). The capture leaf must remain absent:
   start_receiver checks `fresh=True` and the recorder creates it itself
   (boot:209–214; recorder:76–81). Create/validate only their protected parents
   beforehand; use distinct fresh leaves. Existing leaves/socket/receipt refuse,
   not removal/reuse. This is a fresh caller-method clarification, not a source fix.
2. **The arm has an additional historical private-read prerequisite.** Recorder
   hold-cli handling reads the entire fixed corrected-wiring historical raw file
   and requires its recorded prompt (recorder:215–219). Status/arm-only RPC does
   not mean source-only IO. Parent must explicitly retain/authorize that existing
   read and establish its trusted immutable provenance privately; absent/untrusted
   evidence blocks. Do not create a replacement, copy a prompt, bypass the check
   or export its raw/digest. This reviewer did not open it.
3. **Import/construction is not arming; successful return is not current coverage.**
   Helper imports define functions/classes/constants, without invoking main, ROOT
   payload acquisition or serial access. PrivateBoot construction validates paths,
   creates output and binds protected_call; it does not open UART. Only the pinned
   recorder subprocess opens serial. start_receiver verifies recorder hash, child
   PID/UID via SO_PEERCRED, part/adapter, private source/envelope/raw-path claims,
   zero counters/busy and fuser sole ownership, then sends ONE hold-cli arm with
   volatile_cli_hold=True and inspection defaulting false (boot:204–282;
   private helper:115–142). These are point checks, not continuous exclusion or
   proof of physical OFF. Parent must pin actual imported module identities (the
   private helper imports `qc07f_boot` by name), trusted interpreter/dependencies,
   child argv/source association and unchanged sources; no shadowed/cached alternate
   module. Keep confirmed OFF throughout startup and exclude every other client.
4. **Make ON a live, cancellable handoff, not a queued instruction.** ACK ends_by
   is a wall-clock projection of remaining monotonic loop duration, not an enforced
   independent arm deadline; original status exposes no capture timer. Before ON,
   require successful complete startup including receipt, fresh same-PID/start/
   peer/part/source ownership association, still armed, zero RX/TX and no busy or
   inspection, plus an operator presently at the power control. No delayed-message
   ON after an interruption, unknown ownership, expiry or abandoned parent session.
   Parent/operator must explicitly cancel outstanding ON authority and confirm OFF
   on failure/timeout before finalizing/releasing custody. If communication cannot
   support that live handoff and actual ready OFF fallback, **block before ON**.
   Neither 14400s nor a bracketed clock check supplies whole-interval protection.
5. **Specify owned shutdown separately.** PrivateBoot.protected_call rejects
   `stop` (private helper:115–128), so phase6 cannot finalize through that wrapper.
   Parent needs a narrowly selected, same-peer original stop RPC and child-exit/
   private-manifest/socket finalization checks (recorder:281,289–306), with locks
   retained through success/failure postchecks. Do not invoke Boot.owner/status/
   caught/arm or process_remaining as substitutes. Startup/receipt failure can
   leave a live or already-armed child: preserve it and its private evidence,
   establish OFF and retire only that proven owner; no blind socket/process cleanup,
   takeover, retry or manufactured finalization. No second serial reader.
6. **Catch output must be observed, not assumed.** Original hold-cli schedules at
   most three small key packets on early banner/DRAM, or one on the Net fallback;
   password/prompt/read/handoff can cancel the burst (recorder:158–195). No automatic
   inspection is selected. ACK is not proof any packet succeeded. Associate actual
   trigger/TX-completion/missed-window/cancellation events and full new raw context;
   no exact-three requirement, arbitrary text TX or rescue key. Normal ON can boot
   stock software if the catch misses, so ready operator OFF is indispensable.
7. **Classifier and historical-marker roles remain narrow.** Exact19-record bytes,
   CRLF/spacing/build stamp and mandatory EOF match the public grammar; complete
   password challenge is expected but insufficient. Select from the first new byte,
   never last-banner repair. Preserve/check all later bytes independently, including
   the separate10s no-RX/no-TX/no-repeat-boot observation; a prefix match cannot hide
   later contradiction. Exact/incomplete/unsupported facts never authenticate ON,
   cold rails, source/owner or stable hold. Both historical HOLDs stay sticky under
   the exact read-only radio/instance0644 and reset0600 role below owned0700 control,
   unchanged ancestry/path/FD/byte/mode/inode postchecks on every exit. This extends
   neither passive review into TX permission nor C1v2 into a0644 exception.

The fixed fresh23c identity receipt is inside the existing locally excluded evidence
path, not outside-workspace capture storage. It contains identity/location/ACK
metadata, not raw or capture digests; retain it uncommitted with that exact role,
not a generic private-output exemption. Console/events/errors/raw/final digests
remain protected outside Git. No native clock, C1, staging, boot/image/app, watchdog,
DMA/backing or NOR gate is relaxed. No further effect or automatic operation follows.

Only this REVIEW.md was written. No import/call/build/test/fixture/classifier run,
private capture/state/proc/dev/HOLD/RPC/serial/network/hardware access, source/marker/
Git/AGENTS/central-document/model/provider/safeguard change or subworker occurred.
