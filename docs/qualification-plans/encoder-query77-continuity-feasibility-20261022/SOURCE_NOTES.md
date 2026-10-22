# Source anchors, revision scope and limits

## Scope of kernel deductions — NOT the applicable host source

K denotes the public retained tree at
`work/lightbulb-camera/foundation-20261011T041148Z/artifacts/encoder-query77-source02/linux/`.
K/Makefile2–5 identifies **reference source6.18.55**. It is the retained target
build tree, used here only for pinned illustrative control-flow deductions.
**Nothing identifies it as the executing host source or configuration.** No host
uname/config/proc/kernel/namespace/service query occurred. The installed
`/usr/src/linux-cachyos-lts/Makefile` also says6.18.55; its substantive requested
`.c` files are absent. That Makefile/header installation does not identify what
the host runs and is not a substitute for source applicability evidence.

Upstream v6.12 source fetches failed (METHOD.md). Therefore no v6.12 implementation
claim, pristine upstream equivalence, applicable-host proof or downloaded Linux
revision pin is made. Exact K byte pins are in LOCAL_INPUT_PINS01.tsv. This is not
a target-to-host inference or a complete timekeeping/suspend audit.

## Timerfd: cancellation evidence, consumption and notification races

| Reference anchor | Source finding |
|---|---|
| K/fs/timerfd.c95–114 | `timerfd_clock_was_set` samples current host mono-to-real offset once, scans cancel list, sets `moffs=KTIME_MAX`, increments ticks and wakes if stored offset differs. This is an offset comparison at notification processing, not a stored sequence of every clock mutation. |
| same149–173 | `timerfd_canceled` recognizes the sentinel then resets moffs to current offset. CANCEL_ON_SET registration requires REALTIME/REALTIME_ALARM plus both flags. Source registration/source association must be established before anchor. |
| same187–229,454–523 | setup resets ticks/expired, starts configured timer, may then return ECANCELED; settime is not a harmless observation. It changes cancel registration and reprograms state. No rearm/clear/recreate is admissible within the old epoch. |
| same246–318 | poll observes ticks, not why ready. Read consumes cancellation, clears ticks/expired and returns ECANCELED; normal expiry has a different tick-return path. A later copy/read failure is not clean negative evidence. Sticky failure must be retained by sole owner immediately, not inferred from repeated reads. |
| same232–243,350–380,394–451 | Last close removes registration; fd is shared through aliases. TFD_IOC_SET_TICKS can consume cancellation too. ALARM clocks require CAP_WAKE_ALARM. No alias, ioctl, close/reuse, exec-loss or unrecorded read may be silently tolerated. |
| K/kernel/time/timekeeping.c1441–1464,1522–1532 | Time update lock is released before `clock_was_set` notification. In these paths notification is synchronous before setter return, **not** atomic with publication to other threads or serialized with all future setters/effects. |
| K/kernel/time/hrtimer.c933–984 | Timerfd notification follows CPU offset/interrupt updates; other clock changes can interleave before timerfd samples the offset. There is also a work-scheduled notification path. No userspace poll/read is a flush-all-clock-writes barrier. |
| K/kernel/time/timekeeping.c1938–1995; timerfd.c116–129 | Resume's timerfd update is scheduled work. No source bound or userspace activation fence here proves resume notification has run before the parent next executes. |

**Deduction, not executed interleaving/test:** once the sentinel is set, compensating
offset changes do not themselves restore a clean state before consumption. But
compensating mutations that restore the offset before notification samples it are
not ruled out by the offset-comparison implementation. A clock write can also
publish before cancellation processing completes. Thus the implementation cannot
be promoted to an unsuppressible wall-step history or preventive interlock merely
from the API name. No claim that every ordinary sequential setter is missed: the
synchronous path is explicitly distinguished above. Actual-host source matching
and concurrency/continuity assumptions are still missing even for this reference.

## Suspend counters and event delivery

| Reference anchor | Source finding |
|---|---|
| K/kernel/power/suspend.c365–390,569–615 | suspend_enter begin trace precedes freezing; suspend_prepare invokes freeze processing; a second suspend_enter marker occurs BEFORE device/system sleep, not a generic resume-completed marker. Test/error paths also exist. |
| same553–557,625–636; K/kernel/power/power.h275–312; process.c179–212 | enter_state calls suspend_finish, whose configured thaw helper wakes tasks and calls schedule, before returning to pm_suspend and its dpm_save_errno. With freezer disabled the helper is a no-op, not stronger coverage. |
| K/kernel/power/main.c333–383,410–429 | Success/fail counters are unsigned int; increment is return-path bookkeeping. Read-only per-attribute sysfs shows counters, not a synchronized lifetime barrier/event sequence. Two failure-history slots rotate; no timeless unique event identity or nonwrap guarantee. |
| K/include/trace/events/power.h266–286 | Tracepoint payload is action,val,start, no intrinsic gap-detecting global sequence. Actual loss-aware transport/scope is additional. Hibernation/all firmware/emergency paths were not closed by this suspend.c slice. |
| K/kernel/events/ring_buffer.c174–207,248–270 | No ring => no record; pause/full buffer can count loss. PERF_RECORD_LOST is emitted on later successful reservation, so an empty stream/no LOST record alone does not exclude trailing loss. |

**Deduction:** resumed userspace may run before a success counter advances. No
change on an immediate postresume read cannot prove no suspend. Counter wrap,
boot/reset identity, failed attempts/test paths, asynchronous producers and
unaudited sleep paths are additional ambiguity. Stats are diagnostics, not
prevention, a universal event source or a pre-effect synchronization barrier.

## Version-tagged systemd source (reference v257, not installed service facts)

The three URLs/payload pins in URL_INPUT_PINS01.tsv are exact v257 source/doc
reads, not host bus/configuration observations.

| Anchor within pinned v257 source | Source finding |
|---|---|
| man/org.freedesktop.login1.xml735–749,902–912 | Inhibit returns an FD; all duplicates closing releases lock. sleep/block versus delay/weak modes and acquisition privilege differ. Documentation states cooperative login-manager semantics, not kernel-wide exclusion. |
| src/login/logind-inhibit.c173–211,282–363 | Started state, FIFO lifetime/EOF, stop/free and orphan handling. Source explicitly supports state/FIFO persistence over daemon restart; this does NOT establish zero-gap enforcement across a real restart. |
| src/login/logind-dbus.c2080–2128 | Strong block normally rejects without SKIP_INHIBITORS. Explicit skip goes through ignore-inhibit authorization. Do not claim v257 simply ignores every strong block for root; bypass is source/policy dependent. |
| same3663–3669 | Requested inhibition refuses a matching already-running delayed action. Mere FD receipt is not a proof covering every pending kernel/unit/direct-suspend operation or concurrent alternate path. |
| man/org.freedesktop.login1.xml769–782 | PrepareForSleep signals describe before/after notifications; no gap-free loss-aware historical replay or all-kernel-suspend coverage is specified. |

The XML flag prose has numeric inconsistencies beside its symbolic definitions;
no numeric bit value is adopted here. Source symbolic SKIP_INHIBITORS/check logic
is the bounded finding. Weak/delay locks cannot be treated as indefinite block.
Sleep inhibition supplies no wall-clock setter exclusion. Privileged direct
kernel sleep, service replacement and emergency/firmware/power events remain
outside this cooperative interface unless separately excluded by an admitted
host trust/configuration contract. No such contract is established here.

## Installed manual scope (decompressed line references)

- timerfd_create.2:182–205,281–319,362–385,483–528: cancellation/error semantics,
  aliases/close and rearm-even-with-ECANCELED caveat. ALARM capability:80–96.
- perf_event_open.2:49–54,90–111,994–1012,1682–1709,1982–1999,2248–2258,
  3519–3528: disabled events, privilege, lost counters, producer/consumer ordering,
  deferred loss indication. Counts/head-tail are not proof of subscription scope.
- systemd-inhibit.1:29–34,66–86: cooperative block/delay/weak, not global hardware
  prevention. Header says262.1; not the executing logind version or v257 evidence.
- time_namespaces.7:10–37,60–90,169–178: M/B namespace offsets affect timer APIs;
  REALTIME is not virtualized in documented Linux model. Need host boot/domain/
  namespace/source association; a global wall watch doesn't authenticate M/B or
  PID-start mapping. No actual namespace was opened/read.

Manual headers identify Linux-man-pages6.19; no kernel version is inferred.
Manual examples were text only, never run. C2c's own SOURCE_ANCHORS/clock model
retain the PID rounding/recorder native-deadline dependency; not solved here.

## License/provenance boundary

K source SPDX GPL-2.0/GPL-2.0-only remains unchanged; systemd documents/code are
LGPL-2.1-or-later. timerfd/perf manuals carry GPL-2.0-or-later; namespace manual
Linux-man-pages-copyleft. Only paraphrases, short identifiers and anchors are
published in these new docs; no implementation/full source/raw events copied,
no license deletion or global project license grant. Pins identify public bytes,
not authentic host execution, native admission or permission to exercise APIs.
