/* SPDX-License-Identifier: GPL-2.0-only
 * Provider close AFTER original retirement proof, not a watchdog close API.
 */
static int source_close_once(struct qc_rtsp_context *c,int *slot)
{
 if(!slot||*slot<0||slot==&c->watchdog.fd)return capture_failed(c);
 if(qw_owner_check(&c->watchdog))return capture_failed(c); /* keep actual FD */
 int attempted=*slot;
 *slot=-1; /* Linux can consume the FD even when close returns an error. */
 if(close(attempted)){
  c->last_close_attempt=attempted;c->close_unknown=1;
  return capture_failed(c); /* diagnostic number, NOT an owned description */
 }
 return CAMERA_OK;
}
