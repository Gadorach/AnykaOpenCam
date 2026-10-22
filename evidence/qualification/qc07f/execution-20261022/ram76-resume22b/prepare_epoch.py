#!/usr/bin/env python3
"""Fresh22b copies of fixed native-passed76 recipes; originals stay immutable.

Only dates/paths/part/output labels and explicit retained-HOLD handling change.
This prepares source/metadata only; it does not import or execute a recipe.
"""
from pathlib import Path
import datetime
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[5]
HERE = Path(__file__).resolve().parent
OLD = ROOT / 'evidence/qualification/qc07f/execution-20261021/ram76-resume21a'
BASE_OLD = '/home/gadorach/.local/state/anyka-bench-ssh/execute-20261021T0407Z'
BASE_NEW = '/home/gadorach/.local/state/anyka-bench-ssh/unattended-20261022T0419Z'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('Unexpected baseline source for exact replacement')
    return text.replace(old, new)


def main():
    rows = []
    baseline = json.loads((OLD / 'SOURCE_DERIVATION21A01.json').read_bytes())
    baseline_pins = {row['file']: row['new_sha256'] for row in baseline['inputs']}
    for name in ('cold_gate.py', 'ram76_boot.py', 'postcheck.py'):
        original = (OLD / name).read_bytes()
        if digest(original) != baseline_pins[name]:
            raise RuntimeError('Previously accepted21a recipe pin differs')
        text = original.decode()
        text = replace_once(text, BASE_OLD, BASE_NEW)
        # These exact epoch labels name new private/public files, never authority.
        text = text.replace('21A', '22B').replace('21a', '22b').replace('20261021a', '20261022b')
        # Import only the narrow new lock adapter, not a replacement UART transport.
        text = replace_once(text, 'b = private.boot\n', 'b = private.boot\nfrom preserved_hold import operation\n')
        if name == 'cold_gate.py':
            old = '''    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert not lock.read(1)
'''
            text = replace_once(text, old, '    with operation("RAM76_cold22b_readonly_gate"):\n')
            text = replace_once(text, '''            lock.write(json.dumps({"state": "HOLD", "operation": "RAM76_cold22b_readonly_gate"}) + "\\n")
            lock.flush(); os.fsync(lock.fileno())
''', '')
            text = replace_once(text, '            lock.seek(0); lock.truncate(); lock.flush(); os.fsync(lock.fileno())\n', '')
        elif name == 'ram76_boot.py':
            text = replace_once(text, '''    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert not held.read(1)
''', '    with operation("one_fixed_RAM76_cold22b_boot"):\n')
            text = replace_once(text, '''            held.write(json.dumps({"state": "HOLD", "operation": "one_fixed_RAM76_cold22b_boot", "image_sha256": SHA, "flash_write": False}) + "\\n")
            held.flush(); os.fsync(held.fileno())
''', '')
            text = replace_once(text, '            held.seek(0); held.truncate(); held.flush(); os.fsync(held.fileno())\n', '')
        else:
            text = replace_once(text, '''    with (b.CONTROL / "peripheral-gui-operation.lock").open("r+") as held:
        fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert not held.read(1)
        held.write(json.dumps({"state": "HOLD", "operation": "RAM76_readonly_postcheck22b"}) + "\\n")
        held.flush(); os.fsync(held.fileno())
''', '    with operation("RAM76_readonly_postcheck22b"):\n')
            text = replace_once(text, '            held.seek(0); held.truncate(); held.flush(); os.fsync(held.fileno())\n', '')
        result = text.encode()
        compile(result, str(HERE / name), 'exec')  # syntax only; no recipe runs
        with (HERE / name).open('xb') as stream:
            stream.write(result)
        rows.append({'file': name, 'original_sha256': digest(original),
                     'new_sha256': digest(result), 'bytes': len(result),
                     'epoch_path_labels_and_retained_HOLD_adapter_only': True})
    record = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'argv': sys.argv, 'cwd': str(ROOT), 'python': sys.version,
              'preparer_sha256': digest(Path(__file__).read_bytes()),
              'preserved_hold_sha256': digest((HERE / 'preserved_hold.py').read_bytes()),
              'recipe_source_transforms': rows, 'hardware_or_recipe_executed': False,
              'original_failed_radio_HOLD_will_remain_byte_exact': True,
              'new_subordinate_failure_marker_is_sticky': True}
    with (HERE / 'SOURCE_DERIVATION22B01.json').open('x') as stream:
        json.dump(record, stream, indent=2)
        stream.write('\n')
    print('Fresh22b recipe copies prepared; no device/recipe execution; original HOLD handling preserved')


if __name__ == '__main__':
    main()
