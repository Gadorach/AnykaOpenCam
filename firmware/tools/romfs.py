"""Observed Anyka AliOS ROMFS variant, adapted from the original acquisition's
extract_romfs.py (2026-10-08). Type 6 is vendor LZ4 data, NOT a POSIX socket.
No upstream ROMFS/LZ4 implementation is vendored here; host liblz4 supplies LZ4.
"""
import ctypes
from pathlib import PurePosixPath
import struct

MAX_FILE = 32 * 1024 * 1024


def checksum(data):
    if len(data) % 4:
        raise ValueError("Unaligned ROMFS checksum span")
    return sum(struct.unpack(">" + str(len(data)//4) + "I", data)) & 0xffffffff


def unpack_romfs(partition, base, out, unpack=True):
    total = struct.unpack_from(">I", partition, 8)[0]
    if partition[:8] != b"-rom1fs-" or not 32 <= total <= len(partition):
        raise ValueError("Invalid ROMFS extent")
    data = partition[:total]
    if checksum(data[:min(512, total)]) != 0:
        raise ValueError("ROMFS superblock checksum failure")
    source = "containers/00250000-rootfs.romfs"
    out.save(source, data, "original", base, total, "ROMFS declared extent; superblock checksum valid")
    if not unpack:
        return
    label_end = data.index(b"\0", 16, 256)
    root_offset = (label_end + 1 + 15) & ~15
    entries, seen, names = [], set(), set()

    def bounds(offset, size):
        if offset < 0 or size < 0 or offset+size > total:
            raise ValueError("Out-of-bounds ROMFS range")

    def walk(offset, parent, depth=0):
        if depth > 32:
            raise ValueError("ROMFS directory depth limit")
        while offset:
            if offset in seen or len(seen) > 10000:
                raise ValueError("ROMFS repeated inode/cycle/scan limit")
            seen.add(offset)
            bounds(offset, 16)
            if offset % 16:
                raise ValueError("Misaligned ROMFS inode")
            nxt, spec, size, _ = struct.unpack_from(">4I", data, offset)
            end = data.index(b"\0", offset+16, min(total, offset+16+256))
            name = data[offset+16:end].decode("utf-8")
            content = (end+1+15) & ~15
            bounds(offset, content-offset)
            if checksum(data[offset:content]) != 0:
                raise ValueError("ROMFS inode metadata checksum failure")
            kind = nxt & 7
            path = str(PurePosixPath(parent) / name)
            entry = {"path": path, "inode_offset": offset,
                     "flash_inode_offset": base+offset, "type_code": kind,
                     "executable_bit": bool(nxt & 8), "spec": spec,
                     "logical_size": size, "data_offset": content,
                     "metadata_checksum_valid": True}
            entries.append(entry)
            if name in (".", ".."):
                entry["dot_entry_skipped"] = True
            else:
                if not name or "/" in name or PurePosixPath(path).is_absolute() or ".." in PurePosixPath(path).parts:
                    raise ValueError("Unsafe ROMFS member name")
                if path in names:
                    raise ValueError("Duplicate ROMFS path")
                names.add(path)
                if kind == 1:
                    walk(spec, path, depth+1)
                elif kind == 2:
                    bounds(content, size)
                    if size > MAX_FILE:
                        raise ValueError("ROMFS file exceeds bound")
                    entry.update(stored_offset=content, stored_size=size, encoding="plain")
                elif kind == 6:
                    bounds(content, 12)
                    block, codec, logical_size = struct.unpack_from("<3I", data, content)
                    if block != 4096 or codec != 1 or logical_size != size or size > MAX_FILE:
                        raise ValueError("Unsupported ROMFS compressed-file header")
                    count = (size+block-1)//block
                    bounds(content+12, count*8)
                    expected = content+12+count*8
                    blocks = []
                    for i in range(count):
                        stored_size, stored_offset = struct.unpack_from("<2I", data, content+12+i*8)
                        bounds(stored_offset, stored_size)
                        if stored_size < 4 or stored_offset != expected:
                            raise ValueError("Invalid/noncontiguous ROMFS block table")
                        prefix = struct.unpack_from("<I", data, stored_offset)[0]
                        if prefix & 0x7fffffff != stored_size-4:
                            raise ValueError("ROMFS compressed prefix/table mismatch")
                        blocks.append({"offset": stored_offset, "stored_size": stored_size,
                                       "raw_block": bool(prefix & 0x80000000),
                                       "decoded_size": min(block, size-i*block)})
                        expected += stored_size
                    entry.update(stored_offset=content, stored_size=expected-content,
                                 encoding="vendor-lz4-blocks", blocks=blocks)
                else:
                    raise ValueError("Unsupported ROMFS inode type; refusing extraction")
            offset = nxt & ~15

    walk(root_offset, "")
    # Audit every path/table/checksum before creating image-supplied paths.
    out.report("audits/romfs.json", {"filesystem_bytes": total,
                                    "superblock_checksum_valid": True, "entries": entries})
    lz4 = ctypes.CDLL("liblz4.so.1")
    lz4.LZ4_decompress_safe.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
    lz4.LZ4_decompress_safe.restype = ctypes.c_int
    for entry in entries:
        if entry.get("dot_entry_skipped"):
            continue
        path = entry["path"]
        if entry["type_code"] == 1:
            out.directory("filesystems/romfs/" + path)
            continue
        start, size = entry["stored_offset"], entry["stored_size"]
        encoded = data[start:start+size]
        if entry["encoding"] == "vendor-lz4-blocks":
            stored_name = "stored-romfs/"+path+".vendor-lz4"
            out.save(stored_name, encoded, source, start, size, "unchanged vendor LZ4 block table/stream")
            decoded = bytearray()
            for block in entry["blocks"]:
                offset, n = block["offset"], block["stored_size"]
                payload = data[offset+4:offset+n]
                if block["raw_block"]:
                    if len(payload) != block["decoded_size"]:
                        raise ValueError("ROMFS raw block size mismatch")
                    decoded.extend(payload)
                else:
                    buf = ctypes.create_string_buffer(block["decoded_size"])
                    rc = lz4.LZ4_decompress_safe(payload, buf, len(payload), len(buf))
                    if rc != block["decoded_size"]:
                        raise ValueError("ROMFS LZ4 decoded length mismatch")
                    decoded.extend(buf.raw)
            if len(decoded) != entry["logical_size"]:
                raise ValueError("ROMFS decoded file length mismatch")
        else:
            decoded = encoded
        out.save("filesystems/romfs/"+path, decoded, source, start, size,
                 entry["encoding"] + " decoding; execute flag only in audit")
