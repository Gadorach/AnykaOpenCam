/* SPDX-License-Identifier: GPL-2.0-only
 * Only synthetic memory-backed NAL bytes and owned loopback sockets. No device,
 * fixture file or physical camera access. Not a decodable-video qualification. */
#define _POSIX_C_SOURCE 200809L
#include <camera/rtsp_server.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static volatile sig_atomic_t stopped;
static void stop_fixture(int sig){(void)sig;stopped=1;}
struct mock {
 struct camera_producer producer;struct camera_rtsp_common_producer bridge;
 unsigned starts,idles,produced,released,owned;unsigned char nal[6000];
};
static uint64_t clock_ns(void *context)
{struct timespec t;(void)context;if(clock_gettime(CLOCK_MONOTONIC,&t))return UINT64_MAX;return (uint64_t)t.tv_sec*1000000000ULL+(uint64_t)t.tv_nsec;}
static int start_source(void *context)
{struct mock *m=context;if(m->owned)return CAMERA_BUSY;m->starts++;return CAMERA_OK;}
static int idle_source(void *context)
{struct mock *m=context;if(m->owned)return CAMERA_BUSY;m->idles++;return CAMERA_OK;}
static int next_source(void *context,uint32_t force,struct camera_packet *p)
{
 struct mock *m=context;if(m->owned)return CAMERA_BUSY;m->nal[0]=force?0x65:0x41;
 *p=(struct camera_packet){.codec=CAMERA_H264,.keyframe=!!force,.data=m->nal,.bytes=sizeof(m->nal),.monotonic_ns=clock_ns(m)};
 m->owned=1;m->produced++;return CAMERA_OK;
}
static void release_source(void *context,const struct camera_packet *p)
{struct mock *m=context;(void)p;m->owned=0;m->released++;}
int main(int argc,char **argv)
{
 if(argc!=2)return 2;char *end;unsigned long port=strtoul(argv[1],&end,10);
 if(!argv[1][0]||*end||port<1024||port>65535)return 2;
 char live[96],track[112];snprintf(live,sizeof(live),"rtsp://127.0.0.1:%lu/live",port);snprintf(track,sizeof(track),"%s/track1",live);
 struct camera_rtsp_registry registry;if(camera_rtsp_registry_init(&registry,live,track))return 2;
 struct mock m={0};for(size_t i=0;i<sizeof(m.nal);i++)m.nal[i]=(unsigned char)(i*31U);
 struct camera_producer_config config={.operation_timeout_ns=10000000000ULL,.keyframe_supported=1};
 if(camera_producer_init(&m.producer,&config))return 2;
 m.bridge=(struct camera_rtsp_common_producer){.producer=&m.producer,.context=&m,.start=start_source,.idle=idle_source,.clock_ns=clock_ns};
 struct camera_rtsp_lifecycle hooks=camera_rtsp_common_hooks(&m.bridge);
 const unsigned char params[]={0,0,0,1,0x67,1,2,3,0,0,0,1,0x68,4,5};unsigned char storage[8192];
 struct camera_rtsp_producer p={.context=&m,.next=next_source,.release=release_source,.idle_checked=idle_source,
  .parameter_sets=params,.parameter_set_bytes=sizeof(params),.lifecycle=&hooks};
 struct camera_rtsp_options o={.bind_address="127.0.0.1",.sdp="v=0\r\n",.port=(uint16_t)port,.interval_ms=25,
  .send_deadline_ms=500,.hub_storage=storage,.packet_capacity=sizeof(storage),.hub_slots=1,.trace_every=1000,
  .stop=&stopped,.resources=&registry,.request_deadline_ms=2000};
 signal(SIGTERM,stop_fixture);signal(SIGINT,stop_fixture);signal(SIGPIPE,SIG_IGN);
 int r=camera_rtsp_serve(&o,&p);
 fprintf(stderr,"MOCK result=%d starts=%u idles=%u produced=%u released=%u owned=%u\n",r,m.starts,m.idles,m.produced,m.released,m.owned);
 return r==CAMERA_OK&&m.starts==m.idles&&m.produced==m.released&&!m.owned?0:1;
}
