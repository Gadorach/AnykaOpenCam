# Parent-only bounded native02 orchestration recipe

**Offline model-tested candidate, not native qualification.** This fixes orchestration
only: optional `# ` asynchronous shell prompt was rejected by the parent's old PID
regex, and a command echo containing OWN_EXIT later consumed the native01 client
window. Native01 issued **zero TLS requests**; it proved pinned2MiB READY and natural
180s expiry/owned retirement only. Original partial/cleanup/native04 receipts and
the first model failure are preserved under the new parent-campaign evidence scope.

No new guardian/daemon/transport, UART owner/socket creation, recovery arm/reset/
reboot, installation/staging/private-input generation, target cleanup, cert bypass,
trust change or automatic retry. The recipe does not inherit dated h/PID/22:56 arm
metadata. Existing canonical service/launcher/client/assets/tests/plans are read-only.

## Components / run offline first

* `recipe.py`: exact scalar parser, one finite job state machine and fixed commands.
* `owned_client.py`: canonical PC client source pin, exact CLI, bounded scalar pipe,
  source-owned pidfd cancellation, late stdout-vs-exit race handling.
* `parent_only.py`: offline by default; no native imports/actions without explicit
  reviewed public entry source/roots/pins and a measured public admission plan.
* `test_recipe.py` / `test_ownership.py`: logical fake UART/backend/PC ownership only.

```sh
python3 -B apps/auth-web/ui-candidate/parent_campaign/run_tests.py FRESHmodel
python3 -B apps/auth-web/ui-candidate/parent_campaign/parent_only.py
```

Fresh evidence child names refuse overwrite. No completed earlier UI/browser/native
or HOST qualification is repeated. These tests do not invoke the actual private
client or target helpers; optional full HOSTPUBLICDUMMY launch was not rerun.

## Parent entry contract — required integration/HOLD boundary

Parent supplies an explicit source-pinned entry file within explicit approved public
source roots. No helper is discovered/imported by name or created by this job.
`PUBLIC_SOURCE_INPUTS` must list absolute reviewed source paths and SHA256 for the
existing `uart_capture`, `probe`, `checked_command` bindings. Parent audits imports
as side-effect-free: no opening serial/creating receiver/socket/arming/resetting.

Required callbacks (return the dataclasses imported from this `recipe.py`):

| Callback | Existing parent responsibility |
| --- | --- |
| `probe()` → Admission | Proper current JSON request to the existing receiver, with actual SO_PEERCRED, exact selected adapter/part/owner/start_ticks/socket inode/protocol epoch; actual75 GNU buildID/public kernel-config seal and current prompt; exact existing recovery ID/end; no competing transfer/receiver; staged target metadata/pins. Never an empty IPC probe or boolean alias. |
| `checked_command(command, admission)` → TxReceipt | Existing paced parent TX only, current exact authority rechecked; actual pre-TX RX offset and sequence. Do not wait on an echo/substring/typed exit flag. Returned transcript/status strings are not success evidence. |
| `uart_capture(offset)` → iterable Chunk | Existing sole-owner fresh receive bytes with exact source identity/epoch and absolute contiguous byte offsets; no serial open/new capture. Preserve raw evidence in the existing recorder, not this redacted logger. |
| `prove_bootstrap(pid,parent_pid,exe,checked_inspection)` → BootstrapProof | Current exact source-owned process PPID/start_ticks/exe/boot/epoch, not a supplied PID or historical marker. If a fresh retained receipt is insufficient, use only the supplied tracked fixed inspection TX (`cat /proc/PID/stat; ls -l /proc/PID/exe`) and parse start_ticks/PPID in PC memory; no awk/stat utility/ps/proc argv. Hidden/unregistered TX changes fail admission. |
| `poststate(rx_end)` → exact observation dict | Decode the fresh post-command RX (not a cached success boolean): watchdog inactive; UART errors/busy0; ISP readable/guards/IRQ/frames/queue/held/WB-owner0; source lineage gone and18443 listener absent. |

All callbacks must use the already-reviewed proper JSON/status/checked-command
entry and enforce existing bounded RPC/TX timeouts within the current phase budget.
This recipe cannot turn an opaque blocking/unvalidated adapter into authority; if
these obligations cannot be met, **HOLD before native launch**. There is deliberately
no bundled native adapter, guessed socket, empty probe or replacement guardian.

## Explicit measured plan, not stale admission flags

The public plan contains only `spec`, `expected`, `client_source`, `client_sha256`.
No boot/parent PID input is accepted. `Spec` contains marker prefix, fresh trusted
pre-staged launcher/private-scope/service/child paths, three reviewed public image
pins and an **opaque** PC private directory path. Target names are one safe leaf
under `/run` or `/tmp`; native01/signal04 old names are refused. Native01 empty dirs
are left alone; fresh02 names respect the canonical O_EXCL invariants.
V2 also rejects `.` and `..` in every target leaf position and requires the private
scope path to differ from all three binary paths. These lexical checks run during
Spec construction, before PC preflight/backend probe/TX. They do not establish
trusted staging: actual root ownership, modes, exact pins and live admission remain
mandatory and unchanged. Original44 source/receipts are preserved;19 new independent
negative cases cover all16 run/tmp alias-position combinations and3 scope collisions.

Service pin remains `c9cfc8b954bd5c2370c955f0ce6d886224b27d3899d1972d80baf285fa5f0e2e`,
strict child `8bb3d596065efb35338e2f28bedd60718eac8254d4f569e4412e77567f9c119a`.
Launcher pin comes from the parent's reviewed matched build, never a runtime override.

Expected identity tuple is `(stream_id, protocol_epoch, owner_pid, owner_start_ticks,
SO_PEERCRED_pid, SO_PEERCRED_uid, adapter_id, socket_dev, socket_ino)`, plus actual
boot ID, GNU buildID, **public kernel config** SHA256, exact recovery arm ID/end.
The live probe must match it before each operation; ≤1s observation age, sole RX
owner, no transfer, Linux75 seal/current `# ` prompt, unchanged TX sequence and
adequate existing recovery horizon are mandatory. Dates/old PIDs/timers/flags are
never promoted into current authority. Restarts, owner/socket/part/epoch/arm changes,
stale samples and unknown extra TX fail closed.

Target staging metadata is root-owned0700 scope with **four**0600 FD inputs and
three root0500 source binaries, ABI2/CFG768/runtime180 and unchanged2MiB both.
PC originals are never read/statted/hashed by the recipe: only the existing pinned
private_client validates its own mandatory0700/0600 policy and reads its keys/password/
config. Parent supplies brand-new native02 private inputs; lexical fresh names alone
are not a cryptographic freshness claim. No payload/key/credential/config hashes.

## Scalar and process binding

RX scope begins at this campaign's actual pre-TX byte offset. Complete newline
records only, optional single `# ` prefix and wrapped standalone prompt lines;
anchored exact PID/READY/ACK/retirement/rootwait grammars. All split positions are
tested. Entire echoed commands/scripts/comments are not substring matches, and
registered TX echo lines are de-echoed with the same prompt normalization. Old
prefixes/ranges, gaps, duplicate/conflicting PID/READY/retirement, early ACK or
poststate markers, negative receipts and stale epoch are not success.

A bare `# scalar` is inherently indistinguishable from a typed shell comment in
raw bytes alone. Safety therefore also requires exclusive source TX sequencing:
known TX echoes are registered, unknown concurrent TX invalidates admission before
any effect. This is not a fabricated byte-level direction detector. PID authority
comes only from fresh startup RX plus current owned PPID/start_ticks/exe proof;
PID reuse/dead/wrong-exe cannot be signalled as a replacement.

The actual canonical PC command is:

```text
python -B /ABS/CANONICAL/apps/auth-web/native-launch/private_client.py --private-directory /ABS/FRESH02 --static-ui-assets --after-owner-revoke
```

It starts once on bound pinned READY/class/2MiB receipts, with155s remaining for
the unchanged140s client plus retirement/post margin. No wait for a manually typed
flag, no TLS downgrade/URL selector. Only source-owned PC stdout exact assets PASS,
owner_revoke_window and full PASS lines are accepted; process exit0 is separately
required. Unknown output is rejected/redacted, never saved or hashed. Poll drains
the exited client's pipe before publishing terminal status, retaining a late PASS.

On the actual PC revoke-window line, current source/exe proof precedes the fixed
native04 `case ls -l /proc/PID/exe → kill -USR1` command. Require actual signal0,
owner_revoke_sent1 and ACK1 from fresh RX; missing ACK/late grant/client failure
HOLD. Only actual client PASS+exit0+ACK permits normal SIGTERM. Target requested
kill alone is not retirement: require strict-child/service wait0, lineage0,
bootstrap result0, rootwait0 and fresh poststate/listener evidence.

## Preserved bounds / failure behavior

Native04 startup command is byte-equal after only approved paths/marker renaming;
the test compares it to the authoritative public observed source. Variables,
5s watchdog,4s sleep/feed,45 counter, root wait and **only rootwait0** magic-close
remain unchanged. No premature/destructive close or added root permissions.
BusyBox commands are only existing cat/ls/case/test/grep/kill/for/sleep/echo/printf;
no awk/uname/rmdir/stat/ps/new file daemon/argv print. The recipe never removes dirs.

One job maximum180s, bounded by the current arm end minus5s margin; READY10s,
PC client≤140s, owner ACK2s, retirement8s, poststate5s, all clamped to remaining
source/arm budget. Slow callbacks cannot extend phases or start a late client.
HOLD cancels only the PC's actual original pidfd; no PID reacquisition/fallback.
Missing PIDFD means HOLD/no native PASS, not an unsafe PID-based cancellation.
Then only a still-current freshly RX-owned bootstrap may receive the fixed STOP
case. Wrong-exe/dead/owner-changed/arm-expired refusal retains bounded existing
recovery; no uncertain watchdog magic-close/reset/rearm or automatic service retry.
If process descriptor acquisition/cleanup cannot be confirmed, record HOLD and retain
the source-owned handle/failure for parent review, never claim clean retirement.

Fresh poststate command is generated with this job's parsed PID pair:

```text
cat /sys/class/watchdog/watchdog0/state; cat /sys/class/tty/ttyAK0/rx_state;
cat /sys/bus/platform/devices/20000000.isp/state;
grep -E "^(MemAvailable|MemFree|CmaFree):" /proc/meminfo;
test absence of /proc/OWN_BOOT_PID/exe and /proc/OWN_PARENT_PID/exe;
grep the18443(480B) LISTEN row in /proc/net/tcp, then emit fixed scoped scalar markers
```

This text is parent-only; no such target/proc/status command was executed by the
agent. Target volatile cleanup is a separate parent action **after observed actual
retirement**; PC originals retained. This recipe does not repeat native01 cleanup.

## Parent-only entry (never invoked by this agent)

```text
python3 -B .../parent_campaign/parent_only.py --parent-run
  --entry-file /ABS/REVIEWED_PARENT_ENTRY.py --entry-sha256 PUBLIC_CODE_SHA
  --entry-root /ABS/APPROVED_PUBLIC_SOURCE_ROOT [repeat for existing helpers]
  --public-plan /ABS/FRESH_MEASURED_PUBLIC_PLAN.json
  --output .../evidence/delegated/20261012-native-ui/parent-campaign/FRESH02-receipt.json
```

Entry/source binding pins and public plan must be reviewed/integrated by parent
before this command is eligible. No actual driver integration/current authority is
asserted by the logical tests. All native/media/G3/G8/product gates remain open.
**One next task:** parent reviews these sources/tests, then performs one fresh native02
campaign with new private inputs, the same exact UI/child/matched-launcher ELFs and
unchanged bounds; preserve every failed observation and actual poststate.
