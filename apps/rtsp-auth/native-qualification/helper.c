/* SPDX-License-Identifier: GPL-2.0-only
 * Root-only finite first-entry qualifier source. No watchdog, target listener,
 * driver, producer/media, private-password reader or positive native VERIFY. */
#define _GNU_SOURCE
#include "source_abi.h"
#include "paths.h"
#include "entry.h"
#include <camera/access_policy.h>
#include <errno.h>
#include <grp.h>
#include <linux/capability.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#ifdef NQ_HOST_PUBLIC_ONLY
#include "host_image_pin.h"
#else
#include "native_image_pin.h"
#endif
static volatile sig_atomic_t stopping;
static void stop(int signal){(void)signal;stopping=1;}
static int caps(unsigned mask)
{
 struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};struct __user_cap_data_struct c[2]={{0}};
 c[0].effective=c[0].permitted=mask;
 return syscall(SYS_capset,&h,c)||syscall(SYS_capget,&h,c)||c[0].effective!=mask||c[0].permitted!=mask||c[0].inheritable||c[1].effective||c[1].permitted||c[1].inheritable;
}
int main(int argc,char **argv)
{
 int private_dir=-1,code_dir=-1,image=-1,material=-1,result=1;uid_t owner=getuid();struct nq_public_prefix prefix={0};
 struct rlimit core={0,0};struct sigaction action={0};action.sa_handler=stop;sigemptyset(&action.sa_mask);
 if(argc!=3||getuid()!=geteuid()||getgid()!=getegid())return 20;
#ifdef NQ_HOST_PUBLIC_ONLY
 if(!owner){puts("entry_class=HOSTPUBLICDUMMY_root_forbidden");return 20;}
#else
 if(owner!=0||getgid()!=0){puts("entry_class=NATIVE_ROOT_REQUIRED HOLD=1 functional_RTSP_verify_native_NOTtested=1");return 20;}
 /*Only the already-root helper's OWN startup groups; never new privilege/user.*/
 if(setgroups(0,NULL)||getgroups(0,NULL)!=0)return 20;
#endif
 if(setrlimit(RLIMIT_CORE,&core)||prctl(PR_SET_DUMPABLE,0)||sigaction(SIGTERM,&action,NULL)||sigaction(SIGINT,&action,NULL))return 20;
 {struct sigaction normal={0};normal.sa_handler=SIG_DFL;sigemptyset(&normal.sa_mask);if(sigaction(SIGCHLD,&normal,NULL))return 20;}
 signal(SIGPIPE,SIG_IGN);
#ifdef NQ_HOST_PUBLIC_ONLY
 if(caps(0))return 20;
 puts("entry_class=HOSTPUBLICDUMMY_ONLY root_origin_native2M_NOTproved=1");
#else
 if(caps((1U<<CAP_SETUID)|(1U<<CAP_SETGID)|(1U<<CAP_KILL)))return 20; /*no SYS_RESOURCE/IPC_LOCK, no cap acquisition*/
 puts("entry_class=NATIVE_ROOT_ONLY child_memlock_bytes=2097152 first_entry_only=1");
#endif
 private_dir=nq_open_dir(argv[1],owner,0700);code_dir=nq_open_dir(argv[2],owner,0555);if(private_dir<0||code_dir<0)goto done;
 material=nq_open_file(private_dir,"rtsp-material.bin",owner,0600,NQ_MATERIAL_BYTES);image=nq_open_file(code_dir,"verifier",owner,0555,NQ_IMAGE_BYTES);
 if(material<0||image<0||pread(material,&prefix,sizeof(prefix),0)!=(ssize_t)sizeof(prefix)||prefix.abi!=RV_ABI||prefix.bytes!=NQ_MATERIAL_BYTES||
    prefix.domain!=CAMERA_ACCESS_RTSP||(prefix.algorithm!=1&&prefix.algorithm!=2)||!prefix.epoch||!prefix.boot_generation||!prefix.credential_generation)goto done;
#ifdef NQ_HOST_PUBLIC_ONLY
 if(prefix.account!=NQ_DUMMY_ACCOUNT)goto done;
#else
 if(prefix.account!=NQ_PRIVATE_ACCOUNT)goto done; /*reject the SOURCE-public DUMMY partition WITHOUT reading username/HA1*/
#endif
 struct rv_image pinned={.abi=RV_ABI,.profile=NQ_IMAGE_PROFILE,.fd=image,.bytes=NQ_IMAGE_BYTES};memcpy(pinned.sha256,nq_image_sha256,32);
 if(stopping)goto done;
 result=nq_entry(&pinned,material);
 if(result==2){
  /*Canonical UNKNOWN-retirement owner policy: do not free/return/claim wipe.
   * Normal run is finite; this explicit failure exception retains the process
   * and client namespace for parent recovery under its EXISTING ≤45s protection.
   * No watchdog open/renew/reset or automatic respawn/retry is implemented.*/
  puts("entry_HOLD=uncertain_owner_context_retained parent_external_recovery_required=1");fflush(stdout);
  for(;;)poll(NULL,0,100); /*external parent is sole recovery owner, not a daemon*/
 }
 done:
 rv_wipe(&prefix,sizeof(prefix));if(image>=0)close(image);if(material>=0)close(material);if(code_dir>=0)close(code_dir);if(private_dir>=0)close(private_dir);
 puts("functional_RTSP_verify_native_NOTtested=1 native_media_per_effect_IPC_NOTaccepted=1");
 return result?20:0;
}
