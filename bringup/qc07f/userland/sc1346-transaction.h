/* SPDX-License-Identifier: GPL-2.0-only; standby-only exposure transaction. */
#ifndef QC07F_SC1346_TRANSACTION_H
#define QC07F_SC1346_TRANSACTION_H
#include "sc1346-controls.h"
#include <string.h>
struct sc1346_control_bus {
 void *context;
 int (*read)(void *context,uint16_t reg);
 int (*write)(void *context,uint16_t reg,uint8_t value);
};
struct sc1346_exposure_result {
 uint32_t before_code,requested_code,after_code,vts;
 uint8_t before[3],after[3];
 int writes_started,rollback_attempted,rollback_verified;
};
enum sc1346_control_result {SC1346_CONTROL_OK=0,SC1346_CONTROL_INVALID=-1,
 SC1346_CONTROL_READ_FAILED=-2,SC1346_CONTROL_APPLY_FAILED=-3,
 SC1346_CONTROL_ROLLBACK_FAILED=-4};
static inline int sc1346_read_exposure(const struct sc1346_control_bus *b,uint8_t out[3])
{
 for(unsigned i=0;i<3;i++){int v=b->read(b->context,(uint16_t)(0x3e00+i));if(v<0||v>255)return -1;out[i]=(uint8_t)v;}
 return 0;
}
static inline int sc1346_exposure_transaction(const struct sc1346_control_bus *b,uint32_t code,struct sc1346_exposure_result *result)
{
 const uint16_t watched[]={0x0100,0x320c,0x320d,0x320e,0x320f,0x3221,0x3e03,0x3e06,0x3e07,0x3e08,0x3e09};
 uint8_t values[sizeof(watched)/sizeof(watched[0])],wanted[3];
 struct sc1346_exposure_result r={.requested_code=code};int failed=0;
 if(!b||!b->read||!b->write||!result)return SC1346_CONTROL_INVALID;
 for(unsigned i=0;i<sizeof(values);i++){int v=b->read(b->context,watched[i]);if(v<0||v>255)return SC1346_CONTROL_READ_FAILED;values[i]=(uint8_t)v;}
 r.vts=((uint32_t)values[3]<<8)|values[4];
 if(values[0]!=0||(((uint32_t)values[1]<<8)|values[2])!=SC1346_RETAIL_HTS||r.vts!=SC1346_RETAIL_VTS)return SC1346_CONTROL_INVALID;
 if(sc1346_read_exposure(b,r.before))return SC1346_CONTROL_READ_FAILED;
 r.before_code=sc1346_exposure_code(r.before);
 if(sc1346_exposure_pack(code,r.vts,r.before,wanted))return SC1346_CONTROL_INVALID;
 r.writes_started=1;
 for(unsigned i=0;i<3;i++)if(b->write(b->context,(uint16_t)(0x3e00+i),wanted[i])){failed=1;break;}
 if(!failed&&(sc1346_read_exposure(b,r.after)||memcmp(wanted,r.after,3)))failed=1;
 if(!failed)for(unsigned i=0;i<sizeof(values);i++)if(b->read(b->context,watched[i])!=values[i]){failed=1;break;}
 if(!failed){r.after_code=sc1346_exposure_code(r.after);*result=r;return SC1346_CONTROL_OK;}
 /* Failed writes may have reached hardware. Restore ALL original fields and
  * verify exposure plus every untouched watch before reporting rollback.
  */
 r.rollback_attempted=1;int rollback_failed=0;
 for(unsigned i=0;i<3;i++)if(b->write(b->context,(uint16_t)(0x3e00+i),r.before[i]))rollback_failed=1;
 if(sc1346_read_exposure(b,r.after)||memcmp(r.before,r.after,3))rollback_failed=1;
 for(unsigned i=0;i<sizeof(values);i++)if(b->read(b->context,watched[i])!=values[i])rollback_failed=1;
 r.after_code=sc1346_exposure_code(r.after);r.rollback_verified=!rollback_failed;*result=r;
 return rollback_failed?SC1346_CONTROL_ROLLBACK_FAILED:SC1346_CONTROL_APPLY_FAILED;
}
#endif
