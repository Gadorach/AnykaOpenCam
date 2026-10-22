# SPDX-License-Identifier: GPL-2.0-only
"""Exact derivative of reviewed watchdog-v2 generated native/producer bytes."""


def once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('nonunique reviewed overlay context: ' + old[:80])
    return text.replace(old, new)


def derive(native, source):
    source = once(source, '#include "watchdog_owner.h"\n',
        '#include "watchdog_owner.h"\n#include "owned-provider-types.h"\n')
    source = once(source, ' struct qw_owner watchdog;int encoder_hold,legacy_control;',
        ' struct qw_owner watchdog;int encoder_hold,legacy_control;\n struct qp_roles providers;')
    source = once(source, 'c->watchdog=(struct qw_owner)QW_OWNER_EMPTY;}',
        'c->watchdog=(struct qw_owner)QW_OWNER_EMPTY;c->providers=(struct qp_roles)QP_ROLES_EMPTY;}')
    source = once(source, 'static int capture_failed(struct qc_rtsp_context *c)',
        'static void provider_hold(struct qc_rtsp_context *);\nstatic int capture_failed(struct qc_rtsp_context *c)')
    source = once(source, '{c->retirement_uncertain=1;qw_owner_fail(&c->watchdog);return CAMERA_IO_ERROR;}',
        '{c->retirement_uncertain=1;qw_owner_fail(&c->watchdog);provider_hold(c);return CAMERA_IO_ERROR;}')
    source = once(source, '#include "source-close-once.inc"',
        '#include "provider-observe.inc"\n#include "source-close-once.inc"')
    source = once(source, ' if(c->retirement_uncertain||qw_owner_check(&c->watchdog))return capture_failed(c);',
        ' if(c->retirement_uncertain||!provider_shape(c)||qw_owner_check(&c->watchdog))return capture_failed(c);')
    source = once(source, '   if(c->continuous_frame<0){',
        '   if(c->providers.phase==QP_COMMITTED){ /* preloaded FD does NOT skip activation */')
    source = once(source,
        '    c->continuous_sensor=open("/dev/i2c-0",O_RDWR);\n    if(c->continuous_sensor<0||ioctl(c->continuous_sensor,I2C_SLAVE,0x30)||sensor_read(c->continuous_sensor,0x3107)!=0xda||sensor_read(c->continuous_sensor,0x3108)!=0x4d||sensor_read(c->continuous_sensor,0x0100)!=0)goto done;\n     c->continuous_frame=open("/dev/qc07f-frame",O_RDONLY);\n     if(c->continuous_frame<0)goto done;',
        '    if(provider_observe(c)||qw_owner_check(&c->watchdog))goto done;')
    source = once(source,
        '     if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_BIND,&bound))goto done;',
        '     if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_BIND,&bound)||provider_store_frame(c,&bound))goto done;')
    source = once(source, '     c->last_source_instance=bound.source_instance;c->last_file_instance=bound.file_instance;',
        '     /* Selector/register observations are effects ONLY under actual guard,\n'
        '      * after all adopted identities/global lifetime and fresh BIND. */\n'
        '     if(qw_owner_check(&c->watchdog)||ioctl(c->continuous_sensor,I2C_SLAVE,0x30)||\n'
        '        sensor_read(c->continuous_sensor,0x3107)!=0xda||sensor_read(c->continuous_sensor,0x3108)!=0x4d||\n'
        '        sensor_read(c->continuous_sensor,0x0100)!=0)goto done;\n'
        '     c->last_source_instance=bound.source_instance;c->last_file_instance=bound.file_instance;')
    source = once(source, '     if(ioctl(c->continuous_frame,QC_ISP_RECEIPT_ARM,&armed))goto done;',
        '     if(qw_owner_check(&c->watchdog)||ioctl(c->continuous_frame,QC_ISP_RECEIPT_ARM,&armed)||provider_store_frame(c,&armed))goto done;')
    source = once(source, '     if(sensor_stream(c->continuous_sensor,1))goto done;',
        '     if(sensor_stream(c->continuous_sensor,1))goto done;\n     c->providers.phase=QP_ACTIVE;')
    source = source.replace('qc_receipt_query(c->continuous_frame,', 'provider_receipt_query(c,')
    source = once(source, '  int encoder=-1,result=CAMERA_IO_ERROR;', '  int result=CAMERA_IO_ERROR;')
    source = once(source, ' encoder=open("/dev/qc07f-encoder",O_RDWR);if(encoder<0)goto done;',
        '  if(!provider_shape(c)||provider_encoder_observe(c))goto done;')
    source = once(source, ' if(h264_p_frames){\n   if(c->h264_stream_fd<0)c->h264_stream_fd=open("/dev/qc07f-encoder",O_RDWR);\n   if(c->h264_stream_fd<0)goto done;',
        '   if(h264_p_frames==1){')
    source = once(source, '    if(ioctl(c->h264_stream_fd,QC_HWENC_H264_STREAM,&q))goto done;job=q.frame;',
        '    if(ioctl(c->h264_stream_fd,QC_HWENC_H264_STREAM,&q))goto done;\n'
        '    if(q.frame_num>65535)goto done;\n'
        '    c->providers.codec_frame_num=q.frame_num;job=q.frame;')
    source = once(source, ' }else if(ioctl(encoder,QC_HWENC_H264_IDR,&job))goto done;',
        ' }else goto done; /* temporary IDR-only role deliberately unsupported */')
    source = once(source,
        '   if(encoder>=0){\n    if(result!=CAMERA_OK)c->encoder_hold=encoder;\n    else if(source_close_once(c,&encoder)){if(encoder>=0)c->encoder_hold=encoder;result=CAMERA_IO_ERROR;}\n   }',
        '   /* Selected STREAM used only the adopted persistent description.\n'
        '    * No unused temporary open, alias or per-frame close/session claim. */')
    source = once(source, '   struct qc_rtsp_context *c=context;if(!c)return CAMERA_INVALID;',
        '   struct qc_rtsp_context *c=context;if(!c)return CAMERA_INVALID;\n'
        '   if(!provider_shape(c))return CAMERA_NOT_READY;')
    source = once(source,
        ' if(c->h264_stream_fd>=0&&source_close_once(c,&c->h264_stream_fd))return CAMERA_IO_ERROR;\n c->h264_stream_frames=0;c->ownership_idr=1;',
        ' if(c->h264_stream_fd>=0){\n'
        '  if(provider_encoder_observe(c)||source_close_once(c,&c->h264_stream_fd))return CAMERA_IO_ERROR;\n'
        ' }\n c->providers.phase=QP_CLOSED;\n c->h264_stream_frames=0;c->ownership_idr=1;')
    if 'open(' in source:
        raise RuntimeError('provider open fallback survived derivative')
    native = once(native,
        ' (native.watchdog.state==QW_UNKNOWN||native.watchdog.state==QW_DISABLED||native.watchdog.state==QW_ACTIVE5);}',
        ' (native.watchdog.state==QW_UNKNOWN||native.watchdog.state==QW_DISABLED||native.watchdog.state==QW_ACTIVE5)&&provider_shape(&native);}')
    native = once(native,
        ' if(native.h264_stream_fd<0)native.h264_stream_fd=open("/dev/qc07f-encoder",O_RDWR);',
        ' if(provider_observe(&native))return CAMERA_IO_ERROR;')
    native = once(native,
        ' if(native.h264_stream_fd<0||ioctl(native.h264_stream_fd,QC_HWENC_LIFETIME_QUERY,&q)||\n    q.version!=1||q.struct_bytes!=sizeof(q)||q.reserved||q.quarantined||q.dma_uncertain||\n    q.guard_poisoned||q.failed_reset_step||q.reset_error||q.fault_session||!q.session_sequence){\n  native.retirement_uncertain=1;return CAMERA_IO_ERROR;\n }',
        ' if(ioctl(native.h264_stream_fd,QC_HWENC_LIFETIME_QUERY,&q)||q.version!=1||\n'
        '    q.struct_bytes!=sizeof(q)||q.reserved)return capture_failed(&native);\n'
        ' native.providers.global_last=q; /* full logical snapshot before fault refusal */\n'
        ' if(q.quarantined||q.dma_uncertain||q.guard_poisoned||q.failed_reset_step||\n'
        '    q.reset_error||q.fault_session||q.session_sequence<native.providers.encoder_identity.session_identity)\n'
        '  return capture_failed(&native);')
    native = native.replace('qc_receipt_query(native.continuous_frame,', 'provider_receipt_query(&native,')
    native = once(native, '(void)unused;int r=idle_h264_checked(&native);',
        '(void)unused;if(lifetime())return CAMERA_IO_ERROR;int r=idle_h264_checked(&native);')
    native = once(native,
        ' uint64_t begin=nh_native_now(NULL);int r=nh_memory_runtime_gate(&memory_state,NH_MEM_FIXED_PAID,begin);',
        ' uint64_t begin=nh_native_now(NULL);\n'
        ' if(lifetime())return CAMERA_NOT_READY; /* reobserve roles before backing touches */\n'
        ' int r=nh_memory_runtime_gate(&memory_state,NH_MEM_FIXED_PAID,begin);')
    native = once(native,
        'void nh_native_fence(void *unused){(void)unused;native.retirement_uncertain=1;qw_owner_fail(&native.watchdog);}',
        'void nh_native_fence(void *unused){(void)unused;native.retirement_uncertain=1;qw_owner_fail(&native.watchdog);provider_hold(&native);}')
    native = once(native, ' fflush(stderr);\n for(;;)raise(SIGSTOP);',
        ' fprintf(stderr,"NH_PROVIDER_HOLD phase=%u staged_sensor=%d staged_frame=%d staged_encoder=%d expected_source=%llu expected_file=%llu expected_session=%llu last_frame_state=%u last_relation=%u\\n",\n'
        ' (unsigned)native.providers.phase,native.providers.sensor_staged,native.providers.frame_staged.fd,\n'
        ' native.providers.encoder_staged.fd,(unsigned long long)native.providers.frame_identity.source_instance,\n'
        ' (unsigned long long)native.providers.frame_identity.file_instance,\n'
        ' (unsigned long long)native.providers.encoder_identity.session_identity,\n'
        ' native.providers.frame_last.state,native.providers.encoder_last.reference_relation);\n'
        ' fflush(stderr);\n for(;;)raise(SIGSTOP);')
    native += '\n#include "provider-take.inc"\n'
    if 'open(' in native:
        raise RuntimeError('native encoder open fallback survived derivative')
    return {'native_adapter.c': native, 'rtspd-native-import.c': source}
