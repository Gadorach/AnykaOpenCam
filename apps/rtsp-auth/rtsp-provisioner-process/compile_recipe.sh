#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Fixed compile recipe ONLY; does NOT run a test/app/native hardening/old builder.
set -euo pipefail
root=/home/gadorach/Desktop/AnykaOpenCam
out="$root/docs/qualification-plans/rtsp-provisioner-process-20261023/runs/build01"
app="$root/apps/rtsp-auth"
mbed="$root/apps/auth-web/service/vendor/mbedtls"
arm="$root/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/bin/arm-buildroot-linux-musleabi-gcc"
[[ ! -e "$out" ]] || exit 10
mkdir -p "$out/objects" "$out/logs"
run() {
 local log="$1"; shift
 printf '%q ' "$@" >> "$out/COMMANDS.txt"
 printf '\n' >> "$out/COMMANDS.txt"
 "$@" > "$out/logs/$log.txt" 2>&1
}
common=(-std=gnu11 -Wall -Wextra -Werror -fno-common -fno-strict-aliasing
 -ffunction-sections -fdata-sections -fstack-protector-strong -D_FORTIFY_SOURCE=2
 -fstack-usage '-DMBEDTLS_CONFIG_FILE="rtsp_crypto_config.h"'
 "-I$root/framework/include" "-I$app" "-I$app/isolated-verifier"
 "-I$mbed/include" "-I$mbed/library")
sources=("$app/rtsp-provisioner-process/provisioner_process.c"
 "$app/rtsp-private-handoff/private_handoff.c"
 "$app/rtsp-startup-material/startup_material.c"
 "$root/framework/src/rtsp_digest.c" "$root/framework/src/rtsp_digest_wire.c"
 "$root/framework/src/backend.c" "$root/framework/src/rtsp_input.c"
 "$root/framework/src/rtsp_request.c" "$mbed/library/sha256.c"
 "$mbed/library/md5.c" "$mbed/library/constant_time.c" "$mbed/library/platform_util.c")
for arch in host arm; do
 if [[ "$arch" == host ]]; then cc=/usr/bin/cc; target=(-O2); else
  cc="$arm"; target=(-Os -march=armv5te -marm -mfloat-abi=soft)
 fi
 run "$arch-version" "$cc" --version
 run "$arch-machine" "$cc" -dumpmachine
 objects=()
 for i in "${!sources[@]}"; do
  source="${sources[$i]}"; object="$out/objects/$arch-$i.o"; exceptions=()
  # Existing frozen parser alone has its original isolated warning exceptions.
  if [[ "$source" == */rtsp_input.c ]]; then
   exceptions=(-Wno-misleading-indentation -Wno-error=nonnull)
  fi
  run "$arch-cc-$i" "$cc" "${common[@]}" "${target[@]}" "${exceptions[@]}" \
   -MD -MF "$out/objects/$arch-$i.d" -c "$source" -o "$object"
  objects+=("$object")
 done
 run "$arch-REL" "$cc" "${target[@]}" -nostdlib -r "${objects[@]}" \
  -o "$out/provisioner-process-$arch-NOT-EXECUTABLE.o"
 run "$arch-ELF" /usr/bin/readelf -h -A "$out/provisioner-process-$arch-NOT-EXECUTABLE.o"
 run "$arch-undefined" /usr/bin/nm -u "$out/provisioner-process-$arch-NOT-EXECUTABLE.o"
done
for unit in provisioner_process positive_publicdummy; do
 run "publicdummy-cc-$unit" /usr/bin/cc "${common[@]}" -O2 \
  -DRTSP_HANDOFF_HOST_PUBLICDUMMY -MD -MF "$out/objects/publicdummy-$unit.d" \
  -c "$app/rtsp-provisioner-process/$unit.c" -o "$out/objects/publicdummy-$unit.o"
done
run publicdummy-cc-handoff /usr/bin/cc "${common[@]}" -O2 \
 -DRTSP_HANDOFF_HOST_PUBLICDUMMY -MD -MF "$out/objects/publicdummy-handoff.d" \
 -c "$app/rtsp-private-handoff/private_handoff.c" -o "$out/objects/publicdummy-handoff.o"
# Original constructor/Digest/graph/parser/crypto objects from THIS recipe only;
# native process/handoff objects NEVER enter the HOST fixture executable.
objects=("$out/objects/publicdummy-provisioner_process.o"
 "$out/objects/publicdummy-positive_publicdummy.o" "$out/objects/publicdummy-handoff.o")
for i in {2..11}; do objects+=("$out/objects/host-$i.o"); done
run publicdummy-link /usr/bin/cc -Wl,--gc-sections -Wl,-z,relro,-z,now \
 "${objects[@]}" -o "$out/PUBLICDUMMY-NOT-NATIVE"
run publicdummy-symbols /usr/bin/nm -g "$out/PUBLICDUMMY-NOT-NATIVE"
run publicdummy-ELF /usr/bin/readelf -h -A "$out/PUBLICDUMMY-NOT-NATIVE"
