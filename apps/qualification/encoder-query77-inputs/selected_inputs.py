# SPDX-License-Identifier: GPL-2.0-only
"""Closed query77 public-byte inspection. Never admission, execution or freeze.

No authored import-time IO. Read INPUT_CONTRACT.md/README.md before selection.
"""
from dataclasses import dataclass
from enum import Enum
import hashlib
import json
import os
import stat
import struct
import sys
import zlib


_ROOT = "/home/gadorach/Desktop/AnykaOpenCam"
_F = "work/lightbulb-camera/foundation-20261011T041148Z"
_B = _F + "/analysis/encoder-query77-build02"
_A = _F + "/artifacts/encoder-query77-source02"
_I = "work/lightbulb-camera/implementation-20261008T205404Z"
_Q = "docs/qualification-plans/"
_RELEASE = "6.18.55-qc07f-encoder-query77"


@dataclass(frozen=True, slots=True)
class _Pin:
    label: str
    relative: str
    length: int
    sha256: str
    inspection: str = "PIN_ONLY"


_PINS = (
    _Pin("image", _F + "/artifacts/encoder-query77-images02/QC6EQ77.UIM", 3603307, "f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e", "LEGACY_IMAGE_STREAM"),
    _Pin("package", _B + "/PACKAGE02.json", 4011, "7fb787f19892da10c0c9eac9a256e61f283b8dbf154d2d4189661726f3642d37", "PACKAGE_ASSOCIATION"),
    _Pin("config", _B + "/build/.config", 58859, "d7b2ae8f167c8b56f442a201684eb433cc621bce45f52c98c94a87e48879b5bf", "CONFIG_IDENTITY"),
    _Pin("zimage", _B + "/build/arch/arm/boot/zImage", 3600672, "65bb107d9a33ca1733bfbdd8439a00b37e7c72f0f634cecfb9ce2cf924290eb8"),
    _Pin("dtb", _A + "/inputs/attempt45.dtb", 2571, "d837c58c90b56ebb6ce9346b7c1d69689d5a1122dcef55b0f72341e6d003e0b5"),
    _Pin("initramfs", _F + "/artifacts/native-usb76-source01/inputs/initramfs76.cpio", 2082816, "510e5fdc6e2a7b6ad4a52d82ad1de9381a85e143f7014a6f2cc3bdfb951e309f"),
    _Pin("release", _B + "/build/include/config/kernel.release", 30, "fa8bfc9743bfd2ff20c3df4b010af4319f43359523b887d5ce180fac6981d348", "RELEASE_IDENTITY"),
    _Pin("compile", _B + "/build/include/generated/compile.h", 164, "8c51d471baeb130ce5a5b31b20f006b9d0501433d2142a62cf3b5428f38c6a5c", "COMPILER_METADATA"),
    _Pin("client", "apps/qualification/encoder-query77-client/build01/arm/encoder-query77-qualification", 30140, "87a40f291600fee1296d96030726b9db825956e274e0db2a7ee5ea18f6a26ac6", "ELF32_STRUCTURE"),
    _Pin("client_c", "apps/qualification/encoder-query77-client/encoder-query77-qualification.c", 10258, "d132a2eeaae170586aa437a2e7a3102027d9ff656077c5102f57dd79d95268b7"),
    _Pin("uapi_client", "apps/qualification/encoder-query77-client/encoder-open-query.h", 2455, "6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37", "UAPI_BYTE_IDENTITY"),
    _Pin("uapi_frozen", "apps/qualification/kernel76-encoder-identity/encoder-open-query.h", 2455, "6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37", "UAPI_BYTE_IDENTITY"),
    _Pin("uapi_kernel", _A + "/linux/arch/arm/mach-ak3918ev200/encoder-open-query.h", 2455, "6052069fa5428185eb8c61871404256c53c30c12515eaa5c9ab80c424facde37", "UAPI_BYTE_IDENTITY"),
    _Pin("loader", _I + "/artifacts/uart-ram-loader-wordaligned.bin", 1872, "a1630c90f7f44666e941704778cbf119e5e2e13355113adca52f842bd9046ad1"),
    _Pin("recorder", _I + "/uart_capture.py", 25426, "135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00"),
    _Pin("boot", "scripts/qc07f_boot.py", 39829, "6ec0bd70513d530e64507c934ab1885da0cb547b907673c5162bf01c89f7d985"),
    _Pin("protected", "scripts/qc07f_private_root_transfer.py", 12461, "cb224d3cd8b635774b7e364b07b259d90640b30de269ea07351e767c3f97fa3c"),
    _Pin("custody", "apps/qualification/encoder-query77-custody-v2/custody.py", 42123, "d60176e2d2725460a22c82fee6ce1068a82a07a976daddb7287c52daf3ade16f"),
    _Pin("build_disposition", _Q + "encoder-query77-build-20261022/PARENT_DISPOSITION.md", 4625, "c7d4c03c1624b1b34ea95ffbea3b946927146fee3491009094ac376fa4d8de08"),
    _Pin("build_result", _Q + "encoder-query77-build-20261022/BUILD_RESULT.md", 11646, "837109f2375f6f6deed8d5760a9a3a166ebbd01bb74d1062efd1d203701c24b6"),
    _Pin("client_disposition", _Q + "encoder-query77-client-20261022/PARENT_DISPOSITION.md", 3195, "6799bb31468926f08be837e81860df8cda6367bbea595171b79820d1d40cfe79"),
    _Pin("client_contract", _Q + "encoder-query77-client-20261022/SOURCE_CONTRACT.md", 7906, "f54370919ad2282af120ec7f49c2bb24687134deb38095e889486c6b7a1ec439"),
    _Pin("client_report", _Q + "encoder-query77-client-20261022/CANDIDATE_REPORT.md", 11665, "0b2b73d6f1a6991f7d631995a1c086452514349f52aa654e47af852c6d10b981"),
    _Pin("custody_review", _Q + "encoder-query77-custody-v2-review-20261022/REVIEW.md", 12258, "bea15bb8f0201d14820a9e9acaf5df232c531385d951fa0abe8b62a43fe516fd"),
    _Pin("custody_review_seal", _Q + "encoder-query77-custody-v2-review-20261022/FINAL_SEAL01.json", 1579, "b58ec94be7db1ff6cb33295c14770424ade7566c709f043efe92527a1adbf9d4", "REVIEW_ASSOCIATION"),
)


class Reason(Enum):
    NONE = "INPUTS_NO_ERROR"
    PLATFORM = "INPUTS_PLATFORM"
    PATH = "INPUTS_PATH"
    METADATA = "INPUTS_METADATA"
    LENGTH = "INPUTS_LENGTH"
    PIN = "INPUTS_PIN"
    FORMAT = "INPUTS_FORMAT"
    HOST_IO = "INPUTS_HOST_IO"
    CLOSE_UNCERTAIN = "INPUTS_CLOSE_UNCERTAIN"


class InputsError(Exception):
    """Fixed codes only, including original primary and cleanup uncertainty."""
    def __init__(self, primary, postcheck=Reason.NONE, cleanup=Reason.NONE):
        primary = primary if type(primary) is Reason else Reason.HOST_IO
        postcheck = postcheck if type(postcheck) is Reason else Reason.METADATA
        cleanup = cleanup if type(cleanup) is Reason else Reason.CLOSE_UNCERTAIN
        self.primary_code = primary
        self.postcheck_code = postcheck
        self.cleanup_code = cleanup
        self.code = Reason.CLOSE_UNCERTAIN if cleanup is not Reason.NONE else primary
        super().__init__(self.code.value)


def _fail(reason):
    raise InputsError(reason) from None


def _require(condition, reason=Reason.FORMAT):
    if not condition:
        _fail(reason)


@dataclass(frozen=True, slots=True)
class InputFact:
    public_label: str
    bytes_observed: int
    sha256: str
    offline_inspection: str


@dataclass(frozen=True, slots=True)
class OfflineManifest:
    scope: str
    files: tuple[InputFact, ...]
    limitations: tuple[str, ...]


def _pin(label):
    return next(item for item in _PINS if item.label == label)


def _meta(value):
    return (value.st_dev, value.st_ino, value.st_mode, value.st_uid, value.st_gid,
            value.st_nlink, value.st_size, value.st_mtime_ns, value.st_ctime_ns)


def _directory(value):
    return value.st_dev, value.st_ino, value.st_mode, value.st_uid, value.st_gid


class _Read:
    """Fixed-manifest host descriptors only, explicit one-shot teardown."""
    def __init__(self, uid):
        self.uid, self.fd, self.original = uid, None, None
        self.dirs = []

    def _dirs_check(self):
        for index, (fd, name, saved) in enumerate(self.dirs):
            value = os.fstat(fd)
            path = os.stat("/", follow_symlinks=False) if index == 0 else os.stat(name, dir_fd=self.dirs[index - 1][0], follow_symlinks=False)
            _require(stat.S_ISDIR(value.st_mode) and value.st_uid in (0, self.uid)
                     and not value.st_mode & 0o022 and _directory(value) == saved
                     and _directory(path) == saved, Reason.PATH)

    def _file_check(self):
        value = os.fstat(self.fd)
        path = os.stat(self.name, dir_fd=self.dirs[-1][0], follow_symlinks=False)
        _require(_meta(value) == self.original and _meta(path) == self.original, Reason.METADATA)

    def open(self, pin):
        _require(type(pin) is _Pin and pin in _PINS, Reason.PATH)
        path = _ROOT + "/" + pin.relative
        parts = path[1:].split("/")
        _require(len(path) <= 1024 and len(parts) <= 32 and not pin.relative.startswith("/")
                 and all(part not in ("", ".", "..", ".git") for part in parts), Reason.PATH)
        flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK
        fd = os.open("/", flags)
        self.dirs.append([fd, "", None])
        self.dirs[-1][2] = _directory(os.fstat(fd))
        for part in parts[:-1]:
            self._dirs_check()
            fd = os.open(part, flags, dir_fd=self.dirs[-1][0])
            self.dirs.append([fd, part, None])
            self.dirs[-1][2] = _directory(os.fstat(fd))
        self._dirs_check()
        self.name = parts[-1]
        value = os.stat(self.name, dir_fd=self.dirs[-1][0], follow_symlinks=False)
        _require(stat.S_ISREG(value.st_mode) and value.st_uid in (0, self.uid)
                 and value.st_nlink == 1 and not value.st_mode & 0o7022, Reason.METADATA)
        _require(value.st_size == pin.length, Reason.LENGTH)
        self.original = _meta(value)
        self.fd = os.open(self.name, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK,
                          dir_fd=self.dirs[-1][0])
        self._file_check()

    def postcheck(self):
        failed = False
        try:
            self._dirs_check()
        except BaseException:
            failed = True
        if self.fd is not None:
            try:
                self._file_check()
            except BaseException:
                failed = True
        return Reason.METADATA if failed else Reason.NONE

    def close(self):
        failed = False
        fd, self.fd = self.fd, None
        if fd is not None:
            try:
                os.close(fd)
            except BaseException:
                failed = True
        while self.dirs:
            fd, _, _ = self.dirs.pop()
            try:
                os.close(fd)
            except BaseException:
                failed = True
        return Reason.CLOSE_UNCERTAIN if failed else Reason.NONE


def _bzip_update(crc, data):
    # Non-reflected CRC32/BZIP2, initial/final xor ffffffff, not zlib CRC.
    for byte in data:
        crc ^= byte << 24
        for _ in range(8):
            crc = ((crc << 1) ^ (0x04c11db7 if crc & 0x80000000 else 0)) & 0xffffffff
    return crc


def _pairs(rows):
    result = {}
    for key, value in rows:
        _require(key not in result)
        result[key] = value
    return result


def _json(data):
    return json.loads(data.decode("utf-8"), object_pairs_hook=_pairs)


def _package(data):
    value = _json(data)
    for label, key in (("image", "image_pin"), ("zimage", "zImage_pin"),
                       ("dtb", "DTB_pin"), ("initramfs", "initramfs_pin")):
        pin = _pin(label)
        _require(value[key] == {"bytes": pin.length, "sha256": pin.sha256})
    _require(value["image"] == _pin("image").relative and value["kernel_release"] == _RELEASE
             and value["fullimage_CRC32_BZIP2"] == "63ea4eac")
    _require(value["header"] == {"OS": 5, "arch": 2, "data_CRC32": "0d55fce6",
             "entry": 0x82408040, "header_CRC32": "011cd017", "load": 0x82408000,
             "magic": "0x27051956", "name": "QC07F encoder-query77 RAM ONLY",
             "outer_compression": 0, "payload_bytes": 3603243, "timestamp": 0, "type": 2})
    memory = value["memory"]
    for key, expected in (("RAM", [0x81800000, 0x84000000]),
                          ("kernel_live", [0x81808000, 0x82059bc0]),
                          ("staged_uImage", [0x82408000, 0x82777b6b]),
                          ("existing_volatile_media_CMA", [0x82800000, 0x83800000]),
                          ("decompressor_live_upper_bound", 0x82790578)):
        _require(memory[key] == expected)
    _require(0x82408000 + _pin("image").length == 0x82777b6b
             and 3603307 <= 0x400000 and 0x82790578 < 0x82800000)
    _require(value["station76_paired"] is False)


def _elf(data):
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
    ident, kind, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phnum, shsize, shnum, shstr = header
    _require(ident == b"\x7fELF\x01\x01\x01" + b"\0" * 9
             and (kind, machine, version, ehsize, phsize, shsize) == (2, 40, 1, 52, 32, 40))
    _require(flags & 0xff000000 == 0x05000000 and flags & 0x200 and not flags & 0x400)
    _require(1 <= phnum <= 32 and 1 <= shnum <= 128 and shstr < shnum
             and 52 <= phoff <= len(data) - phnum * 32 and 52 <= shoff <= len(data) - shnum * 40)
    entry_backed, stacks = False, 0
    for index in range(phnum):
        kind, offset, address, _, filesz, memsz, flags, align = struct.unpack_from("<8I", data, phoff + index * 32)
        _require(kind not in (2, 3) and offset <= len(data) and filesz <= len(data) - offset)
        if kind == 1:
            _require(filesz <= memsz and address + memsz <= 1 << 32 and flags & 3 != 3
                     and flags & ~7 == 0 and (align <= 1 or (align & (align - 1) == 0 and address % align == offset % align)))
            entry_backed |= bool(flags & 1 and address <= entry < address + filesz)
        if kind == 0x6474e551:
            _require(flags == 6)
            stacks += 1
    _require(entry_backed and stacks == 1)
    for index in range(shnum):
        _, kind, _, address, offset, size, _, _, _, _ = struct.unpack_from("<10I", data, shoff + index * 40)
        _require(kind != 6 and address + size <= 1 << 32)
        if kind != 8:
            _require(offset <= len(data) and size <= len(data) - offset)


def _small(pin, data):
    if pin.inspection == "PACKAGE_ASSOCIATION":
        _package(data)
    elif pin.inspection == "CONFIG_IDENTITY":
        lines = data.splitlines()
        for expected in (b'CONFIG_CC_VERSION_TEXT="clang version 23.1.3"', b"CONFIG_CLANG_VERSION=230103",
                         b"CONFIG_AS_VERSION=230103", b"CONFIG_LLD_VERSION=230103",
                         b'CONFIG_LOCALVERSION="-qc07f-encoder-query77"',
                         ('CONFIG_INITRAMFS_SOURCE="' + _ROOT + '/' + _pin("initramfs").relative + '"').encode()):
            _require(lines.count(expected) == 1)
    elif pin.inspection == "RELEASE_IDENTITY":
        _require(data == (_RELEASE + "\n").encode())
    elif pin.inspection == "COMPILER_METADATA":
        _require(data.splitlines().count(b'#define LINUX_COMPILER\t\t"clang version 23.1.3, LLD 23.1.3"') == 1)
    elif pin.inspection == "ELF32_STRUCTURE":
        _elf(data)
    elif pin.inspection == "REVIEW_ASSOCIATION":
        row = _json(data)["output_pins_excluding_this_seal"][_pin("custody_review").relative]
        _require(row == {"bytes": _pin("custody_review").length, "sha256": _pin("custody_review").sha256})


def _stream(reader, pin):
    digest, zspan, dtspan = hashlib.sha256(), hashlib.sha256(), hashlib.sha256()
    whole, payload, header = 0xffffffff, 0, b""
    small = bytearray()
    retain = pin.inspection not in ("PIN_ONLY", "LEGACY_IMAGE_STREAM")
    _require(not retain or pin.length <= 65536, Reason.LENGTH)
    for offset in range(0, pin.length, 4096):
        count = min(4096, pin.length - offset)
        chunk = os.pread(reader.fd, count, offset)
        _require(len(chunk) == count, Reason.LENGTH)
        digest.update(chunk)
        if retain:
            small.extend(chunk)
        if pin.label == "image":
            if offset == 0:
                header = chunk[:64]
            whole = _bzip_update(whole, chunk)
            payload = zlib.crc32(chunk[max(0, 64 - offset):], payload)
            for start, end, part in ((64, 3600736, zspan), (3600736, 3603307, dtspan)):
                left, right = max(start, offset) - offset, min(end, offset + count) - offset
                if left < right:
                    part.update(chunk[left:right])
    observed = digest.hexdigest()
    _require(observed == pin.sha256, Reason.PIN)
    _require(os.pread(reader.fd, 1, pin.length) == b"", Reason.LENGTH)
    if pin.label == "image":
        fields = struct.unpack(">7I4B32s", header)
        _require(fields == (0x27051956, 0x011cd017, 0, 3603243, 0x82408000, 0x82408040,
                 0x0d55fce6, 5, 2, 2, 0, b"QC07F encoder-query77 RAM ONLY".ljust(32, b"\0")))
        _require(zlib.crc32(header[:4] + b"\0" * 4 + header[8:]) == 0x011cd017
                 and payload == 0x0d55fce6 and whole ^ 0xffffffff == 0x63ea4eac)
        _require(zspan.hexdigest() == _pin("zimage").sha256 and dtspan.hexdigest() == _pin("dtb").sha256)
    elif retain:
        _small(pin, small)
    return InputFact(pin.label, pin.length, observed, pin.inspection), bytes(small) if pin.inspection == "UAPI_BYTE_IDENTITY" else None


def _observe(pin, uid):
    reader = _Read(uid)
    primary, post, cleanup = Reason.NONE, Reason.NONE, Reason.NONE
    result = None
    try:
        reader.open(pin)
        result = _stream(reader, pin)
    except BaseException as error:
        primary = error.primary_code if type(error) is InputsError else Reason.HOST_IO
    finally:
        try:
            post = reader.postcheck()
        except BaseException:
            post = Reason.METADATA
        cleanup = reader.close()
    if primary is not Reason.NONE or post is not Reason.NONE or cleanup is not Reason.NONE:
        raise InputsError(primary if primary is not Reason.NONE else post, post, cleanup) from None
    return result


def validate_selected_inputs():
    """Future explicit OFFLINE public-byte read only; no arguments or authority."""
    try:
        _require(sys.platform == "linux" and os.open in os.supports_dir_fd
                 and os.stat in os.supports_dir_fd and os.stat in os.supports_follow_symlinks
                 and all(hasattr(os, name) for name in ("O_NOFOLLOW", "O_CLOEXEC", "O_NONBLOCK", "O_DIRECTORY", "pread")), Reason.PLATFORM)
        uid, facts, reference = os.getuid(), [], None
        for pin in _PINS:
            fact, uapi = _observe(pin, uid)  # all that file's descriptors close before return
            facts.append(fact)
            if uapi is not None:
                if reference is None:
                    reference = uapi
                else:
                    _require(uapi == reference)
        return OfflineManifest("OFFLINE_NOT_ADMISSION", tuple(facts), (
            "DESCRIPTORS_CLOSED_NO_FUTURE_FREEZE_OR_LOADED_CODE_ASSOCIATION",
            "NO_BUILD_REPRODUCTION_OR_NATIVE_ABI_OWNER_CLOCK_CATCH_RECOVERY_PROOF",
            "STATIC_RANGES_FROM_PINNED_PACKAGE_NOT_FRESH_DECOMPRESSION_OR_MEMORY_FIT",
            "ARMV5TE_AND_QUERY88_MEANING_TIED_TO_FROZEN_PINS_NOT_FRESH_ATTRIBUTE_OR_C_COMPILE",
            "EMBEDDED_STATION76_UNPAIRED_UNSUPPORTED_FORBIDDEN_TO_LOAD_OR_AUTOLOAD",
        ))
    except BaseException as error:
        if type(error) is InputsError:
            raise InputsError(error.primary_code, error.postcheck_code, error.cleanup_code) from None
        raise InputsError(Reason.HOST_IO) from None
