# SPDX-License-Identifier: GPL-2.0-only
"""Exact reviewed owned-provider build02 derivative; no producer/op replacement."""


def once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('nonunique reviewed context: ' + old[:80])
    return text.replace(old, new)


def derive(native, source):
    for marker in ('int nh_native_storage_select(const char *mode)\n{',
                   'int nh_native_optin_exact139(void)\n{',
                   'int nh_native_config(struct camera_fmp4_config *c)\n{'):
        native = once(native, marker,
            marker + '\n if(provider_terminal(&native))return CAMERA_NOT_READY;')
    native += '\n#include "provider-return.inc"\n'
    return {'native_adapter.c': native, 'rtspd-native-import.c': source}
