# Cold-entry23 v3 — R2 typed-role correction only

2026-10-23; BASE-01/05/07 / PLAT-06/08. Parent's read-only node inspection and
independent ordinary review identified normal tty mtime updates as incompatible
with v2's immutable full-file tuple at post-catch checks. V1/v2 remain uninvoked
and frozen; preserve both reviews and the original23c caller failure.

Add a distinct char-device association helper retaining dev/inode, UID/GID/mode/
nlink, rdev and ctime, excluding only normal-IO mtime/irrelevant file size. Use
it at the existing tty baseline and child-identity check. Do not change ANY
regular/private/source/marker tuple, bytes/hash/ancestry check, source pin,
counter/catch/trailer/quiet/HTTP/owned-child/finalization or failure policy.

R1's separate POST acknowledgement and strict GET state observations remain
unchanged. This does not establish privileged/global exclusion, electrical
identity, C1/native-clock coverage, a host-hang deadline or independent recovery.
No RAM/image/boot/motor/GPIO/NOR/media effect or broadened controller is added.

Only AST/syntax and public source/delta pin checks are selected now; no imports,
calls/tests/private inputs/HTTP/UART/hardware. Focused independent ordinary R2
review and parent selection are required before ONE foreground cold observation.
