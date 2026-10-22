# SPDX-License-Identifier: GPL-2.0-only
"""Exact-context derivative of immutable v6 adapter/import; no execution."""


def once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('nonunique frozen overlay context: ' + old[:80])
    return text.replace(old, new)


def derive(native, source):
    source = once(source, '#include "sensor_baseline.h"\n',
                  '#include "sensor_baseline.h"\n#include "watchdog_owner.h"\n')
    source = once(source, ' int watchdog_hold,encoder_hold,legacy_control;',
        ' struct qw_owner watchdog;int encoder_hold,legacy_control;\n int last_close_attempt;unsigned close_unknown;')
    source = once(source,
        'c->h264_stream_fd=c->continuous_sensor=c->continuous_frame=c->watchdog_hold=c->encoder_hold=c->legacy_control=-1;',
        'c->h264_stream_fd=c->continuous_sensor=c->continuous_frame=c->encoder_hold=c->legacy_control=-1;c->last_close_attempt=-1;c->watchdog=(struct qw_owner)QW_OWNER_EMPTY;')
    source = once(source,
        'static int capture_failed(struct qc_rtsp_context *c)\n{c->retirement_uncertain=1;return CAMERA_IO_ERROR;}',
        'static int capture_failed(struct qc_rtsp_context *c)\n{c->retirement_uncertain=1;qw_owner_fail(&c->watchdog);return CAMERA_IO_ERROR;}')
    start = source.index('static int protected_guard_finish(')
    end = source.index('static int continuous_retire(', start)
    source = source[:start] + '#include "source-close-once.inc"\n' + source[end:]
    for field in ('continuous_sensor', 'continuous_frame'):
        source = once(source, 'if(close(c->'+field+'))return capture_failed(c);c->'+field+'=-1;',
                      'if(source_close_once(c,&c->'+field+'))return CAMERA_IO_ERROR;')
    source = once(source,
        'if(c->h264_stream_fd>=0){if(close(c->h264_stream_fd))return capture_failed(c);c->h264_stream_fd=-1;}',
        'if(c->h264_stream_fd>=0&&source_close_once(c,&c->h264_stream_fd))return CAMERA_IO_ERROR;')
    source = once(source, ' int watchdog=-1,timeout=5,result=-1;', ' int result=-1;')
    source = once(source, ' if(c->retirement_uncertain||c->watchdog_hold>=0)return -1;',
                  ' if(c->retirement_uncertain||qw_owner_check(&c->watchdog))return capture_failed(c);')
    source = once(source,
        ' watchdog=open("/dev/watchdog0",O_WRONLY);if(watchdog<0||ioctl(watchdog,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;\n', '')
    source = once(source, 'nh_sensor_exact139(c->continuous_sensor,watchdog,',
                  'nh_sensor_exact139(c->continuous_sensor,c->watchdog.fd,')
    source = once(source,
        'c->receipt_evidence=active;c->capture_sequence=q.sequence;c->raw_valid=1;result=0;break;',
        'c->receipt_evidence=active;c->capture_sequence=q.sequence;c->raw_valid=1;\n    /* Full extent/status/CRC + keyed ACTIVE receipt succeeded BEFORE this sole ping. */\n    if(qw_owner_capture_progress(&c->watchdog))goto done;result=0;break;')
    source = once(source, '  if(protected_guard_finish(c,watchdog,!result)!=CAMERA_OK)result=-1;',
                  '  if(!result&&qw_owner_check(&c->watchdog))result=capture_failed(c);')
    source = once(source, 'int encoder=-1,guard=-1,result=CAMERA_IO_ERROR,timeout=5;',
                  'int encoder=-1,result=CAMERA_IO_ERROR;')
    source = once(source, 'c->producer_begin_ns=begin*1000000ULL;',
                  'c->producer_begin_ns=c->watchdog.start_ns; /* include outer lifetime/admission cost */')
    source = once(source, '   if(prepare_output(c,c->producer_begin_ns)!=CAMERA_OK)return CAMERA_NOT_READY;',
                  '   if(qw_owner_check(&c->watchdog))return capture_failed(c);\n   if(prepare_output(c,c->producer_begin_ns)!=CAMERA_OK)return CAMERA_NOT_READY;')
    source = once(source,
        '  guard=open("/dev/watchdog0",O_WRONLY);if(guard<0||ioctl(guard,WDIOC_SETTIMEOUT,&timeout)||timeout!=5)goto done;',
        '  if(qw_owner_check(&c->watchdog))return capture_failed(c);')
    source = once(source,
        'else if(close(encoder)){c->encoder_hold=encoder;c->retirement_uncertain=1;result=CAMERA_IO_ERROR;}',
        'else if(source_close_once(c,&encoder)){if(encoder>=0)c->encoder_hold=encoder;result=CAMERA_IO_ERROR;}')
    source = once(source, '   if(protected_guard_finish(c,guard,result==CAMERA_OK)!=CAMERA_OK)result=CAMERA_IO_ERROR;',
                  '   if(result==CAMERA_OK&&qw_owner_check(&c->watchdog))result=CAMERA_IO_ERROR;')
    source = once(source,
        'if(c->h264_stream_fd>=0){int fd=c->h264_stream_fd;c->h264_stream_fd=-1;if(close(fd))c->retirement_uncertain=1;}',
        'if(c->h264_stream_fd>=0&&source_close_once(c,&c->h264_stream_fd))return CAMERA_IO_ERROR;')
    if '/dev/watchdog0' in source or 'protected_guard_finish' in source or 'watchdog_hold' in source:
        raise RuntimeError('old watchdog open/close alias survived derivative')
    native = once(native, 'static unsigned storage_selected,raw_reuse_requested;',
        'static unsigned storage_selected,raw_reuse_requested;\n'
        'static int native_guard_present(void)\n'
        '{return !stop&&!native.retirement_uncertain&&!source_clock.failed&&native.watchdog.fd>=3&&\n'
        ' (native.watchdog.state==QW_UNKNOWN||native.watchdog.state==QW_DISABLED||native.watchdog.state==QW_ACTIVE5);}')
    native = once(native, ' if(c->resident_storage)return CAMERA_OK;',
        ' if(qw_owner_check(&c->watchdog)){nh_native_fence(NULL);return CAMERA_IO_ERROR;}\n if(c->resident_storage)return CAMERA_OK;')
    for marker in ('int nh_native_memory_start(uint64_t start)\n{',
                   'int nh_native_memory_fixed_buffers(void *hub)\n{',
                   'int nh_native_memory_gate(void *unused,enum nh_memory_stage stage)\n{'):
        native = once(native, marker, marker+'\n if(!native_guard_present())return CAMERA_NOT_READY;')
    for old, new in (
        ('void nh_native_init(void)', 'static void native_init_legacy(void)'),
        ('int nh_native_memory_start(uint64_t start)', 'static int native_memory_start_legacy(uint64_t start)'),
        ('int nh_native_next(void *unused,', 'static int native_next_legacy(void *unused,'),
        ('int nh_native_release(void *unused,', 'static int native_release_legacy(void *unused,'),
        ('int nh_native_retire(void *unused)', 'static int native_retire_body(void *unused)'),
        ('int nh_native_retire_held(void *unused,', 'static int native_retire_held_legacy(void *unused,')):
        native = once(native, old, new)
    native = once(native, 'int r=nh_native_retire(NULL);', 'int r=native_retire_body(NULL);')
    native = once(native,
        ' if(!r&&!parked_after_stop&&native.resident_storage){if(native.resident_storage!=raw)free(native.resident_storage);native.resident_storage=NULL;}',
        ' if(!r&&qw_owner_check(&native.watchdog)){nh_native_fence(NULL);return CAMERA_IO_ERROR;}\n /* Retain resident backing until outer logical watchdog-disable ACK. */')
    native = once(native,
        ' /* Actual normal source END/QUERY/ACK succeeded before this known CPU free. */',
        ' /* Actual normal source END/QUERY/ACK succeeded before this known CPU free. */\n if(qw_owner_check(&native.watchdog)){native.produced_storage=parked_after_stop;parked_after_stop=NULL;nh_native_fence(NULL);return CAMERA_IO_ERROR;}')
    native = once(native,
        ' if(native.resident_storage!=raw)free(native.resident_storage);native.resident_storage=NULL;parked_after_stop=NULL;memset(&expected,0,sizeof(expected));return CAMERA_OK;',
        ' /* Retain actual parked/resident backing until outer disable ACK. */ return CAMERA_OK;')
    native = once(native, 'if(now==UINT64_MAX)native.retirement_uncertain=1;',
                  'if(now==UINT64_MAX){native.retirement_uncertain=1;qw_owner_fail(&native.watchdog);}')
    native = once(native, '{(void)unused;native.retirement_uncertain=1;}',
                  '{(void)unused;native.retirement_uncertain=1;qw_owner_fail(&native.watchdog);}')
    native = once(native, ' native.retirement_uncertain=1;\n fprintf(stderr,"NH_QUARANTINE',
                  '  nh_native_fence(NULL);\n fprintf(stderr,"NH_QUARANTINE')
    native = once(native, 'watchdog=%d CPU_loan=',
        'watchdog_actual_fd=%d watchdog_state=%u close_unknown=%u last_close_attempt_NOTowned=%d CPU_parked=%u CPU_loan=')
    native = once(native,
        'native.encoder_hold,native.watchdog_hold,native.produced_storage!=NULL,',
        'native.encoder_hold,native.watchdog.fd,(unsigned)native.watchdog.state,native.close_unknown,native.last_close_attempt,parked_after_stop!=NULL,native.produced_storage!=NULL,')
    native += '\n#include "native-watchdog.inc"\n'
    return {'native_adapter.c': native, 'rtspd-native-import.c': source}
