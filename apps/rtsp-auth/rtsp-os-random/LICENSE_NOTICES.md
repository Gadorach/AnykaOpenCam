# License scope

New C/header/harness and fixed evidence recipe: `GPL-2.0-only` as marked.
No global license grant for acquired firmware or unrelated project files.

The unchanged Mbed TLS `platform_util.c` compiled by this lane retains its own
copyright/SPDX `Apache-2.0 OR GPL-2.0-or-later`; use the GPL-compatible option for
this combined candidate. Original notices/text are preserved at
`apps/auth-web/service/vendor/mbedtls/LICENSE` and in that source and its headers.
Existing GPL-only camera headers remain unchanged. No vendored implementation
is copied or relicensed. Compiler/libc runtime notices remain with the retained
toolchains; generated objects/ELF are local uncurated evidence, not a release.
