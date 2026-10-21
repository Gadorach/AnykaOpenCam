/* SPDX-License-Identifier: GPL-2.0-only */
#include "host-core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct fixture {
 struct qc_engine *engine;
 unsigned int ticks, samples, writes, claims, releases, retirements, copies;
 unsigned int freeze, cancel_at, command_status, data_status, claimed;
 unsigned int fail_claim, fail_prepare, fail_finish, fail_retire, fail_copy;
 unsigned int busy, transferred, count, command_started, data_started;
 unsigned int cancel_on_retire, delay_on_retire, response0;
 unsigned int inject_old_irq;
 qc_u32 start;
};
static int claim(void *p) {
 struct fixture *f = p;
 f->claims++;
 if (f->fail_claim) return -1;
 assert(!f->claimed); f->claimed = 1; return 0;
}
static void release(void *p) {
 struct fixture *f = p;
 assert(f->claimed && f->retirements && !f->fail_retire);
 f->claimed = 0; f->releases++;
}
static void write_reg(void *p, qc_u32 reg, qc_u32 value) {
 struct fixture *f = p;
 assert(f->claimed); f->writes++;
 if (reg == QC_CMD && value) f->command_started = 1;
 if (reg == QC_DATA && value) {
  assert((value & 3) == 3); /* direction never write */
  assert(!(value & (3u << 3))); /* never 4/8bit */
  assert((value >> 16) <= 512); f->data_started = 1;
 }
}
static qc_u32 read_reg(void *p, qc_u32 reg) {
 struct fixture *f = p;
 if (reg == QC_COUNT) return f->count;
 return reg == QC_RESP0 ? f->response0 : 0;
}
static qc_u32 sample(void *p, qc_u32 generation) {
 struct fixture *f = p;
 assert(generation == f->engine->generation);
 if (f->inject_old_irq) {
  qc_u32 events = f->engine->events;
  assert(!qc_irq_event(f->engine, 1, QC_RESPONSE_END|QC_DATA_END));
  assert(f->engine->events == events);
 }
 f->samples++;
 return f->command_started ? f->command_status | f->data_status : 0;
}
static qc_u32 now(void *p) {
 struct fixture *f = p; return f->start + f->ticks;
}
static int cancelled(void *p) {
 struct fixture *f = p; return f->cancel_at && f->samples >= f->cancel_at;
}
static void relax(void *p) {
 struct fixture *f = p; if (!f->freeze) f->ticks++;
}
static int prepare(void *p, qc_u32 bytes) {
 struct fixture *f = p; assert(bytes > 0 && bytes <= 512);
 return f->fail_prepare ? -1 : 0;
}
static int finish(void *p, qc_u32 *bytes) {
 struct fixture *f = p; *bytes = f->transferred;
 return f->fail_finish ? -1 : 0;
}
static int busy(void *p) { return ((struct fixture *)p)->busy; }
static int retire(void *p, qc_u32 generation) {
 struct fixture *f = p;
 assert(!f->engine->active && f->claimed && generation == f->engine->generation);
 /* A late IRQ cannot resurrect any retired request. */
 assert(!qc_irq_event(f->engine, generation, QC_RESPONSE_END));
 f->retirements++;
 if (f->cancel_on_retire) f->cancel_at=1;
 if (f->delay_on_retire) f->ticks += QC_DEADLINE_MS;
 return !f->fail_retire;
}
static int publish(void *p, qc_u32 bytes) {
 struct fixture *f = p;
 assert(f->retirements && !f->fail_retire && bytes == f->transferred);
 f->copies++; return f->fail_copy ? -1 : 0;
}
static struct qc_io io(struct fixture *f) {
 return (struct qc_io){f, claim, release, write_reg, read_reg, sample, now,
  cancelled, relax, prepare, finish, busy, retire, publish};
}
static void reset(struct fixture *f, struct qc_engine *e) {
 memset(e, 0, sizeof(*e)); memset(f, 0, sizeof(*f)); f->engine = e;
 f->command_status = QC_RESPONSE_END; f->data_status = QC_DATA_END;
 f->transferred = 512;
 f->response0 = 1u << 5;
}
static struct qc_request read_request(void) {
 return (struct qc_request){.opcode=17, .response_flags=QC_RSP_PRESENT|QC_RSP_CRC,
  .data_flags=QC_READ, .bytes=512, .blocks=1};
}
static unsigned int checks;
#define CHECK(x) do { assert(x); checks++; } while (0)

static void generation_tests(void)
{
 struct qc_engine e, olde;
 struct fixture f, oldf;
 struct qc_request r;
 struct qc_io ops;
 unsigned int mode, queued = 0;
 const qc_u32 max = UINT32_MAX;
 CHECK(!qc_irq_event(NULL, 1, QC_RESPONSE_END|QC_DATA_END));
 CHECK(qc_request_ready(NULL) == QC_DENIED);
 /* Exhaustion is a pure refusal: every provider counter/state is unchanged,
  * no next command context consumed, no new generation or queued work. */
 for (mode = 0; mode < 4; mode++) {
  reset(&f,&e); ops=io(&f); e.generation=max; e.app_pending=1;
  r=(struct qc_request){.opcode=41,.arg=0x40300000,
                       .response_flags=QC_RSP_PRESENT};
  if (mode==1) { r=read_request(); e.app_pending=0; }
  if (mode==2) { r.opcode=24; r.data_flags=QC_WRITE; }
  if (mode==3) ops.claim=NULL; /* fence precedes even callback validation */
  olde=e; oldf=f;
  if (qc_request_ready(&e)==QC_OK) queued++; /* same glue scheduling gate */
  CHECK(qc_request_ready(&e)==QC_EXHAUSTED);
  CHECK(qc_run(&e,&r,&ops)==QC_EXHAUSTED);
  CHECK(!memcmp(&e,&olde,sizeof(e)) && !memcmp(&f,&oldf,sizeof(f)));
  CHECK(!queued && !f.claims && !f.writes && !f.samples && !f.copies &&
        !f.releases && !f.retirements && !f.command_started && !f.data_started);
  CHECK(!qc_irq_event(&e,1,QC_RESPONSE_END|QC_DATA_END) && !e.events);
 }
 /* Near-max commands legitimately issue max-1 and then max, retire exactly
  * once each; the NEXT attempt is refused without any provider activity. */
 reset(&f,&e); ops=io(&f); e.generation=max-2; f.inject_old_irq=1;
 r=(struct qc_request){.opcode=55,.response_flags=QC_RSP_PRESENT};
 CHECK(qc_run(&e,&r,&ops)==QC_OK && e.generation==max-1 && e.app_pending);
 r=(struct qc_request){.opcode=41,.arg=0x40300000,
                      .response_flags=QC_RSP_PRESENT};
 CHECK(qc_run(&e,&r,&ops)==QC_OK && e.generation==max && !e.app_pending);
 CHECK(f.claims==2 && f.retirements==2 && f.releases==2 && !e.active);
 olde=e; oldf=f; r=read_request();
 CHECK(qc_run(&e,&r,&ops)==QC_EXHAUSTED && !memcmp(&e,&olde,sizeof(e)) &&
       !memcmp(&f,&oldf,sizeof(f)));
 /* Stale generation1 events alone must not complete the final valid read. */
 reset(&f,&e); ops=io(&f); e.generation=max-1; f.inject_old_irq=1;
 f.command_status=0; f.data_status=0; r=read_request();
 CHECK(qc_run(&e,&r,&ops)==QC_TIMEOUT && e.generation==max && !f.copies);
 CHECK(f.claims==1 && f.retirements==1 && f.releases==1);
 /* The final valid read still publishes normally with its own events. */
 reset(&f,&e); ops=io(&f); e.generation=max-1; f.inject_old_irq=1;
 r=read_request();
 CHECK(qc_run(&e,&r,&ops)==QC_OK && e.generation==max && f.copies==1);
 /* A colliding new request must not retire/cancel an existing final active
  * operation, nor reject its valid IRQ just because its namespace is max. */
 reset(&f,&e); ops=io(&f); e.generation=max; e.active=1; f.claimed=1;
 e.events=QC_COMMAND_SENT; r=read_request(); olde=e; oldf=f;
 CHECK(qc_request_ready(&e)==QC_DENIED && qc_run(&e,&r,&ops)==QC_DENIED);
 CHECK(!memcmp(&e,&olde,sizeof(e)) && !memcmp(&f,&oldf,sizeof(f)));
 CHECK(!qc_irq_event(&e,1,QC_RESPONSE_END) && e.events==QC_COMMAND_SENT);
 CHECK(qc_irq_event(&e,max,QC_RESPONSE_END) && e.active && !f.retirements);
 /* Preserve poison precedence and failed final-generation retirement: no
  * namespace-exhaustion retry may release the held provider mapping/lease. */
 reset(&f,&e); ops=io(&f); e.generation=max-1; f.fail_retire=1; r=read_request();
 CHECK(qc_run(&e,&r,&ops)==QC_QUARANTINED && e.generation==max && e.poisoned);
 CHECK(f.claimed && !f.releases && !f.copies && f.retirements==1);
 olde=e; oldf=f;
 CHECK(qc_request_ready(&e)==QC_QUARANTINED && qc_run(&e,&r,&ops)==QC_QUARANTINED);
 CHECK(!memcmp(&e,&olde,sizeof(e)) && !memcmp(&f,&oldf,sizeof(f)));
 CHECK(!qc_irq_event(&e,1,QC_DATA_END) && !qc_irq_event(&e,max,QC_DATA_END));
 /* A failed claim at max-1 doesn't consume the final namespace. */
 reset(&f,&e); ops=io(&f); e.generation=max-1; f.fail_claim=1; r=read_request();
 CHECK(qc_run(&e,&r,&ops)==QC_IO && e.generation==max-1 && !f.writes);
 f.fail_claim=0;
 CHECK(qc_run(&e,&r,&ops)==QC_OK && e.generation==max);
}

int main(void)
{
 struct qc_engine e;
 struct fixture f;
 struct qc_request r;
 struct qc_io ops;
 qc_u32 bits, actual, parent, requested;
 unsigned int op, flags, app, len, mode;
 /* Exhaustive command/descriptor denial: no admission may admit a write. */
 for (app=0; app<2; app++) for (op=0; op<256; op++)
  for (flags=0; flags<8; flags++) for (len=0; len<514; len++) {
   r = (struct qc_request){.opcode=op, .arg=0, .data_flags=flags,
    .bytes=len, .blocks=!!len};
   if (qc_admit(&r, app) == QC_OK) {
    CHECK(!(flags & QC_WRITE)); CHECK(len <= 512);
    CHECK(op != 24 && op != 25 && op != 32 && op != 33 && op != 38 && op != 42);
   }
  }
 for (parent=24000000; parent<=192000000; parent+=24000000)
  for (requested=1; requested<=400000; requested+=997) {
   int result = qc_clock_div(parent,requested,&bits,&actual);
   if (!result) { CHECK(actual <= requested); CHECK((bits & 0xffffu) <= 0xffffu); }
  }
 CHECK(qc_clock_div(120000000,400001,&bits,&actual) == QC_DENIED);
 CHECK(qc_clock_div(120000000,0,&bits,&actual) == QC_OK && !bits && !actual);
 CHECK(qc_clock_div(0,400000,&bits,&actual) == QC_DENIED);
 for (mode=0; mode<17; mode++) {
  reset(&f,&e); ops=io(&f); r=read_request();
  switch (mode) {
  case 1: f.command_status=QC_CMD_TIMEOUT; break;
  case 2: f.command_status=QC_CMD_CRC; break;
  case 3: f.data_status=QC_DATA_TIMEOUT; break;
  case 4: f.data_status=QC_DATA_CRC; break;
  case 5: f.data_status=QC_STARTBIT; break;
  case 6: f.fail_prepare=1; break;
  case 7: f.fail_finish=1; break;
  case 8: f.count=1; break;
  case 9: f.transferred=511; break;
  case 10: f.cancel_at=1; break;
  case 11: f.fail_retire=1; break;
  case 12: f.fail_copy=1; break;
  case 13: f.command_status=0; f.start=0xfffffff0u; break;
  case 14: f.command_status=0; f.freeze=1; break;
  case 15: f.cancel_on_retire=1; break;
  case 16: f.delay_on_retire=1; break;
  }
  int result=qc_run(&e,&r,&ops);
  CHECK((mode==0) == (result==QC_OK));
  CHECK(f.retirements == 1);
  CHECK(f.samples <= QC_MAX_SAMPLES+2);
  CHECK(!e.active);
  if (mode==11) {
   CHECK(result==QC_QUARANTINED && e.poisoned && !f.releases && !f.copies);
   unsigned int oldwrites=f.writes;
   CHECK(qc_run(&e,&r,&ops)==QC_QUARANTINED && oldwrites==f.writes);
  } else { CHECK(f.releases==1 && !f.claimed); }
  if (mode && mode!=12) CHECK(!f.copies);
 }
 reset(&f,&e); ops=io(&f); r=read_request(); f.fail_claim=1;
 CHECK(qc_run(&e,&r,&ops)==QC_IO && !f.writes && !f.retirements);
 reset(&f,&e); ops=io(&f); r=read_request(); r.opcode=24; r.data_flags=QC_WRITE;
 CHECK(qc_run(&e,&r,&ops)==QC_DENIED && !f.claims && !f.writes);
 reset(&f,&e); ops=io(&f);
 r=(struct qc_request){.opcode=55,.response_flags=QC_RSP_PRESENT};
 CHECK(qc_run(&e,&r,&ops)==QC_OK && e.app_pending);
 r=(struct qc_request){.opcode=41,.arg=0x40300000,.response_flags=QC_RSP_PRESENT};
 f.command_status=QC_CMD_CRC; /* R3 no CRC */
 CHECK(qc_run(&e,&r,&ops)==QC_OK && !e.app_pending);
 CHECK(qc_run(&e,&r,&ops)==QC_DENIED);
 e.app_pending=1; r.arg=0x41300000;
 CHECK(qc_run(&e,&r,&ops)==QC_DENIED && !e.app_pending);
 reset(&f,&e); ops=io(&f);
 r=(struct qc_request){.opcode=55,.response_flags=QC_RSP_PRESENT};
 f.response0=(1u<<5)|(1u<<22);
 CHECK(qc_run(&e,&r,&ops)==QC_OK && !e.app_pending);
 r=(struct qc_request){.opcode=6,.arg=0x00fffff0,.response_flags=QC_RSP_PRESENT,
                      .data_flags=QC_READ,.blocks=1,.bytes=64};
 f.transferred=64;
 CHECK(qc_run(&e,&r,&ops)==QC_OK);
 r.arg |= 0x80000000u;
 CHECK(qc_run(&e,&r,&ops)==QC_DENIED);
 r=(struct qc_request){.opcode=0}; f.command_status=QC_COMMAND_SENT;
 CHECK(qc_run(&e,&r,&ops)==QC_OK);
 reset(&f,&e); ops=io(&f);
 r=(struct qc_request){.opcode=7,.response_flags=QC_RSP_PRESENT|QC_RSP_BUSY};
 f.busy=1;
 CHECK(qc_run(&e,&r,&ops)==QC_TIMEOUT && f.retirements==1);
 e.active=1; e.generation=7; e.events=0;
 CHECK(!qc_irq_event(&e,6,QC_RESPONSE_END) && !e.events);
 CHECK(qc_irq_event(&e,7,QC_RESPONSE_END) && e.events==QC_RESPONSE_END);
 e.active=0; CHECK(!qc_irq_event(&e,7,QC_DATA_END));
 r=read_request(); CHECK(qc_command_word(&r)==(1u|(17u<<1)|(1u<<7)|(1u<<11)));
 generation_tests();
 printf("PASS %u checks; 2105344 descriptor cases; 17 lifecycle modes plus APP/busy/IRQ/clock and nonwrapping-generation tests\n",checks);
 return 0;
}
