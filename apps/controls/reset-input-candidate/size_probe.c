/* SPDX-License-Identifier: GPL-2.0-only */
#include "reset_input.h"
/* Unlinked ABI-size probe, not product allocations. */
char reset_input_storage_bytes[sizeof(struct reset_input)];
char reset_input_event_bytes[sizeof(struct ri_event)];
char reset_input_observation_bytes[sizeof(struct ri_observation)];
char reset_input_view_bytes[sizeof(struct ri_view)];
