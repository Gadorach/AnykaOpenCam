# Oct23 native re-entry — passive observation only

User renewed unattended project execution approval, explicitly including motor
testing and onboard NOR programming if needed. This is authorization, not proof
of current wiring/runtime or permission to skip bounded procedures/rollback.
QC07F remains first; later ports wait for G8. Existing radio/reset failures stay.

Selected first hardware step: one120-second protected receive-only UART capture
using the unchanged independently source-reviewed explicit-port recorder
`apps/qualification/encoder-query77-recorder-explicit-port/uart_capture.py`, SHA256
`c8433e225675bf16ca153d3b2c700cb4d3ab80dd70083d4e4a7bc19dffc19445`.
Read/audited its complete capture/CLI path. Parent selects only passive capture,
not its TX/catch/boot capability or a native-effect controller. Private output is
fresh0700/0600 outside Git, explicit exact historical by-id adapter only; never
discover a replacement or fall back to a numbered tty. No second serial reader.

Host discovery: intended adapter is present, matching the historical real tty;
old recorder PID and active socket are absent. Visible process FD inspection and
unprivileged fuser report no owner. Root FD visibility is limited; noninteractive
sudo is unavailable. Do not claim privileged global no-owner proof. Preserve old
socket artifacts and lock contents; no kill/takeover/rename/reconfiguration.
Use original private capture behavior: request DTR/RTS false while closed,115200
8N1, no flow control, exclusive open, no TX request and no catch arm. The source
warns that OS/adapter modem-line transients and initial input flush are possible;
no new electrical waveform/connection measurement is available. Reset/boot and
adapter power connections must remain absent as in the established UART setup.

Lock both original operation-marker files without modifying their bytes. Refuse
an existing recorder/socket/visible UART owner or lock conflict. Create a fresh
private phase record before serial open; retain all capture/error outputs. Await
normal capture completion; no concurrent UART/RPC/polling/helper changes or TX.
Check final manifest, no TX/arm/control events and byte-exact original markers.
Do not print/hash private RX publicly. Classify only new observed output; silence
cannot establish OFF, held U-Boot, Linux, health or recovery. No blind newline,
reset, boot, staging, motor/GPIO operation, flash or storage write follows silence.

This observation does not rely on/establish the blocked native whole-interval
clock guarantee, timer coverage or a recovery lease: no native effect is admitted
by its duration. A fresh cold held boot and actual recovery are still required
before effectful testing. If physical power/reset/wiring action is required and
no qualified remotely controlled camera power path exists, record that concrete
blocker and continue eligible feature work; broad approval cannot perform a
physical action by itself. Do not borrow a switch outlet or invent a camera relay.

In parallel, one isolated source author advances the real private RTSP
provisioning→bootstrap descriptor handoff. It owns no hardware or central status.
Diagnostic-image/clock/receipt framework work stays parked; further adversarial
testing remains deferred. Motor tests must be bounded with neutral/STOP proof;
NOR writes additionally require exact image/ranges/current boot policy/verified
rollback and recovery, not an unattended installer shortcut.
