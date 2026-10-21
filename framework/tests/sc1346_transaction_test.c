/* SPDX-License-Identifier: GPL-2.0-only; simulated registers, no hardware. */
#include "../../bringup/qc07f/userland/sc1346-transaction.h"
#include <assert.h>
#include <stdio.h>
struct mock {uint8_t registers[65536];unsigned writes;int fail_write,fail_reads,change_watch;};
static int read_reg(void *context,uint16_t reg)
{
 struct mock *m=context;
 if(m->fail_reads&&m->writes==3&&reg==0x3e00){m->fail_reads=0;return -1;}
 return m->registers[reg];
}
static int write_reg(void *context,uint16_t reg,uint8_t value)
{
 struct mock *m=context;m->writes++;m->registers[reg]=value;
 if(m->change_watch&&m->writes==3)m->registers[0x3e07]++;
 return m->fail_write==(int)m->writes?-1:0; /* side effect BEFORE error */
}
static void init(struct mock *m)
{
 memset(m,0,sizeof(*m));m->registers[0x320c]=7;m->registers[0x320d]=8;
 m->registers[0x320e]=2;m->registers[0x320f]=0xee;
 m->registers[0x3e00]=0xa0;m->registers[0x3e01]=0x2e;m->registers[0x3e02]=0x8b;m->registers[0x3e07]=0x80;
}
int main(void)
{
 struct mock m;struct sc1346_control_bus b={.context=&m,.read=read_reg,.write=write_reg};struct sc1346_exposure_result r;
 init(&m);assert(sc1346_exposure_transaction(&b,186,&r)==SC1346_CONTROL_OK);
 assert(r.before_code==744&&r.after_code==186&&m.writes==3);
 assert(m.registers[0x3e00]==0xa0&&m.registers[0x3e02]==0xab&&m.registers[0x3e07]==0x80);
 init(&m);m.registers[0x0100]=1;assert(sc1346_exposure_transaction(&b,186,&r)==SC1346_CONTROL_INVALID&&!m.writes);
 init(&m);assert(sc1346_exposure_transaction(&b,745,&r)==SC1346_CONTROL_INVALID&&!m.writes);
 for(int fail=1;fail<=3;fail++){
  init(&m);m.fail_write=fail;assert(sc1346_exposure_transaction(&b,186,&r)==SC1346_CONTROL_APPLY_FAILED);
  assert(r.rollback_attempted&&r.rollback_verified&&r.after_code==744&&m.registers[0x3e00]==0xa0&&m.registers[0x3e01]==0x2e&&m.registers[0x3e02]==0x8b);
 }
 init(&m);m.fail_reads=1;assert(sc1346_exposure_transaction(&b,186,&r)==SC1346_CONTROL_APPLY_FAILED&&r.rollback_verified&&r.after_code==744);
 init(&m);m.change_watch=1;assert(sc1346_exposure_transaction(&b,186,&r)==SC1346_CONTROL_ROLLBACK_FAILED&&!r.rollback_verified);
 assert(sc1346_exposure_transaction(NULL,186,&r)==SC1346_CONTROL_INVALID);
 puts("SC1346 standby/range/readback/partial-side-effect rollback/unrelated-state refusal fixtures PASS");return 0;
}
