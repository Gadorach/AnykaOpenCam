# Actual private span-runtime / sender composition — contract BEFORE code

2026-10-24. CAP-01/05/06/07, CORE-03/04, RP-04/09, B07; bounded
STREAM-01/02/03/07. Isolated SOURCE lane, not adoption. No candidate invocation,
including HOST. Only public source/evidence reads, offline compilation/link and
ELF/archive metadata inspection are selected. No hardware or execution queue.

## Feasibility from actual source

The accepted runtime includes the actual native adapter in ONE TU. Its
`idle_source` calls `nh_native_retire`, and sets `r->retired=1` only on success.
The real common bridge dispatches that idle callback only for its STOP work,
then checks time and completes that exact work. Mere STOPPED (including freshly
initialized STOPPED) is NOT actual native retirement.

`native-watchdog.inc:105–116` refuses COMMITTED, executes the existing guarded
retirement, requires no native/source fault and successful logical watchdog
disable, then releases resident CPU output. `native_retire_body` performs the
actual lifetime observation and `idle_h264_checked`. `continuous_retire` requires
the keyed ACTIVE receipt, sensor-off, END, retired/END query, receipt ACK, each
source close-once and encoder observation/close; it then sets QP_CLOSED.
Successful close/disable is actual local acknowledgment, not electrical safety,
external survival or strict C1 admission. No new query/END/disable/close is needed
or permitted in borrow relinquishment. The watchdog FD remains owned/disabled.

The sender-v3 real tail already closes clients, STOPs/drains the SAME hub, checks
all four canonical tokens, retires the common producer, and checks STOPPED/no
demands/no active work. Its final runtime cleanup cannot return with the current
borrow history. Insert the explicit normal boundary there, before final cleanup
and guard unbind. Never manufacture the missing native retirement for a
COMMITTED/never-started session: that path retains, even if the hub is drained.

## Compact exclusive replacement

Fresh runtime C/header and sender C/header only, plus notices and a fixed offline
recipe. Include the accepted span-runtime native adapter/headers and actual
imports unchanged, and compile accepted memory/reader/reserve, v2 hub/span and
the unchanged real sensor with complete selected headers. No copied native tree,
old runtime/server/memory/flat-hub object or old combined REL. SOURCE_API3 changes
runtime layout; rebuild EVERY consumer. Original auth/backend/prestarted and
provider/FD/watchdog implementations stay unchanged. No new child/fork/init/
destroy/bootstrap/main in serve. One typed entry supplies the SAME runtime's
owner/source/config/paid hub and persistent real nonreturning native sink.

## Normal boundary and scalar history

Before the unchanged actual native retirement, validate the live paid association
against canonical chunks (ACTIVE, or STOPPED only for retirement). Preserve the
optional producer output's numeric extent BEFORE its existing native free; never
read a freed output pointer afterward. Latch the retirement attempt before calling
native, and its acknowledgment only after successful actual return. Failure
retains, without clearing history or retrying.

At the actual sender tail verify exact gate function/context binding, actual
closed/clean clients, no AU leases/pending control/deferred cleanup/uncertainty,
no backend pending/peer/revoke/failure/source HOLD, and real common STOPPED/no
demands/active/pending/frame/bridge clock fault. Inside the private runtime require
all original native-retired facts (except the active CPU hub borrow): real
`r->retired`, QP_CLOSED, every staged/adopted sensor/frame/encoder/legacy FD absent,
no receipt/CPU loan/resident/parked/close uncertainty, and QW_DISABLED with the
actual owned FD still >=3. Require the normal END receipt retained by that path.
The SAME live hub must be STOPPED/drained, unsubscribed, no pending/acquired bits,
all four active tokens zero, with the exact canonical paid ledger/retained owner,
all eight integer ranges/count/interior-page credit and raw/optional-output
association still matching. No receiver Boolean or opaque retirement receipt.

Only after ALL checks, without further callback/refusal, record the hub owner's
numeric uintptr_t identity, clear live `r->span_hub`, `ledger.hub.owner` and
`span_retained.owner`, and latch terminal normal-borrow-relinquished. Preserve
every attempt, payment, range/count/credit, failure and native-retirement history;
never freshen/reset/reuse the ledger. Later ready/setup/release/cleanup reject or
validate native retirement BEFORE any retired hub access. Pre-memory return
explicitly rejects every new scalar/live/terminal history field. Terminal flag
alone never makes cleanup safe. No later memory gate or new native session can
use the terminal ledger. Runtime/config/native owner remain persistent; borrower
release is not lender destruction, hub disposal or watchdog giveback/close.

## Refusal / retention and unchanged semantics

Known preborrow invalid identity may return without effects. Preserve original
BUSY-first and one-use owner/listener/outcome publication. Ordinary early options/
backend refusal with borrowed/native custody must pass actual final runtime
cleanup, not unwind into a destructor. Every failed validation/observed source,
clock/native/client uncertainty after borrow retains CURRENT state via real
first-error latch/fence/quarantine; sender retains actual frame/span before
cleanup. Sink unexpectedly returning still uses real native retention/fallback.
No old-hub cast or NULL fakehub is sent to config.retain_uncertain.

V3 retain-before-cleanup and detach-ACK ordering, auth, consumer tokens,
post-BEGIN/precopy gates, copy → producer release → exact END, sequence/partial
send/deadlines, original fixed2s/10s/128-frame limits and accepted memory debt
remain. No ready Boolean/synthetic provider/callback/clock/receipt framework.
Trusted valid stable disjoint private storage, serialization, immutable callbacks,
no aliases/reentry/signals/concurrency/later fork or FD mutation, real last-fork/
provider/watchdog ordering and independently admitted backing/recovery remain
prerequisites, not metadata proof.

## Evidence and open gates

Fixed fresh output directory; exact argv/tool/source/dependency/log/artifact pins,
preserved failures and rooted production REL closure. Reuse only audited archive
roles not consuming changed layouts. Preserve three raw LOCAL/NOTYPE Digest UND
names with zero retained relocations; libc/compiler/ARM __ehdr_start are normal
REL dependencies, not final-link/native ABI evidence. No program/helper/reader/
credit/prefault/native/socket/verifier/child/function/test execution.

Independent ordinary review and parent adoption required BEFORE use. Native1MiB
output/raw cap, allocator/library/stack/socket/process2MiB,40MiB/CMA/TLS/combined
load, external survival/recovery/backing, provisioning/main/entropy/image ABI and
strict C1 remain open. Last actual camera boundary OFF/finalized23h; no current
lease or hardware permission is inferred. Existing failures remain unchanged.
