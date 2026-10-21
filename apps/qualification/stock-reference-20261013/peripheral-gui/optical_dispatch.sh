# SPDX-License-Identifier: GPL-2.0-or-later
# SOURCE in the already-admitted persistent ash; never execute in an exiting sh -c.
# Fixed RAM helper, fixed actions. No setup, boot, arbitrary path, rearm or retry.
qc_optical_dispatch() {
    local qo_action="$1" qo_nonce="$2" qo_boot="$3" qo_sha="$4"
    local qo_init="$5" qo_shell="$6" qo_rc=125 qo_line qo_step qo_pid
    local qo_prefix qo_mask qo_noclobber qo_uid=0 qo_key qo_real qo_eff qo_saved qo_fs
    local qo_clisum qo_modsum
    test "$#" = 6 || return 125
    case "$qo_action" in status|pulse-a|pulse-b|ir-on|ir-off) ;; *) return 125 ;; esac
    test "${#qo_nonce}" = 32 || return 125
    case "$qo_nonce" in *[!0123456789abcdef]*) return 125 ;; esac
    test "${#qo_sha}" = 64 || return 125
    case "$qo_sha" in *[!0123456789abcdef]*) return 125 ;; esac
    case "$qo_init" in init|sh) ;; *) return 125 ;; esac
    case "$qo_shell" in ''|*[!0123456789]*) return 125 ;; esac
    # The pinned minimal BusyBox has neither id nor cut. Read this shell's
    # procfs identity using builtins; parse fixed sha output with parameter expansion.
    while read -r qo_key qo_real qo_eff qo_saved qo_fs; do
        if test "$qo_key" = Uid:; then
            if test "$qo_real" = 0 && test "$qo_eff" = 0 &&
               test "$qo_saved" = 0 && test "$qo_fs" = 0; then qo_uid=1; fi
            break
        fi
    done </proc/self/status
    qo_clisum=$(/bin/busybox sha256sum /tmp/qc07f-optical-cli)
    qo_modsum=$(/bin/busybox sha256sum /run/qc-stage-optical75.ko)
    # Executing this file in a new shell fails before any helper action.
    if test "$qo_shell" != "$$" ||
       test "$qo_uid" != 1 ||
       test "$(/bin/busybox cat /proc/sys/kernel/osrelease)" != 6.18.55-qc07f-prototype-dirty ||
       test "$(/bin/busybox cat /proc/1/comm)" != "$qo_init" ||
       test "$(/bin/busybox cat /proc/sys/kernel/random/boot_id)" != "$qo_boot" ||
       test "${qo_clisum%% *}" != "$qo_sha" ||
       test "${qo_modsum%% *}" != b839958867cfbf89877a17cfc72a448db12b3703f13bf3528faaa720a6a3a61e; then
        printf 'OB_BEGIN_%s\nOB_DONE_%s:125\n' "$qo_nonce" "$qo_nonce"
        return 125
    fi
    if test "$qo_action" = ir-on; then
        qo_prefix="/run/qc-optical-receipts/$qo_nonce"
        # Parent must separately create/admit this private RAM directory.
        if test ! -d /run/qc-optical-receipts || test -L /run/qc-optical-receipts ||
           test -e "$qo_prefix.json" || test -L "$qo_prefix.json" ||
           test -e "$qo_prefix.err" || test -L "$qo_prefix.err" ||
           test -e "$qo_prefix.pid" || test -L "$qo_prefix.pid"; then
            printf 'OB_BEGIN_%s\nOB_DONE_%s:125\n' "$qo_nonce" "$qo_nonce"
            return 125
        fi
        qo_mask=$(umask)
        case "$-" in *C*) qo_noclobber=1 ;; *) qo_noclobber=0 ;; esac
        umask 077
        set -C
        # Simple direct background command: persistent ash is its parent/reaper.
        # No pipeline, command substitution, intermediate shell or helper fork.
        /tmp/qc07f-optical-cli --ir-on-hold "$qo_nonce" </dev/null >"$qo_prefix.json" 2>"$qo_prefix.err" &
        qo_pid=$!
        if printf '%s\n' "$qo_pid" >"$qo_prefix.pid"; then qo_rc=0; fi
        test "$qo_noclobber" = 1 || set +C
        umask "$qo_mask"
        # Launch/job notices precede BEGIN, not the one-line receipt body.
        printf 'OB_BEGIN_%s\n' "$qo_nonce"
        if test "$qo_rc" = 0; then
            qo_rc=124
            # Pinned BusyBox has integer-only sleep. At most two seconds;
            # existing/partial files, job PID and a prompt are not readiness.
            for qo_step in 0 1 2; do
                if test -f "$qo_prefix.json" &&
                   IFS= read -r qo_line <"$qo_prefix.json"; then
                    if test ! -s "$qo_prefix.err"; then
                        printf '%s\n' "$qo_line"
                        qo_rc=0
                    fi
                    break
                fi
                test "$qo_step" = 2 || /bin/busybox sleep 1
            done
        fi
    else
        printf 'OB_BEGIN_%s\n' "$qo_nonce"
        /tmp/qc07f-optical-cli "$qo_action" "$qo_nonce"
        qo_rc=$?
    fi
    if test "$(/bin/busybox cat /proc/sys/kernel/random/boot_id)" != "$qo_boot"; then qo_rc=126; fi
    printf 'OB_DONE_%s:%s\n' "$qo_nonce" "$qo_rc"
    # An ON receipt proves command/snapshot, NOT holder exit. Parent accounts
    # for this shell's finite job after its deadline; no automatic kill/retry.
    return "$qo_rc"
}
qc_optical_dispatch "$@"
return "$?"
