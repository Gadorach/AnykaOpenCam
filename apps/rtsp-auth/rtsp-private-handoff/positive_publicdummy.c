/* SPDX-License-Identifier: GPL-2.0-only
 * ONE normal functional HOST PUBLICDUMMY fixture. Never native/media launch.
 */
#define _GNU_SOURCE
#ifndef RTSP_HANDOFF_HOST_PUBLICDUMMY
#error "This harness is only for the separately compiled PUBLICDUMMY handoff"
#endif
#include "private_handoff.h"
#include <mbedtls/platform_util.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(c) do { if(!(c)){failed_line=__LINE__;goto failed;} checks++; } while(0)

int main(void)
{
 unsigned checks=0,failed_line=0;
 struct rtsp_private_handoff h=RTSP_PRIVATE_HANDOFF_INIT;
 struct candidate_prestarted owner=CANDIDATE_PRESTARTED_INIT;
 struct rv_material scratch={0};
 /* Fixed explicitly PUBLIC fixture ONLY, not operator input/product defaults.
  * Values are never printed, inventoried, hashed or read back from the file. */
 unsigned char password[]="PUBLICDUMMY-only-not-a-product-credential";
 const unsigned char username[]="PUBLICDUMMY",realm[]="PUBLICDUMMY fixture";
 const struct camera_device devices[]={
  {.id="fixture_sensor",.kind=CAMERA_DEVICE_SENSOR,.state=CAMERA_PRESENT_UNQUALIFIED},
  {.id="fixture_encoder",.kind=CAMERA_DEVICE_ENCODER,.state=CAMERA_PRESENT_UNQUALIFIED}
 };
 const struct camera_profile profile={.id="fixture_720p",.camera_id="fixture_sensor",
  .encoder_id="fixture_encoder",.width=1280,.height=720,.codec=CAMERA_H264};
 const struct camera_graph graph={.devices=devices,.device_count=2,.profiles=&profile,.profile_count=1};
 struct camera_rtsp_registry routes;
 struct candidate_config cfg={.abi=CANDIDATE_AUTH_ABI,.struct_bytes=sizeof(cfg),
  .host_fixture_only=1,.algorithm=CAMERA_DIGEST_SHA256,.boot_generation=1,.epoch=1,
  .credential_generation=1,.device_id="fixture_device",.account_id="fixture_account",
  .profile_id="fixture_720p",.graph=&graph,.nonce_ttl_ms=5000,.rate_window_ms=1000,
  .global_verifies=16,.global_challenges=8,.account_verifies=16,.account_challenges=8,
  .peer_verifies=16,.peer_challenges=8,.begin_contract=CANDIDATE_BEGIN_LOGICAL_ONLY,
  .image={.abi=RV_ABI,.profile=RV_HOST_PUBLIC_4M,.fd=-1},.material_fd=-1};
 char directory[]="/tmp/opencode/rtsp-private-handoff-publicdummy-XXXXXX";
 int directory_fd=-1;struct stat st,named;
 CHECK(getuid()!=0&&getuid()==geteuid()&&getgid()==getegid());
 CHECK(camera_rtsp_registry_init(&routes,"rtsp://127.0.0.1/fixture/live",
                                  "rtsp://127.0.0.1/fixture/live/track0")==CAMERA_OK);
 CHECK(mkdtemp(directory)!=NULL);
 directory_fd=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 CHECK(directory_fd>=0);
 int result=rtsp_private_handoff_create(&h,directory_fd,&cfg,&routes,
  (struct camera_digest_span){username,sizeof(username)-1},
  (struct camera_digest_span){realm,sizeof(realm)-1},
  (struct camera_digest_span){password,sizeof(password)-1},&scratch);
 mbedtls_platform_zeroize(password,sizeof(password));
 CHECK(result==CAMERA_OK&&h.phase==RTSP_PH_READY);
 CHECK(h.created&&h.identity_valid&&h.bytes_written==1536&&h.file_synced&&h.writer_close_known);
 CHECK(h.writer_fd==-1&&h.readonly_fd>=0&&cfg.material_fd==-1);
 for(size_t i=0;i<sizeof(scratch);i++)CHECK(((const unsigned char *)&scratch)[i]==0);
 CHECK(fstat(h.readonly_fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink==1&&
       st.st_uid==getuid()&&(st.st_mode&07777)==0600&&st.st_size==1536);
 CHECK(st.st_dev==h.file_inode.st_dev&&st.st_ino==h.file_inode.st_ino);
 CHECK((fcntl(h.readonly_fd,F_GETFL)&O_ACCMODE)==O_RDONLY);
 CHECK((fcntl(h.readonly_fd,F_GETFD)&FD_CLOEXEC)!=0&&lseek(h.readonly_fd,0,SEEK_CUR)==0);
 CHECK(rtsp_private_handoff_move(&h,&cfg,&owner)==CAMERA_OK);
 CHECK(h.phase==RTSP_PH_MOVED&&h.readonly_fd==-1&&cfg.material_fd>=0);
 CHECK(owner.phase==CANDIDATE_PS_EMPTY&&!owner.self&&!owner.config&&!owner.backend.child);
 CHECK(fstat(cfg.material_fd,&st)==0&&S_ISREG(st.st_mode)&&st.st_nlink==1&&
       st.st_uid==getuid()&&(st.st_mode&07777)==0600&&st.st_size==1536&&
       st.st_dev==h.file_inode.st_dev&&st.st_ino==h.file_inode.st_ino&&
       (fcntl(cfg.material_fd,F_GETFL)&O_ACCMODE)==O_RDONLY&&
       (fcntl(cfg.material_fd,F_GETFD)&FD_CLOEXEC)&&lseek(cfg.material_fd,0,SEEK_CUR)==0);
 /* Explicit reviewed TEST cleanup, after successful normal-path assertions ONLY.
  * Clear each owned slot BEFORE one close; never close-retry or broad deletion. */
 {int fd=cfg.material_fd;cfg.material_fd=-1;CHECK(close(fd)==0);}
 CHECK(fstatat(directory_fd,RTSP_PRIVATE_HANDOFF_FILE,&named,AT_SYMLINK_NOFOLLOW)==0&&
       named.st_dev==st.st_dev&&named.st_ino==st.st_ino&&named.st_nlink==1);
 CHECK(unlinkat(directory_fd,RTSP_PRIVATE_HANDOFF_FILE,0)==0);
 {int fd=directory_fd;directory_fd=-1;CHECK(close(fd)==0);}
 CHECK(rmdir(directory)==0);
 puts("PUBLICDUMMY normal_fixture=PASS actual_constructor=1 bytes_written=1536 writer_close_known=1 readonly_metadata=PASS explicit_move=PASS scratch_zero=1 cleanup=PASS native_or_media_launch=0 payload_readback_or_dump=0");
 return 0;
failed:
 mbedtls_platform_zeroize(password,sizeof(password));
 qc07f_rtsp_startup_material_clear_v1(&scratch);
 fprintf(stderr,"PUBLICDUMMY normal_fixture=FAIL line=%u checks=%u phase=%u created=%u no_failure_cleanup=1\n",
         failed_line,checks,(unsigned)h.phase,h.created);
 return 1;
}
