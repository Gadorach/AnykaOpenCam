/* Complete production event consumer plus real USB/rx/fwctrl/A1 functions above.
 * Public synthetic inputs only. Output is fixed labels/counts, never contents.
 */
static unsigned int checks, fixtures, command_sends;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"FAIL line %d\n",__LINE__); exit(2); } } while (0)
static struct hgics_wdev hg;
static struct hgic_usb usb;
static struct hgic_usb_buf buf;
static struct urb urb;
static struct workqueue_struct wq;
static struct hgic_cmd_response pending;
static struct sk_buff *seed;
static u8 packet[65544];
static int nbytes, declared_len, status_code, offset, transfer_len;
static bool wrong_backing, null_backing, deferred;
static int command_mode;

static void make_event(int type, unsigned int id, int n)
{
    memset(packet, 0, sizeof(packet));
    put_unaligned_le16(HGIC_HDR_RX_MAGIC, packet);
    packet[2] = type; packet[3] = 0xf9;
    put_unaligned_le16(n, packet + 4);
    put_unaligned_le16(11, packet + 6);
    put_unaligned_le16(id, packet + 8);
    if (type == HGIC_HDR_TYPE_EVENT) packet[9] = 0xed;
    put_unaligned_le16(0xbeef, packet + 10);
    nbytes = declared_len = n;
}
static void make_info(u16 cookie)
{
    memset(packet, 0, sizeof(packet));
    put_unaligned_le16(HGIC_HDR_RX_MAGIC, packet);
    packet[2] = HGIC_HDR_TYPE_CMD; packet[3] = 1;
    put_unaligned_le16(36, packet + 4); put_unaligned_le16(cookie, packet + 6);
    packet[8] = HGIC_CMD_GET_FW_INFO;
    put_unaligned_le16(24, packet + 10);
    put_unaligned_le32(0x02010001, packet + 12);
    put_unaligned_le32(22118, packet + 16);
    put_unaligned_le16(HGIC_WLAN_8400, packet + 20);
    put_unaligned_le16(1, packet + 22);
    memcpy(packet + 24, (u8[]){2,0x11,0x22,0x33,0x44,0x55}, 6);
    nbytes = declared_len = 36;
}
static void dispatch(void)
{
    event_phase = true;
    hgic_fwctrl_work(&hg.ctrl.work);
    event_phase = false;
}
static void deliver(bool failure, bool submit, bool drain)
{
    size_t n = nbytes >= 0 && nbytes <= HGIC_PKT_MAX_LEN+1 ? nbytes : 12;
    u8 *mem = malloc(n + offset + 1), *snapshot = malloc(n + offset + 1);
    CHECK(mem && snapshot);
    memset(mem, 0xa6, n + offset + 1);
    memcpy(mem + offset, packet, n);
    memcpy(snapshot, mem, n + offset + 1);
    buf.data = null_backing ? NULL : mem + offset;
    urb.context = &buf; urb.transfer_buffer = wrong_backing ? packet : buf.data;
    urb.transfer_buffer_length = transfer_len;
    urb.actual_length = nbytes; urb.status = status_code;
    buf.flag = HGIC_USB_BUF_FLAG_USED | HGIC_USB_BUF_FLAG_RX;
    INIT_LIST_HEAD(&buf.list); list_add_tail(&buf.list, &usb.used);
    unsigned int before = submissions;
    event_phase = true;
    hgic_usb_receive(&urb);
    event_phase = false;
    CHECK(submissions == before + (submit ? 1U : 0U));
    CHECK(!memcmp(mem, snapshot, n + offset + 1));
    list_del(&buf.list);
    /* No original URB storage survives as event data: copied work is independent. */
    free(snapshot); free(mem);
    if (drain) {
        dispatch();
        CHECK(!!hg.ctrl.receipt_error == failure);
    }
    fixtures++;
}
static void schedule(struct hgic_fwctrl *c)
{
    struct sk_buff *tx = skb_dequeue(&c->txq);
    CHECK(tx != NULL); command_sends++;
    u16 cookie = get_unaligned_le16(tx->data + 6);
    dev_kfree_skb_any(tx);
    if (command_mode == 1 || command_mode == 2 || command_mode == 3) {
        make_event(HGIC_HDR_TYPE_EVENT2, command_mode == 1 ? 18 : 31, 12);
        put_unaligned_le16(cookie, packet + 6);
        deliver(command_mode != 1, true, false);
        make_info(cookie);
        if (command_mode == 3) {
            /* Good FWINFO queued before bad event, but no caller copy until both
             * ordered worker records have been consumed. */
            struct sk_buff *bad = skb_dequeue(&c->rxq);
            CHECK(bad != NULL);
            hgic_fwctrl_rx(c, packet, 36);
            skb_queue_tail(&c->rxq, bad);
        } else {
            deliver(command_mode != 1, true, false);
        }
        dispatch();
    } else {
        make_info(cookie); deliver(false,true,true);
    }
}
static void init(bool immediate)
{
    CHECK(live_skb == 0);
    memset(&hg,0,sizeof(hg)); memset(&usb,0,sizeof(usb));
    memset(&buf,0,sizeof(buf)); memset(&urb,0,sizeof(urb));
    memset(&pending,0,sizeof(pending));
    formats_payload = formats_constant = event_queue = semaphore_ups = 0;
    event_frees = all_frees = success_completions = error_completions = fake_logs = 0;
    exports = rtos_calls = tx_effects = recovery = scans = keys = init_success = 0;
    payload_begin = UINTPTR_MAX; payload_end = 0;
    watched_skb = nonlinear_skb = NULL; watched_guard = NULL;
    usb.bus.drv_tx_headroom = 3; usb.bus.bus_priv = &hg;
    usb.bus.rx_packet = hgics_rx_data; usb.bus.rx_error = hgics_rx_error;
    hg.bus = &usb.bus; hg.ctrl.bus = &usb.bus;
    hg.ctrl.wq = &wq; hg.ctrl.schedule = schedule;
    /* Exact production callback, not either regression suite's unrelated stub. */
    hg.ctrl.rx_event = hgics_rx_fw_event;
    CHECK(hg.ctrl.rx_event == hgics_rx_fw_event);
    watched_error = &hg.ctrl.receipt_error; watched_queue = &hg.evt_list;
    INIT_LIST_HEAD(&hg.ctrl.pd_list);
    INIT_LIST_HEAD(&usb.used); INIT_LIST_HEAD(&usb.rx_freeq); INIT_LIST_HEAD(&usb.tx_freeq);
    buf.usbdev = &usb; buf.urb = &urb;
    seed = dev_alloc_skb(1); *skb_put(seed,1) = 0x62;
    /* Seed without event publication hook; candidate must not touch it. */
    hg.evt_list.head = hg.evt_list.tail = seed; hg.evt_list.qlen = 1;
    hg.evt_sema.count = 7;
    pending.type = HGIC_HDR_TYPE_CMD; pending.cookie = 11;
    pending.ifidx = 1; pending.opcode = HGIC_CMD_GET_FW_INFO;
    list_add(&pending.list,&hg.ctrl.pd_list);
    host_immediate = immediate; deferred = !immediate;
    nbytes = declared_len = 12; status_code = offset = submit_error = command_mode = 0;
    transfer_len = HGIC_PKT_MAX_LEN; wrong_backing = null_backing = false;
    make_event(HGIC_HDR_TYPE_EVENT,18,12);
}
static void quiet(void)
{
    CHECK(!formats_payload && !formats_constant && !fake_logs);
    CHECK(!event_queue && !semaphore_ups && !success_completions);
    CHECK(!exports && !rtos_calls && !tx_effects && !recovery && !scans && !keys && !init_success);
    CHECK(hg.evt_list.head == seed && hg.evt_list.tail == seed && hg.evt_list.qlen == 1);
    CHECK(seed->len == 1 && seed->data[0] == 0x62 && hg.evt_sema.count == 7);
    CHECK(!pending.skb);
}
static void finish(bool failure)
{
    quiet();
    CHECK(!!hg.ctrl.receipt_error == failure && !!usb.bus.rx_failed == failure);
    CHECK(pending.done == failure && pending.cmpl.done == (failure ? 1U : 0U));
    CHECK(!hg.ctrl.rxq.qlen && !hg.ctrl.txq.qlen);
    list_del(&pending.list);
    CHECK(hg.ctrl.pd_list.next == &hg.ctrl.pd_list);
    dev_kfree_skb_any(seed);
    CHECK(!live_skb);
}
static void direct(int n, int align, int special, bool failure)
{
    struct sk_buff *s = NULL;
    if (special != 1) {
        size_t size = n >= 0 && n <= 65535 ? n : 12;
        s = dev_alloc_skb(size + align + 1);
        memset(s->base,0xa6,s->capacity); skb_reserve(s,align);
        memcpy(skb_put(s,size),packet,size);
        watched_skb = s; watched_guard = s->data + size;
        payload_begin = (uintptr_t)s->data + 12;
        payload_end = (uintptr_t)s->data + size;
        if (special == 2) nonlinear_skb = s;
        if (special == 3) s->data = NULL;
        if (special == 4) s->len = 65536;
    }
    unsigned int before = live_skb;
    event_phase = true;
    hg.ctrl.rx_event(&hg.ctrl,s);
    event_phase = false;
    CHECK(live_skb == before - (special == 1 ? 0U : 1U));
    CHECK(event_frees == (special == 1 ? 0U : 1U));
    CHECK(!!hg.ctrl.receipt_error == failure);
    watched_skb = nonlinear_skb = NULL; watched_guard = NULL;
    fixtures++;
}
static void debug_cases(void)
{
    static const char *const synthetic[] = {"%n", "%p", "%s", "%", "unterminated", "", "\0tail", "binary"};
    static const int lengths[] = {2,2,2,1,12,0,5,6};
    for (int type = 4; type <= 14; type += 10) {
        for (unsigned int i=0; i<sizeof(lengths)/sizeof(lengths[0]); i++) {
            for (int align=0; align<8; align++) {
                init(false); make_event(type,18,12+lengths[i]);
                memcpy(packet+12,synthetic[i],lengths[i]);
                direct(nbytes,align,0,false); finish(false);
                for (int immediate=0; immediate<2; immediate++) {
                    init(immediate); make_event(type,18,12+lengths[i]);
                    memcpy(packet+12,synthetic[i],lengths[i]); offset=align;
                    deliver(false,true,true); finish(false);
                }
            }
        }
        init(false); make_event(type,18,65535); memset(packet+12,0xa5,65535-12);
        direct(65535,7,0,false); finish(false);
        for (int immediate=0; immediate<2; immediate++) {
            init(immediate); make_event(type,18,HGIC_PKT_MAX_LEN);
            memset(packet+12,0xff,HGIC_PKT_MAX_LEN-12);
            deliver(false,true,true); finish(false);
        }
    }
}
static void ids_and_fields(void)
{
    for (int type=4; type<=14; type+=10) {
        unsigned int max = type == 4 ? 255 : 65535;
        for (unsigned int id=0; id<=max; id++) {
            init(false); make_event(type,id,12);
            direct(12,id%8,0,id!=18); finish(id!=18);
            for (int immediate=0; immediate<2; immediate++) {
                init(immediate); make_event(type,id,12); offset=id%8;
                deliver(id!=18,id==18 || !immediate,true); finish(id!=18);
            }
        }
        for (unsigned int field=0; field<65536; field++) {
            init(false); make_event(type,18,12);
            packet[3] = field; put_unaligned_le16(field,packet+6);
            put_unaligned_le16(field,packet+10);
            if (type==4) packet[9]=field>>8;
            direct(12,field%8,0,false); finish(false);
        }
    }
}
static void envelope_cases(void)
{
    for (int type=4;type<=14;type+=10) {
        for (unsigned int d=0;d<=65535;d++) {
            init(false); make_event(type,18,12); put_unaligned_le16(d,packet+4);
            direct(12,d%8,0,d!=12); finish(d!=12);
            init(false); make_event(type,18,12); put_unaligned_le16(d,packet+4);
            deliver(d!=12,d==12,true); finish(d!=12);
        }
        /* Full actual range, tiny backings for values refused by USB before read. */
        for (int n=0;n<=65535;n++) {
            init(false); make_event(type,18,12); nbytes=n;
            deliver(n!=12,n==12,true); finish(n!=12);
        }
        for (int n=0;n<12;n++) {
            init(false); make_event(type,18,n);
            direct(n,n%8,0,true); finish(true);
            init(false); make_event(type,18,n);
            deliver(true,false,true); finish(true);
        }
        for (int prefix=1;prefix<=4;prefix++) {
            init(false); make_event(type,18,12);
            memmove(packet+prefix,packet,12); memset(packet,0xff,prefix); nbytes=12+prefix;
            deliver(true,false,true); finish(true);
        }
        for (int immediate=0;immediate<2;immediate++) {
            for (int n=13;n<=HGIC_PKT_MAX_LEN;n++) {
                init(immediate); make_event(type,18,n); offset=n%8;
                memset(packet+12,0xff,n-12); deliver(false,true,true); finish(false);
            }
            for (int surplus=0;surplus<3;surplus++) {
                init(immediate); make_event(type,18,12); nbytes=24;
                if (surplus==1) memset(packet+12,0xbb,12);
                if (surplus==2) memcpy(packet+12,packet,12);
                deliver(true,false,true); finish(true);
            }
        }
    }
    for (int special=1;special<=4;special++) {
        init(false); direct(12,0,special,true); finish(true);
    }
    for (int type=0;type<256;type++) {
        if (type==4 || type==14) continue;
        init(false); make_event(type,18,12); direct(12,type%8,0,true); finish(true);
    }
    init(false); packet[0]^=1; direct(12,0,0,true); finish(true);
}
static void completion_cases(void)
{
    for (int s=0;s<8;s++) for (int zero=0;zero<2;zero++) {
        init(s&1); status_code=(int[]){-EIO,-EPROTO,-ENOENT,-ECONNRESET,1,INT_MAX,INT_MIN,-EPIPE}[s];
        nbytes=zero?0:12; deliver(true,false,true); finish(true);
    }
    for (int s=0;s<4;s++) {
        init(false); usb.status=HGIC_USB_STATUS_STOP | (s&1?HGIC_USB_STATUS_ERR:0);
        status_code=s&2?-ENOENT:0; null_backing=true;
        deliver(false,false,true);
        /* Unchanged A2a USB STOP diagnostic is constant, not event output. */
        CHECK(formats_constant==1 && fake_logs==1 && !formats_payload);
        formats_constant=fake_logs=0; finish(false);
    }
    init(false); usb.status=HGIC_USB_STATUS_ERR; deliver(true,false,true); finish(true);
    for (int kind=0;kind<8;kind++) {
        init(false);
        if(kind==0)nbytes=-1;
        if(kind==1)nbytes=INT_MIN;
        if(kind==2)nbytes=INT_MAX;
        if(kind==3)transfer_len=-1;
        if(kind==4)transfer_len=11;
        if(kind==5)transfer_len=HGIC_PKT_MAX_LEN+1;
        if(kind==6)null_backing=true;
        if(kind==7)wrong_backing=true;
        deliver(true,false,true); finish(true);
    }
}
static void persistence_flood_commands(void)
{
    /* A bus-only failure must also cause disposal, not a new completion. */
    init(false); usb.bus.rx_failed=1; direct(12,0,0,false);
    CHECK(!hg.ctrl.receipt_error && !pending.done);
    usb.bus.rx_failed=0; finish(false);
    init(false); hg.ctrl.receipt_error=-EPROTO; direct(12,0,0,true);
    CHECK(!usb.bus.rx_failed && !pending.done && !error_completions);
    hg.ctrl.receipt_error=0; finish(false);
    for (int immediate=0;immediate<2;immediate++) {
        init(immediate);
        for (int i=0;i<64;i++) { make_event(i&1?4:14,18,12); deliver(false,true,true); }
        finish(false);
        init(immediate);
        for (int i=0;i<64;i++) { make_event(i&1?4:14,18,12); deliver(false,true,false); }
        CHECK(live_skb==(immediate?1U:65U));
        dispatch(); CHECK(all_frees==64 && live_skb==1); finish(false);
        init(immediate); make_event(4,31,12); deliver(true,!immediate,true);
        make_event(14,18,12); deliver(true,false,true);
        /* Direct queued work after failure, including bad types, cannot repair. */
        direct(12,0,0,true); CHECK(error_completions==1);
        make_info(11); deliver(true,false,true);
        u8 out[26]; memset(out,0xa5,sizeof(out)); unsigned int before=command_sends;
        CHECK(hgic_fwctrl_do_cmd(&hg.ctrl,1,HGIC_CMD_GET_FW_INFO,NULL,0,out+1,24)==-EPROTO);
        CHECK(command_sends==before);
        for(size_t j=0;j<sizeof(out);j++) CHECK(out[j]==0xa5);
        CHECK(formats_constant==1 && fake_logs==1 && !formats_payload);
        formats_constant=fake_logs=0; finish(true);
    }
    init(false);
    for(int i=0;i<64;i++) {
        event_frees=0; make_event(4,18,12); direct(12,i%8,0,false);
    }
    finish(false);
    init(false); make_event(14,18,36); packet[12]=HGIC_HDR_TYPE_CMD;
    packet[13]=HGIC_CMD_GET_FW_INFO; put_unaligned_le16(24,packet+14);
    direct(36,7,0,false); finish(false);
    CHECK(command_sends==0);
    /* Deferred unknown followed by already-queued debug: the same worker owns
     * both, denial precedes disposal, and two earlier resubmits remain visible. */
    init(false); make_event(14,31,12); deliver(false,true,false);
    make_event(4,18,12); deliver(false,true,false); dispatch(); finish(true);
    for(int m=1;m<=3;m++) {
        init(false); command_mode=m;
        u8 out[26]; memset(out,0xa5,sizeof(out));
        int rc=hgic_fwctrl_do_cmd(&hg.ctrl,1,HGIC_CMD_GET_FW_INFO,NULL,0,out+1,24);
        CHECK(rc==(m==1?24:-EPROTO));
        CHECK(out[0]==0xa5 && out[25]==0xa5);
        if(m==1) CHECK(!memcmp(out+1,packet+12,24));
        else for(size_t j=0;j<sizeof(out);j++) CHECK(out[j]==0xa5);
        /* A genuine CMD success is allowed, but not classified as event success. */
        CHECK(success_completions==(m==2?0U:1U)); success_completions=0;
        if(m!=1) {
            /* Existing do_cmd failure diagnostic is constant, not event output. */
            CHECK(formats_constant==1 && fake_logs==1 && !formats_payload);
            formats_constant=fake_logs=0;
        }
        finish(m!=1);
    }
}
static void original_observation(void)
{
    static const char *const fixtures_[] = {"%n","%p","%s","%","unterminated",""};
    static const int sizes[] = {2,2,2,1,12,0};
    unsigned int observed=0;
    for(int type=4;type<=14;type+=10) for(int i=0;i<6;i++) {
        init(false); make_event(type,18,12+sizes[i]); memcpy(packet+12,fixtures_[i],sizes[i]);
        direct(nbytes,0,0,false);
        CHECK(formats_payload==1 && formats_constant==0 && fake_logs==1);
        formats_payload=fake_logs=0; finish(false); observed++;
    }
    for(int type=4;type<=14;type+=10) {
        init(false); make_event(type,31,12);
        struct sk_buff *s=dev_alloc_skb(12); memcpy(skb_put(s,12),packet,12);
        payload_begin=(uintptr_t)s->data+12; payload_end=payload_begin;
        hg.ctrl.rx_event(&hg.ctrl,s);
        CHECK(event_queue==1 && semaphore_ups==1 && hg.evt_list.qlen==2);
        CHECK(skb_dequeue(&hg.evt_list)==seed); dev_kfree_skb_any(seed);
        CHECK(skb_dequeue(&hg.evt_list)==s); dev_kfree_skb_any(s);
        list_del(&pending.list); CHECK(!live_skb);
    }
    printf("OBSERVED FORMAT_FROM_PAYLOAD %u; CONSTANT 0; nondebug queue hooks 2\n",observed);
}
int main(void)
{
    u16 endian=1; CHECK(*(u8 *)&endian==1);
#ifdef ORIGINAL_EVENT_OBSERVATION
    original_observation();
#else
    debug_cases(); ids_and_fields(); envelope_cases(); completion_cases(); persistence_flood_commands();
    CHECK(!live_skb && !data_calls && !cancel_calls);
    printf("PASS A2b real event and ingress: %u assertions, %u fixtures, %u modeled command sends\n",checks,fixtures,command_sends);
#endif
    return 0;
}
