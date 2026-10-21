# SPDX-License-Identifier: GPL-2.0-or-later
# SOURCE only in the reviewed persistent ash. Exactly one selected effect;
# other components are read-only. Responses/holders remain private RAM metadata.
qc_aim_hash() {
    local qa_sum
    qa_sum=$(/bin/busybox sha256sum "$1") || return 125
    test "${qa_sum%% *}" = "$2"
}
qc_aim_dispatch() {
    local qa_action="$1" qa_nonce="$2" qa_boot="$3" qa_optpin="$4" qa_init="$5" qa_shell="$6"
    local qa_uid=0 qa_key qa_real qa_eff qa_saved qa_fs qa_rc=125 qa_prefix qa_mask qa_c
    local qa_opt=status qa_white=status qa_ptz=status qa_step qa_line qa_pid
    test "$#" = 6 || return 125
    case "$qa_action" in
        status) ;;
        pulse-a|pulse-b|ir-on|ir-off) qa_opt="$qa_action" ;;
        white-on|white-off) qa_white="$qa_action" ;;
        motor0-minus|motor0-plus|motor1-minus|motor1-plus) qa_ptz="$qa_action" ;;
        *) return 125 ;;
    esac
    test "${#qa_nonce}" = 32 || return 125
    case "$qa_nonce" in *[!0123456789abcdef]*) return 125 ;; esac
    case "$qa_shell" in ''|*[!0123456789]*) return 125 ;; esac
    case "$qa_init" in init|sh) ;; *) return 125 ;; esac
    while read -r qa_key qa_real qa_eff qa_saved qa_fs; do
        if test "$qa_key" = Uid:; then
            if test "$qa_real" = 0 && test "$qa_eff" = 0 && test "$qa_saved" = 0 && test "$qa_fs" = 0; then qa_uid=1; fi
            break
        fi
    done </proc/self/status
    if test "$qa_shell" != "$$" || test "$qa_uid" != 1 ||
       test "$qa_optpin" != eaded5d3827ec9ee073cc09efb0904b06f6f7d073405bb7bfc3e757c5db4f128 ||
       test "$(/bin/busybox cat /proc/sys/kernel/osrelease)" != 6.18.55-qc07f-prototype-dirty ||
       test "$(/bin/busybox cat /proc/1/comm)" != "$qa_init" ||
       test "$(/bin/busybox cat /proc/sys/kernel/random/boot_id)" != "$qa_boot" ||
       ! qc_aim_hash /tmp/qc07f-optical-cli "$qa_optpin" ||
       ! qc_aim_hash /tmp/qc07f-optical-dispatch.sh cfc10dea3fff5a3fb434a98053d5113a3573b5a7bc9ff2b11778853eb8d63df6 ||
       ! qc_aim_hash /run/qc-stage-optical75.ko b839958867cfbf89877a17cfc72a448db12b3703f13bf3528faaa720a6a3a61e ||
       ! qc_aim_hash /tmp/qc07f-white-cli d08cb896298cd646c16ddcf9df86802b30d6f00dc5261a60750c94df8d694697 ||
       ! qc_aim_hash /run/qc-stage-white75.ko 195c05ad2421894d8abc796d471c88ede64082717658cc58603f120965d21110 ||
       ! qc_aim_hash /tmp/qc07f-ptz-cli 6ee85b38354f8c9a58e963857cb0f6d3a5cf60ea412587200be8e904e9e7fdd4; then
        printf 'AIM_BEGIN_%s\nAIM_DONE_%s:125\n' "$qa_nonce" "$qa_nonce"
        return 125
    fi
    qa_prefix="/run/qc-aim-receipts/$qa_nonce"
    if test ! -d /run/qc-aim-receipts || test -L /run/qc-aim-receipts; then return 125; fi
    for qa_step in opt white ptz werr wpid; do
        if test -e "$qa_prefix.$qa_step" || test -L "$qa_prefix.$qa_step"; then
            printf 'AIM_BEGIN_%s\nAIM_DONE_%s:125\n' "$qa_nonce" "$qa_nonce"
            return 125
        fi
    done
    qa_mask=$(umask)
    case "$-" in *C*) qa_c=1 ;; *) qa_c=0 ;; esac
    umask 077
    set -C
    # Effect goes FIRST; the other two components are sampled afterwards.
    # Source the old optical dispatcher in this same ash, never a new sh -c.
    case "$qa_action" in
        white-*)
            if test "$qa_white" = white-on; then
                /tmp/qc07f-white-cli --white-on-hold "$qa_nonce" </dev/null >"$qa_prefix.white" 2>"$qa_prefix.werr" &
                qa_pid=$!
                qa_rc=125
                if printf '%s\n' "$qa_pid" >"$qa_prefix.wpid"; then
                    qa_rc=124
                    for qa_step in 0 1 2; do
                        if test -f "$qa_prefix.white" && IFS= read -r qa_line <"$qa_prefix.white"; then
                            test -s "$qa_prefix.werr" || qa_rc=0
                            break
                        fi
                        test "$qa_step" = 2 || /bin/busybox sleep 1
                    done
                fi
            else
                /tmp/qc07f-white-cli white-off "$qa_nonce" >"$qa_prefix.white"
                qa_rc=$?
            fi
            if test "$qa_rc" = 0; then
                . /tmp/qc07f-optical-dispatch.sh status "$qa_nonce" "$qa_boot" "$qa_optpin" "$qa_init" "$qa_shell" >"$qa_prefix.opt"
                qa_rc=$?
            fi
            if test "$qa_rc" = 0; then
                /tmp/qc07f-ptz-cli status "$qa_nonce" >"$qa_prefix.ptz"
                qa_rc=$?
            fi ;;
        motor*)
            /tmp/qc07f-ptz-cli "$qa_ptz" "$qa_nonce" >"$qa_prefix.ptz"
            qa_rc=$?
            if test "$qa_rc" = 0; then
                . /tmp/qc07f-optical-dispatch.sh status "$qa_nonce" "$qa_boot" "$qa_optpin" "$qa_init" "$qa_shell" >"$qa_prefix.opt"
                qa_rc=$?
            fi
            if test "$qa_rc" = 0; then
                /tmp/qc07f-white-cli status "$qa_nonce" >"$qa_prefix.white"
                qa_rc=$?
            fi ;;
        *)
            . /tmp/qc07f-optical-dispatch.sh "$qa_opt" "$qa_nonce" "$qa_boot" "$qa_optpin" "$qa_init" "$qa_shell" >"$qa_prefix.opt"
            qa_rc=$?
            if test "$qa_rc" = 0; then
                /tmp/qc07f-white-cli status "$qa_nonce" >"$qa_prefix.white"
                qa_rc=$?
            fi
            if test "$qa_rc" = 0; then
                /tmp/qc07f-ptz-cli status "$qa_nonce" >"$qa_prefix.ptz"
                qa_rc=$?
            fi ;;
    esac
    test "$qa_c" = 1 || set +C
    umask "$qa_mask"
    if test "$(/bin/busybox cat /proc/sys/kernel/random/boot_id)" != "$qa_boot"; then qa_rc=126; fi
    # Job notices and all native execution precede the outer receipt frame.
    printf 'AIM_BEGIN_%s\n' "$qa_nonce"
    if test "$qa_rc" = 0; then
        /bin/busybox cat "$qa_prefix.opt"
        printf 'WHITE_BEGIN_%s\n' "$qa_nonce"
        /bin/busybox cat "$qa_prefix.white"
        printf 'WHITE_DONE_%s:0\nPTZ_BEGIN_%s\n' "$qa_nonce" "$qa_nonce"
        /bin/busybox cat "$qa_prefix.ptz"
        printf 'PTZ_DONE_%s:0\n' "$qa_nonce"
    fi
    printf 'AIM_DONE_%s:%s\n' "$qa_nonce" "$qa_rc"
    return "$qa_rc"
}
qc_aim_dispatch "$@"
return "$?"
