/* Intentionally non-launchable ARM artifact: real adapter linked for inspection.
 * Admission gaps cannot be bypassed by argv/env/build-time enable flags. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "capture.h"
#if !defined(__arm__) || __ARM_ARCH != 5 || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Native is ARMv5 little endian only; execute host_test, never this binary on host.
#endif
_Static_assert(sizeof(void*)==4 && sizeof(long)==4, "ARM32 native");
int main(int argc,char **argv)
{
    int rc=fcntl(3,F_GETFD),closed=rc==-1 && errno==EBADF;
    static const char hold[]="{\"record\":\"stock-capture-admission-v1\",\"state\":\"NO_NATIVE_LAUNCH\",\"class\":\"CANDIDATE_HOLD\",\"reason\":\"PARENT_NATIVE_INSTANCE_CLOCK_RESET_SEAL_REQUIRED\",\"adapter\":\"LINKED_NOT_ADMITTED\",\"device_operations\":false,\"allocation\":false,\"sensor_stream\":false}\n";
    static const char bad[]="{\"state\":\"HOLD\",\"reason\":\"EXACT_SELECTOR_AND_CLOSED_INHERITED_FD3_REQUIRED\",\"device_operations\":false}\n";
    if(!cr_entry_allowed(argc,argv,closed)){(void)write(1,bad,sizeof(bad)-1);return 2;}
    (void)write(1,hold,sizeof(hold)-1);return 2;
}
