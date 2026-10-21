# INERT DEFINITIONS ONLY. Source from the UART PID1 shell, then explicitly call
# pivot01 I_REVIEWED_LIVE_STATE. Never execute as an SSH/background/child script.
# Parent must first finish the process/FD/MM/mount/backup/recovery checks in PIVOT.md.
pivot01()
{
    [ "${1-}" = I_REVIEWED_LIVE_STATE ] && [ "$$" = 1 ] || {
        printf '%s\n' 'PIVOT01_REFUSE: explicit parent review and UART PID1 required'
        return 1
    }
    pivot01_new=/tmp/newroot
    pivot01_bb=$pivot01_new/bin/busybox
    pivot01_oldbb=$pivot01_new/rescue-tools/busybox
    [ -x "$pivot01_bb" ] && [ -x "$pivot01_oldbb" ] || return 1
    [ -d "$pivot01_new/oldroot" ] && [ ! -L "$pivot01_new/oldroot" ] || return 1
    [ -d "$pivot01_new/state" ] && [ ! -L "$pivot01_new/state" ] || return 1
    pivot01_root=0 pivot01_tmp=0 pivot01_newmnt=0
    pivot01_dev=0 pivot01_proc=0 pivot01_sys=0 pivot01_usr=0
    while read -r pivot01_source pivot01_where pivot01_type pivot01_opts pivot01_rest
    do
        case "$pivot01_where:$pivot01_type" in
            /:rootfs) ;;
            /:squashfs)
                case ",$pivot01_opts," in *,ro,*) pivot01_root=1;; *) return 1;; esac ;;
            /tmp:tmpfs) pivot01_tmp=1 ;;
            /tmp/newroot:tmpfs) pivot01_newmnt=1 ;;
            /dev:devtmpfs|/dev:tmpfs) pivot01_dev=1 ;;
            /proc:proc) pivot01_proc=1 ;;
            /sys:sysfs) pivot01_sys=1 ;;
            /dev/pts:devpts) ;;
            /usr:squashfs)
                case ",$pivot01_opts," in *,ro,*) pivot01_usr=1;; *) return 1;; esac ;;
            *) printf '%s\n' 'PIVOT01_REFUSE: unexpected mount topology'; return 1 ;;
        esac
    done < /proc/mounts
    [ "$pivot01_root$pivot01_tmp$pivot01_newmnt$pivot01_dev$pivot01_proc$pivot01_sys" = 111111 ] || {
        printf '%s\n' 'PIVOT01_REFUSE: required separate mounts absent'
        return 1
    }
    while read -r pivot01_line
    do
        case "$pivot01_line" in *' shared:'*)
            printf '%s\n' 'PIVOT01_REFUSE: shared mount propagation'
            return 1 ;;
        esac
    done < /proc/1/mountinfo
    # This static BusyBox has SHA256SUM but NOT checksum-file (-c) support.
    [ -s "$pivot01_new/rescue/SHA256SUMS" ] || return 1
    while read -r pivot01_expected pivot01_file
    do
        case "$pivot01_file" in ''|/*|*..*) return 1;; esac
        pivot01_actual=$("$pivot01_bb" sha256sum "$pivot01_new/$pivot01_file") || return 1
        [ "${pivot01_actual%% *}" = "$pivot01_expected" ] || {
            printf '%s\n' 'PIVOT01_REFUSE: public bundle hash mismatch'
            return 1
        }
    done < "$pivot01_new/rescue/SHA256SUMS"
    # These redirections permanently rebind PID1's 0/1/2 to the separately
    # mounted console before moving /dev. No tty settings or modem lines change.
    exec < /dev/console > /dev/console 2>&1
    cd /tmp/newroot || return 1
    "$pivot01_oldbb" pivot_root . oldroot || return 1
    # After successful pivot ALL commands below resolve inside the new root.
    /rescue-tools/busybox mount --move /oldroot/dev /dev || return 1
    /rescue-tools/busybox mount --move /oldroot/proc /proc || return 1
    /rescue-tools/busybox mount --move /oldroot/sys /sys || return 1
    if [ "$pivot01_usr" = 1 ]; then
        /rescue-tools/busybox mount --move /oldroot/usr /usr || return 1
    fi
    /rescue-tools/busybox mount --move /oldroot/tmp /state || return 1
    cd / || return 1
    # The exec, not pivot_root alone, releases old PID1 BusyBox/loader mappings.
    exec /bin/busybox sh /rescue/pivot-stage2.sh
}
