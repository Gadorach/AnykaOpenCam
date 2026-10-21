/* Compile only; exact public stock ION header, no target execution. */
#include <stddef.h>
#include <linux/ioctl.h>
#include "wire.h"
#include ORIGINAL_ION_HEADER
_Static_assert(sizeof(void *) == 4 && sizeof(size_t) == 4, "ARM32");
_Static_assert(sizeof(struct ion_allocation_data) == 20, "stock alloc20");
_Static_assert(offsetof(struct ion_allocation_data, handle) == 16, "stock handle16");
_Static_assert(sizeof(struct ion_fd_data) == 8, "stock map8");
_Static_assert(sizeof(struct ion_handle_data) == 4, "stock free4");
_Static_assert(ION_IOC_ALLOC == I_ALLOC && ION_IOC_MAP == I_MAP && ION_IOC_FREE == I_FREE, "stock requests");
_Static_assert((ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC) == ALLOC_FLAGS, "retail flags3");
const uint32_t abi_values[] = {20,8,4,ION_IOC_ALLOC,ION_IOC_MAP,ION_IOC_FREE,ALLOC_FLAGS,HEAP_MASK};
