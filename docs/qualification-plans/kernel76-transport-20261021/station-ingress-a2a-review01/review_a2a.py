#!/usr/bin/env python3
"""Independent, exclusive public-source HOST review. No author runner execution."""
import argparse
import datetime
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

assert not sys.flags.optimize
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
DOC = HERE.parent
APP = ROOT / 'apps/qualification/kernel76-station-ingress'
SRC = APP / 'source'
A1 = ROOT / 'apps/qualification/kernel76-station'
AUTHOR = DOC / 'station-ingress-a2a-host01'
parser = argparse.ArgumentParser()
parser.add_argument('--run', default='run01')
args = parser.parse_args()
assert re.fullmatch(r'run[0-9]{2}', args.run)
OUT = HERE / args.run
OUT.mkdir(exist_ok=False)
inputs, extracts, commands = {}, [], []
def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()
def sha(b):
    return hashlib.sha256(b).hexdigest()
def read(p):
    b = p.read_bytes()
    inputs[str(p.relative_to(ROOT))] = {'bytes': len(b), 'sha256': sha(b)}
    return b
def write(p, b):
    with p.open('xb') as f:
        f.write(b.encode() if isinstance(b, str) else b)
def load(p):
    return json.loads(read(p))
def verify(p, pin):
    b = read(p)
    assert len(b) == pin['bytes'] and sha(b) == pin['sha256'], p
    return b
def command(label, argv, fail=False):
    start = utc()
    r = subprocess.run([str(x) for x in argv], cwd=ROOT, capture_output=True,
                       timeout=100, env={'PATH': '/usr/bin:/bin', 'LC_ALL': 'C',
                       'ASAN_OPTIONS': 'detect_leaks=1:abort_on_error=0',
                       'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'})
    write(OUT / (label+'.stdout'), r.stdout)
    write(OUT / (label+'.stderr'), r.stderr)
    commands.append({'label': label, 'argv': list(map(str, argv)), 'cwd': str(ROOT),
                     'utc_start': start, 'utc_end': utc(), 'exit': r.returncode,
                     'stdout_sha256': sha(r.stdout), 'stderr_sha256': sha(r.stderr)})
    if fail:
        assert r.returncode != 0 and (b'FAIL line' in r.stderr or b'Sanitizer' in r.stderr), label
    else:
        assert r.returncode == 0, (label, r.returncode, r.stderr.decode(errors='replace'))
    return r
def take(p, start, end=None, replacement=None):
    original = read(p).decode()
    t = original if replacement is None else replacement
    start = start if start in t else start.replace('\n', '\r\n')
    assert t.count(start) == 1, (p, start)
    a = t.index(start)
    if end is None:
        b = len(t)
    else:
        end = end if end in t else end.replace('\n', '\r\n')
        assert t.count(end) == 1
        b = t.index(end)
    assert a < b
    body = t[a:b]
    extracts.append({'source': str(p.relative_to(ROOT)), 'source_sha256': sha(original.encode()),
                     'start_byte': a, 'bytes': len(body.encode()), 'start_line': t.count('\n', 0, a)+1,
                     'start_marker': start, 'end_marker': end or 'EOF',
                     'sha256': sha(body.encode()), 'mutated': replacement is not None})
    return body

def main():
    baseline = load(AUTHOR / 'copy-baseline.json')
    a1sealpath = DOC / 'station-control-a1-host01/candidate-seal.json'
    assert sha(read(a1sealpath)) == baseline['a1_seal_sha256']
    assert sha(read(DOC / 'STATION76_CONTROL_A1_REVIEW.md')) == baseline['a1_review_sha256']
    a1seal = load(a1sealpath)
    seal = load(AUTHOR / 'candidate-seal.json')
    for rel, pin in seal['files'].items():
        assert rel.startswith(('apps/qualification/kernel76-station-ingress/',
                               str(AUTHOR.relative_to(ROOT))+'/')) or rel == str((DOC / 'STATION76_INGRESS_A2A_CANDIDATE.md').relative_to(ROOT))
        assert '..' not in Path(rel).parts
        verify(ROOT / rel, pin)
    changed, pins = [], {}
    assert baseline['count'] == len(baseline['files']) == 68
    paths = {r['path'] for r in baseline['files']}
    for directory in (A1 / 'source', SRC):
        assert {str(p.relative_to(directory)) for p in directory.rglob('*') if p.is_file()} == paths
    for row in baseline['files']:
        rel = row['path']
        a = verify(A1 / 'source' / rel, a1seal['files'][str((A1 / 'source' / rel).relative_to(ROOT))])
        b = read(SRC / rel)
        assert row['offset'] == 0 and len(a) == row['bytes']
        assert sha(a) == row['original_sha256'] == row['copied_sha256']
        pins[rel] = {'A1_sha256': sha(a), 'A2a_sha256': sha(b), 'A2a_bytes': len(b)}
        if a != b:
            changed.append(rel)
            write(OUT / (rel.replace('/', '_')+'.diff'), ''.join(difflib.unified_diff(
                a.decode().splitlines(True), b.decode().splitlines(True), fromfile='A1/'+rel, tofile='A2a/'+rel)))
    assert changed == seal['changed_files'] and len(changed) == 7
    report = read(DOC / 'STATION76_INGRESS_A2A_CANDIDATE.md').decode()
    for rel in changed:
        assert f"| `{rel}` |{pins[rel]['A2a_bytes']}|`{pins[rel]['A2a_sha256']}`|" in report
    assert read(SRC / 'LICENSE') == read(A1 / 'source/LICENSE')
    testcopy = load(AUTHOR / 'test-copy.json')
    for name, pin in testcopy['files'].items():
        a = read(A1 / 'tests' / name)
        b = read(APP / 'tests' / name)
        assert sha(a) == pin['a1_sha256'] and sha(b) == pin['copied_sha256']
        assert len(a) == pin['a1_bytes'] and len(b) == pin['copied_bytes']
        if name == 'shim.h':
            a = a.replace(b'struct hgic_bus { unsigned int drv_tx_headroom; };',
                b'#ifndef A2A_BUS_DECL\nstruct hgic_bus { unsigned int drv_tx_headroom; int rx_failed; };\n#endif')
        assert a == b
    final = load(AUTHOR / 'run08/result.json')
    for rel, pin in final['inputs'].items():
        assert rel.startswith(('apps/qualification/kernel76-station', str(AUTHOR.relative_to(ROOT))))
        verify(ROOT / rel, pin)
    for rel, pin in load(AUTHOR / 'run08/output-hashes.json').items():
        assert '..' not in Path(rel).parts and not Path(rel).is_absolute()
        verify(AUTHOR / 'run08' / rel, pin)
    for e in final['extractions']:
        if not e['mutated']:
            b = read(ROOT / e['source']); part = b[e['start_byte']:e['start_byte']+e['bytes']]
            assert len(part) == e['bytes'] and sha(part) == e['sha256']
            assert b[:e['start_byte']].count(b'\n')+1 == e['start_line']
            if e.get('end_marker') == 'EOF':
                assert e['start_byte']+e['bytes'] == len(b)
    history = {}
    for n in range(1, 9):
        j = load(AUTHOR / f'run{n:02}/result.json')
        history[f'run{n:02}'] = {'all_pass': j.get('all_pass'), 'failure': j.get('failure'),
                                'commands': [(c['label'], c['exit']) for c in j['commands']]}
    write(OUT / 'provenance.json', json.dumps({'all68': pins, 'changed': changed,
          'A1_seal_sha256': sha(read(a1sealpath)), 'A2a_seal_sha256': sha(read(AUTHOR / 'candidate-seal.json')),
          'author_history': history}, indent=2)+'\n')

    # Construct our own units from uniquely bounded, byte-preserving source slices.
    # Match the inspected author's generated units exactly before any execution.
    defs = read(SRC / 'hgic_def.h').decode()
    constants = []
    for name in ('HGIC_WDEV_ID_STA', 'HGIC_HDR_TX_MAGIC', 'HGIC_HDR_RX_MAGIC', 'HGIC_WLAN_8400',
                 'HGIC_TX_COOKIE_MASK', 'HGIC_CMD_TIMEOUT', 'HGIC_BLOCK_ACK_CNT'):
        found = re.findall(r'^#define '+name+r'\s+[^\r\n]+', defs, re.M)
        assert len(found) == 1
        constants += found
    bus = take(SRC / 'hgic_def.h', 'struct hgic_bus {', '#ifdef __RTOS__\n#define ALLOC_ORDERED_WORKQUEUE')
    ack = take(SRC / 'hgic_def.h', 'struct hgic_dack_hdr {', 'struct hgic_nlmsg_hdr {')
    state = take(SRC / 'hgic_smac/hgics.h', 'enum hgics_state {', 'enum hgics_vif_state {')
    status = take(SRC / 'hgic_smac/hgics.h', 'struct hgics_txstatus {', 'int hgics_rx_data(')
    def fw(source):
        return take(source / 'utils/fwctrl.c', '#define STR_LEN', 'void hgic_fwctrl_init(')+take(
            source / 'utils/fwctrl.c', 'int hgic_fwctrl_get_fwinfo(', 'int hgic_fwctrl_set_countryregion(')
    def base(source, ingress):
        return '#include "'+('ingress_shim.h' if ingress else 'shim.h')+'"\n'+\
            '#include "'+str(source / 'hgic.h')+'"\n'+'\n'.join(constants)+'\n'+\
            (bus if ingress else '')+'#include "'+str(source / 'utils/fwctrl.h')+'"\n'+\
            '#include "'+str(source / 'utils/control_receipt.h')+'"\n'
    def unit(source, overrides=None):
        overrides = overrides or {}
        code = base(source, True)+fw(source)+ack+state+status
        code += '''
struct hgics_wdev {
    struct hgic_bus *bus;
    struct hgic_fwctrl ctrl;
    unsigned long state, last_rx;
    int if_test, test_rx_len, txwnd;
    struct completion txwnd_cp;
};
static void hgics_rx_ack(struct hgics_wdev *h, struct hgics_txstatus *s) { (void)h; (void)s; data_calls++; }
static void hgics_rx_agg_frm(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
static void hgics_rx_sigfrm(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
static void hgics_rx_bt_data(struct hgics_wdev *h, u8 *d, int n) { (void)h; (void)d; (void)n; data_calls++; }
'''
        original = source == A1 / 'source'
        if original:
            code += '#define A2A_ORIGINAL_REPRO 1\nstatic void hgics_rx_error(void *p) { (void)p; abort(); }\n'
        code += take(source / 'hgic_smac/rx.c', 'int hgics_rx_data(' if original else 'void hgics_rx_error(', replacement=overrides.get('rx'))
        p = source / 'utils/if_usb.c'
        code += take(p, '#define HGIC_PKT_MAX_LEN', 'static int txq_cnt', overrides.get('usb'))
        code += '''
static u32 max_pkt_len = HGIC_PKT_MAX_LEN;
static void hgic_usb_receive(struct urb *urb);
static void hgic_usb_cancle(struct hgic_usb *d) { (void)d; cancel_calls++; }
'''
        code += take(p, 'static void hgic_usb_enq(', 'static int hgic_usb_submit_rx_urbs(', overrides.get('usb'))
        code += take(p, 'static void hgic_usb_receive(struct urb *urb)\n{', 'static void hgic_usb_tx_complete(', overrides.get('usb'))
        return code+'#include "'+str(APP / 'tests/test_ingress.c')+'"\n'
    inc = OUT / 'include'
    (inc / 'linux').mkdir(parents=True)
    for name in ('completion.h', 'workqueue.h', 'unaligned.h', 'etherdevice.h', 'build_bug.h'):
        write(inc / 'linux' / name, '#include "shim.h"\n')
    write(inc / 'linux/errno.h', '#include_next <linux/errno.h>\n')
    command('compiler-version', ['cc', '--version'])
    assert command('compiler-target', ['cc', '-dumpmachine']).stdout.strip() == b'x86_64-pc-linux-gnu'
    common = ['cc', '-std=gnu11', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
              '-Wno-unused-parameter', '-Wno-sign-compare', '-I'+str(inc), '-I'+str(APP / 'tests')]
    modes = [('optimized', ['-O2']), ('sanitized', ['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]
    candidate = unit(SRC)
    units = {'ingress': candidate, 'original-a1-surplus': unit(A1 / 'source'),
             'a1-regression': base(SRC, False)+fw(SRC)+'\n#include "'+str(APP / 'tests/test_control.c')+'"\n'}
    for name, code in units.items():
        assert code.encode() == read(AUTHOR / 'run08' / (name+'.c')), name
    # Separate main, same real functions and original fixture helpers; no parser replica.
    read(HERE / 'supplement.c')
    units['supplement'] = '#define main candidate_main\n'+candidate+'\n#undef main\n#include "'+str(HERE / 'supplement.c')+'"\n'
    for name, code in units.items():
        c = OUT / (name+'.c'); write(c, code)
        for mode, flags in modes:
            binary = OUT / (name+'-'+mode+'.host')
            command(name+'-'+mode+'-compile', common+flags+[c, '-o', binary])
            command(name+'-'+mode+'-test', [binary])
    mutants = []
    for row in final['mutants']:
        name, where, old, new = row['name'], row['target'], row['old'], row['new']
        p = SRC / ('hgic_smac/rx.c' if where == 'rx' else 'utils/if_usb.c')
        text = read(p).decode(); assert text.count(old) == 1
        code = unit(SRC, {where: text.replace(old, new)})
        assert code.encode() == read(AUTHOR / 'run08' / (name+'.c'))
        c = OUT / (name+'.c'); write(c, code)
        binary = OUT / (name+'.host')
        command(name+'-compile', common+modes[1][1]+['-Wno-unused-variable', c, '-o', binary])
        r = command(name+'-test', [binary], fail=True)
        mutants.append({'name': name, 'compile_exit': 0, 'runtime_exit': r.returncode})
    # All inputs, including original A1 files and source/test/runner freeze, recheck.
    for rel, pin in list(inputs.items()):
        verify(ROOT / rel, pin)
    return {'all_pass': True, 'source_files': 68, 'unchanged': 61, 'changed': changed,
            'exact_author_generated_units': True, 'runtime_mutants': mutants,
            'all_inputs_rechecked': True}

result = {'utc_start': utc(), 'argv': sys.argv, 'python': sys.version,
          'scope': 'INDEPENDENT_PUBLIC_SOURCE_SYNTHETIC_NATIVE_HOST_ONLY'}
read(Path(__file__))
try:
    result.update(main())
except Exception as exc:
    result.update({'all_pass': False, 'failure': repr(exc)})
    raise
finally:
    result.update({'utc_end': utc(), 'inputs': inputs, 'extractions': extracts, 'commands': commands})
    write(OUT / 'result.json', json.dumps(result, indent=2)+'\n')
    write(OUT / 'output-hashes.json', json.dumps({str(p.relative_to(OUT)): {
        'bytes': p.stat().st_size, 'sha256': sha(p.read_bytes())}
        for p in sorted(OUT.rglob('*')) if p.is_file()}, indent=2)+'\n')
print('Independent A2a SOURCE/HOST review PASS; no build/native authorization')
