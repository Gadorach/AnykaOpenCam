/* SPDX-License-Identifier: GPL-2.0-only
 * Deterministic partial-write/EAGAIN/deadline fixture; no real sockets/devices.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
static uint64_t fake_ms=1000;
static uint8_t received[3][16384];static size_t received_bytes[3];
static unsigned attempts[3],closed,stalled_attempts;
static int fake_clock(clockid_t clock,struct timespec *t)
{(void)clock;t->tv_sec=(time_t)(fake_ms/1000);t->tv_nsec=(long)(fake_ms%1000)*1000000;return 0;}
static ssize_t fake_send(int fd,const void *data,size_t bytes,int flags)
{
 (void)flags;
 if(fd==100){stalled_attempts++;errno=EAGAIN;return -1;}
 assert(fd>=101&&fd<=103);unsigned i=(unsigned)(fd-101);
 if(++attempts[i]%3==0){errno=EAGAIN;return -1;}
 size_t cap=i==0?17:i==1?31:127;if(bytes>cap)bytes=cap;
 assert(received_bytes[i]+bytes<=sizeof(received[i]));
 memcpy(received[i]+received_bytes[i],data,bytes);received_bytes[i]+=bytes;return (ssize_t)bytes;
}
static int fake_poll(struct pollfd *fds,nfds_t count,int timeout)
{
 for(nfds_t i=0;i<count;i++)if(fds[i].fd>=101){fds[i].revents=POLLOUT;return 1;}
 assert(timeout>=0);fake_ms+=(unsigned)timeout;return 0;
}
static int fake_close(int fd){assert(fd==100);closed++;return 0;}
#define clock_gettime fake_clock
#define send fake_send
#define poll fake_poll
#define close fake_close
#include "../src/rtsp_server.c"
#undef clock_gettime
#undef send
#undef poll
#undef close
static void verify(unsigned i,const uint8_t *nal,size_t bytes)
{
 uint8_t reconstructed[6000];size_t used=0,pos=0;unsigned seq=1,markers=0,starts=0,ends=0;
 while(pos<received_bytes[i]){
  const uint8_t *p=received[i]+pos;assert(received_bytes[i]-pos>=4&&p[0]=='$'&&p[1]==i*2);
  size_t n=4+((unsigned)p[2]<<8)+p[3];assert(n<=received_bytes[i]-pos&&n>=17);
  assert(((unsigned)p[6]<<8|p[7])==seq++);markers+=!!(p[5]&0x80);
  if((p[16]&31)==28){
   if(p[17]&0x80){assert(!used);reconstructed[used++]=(p[16]&0xe0)|(p[17]&31);starts++;}
   ends+=!!(p[17]&0x40);assert(used+n-18<=sizeof(reconstructed));
   memcpy(reconstructed+used,p+18,n-18);used+=n-18;
  }else assert((p[16]&31)==7||(p[16]&31)==8);
  pos+=n;
 }
 assert(used==bytes&&!memcmp(reconstructed,nal,bytes)&&markers==1&&starts==1&&ends==1);
}
int main(void)
{
 uint8_t storage[8192],nal[6000];for(unsigned i=0;i<sizeof(nal);i++)nal[i]=(uint8_t)(i*31);nal[0]=0x65;
 static const uint8_t params[]={0,0,0,1,0x67,1,2,3,0,0,0,1,0x68,4,5};
 struct camera_packet_hub hub;assert(camera_hub_init_count(&hub,storage,sizeof(storage),1)==CAMERA_OK);
 struct rtsp_client clients[4];memset(clients,0,sizeof(clients));
 for(unsigned i=0;i<4;i++){
  assert(camera_hub_subscribe(&hub,i)==CAMERA_OK);clients[i].fd=100+(int)i;clients[i].playing=1;clients[i].tcp_coalesce=1;
  clients[i].rtp=(struct camera_rtp_session){.sequence=1,.channel=i?((i-1)*2):6,.payload_type=96,.ssrc=123+i};
 }
 struct camera_packet packet={.codec=CAMERA_H264,.data=nal,.bytes=sizeof(nal),.keyframe=1,.monotonic_ns=1000000000};
 assert(camera_hub_publish(&hub,&packet)==CAMERA_OK);
 struct camera_rtsp_producer producer={.parameter_sets=params,.parameter_set_bytes=sizeof(params)};
 send_access_unit(clients,&hub,&producer,500,1);
 assert(closed==1&&clients[0].fd==-1&&stalled_attempts>10&&fake_ms==1500);
 for(unsigned i=0;i<3;i++){assert(clients[i+1].playing&&clients[i+1].send_error==0);verify(i,nal,sizeof(nal));}
 /* One-slot publish succeeds again only because every old lease was released. */
 assert(camera_hub_publish(&hub,&packet)==CAMERA_OK);
 camera_hub_stop(&hub);
 puts("Fair AU cursors: three partial/EAGAIN peers complete byte-exactly before stalled-peer deadline; leases released PASS");return 0;
}
