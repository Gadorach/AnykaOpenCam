/* SPDX-License-Identifier: GPL-2.0-only */
#define _GNU_SOURCE
#include "entry.h"
#include "wait_receipt.h"
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
/*No password/username/HA1 reader and no positive VERIFY in FIRST entry scope.*/
#if defined(NQ_HOST_ENTRY_HARDFAULT_TEST) && !defined(NQ_HOST_PUBLIC_ONLY)
#error "Hardfault hook is HOST PUBLICDUMMY only"
#endif
static struct rv_client *volatile retained_owner;
int nq_entry(const struct rv_image *image,int material_fd)
{
 struct rv_client *child=NULL;struct rv_message message={0};uint64_t connection=0;int result=1;
 unsigned before=nq_owned_exits(),before_zero=nq_owned_exit_zero();
 int start=rv_start(&child,image,material_fd);
 if(start!=RV_OK){puts("entry_start=denied functional_RTSP_verify_native_NOTtested=1");goto retire;}
 if(rv_ready_flags(child)!=RV_FLAGS15||rv_ready_profile(child)!=image->profile)goto retire;
 puts("entry_ready=1 flags=15 provision_ACK=1 functional_RTSP_verify_native_NOTtested=1");
#ifdef NQ_HOST_ENTRY_HARDFAULT_TEST
 /*Separate PUBLICDUMMY HOST failure binary; no native default or wipe/exit0 claim.*/
 if(rv_owned_signal(child,SIGKILL)!=RV_OK)goto retire;
 goto retire;
#endif
 message.kind=RV_OPEN;if(rv_rpc(child,&message)!=RV_OK||!message.connection)goto retire;connection=message.connection;
 rv_wipe(&message,sizeof(message));message.kind=RV_CHALLENGE;message.connection=connection;message.bytes=8;rv_put32(message.payload,1);rv_put32(message.payload+4,0);
 if(rv_rpc(child,&message)!=RV_OK||!message.bytes||message.bytes>512)goto retire;
 /*Nonce-bearing response is not inspected, forwarded, printed or hashed.*/
 rv_wipe(&message,sizeof(message));puts("entry_OPEN_CHALLENGE=passed challenge_content_discarded=1");
  /*Reviewed engine returns RV_BAD for a mismatched handle (RV_DENIED is for
   * a matching but revoked/expired grant). Never accept RV_LOST/UNCERTAIN.*/
  if(rv_query(child,connection,UINT64_C(0x7fffffffffffffff))!=RV_BAD)goto retire;
 puts("entry_forged_query=denied positive_crypto_NOTtested=1");
 if(rv_owner_revoke(child)!=RV_OK)goto retire;
 puts("entry_fixed_owner_REVOKE=ACK generation_validated=1");result=0;
 retire:
 rv_wipe(&message,sizeof(message));
 if(child){
  if(rv_stop(child)!=RV_OK||!rv_retired(child)){retained_owner=child;puts("entry_retirement=uncertain context_retained=1 no_successful_STOP_claim=1");return 2;}
  if(result==0&&(!rv_wipe_observed(child)||nq_owned_exits()!=before+1||nq_owned_exit_zero()!=before_zero+1))result=1;
  if(rv_destroy(&child)!=RV_OK||child){retained_owner=child;puts("entry_retirement=uncertain context_retained=1");return 2;}
 }
 int status;errno=0;
 if(waitpid(-1,&status,WNOHANG)!=-1||errno!=ECHILD)result=1;
 if(!result)puts("entry_owned_SIGTERM_BYE_wipe_wait0=observed no_lineage=1 functional_RTSP_verify_native_NOTtested=1");
 else puts("entry_result=HOLD observed_or_no_child=1 functional_RTSP_verify_native_NOTtested=1");
 return result;
}
