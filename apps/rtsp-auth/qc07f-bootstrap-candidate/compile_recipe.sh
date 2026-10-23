#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Fixed OFFLINE compile/REL recipe only. No app/test/generator/old builder.
set -euo pipefail
root=/home/gadorach/Desktop/AnykaOpenCam
doc="$root/docs/qualification-plans/qc07f-bootstrap-20261023"
out="$doc/runs/build01"
runtime="$root/docs/qualification-plans/rtsp-owned-runtime-20261023/runs/build02"
pp="$root/docs/qualification-plans/rtsp-provisioner-process-20261023/runs/build01"
app="$root/apps/rtsp-auth"
arm="$root/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/bin/arm-buildroot-linux-musleabi-gcc"
[[ ! -e "$out" ]] || exit 10
mkdir -p "$out/objects" "$out/logs" "$out/tmp"
export TMPDIR="$out/tmp"
run() {
 local label="$1"; shift
 printf '%q ' "$@" >> "$out/COMMANDS.txt"
 printf '\n' >> "$out/COMMANDS.txt"
 date -u +"$label START %Y-%m-%dT%H:%M:%S.%NZ" >> "$out/TIMES.txt"
 "$@" > "$out/logs/$label.txt" 2>&1
 date -u +"$label PASS %Y-%m-%dT%H:%M:%S.%NZ" >> "$out/TIMES.txt"
}
run input-pins /usr/bin/sha256sum --check "$doc/BUILD_INPUTS.sha256"
common=(-std=gnu11 -Wall -Wextra -Werror -fno-common -fno-strict-aliasing
 -ffunction-sections -fdata-sections -fstack-protector-strong -D_FORTIFY_SOURCE=2
 -fstack-usage "-I$root/framework/include" "-I$app/isolated-verifier")
for arch in host arm; do
 if [[ "$arch" == host ]]; then cc=/usr/bin/cc; flags=(-O2); else
  cc="$arm"; flags=(-Os -march=armv5te -marm -mfloat-abi=soft)
 fi
 archive="$runtime/libqc07f-owned-runtime-$arch.a"
 # Native-shaped process/handoff/constructor + missing Digest/MD5 ONLY.
 # Graph/wire/parser/SHA/constant-time/zeroize come from runtime archive ONCE.
 objects=()
 for n in 0 1 2 3 9; do objects+=("$pp/objects/$arch-$n.o"); done
 run "$arch-version" "$cc" --version
 run "$arch-machine" "$cc" -dumpmachine
 # Read-only real member/header/symbol inspection BEFORE any archive is linked.
 run "$arch-members" /usr/bin/ar t "$archive"
 run "$arch-archive-ELF" /usr/bin/readelf -h -A "$archive"
 run "$arch-archive-symbols" /usr/bin/nm -g --defined-only "$archive"
 run "$arch-provisioner-ELF" /usr/bin/readelf -h -A "${objects[@]}"
 run "$arch-cc" "$cc" "${common[@]}" "${flags[@]}" -MD \
  -MF "$out/objects/$arch-bootstrap.d" -c "$app/qc07f-bootstrap-candidate/bootstrap.c" \
  -o "$out/objects/$arch-bootstrap.o"
 run "$arch-REL" "$cc" "${flags[@]}" -nostdlib -r \
  "$out/objects/$arch-bootstrap.o" "${objects[@]}" \
  -Wl,--whole-archive "$archive" -Wl,--no-whole-archive \
  "-Wl,-Map=$out/$arch.map" -o "$out/bootstrap-$arch-NOT-EXECUTABLE.o"
 run "$arch-defined" /usr/bin/nm -g --defined-only "$out/bootstrap-$arch-NOT-EXECUTABLE.o"
 run "$arch-undefined" /usr/bin/nm -u "$out/bootstrap-$arch-NOT-EXECUTABLE.o"
 run "$arch-ELF" /usr/bin/readelf -h -A "$out/bootstrap-$arch-NOT-EXECUTABLE.o"
done
run final-input-pins /usr/bin/sha256sum --check "$doc/BUILD_INPUTS.sha256"
