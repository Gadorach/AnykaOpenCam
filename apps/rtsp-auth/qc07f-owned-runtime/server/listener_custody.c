/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "listener_custody.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int socket_integer(int fd,int option,int expected)
{
 int value=0;socklen_t bytes=sizeof(value);
 return getsockopt(fd,SOL_SOCKET,option,&value,&bytes)||
        bytes!=sizeof(value)||value!=expected;
}

int owned_listener_observe(int fd,uint16_t port,struct owned_listener_observation *out)
{
 struct owned_listener_observation observed={.fd=fd};
 struct sockaddr_in address;memset(&address,0,sizeof(address));
 socklen_t bytes=sizeof(address);int flags,descriptor_flags;
 if(!out||fd<0||port<1024)return CAMERA_INVALID;
 if(fstat(fd,&observed.identity)||!S_ISSOCK(observed.identity.st_mode)||
    (flags=fcntl(fd,F_GETFL))<0||!(flags&O_NONBLOCK)||
    (descriptor_flags=fcntl(fd,F_GETFD))<0||!(descriptor_flags&FD_CLOEXEC)||
    socket_integer(fd,SO_TYPE,SOCK_STREAM)||
    socket_integer(fd,SO_ACCEPTCONN,1)||socket_integer(fd,SO_PROTOCOL,IPPROTO_TCP)||
    getsockname(fd,(struct sockaddr *)&address,&bytes)||bytes!=sizeof(address)||
    address.sin_family!=AF_INET||address.sin_addr.s_addr!=htonl(INADDR_LOOPBACK)||
    address.sin_port!=htons(port))return CAMERA_NOT_READY;
 *out=observed;return CAMERA_OK;
}

int owned_listener_move(int *caller,uint16_t port,const struct owned_listener_observation *before,
 int *owned,struct camera_rtsp_owned_listener_outcome_v1 *outcome)
{
 struct owned_listener_observation after;
 if(!caller||!before||!owned||!outcome||*owned!=-1||outcome->consumed||
    *caller!=before->fd)return CAMERA_INVALID;
 int r=owned_listener_observe(*caller,port,&after);if(r)return r;
 if(after.identity.st_dev!=before->identity.st_dev||
    after.identity.st_ino!=before->identity.st_ino)return CAMERA_NOT_READY;
 /* Caller supplies exact serialized FD-table ownership; observations alone
  * cannot prove absence of dup/fork/SCM aliases. No callback/effect in commit. */
 *owned=*caller;*caller=-1;outcome->consumed=1;
 return CAMERA_OK;
}

int owned_listener_close_once(int *owned,struct camera_rtsp_owned_listener_outcome_v1 *outcome)
{
 if(!owned||!outcome||*owned<0||!outcome->consumed||outcome->close_attempted)
  return CAMERA_INVALID;
 int fd=*owned;*owned=-1;outcome->close_attempted=1;
 if(close(fd)){outcome->close_errno=errno;return CAMERA_IO_ERROR;}
 return CAMERA_OK; /* One successful close, not a source/DMA retirement proof. */
}
