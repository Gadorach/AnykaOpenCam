 /* Fixed public UI only. The API/authentication dispatch below is unchanged. */
 if(equal(h->method,"GET")){
  const struct ui_asset *asset=NULL;size_t j;char headers[512];int n,peer;
  for(j=0;j<sizeof(ui_assets)/sizeof(ui_assets[0]);j++)
   if(equal(h->uri,ui_assets[j].route)){asset=&ui_assets[j];break;}
  if(asset){
   n=snprintf(headers,sizeof(headers),
    "Content-Type: %s\r\nCache-Control: no-store\r\nConnection: close\r\n"
    "X-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\n"
    "Content-Security-Policy: default-src 'none'; script-src 'self'; style-src 'self'; "
    "connect-src 'self'; media-src blob:; form-action 'self'; base-uri 'none'; frame-ancestors 'none'\r\n",
    asset->type);
   /* Includes Mongoose's exact fixed status/Content-Length framing, before any send. */
   if(n<0||(size_t)n>=sizeof(headers)||asset->bytes>7168U||c->send.len||
      asset->bytes+(size_t)n+sizeof("HTTP/1.1 200 OK\r\nContent-Length:            \r\n\r\n")-1>8192U){
    fatal=1;c->is_closing=1;return;
   }
   peer=peer_find(c->id);
   mg_http_reply(c,200,headers,"%s",(const char *)asset->data);
   c->is_draining=1;c->is_full=1;
   if(peer>=0){app.peers[peer].done=1;app.peers[peer].drain_deadline=now+2*WEB_NS;}
   if(c->send.len>8192||web_heap_failed()){fatal=1;c->is_closing=1;}
   return;
  }
 }
