#!/bin/sh
# Controlled bench station only: no default route, DNS or persistent writes.
test "$interface" = wlan0 || exit 1
case "$1" in
  deconfig) exit 0 ;;
  bound|renew)
    test "$subnet" = 255.255.255.0 || exit 1
    case "$ip" in 192.168.1.*) ;; *) exit 1 ;; esac
    tail=${ip#192.168.1.}
    case "$tail" in ''|*[!0-9]*) exit 1 ;; esac
    test "$tail" -gt 1 && test "$tail" -lt 255 || exit 1
    /bin/busybox ifconfig wlan0 "$ip" netmask "$subnet" || exit 1
    echo SSH_BENCH_DHCP_LOCAL_BOUND
    ;;
  *) exit 1 ;;
esac
