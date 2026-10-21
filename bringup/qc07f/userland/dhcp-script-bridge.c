/* SPDX-License-Identifier: GPL-2.0-only
 * Current RAM kernel lacks BINFMT_SCRIPT. Preserve exact DHCP environment
 * and invoke our own RAM script through the freshly built ELF shell.
 */
#include <unistd.h>
int main(int argc, char **argv)
{
    char *args[] = { "/bin/busybox", "ash", "/bin/udhcpc.script", argc > 1 ? argv[1] : "", 0 };
    execv(args[0], args);
    return 127;
}
