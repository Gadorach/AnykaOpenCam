/* SPDX-License-Identifier: GPL-2.0-only */
#include "audio_packet.h"
int main(){am_format format{};return am_format_select(G711_MULAW,&format)==AM_OK&&format.payload_type==0&&format.clock_hz==8000?0:1;}
