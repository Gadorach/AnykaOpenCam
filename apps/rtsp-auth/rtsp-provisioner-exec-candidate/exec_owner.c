/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "exec_owner.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
_Static_assert(sizeof(struct pe_exec_owner)<131072,"bounded owner storage");
static struct pe_exec_owner *attempt;
static int hold(struct pe_exec_owner *o,enum pe_error error)
{if(o->error==PE_E_NONE)o->error=error;o->phase=PE_HOLD;return PE_REFUSED;}
static int fresh(const struct candidate_prestarted *p)
{return p&&p->phase==CANDIDATE_PS_EMPTY&&!p->self&&!p->config&&!p->backend.child&&
 !p->backend.initialized&&!p->backend.config&&!p->source_hold;}
static int directory_same(const struct pe_exec_owner *o)
{struct stat now;return !pe_directory(o->directory,&now)&&pe_same_inode(&now,&o->directory_inode);}
static int unchanged(const struct pe_exec_owner *o)
{
 unsigned char encoded[PE_PACKET];size_t n;struct stat verifier;
 /* Exact byte snapshots retain original callback/context/image/graph addresses.
  * Full value serialization also detects pointed-to graph record changes. */
 return o->config&&o->routes&&o->image&&fresh(o->unused)&&
  !memcmp(o->config,&o->config_snapshot,sizeof(*o->config))&&
  !memcmp(o->routes,&o->routes_snapshot,sizeof(*o->routes))&&
  !memcmp(o->unused,&o->unused_snapshot,sizeof(*o->unused))&&
  !memcmp(o->image,&o->image_snapshot,sizeof(*o->image))&&
  !fstat(o->config->image.fd,&verifier)&&pe_same_inode(&verifier,&o->verifier_inode)&&
  o->config->graph&&!memcmp(o->config->graph,&o->graph_snapshot,sizeof(o->graph_snapshot))&&
  !pe_encode_request(encoded,&n,o->config,o->routes,&o->binding)&&n==o->request_bytes&&
  !memcmp(encoded,o->request,n)&&directory_same(o)&&
  !pe_image_unchanged(o->image,&o->image_inode);
}
static int material_check(const struct pe_exec_owner *o)
{
 struct stat st,named;int fd=o->received[0];
 int f=fcntl(fd,F_GETFL),d=fcntl(fd,F_GETFD);
 return pe_root_ids()&&o->received_count==1&&fd>=0&&fd!=o->config->image.fd&&
  fd!=o->image->fd&&directory_same(o)&&f>=0&&(f&O_ACCMODE)==O_RDONLY&&
  (f&O_NONBLOCK)&&d>=0&&(d&FD_CLOEXEC)&&!fstat(fd,&st)&&S_ISREG(st.st_mode)&&
  !st.st_uid&&(st.st_mode&07777)==0600&&st.st_nlink==1&&st.st_size==1536&&
  (uint64_t)st.st_dev==pe_get64(o->reply+32)&&(uint64_t)st.st_ino==pe_get64(o->reply+40)&&
  !pe_same_inode(&st,&o->image_inode)&&!pe_same_inode(&st,&o->verifier_inode)&&
  lseek(fd,0,SEEK_CUR)==0&&
  !fstatat(o->directory,RTSP_PRIVATE_HANDOFF_FILE,&named,AT_SYMLINK_NOFOLLOW)&&
  S_ISREG(named.st_mode)&&!named.st_uid&&(named.st_mode&07777)==0600&&
  named.st_nlink==1&&named.st_size==1536&&pe_same_inode(&st,&named)&&directory_same(o);
}
static int valid_owner(struct pe_exec_owner *o)
{return o&&o==attempt&&o->self==o;}
static void close_parent_slot(struct pe_exec_owner *o,int *slot,unsigned mask)
{
 if(*slot>=0&&pe_close_once(slot)){o->close_unknown|=mask;hold(o,PE_E_CLOSE_UNKNOWN);}
}
int qc07f_rtsp_exec_begin_v1(struct pe_exec_owner *o,int *ingress,int directory,
 struct candidate_config *c,const struct camera_rtsp_registry *r,
 const struct candidate_prestarted *unused,const struct pe_worker_image *image,uint32_t budget)
{
 if(!o||attempt||o->self||o->phase!=PE_EMPTY||o->ingress!=-1||o->directory!=-1||
    o->control[0]!=-1||o->control[1]!=-1||o->received_count)return PE_REFUSED;
 for(unsigned i=0;i<4;i++)if(o->stage[i]!=-1)return PE_REFUSED;
 for(unsigned i=0;i<PE_RIGHTS_MAX;i++)if(o->received[i]!=-1)return PE_REFUSED;
 attempt=o;o->self=o;o->phase=PE_PENDING;o->config=c;o->routes=r;o->unused=unused;
 o->directory=directory;o->image=image;
 if(!ingress||*ingress<0)return hold(o,PE_E_PREFLIGHT);
 o->ingress=*ingress;*ingress=-1; /* sole canonical move BEFORE all preflight */
 if(!budget||budget>10000||pe_clock(&o->binding.started)||
    o->binding.started>UINT64_MAX-budget)return hold(o,PE_E_CLOCK);
 o->last=o->binding.started;o->binding.deadline=o->binding.started+budget;
 struct sigaction action;
 if(!pe_root_ids()||prctl(PR_GET_SECCOMP,0,0,0,0)!=0||!pe_config_class(c)||!fresh(unused)||
    pe_public_validate(c,r)||!image||image->fd<0||c->image.fd<0||
    image->fd==c->image.fd||image->fd==o->ingress||image->fd==directory||
    o->ingress==directory||o->ingress==c->image.fd||directory==c->image.fd||
    pe_ingress(o->ingress,&o->ingress_inode)||pe_directory(directory,&o->directory_inode)||
    fstat(c->image.fd,&o->verifier_inode)||!S_ISREG(o->verifier_inode.st_mode)||
    pe_same_inode(&o->ingress_inode,&o->verifier_inode)||
    pe_same_inode(&o->directory_inode,&o->verifier_inode)||
    sigaction(SIGCHLD,NULL,&action)||action.sa_handler!=SIG_DFL||(action.sa_flags&SA_NOCLDWAIT))
  return hold(o,PE_E_PREFLIGHT);
 if(pe_image_validate(image,&o->image_inode)||pe_same_inode(&o->image_inode,&o->verifier_inode)||
    pe_same_inode(&o->image_inode,&o->directory_inode)||pe_same_inode(&o->image_inode,&o->ingress_inode))return hold(o,PE_E_IMAGE);
 memcpy(&o->config_snapshot,c,sizeof(*c));memcpy(&o->routes_snapshot,r,sizeof(*r));
 memcpy(&o->graph_snapshot,c->graph,sizeof(o->graph_snapshot));memcpy(&o->unused_snapshot,unused,sizeof(*unused));
 memcpy(&o->image_snapshot,image,sizeof(*image));o->parent=getpid();
 int pair[2];if(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,0,pair))return hold(o,PE_E_SOCKET);
 o->control[0]=pair[0];o->control[1]=pair[1]; /* both recorded BEFORE fallible work */
 if(pe_control(o->control[1],o->parent,&o->control_inode))return hold(o,PE_E_SOCKET);
 const struct stat *st[3]={&o->ingress_inode,&o->directory_inode,&o->control_inode};
 for(unsigned i=0;i<3;i++){o->binding.device[i]=(uint64_t)st[i]->st_dev;o->binding.inode[i]=(uint64_t)st[i]->st_ino;}
 if(pe_encode_request(o->request,&o->request_bytes,c,r,&o->binding))return hold(o,PE_E_BINDING);
 int sources[4]={o->control[1],o->ingress,directory,image->fd};
 for(unsigned i=0;i<4;i++){
  o->stage[i]=fcntl(sources[i],F_DUPFD_CLOEXEC,16); /* immediately canonical */
  if(o->stage[i]<0)return hold(o,PE_E_STAGE);
 }
 if(!unchanged(o)||pe_tick(&o->last,o->binding.deadline))return hold(o,PE_E_CHANGED);
 o->child=fork();
 if(o->child<0)return hold(o,PE_E_FORK);
 if(!o->child)pe_child_exec(o->stage,o->parent);
 o->unreaped=1;o->possible_file=1; /* record OWN child before all closes/send */
 close_parent_slot(o,&o->ingress,1U);close_parent_slot(o,&o->control[1],2U);
 for(unsigned i=0;i<4;i++)close_parent_slot(o,&o->stage[i],4U<<i);
 if(o->phase==PE_HOLD)return PE_REFUSED;
 if(pe_tick(&o->last,o->binding.deadline)||!unchanged(o))return hold(o,PE_E_CHANGED);
 /* One whole public packet, no ancillary, no retry even EINTR/unknown delivery. */
 if(send(o->control[0],o->request,o->request_bytes,MSG_DONTWAIT|MSG_NOSIGNAL)!=(ssize_t)o->request_bytes)return hold(o,PE_E_SEND);
 if(pe_tick(&o->last,o->binding.deadline))return hold(o,PE_E_CLOCK);
 return PE_MORE;
}
static void observe_child(struct pe_exec_owner *o)
{
 if(!o->unreaped||o->wait_unknown)return;
 int status;pid_t p=waitpid(o->child,&status,WNOHANG);
 if(!p||(p<0&&errno==EINTR))return;
 if(p!=o->child){o->wait_unknown=1;hold(o,PE_E_WAIT_UNKNOWN);return;}
 o->unreaped=0;o->wait_observed=1;o->wait_status=status;o->wait0=WIFEXITED(status)&&WEXITSTATUS(status)==0;
 if(!o->wait0)hold(o,PE_E_CHILD);
}
static int receive(struct pe_exec_owner *o)
{
 ssize_t n;unsigned bad;
 int result=pe_receive(o->control[0],o->reply,PE_REPLY,&n,o->received,&o->received_count,&bad);
 if(result==1)return PE_MORE;
 if(result<0)return hold(o,PE_E_RECEIVE);
 o->message_received=1;
 if(n!=PE_REPLY||bad||o->received_count!=1)return hold(o,PE_E_RECEIVE);
 unsigned char expected[PE_REPLY];
 if(!unchanged(o)||pe_encode_reply(expected,o->config,o->routes,pe_get64(o->reply+32),pe_get64(o->reply+40))||
    memcmp(expected,o->reply,PE_REPLY))return hold(o,PE_E_BINDING);
 if(!material_check(o))return hold(o,PE_E_METADATA);
 return PE_OK;
}
int qc07f_rtsp_exec_observe_v1(struct pe_exec_owner *o)
{
 if(!valid_owner(o))return PE_REFUSED;
 if(o->phase==PE_HOLD){observe_child(o);return PE_REFUSED;}
 if(o->phase!=PE_PENDING)return o->phase==PE_READY?PE_OK:PE_REFUSED;
 if(pe_tick(&o->last,o->binding.deadline))hold(o,PE_E_CLOCK);
 if(o->phase!=PE_HOLD&&!o->message_received)(void)receive(o);
 observe_child(o);
 if(o->phase==PE_HOLD)return PE_REFUSED;
 if(!o->message_received||!o->wait0)return PE_MORE;
 if(!unchanged(o))return hold(o,PE_E_CHANGED);
 if(!material_check(o))return hold(o,PE_E_METADATA);
 close_parent_slot(o,&o->control[0],64U);
 if(o->phase==PE_HOLD)return PE_REFUSED;
 if(pe_tick(&o->last,o->binding.deadline))return hold(o,PE_E_CLOCK);
 o->phase=PE_READY;return PE_OK;
}
int qc07f_rtsp_exec_bind_v1(struct pe_exec_owner *o,struct candidate_config *c,const struct candidate_prestarted *unused)
{
 if(!valid_owner(o)||o->phase!=PE_READY)return PE_REFUSED;
 if(c!=o->config||unused!=o->unused||!unchanged(o)||!o->wait_observed||!o->wait0||
    o->unreaped||o->wait_unknown||o->close_unknown||o->ingress!=-1||o->control[0]!=-1||o->control[1]!=-1)
  return hold(o,PE_E_CHANGED);
 for(unsigned i=0;i<4;i++)if(o->stage[i]!=-1)return hold(o,PE_E_CHANGED);
 if(pe_tick(&o->last,o->binding.deadline))return hold(o,PE_E_CLOCK);
 unsigned char expected[PE_REPLY];
 if(pe_encode_reply(expected,c,o->routes,pe_get64(o->reply+32),pe_get64(o->reply+40))||
    memcmp(expected,o->reply,PE_REPLY)||!material_check(o))return hold(o,PE_E_METADATA);
 if(pe_tick(&o->last,o->binding.deadline))return hold(o,PE_E_CLOCK);
 int fd=o->received[0];o->received[0]=-1;c->material_fd=fd;o->phase=PE_BOUND;
 return PE_OK;
}
