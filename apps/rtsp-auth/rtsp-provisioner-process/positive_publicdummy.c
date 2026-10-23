/* SPDX-License-Identifier: GPL-2.0-only
 * ONE allowed normal functional HOST PUBLICDUMMY case, not product startup.
 */
#define _GNU_SOURCE
#ifndef RTSP_HANDOFF_HOST_PUBLICDUMMY
#error "PUBLICDUMMY compile class required"
#endif
#include "provisioner_process.h"
#include <mbedtls/platform_util.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Static owner survives every failure path, including an explicitly retained
 * live keeper on unknown child retirement. No scope-exit stack HOLD. */
static struct rtsp_provisioner_process provisioner=RTSP_PROVISIONER_PROCESS_INIT;
static struct candidate_prestarted unused=CANDIDATE_PRESTARTED_INIT;
static const struct camera_device devices[]={
 {.id="fixture_sensor",.kind=CAMERA_DEVICE_SENSOR,.state=CAMERA_PRESENT_UNQUALIFIED},
 {.id="fixture_encoder",.kind=CAMERA_DEVICE_ENCODER,.state=CAMERA_PRESENT_UNQUALIFIED}
};
static const struct camera_profile profile={.id="fixture_720p",.camera_id="fixture_sensor",
 .encoder_id="fixture_encoder",.width=1280,.height=720,.codec=CAMERA_H264};
static const struct camera_graph graph={.devices=devices,.device_count=2,.profiles=&profile,.profile_count=1};
static struct camera_rtsp_registry routes;
static struct candidate_config cfg={.abi=CANDIDATE_AUTH_ABI,.struct_bytes=sizeof(cfg),
 .host_fixture_only=1,.algorithm=CAMERA_DIGEST_SHA256,.boot_generation=1,.epoch=1,
 .credential_generation=1,.device_id="fixture_device",.account_id="fixture_account",
 .profile_id="fixture_720p",.graph=&graph,.nonce_ttl_ms=5000,.rate_window_ms=1000,
 .global_verifies=16,.global_challenges=8,.account_verifies=16,.account_challenges=8,
 .peer_verifies=16,.peer_challenges=8,.begin_contract=CANDIDATE_BEGIN_LOGICAL_ONLY,
 .image={.abi=RV_ABI,.profile=RV_HOST_PUBLIC_4M,.fd=-1},.material_fd=-1};
static char directory[]="/tmp/opencode/rtsp-provisioner-process-publicdummy-XXXXXX";
static int directory_fd=-1,pipes[2]={-1,-1};

static void put32(unsigned char *out,uint32_t value)
{for(unsigned i=0;i<4;i++)out[i]=(unsigned char)(value>>(8*i));}

#define CHECK(c) do { if(!(c)){failed_line=__LINE__;goto failed;} checks++; } while(0)
int main(void)
{
 unsigned failed_line=0,checks=0;
 /* Fixed explicit PUBLICDUMMY fixture ONLY. Parent encodes/writes dummy bytes;
  * this does NOT prove a product parent was secret-free at fork. No real input.
  * Values are never printed, payload-read-back, hashed or inventoried as material. */
 const unsigned char username[]="PUBLICDUMMY",realm[]="PUBLICDUMMY fixture";
 const unsigned char password[]="PUBLICDUMMY-only-not-a-product-credential";
 unsigned char frame[RTSP_PP_INGRESS_HEADER+2*CAMERA_DIGEST_NAME_MAX+CAMERA_DIGEST_PASSWORD_MAX]={0};
 size_t u=sizeof(username)-1,r=sizeof(realm)-1,p=sizeof(password)-1,n=16+u+r+p;
 struct stat st,named;
 CHECK(getuid()!=0&&getuid()==geteuid()&&getgid()==getegid());
 CHECK(camera_rtsp_registry_init(&routes,"rtsp://127.0.0.1/fixture/live",
                                  "rtsp://127.0.0.1/fixture/live/track0")==CAMERA_OK);
 CHECK(mkdtemp(directory)!=NULL);
 directory_fd=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 CHECK(directory_fd>=0);
 CHECK(pipe2(pipes,O_CLOEXEC|O_NONBLOCK)==0);
 put32(frame,RTSP_PP_VERSION);put32(frame+4,(uint32_t)u);
 put32(frame+8,(uint32_t)r);put32(frame+12,(uint32_t)p);
 memcpy(frame+16,username,u);memcpy(frame+16+u,realm,r);memcpy(frame+16+u+r,password,p);
 CHECK(write(pipes[1],frame,n)==(ssize_t)n);
 mbedtls_platform_zeroize(frame,sizeof(frame));
 {int fd=pipes[1];pipes[1]=-1;CHECK(close(fd)==0);}
 CHECK(rtsp_provisioner_begin(&provisioner,&pipes[0],directory_fd,&cfg,&routes,&unused,3000)==RTSP_PP_MORE);
 CHECK(pipes[0]==-1&&provisioner.ingress_fd==-1&&provisioner.child_pid>0);
 CHECK(cfg.material_fd==-1&&unused.phase==CANDIDATE_PS_EMPTY&&!unused.self);
 int result=RTSP_PP_MORE;
 for(unsigned calls=0;calls<4096&&result==RTSP_PP_MORE;calls++){
  result=rtsp_provisioner_observe(&provisioner);
  if(result==RTSP_PP_MORE)(void)poll(NULL,0,1);
 }
 CHECK(result==RTSP_PP_OK&&provisioner.phase==RTSP_PP_READY);
 CHECK(provisioner.wait_observed&&provisioner.wait0&&!provisioner.child_unreaped&&
       !provisioner.wait_unknown&&!provisioner.close_unknown&&provisioner.message_received&&
       provisioner.received_count==1&&provisioner.sockets[0]==-1&&provisioner.sockets[1]==-1);
 CHECK(cfg.material_fd==-1); /* output cannot activate merely on recv/send ACK */
 CHECK(rtsp_provisioner_bind(&provisioner,&cfg,&unused)==RTSP_PP_OK);
 CHECK(provisioner.phase==RTSP_PP_BOUND&&provisioner.received[0]==-1&&cfg.material_fd>=0);
 CHECK(unused.phase==CANDIDATE_PS_EMPTY&&!unused.self&&!unused.config&&!unused.backend.child);
 CHECK(fstat(cfg.material_fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink==1&&
       st.st_uid==getuid()&&(st.st_mode&07777)==0600&&st.st_size==1536&&
       (uint64_t)st.st_dev==provisioner.received_envelope.file_device&&
       (uint64_t)st.st_ino==provisioner.received_envelope.file_inode&&
       (fcntl(cfg.material_fd,F_GETFL)&O_ACCMODE)==O_RDONLY&&
       (fcntl(cfg.material_fd,F_GETFD)&FD_CLOEXEC)&&lseek(cfg.material_fd,0,SEEK_CUR)==0);
 /* Explicit test-only success cleanup. Every slot cleared BEFORE one close;
  * linked name identity checked BEFORE one unlink. No production destructor. */
 {int fd=cfg.material_fd;cfg.material_fd=-1;CHECK(close(fd)==0);}
 CHECK(fstatat(directory_fd,RTSP_PRIVATE_HANDOFF_FILE,&named,AT_SYMLINK_NOFOLLOW)==0&&
       named.st_dev==st.st_dev&&named.st_ino==st.st_ino&&named.st_nlink==1);
 CHECK(unlinkat(directory_fd,RTSP_PRIVATE_HANDOFF_FILE,0)==0);
 provisioner.directory_fd=-1; /* borrowed slot retired only by this test owner */
 {int fd=directory_fd;directory_fd=-1;CHECK(close(fd)==0);}
 CHECK(rmdir(directory)==0);
 puts("PUBLICDUMMY normal=PASS actual_fork=1 child_ingress_constructor=1 actual_SCM_RIGHTS=1 received_FDs=1 child_wait0=1 explicit_parent_bind=1 readonly1536=1 cleanup=PASS native_hardening_or_verifier_media_launch=0 payload_readback_or_hash=0 product_secret_free_fork_proof=0");
 return 0;
failed:
 mbedtls_platform_zeroize(frame,sizeof(frame));
 fprintf(stderr,"PUBLICDUMMY normal=FAIL line=%u checks=%u phase=%u error=%u child_unreaped=%u wait_unknown=%u no_failure_cleanup=1\n",
  failed_line,checks,(unsigned)provisioner.phase,(unsigned)provisioner.error,
  provisioner.child_unreaped,provisioner.wait_unknown);
 /* Not a second trial. On failure do not unlink/close/kill/abandon an OWN child.
  * Keep the actual static owner alive until known retirement. Unknown wait leaves
  * a live keeper indefinitely for manual parent disposition, never PID retries.
  * No nominal timing guarantee or automatic external cleanup is claimed. */
 while(provisioner.child_unreaped){
  (void)rtsp_provisioner_observe(&provisioner);
  (void)poll(NULL,0,10);
 }
 return 1;
}
