# Offline encoder-query77 qualification kernel builder

PLAT-01/08 and ENC-02 dependency only. New Linux6.18.55/Clang23.1.3
compiler/config/header/image baseline, **not retained76 reproduction**.

Run from the canonical workspace with Python3, only when explicitly selected:

```text
python3 -B apps/qualification/encoder-query77-build/build.py prepare
python3 -B apps/qualification/encoder-query77-build/build.py configure
# Inspect CONFIG_REVIEW.json and inputs/config.diff before compile.
python3 -B apps/qualification/encoder-query77-build/build.py precompile
python3 -B apps/qualification/encoder-query77-build/build.py compile
python3 -B apps/qualification/encoder-query77-build/build.py closure
python3 -B apps/qualification/encoder-query77-build/build.py custody
```

The actual successful selection used `--run 02`; run01 is a preserved refused
configuration attempt. After compile, the selected format/metadata tools are:

```text
python3 -B apps/qualification/encoder-query77-build/package02.py --run 02
python3 -B apps/qualification/encoder-query77-build/inspect_build.py --run 02
```

The actual successful packager was invoked through `Build.command` with the
recorded sanitized environment/loader tracing (see `commands.jsonl`). Do not run
`package.py` directly: its initial incorrect GZIP-kernel assumption/main is kept
as failed evidence; only its unchanged format readers are reused by package02.

`--run NN` selects fresh owned source/build/image directories. No unknown output
is removed. A failed phase and its outputs are retained; corrections require a
fresh log/output phase, never disguising old failures.

Preparation hashes the entire original source (SCM metadata separately), actual
final76 build and frozen query evidence. A full source copy omits only `.git`;
relative internal symlinks are checked first and copied as symlinks. Exactly the
reviewed three insertion sites/two new include files are introduced and verified
against frozen candidate bytes. License files and notices are retained.

Explicit empty `LOCALVERSION`, disabled `CONFIG_LOCALVERSION_AUTO`, and new
`CONFIG_LOCALVERSION="-qc07f-encoder-query77"` prevent inherited repository-dirty
identity without invoking Git or editing source version scripts. Fresh `O=` uses
current installed LLVM23.1.3 and host Clang. Only five explicit compiler/localversion
identity symbols may differ after olddefconfig; any other difference stops before
full build. Config features/driver guards/warnings/memory limits are not relaxed.

The actual original initramfs path remains the config input. Its bytes are pinned,
copied for custody, and compared with both original/new embedded decompressed
archives. Archive names/types/symlinks are inspected in memory, not extracted.
The retained DTB is copied byte-exact, not silently recompiled.

Only real built-in kernel targets are selected: zImage/vmlinux; built-in modpost
provides `vmlinux.symvers`, not external-module `Module.symvers`.
No `modules`, station/hgics source build, install, target program, test, generator
payload, service, listener, hardware, network or private I/O is selected.
Standard Kbuild host configuration/generation tools are the sole executable
outputs allowed. Verbose Kbuild recipes and saved `.cmd` files record expanded
build argv/dependencies; this is not syscall-level tracing. A sanitized complete
environment and dynamic loader logs retain actual host tool/library associations.
Loader traces use separate per-process files, not probe stderr (run01's refused
instrumentation artifact is preserved; see the method correction report).

All generated source/objects/images/header inventories stay in ignored `work/`.
Nothing is staged/committed. Image packaging is a separate explicit phase only
after original format, addresses and real new ELF/live extents are established.
The unchanged embedded station76 module is **not paired to query77**; loading it
is unsupported. No WLAN/media autostart is added. PLAT-02 and native/product
gates remain open, as do original radio/reset-controller failure HOLDs.
