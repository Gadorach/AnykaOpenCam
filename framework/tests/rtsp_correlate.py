#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Correlate strict receiver NALs against preserved target-source UART evidence."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import sys

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-evidence', type=Path, required=True)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    text = json.loads(args.source_evidence.read_text())['text']
    found = re.findall(r'RTSP SOURCE timestamp=(\d+) nal_bytes=(\d+) crc_bzip2=([0-9a-f]{8}) keyframe=([01])', text)
    source = {}
    for ts, count, crc, key in found:
        if int(ts) in source:
            raise ValueError('Ambiguous repeated source timestamp')
        source[int(ts)] = (int(count), crc, bool(int(key)))
    units = [json.loads(line) for line in (args.capture / 'units.jsonl').read_text().splitlines()]
    mismatches = [u for u in units if source.get(u['timestamp']) !=
                  (u['nal_bytes'], u['crc_bzip2'], u['keyframe'])]
    sends = [dict(zip(('consumer', 'timestamp', 'nal_bytes', 'keyframe', 'send_ms',
                      'wire_bytes', 'packet_calls', 'complete', 'errno'), map(int, row)))
             for row in re.findall(r'SHARED RTSP AU consumer=(\d+) timestamp=(\d+) nal_bytes=(\d+) keyframe=([01]) send_ms=(\d+) wire_bytes=(\d+) packet_calls=(\d+) complete=([01]) errno=(\d+)', text)]
    capture = json.loads((args.capture / 'manifest.json').read_text())
    inputs = [args.source_evidence, args.capture / 'units.jsonl', args.capture / 'manifest.json', Path(__file__)]
    report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'argv': sys.argv,
              'inputs': [{'path': str(p), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in inputs],
              'checked_frames': len(units), 'source_frames': len(found), 'mismatches': mismatches,
              'all_received_nal_crc_lengths_keyframe_flags_match_source': bool(units) and not mismatches,
              'send_max_ms': max((s['send_ms'] for s in sends), default=None),
              'successful_sends_over_500ms': [s for s in sends if s['complete'] and s['send_ms'] > 500],
              'failed_sends': [s for s in sends if not s['complete']],
              'whole_capture_passed': capture['all_complete'], 'partial_fu_bytes': capture['partial_fu_bytes'],
              'cryptographic_corruption_proof': False,
              'scope': 'CRC/length/flag correlation, not a cryptographic proof or whole-product qualification'}
    args.output.open('x').write(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))
    return 0 if report['all_received_nal_crc_lengths_keyframe_flags_match_source'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
