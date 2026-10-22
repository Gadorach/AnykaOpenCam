"""Static public source custody/excerpt reader. Never imports/executes candidate code."""
from pathlib import Path
from datetime import datetime, timezone
import difflib
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[4]
D = Path('docs/qualification-plans/kernel76-transport-20261021')
OUT = D / 'station-control-owned-p3-review01'
E = D / 'station-control-owned-p3-source01'
S = Path('apps/qualification/kernel76-station-control-owned/source')
B = Path('apps/qualification/kernel76-station-owner-lifetime/source')
K = Path('work/lightbulb-camera/foundation-20261011T041148Z/artifacts/native-usb76-source01/linux')
inputs = {}


def pin(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def raw(path):
    path = Path(path)
    assert not path.is_absolute() and '..' not in path.parts
    assert not any(x in str(path) for x in ('station-attach-rx', 'P1_REVIEW', 'P1_TOOL'))
    p = ROOT / path
    assert p.is_file() and not p.is_symlink(), str(path)
    data = p.read_bytes()
    current = pin(data)
    assert str(path) not in inputs or inputs[str(path)] == current
    inputs[str(path)] = current
    return data


def obj(path):
    return json.loads(raw(path))


def verify(path, expected):
    actual = pin(raw(path))
    assert actual == expected, str(path)


def emit(name, value):
    p = ROOT / OUT / name
    with p.open('x') as f:
        f.write(value if isinstance(value, str) else json.dumps(value, indent=2) + '\n')


def seal(path, expected_hash=None):
    data = raw(path)
    if expected_hash:
        assert pin(data)['sha256'] == expected_hash
    files = json.loads(data)['files']
    for p, expected in files.items():
        verify(p, expected)
    return files


def text(path):
    return raw(path).decode().replace('\r\n', '\n')


def region(path, start, end):
    t = text(path)
    a = t.index(start)
    return t[a:t.index(end, a + len(start))].encode()


candidate_seal = E / 'candidate-seal.json'
files = seal(candidate_seal, '5d8a96a78e1b6d99f7a16f259d36f0e077644f7e7378c8ecd88b58109add66eb')
assert len(files) == 132
actual = {str(p.relative_to(ROOT)) for base in (S.parent, E)
          for p in (ROOT / base).rglob('*') if p.is_file()}
actual.add(str(D / 'STATION76_CONTROL_OWNED_P3_CANDIDATE.md'))
assert actual == set(files) | {str(candidate_seal)}

baseline = obj(E / 'copy-baseline.json')
inventory = obj(E / 'source-inventory.json')
source = {str(p.relative_to(ROOT / S)): pin(raw(p.relative_to(ROOT)))
          for p in sorted((ROOT / S).rglob('*')) if p.is_file()}
assert source == inventory['source']
assert hashlib.sha256(json.dumps(source, sort_keys=True, separators=(',', ':')).encode()).hexdigest() == inventory['source_sha256']
base_source = {str(p.relative_to(ROOT / B)): pin(raw(p.relative_to(ROOT)))
               for p in sorted((ROOT / B).rglob('*')) if p.is_file()}
assert base_source == baseline['baseline'] and len(base_source) == 72
changed = sorted(p for p in source if p in base_source and source[p] != base_source[p])
added = sorted(set(source) - set(base_source))
assert not set(base_source) - set(source)
assert len(changed) == 6 and len(added) == 2
delta = ''.join(''.join(difflib.unified_diff(
    text(B / p).splitlines(keepends=True) if p in base_source else [],
    text(S / p).splitlines(keepends=True),
    fromfile='p4a/' + p if p in base_source else '/dev/null', tofile='p3-owned/' + p))
    for p in changed + added)
assert delta.encode() == raw(E / 'OWNED_CONTROL_DELTA.patch')
emit('actual-p4a-p3-diff.patch', delta)

# Reconstruct the complete custody union independently, not just its count.
prior_seals = [
    D / 'station-attach-init-p2-source01/candidate-seal.json',
    D / 'station-attach-init-p2-review01/review-seal.json',
    D / 'station-attach-init-p2-v2-source01/candidate-seal.json',
    D / 'station-attach-init-p2-v2-review01/review-seal.json',
    D / 'station-attach-tx-p3-source01/candidate-seal.json',
    D / 'station-owner-lifetime-p4a-source01/candidate-seal.json',
    D / 'station-owner-lifetime-p4a-review01/review-seal.json',
]
union = {}
for p in prior_seals:
    entries = seal(p, 'c677fe73b1e28691365e31383780f12b89b797bafe6baa075f6d7ae366bdc4ff'
                   if p.parent.name == 'station-owner-lifetime-p4a-review01' else None)
    for name, value in {**entries, str(p): pin(raw(p))}.items():
        assert name not in union or union[name] == value
        union[name] = value
assert union == baseline['frozen_custody'] and len(union) == 451
accepted_count = 0
for name, record in baseline['accepted'].items():
    p = D / name / 'candidate-seal.json'
    verify(p, record['seal'])
    accepted_seal = obj(p)['files']
    assert {k: v for k, v in accepted_seal.items() if '/source/' in k} == record['source']
    for p, expected in record['source'].items():
        verify(p, expected)
        accepted_count += 1
assert accepted_count == 204

# Match the author's explicitly preserved ranges, plus individually selected
# actual producer/return functions whose byte identity matters to this review.
blocks = [
    ('hgic_smac/attach_init.c', 'static void attach_owner_stop(', 'static int attach_core_probe('),
    ('utils/owner_usb.c', 'static void hgic_usb_dispose(', 'static int hgic_usb_probe('),
    ('utils/owner_usb.c', 'static void hgic_usb_receive(struct urb *urb)\n{', 'static void hgic_usb_tx_complete(struct urb *urb)\n{'),
    ('utils/owner_usb.c', 'static void hgic_usb_reserve_slot(', 'static void hgic_usb_requeue('),
]
preserved = {}
for p, start, end in blocks:
    a, b = region(S / p, start, end), region(B / p, start, end)
    assert a == b
    key = p + '::' + start
    assert pin(a) == inventory['preserved_p4a_blocks'][key]
    preserved[key] = pin(a)
for p, start, end in [
    ('hgic_smac/attach_init.c', 'static void attach_probe_post(', 'static void attach_owner_stop('),
    ('hgic_smac/attach_init.c', 'static int attach_core_probe(', 'static int attach_netdev_event('),
    ('hgic_smac/attach_init.c', 'static void __exit attach_module_exit(', 'module_init('),
    ('utils/owner_usb.c', 'static int hgic_usb_submit_rx(', 'static void hgic_usb_receive(struct urb *urb)\n{'),
    ('utils/owner_usb.c', 'static int hgic_usb_probe(', 'static struct usb_driver hgic_usb_driver'),
]:
    a, b = region(S / p, start, end), region(B / p, start, end)
    assert a == b
    preserved[p + '::' + start] = pin(a)

excerpts = {}
def excerpt(path, start, end):
    lines = raw(path).decode().splitlines(keepends=True)
    value = ''.join(lines[start - 1:end])
    excerpts.setdefault(str(path), []).append({
        'start_line': start, 'end_line': end, 'text': value,
        'sha256': hashlib.sha256(value.encode()).hexdigest()})

# Every author P3 and reviewed P4a excerpt is checked against real source bytes.
api_counts = {}
for p in [E / 'request-api-contracts.json',
          D / 'station-owner-lifetime-p4a-review01/public-excerpts.json']:
    data = obj(p)
    api_counts[str(p)] = 0
    if isinstance(data, list):
        rows = [(x['path'], x) for x in data]
    else:
        rows = []
        for name, record in data.items():
            verify(name, {k: record[k] for k in ('bytes', 'sha256')})
            rows.extend((name, x) for x in record['excerpts'])
    for name, x in rows:
        value = ''.join(raw(name).decode().splitlines(keepends=True)[x['start_line'] - 1:x['end_line']])
        assert value == x['text'] and hashlib.sha256(value.encode()).hexdigest() == x['sha256']
        excerpt(name, x['start_line'], x['end_line'])
        api_counts[str(p)] += 1
for p, ranges in {
    'utils/owned_control.c': [(1, 432)],
    'utils/owned_control.h': [(1, 65)],
    'utils/fwctrl.c': [(55, 157), (225, 279), (325, 375), (430, 468), (494, 574)],
    'hgic_smac/attach_init.c': [(26, 109), (288, 339), (430, 515), (700, 720)],
    'utils/owner_usb.c': [(225, 411)],
    'hgic_smac/Makefile': [(1, 29)],
    'utils/control_receipt.h': [(12, 97)],
}.items():
    for a, b in ranges:
        excerpt(S / p, a, b)
for p, ranges in {
    'kernel/sched/completion.c': [(21, 54), (85, 174)],
    'kernel/locking/semaphore.c': [(158, 184), (213, 250)],
    'include/linux/refcount.h': [(364, 450)],
    'kernel/time/time.c': [(781, 812)],
    'include/linux/timekeeping.h': [(24, 36), (168, 176)],
    'drivers/usb/core/hcd.c': [(1519, 1569), (1634, 1676)],
    'drivers/usb/core/urb.c': [(724, 770)],
    'kernel/workqueue.c': [(4463, 4485)],
    'kernel/module/main.c': [(810, 847)],
    'include/linux/netdevice.h': [(4173, 4213)],
    'include/linux/skbuff.h': [(3477, 3510)],
}.items():
    for a, b in ranges:
        excerpt(K / p, a, b)
syntax = obj(E / 'syntax04/receipt.json')
assert syntax['source'] == {str(S / p): v for p, v in source.items()} and syntax['pass'] is True
assert len(syntax['commands']) == 5
for command in syntax['commands']:
    assert command['returncode'] == 0 and '-fsyntax-only' in command['argv']
    verify(E / 'syntax04' / command['log'], command['log_pin'])
raw(Path(__file__).relative_to(ROOT))
emit('public-pin-verification.json', {
    'candidate_seal_sha256': pin(raw(candidate_seal))['sha256'],
    'candidate_files_exact_set': len(files), 'source': source,
    'source_sha256': inventory['source_sha256'], 'changed': changed, 'added': added,
    'unchanged_p4a': 66, 'prior_custody_exact_union': len(union),
    'accepted_source_pins': accepted_count, 'preserved_p4a_blocks': preserved,
    'api_excerpt_counts': api_counts,
    'syntax04': {'commands': 5, 'returncodes': [c['returncode'] for c in syntax['commands']],
                 'rerun': False, 'external_installation_rehashed': False},
    'scope': 'Static byte/text/API/metadata verification only; not behavioral evidence.'})
emit('public-excerpts.json', {p: {**inputs[p], 'excerpts': x} for p, x in excerpts.items()})
emit('reader-receipt.json', {
    'utc': datetime.now(timezone.utc).isoformat(), 'argv': sys.argv,
    'cwd': str(ROOT), 'python': sys.version,
    'scope': 'Public static reading/diff/hash/metadata only; no candidate/helper/test/build execution, P1, target, private IO, network or Git/index operations.',
    'inputs': inputs})
print(json.dumps({'candidate_files': len(files), 'source_files': len(source),
                  'custody': len(union), 'accepted_pins': accepted_count,
                  'inputs': len(inputs), 'excerpts': sum(map(len, excerpts.values()))}))
