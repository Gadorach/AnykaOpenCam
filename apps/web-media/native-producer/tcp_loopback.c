/* SPDX-License-Identifier: GPL-2.0-only; PUBLICDUMMY synthetic localhost only. */
#define _POSIX_C_SOURCE 200809L
#include "tcp_sink.h"
#include "h264-baseline.h"
#include "crc-bzip2.h"
#include <camera/rtsp_clock.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct camera_rtsp_clock clock_state;
static uint64_t now(void *c){(void)c;return camera_rtsp_clock_ns(&clock_state);}
static int cancelled(void *c){(void)c;return 0;}
static uint8_t producer[64],hub[1048576];static unsigned produced,held;
static int next(void *c,struct camera_packet *p,struct nh_key *k)
{
 (void)c;assert(!held);held=1;produced++;memset(producer,0,sizeof(producer));
 producer[0]=produced==1?0x65:0x41;memcpy(producer+1,"PUBLICDUMMY-NOT-FOOTAGE",22);
 *p=(struct camera_packet){.codec=CAMERA_H264,.keyframe=produced==1,.data=producer,.bytes=32,.monotonic_ns=1000000000ULL+(produced-1)*450000000ULL};
 *k=(struct nh_key){1,2,3,produced,qc_crc_bzip2(producer,32)};return 0;
}
static int release(void *c,const struct camera_packet *p){(void)c;(void)p;assert(held);held=0;return 0;}
static int retire(void *c){(void)c;assert(!held);return 0;}
static void fence(void *c){(void)c;assert(0);}
int main(int argc,char **argv)
{
 if(argc!=2)return 64;struct sockaddr_in address;assert(!nh_tcp_endpoint("127.0.0.1",argv[1],&address));
 struct nh_tcp_sink s;uint64_t start=now(NULL);assert(!nh_tcp_connect(&s,&address,start,now,cancelled,NULL));
 uint8_t params[256];size_t n=qc_h264_parameter_sets(params),split=4;
 while(split+4<n&&memcmp(params+split,"\0\0\0\1",4))split++;
 struct camera_fmp4_config config={.sps=params+4,.sps_bytes=split-4,.pps=params+split+4,.pps_bytes=n-split-4};
 struct nh_backend b={.context=&s,.next=next,.release=release,.retire=retire,.now=now,.cancelled=cancelled,
 .write=nh_tcp_write,.discard_output=nh_tcp_discard,.quarantine=fence,.session_start_ns=start,.output_begin=nh_tcp_begin};
 struct nh_report r;assert(!nh_run(&b,&config,hub,sizeof(hub),&r));assert(!nh_tcp_close(&s));
 assert(produced==13&&r.output==12&&r.committed==r.accepted&&r.retired);
 printf("PUBLICDUMMY localhost13/12 accepted=%zu complete=%zu remote_ACK=0 source_is_MOCK=1\n",r.accepted,r.committed);return 0;
}
