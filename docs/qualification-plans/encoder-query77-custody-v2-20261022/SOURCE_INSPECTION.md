# v2 author static correction/callsite inspection — not independent acceptance

2026-10-22. Module `apps/qualification/encoder-query77-custody-v2/custody.py`,962
lines/42123B, SHA `d60176e2d2725460a22c82fee6ce1068a82a07a976daddb7287c52daf3ade16f`.
One fresh derivative, pre-code contract first. Static reads/AST/delta/in-memory
syntax only; no candidate import/call/test or private IO. V1 and its failed
independent R1/R2 review stay unchanged/unaccepted.

## R1 actual branches and callers

- `_validate_previous`209–220: only caller `validate_observation`239; public
  type/Epoch equality checks precede that call. Previous clocks must be >=original
  with exact BOOTTIME/wall deltas, counters>=original, catch reachable from initial
  state, cap within original natural remaining and at least the base150s reserve.
  No earlier selected-cost fact is invented. It is not an authenticated history.
- `validate_observation`223–262: validates previous **before use**; current then
  loops both original and previous anchors for clocks, counters and catch. Original
  non-ARMED cannot become a different state through previous. Original ARMED may
  enter CONSUMED/UNARMED only in BOOKKEEPING_ONLY, then cannot change/rearm through
  the previous check. REQUIRED still refuses any non-ARMED current state.
- Current cap stays <=original natural and prior cap-minus-elapsed, original
  fixed expiry/lifetime,150s+selected cost. Strict frozen schema/int-not-bool/
  signed63 bounds and tuples are unchanged. No tolerance/sampler/new source facts.
- Actual guard callers remain constructor pure initial check, `observe`858 and
  `complete_local`889. Their source body AST is unchanged except constructor path
  role selection elsewhere. No extra catch argument or ready Boolean was added.
  No synthetic counter/catch/clock traces were instantiated/executed here.

## R2 actual exceptional/close/partial-constructor closure

| Source | Author-inspected static path |
|---|---|
| `_call`813–831 | Busy/closed/complete/failed refusal before try has no owner/ledger IO. Admitted entry/body/normal-exit failure -> `_failed_exit(error,"CALL")`828 before fixed safe rethrow. First failure survives a previously set local complete flag. |
| Constructor445–542 | Roots/slots/first-failure-recorded and three fixed report slots initialized before acquisition. Exception539 -> `_failed_exit(...,"CONSTRUCTOR")` before existing host teardown. No current/private path is probed to fill absent context. |
| `_failed_exit`747–753 | Sticky primary selected/set **before** read-only postchecks. Calls `_postchecks`, then safe `_failure`; secondary outcomes cannot replace primary. No external callback or authority return. |
| `_available_owner_postchecks`687–736 | Separate try per root, per hold and per source. Operation-dir association separate too. Seven fixed categories maximum; no private marker/ledger integrity check, writes, opens, fsync or lock reacquisition in this path. One category failure does not skip others. |
| `_Root._acquired`, root checks | Existing descriptors only; partial traversal uses bounded partial-root checks and reports PARTIAL, not full PASS. Empty nodes => UNAVAILABLE without IO. UID stored before acquisition so failed available checks have bounded identity context. |
| `_check_hold`553–565 | Factored from original. Raw/metadata/path comparisons for each pinned description, independent of root/ledger success. Acquired but unpinned description gets only available metadata observation then HOLD/UNPINNED; never creates/rebases its raw baseline. Missing second hold is UNAVAILABLE, no open. |
| `_check_op_entry`571–579 | Read-only operation directory/path association observation, separate from private-root/ledger admission. No op FD => UNAVAILABLE. Missing acquisition identity cannot become PASS. Private-root failure remains separately reported even if the held op entry comparison succeeds. |
| Source slots | None => UNAVAILABLE, no read. Retained FD rehash/stable metadata with original<=64KiB/4096B limits. Missing metadata cannot be replaced with a fresh source baseline; no source pathname/proc fallback. Each slot attempted after another fails. |
| `_postchecks`738–745 | One bounded report tuple per CONSTRUCTOR/CALL/CLOSE slot, with PASS/FAIL/UNAVAILABLE/UNPINNED/PARTIAL. Unexpected infrastructure failure records fixed POSTCHECK_INTERNAL/FAIL, not success. No public exporter. |
| `_failure`786–810 | First primary never overwritten; poison/no ledger/closed skips formatting/append. Otherwise verifies owned private ledger **before** bounded builtin diagnostic formatting. FIRST_FAILURE then distinct EXIT_POSTCHECKS event if later close reports can safely append. Existing first code remains. Failure to record makes poison sticky, never retries. |
| `_append`763–784, reader588–656 | Adds closed bounded `exit_checks` field to hash-covered record, at most seven unique fixed category/status/code triples. Same4096B/64records/64KiB caps. One write/fsync, directory fsync/readback; unknown outcomes forbid diagnostic writes. No v1 ledger adoption/schema migration. |
| `close`926–962 | Busy/closed refusal has no IO. After ordinary entry path (including halfway failure), finally949 attempts independent available owner postchecks before teardown. Primary retained; if no prior error, any non-PASS category is non-OK. Safe failure evidence only, then same clear-before-one-close host teardown. Complete-local cannot override earlier or close failure. |

All three changed `_failure`-related paths inspected: only `_failed_exit`752 and
`close`956 directly call the revised signature. `_failed_exit` is called only by
constructor539 and `_call`828. `_postchecks` only by `_failed_exit`751 and close949.
No leftover old-signature caller or new helper path reaches an old v1 module.
`_check_holds`551 and postcheck719 call the same factored hold comparison.
`_check_op` delegates to entry571; postcheck707 independently calls entry, not
private-ledger logic. Original `_check_all` remains entry/normal-exit checking,
but failed-boundary coverage no longer depends on its successful completion.

These postchecks observe failed work before bounded diagnostic bookkeeping and
host teardown, not an atomic claim that nothing changes afterward. If ledger
poison prevents writing, reports/primary remain in memory and permanent marker
HOLD stays authoritative. Cleanup after ledger closure cannot persist another
diagnostic; fixed CLEANUP/non-OK and first error remain, no numeric-FD retention.
No broad signal/thread/MemoryError/host crash/durability proof is claimed.

## Exact historical/private role amendment closure

- Source constants18–22 name canonical workspace, exact historical CONTROL and
  original Boot source pin. No Boot import/path discovery/current-state getter.
  Public Boot23–25 was read as text; actual CONTROL/.git/marker paths were not read.
- `Custody.open(private_root,operation_name,profile,epoch)`445 removes caller
  control choice. `_control=_HistoricalRoot()`, `_private=_Root()`; only historical
  `_HistoricalRoot.acquire(uid)`367 calls base acquire with the exact constant.
  No role selector/ignore-Git Boolean, alternate or compatibility root.
- `_Root._git_condition`324 always `_git_absent`, including the entire private
  ancestry and op entry. Private output root never inherits historical policy.
- `_HistoricalRoot._git_condition`374 computes the already-walked canonical path;
  all nodes except the exact workspace require `.git` absence. At workspace only,
  `.git` must be safe non-symlink dir/regular marker, trusted uid/no022 (regular
  single-link); full metadata initially pinned during acquire only and compared
  thereafter. Capture lifecycle flag is internal, not caller exemption; failure
  postcheck cannot first pin or rebase. No `.git` contents read/write/open.
- Existing selected root0700, ancestry ownership/modes/path-to-FD identities and
  explicit private/control same-inode refusal stay. Root/role checks repeated
  during every normal boundary and independently before failed close/constructor
  teardown. Constants do not prove genuine current historical-marker custody.
- Historical file opens488 remain two fixed names, RDONLY/NOFOLLOW/CLOEXEC/
  NONBLOCK and radio-then-reset EX|NB flock491. New file opens508/510 remain only
  under the private op directory502. No operation output under historical root.

## External effects inventory and unchanged helpers

Source-only callsite lines (not executed):

- `os.open`:313,318 directories;488 fixed historical file;502 private op dir;
  508/510 fresh new marker/ledger. `os.mkdir`:501 private root only.
- `os.write`:758 only `_write_once`, unchanged actual callsites: constructor527
  new marker, append775 new ledger. No original write/create/chmod/truncate/unlink.
- `os.fsync`:506,528,529,530,760,782 only new private files/dirs; no retry.
- `fcntl.flock`:491 historical EX|NB; `fcntl.fcntl`:660 metadata and847 source dup.
- `os.close`:354,902,908,916; original clear/pop-before-once-close bodies unchanged.
- `os.pread`:429 historical bounded raw;594 marker;604 ledger streaming;668 source.
  `os.lseek`:662 source position query only; `os.getuid`:467 explicit constructor.
- New external call379 is read-only expected Git-boundary stat. Other stat/fstat
  roles remain association/metadata checking. No process/clock/source-file exec,
  UART/RPC/network/current-state collector or import-time authored IO.

`SOURCE_DELTA01.json` records all changed/new/unchanged definitions and exact
text-diff fingerprint against retained source, not a runtime test. Frozen types,
fixed safe errors, source hashing/dup, write poison, teardown, local phases,
limits and stdlib imports are unchanged at AST-definition level. No C2 effect
profile, native admission, timing resample, parser/transfer/recovery/media change.
Independent R1/R2/path-role review remains required; no next operation is queued.
