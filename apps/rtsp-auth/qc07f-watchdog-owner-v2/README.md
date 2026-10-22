# One owned watchdog v2 — W1-W3 correction candidate, compile only

Fresh byte-derived revision of the frozen v1 app/build04, addressing only review
W1-W3. Baseline app/provenance/review custody and exact app/generated deltas are
retained in `docs/qualification-plans/qc07f-watchdog-owner-v2-20261022/`.

- W1: one-shot memory start refuses repeats before unchanged initializer/accounting
  memset. A real-clock-checked nonfuture start, <2s old, seals start+10s in the owner
  before setup. Every begin must supply exactly that deadline; failed setup fences
  rather than allowing a new session. Memory start still returns only the original
  CAMERA_OK/CAMERA_NOT_READY shape. Three memory-policy C units stay unchanged.
- W2: each non-NULL outward NEXT packet/key is zeroed before any early refusal;
  no private expected packet, backing, loan or FD is cleared to implement it.
- W3: after sticky uncertainty/HOLD precedence, a known produced/parked loan is
  BUSY before clock/control/legacy codec query. No rearm or private custody change.
  Arguments/outputs/slots are trusted stable/disjoint storage, not alias-proofed.

One moved actual preopened O_WRONLY/CLOEXEC character-device description, no alias,
second open, V write, periodic feeder/guardian or watchdog close. Take means UNKNOWN
because preopen already starts hardware. Real SETTIMEOUT5/ENABLE/GETTIMEOUT precede
risky work; only validated raw/CRC/keyed ACTIVE capture progress may KEEPALIVE once.
Real producer lifetime/cancel/deadline checks remain INSIDE the same owner interval.
Successful source/postchecks or normal retirement can DISABLE while retaining FD.
Acknowledged disable is logical only; failure may leave it stopped or unknown,
so HOLD/retained FD NEVER guarantees armed/electrical recovery or DMA retirement.

`overlay.py` derives exact frozen v6 native adapter/import into fresh run source,
not a second producer. `native-watchdog.inc` wraps the real renamed bodies. The
source2s and original10s session budgets are checked before/after control and after
metadata cost; no servicing after expired/unknown clock. No timeout grows. Sensor
exact139 preparation, CRC/receipt checks and memory forecast implementations/table
stay byte-exact; sensor code only GETTIMEOUTs the same borrowed owned FD, no ping.

Normal provider close now clears its slot BEFORE the single close attempt. An
error's former number is diagnostic NOT an actually retained descriptor; there is
no close retry. The actual watchdog is never closed by this code. Uncertain work
retains its actual remaining FDs/backing and original source process quarantine.
No error-return/metadata sink is a substitute for real failed-next process ownership.

Fresh singleton init -> move watchdog -> old explicit storage/exact139/memory
preparation -> bounded next/release/normal retire -> disabled custody give-back.
No reinitialize while owned, uncertainty repair, STOP/END retry or fake qualified
flag. Exact core/driver/image/current recovery/sole serialized custody and all
provider/backing/resource prerequisites remain mandatory; metadata cannot prove them.
Existing large v6 allocations/raw backing are STILL HOLD under128KiB/product limits.

```sh
python3 -B apps/rtsp-auth/qc07f-watchdog-owner-v2/build.py fresh-name --toolchain /path/to/retained/armv5-eabi--musl--stable-2025.08-1
```

Fresh outputs only in `docs/qualification-plans/qc07f-watchdog-owner-v2-20261022/runs/`.
Only changed real native_adapter/watchdog_owner units compile; archives/REL links
reuse the four pinned unchanged v1 object units per platform with their provenance
and dependency pins. Ordinary functional checks and every runtime/test remain NOT
RUN. No unchanged UAPI suites/driver builds, no full app/provider/RTSP adoption.
Exact original/generated source, applied diff, licenses, commands/source/header/tool
closure and HOST/ARM archives/REL objects retained. No main/listener, output/test/
synthetic/runtime campaign, kernel/module/image/native build/use or target/private
IO. New code GPL-2.0-only; original notices/licenses retained, including the
watchdog driver's GPL-2.0-or-later notice, not relicensed by this candidate.
Source review/native guard qualification and full provider/failed-next/backing/
provisioning/listener/admission remain open; no child pin/loopback/radio change or
native memory/40MiB/CMA/2MiB-vs4MiB/G2/G3/G8 acceptance follows.
