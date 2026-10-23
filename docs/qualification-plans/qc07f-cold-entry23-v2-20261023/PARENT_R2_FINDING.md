# Additional ordinary runtime-interface finding — tty mtime

2026-10-23. V2's R1 HTTP correction received narrow conditional review, but it
has NOT been invoked. Parent dependency/preflight inspection found another
concrete role mismatch before selecting hardware execution.

`identity()` includes st_mtime_ns and is correctly used for immutable sources,
historical markers and private input files. V1/v2 also use that entire tuple as
`port_meta` before serial open and compare it during every child-identity check.
TTY mtime is not an immutable ownership field: ordinary writes may update it.
Parent read-only metadata inspection of the selected tty found its current mtime
within the already finalized cold23c capture interval, while ctime predates that
interval. No new serial open, request or power effect was used to discover this.
Actual timestamps/selector and provenance remain private outside Git.

Before a new power cycle, obtain focused ordinary review of this finding and a
minimal **separate tty-role association** tuple: retain device/inode/UID/GID/mode/
nlink/rdev/ctime association, but do not treat normal tty IO mtime as immutable.
Continue strict full byte/hash/mtime/ctime checks for ALL regular/private/source/
marker inputs. Do not drop those guards globally, normalize a changed device,
ignore rdev, weaken source pins or claim hostile-root/global-owner exclusion.

If confirmed, preserve v1/v2 and their reviews, create a fresh R2-only derivative
with that typed role distinction and repeat independent source review. No
capture/ON was invoked to demonstrate this defect; no retry or native effect is
selected here. Isolated system pyserial availability was also observed, without
importing the candidate or opening serial; no dependency installation required.
