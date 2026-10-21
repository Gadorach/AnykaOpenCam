/* SPDX-License-Identifier: GPL-2.0-only */
#include "ptz_owner.h"
/* Unlinked compiler-size probe, not memory allocations in the product. */
char ptz_owner_bytes[sizeof(struct ptz_owner)];
char ptz_registry_bytes[sizeof(struct ptz_registry)];
char ptz_auth_bytes[sizeof(struct ptz_auth)];
char ptz_request_bytes[sizeof(struct ptz_request)];
char ptz_completion_bytes[sizeof(struct ptz_completion)];
char ptz_policy_bytes[sizeof(struct camera_access_policy)];
