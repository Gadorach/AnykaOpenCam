# Invoked only by an explicitly reviewed PID1 exec after pivot-stage1.
[ "$$" = 1 ] || exit 1
cd / || exit 1
umask 077
printf '%s\n' 'RAM_ROOT_PIVOT01_STATIC_PID1_READY_OLDROOT_STILL_MOUNTED'
# No profiles, daemon, networking, mdev scan, module loads, unmount or writes.
exec /bin/busybox env -i HOME=/root TERM=vt100 \
    PATH=/bin:/sbin:/usr/bin:/usr/sbin ENV=/dev/null \
    PS1='ram-pivot01# ' /bin/busybox sh -i < /dev/console > /dev/console 2>&1
