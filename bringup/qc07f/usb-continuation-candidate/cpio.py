# SPDX-License-Identifier: GPL-2.0-only
"""Strict in-memory newc transformation; never extracts or executes entries."""
from pathlib import PurePosixPath
import stat

if not __debug__:
    raise SystemExit("Archive safety checks require assertions; do not use Python -O")


def align(value):
    return (value + 3) & ~3


def parse(data):
    entries = []
    seen = set()
    offset = 0
    while True:
        assert offset + 110 <= len(data) and data[offset:offset + 6] == b"070701"
        raw_header = data[offset + 6:offset + 110]
        assert all(x in b"0123456789abcdefABCDEF" for x in raw_header)
        fields = [int(raw_header[i:i + 8], 16) for i in range(0, 104, 8)]
        assert fields[11] > 0 and fields[12] == 0
        name_end = offset + 110 + fields[11]
        assert name_end <= len(data)
        name_bytes = data[offset + 110:name_end]
        assert name_bytes.endswith(b"\0") and b"\0" not in name_bytes[:-1]
        name = name_bytes[:-1].decode("ascii")
        path = name[2:] if name.startswith("./") else name
        assert path and not PurePosixPath(path).is_absolute() and ".." not in PurePosixPath(path).parts
        assert str(PurePosixPath(path)) == path
        assert path not in seen
        seen.add(path)
        start = align(name_end); end = start + fields[6]; next_offset = align(end)
        assert next_offset <= len(data)
        assert not any(data[name_end:start]) and not any(data[end:next_offset])
        entries.append({"path": path, "name": name, "fields": fields,
                        "payload": data[start:end], "raw": data[offset:next_offset]})
        if path == "TRAILER!!!":
            assert fields[6] == 0 and not any(data[next_offset:])
            return entries
        offset = next_offset


def record(name, fields, payload):
    encoded = name.encode("ascii") + b"\0"
    words = list(fields)
    assert len(words) == 13 and all(0 <= x <= 0xffffffff for x in words)
    assert len(payload) <= 0xffffffff and len(encoded) <= 0xffffffff
    words[6], words[11], words[12] = len(payload), len(encoded), 0
    result = b"070701" + b"".join(f"{x:08x}".encode() for x in words) + encoded
    result += b"\0" * (align(len(result)) - len(result))
    result += payload
    return result + b"\0" * (align(len(result)) - len(result))


def update(data, replacements, additions):
    old = parse(data)
    names = {x["path"] for x in old}
    assert set(replacements) <= names and not set(additions) & names
    assert not set(replacements) & set(additions)
    assert "TRAILER!!!" not in replacements and "TRAILER!!!" not in additions
    parts = []
    for entry in old[:-1]:
        if entry["path"] in replacements:
            assert stat.S_ISREG(entry["fields"][1]) and entry["fields"][4] == 1
            parts.append(record(entry["name"], entry["fields"], replacements[entry["path"]]))
        else:
            parts.append(entry["raw"])
    inode = max(x["fields"][0] for x in old) + 1
    for name, payload in additions.items():
        parts.append(record(name, [inode, stat.S_IFREG | 0o755, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0], payload))
        inode += 1
    parts.append(old[-1]["raw"])
    result = b"".join(parts)
    result += b"\0" * ((-len(result)) % 512)
    new = {x["path"]: x for x in parse(result)}
    assert set(new) == names | set(additions)
    for entry in old:
        current = new[entry["path"]]
        if entry["path"] in replacements:
            assert current["payload"] == replacements[entry["path"]]
            assert all(a == b for i, (a, b) in enumerate(zip(entry["fields"], current["fields"])) if i != 6)
        else:
            assert current["raw"] == entry["raw"]
    for name, payload in additions.items():
        assert new[name]["payload"] == payload
    return result
