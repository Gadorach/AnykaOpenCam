/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef NH_TCP_SINK_H
#define NH_TCP_SINK_H
#include "handoff.h"
#include <netinet/in.h>
#define NH_TCP_PARENT_PORT 17891U
#define NH_TCP_SNDBUF_REQUEST 4096
#define NH_TCP_RCVBUF_REQUEST 1024
struct nh_tcp_sink {
 int fd,close_attempted,close_result,failed,requested_send,actual_send,requested_receive,actual_receive;
 uint64_t session_start,unit_start,last_clock;
 size_t accepted,unit_remaining,complete_prefix;
 uint64_t (*now)(void *);int (*cancelled)(void *);void *clock_context;
};
int nh_tcp_endpoint(const char *,const char *,struct sockaddr_in *);
int nh_tcp_connect(struct nh_tcp_sink *,const struct sockaddr_in *,uint64_t,
 uint64_t (*)(void *),int (*)(void *),void *);
int nh_tcp_begin(void *,uint64_t,size_t);
int nh_tcp_write(void *,const uint8_t *,size_t,size_t *);
int nh_tcp_discard(void *,size_t); /* close only; NEVER remote rollback/ACK proof */
int nh_tcp_close(struct nh_tcp_sink *); /* once, even on EINTR/ambiguous failure */
#endif
