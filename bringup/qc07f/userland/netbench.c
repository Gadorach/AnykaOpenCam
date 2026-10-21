/* SPDX-License-Identifier: GPL-2.0-only
 * Bounded owned-host TCP goodput test, independent of camera capture/JPEG.
 * Fixed public pattern, no credentials, file/card/SPI or hardware writes.
 */
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
static uint64_t ms(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
static int write_all(int fd,const void *data,size_t n){const uint8_t *p=data;while(n){ssize_t r=send(fd,p,n,MSG_NOSIGNAL);if(r<0&&errno==EINTR)continue;if(r<=0)return -1;p+=r;n-=r;}return 0;}
static int read_all(int fd,void *data,size_t n){uint8_t *p=data;while(n){ssize_t r=recv(fd,p,n,0);if(r<0&&errno==EINTR)continue;if(r<=0)return -1;p+=r;n-=r;}return 0;}
int main(int argc,char **argv)
{
 if(argc!=4||strcmp(argv[1],"192.168.1.15")||(strcmp(argv[2],"up")&&strcmp(argv[2],"down")))return 2;
 unsigned long size=strtoul(argv[3],NULL,10);if(size<32768||size>2097152)return 2;
 int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return 1;
 struct timeval timeout={4,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
 struct sockaddr_in peer={.sin_family=AF_INET,.sin_port=htons(19562)};inet_pton(AF_INET,argv[1],&peer.sin_addr);
 if(connect(fd,(struct sockaddr *)&peer,sizeof(peer))){perror("bench connect");close(fd);return 1;}
 uint8_t header[16]={ 'Q','C','N','E','T','B','0','1',0,0,0,0,0,0,0,0 },buffer[16384],ack[8];
 header[8]=!strcmp(argv[2],"up")?'U':'D';for(unsigned i=0;i<4;i++)header[12+i]=size>>(8*i);
 if(write_all(fd,header,sizeof(header))||read_all(fd,ack,sizeof(ack))||memcmp(ack,"READY001",8))goto fail;
 uint64_t begin=ms();unsigned long offset=0;
 while(offset<size){
  size_t n=size-offset;if(n>sizeof(buffer))n=sizeof(buffer);
  if(header[8]=='U'){for(size_t i=0;i<n;i++)buffer[i]=(offset+i)&255;if(write_all(fd,buffer,n))goto fail;}
  else{if(read_all(fd,buffer,n))goto fail;for(size_t i=0;i<n;i++)if(buffer[i]!=(uint8_t)(offset+i))goto fail;}
  offset+=n;if(ms()-begin>30000)goto fail;
 }
 if(header[8]=='U'){if(read_all(fd,ack,8)||memcmp(ack,"PASS0001",8))goto fail;}
 else if(write_all(fd,"PASS0001",8))goto fail;
 uint64_t elapsed=ms()-begin;
 printf("QCNETBENCH direction=%s bytes=%lu elapsed_ms=%llu pattern_verified=1\n",argv[2],size,(unsigned long long)elapsed);
 close(fd);return 0;
fail:fprintf(stderr,"QCNETBENCH failed; no complete/padded transfer claim errno=%d\n",errno);close(fd);return 1;
}
