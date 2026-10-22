/* SPDX-License-Identifier: GPL-2.0-only */
#define _POSIX_C_SOURCE 200809L
#include "encoder_fd_custody.h"
#include "stage-telemetry-uapi.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

_Static_assert(sizeof(struct qc_hwenc_open_query)==88,"frozen query ABI");

static int schema(const struct qc_hwenc_open_query *q)
{
 if(q->version!=QC_HWENC_OPEN_QUERY_VERSION||q->struct_bytes!=sizeof(*q)||
    q->reserved||!q->session_identity||q->reference_frame_num>65535||
    q->quarantined>1||q->dma_uncertain>1||q->guard_poisoned>1||q->ledger_active>1)
  return 0;
 switch(q->reference_relation){
 case QC_HWENC_REFERENCE_NONE:
  return !q->reference_owner_identity&&!q->reference_frame_num;
 case QC_HWENC_REFERENCE_THIS_OPEN:
  return q->reference_owner_identity==q->session_identity;
 case QC_HWENC_REFERENCE_OTHER_OPEN:
  return q->reference_owner_identity&&q->reference_owner_identity!=q->session_identity;
 default:return 0;
 }
}

static int clear_observation(const struct qc_hwenc_open_query *q)
{
 /* Observational refusal predicate, NEVER a health/permission/retirement grant.
  * Saturating generations/history remain exactly in last_observed. Known error
  * or unknown telemetry flags refuse, rather than reinterpret a future ABI. */
 return q->reference_relation==QC_HWENC_REFERENCE_NONE&&
        !q->reference_owner_identity&&!q->reference_frame_num&&
        !q->quarantined&&!q->dma_uncertain&&!q->guard_poisoned&&!q->ledger_active&&
        !(q->telemetry_flags&~QC_STAGE_F_SATURATED);
}

static int observe(int fd,const struct qc_encoder_fd_identity *expected,
                    struct stat *node,struct qc_hwenc_open_query *q)
{
 if(fd<3)return QC_ENCODER_FD_INVALID;
 int flags=fcntl(fd,F_GETFL),descriptor_flags=fcntl(fd,F_GETFD);
 if(flags<0||descriptor_flags<0||fstat(fd,node))return QC_ENCODER_FD_SYSTEM;
 if((flags&O_ACCMODE)!=O_RDWR||!(descriptor_flags&FD_CLOEXEC)||
    !S_ISCHR(node->st_mode)||node->st_rdev!=expected->device)
  return QC_ENCODER_FD_IDENTITY;
 *q=(struct qc_hwenc_open_query){0};
 if(ioctl(fd,QC_HWENC_OPEN_QUERY,q))return QC_ENCODER_FD_SYSTEM;
 /* Output-only query. Never inspect a partial/failed reply, synthesize support
  * or substitute global LIFETIME/INFO/stage queries, even on ENOTTY. */
 if(!schema(q))return QC_ENCODER_FD_SNAPSHOT;
 if(q->session_identity!=expected->session_identity)return QC_ENCODER_FD_IDENTITY;
 return QC_ENCODER_FD_OK;
}

int qc_encoder_fd_take(struct qc_encoder_fd_custody *owner,int *bootstrap_slot,
                       const struct qc_encoder_fd_identity *expected)
{
 if(!owner||!bootstrap_slot||!expected||!expected->device||!expected->session_identity||
    owner->fd!=-1||owner->state!=QC_ENCODER_FD_EMPTY||owner->identity.device||
    owner->identity.session_identity)return QC_ENCODER_FD_INVALID;
 struct stat node;
 struct qc_hwenc_open_query q;
 int result=observe(*bootstrap_slot,expected,&node,&q);
 if(result)return result;
 if(!clear_observation(&q))return QC_ENCODER_FD_NOT_CLEAR;
 *owner=(struct qc_encoder_fd_custody){.fd=*bootstrap_slot,.state=QC_ENCODER_FD_HELD,
  .identity=*expected,.node_filesystem=node.st_dev,.node_inode=node.st_ino,.last_observed=q};
 *bootstrap_slot=-1;
 return QC_ENCODER_FD_OK;
}

int qc_encoder_fd_give_back(struct qc_encoder_fd_custody *owner,int *recipient_slot)
{
 if(!owner||!recipient_slot||*recipient_slot!=-1)return QC_ENCODER_FD_INVALID;
 if(owner->state==QC_ENCODER_FD_HOLD)return QC_ENCODER_FD_UNCERTAIN;
 if(owner->state!=QC_ENCODER_FD_HELD||owner->fd<3)return QC_ENCODER_FD_INVALID;
 struct stat node;
 struct qc_hwenc_open_query q;
 int result=observe(owner->fd,&owner->identity,&node,&q);
 if(!result)owner->last_observed=q;
 if(result||node.st_dev!=owner->node_filesystem||node.st_ino!=owner->node_inode||
    !clear_observation(&q)){
  owner->state=QC_ENCODER_FD_HOLD;
  return result?result:QC_ENCODER_FD_UNCERTAIN;
 }
 *recipient_slot=owner->fd;
 *owner=(struct qc_encoder_fd_custody)QC_ENCODER_FD_CUSTODY_EMPTY;
 return QC_ENCODER_FD_OK;
}
