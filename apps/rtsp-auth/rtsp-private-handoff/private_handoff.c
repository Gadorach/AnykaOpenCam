/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "private_handoff.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <unistd.h>

_Static_assert(sizeof(struct rv_material)==1536,"actual verifier material ABI");

static int ids_valid(void)
{
#ifdef RTSP_HANDOFF_HOST_PUBLICDUMMY
 return getuid()!=0&&getuid()==geteuid()&&getgid()==getegid();
#else
 uid_t r,e,s;gid_t gr,ge,gs;
 return !getresuid(&r,&e,&s)&&!getresgid(&gr,&ge,&gs)&&
        !r&&!e&&!s&&!gr&&!ge&&!gs&&getgroups(0,NULL)==0;
#endif
}

#ifndef RTSP_HANDOFF_HOST_PUBLICDUMMY
int qc07f_rtsp_private_provisioner_harden_v1(void)
{
 struct rlimit core={0,0},observed;
 if(!ids_valid()||setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||
    prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||mlockall(MCL_CURRENT|MCL_FUTURE)||
    getrlimit(RLIMIT_CORE,&observed)||observed.rlim_cur||observed.rlim_max||
    prctl(PR_GET_DUMPABLE)!=0||prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)!=1)
  return CAMERA_NOT_READY;
 return CAMERA_OK;
}
#endif

static int config_class(const struct candidate_config *cfg)
{
 if(!cfg||cfg->abi!=CANDIDATE_AUTH_ABI||cfg->struct_bytes!=sizeof(*cfg)||
    cfg->material_fd!=-1||cfg->image.abi!=RV_ABI)return 0;
#ifdef RTSP_HANDOFF_HOST_PUBLICDUMMY
 return cfg->host_fixture_only==1&&cfg->image.profile==RV_HOST_PUBLIC_4M;
#else
 return !cfg->host_fixture_only&&cfg->image.profile==RV_NATIVE;
#endif
}

static int directory_stat(const struct stat *st)
{return S_ISDIR(st->st_mode)&&st->st_uid==getuid()&&
        (st->st_mode&07777)==0700&&st->st_nlink!=0;}

static int same_inode(const struct stat *a,const struct stat *b)
{return a->st_dev==b->st_dev&&a->st_ino==b->st_ino;}

static int directory_recheck(const struct rtsp_private_handoff *h)
{
 struct stat st;int flags=fcntl(h->directory_fd,F_GETFD);
 return flags>=0&&(flags&FD_CLOEXEC)&&!fstat(h->directory_fd,&st)&&
        directory_stat(&st)&&same_inode(&st,&h->directory_inode);
}

static int material_stat(const struct stat *st,off_t bytes)
{return S_ISREG(st->st_mode)&&st->st_uid==getuid()&&
        (st->st_mode&07777)==0600&&st->st_nlink==1&&st->st_size==bytes;}

static int named_recheck(const struct rtsp_private_handoff *h)
{
 struct stat st;
 return directory_recheck(h)&&h->identity_valid&&
        !fstatat(h->directory_fd,RTSP_PRIVATE_HANDOFF_FILE,&st,AT_SYMLINK_NOFOLLOW)&&
        material_stat(&st,1536)&&same_inode(&st,&h->file_inode);
}

static int readonly_recheck(const struct rtsp_private_handoff *h)
{
 struct stat st;int access=fcntl(h->readonly_fd,F_GETFL);
 int descriptor=fcntl(h->readonly_fd,F_GETFD);
 return ids_valid()&&named_recheck(h)&&access>=0&&
        (access&O_ACCMODE)==O_RDONLY&&descriptor>=0&&(descriptor&FD_CLOEXEC)&&
        !fstat(h->readonly_fd,&st)&&material_stat(&st,1536)&&
        same_inode(&st,&h->file_inode)&&lseek(h->readonly_fd,0,SEEK_CUR)==0&&
        named_recheck(h);
}

static int close_writer_once(struct rtsp_private_handoff *h)
{
 /* Actual ownership move to this one close; NEVER close again even on EINTR.
  * A failed close cannot establish whether a live writer remains: sticky HOLD. */
 int fd=h->writer_fd;h->writer_fd=-1;
 if(close(fd)){
  h->phase=RTSP_PH_WRITER_CLOSE_UNKNOWN;
  return CAMERA_IO_ERROR;
 }
 h->writer_close_known=1;
 return CAMERA_OK;
}

int rtsp_private_handoff_create(struct rtsp_private_handoff *h,int directory_fd,
 const struct candidate_config *cfg,const struct camera_rtsp_registry *routes,
 struct camera_digest_span username,struct camera_digest_span realm,
 struct camera_digest_span password,struct rv_material *scratch)
{
 int result=CAMERA_INVALID,complete=0;struct stat st;
 if(!scratch)return CAMERA_INVALID;
 qc07f_rtsp_startup_material_clear_v1(scratch);
 if(!h||h->phase!=RTSP_PH_EMPTY||h->self||h->config||h->routes||
    h->writer_fd!=-1||h->readonly_fd!=-1||h->directory_fd!=-1)goto done;
 h->self=h;h->phase=RTSP_PH_ATTEMPTING;h->config=cfg;h->routes=routes;
 h->directory_fd=directory_fd;
 if(!config_class(cfg)||!routes||!ids_valid()||directory_fd<0||
    fstat(directory_fd,&h->directory_inode)||!directory_stat(&h->directory_inode)||
    !directory_recheck(h))goto rejected;
#ifndef RTSP_HANDOFF_HOST_PUBLICDUMMY
 result=qc07f_rtsp_private_provisioner_harden_v1();
 if(result)goto rejected;
 result=qc07f_rtsp_startup_material_build_v1(cfg,routes,username,realm,password,scratch);
#else
 /* This local public shape is ONLY for exercising the original constructor.
  * Different exported handoff symbols and PUBLICDUMMY account prohibit native use. */
 struct candidate_config fixture_shape=*cfg;
 fixture_shape.host_fixture_only=0;fixture_shape.image.profile=RV_NATIVE;
 result=qc07f_rtsp_startup_material_build_v1(&fixture_shape,routes,username,realm,password,scratch);
 if(!result)scratch->account=UINT64_C(0x505542444d59);
#endif
 if(result)goto rejected;
 result=CAMERA_IO_ERROR;
 if(!directory_recheck(h))goto rejected;
 h->writer_fd=openat(directory_fd,RTSP_PRIVATE_HANDOFF_FILE,
                    O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
 if(h->writer_fd<0)goto rejected;
 h->created=1;h->phase=RTSP_PH_CREATED_PARTIAL;
 /* Do not chmod/repair unexpected metadata or adopt a pre-existing file. */
 if(fstat(h->writer_fd,&h->file_inode)||!material_stat(&h->file_inode,0))goto writer_done;
 h->identity_valid=1;
 int access=fcntl(h->writer_fd,F_GETFL),descriptor=fcntl(h->writer_fd,F_GETFD);
 if(access<0||(access&O_ACCMODE)!=O_WRONLY||descriptor<0||!(descriptor&FD_CLOEXEC))goto writer_done;
 for(unsigned calls=0;calls<64&&h->bytes_written<sizeof(*scratch);calls++){
  ssize_t n=write(h->writer_fd,(const unsigned char *)scratch+h->bytes_written,
                  sizeof(*scratch)-h->bytes_written);
  if(n<0&&errno==EINTR)continue;
  if(n<=0)goto writer_done;
  h->bytes_written+=(size_t)n;
 }
 if(h->bytes_written!=sizeof(*scratch)||fstat(h->writer_fd,&st)||
    !material_stat(&st,1536)||!same_inode(&st,&h->file_inode)||
    !named_recheck(h)||fsync(h->writer_fd))goto writer_done;
 h->file_synced=1;
 if(fstat(h->writer_fd,&st)||!material_stat(&st,1536)||
    !same_inode(&st,&h->file_inode)||!named_recheck(h))goto writer_done;
 complete=1;
writer_done:
 if(close_writer_once(h))goto done;
 if(!complete)goto done; /* CREATED_PARTIAL preserved, even if all bytes landed */
 h->phase=RTSP_PH_CLOSED_FILE_HOLD;
 if(!named_recheck(h))goto done;
 h->readonly_fd=openat(directory_fd,RTSP_PRIVATE_HANDOFF_FILE,
                       O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);
 if(h->readonly_fd<0)goto done;
 h->phase=RTSP_PH_READONLY_FILLED; /* Slot is owned BEFORE any fallible recheck. */
 if(!readonly_recheck(h)){h->phase=RTSP_PH_READONLY_HOLD;goto done;}
 h->phase=RTSP_PH_READY;result=CAMERA_OK;goto done;
rejected:
 h->phase=RTSP_PH_REJECTED;
done:
 qc07f_rtsp_startup_material_clear_v1(scratch);
 return result;
}

int rtsp_private_handoff_move(struct rtsp_private_handoff *h,
 struct candidate_config *cfg,const struct candidate_prestarted *owner)
{
 if(!h||h->self!=h||h->phase!=RTSP_PH_READY||h->readonly_fd<0)return CAMERA_INVALID;
 h->phase=RTSP_PH_READONLY_HOLD; /* An unsuccessful move never loses this FD. */
 if(cfg!=h->config||!config_class(cfg)||!owner||owner->phase!=CANDIDATE_PS_EMPTY||
    owner->self||owner->config||owner->backend.child||owner->backend.initialized||
    owner->backend.config||owner->source_hold||!h->writer_close_known||
    h->writer_fd!=-1||!h->file_synced||h->bytes_written!=1536||
    cfg->image.fd==h->readonly_fd||!readonly_recheck(h))return CAMERA_NOT_READY;
 /* Serialized trusted C move, not a concurrency-safe transfer or FD duplicate. */
 int fd=h->readonly_fd;h->readonly_fd=-1;cfg->material_fd=fd;
 h->phase=RTSP_PH_MOVED;
 return CAMERA_OK;
}
