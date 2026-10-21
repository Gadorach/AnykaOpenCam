# Optical preview — bounded OFFLINE candidate, parent review required

## Latest lifecycle correction — Oct20, stopped / not reopened

User's expired trial window was stopped using pidfd-bound SIGTERM after exact
PID/argv/start validation; no decoder child or active operation HOLD existed.
Read-only target postcheck verified lights/pairLOW, used motor coils retired,
capture/WD idle and UARTerrors0. No new actuation. See
`docs/qualification-plans/stock-reference-20261013/optical-live01/GUI_CLOSE_FIX_20261020.md`.

Known pre-TX refusal now has a trusted `NotAdmitted` no-send outcome: controls
revoke but Close is allowed. Native/post-TX failures may never use that outcome.
Worker-side pure receipt validation records actual completion time, so a queued
timely response is not falsely timed out merely because Tk polls late. Actual
late/unknown outcomes retain the sticky fault/HOLD. The UI stops new requests with
90s admission life left, ahead of the backend75s floor.71 guarded fixtures including
actual mocked GUI-close paths pass (`20261016-optical-viewer/run16`); no repaired
native window test or automatic reopen occurred. Earlier checkpoints below are history.

Task: **20261016 optical viewer**, separate from the legacy peripheral/audio GUI.
Only `optical_app.py`, `optical_video.py`, `optical_control.py`, `test_optical*.py`
and this document are added in this tool directory. Existing `app.py`, backends,
requirements, launcher, tests and installed FreeSimpleGUI notices are unchanged.

**The initial delegated unit launched no GUI, FFmpeg, stream, UART, SSH, target
command or real network connection.** Parent later ran a localhost synthetic RTSP
decoder test described below; no camera connection or private profile was created. Source
and mock acceptance are not live video, kernel75 admission, optical movement or
electrical/timer qualification. Parent alone owns hardware and live integration.

## Dedicated entry (instructions only — not executed here)

```sh
work/peripheral-gui-venv/bin/python -B \
  apps/qualification/stock-reference-20261013/peripheral-gui/optical_app.py
```

Uses the retained venv; no install or changes to `run.sh`. Startup shows a blank
host and blank image and makes no decoder/driver call. Enter **only a numeric
RFC1918 IPv4 host**, session duration integer **1..300 seconds** (default 60), then
explicitly click **Connect preview**. There is no hardcoded camera address,
discovery, credential, user-supplied URI/path/query or public/IPv6/link-local host.
`--loopback-mock` additionally allows 127/8 preview fixtures, never control admission.
This is only for the parent's controlled owned bench, not arbitrary RTSP servers.

The fixed `/usr/bin/ffmpeg` child uses RTSP interleaved **TCP**, protocol whitelist
`rtsp,tcp`, video stream 0, no audio/subtitles/data, one decoder/encoder thread,
at most5 selected source frames/s and aspect-preserving scale/pad to **640x360 RGB8 PNG** through
stdout. Input probe 1 MiB/1s, RTSP timeout 2s, individual FFmpeg allocation cap
16 MiB. The worker enforces wall duration independently of FFmpeg `-t`. No frame
is saved; there is no recording, snapshot, voice capture, URI/stderr log, FFREPORT
or inherited proxy/preload environment. The RTSP URI is necessarily present in
the local child's argv; no credentials are accepted. Parent must trust/review the
owned RTSP endpoint, including redirects/SDP: protocol whitelisting is **not** a
destination firewall or security sandbox for malicious servers.

Selection drops excess source frames without manufacturing duplicates from a
slow stream; passthrough output uses an explicit1/90000 encoder timebase. Desktop
decode arrival is not proof of sensor/capture freshness; native capture receipts
and the optical comparison remain required.

The PNG parser checks CRCs, exact format, chunk grammar, zlib integrity, row filters
and exact decompressed pixel count. Maximum encoded frame **768 KiB**, read chunks
16 KiB, partial buffer at most frame bound + one read. There is **one overwriting
latest-frame slot**, never an event queue of images; pipe backpressure is bounded.
PNG decompression is capped at 691560 bytes (+1 overflow detector). Any stderr at
error log level is a terminal decoder error; at most one 16 KiB read is examined,
no text is retained or surfaced. Python errors surface only fixed public messages.

Stop/end/error/child exit, wall duration, or last frame older than **3s** invalidates
readiness immediately on snapshot. Stall is terminal, not pause/resume. There is
no reconnect loop; Stop retires the old decoder, then explicit Connect creates a
new session. Only this owned local FFmpeg object can receive terminate, followed
by kill after 1s if necessary, wait/reap and descriptor close. No process-group,
remote PID/helper kill, target OFF or recovery command exists. GUI calls stay on
the main thread; decoder/control workers communicate through bounded slots.

## Controls: ADAPTER NOT YET ADMITTED

The shipped CLI deliberately has **no transport** and cannot enable effects.
Read controls status explains that the parent live endpoint is missing. There is
no invented SSH server on kernel75, executable selector, command string, profile
loader, UART/SSH fallback or network daemon in this unit.

`optical_app.run(sg, driver=..., epoch=...)` is the explicit parent integration
seam. The default `UnavailableDriver` has no effects. Tests inject a mock driver
and exercise actual GUI dispatch, not fake buttons that merely change text.

Parent integration must supply a reviewed driver implementing:

```python
status(request: Request) -> bytes
act(action: str, request: Request) -> bytes
```

The explicit request parameter is required for fresh client nonce binding; it is
not optional state discovered by the GUI. `Request` holds a unique lowercase
hyphenated UUID `request_id`, immutable parent `Epoch`, and exact action. Methods
must return within **8 seconds**, never retry, and return at most **4096 UTF-8 JSON
bytes**. The future remote adapter **must implement the existing shared operation
lock and durable uncertain-completion HOLD protocol**, including parent operations
and other GUI entry points. No remote adapter is implemented here, so no new lock
or live admission is claimed. An arbitrary Python object injected into `run()` is
trusted code, not a secure plugin sandbox. Parent must separately review transport,
lease retention, endpoint/helper pinning, timer and actual runtime evidence.

`Epoch` binds host, generation UUID, actual boot UUID, integer Unix `expires_at`,
kernel SHA256 and helper SHA256. Host must match the connected preview host;
at least 10s admission life must remain before dispatch and receipt acceptance.
The expected kernel label is exactly `kernel75`; hashes are exact lowercase 64-hex
pins. The GUI reads no private file and does not synthesize an epoch. A parent
epoch is an admission statement, not proof by itself of running image identity.

### Strict receipt schema v1

Every receipt is one JSON object with **exactly** these keys. No unknown keys,
duplicate keys (at any depth), nonfinite values, bool-as-integer, alternate types,
unknown completion/state, stale nonce, wrong generation/boot/action/pin is accepted.

| Fields | Required value |
|---|---|
| `schema` | `optical-control-v1` |
| `request_id`, `generation`, `boot_id`, `host` | Exact request/parent epoch values |
| `action` | `status`, `pulse-a`, `pulse-b`, `ir-on`, `ir-off`; exact request action |
| `completion`, `admission` | `COMPLETE`, `ADMITTED` |
| `kernel`, `kernel_sha256`, `helper_sha256`, `expires_at` | `kernel75`, exact epoch pins and integer expiry |
| `release` | String `00`, including status/IR actions; independently proven pair release |
| `pulse_ms` | Integer 10 for A/B; integer 0 for status/IR LED |
| `ir_state` | Exactly `OFF` or `ON` |
| `timer_owner`, `timer_remaining_ms`, `timer_bound_ms`, `proof` | Consistent state tuple below |

The only legal state tuples:

* OFF: `ir_state=OFF`, `timer_owner=none`, remaining=0, bound=0,
  `proof=OFF_CONFIRMED`.
* ON: `ir_state=ON`, `timer_owner=target`, integer remaining=1..5000, bound=5000,
  `proof=TARGET_TIMER_ARMED`.

`ir-on` requires ON; `ir-off` requires OFF. Status and pulse receipts may describe
either consistent IR state. `pulse_ms=10` is nominal intent/receipt, **not measured
electrical width**. `release=00`/timer proof are contract assertions the future
driver must derive from reviewed native receipts, never invent from child exit.

Controls require a fresh decoded frame **and a separate manual controls-status**.
Fresh video alone, restarting preview, or a late status from a revoked preview
session cannot admit controls. No A/B day/night mapping is asserted. Actions are
serialized, no repeat/sweep exists, cooldown **2s after completed action**. IR LED
OFF is independent of IR-Cut and deliberately bypasses cooldown/ON-timer blocking,
but not serialization, current preview/admission or sticky fault. IR-Cut A/B is
available after cooldown during independently timed IR illumination. A second IR ON
remains blocked until a native receipt proves OFF, even after5s locally;
the desktop never assumes timer expiry is native OFF proof. Automatic OFF must
run **on the target** and survive desktop disconnect/exit; no desktop auto-OFF
thread or auto action is used.

Exception, timeout, malformed/mismatched/UNKNOWN receipt produces **sticky UNKNOWN**:
no retries, fresh-frame reset, automatic status/OFF/kill or automatic recovery.
Pending worker/state are retained; Close stops only local video and refuses to
silently abandon the control. Parent retirement/recovery is required. A future
bridge must retain its durable HOLD even if the desktop is forcibly terminated;
this offline interface cannot guarantee another process's lock behavior.

## Offline validation / evidence

Development command (mocks only):

```sh
work/peripheral-gui-venv/bin/python -B -m unittest discover \
  -s apps/qualification/stock-reference-20261013/peripheral-gui \
  -p 'test_optical*.py' -v
```

Initial development run: **37 tests, 1 fixture assertion failure**. The assertion
mistakenly searched all layout disclaimer text for `ambient`, rather than checking
control keys. Corrected without adding/removing a control. Later additions test
GUI close during a pending effect and explicit out-of-range duration refusal.
This failed development observation is retained here, not presented as a pass.

Final guarded run, fresh directory only (refuses overwrite):

```sh
work/peripheral-gui-venv/bin/python -B \
  apps/qualification/stock-reference-20261013/peripheral-gui/test_optical_app.py \
  --evidence evidence/delegated/20261016-optical-viewer/run01
```

`run01/tests.txt` and `run01/result.json` record exact command, timestamps,
Python/FreeSimpleGUI versions and SHA256 of every new source/test/document input.
The runner denies actual subprocess/socket operations, device/private/unrelated
file opens, and uses only synthetic public epochs, fake process pipes/selectors
and a mock desktop. It does not import/launch FreeSimpleGUI, execute FFmpeg, read
profiles or touch hardware. Test outputs distinguish mock success from native
qualification. Any pre-existing legacy files remain outside this unit's ownership.

## Parent review and actual localhost test

Parent corrected the initial ON-timer admission rule to permit IR-Cut A/B during
independent timed IR illumination after cooldown; rearming IR ON still needs a
native OFF receipt. A dedicated regression exercises that requested workflow.
Parent also removed synthetic `fps=5` duplication, preserving source-frame
selection and revoking stale readiness rather than refreshing it with duplicates.

`run02` independently passed39 guarded mock tests. The evidence runner now accepts
fresh `runNN` names instead of hardcoding an unreproducible existing `run01` path.

Actual FFmpeg+our loopback-only RTSP fixture was then exercised with generated
public `testsrc2` H.264, **not footage or a live camera**. Failures are preserved:
`loopback01` hit full system `/tmp` during C compilation (subsequent builds use an
owned TMPDIR); `loopback02..04` stopped on nonmonotonic PNG-mux DTS because the
input-declared timebase was too coarse for the selected frames. Explicit encoder
timebase1/90000 fixed that without ignoring FFmpeg errors. `loopback05` produced25
decoded preview frames, expired after5s, invalidated readiness and reaped its own
decoder/server.24 sequences were sampled by the observer; neither count proves
distinct scene content (the fixture deliberately repeats one IDR).

Reproduction (fresh loopbackNN required, binds **127.0.0.1:8554 only**):

```sh
work/peripheral-gui-venv/bin/python -B \
  evidence/delegated/20261016-optical-viewer/real_loopback.py loopback06
```

The harness records stderr only for its own synthetic fixture to diagnose local
errors; the production decoder still suppresses it and never records camera frames.
No actual GUI launch, native camera feed/control or optical acceptance is claimed.

**Next task:** integrate the separately reviewed exact-kernel75 endpoint under
shared lease/HOLD, then perform the freshly admitted finite feed-only native
baseline before enabling effects. The shipped CLI still has no control transport.

Parent integration work adds `optical_uart.py` and its mock tests. This is an
**explicit private-recorder transport candidate**, not automatic UART fallback.
It remains pre-transmission disabled until the separately built ARM C helper's
receipt envelope is reviewed. No usable profile or CLI control switch is created.
`run04` passes44 guarded tests including mock shared-lock/durable-HOLD behavior;
those mocks are not native UART or GPIO qualification. The default viewer still
constructs no transport and cannot actuate. Existing legacy GUI files are unchanged.

## UTC Oct20 integration checkpoint — native gate still closed

The explicit `--uart-profile PATH` entry is now wired to the parent private-recorder
adapter; it is **pre-TX disabled** pending native qualification. It neither opens a
serial device nor falls back to SSH/legacy UART. Default preview still constructs
no adapter. No usable ADMITTED profile has been created.

The fixed ARM CLI was corrected because the exact RAM monitor waits only for its
specific ash child and cannot be assumed to reap orphan holders. Parent reproduced
the no-fork60304B CLI/SHAeaded5d3... in `20261016-optical-cli/run09`:107 optimized and
sanitized cases/90 parsed receipts per mode. `--ir-on-hold` emits its checked ON
receipt then holds the same fd in that same finite process; no fork/rearm. The
historical fork lease is always rejected by the desktop native parser.

`optical_dispatch.sh` must be **sourced in the already-owned persistent ash** and
checks its admitted shell PID, kernel/boot/PID1 and pinned helper/module. A pinned
single-line caller verifies dispatcher bytes before sourcing. Only five optical
actions exist; SETUP is not exposed. The direct background IR job redirects to
new no-clobber private RAM nonce files; launch notices precede BEGIN, one complete
JSON line is required, and integer-only BusyBox sleep bounds receipt waiting to
two seconds. The kernel4900ms timer stays primary. No nested exiting shell or
automatic retry/rearm; job exit/accounting is separate from the early ON receipt.
Parent must prepare the private RAM receipt directory and qualify actual ash
job messages/reaping and fd isolation before admission. No JSON text is executed.

The profile additionally pins exact dispatcher/new CLI bytes and the persistent
shell PID; it must attest feed retirement and native bench controls, not optical
movement. Expiry must be75..600s at dispatch. Unknown/malformed/contradictory
completion retains the durable shared HOLD even if a client exits.

`20261016-optical-viewer/run07` passes46 guarded mock cases. A separate host-shell
fixture (`evidence/delegated/20261020-optical-dispatch/test_shell.py run01`) passes10
actual Bash dispatch/framing/direct-parent/wait cases with **synthetic getters and
helper only**. Its copy substitutes three executable/RAM paths, not predicates.
It checks no effects on wrong boot/shell/existing file, preserves existing data,
and rejects partial/no-newline or stderr responses. This is not native BusyBox,
real module/IR, electrical timing or optical proof. Control admission remains closed.
