/* SPDX-License-Identifier: GPL-2.0-only
 * Owned 127.0.0.1 mock sockets + finite memory tests; no device/real credentials.
 * Reuses frozen a24 lifecycle positives; output has scalar codes/counts only. */
#define CAMERA_RTSP_TEST_ENTRY saved_lifecycle_tests
#include "rtsp_producer_lifecycle_test.c"
static unsigned integration_cases;
static void zero_tail(const struct camera_rtsp_input *input)
{for(size_t i=input->used;i<sizeof(input->data);i++)assert(input->data[i]==0);}
static void one_negative(const char *wire,enum camera_rtsp_input_failure expected)
{
 struct fixture f;struct rtsp_client c;fixture_init(&f);int writer=peer(&f,&c);uint32_t force=0;
 assert(!camera_rtsp_input_append(&c.input,wire,strlen(wire),milliseconds()));
 assert(process_input(&c,0,&f.hub,&test_options,&force)==-1);
 assert(c.input.failure==expected&&!f.producer.demand_count&&!f.detaches&&!f.starts&&!f.produced&&!force);
 close_client(&c,&f.hub,0);zero_tail(&c.input);close(writer);integration_cases++;
}
static void policy_negatives(void)
{
 const char *prefix="SETUP rtsp://127.0.0.1/live/track1 RTSP/1.0\r\nCSeq: 1\r\nTransport: ";
 static const char *const transports[]={
  "RTP/AVP;unicast;client_port=0-1","RTP/AVP/TCP;unicast;interleaved=2-3",
  "RTP/AVP/TCP;unicast;interleaved=0-1x","X-RTP/AVP/TCP;unicast;interleaved=0-1",
  "RTP/AVP/TCP;unicast;interleaved=0-1;interleaved=0-1",
  "RTP/AVP/TCP;unicast;unicast;interleaved=0-1",
  "RTP/AVP/TCP;unicast;interleaved=0-1, RTP/AVP/TCP;unicast;interleaved=0-1",
  "RTP/AVP/TCP;multicast;interleaved=0-1","RTP/AVP/TCP;interleaved=0-1",
  "RTP/AVP/TCP;unicast;interleaved=0-1;mode=\"PLAY\""};
 char wire[2048];
 for(size_t i=0;i<sizeof(transports)/sizeof(transports[0]);i++){
  int n=snprintf(wire,sizeof(wire),"%s%s\r\n\r\n",prefix,transports[i]);assert(n>0&&(size_t)n<sizeof(wire));
  one_negative(wire,CAMERA_RTSP_INPUT_TRANSPORT);
 }
 one_negative("PLAY rtsp://127.0.0.1/live RTSP/1.0\r\nCSeq: 1\r\nSession: CAM1\r\n\r\n",CAMERA_RTSP_INPUT_SESSION);
 one_negative("DESCRIBE rtsp://127.0.0.1/etc/passwd RTSP/1.0\r\nCSeq: 1\r\n\r\n",CAMERA_RTSP_INPUT_RESOURCE);
 one_negative("DESCRIBE rtsp://127.0.0.1/l%69ve RTSP/1.0\r\nCSeq: 1\r\n\r\n",CAMERA_RTSP_INPUT_RESOURCE);
 one_negative("DESCRIBE rtsp://127.0.0.1/live?x=1 RTSP/1.0\r\nCSeq: 1\r\n\r\n",CAMERA_RTSP_INPUT_RESOURCE);
 one_negative("OPTIONS rtsp://DUMMY-PUBLIC@127.0.0.1/live RTSP/1.0\r\nCSeq: 1\r\n\r\n",CAMERA_RTSP_INPUT_FRAMING);
 one_negative("SETUP rtsp://127.0.0.1/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\nSession: CAM1\r\n\r\n",CAMERA_RTSP_INPUT_SESSION);
 one_negative("OPTIONS * RTSP/2.0\r\nCSeq: 1\r\n\r\n",CAMERA_RTSP_INPUT_FRAMING);
 one_negative("OPTIONS * RTSP/1.0\r\nCSeq: 1\r\ncseq: 1\r\n\r\n",CAMERA_RTSP_INPUT_FRAMING);
 one_negative("OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 0\r\ncontent-length: 0\r\n\r\n",CAMERA_RTSP_INPUT_FRAMING);
 one_negative("OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nAuthorization: DUMMY-PUBLIC\r\nauthorization: DUMMY-PUBLIC\r\n\r\n",CAMERA_RTSP_INPUT_FRAMING);
 one_negative("OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nSession: CAM1\r\nsession: CAM1\r\n\r\n",CAMERA_RTSP_INPUT_FRAMING);
 one_negative("SETUP rtsp://127.0.0.1/live RTSP/1.0\r\nCSeq: 1\r\nTransport: DUMMY-PUBLIC\r\ntransport: DUMMY-PUBLIC\r\n\r\n",CAMERA_RTSP_INPUT_FRAMING);
 one_negative("OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nTransfer-Encoding: chunked\r\n\r\n",CAMERA_RTSP_INPUT_FRAMING);
 one_negative("OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 4\r\n\r\n$\r\nx",CAMERA_RTSP_INPUT_BODY);
 /* Unknown verbs cannot trigger resource or producer effects. */
 struct fixture f;struct rtsp_client c;fixture_init(&f);int writer=peer(&f,&c);uint32_t force=0;
 const char *unknown="PLAYBACK rtsp://127.0.0.1/live RTSP/1.0\r\nCSeq: 4294967295\r\n\r\n";
 assert(!camera_rtsp_input_append(&c.input,unknown,strlen(unknown),milliseconds()));
 assert(!process_input(&c,0,&f.hub,&test_options,&force)&&!c.setup&&!c.playing&&!force&&!f.producer.demand_count);
 unsigned char response[256];ssize_t n=recv(writer,response,sizeof(response),0);assert(n>0);
 assert((size_t)n>=strlen("RTSP/1.0 501 Unsupported\r\nCSeq: 4294967295\r\n"));
 assert(!memcmp(response,"RTSP/1.0 501 Unsupported\r\nCSeq: 4294967295\r\n",strlen("RTSP/1.0 501 Unsupported\r\nCSeq: 4294967295\r\n")));
 zero_tail(&c.input);close_client(&c,&f.hub,0);close(writer);integration_cases++;
}
static const unsigned char rr[]={ '$',1,0,8,0x80,201,0,1,0,0,0,1 };
static void interleaved_and_session(void)
{
 struct fixture f;struct rtsp_client c;fixture_init(&f);int writer=peer(&f,&c);uint32_t force=0;
 const char *setup="SETUP rtsp://127.0.0.1/live/track1 RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;interleaved=0-1;unicast\r\n\r\n";
 const char *play="PLAY rtsp://127.0.0.1/live RTSP/1.0\r\nCSeq: 2\r\nSession: CAM1;timeout=30\r\n\r\n";
 assert(!camera_rtsp_input_append(&c.input,setup,strlen(setup),milliseconds()));
 assert(!camera_rtsp_input_append(&c.input,rr,sizeof(rr),milliseconds()));
 assert(!camera_rtsp_input_append(&c.input,play,strlen(play),milliseconds()));
 assert(!process_input(&c,0,&f.hub,&test_options,&force));
 assert(c.setup&&c.playing&&f.producer.demand_count==1&&force==1);zero_tail(&c.input);
 struct camera_producer_demand demand=c.demand;
 assert(!camera_rtsp_input_append(&c.input,play,strlen(play),milliseconds()));
 assert(!process_input(&c,0,&f.hub,&test_options,&force));assert(c.demand.id==demand.id&&f.producer.demand_count==1);
 /* A valid compound RR+APP contains header-looking bytes; ignored, not scanned. */
 unsigned char compound[40]={ '$',1,0,36,0x80,201,0,1,0,0,0,1,0x80,204,0,6,0,0,0,1,'D','U','M','Y'};
 memcpy(compound+24,"PLAY\r\nCSeq: 9\r\n",15);
 assert(!camera_rtsp_input_append(&c.input,compound,sizeof(compound),milliseconds()));
 assert(!process_input(&c,0,&f.hub,&test_options,&force));assert(c.demand.id==demand.id);
 const char *wrong="PLAY rtsp://127.0.0.1/live RTSP/1.0\r\nCSeq: 3\r\nSession: CAM2\r\n\r\n";
 assert(!camera_rtsp_input_append(&c.input,wrong,strlen(wrong),milliseconds()));
 assert(process_input(&c,0,&f.hub,&test_options,&force)==-1&&c.input.failure==CAMERA_RTSP_INPUT_SESSION);
 close_client(&c,&f.hub,0);close_client(&c,&f.hub,0);assert(f.detaches==1&&!f.producer.demand_count&&!f.produced);
 assert(f.hooks.retire(f.hooks.context,monotonic_ns())==CAMERA_OK);zero_tail(&c.input);close(writer);integration_cases++;
 /* Missing Session after real SETUP (unchanged capture.py compatibility gap). */
 fixture_init(&f);writer=peer(&f,&c);force=0;
 assert(!command_client(&f,&c,0,writer,"SETUP",&force));
 const char *missing="PLAY rtsp://127.0.0.1/live RTSP/1.0\r\nCSeq: 2\r\n\r\n";
 assert(!camera_rtsp_input_append(&c.input,missing,strlen(missing),milliseconds()));
 assert(process_input(&c,0,&f.hub,&test_options,&force)==-1&&c.input.failure==CAMERA_RTSP_INPUT_SESSION);
 close_client(&c,&f.hub,0);close(writer);assert(!f.detaches&&!f.producer.demand_count&&!f.produced);integration_cases++;
}
static void memory_units(void)
{
 struct camera_rtsp_input input;struct camera_rtsp_request request,before;size_t consumed;
 struct camera_rtsp_registry registry;
 assert(camera_rtsp_registry_init(&registry,"rtsps://public.invalid/live","rtsps://public.invalid/live/track1")==-1);
 assert(camera_rtsp_registry_init(&registry,"rtsp://DUMMY-PUBLIC@public.invalid/live","rtsp://public.invalid/live/track1")==-1);
 assert(camera_rtsp_registry_init(&registry,"rtsp://public.invalid/live","rtsp://public.invalid/live")==-1);
 assert(!camera_rtsp_registry_init(&registry,"rtsp://public.invalid/l%69ve","rtsp://public.invalid/l%69ve/track1"));
 const char *encoded="DESCRIBE rtsp://public.invalid/l%69ve RTSP/1.0\r\nCSeq: 1\r\n\r\n";
 enum camera_rtsp_method method;
 assert(camera_rtsp_request_parse(encoded,strlen(encoded),&request)==CAMERA_RTSP_READY);
 assert(camera_rtsp_request_policy(&request,&registry,0,0,"CAM1",&method)==CAMERA_RTSP_INPUT_NONE);
 assert(request.target.length==strlen("rtsp://public.invalid/l%69ve")&&!memcmp(request.target.data,"rtsp://public.invalid/l%69ve",request.target.length));
 integration_cases++;
 for(size_t i=0;i<=sizeof(rr);i++) {
  assert(!camera_rtsp_input_init(&input,100,2000));
  assert(!camera_rtsp_input_append(&input,rr,i,100));
  memset(&request,0xa5,sizeof(request));memcpy(&before,&request,sizeof(request));consumed=999;
  assert(camera_rtsp_input_next(&input,1,101,&request,&consumed)==(i<sizeof(rr)?CAMERA_RTSP_INPUT_NEED_MORE:CAMERA_RTSP_INPUT_RTCP));
  assert(!memcmp(&request,&before,sizeof(request)));assert(consumed==(i<sizeof(rr)?999:sizeof(rr)));
  camera_rtsp_input_clear(&input);zero_tail(&input);integration_cases++;
 }
 assert(!camera_rtsp_input_init(&input,100,2000));assert(!camera_rtsp_input_append(&input,rr,1,100));
 assert(camera_rtsp_input_next(&input,0,101,&request,&consumed)==CAMERA_RTSP_INPUT_INVALID&&input.failure==CAMERA_RTSP_INPUT_STATE);integration_cases++;
 for(unsigned kind=0;kind<6;kind++) {
  unsigned char bad[sizeof(rr)];memcpy(bad,rr,sizeof(rr));
  if(kind<2)bad[1]=(unsigned char)(kind?2:0);
  else if(kind==2){bad[2]=255;bad[3]=255;}
  else if(kind==3)bad[4]=0xa0;
  else if(kind==4)bad[5]=205;
  else bad[7]=2;
  assert(!camera_rtsp_input_init(&input,100,2000));assert(!camera_rtsp_input_append(&input,bad,sizeof(bad),100));
  assert(camera_rtsp_input_next(&input,1,101,&request,&consumed)==CAMERA_RTSP_INPUT_INVALID);
  camera_rtsp_input_clear(&input);zero_tail(&input);integration_cases++;
 }
 /* First header starts at accept, not first byte; equality expires. */
 assert(!camera_rtsp_input_init(&input,100,2000));assert(!camera_rtsp_input_tick(&input,2099));
 assert(camera_rtsp_input_tick(&input,2100)==-1&&input.failure==CAMERA_RTSP_INPUT_DEADLINE);integration_cases++;
 assert(!camera_rtsp_input_init(&input,100,2000));
 for(unsigned i=0;i<19;i++)assert(!camera_rtsp_input_append(&input,"X",1,100U+100U*i));
 assert(camera_rtsp_input_tick(&input,2100)==-1&&input.failure==CAMERA_RTSP_INPUT_DEADLINE);integration_cases++;
 assert(!camera_rtsp_input_init(&input,100,2000));assert(camera_rtsp_input_tick(&input,99)==-1&&input.failure==CAMERA_RTSP_INPUT_CLOCK);integration_cases++;
 assert(!camera_rtsp_input_init(&input,100,2000));assert(camera_rtsp_input_tick(&input,UINT64_MAX)==-1);integration_cases++;
 assert(camera_rtsp_input_init(&input,UINT64_MAX,2000)==-1);assert(camera_rtsp_input_init(&input,0,499)==-1);
 /* Erase retired opaque Authorization while preserving a coalesced partial tail. */
 const char *s="OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nAuthorization: DUMMY-PUBLIC,a,b\r\n\r\n";
 assert(!camera_rtsp_input_init(&input,100,2000));assert(!camera_rtsp_input_append(&input,s,strlen(s),100));
 assert(!camera_rtsp_input_append(&input,"OPT",3,101));
 assert(camera_rtsp_input_next(&input,0,102,&request,&consumed)==CAMERA_RTSP_INPUT_REQUEST);
 assert(request.method.length==7&&!memcmp(request.method.data,"OPTIONS",7));
 assert(request.target.length==1&&request.target.data[0]=='*');
 assert(!camera_rtsp_input_retire(&input,consumed,103));assert(input.used==3&&!memcmp(input.data,"OPT",3));zero_tail(&input);
 assert(input.started_ms==100&&camera_rtsp_input_tick(&input,2100)==-1);camera_rtsp_input_clear(&input);zero_tail(&input);integration_cases++;
 const char *body="OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 4096\r\n\r\nCSeq: 2\r\n\r\n";
 assert(!camera_rtsp_input_init(&input,100,2000));assert(!camera_rtsp_input_append(&input,body,strlen(body),100));
 assert(camera_rtsp_input_next(&input,0,101,&request,&consumed)==CAMERA_RTSP_INPUT_NEED_MORE);
 assert(camera_rtsp_input_tick(&input,2100)==-1);integration_cases++;
}
static void legacy_input_negatives(void)
{
 static const char *const bad[]={
  "OPTIONS * RTSP/2.0\r\nCSeq: 1\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\ncseq: 1\r\n\r\n",
  "DESCRIBE rtsp://mock/etc/passwd RTSP/1.0\r\nCSeq: 1\r\n\r\n",
  "SETUP rtsp://mock/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n\r\n",
  "SETUP rtsp://mock/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\nPLAY rtsp://mock/live RTSP/1.0\r\nCSeq: 2\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nAuthorization: DUMMY-PUBLIC\r\nauthorization: DUMMY-PUBLIC\r\n\r\n",
  "OPTIONS * RTSP/1.0\r\nCSeq: 1\r\nContent-Length: 4\r\n\r\n$\r\nx"
 };
 for(size_t i=0;i<sizeof(bad)/sizeof(bad[0])+1;i++) {
  qc_legacy_reset_mocks(2);
  qc_request_override=i<sizeof(bad)/sizeof(bad[0])?(const void *)bad[i]:(const void *)rr;
  qc_request_override_bytes=i<sizeof(bad)/sizeof(bad[0])?strlen(bad[i]):sizeof(rr);
  assert(!camera_rtsp_registry_init(&resources,"rtsp://mock/live","rtsp://mock/live/track1"));
  struct qc_legacy_service service;source_context_init(&service.source);service.terminal_result=CAMERA_OK;
  assert(client_session(&service,50)==CAMERA_OK);
  assert(qc_client_closes==1&&qc_recvs==1&&!qc_opens&&!qc_begins&&!qc_nexts&&!qc_ends&&!qc_closes);
  assert(!service.source.retirement_uncertain&&service.source.continuous_frame==-1&&service.source.h264_stream_fd==-1);
  integration_cases++;
 }
 /* Good coalesced SETUP/RR/TEARDOWN uses the same legacy demultiplexer and
  * Session policy, with no physical source request or descriptor allocation. */
 qc_legacy_reset_mocks(2);unsigned char wire[512];
 const char *setup="SETUP rtsp://mock/live RTSP/1.0\r\nCSeq: 1\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n";
 const char *end="TEARDOWN rtsp://mock/live RTSP/1.0\r\nCSeq: 2\r\nSession: QC07F1\r\n\r\n";
 size_t n=strlen(setup);memcpy(wire,setup,n);memcpy(wire+n,rr,sizeof(rr));n+=sizeof(rr);memcpy(wire+n,end,strlen(end));n+=strlen(end);
 qc_request_override=wire;qc_request_override_bytes=n;
 struct qc_legacy_service service;source_context_init(&service.source);service.terminal_result=CAMERA_OK;
 assert(client_session(&service,50)==CAMERA_OK&&qc_client_closes==1&&qc_sends==2&&!qc_opens&&!qc_nexts);
 qc_legacy_mode=0;qc_request_override=NULL;qc_request_override_bytes=0;stop=0;integration_cases++;
}
int main(void)
{
 char *argv[]={"owned-host-fixture",NULL};assert(saved_lifecycle_tests(1,argv)==0);
 policy_negatives();interleaved_and_session();memory_units();legacy_input_negatives();
 printf("Strict parser integration: PASS cases=%u; owned loopback + memory only\n",integration_cases);return 0;
}
