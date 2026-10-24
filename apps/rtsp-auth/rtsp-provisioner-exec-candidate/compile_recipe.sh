#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Fixed offline source compile/link/metadata ONLY. Never runs any built object.
set -Eeuo pipefail
root=/home/gadorach/Desktop/AnykaOpenCam
app="$root/apps/rtsp-auth"
src="$app/rtsp-provisioner-exec-candidate"
docs="$root/docs/qualification-plans/rtsp-provisioner-exec-20261024"
out="$docs/build01"
arm="$root/work/lightbulb-camera/implementation-20261008T205404Z/sources/toolchain-armv5-musl-202508/armv5-eabi--musl--stable-2025.08-1/bin/arm-buildroot-linux-musleabi-gcc"
mbed="$root/apps/auth-web/service/vendor/mbedtls"
[[ ! -e "$out" ]] || exit 10
mkdir "$out"
mkdir "$out/objects" "$out/logs" "$out/consumed"
current=preflight
trap 'rc=$?; printf "FAILED_PRESERVED status=%s command=%s; STOP; no retry\n" "$rc" "$current" > "$out/FAILED.txt"; exit "$rc"' ERR
run() {
 current="$1";shift
 printf '%q ' "$@" >> "$out/COMMANDS.txt"
 printf '\n' >> "$out/COMMANDS.txt"
 "$@" > "$out/logs/$current.txt" 2>&1
 printf '%s\n' "$current" >> "$out/SUCCESS.txt"
}
common=(-std=gnu11 -Wall -Wextra -Werror -fno-common -fno-strict-aliasing
 -ffunction-sections -fdata-sections -fstack-protector-strong -D_FORTIFY_SOURCE=2
 -fstack-usage '-DMBEDTLS_CONFIG_FILE="rtsp_crypto_config.h"'
 "-I$root/framework/include" "-I$app" "-I$app/isolated-verifier"
 "-I$mbed/include" "-I$mbed/library")
# Parent has no constructor/handoff/old provisioner object or worker main.
# Worker excludes exec_owner. Same production units/defines on HOST/ARM; no
# PUBLICDUMMY, fixtures, test macros, native fallback, old builder or engine.
sources=("$src/public_codec.c" "$src/launch_security.c" "$src/exec_owner.c"
 "$src/worker_entry.c" "$app/rtsp-private-handoff/private_handoff.c"
 "$app/rtsp-startup-material/startup_material.c" "$root/framework/src/backend.c"
 "$root/framework/src/rtsp_digest_wire.c" "$root/framework/src/rtsp_input.c"
 "$root/framework/src/rtsp_request.c" "$root/framework/src/rtsp_digest.c"
 "$mbed/library/sha256.c" "$mbed/library/md5.c" "$mbed/library/constant_time.c"
 "$mbed/library/platform_util.c" "$src/abi_metadata.c")
frozen=("$app/rtsp-private-handoff/private_handoff.c" "$app/rtsp-private-handoff/private_handoff.h"
 "$app/rtsp-startup-material/startup_material.c" "$app/rtsp-startup-material/startup_material.h"
 "$app/rtsp-provisioner-process/provisioner_process.c" "$app/rtsp-provisioner-process/provisioner_process.h"
 "$app/qc07f-owned-runtime/server/candidate_auth.h" "$app/qc07f-owned-runtime/server/verifier_adapter_overlay.c"
 "$app/isolated-verifier/verifier.h" "$app/isolated-verifier/isolation.c"
 "$app/qc07f-writer-bootstrap-candidate/bootstrap.c"
 "$docs/../rtsp-provisioner-exec-design-20261024/FEASIBILITY.md"
 "$docs/../rtsp-provisioner-exec-design-20261024/CONTRACT_DRAFT.md"
 "$docs/../rtsp-provisioner-process-20261023/PARENT_DISPOSITION.md"
 "$docs/../rtsp-private-handoff-20261023/PARENT_DISPOSITION.md"
 "$docs/../rtsp-startup-material-20261023/PARENT_DISPOSITION.md"
 "$docs/../qc07f-writer-bootstrap-20261024/PARENT_DISPOSITION.md")
run selected-source-pins /usr/bin/sha256sum "${sources[@]}" "$src/"*.h "$src/compile_recipe.sh" "$src/NOTICE.md" "$docs/CONTRACT.md"
run frozen-before /usr/bin/sha256sum "${frozen[@]}"
for i in "${!sources[@]}"; do
 # Retain exact consumed PUBLIC source text, including original dependencies.
 run "source-copy-$i" /usr/bin/cp -- "${sources[$i]}" "$out/consumed/$i.c"
 run "source-diff-$i" /usr/bin/diff -u "${sources[$i]}" "$out/consumed/$i.c"
done
for arch in host arm; do
 if [[ "$arch" == host ]]; then cc=/usr/bin/cc; target=(-O2); else
  cc="$arm";target=(-Os -march=armv5te -marm -mfloat-abi=soft)
 fi
 run "$arch-version" "$cc" --version
 run "$arch-machine" "$cc" -dumpmachine
 run "$arch-tool-pin" /usr/bin/sha256sum "$cc"
 for tool in cc1 collect2 as ld; do
  run "$arch-location-$tool" "$cc" "-print-prog-name=$tool"
  IFS= read -r toolpath < "$out/logs/$arch-location-$tool.txt"
  if [[ "$toolpath" != */* ]]; then toolpath=$(command -v "$toolpath");fi
  run "$arch-pin-$tool" /usr/bin/sha256sum "$toolpath"
 done
 for i in "${!sources[@]}"; do
  exceptions=()
  # Frozen legacy parser's existing isolated exceptions only, not new units.
  if [[ "${sources[$i]}" == */rtsp_input.c ]]; then exceptions=(-Wno-misleading-indentation -Wno-error=nonnull);fi
  run "$arch-cc-$i" "$cc" "${common[@]}" "${target[@]}" "${exceptions[@]}" \
   -MD -MF "$out/objects/$arch-$i.d" -c "${sources[$i]}" -o "$out/objects/$arch-$i.o"
 done
 parent=();worker=()
 for i in 0 1 2 6 7 8 9 11 13 14; do parent+=("$out/objects/$arch-$i.o");done
 for i in 0 1 3 4 5 6 7 8 9 10 11 12 13 14; do worker+=("$out/objects/$arch-$i.o");done
 run "$arch-parent-REL" "$cc" "${target[@]}" -nostdlib -r -Wl,--gc-sections \
  -Wl,-u,qc07f_rtsp_exec_begin_v1,-u,qc07f_rtsp_exec_observe_v1,-u,qc07f_rtsp_exec_bind_v1 \
  "${parent[@]}" "-Wl,-Map,$out/parent-$arch.map" -o "$out/parent-$arch.o"
 run "$arch-worker-REL" "$cc" "${target[@]}" -nostdlib -r -Wl,--gc-sections -Wl,-u,main \
  "${worker[@]}" "-Wl,-Map,$out/worker-$arch.map" -o "$out/worker-$arch.o"
 for role in parent worker; do
  run "$arch-$role-symbols" /usr/bin/readelf -Ws "$out/$role-$arch.o"
  run "$arch-$role-relocations" /usr/bin/readelf -Wr "$out/$role-$arch.o"
  run "$arch-$role-ELF" /usr/bin/readelf -h -A "$out/$role-$arch.o"
  run "$arch-$role-nm" /usr/bin/nm -a "$out/$role-$arch.o"
  run "$arch-$role-undefined" /usr/bin/nm -u "$out/$role-$arch.o"
 done
 run "$arch-public-ABI" /usr/bin/readelf -x .rodata.pe_public_abi_metadata "$out/objects/$arch-15.o"
 if [[ "$arch" == arm ]]; then
  # Real musl static companion, not executable admission/provenance. No chmod,
  # chown/install/stage or invocation. Existing native2MiB ceiling is unchanged.
  run arm-worker-static "$cc" "${target[@]}" -static -no-pie -Wl,--gc-sections \
   -Wl,-z,noexecstack,-z,relro,-z,now "${worker[@]}" \
   "-Wl,-Map,$out/worker-arm-static.map" -o "$out/worker-arm-static-NOT-ADMITTED.elf"
  run arm-static-ELF /usr/bin/readelf -h -l -A -d "$out/worker-arm-static-NOT-ADMITTED.elf"
  run arm-static-symbols /usr/bin/readelf -Ws "$out/worker-arm-static-NOT-ADMITTED.elf"
  run arm-static-nm /usr/bin/nm -a "$out/worker-arm-static-NOT-ADMITTED.elf"
  run arm-static-undefined /usr/bin/nm -u "$out/worker-arm-static-NOT-ADMITTED.elf"
 fi
done
# Actual compiler dependencies, not guessed closure. Paths here are public source
# and tool headers with no whitespace (required for this fixed recipe).
dependencies=()
for dep in "$out/objects/"*.d; do
 while IFS= read -r line; do
  line=${line#*:};line=${line//\\/}
  read -r -a names <<< "$line"
  for name in "${names[@]}"; do dependencies+=("$name");done
 done < "$dep"
done
run dependency-pins /usr/bin/sha256sum "${dependencies[@]}"
run dependency-pin-copy /usr/bin/cp "$out/logs/dependency-pins.txt" "$out/DEPENDENCIES.sha256"
run frozen-after /usr/bin/sha256sum "${frozen[@]}"
run frozen-diff /usr/bin/diff -u "$out/logs/frozen-before.txt" "$out/logs/frozen-after.txt"
run selected-source-after /usr/bin/sha256sum "${sources[@]}" "$src/"*.h "$src/compile_recipe.sh" "$src/NOTICE.md" "$docs/CONTRACT.md"
run selected-source-diff /usr/bin/diff -u "$out/logs/selected-source-pins.txt" "$out/logs/selected-source-after.txt"
run artifact-pins /usr/bin/sha256sum "$out/objects/"* "$out/"*.o "$out/"*.map "$out/"*.elf
run metadata-tool-pins /usr/bin/sha256sum /usr/bin/bash /usr/bin/nm /usr/bin/readelf /usr/bin/sha256sum /usr/bin/diff /usr/bin/cp
printf '%s\n' 'COMPILE_LINK_METADATA_COMPLETE; NOT RUN; role/relocation/fit review still required' > "$out/RESULT.txt"
