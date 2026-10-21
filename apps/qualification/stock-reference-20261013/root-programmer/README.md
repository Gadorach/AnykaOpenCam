# QC07F fixed ROOT programmer — offline candidate only

**SOURCE / static BUILD / HOST MOCKS only. Native programming UNQUALIFIED.**
Do not execute this helper against a real device, even `--check`, on the authority
of this build. The parent owns target qualification, backups, recovery and any
later exact-image write authorization.

`root_programmer.c` is a narrow standalone adaptation of pinned BusyBox1.37.0
`miscutils/flashcp.c`: per-block `MEMERASE`, 4KiB copying, complete byte compare.
It is not another MTD framework. Its only device is `/dev/mtd4`, ROOT1MiB at
physical `[0x200000,0x300000)`. Stefan Seyfried's copyright/GPLv2 notice is retained;
`LICENSE` is copied byte-exact from that BusyBox source tree (GPLv2 only). This
does not assign a license to the wider workspace.

## Offline build / tests

From the workspace root, use an unused build number:

```sh
PYTHONDONTWRITEBYTECODE=1 /usr/bin/python3 -B apps/qualification/stock-reference-20261013/root-programmer/build_and_mock.py build03
```

The recipe builds but never executes ARM code. Only `ROOT_PROGRAMMER_MOCK` builds
run; all file, MTD, mount, signal and identity interfaces are replaced by in-memory
fixtures. Evidence goes to `evidence/delegated/20261015-resume/root-programmer01/`;
fresh compiler outputs go to `work/qualification/root-programmer-20261015/`.
Existing build directories are refused, not removed or overwritten.

The final reviewed build is **build02** (119 cases each optimized and ASan/UBSan).
Read `docs/qualification-plans/stock-reference-20261013/root-programmer01/CONTRACT.md`
and `WORKER_REPORT.md` before considering any native qualification.

## CLI contract, not a target command to run

Required arguments: `--image ABSOLUTE_FILE`, `--expected-erasesize DECIMAL_BYTES`,
`--board-nor-root-only`. Omitting the mode means `--check`: read-only, no backup
creation, writable MTD open, erase or program. No geometry default exists.

Only explicit `--program-root-only --backup FRESH_ABSOLUTE_FILE` permits the
destructive path, after admission and a complete verified backup. `--backup` with
`--check` is refused. Unknown, abbreviated, equals-form and duplicate arguments
are refused. No device/address/unlock/force/whole-chip option exists.

Input must already be **exactly1048576 bytes**, root-owned, mode0600, one-link
regular file, opened with `O_NOFOLLOW`. It must already have been whole-image
hash/manifest verified privately by the parent. No padding, candidate hash or key
digest is embedded or emitted here. The parent-reported970752-byte Squashfs
component cannot be supplied directly: final partition-tail composition belongs
to the parent's private image preparation, not this helper.
