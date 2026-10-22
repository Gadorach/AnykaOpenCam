/* SPDX-License-Identifier: GPL-2.0-only
 *Compile-time C shape measurement, no process/proc/RSS/target/material reads.*/
#include "rtsp_server_candidate.c"
#include "adapter_state.h"
/*Kept public scalar section supports ARM size extraction WITHOUT execution.*/
const uint32_t is_native_shape_values[] __attribute__((section(".is_shape"),used))={
 sizeof(struct candidate_backend),sizeof(struct candidate_peer),4*sizeof(struct rtsp_client),4*sizeof(struct au_cursor),sizeof(struct rv_message),sizeof(struct candidate_config),sizeof(struct camera_packet_hub)};
int main(void)
{
 printf("HOST_ONLY SHAPE backend=%zu peer=%zu clients4=%zu RTP_cursors4=%zu owned_adapter=%zu IPC_message=%zu public_config=%zu hub_metadata=%zu fixture_NAL_backing=%u fixture_hub_backing=%u native_child_limit=%u HOST_child_limit=%u native_fit_NOTproved=1\n",
 sizeof(struct candidate_backend),sizeof(struct candidate_peer),4*sizeof(struct rtsp_client),4*sizeof(struct au_cursor),is_rv_context_bytes(),sizeof(struct rv_message),sizeof(struct candidate_config),sizeof(struct camera_packet_hub),CANDIDATE_HUB_BYTES+32,CANDIDATE_HUB_BYTES+32,2097152U,4194304U);
 return 0;
}
