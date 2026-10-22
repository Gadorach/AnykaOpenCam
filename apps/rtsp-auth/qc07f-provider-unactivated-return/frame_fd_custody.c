/* SPDX-License-Identifier: GPL-2.0-only
 * Narrow preopened frame-FD custody; no acquisition or provider operation API.
 */
#define _POSIX_C_SOURCE 200809L
#include "frame_fd_custody.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

_Static_assert(sizeof(struct qc_capture_receipt)==56,"retained receipt ABI");

static int header(const struct qc_capture_receipt *q)
{
 return q->version==QC_CAPTURE_RECEIPT_VERSION&&!q->reserved&&!q->reserved2&&
        q->source_instance&&q->file_instance&&q->state<=QC_RECEIPT_UNCERTAIN&&
        q->reason<=QC_STOP_CLOSE;
}

static int fresh(const struct qc_capture_receipt *q)
{
 return !q->generation&&q->state==QC_RECEIPT_NONE&&q->reason==QC_STOP_NONE&&
        !q->stop_result&&!q->stopped_ns;
}

static int observe(int fd,const struct qc_frame_fd_identity *expected,
                    struct stat *node,struct qc_capture_receipt *q)
{
 if(fd<3)return QC_FRAME_FD_INVALID;
 int flags=fcntl(fd,F_GETFL),descriptor_flags=fcntl(fd,F_GETFD);
 if(flags<0||descriptor_flags<0||fstat(fd,node))return QC_FRAME_FD_SYSTEM;
 if((flags&O_ACCMODE)!=O_RDONLY||!(descriptor_flags&FD_CLOEXEC)||
    !S_ISCHR(node->st_mode)||node->st_rdev!=expected->device)
  return QC_FRAME_FD_IDENTITY;
 *q=(struct qc_capture_receipt){.version=QC_CAPTURE_RECEIPT_VERSION};
 /* Exact implementation: BIND copies the existing per-open receipt, no ARM,
  * generation increment, reservation, reset, allocation or hardware operation.
  * Never inspect even a partially copied reply on a failed syscall. */
 if(ioctl(fd,QC_ISP_RECEIPT_BIND,q))return QC_FRAME_FD_SYSTEM;
 if(!header(q)||q->source_instance!=expected->source_instance||
    q->file_instance!=expected->file_instance)return QC_FRAME_FD_IDENTITY;
 return QC_FRAME_FD_OK;
}

int qc_frame_fd_take(struct qc_frame_fd_custody *owner,int *bootstrap_slot,
                     const struct qc_frame_fd_identity *expected)
{
 if(!owner||!bootstrap_slot||!expected||!expected->device||
    !expected->source_instance||!expected->file_instance||
    owner->fd!=-1||owner->state!=QC_FRAME_FD_EMPTY||
    owner->identity.source_instance||owner->identity.file_instance)
  return QC_FRAME_FD_INVALID;
 struct stat node;
 struct qc_capture_receipt q;
 int result=observe(*bootstrap_slot,expected,&node,&q);
 if(result)return result;
 if(!fresh(&q))return QC_FRAME_FD_NOT_FRESH;
 /* Caller serializes the entire commit, including signal/fork/fd-table access.
  * No kernel reference is duplicated; the original owning slot is invalidated. */
 *owner=(struct qc_frame_fd_custody){.fd=*bootstrap_slot,.state=QC_FRAME_FD_HELD,
  .identity=*expected,.node_filesystem=node.st_dev,.node_inode=node.st_ino,
  .last_observed=q};
 *bootstrap_slot=-1;
 return QC_FRAME_FD_OK;
}

int qc_frame_fd_give_back(struct qc_frame_fd_custody *owner,int *recipient_slot)
{
 if(!owner||!recipient_slot||*recipient_slot!=-1)return QC_FRAME_FD_INVALID;
 if(owner->state==QC_FRAME_FD_HOLD)return QC_FRAME_FD_UNCERTAIN;
 if(owner->state!=QC_FRAME_FD_HELD||owner->fd<3)return QC_FRAME_FD_INVALID;
 struct stat node;
 struct qc_capture_receipt q;
 int result=observe(owner->fd,&owner->identity,&node,&q);
 if(!result)owner->last_observed=q;
 if(result||node.st_dev!=owner->node_filesystem||node.st_ino!=owner->node_inode||
    !fresh(&q)){
  owner->state=QC_FRAME_FD_HOLD;
  return result?result:QC_FRAME_FD_UNCERTAIN;
 }
 *recipient_slot=owner->fd;
 *owner=(struct qc_frame_fd_custody)QC_FRAME_FD_CUSTODY_EMPTY;
 return QC_FRAME_FD_OK;
}
