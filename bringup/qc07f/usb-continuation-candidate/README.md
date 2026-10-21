# QC07F candidate76 — Anyka MUSB PIO continuation

NET-02 / PLAT-01 source/build increment, 2026-10-20. **Not native-qualified.**
Keep kernel75 and MTU384 as rollback. No target/serial/network/flash/card operation
is implemented by these scripts; no firmware executable is run on the host.

## Change

Stock's exact OUT routine kicks low TXCSR1 only. Our selected Linux6.18.55 MUSB
continuation and NAK-retry constant16-bit0x00a7 write clears shared TXCSR2.MODE.
Inject `musb_h_tx_pio_kick` and replace just those two call sites:

- Anyka PIO quirk **and** PIO-only configuration: byte write TXPKTRDY at TXCSR;
  no new register read, high-byte replacement, CLRDATATOG or software toggle update.
- Other platforms or DMA-capable configuration: unchanged original word kick.

Both callers already hold the MUSB lock and select the endpoint. Other reads,
initial setup, error/flush and DMA paths remain unchanged. This is a specific
correctness hypothesis, not proof that MODE was the only large-transfer failure.
No FIFO/timeout/PLL/firmware/framing/tail/credit/MTU-default change.

The byte value1 also clears low WZC flags under the register header's convention,
unlike generic0xa7. That is deliberate stock-like acknowledgement, particularly
on NAK retry, not an assertion that all status semantics are unchanged. Actual
NAK/FIFO/error/toggle behavior and any status-arrival race remain native gates.

Reference: `docs/qualification-plans/stock-reference-20261013/closeout-wifi01/REPORT.md`.
The radio's four-byte alignment/tail remains; a2052-byte wire transfer for a2048
payload is not changed into a ZLP shortcut.

## Reproduction (fresh output paths only)

From workspace root:

```sh
python3 -B bringup/qc07f/usb-continuation-candidate/prepare.py
python3 -B bringup/qc07f/usb-continuation-candidate/test.py evidence/qualification/qc07f/kernel76-20261020/usb-host-FRESH
python3 -B bringup/qc07f/usb-continuation-candidate/build.py
python3 -B bringup/qc07f/usb-continuation-candidate/test_cpio.py
python3 -B bringup/qc07f/usb-continuation-candidate/package.py
python3 -B bringup/qc07f/usb-continuation-candidate/verify.py evidence/qualification/qc07f/kernel76-20261020/final-verify-FRESH
```

`prepare.py` refuses existing candidate directories. It checks all91,268 sealed
kernel75 files, creates a separate local checkout plus exact retained75 patch and
untracked source files, verifies equality, and changes only `musb_host.c`. It copies
only sealed radio **source** inputs, never stale `.o/.cmd/.ko` outputs. Config is
identical to75 for the preliminary build. Its retained DT/initramfs are not
automatically declared coherent or product-ready; see final packaging below.

`test.py` compiles the actual injected helper with definitions extracted from the
selected kernel headers. Both actual call sites are checked. All65,536 CSR word
states × four platform/DMA combinations =262,144 cases per build. Three mutants
must compile and then fail at runtime: original word kick, omitted DMA guard and
inverted quirk guard. Tests model bus write width/values, **not** electrical/WZC
controller behavior, IRQ timing, USB ACKs or RF delivery.

`build.py` uses a fresh out-of-tree ARM build and fresh paired station76 module.
It checks release/config/exported-symbol identity and preserves original75 outputs.
It does not install, stage, autoload or certify any control module.

`package.py` deliberately regenerates `initramfs76.cpio`: replaces the stale
embedded radio with freshly built station76, replaces stale `/bin/rtspd` with the
reviewed current receipt-aware daemon, and adds the pinned ELF DHCP bridge at
`/bin/qc07f-dhcp-script-bridge`. All other archive entry records and `/init` remain
byte-exact; no extraction, autostart, private config or credential addition. A
separate final kernel output selects that cpio (only config-path delta), and a
second radio build against FINAL metadata must be byte-identical to the embedded
module before creating `images/QC6U76.UIM`. Memory ranges, linked syscall/CRC tables,
both CRC domains and actual embedded cpio are checked. Baseline75 and preliminary76
build outputs are retained unchanged. This is not complete product integration.

## Current evidence boundary

Final package succeeded: `QC6U76.UIM`3603555B, SHA256
`bd8a2a08be1483e1ddcd1efa2c68c9e20549047dec79796137979179197f654f`.
Independent verifier20 checks PASS, including actual embedded archive, paired
radio, memory/CRC bounds and rollback source/output preservation.17 current
archive tests and7 optimized-Python refusals pass. Exact completed recipes are
retained in `native-usb76-package01/recipe-snapshot/`; later host-only hardening
is recorded separately and reproduces cpio76 byte-exact. No native claim.
Full result: `docs/qualification-plans/kernel76-integration-20261020/RESULTS.md`.

Host `usb-host05`:262,144 optimized +262,144 ASan/UBSan cases,3 negative mutants
rejected with required SIGABRT and exact assertion diagnostics. Source review of
03 already verified those diagnostics independently;04 hardens future runner
acceptance;05 runs the current safety-hardened recipes. `usb-host01`/`02` are retained
failed fixture runs: unused mock callbacks
and an unread mock DMA flag prevented mutant compilation under `-Werror`; the
production helper was unchanged. Explicit mock references/state checks corrected
the harness, not the driver result. Parent/source reviews and full ARM build have
separate evidence in `docs/qualification-plans/kernel76-integration-20261020/` and
`work/lightbulb-camera/foundation-20261011T041148Z/analysis/native-usb76-build01/`.

Active ledger: `docs/plan/camera-foundation/KERNEL76_TASKS_20261020.md`.
Camera remains OFF with no UART recovery owner. Future native test needs fresh
physical/current-runtime/recovery admission and a finite reviewed recipe; no
automatic firmware reload/RUN, module load, MTU increase or boot follows a build.
