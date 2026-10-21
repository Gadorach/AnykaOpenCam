/* SPDX-License-Identifier: GPL-2.0-only
 * Loopback-only repeated recorded IDR media fixture. NOT a live camera,
 * software encoder fallback, protected product server, or hardware proof.
 */
#define _POSIX_C_SOURCE 200809L
#include <camera/rtsp_server.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static volatile sig_atomic_t stopped;
static void stop(int sig){(void)sig;stopped=1;}
struct fixture {const uint8_t *nal;size_t bytes;unsigned frames;};
static int next(void *context,uint32_t force,struct camera_packet *packet)
{
 struct fixture *f=context;struct timespec t;(void)force;clock_gettime(CLOCK_MONOTONIC,&t);
 *packet=(struct camera_packet){.codec=CAMERA_H264,.keyframe=1,.data=f->nal,.bytes=f->bytes,.monotonic_ns=(uint64_t)t.tv_sec*1000000000ULL+t.tv_nsec};
 fprintf(stderr,"FIXTURE PRODUCER frame=%u NOT-LIVE\n",++f->frames);return CAMERA_OK;
}
static void release(void *context,const struct camera_packet *packet){(void)context;(void)packet;}
static void idle(void *context){(void)context;}
int main(int argc,char **argv)
{
  if(argc<3||argc>5)return 2;char *end;unsigned long port=strtoul(argv[2],&end,10),deadline=100,coalesce=0;
 if(!argv[2][0]||*end||port<1024||port>65535)return 2;
  if(argc>=4){deadline=strtoul(argv[3],&end,10);if(!argv[3][0]||*end||deadline<10||deadline>2000)return 2;}
  if(argc==5){coalesce=strtoul(argv[4],&end,10);if(!argv[4][0]||*end||coalesce>1)return 2;}
 FILE *file=fopen(argv[1],"rb");if(!file)return 1;
 uint8_t *data=malloc(1048577),*storage=malloc(131072);
 if(!data||!storage){fclose(file);free(data);free(storage);return 1;}
 size_t bytes=fread(data,1,1048577,file);int error=ferror(file);fclose(file);
 if(error||bytes<27||bytes>1048576){free(data);free(storage);return 1;}
 /* Bounded public software-generated/recorded fixture: exactly SPS, PPS,
  * optional SEI and one IDR. No target media acquisition or encoding fallback. */
 uint8_t parameters[1024];size_t parameter_bytes=0,pos=0,vcl_pos=0,vcl_bytes=0;unsigned seen=0,nals=0;
 while(pos<bytes){
  size_t prefix=0;if(bytes-pos>=4&&!memcmp(data+pos,"\0\0\0\1",4))prefix=4;
  else if(bytes-pos>=3&&!memcmp(data+pos,"\0\0\1",3))prefix=3;
  if(!prefix||pos+prefix>=bytes||++nals>8)break;
  size_t start=pos+prefix,end=start+1;
  while(end<bytes&&!(bytes-end>=3&&!memcmp(data+end,"\0\0\1",3))&&!(bytes-end>=4&&!memcmp(data+end,"\0\0\0\1",4)))end++;
  unsigned kind=data[start]&31;
  if((kind==7&&seen==0)||(kind==8&&seen==1)){
   if(end-start>sizeof(parameters)-parameter_bytes-4)break;
   memcpy(parameters+parameter_bytes,"\0\0\0\1",4);parameter_bytes+=4;
   memcpy(parameters+parameter_bytes,data+start,end-start);parameter_bytes+=end-start;seen++;
  }else if(kind==6&&seen==2){}else if(kind==5&&seen==2){vcl_pos=start;vcl_bytes=end-start;seen++;}
  else break;
  pos=end;
 }
 if(pos!=bytes||seen!=3||vcl_bytes<2||vcl_bytes>131072){free(data);free(storage);return 1;}
 struct fixture f={.nal=data+vcl_pos,.bytes=vcl_bytes};
 struct camera_rtsp_producer p={.context=&f,.next=next,.release=release,.idle=idle,.parameter_sets=parameters,.parameter_set_bytes=parameter_bytes};
 const char *sdp="v=0\r\no=- 1 1 IN IP4 127.0.0.1\r\ns=RECORDED-MEDIA-FIXTURE-NOT-LIVE\r\nt=0 0\r\na=control:*\r\nm=video 0 RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\na=fmtp:96 packetization-mode=1;profile-level-id=42e01f\r\na=control:track1\r\n";
  char live[96],track[112];snprintf(live,sizeof(live),"rtsp://127.0.0.1:%lu/live",port);snprintf(track,sizeof(track),"%s/track1",live);
  struct camera_rtsp_registry registry;if(camera_rtsp_registry_init(&registry,live,track)){free(data);free(storage);return 1;}
  struct camera_rtsp_options o={.bind_address="127.0.0.1",.sdp=sdp,.port=(uint16_t)port,.interval_ms=25,.send_deadline_ms=(uint32_t)deadline,.hub_storage=storage,.packet_capacity=131072,.hub_slots=1,.tcp_coalesce=(uint32_t)coalesce,.stop=&stopped,.resources=&registry};
 signal(SIGTERM,stop);signal(SIGINT,stop);signal(SIGPIPE,SIG_IGN);
 int result=camera_rtsp_serve(&o,&p);free(data);free(storage);return result==CAMERA_OK?0:1;
}
