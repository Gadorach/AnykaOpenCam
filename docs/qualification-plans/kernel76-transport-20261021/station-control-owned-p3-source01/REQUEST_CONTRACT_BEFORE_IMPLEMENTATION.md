# Station76 owned-control P3 — SOURCE contract before implementation

2026-10-22. Fresh parent-selected P3 on exact reviewed P4a. No P1 integration,
generic lifetime API, full P4/P5/native or runtime/test selection.

## Local request / ref / lock contract (written first)

- Exactly one bounded heap request embeds its response/completion; no async user
  points to a caller-stack response. Static size checks bound the request and the
  skb-cb heap-pointer metadata. Original A1 wire/copy/validation bytes remain.
- Explicit refs/roles: synchronous waiter; module/device-lifetime registry;
  pending correlation list; queued UNSENT; sole ordered TX worker (queue ref
  transfers to worker); submitting return path; actual callback reservation.
  Every role mutation and refcount update is serialized by the existing receipt
  lock. A registry ref keeps the object until P4a final release, after joins/poison.
- States distinguish NEW, QUEUED_UNSENT, WORKER_PREPARING, SUBMITTING (potentially
  submitted), SUBMITTED, CALLBACK_RECONCILED, CANCELLED_UNSENT, LOCAL_FAILED and
  final RETIRED. Callback reconciliation is not proof its return tail has returned;
  only P4a poison/worker joins permit final request/private-owner release.
- Use the **actual immutable first-error lock** for both request cancellation and
  final submit CLAIM. The existing first-error recorder calls a locked request
  hook: queued UNSENT is detached/freed once; worker/USB backing is never freed
  by this hook. Other owned phases retain backing and receive sticky failure.
- Lock order: P4a bus admission → FWCTRL correlation → receipt/request, with
  permitted subsets. No receipt/request-held path takes either earlier lock. No
  callback-used lock is held across usb_submit_urb, waits, joins or poison.
- Pending list and queue publication use bus/FWCTRL/request serialization. The
  sole ordered worker claims the one queue item; no generic skb queue eviction,
  capacity retry, new producer/timer/worker or request cancellation thread exists.
- Seal one immutable absolute ktime monotonic deadline **before allocation and
  admission**, bounded by existing500ms. Check remaining budget at publication,
  worker claim, preparation/token, submit claim, response/copy and the sole wait.
  Never restart/grow it; no sleep/credit/retry loop. A sub-jiffy remaining budget
  may fail conservatively rather than request a wait longer than that budget.
- Replace selected USB1000-jiffy token wait with one nonblocking admission.
  Before USB entry reserve submitting and actual-callback request refs and token
  ownership, under admission + receipt/request locks. If error/STOP/deadline wins,
  no claim is made. If claim wins, later failure means potentially submitted;
  it is not relabeled UNSENT or a physical atomic TX-cancellation guarantee.
- Successful submit return touches only its independently referenced heap request,
  never skb/backing/reused slot. Actual callback performs skb/slot/token cleanup
  before its final request reconciliation. Submit error has NO real callback;
  the local path reconciles callback reservation, submit ref, token and skb once.
- RX uses the actual A1 association/status/copy functions. A valid signed negative
  status is latched while FWCTRL lock is held before response publication/wakeup.
  A response must also belong to a request that reached a submit claim. First
  response wins; negative/late/failed results never copy or repair terminal state.
- STOP cancels only owned UNSENT and wakes live heap waiters. P4a joins the same
  init/TX/FWCTRL producers and poisons the same48 slots before final FWCTRL release.
  Async request users cannot outlive those proofs. A role/ref discrepancy blocks
  final release with owners retained; leaking and returning is not module safety.
- Keep16TX/32RX/2560B,128KiB allocation cap and existing5s guard policy. Init/RTNL/
  poison waits remain unbounded; no2s/5s native/recovery closure is claimed. P4a
  reservation bits remain conservative owner reservations, not new request refs.

## Final outcome / evidence

Pending implementation and ordinary changed-unit HOST syntax only. The contract
itself is not a compiled, executed, independently reviewed or accepted result.
