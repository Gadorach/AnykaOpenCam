# Offline method notes / retained non-build failures

This lane builds a distinct kernel; it does not try to reproduce compiler23.1.1.
Run01's configuration refusal and the reviewed fresh-run02 tracing correction
are detailed in `METHOD_CORRECTION01.md`.

Before the first full compile, two metadata-collection interruptions occurred:

1. An ad-hoc prebuild collector successfully saved observed host ELF inputs, then
   attempted a broader installed `/usr/bin` census. Read permission was denied on
   the unrelated `/usr/bin/cupsd` service binary. No service/binary was executed,
   no permission was changed, and no kernel compile was invoked by that wrapper.
   `inputs/prebuild-loaded-host-ELF.json` and the exclusively created empty
   `inputs/installed-host-bin.json` remain preserved in build02. The unrelated
   privileged executable is not a kernel toolchain dependency; no privileged
   read/retry is selected. The eventual collector is restricted to the selected
   standard build tools and actually observed loaded host ELF paths.
2. The first `precompile --run 02` invocation exceeded the shell's120000ms limit
   while redundantly hashing the same large LLVM libraries for each of424 loader
   trace files. It had not written its receipt/input manifest or started a kernel
   compile. The correction deduplicates paths **before** hashing. It changes no
   compiler flags/config/source or selected capability; a later invocation writes
   fresh previously absent `precompile-tools-and-ELF.json`, `PRECOMPILE.json` and
   `recipe-at-compile02/`. No earlier outputs are removed/overwritten.

Static Kbuild inspection also corrected the proposed compile targets **before any
full compile was invoked**: this kernel's `scripts/Makefile.modpost:104–110`
generates `vmlinux.symvers` for built-in-only compilation. Top-level
`Module.symvers` is not a standalone target. The final invocation selects only
`zImage vmlinux`, obtains real `vmlinux.symvers`, and does not invoke `modules` or
manufacture an external-module ABI receipt. No config symbol is changed to work
around this distinction; the effective selected config has zero `=m` symbols.

Exact final top-level argv/environment/UTC/builder pins are retained in
`commands.jsonl`; verbose Kbuild logs and saved `.cmd` files retain expanded
compile/assembler/linker recipes and dependency inputs. Source snapshots at the
actual compile boundary retain the final reviewable builder/packager/recipe bytes.
This is not syscall-level tracing or independently reproduced build acceptance.

All interruptions are author-method evidence, not observed kernel/native bugs.
No program/unit/runtime/security/mutation test, target execution or hardware
operation was performed, and no original evidence/source/build output was edited.

After full kernel compilation succeeded, the first packager and inspector CLI
invocations both failed **before argparse completed**: naming the newly authored
helper `inspect.py` shadowed Python's standard-library `inspect`, causing
`AttributeError: module 'inspect' has no attribute 'signature'` in Python3.14's
dataclasses/colorized argparse initialization. No packager logic ran, no image
existed, and no build output changed. Both original helper/packager bytes and the
raw inspector failure log are retained under build02 inputs/logs. The correction
renames only the authored helper to `inspect_build.py` (with a clarified docstring);
the packager and kernel/config/payload bytes remain unchanged. Subsequent CLI
invocations use fresh log/receipt/image names. This is an ordinary author naming
defect, not a tool-policy rejection, target fault, or reason to relax any guard.

The next packager attempt correctly passed the original uImage/DT/CRC checks,
then refused with `BadGzipFile` before any image was created. The authored initial
packager had incorrectly assumed GZIP for the **kernel** as well as initramfs.
Actual original/new configs both select `CONFIG_KERNEL_XZ=y`, while initramfs
selects GZIP; `.piggy_data.cmd` proves `scripts/xz_wrap.sh` plus an appended LE32
uncompressed size. No build/compression option changed. Frozen `package.py` and
`PACKAGE_FAILURE.json` retain that error. Fresh `package02.py` uses the actual
XZ stream excluding its verified four-byte size suffix and compares decompressed
bytes to the real Image; its separate `PACKAGE02.json` records the successful
format/association inspection and new image. Its shared `uimage`/ELF/FDT readers
are imported from unchanged `package.py`; the failed main is never called.

The successful output uses precisely the original legacy header/address/outer
compression/DT/initramfs scheme. Corrections affected only host evidence helpers,
not the sealed kernel source, actual built ELF/config, monitor or payload policy.
