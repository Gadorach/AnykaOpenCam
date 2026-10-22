# SPDX-License-Identifier: GPL-2.0-only
"""Conditional host-local C1 custody. No CLI, live getter, effect or native GO.

Explicit calls can own host regular files/dirs only. Import does not acquire any.
Read C1_OWNERSHIP_CONTRACT.md and README.md before selecting even source use.
"""
from contextlib import contextmanager
from dataclasses import dataclass
from enum import Enum
import fcntl
import hashlib
import json
import os
import re
import stat
import sys


class Code(Enum):
    ARGUMENT = "C1_ARGUMENT"
    PLATFORM = "C1_PLATFORM"
    PATH = "C1_PATH"
    FILE = "C1_FILE"
    HOLD = "C1_HISTORICAL_HOLD"
    SOURCE = "C1_SOURCE"
    EPOCH = "C1_EPOCH"
    CLOCK = "C1_CLOCK"
    BUDGET = "C1_BUDGET"
    CATCH = "C1_CATCH"
    LEDGER = "C1_LEDGER"
    WRITE_UNKNOWN = "C1_WRITE_UNKNOWN"
    ORDER = "C1_ORDER"
    REENTRY = "C1_REENTRY"
    TERMINAL = "C1_TERMINAL"
    CLEANUP = "C1_CLEANUP"
    IO = "C1_HOST_IO"


class C1Error(Exception):
    """Only fixed codes are public. No OS/path/raw/digest exception formatting."""
    def __init__(self, code):
        selected = code if type(code) is Code else Code.ARGUMENT
        self.code = selected
        super().__init__(selected.value)


def _refuse(code):
    raise C1Error(code) from None


def _code(error):
    return error.code if type(error) is C1Error else Code.IO


def _integer(value, low=0, high=9223372036854775807):
    if type(value) is not int or not low <= value <= high:
        _refuse(Code.ARGUMENT)
    return value


def _text(value, pattern):
    if type(value) is not str or len(value) > 1024 or re.fullmatch(pattern, value) is None:
        _refuse(Code.ARGUMENT)
    return value


def _digest(value):
    return _text(value, r"[0-9a-f]{64}")


class Catch(Enum):
    ARMED = "ARMED"
    CONSUMED = "CONSUMED"
    UNARMED = "UNARMED"


class CatchRequirement(Enum):
    REQUIRED = "REQUIRED"
    BOOKKEEPING_ONLY = "BOOKKEEPING_ONLY"


class SourceSlot(Enum):
    LIBRARY = "LIBRARY_SOURCE"
    PROFILE = "PROFILE_SOURCE"


class LocalPhase(Enum):
    SOURCE_PINS = "C1_SOURCE_PINS"
    EPOCH_RECORDS = "C1_EPOCH_RECORDS"


@dataclass(frozen=True, slots=True, repr=False)
class Profile:
    """Caller-sealed reviewed constants, NOT authenticity/authorization evidence."""
    library_source_sha256: str
    profile_source_sha256: str
    design_sha256: str = "74e936bf975ce6d2d257c0551428819f37eec7fca9d0a9c1bac642b1d8df1192"
    image_sha256: str = "f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e"
    recorder_source_sha256: str = "135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00"
    release: str = "6.18.55-qc07f-encoder-query77"

    def __post_init__(self):
        _digest(self.library_source_sha256)
        _digest(self.profile_source_sha256)
        _digest(self.design_sha256)
        _digest(self.image_sha256)
        _digest(self.recorder_source_sha256)
        _text(self.release, r"6\.18\.55-qc07f-encoder-query77")
        if (self.design_sha256 != "74e936bf975ce6d2d257c0551428819f37eec7fca9d0a9c1bac642b1d8df1192" or
                self.image_sha256 != "f7c1996073644d8cd6c5c7ba1d2a8c2b71d2d6df4fb4697d01ad59a1e7525a5e" or
                self.recorder_source_sha256 != "135a694bf79a1e46d07f089c2256d40ce8c584780c2fd8fa9d7f8b8137a29d00" or
                self.release != "6.18.55-qc07f-encoder-query77"):
            _refuse(Code.ARGUMENT)


@dataclass(frozen=True, slots=True, repr=False)
class Epoch:
    profile: Profile
    capture_part: str
    peer_pid: int
    peer_uid: int
    peer_gid: int
    pid_start_ticks: int
    adapter_identity: str
    raw_dev: int
    raw_ino: int
    boot_id: str
    runtime: str
    catch_identity: str
    catch_state: Catch
    mono_ns: int
    boottime_ns: int
    wall_ns: int
    expiry_wall_ns: int
    lifetime_remaining_ns: int
    rx_bytes: int
    tx_bytes: int

    def __post_init__(self):
        if type(self.profile) is not Profile:
            _refuse(Code.ARGUMENT)
        _text(self.capture_part, r"foundation-[0-9]{8}[a-z]")
        _integer(self.peer_pid, 1, 4194304)
        _integer(self.peer_uid, 0, 4294967295)
        _integer(self.peer_gid, 0, 4294967295)
        _integer(self.pid_start_ticks, 1)
        _text(self.adapter_identity, r"[A-Za-z0-9_./:-]{1,256}")
        _integer(self.raw_dev)
        _integer(self.raw_ino, 1)
        _text(self.runtime, r"HELD_UBOOT|LINUX77")
        _text(self.boot_id, r"(?:[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12})?")
        if self.runtime == "HELD_UBOOT":
            if self.boot_id != "":
                _refuse(Code.ARGUMENT)
        elif self.runtime == "LINUX77":
            _text(self.boot_id, r"[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}")
        else:
            _refuse(Code.ARGUMENT)
        _text(self.catch_identity, r"[0-9a-f]{32}")
        if type(self.catch_state) is not Catch:
            _refuse(Code.ARGUMENT)
        for value in (self.mono_ns, self.boottime_ns, self.wall_ns,
                      self.expiry_wall_ns, self.rx_bytes, self.tx_bytes):
            _integer(value)
        _integer(self.lifetime_remaining_ns, 1, 14400000000000)
        if self.expiry_wall_ns <= self.wall_ns:
            _refuse(Code.BUDGET)
        remaining = min(self.expiry_wall_ns - self.wall_ns, self.lifetime_remaining_ns)
        _integer(self.mono_ns + remaining)


@dataclass(frozen=True, slots=True, repr=False)
class Observation:
    epoch: Epoch
    mono_ns: int
    boottime_ns: int
    wall_ns: int
    rx_bytes: int
    tx_bytes: int
    catch_state: Catch
    remaining_cap_ns: int

    def __post_init__(self):
        if type(self.epoch) is not Epoch or type(self.catch_state) is not Catch:
            _refuse(Code.ARGUMENT)
        for value in (self.mono_ns, self.boottime_ns, self.wall_ns,
                      self.rx_bytes, self.tx_bytes, self.remaining_cap_ns):
            _integer(value)


@dataclass(frozen=True, slots=True, repr=False)
class Budget:
    """Constrained record arithmetic only; not a measured or renewable lease."""
    deadline_mono_ns: int
    remaining_ns: int

    def __post_init__(self):
        _integer(self.deadline_mono_ns)
        _integer(self.remaining_ns)


def validate_observation(epoch, previous, observation, selected_cost_ns,
                         catch_requirement=CatchRequirement.REQUIRED):
    """Pure supplied-record validation. No OS clock/PID/capture/status access.

    Exact equal clock deltas deliberately fail closed: no real sampler/tolerance
    is supplied here. Repeated evidence cannot refresh the original deadline.
    """
    if (type(epoch) is not Epoch or type(observation) is not Observation or
            type(catch_requirement) is not CatchRequirement or
            (previous is not None and type(previous) is not Observation)):
        _refuse(Code.ARGUMENT)
    _integer(selected_cost_ns, 0, 14400000000000)
    if observation.epoch != epoch or (previous is not None and previous.epoch != epoch):
        _refuse(Code.EPOCH)
    initial = min(epoch.expiry_wall_ns - epoch.wall_ns, epoch.lifetime_remaining_ns)
    base_mono = epoch.mono_ns if previous is None else previous.mono_ns
    base_boot = epoch.boottime_ns if previous is None else previous.boottime_ns
    base_wall = epoch.wall_ns if previous is None else previous.wall_ns
    for anchor in (None, previous):
        if anchor is None:
            mono, boot, wall = epoch.mono_ns, epoch.boottime_ns, epoch.wall_ns
        else:
            mono, boot, wall = anchor.mono_ns, anchor.boottime_ns, anchor.wall_ns
        delta = observation.mono_ns - mono
        if (delta < 0 or observation.boottime_ns - boot != delta or
                observation.wall_ns - wall != delta):
            _refuse(Code.CLOCK)
    if observation.mono_ns < base_mono or observation.boottime_ns < base_boot or observation.wall_ns < base_wall:
        _refuse(Code.CLOCK)
    old_rx = epoch.rx_bytes if previous is None else previous.rx_bytes
    old_tx = epoch.tx_bytes if previous is None else previous.tx_bytes
    if observation.rx_bytes < old_rx or observation.tx_bytes < old_tx:
        _refuse(Code.EPOCH)
    old_catch = epoch.catch_state if previous is None else previous.catch_state
    if old_catch is not Catch.ARMED and observation.catch_state is not old_catch:
        _refuse(Code.CATCH)
    if catch_requirement is CatchRequirement.REQUIRED and observation.catch_state is not Catch.ARMED:
        _refuse(Code.CATCH)
    old_cap = initial if previous is None else previous.remaining_cap_ns
    elapsed = observation.mono_ns - epoch.mono_ns
    natural = initial - elapsed
    # A lowered cap is a fixed earlier deadline, not a fresh duration each time.
    old_deadline_remaining = natural if previous is None else previous.remaining_cap_ns - (observation.mono_ns - previous.mono_ns)
    if not 0 <= observation.remaining_cap_ns <= min(old_cap, natural, old_deadline_remaining):
        _refuse(Code.BUDGET)
    remaining = observation.remaining_cap_ns
    if remaining < 150000000000 + selected_cost_ns:
        _refuse(Code.BUDGET)
    return Budget(observation.mono_ns + remaining, remaining)


def _metadata(value):
    return (value.st_dev, value.st_ino, value.st_mode, value.st_uid, value.st_gid,
            value.st_nlink, value.st_size, value.st_mtime_ns, value.st_ctime_ns)


def _association(value):
    # Directory timestamps/link counts legitimately change on our exclusive mkdir.
    return value.st_dev, value.st_ino, value.st_mode, value.st_uid, value.st_gid


def _canonical(value):
    if (type(value) is not str or not 1 < len(value) <= 1024 or
            not value.startswith("/") or value.endswith("/")):
        _refuse(Code.PATH)
    components = value[1:].split("/")
    if not 1 <= len(components) <= 32:
        _refuse(Code.PATH)
    for component in components:
        if component in (".", "..", ".git") or re.fullmatch(r"[A-Za-z0-9_.-]{1,128}", component) is None:
            _refuse(Code.PATH)
    return components


def _platform():
    if (sys.platform != "linux" or any(not hasattr(os, name) for name in
            ("O_NOFOLLOW", "O_CLOEXEC", "O_NONBLOCK", "O_DIRECTORY", "pread")) or
            os.open not in os.supports_dir_fd or os.stat not in os.supports_dir_fd or
            os.mkdir not in os.supports_dir_fd or os.stat not in os.supports_follow_symlinks):
        _refuse(Code.PLATFORM)


def _git_absent(fd):
    try:
        os.stat(".git", dir_fd=fd, follow_symlinks=False)
    except FileNotFoundError:
        return
    _refuse(Code.PATH)


class _Root:
    def __init__(self):
        self.nodes = []

    def acquire(self, value, uid):
        components = _canonical(value)
        flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK
        fd = os.open("/", flags)
        self.nodes.append([fd, "", None])
        self.nodes[-1][2] = _association(os.fstat(fd))
        self.uid = uid
        for component in components:
            self.check(False)
            child = os.open(component, flags, dir_fd=self.nodes[-1][0])
            self.nodes.append([child, component, None])
            self.nodes[-1][2] = _association(os.fstat(child))
        self.check(True)

    def check(self, selected=True):
        if not self.nodes:
            _refuse(Code.PATH)
        for index, (fd, name, original) in enumerate(self.nodes):
            value = os.fstat(fd)
            path = os.stat("/", follow_symlinks=False) if index == 0 else os.stat(name, dir_fd=self.nodes[index - 1][0], follow_symlinks=False)
            if (not stat.S_ISDIR(value.st_mode) or value.st_uid not in (0, self.uid) or
                    value.st_mode & 0o022 or _association(value) != original or
                    _association(path) != original):
                _refuse(Code.PATH)
            _git_absent(fd)
        last = os.fstat(self.nodes[-1][0])
        if selected and (last.st_uid != self.uid or stat.S_IMODE(last.st_mode) != 0o700):
            _refuse(Code.PATH)

    @property
    def fd(self):
        if not self.nodes:
            _refuse(Code.PATH)
        return self.nodes[-1][0]

    def close(self):
        failed = False
        while self.nodes:
            fd, _, _ = self.nodes.pop()  # clear ownership BEFORE exactly one close
            try:
                os.close(fd)
            except BaseException:
                failed = True
        return failed


def _regular(fd, uid, private=True):
    value = os.fstat(fd)
    if (not stat.S_ISREG(value.st_mode) or value.st_nlink != 1 or
            (private and (value.st_uid != uid or stat.S_IMODE(value.st_mode) != 0o600))):
        _refuse(Code.FILE)
    return value


def _pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            _refuse(Code.FILE)
        result[key] = value
    return result


def _json(data):
    return json.loads(data.decode("utf-8"), object_pairs_hook=_pairs)


def _encoded(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True, allow_nan=False).encode("ascii")


def _observation_fields(observation):
    if observation is None:
        return None
    return {"mono": observation.mono_ns, "boot": observation.boottime_ns,
            "wall": observation.wall_ns, "rx": observation.rx_bytes,
            "tx": observation.tx_bytes, "cap": observation.remaining_cap_ns,
            "catch": observation.catch_state.value}


def _read(fd, size):
    _integer(size, 0, 4096)
    data = os.pread(fd, size + 1, 0)
    if len(data) != size:
        _refuse(Code.FILE)
    return data


class Custody:
    """One-shot host custody. Explicit close required; no destructor or CLI.

    Callers must serialize lifetime and exclude descriptor/mount/namespace races.
    No authorization, target FD retention, process supervision or native receipt.
    """
    def __init__(self):
        _refuse(Code.ARGUMENT)

    @classmethod
    def open(cls, control_root, private_root, operation_name, profile, epoch):
        if cls is not Custody:
            _refuse(Code.ARGUMENT)
        self = object.__new__(cls)
        self._control, self._private = _Root(), _Root()
        self._holds = []
        # Preallocate source slots before any descriptor acquisition; successful
        # dup installs directly into an existing slot, no post-dup dict growth.
        self._sources = {SourceSlot.LIBRARY: [None, None, None],
                         SourceSlot.PROFILE: [None, None, None]}
        self._op = self._marker = self._ledger = None
        self._seq, self._ledger_bytes, self._tip = 0, 0, "0" * 64
        self._first_failure, self._busy = None, False
        self._poisoned, self._closed, self._complete = False, False, False
        self._pending, self._done = None, set()
        try:
            _platform()
            if type(profile) is not Profile or type(epoch) is not Epoch or epoch.profile != profile:
                _refuse(Code.ARGUMENT)
            _text(operation_name, r"[a-z][a-z0-9-]{0,47}")
            self._uid = os.getuid()
            self._profile, self._epoch, self._last = profile, epoch, None
            initial_cap = min(epoch.expiry_wall_ns - epoch.wall_ns, epoch.lifetime_remaining_ns)
            initial = Observation(epoch, epoch.mono_ns, epoch.boottime_ns, epoch.wall_ns,
                                  epoch.rx_bytes, epoch.tx_bytes, epoch.catch_state, initial_cap)
            validate_observation(epoch, None, initial, 0)
            self._control.acquire(control_root, self._uid)
            self._private.acquire(private_root, self._uid)
            if _association(os.fstat(self._control.fd)) == _association(os.fstat(self._private.fd)):
                _refuse(Code.PATH)
            self._name = operation_name
            for name, operation in (
                    ("peripheral-gui-operation.lock", "one_radio_native_load_RUN21a"),
                    ("peripheral-unattended22b-operation.lock", "one_idle_watchdog_reset22b_stop_at_uboot")):
                # Precheck known nonregular entries before open. Atomic exclusion
                # of same-uid namespace replacement is a caller serialization
                # prerequisite, not a property Linux O_NOFOLLOW alone supplies.
                path = os.stat(name, dir_fd=self._control.fd, follow_symlinks=False)
                if (not stat.S_ISREG(path.st_mode) or path.st_uid != self._uid or
                        path.st_nlink != 1 or stat.S_IMODE(path.st_mode) != 0o600):
                    _refuse(Code.FILE)
                fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK, dir_fd=self._control.fd)
                item = [fd, name, None, None]
                self._holds.append(item)
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                value = _regular(fd, self._uid)
                if _metadata(value) != _metadata(path) or not 1 <= value.st_size <= 4096:
                    _refuse(Code.HOLD)
                raw = _read(fd, value.st_size)
                if _json(raw) != {"state": "HOLD", "operation": operation}:
                    _refuse(Code.HOLD)
                item[2], item[3] = _metadata(value), raw
                self._check_holds()
            self._check_roots()
            os.mkdir(operation_name, 0o700, dir_fd=self._private.fd)
            self._op = os.open(operation_name, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK, dir_fd=self._private.fd)
            self._op_identity = _association(os.fstat(self._op))
            self._check_op()
            # Exclusive mkdir persisted before creating any new regular file.
            os.fsync(self._private.fd)
            flags = os.O_RDWR | os.O_APPEND | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK
            self._marker = os.open("marker.json", flags, 0o600, dir_fd=self._op)
            self._marker_identity = _association(_regular(self._marker, self._uid))
            self._ledger = os.open("ledger.jsonl", flags, 0o600, dir_fd=self._op)
            self._ledger_identity = _association(_regular(self._ledger, self._uid))
            raw = _encoded({"state": "IN_FLIGHT_UNKNOWN/HOLD", "scope": "C1_LOCAL_NOT_NATIVE",
                            "design": profile.design_sha256, "image": profile.image_sha256,
                            "library": profile.library_source_sha256, "profile": profile.profile_source_sha256,
                            "recorder_source": profile.recorder_source_sha256,
                            "epoch": {"part": epoch.capture_part, "peer": [epoch.peer_pid, epoch.peer_uid, epoch.peer_gid],
                                      "pid_start": epoch.pid_start_ticks, "adapter": epoch.adapter_identity,
                                      "raw_identity": [epoch.raw_dev, epoch.raw_ino], "boot_id": epoch.boot_id,
                                      "runtime": epoch.runtime, "release": profile.release,
                                      "catch_identity": epoch.catch_identity, "expiry_wall": epoch.expiry_wall_ns,
                                      "lifetime_remaining": epoch.lifetime_remaining_ns,
                                      "initial_observation": _observation_fields(initial)},
                            "historical": [{"metadata": row[2], "raw_hex": row[3].hex(),
                                            "sha256": hashlib.sha256(row[3]).hexdigest()} for row in self._holds]}) + b"\n"
            if len(raw) > 24576:
                _refuse(Code.FILE)
            self._write_once(self._marker, raw)
            os.fsync(self._ledger)
            os.fsync(self._op)
            os.fsync(self._private.fd)
            self._marker_raw = raw
            self._marker_metadata = _metadata(os.fstat(self._marker))
            self._ledger_metadata = _metadata(os.fstat(self._ledger))
            self._check_all()
            self._append("START", "", "", "")
            self._check_all()
            return self
        except BaseException as error:
            selected = _code(error)
            self._failure(selected, error)
            if self._teardown():
                selected = Code.CLEANUP
            raise C1Error(selected) from None

    def _check_roots(self):
        self._control.check()
        self._private.check()

    def _check_holds(self):
        self._control.check()
        for fd, name, original, raw in self._holds:
            if original is None:
                _refuse(Code.HOLD)
            before = _regular(fd, self._uid)
            path = os.stat(name, dir_fd=self._control.fd, follow_symlinks=False)
            if _metadata(before) != original or _metadata(path) != original:
                _refuse(Code.HOLD)
            if _read(fd, before.st_size) != raw or _metadata(os.fstat(fd)) != original:
                _refuse(Code.HOLD)
            if _metadata(os.stat(name, dir_fd=self._control.fd, follow_symlinks=False)) != original:
                _refuse(Code.HOLD)

    def _check_op(self):
        self._private.check()
        value = os.fstat(self._op)
        path = os.stat(self._name, dir_fd=self._private.fd, follow_symlinks=False)
        if (not stat.S_ISDIR(value.st_mode) or value.st_uid != self._uid or
                stat.S_IMODE(value.st_mode) != 0o700 or
                _association(value) != self._op_identity or _association(path) != self._op_identity):
            _refuse(Code.PATH)
        _git_absent(self._op)

    def _private_file(self, fd, name, identity):
        value = _regular(fd, self._uid)
        path = os.stat(name, dir_fd=self._op, follow_symlinks=False)
        if _association(value) != identity or _metadata(value) != _metadata(path):
            _refuse(Code.FILE)
        return value

    def _check_private(self):
        self._check_op()
        marker = self._private_file(self._marker, "marker.json", self._marker_identity)
        if _metadata(marker) != self._marker_metadata or marker.st_size > 24576:
            _refuse(Code.FILE)
        # Marker is bounded separately from 4KiB historical marker reads.
        data = os.pread(self._marker, marker.st_size + 1, 0)
        if data != self._marker_raw or _metadata(os.fstat(self._marker)) != self._marker_metadata:
            _refuse(Code.FILE)
        self._private_file(self._marker, "marker.json", self._marker_identity)
        ledger = self._private_file(self._ledger, "ledger.jsonl", self._ledger_identity)
        if (ledger.st_size != self._ledger_bytes or ledger.st_size > 65536 or
                _metadata(ledger) != self._ledger_metadata):
            _refuse(Code.LEDGER)
        tip, offset, seq = "0" * 64, 0, 0
        while offset < ledger.st_size:
            chunk = os.pread(self._ledger, min(4096, ledger.st_size - offset), offset)
            if not chunk or b"\n" not in chunk:
                _refuse(Code.LEDGER)
            line = chunk.split(b"\n", 1)[0]
            row = _json(line)
            if type(row) is not dict or set(row) != {"seq", "prev", "event", "phase", "receipt", "code", "hash", "observation", "private_error"}:
                _refuse(Code.LEDGER)
            _integer(row["seq"], 1, 64)
            for key in ("prev", "hash"):
                _digest(row[key])
            for key in ("event", "phase", "receipt", "code"):
                _text(row[key], r"[A-Za-z0-9_]{0,64}")
            observed = row["observation"]
            if observed is not None:
                if type(observed) is not dict or set(observed) != {"mono", "boot", "wall", "rx", "tx", "cap", "catch"}:
                    _refuse(Code.LEDGER)
                for key in ("mono", "boot", "wall", "rx", "tx", "cap"):
                    _integer(observed[key])
                _text(observed["catch"], r"ARMED|CONSUMED|UNARMED")
            diagnostic = row["private_error"]
            if diagnostic is not None:
                if type(diagnostic) is not dict or set(diagnostic) != {"type", "errno", "detail", "truncated"}:
                    _refuse(Code.LEDGER)
                _text(diagnostic["type"], r"[A-Za-z]{1,40}")
                if diagnostic["errno"] is not None:
                    _integer(diagnostic["errno"], -2147483648, 2147483647)
                if type(diagnostic["detail"]) is not str or len(diagnostic["detail"]) > 256 or type(diagnostic["truncated"]) is not bool:
                    _refuse(Code.LEDGER)
            claimed = row.pop("hash")
            if (row["seq"] != seq + 1 or row["prev"] != tip or
                    hashlib.sha256(_encoded(row)).hexdigest() != claimed):
                _refuse(Code.LEDGER)
            tip, seq = claimed, seq + 1
            offset += len(line) + 1
        if seq != self._seq or tip != self._tip or _metadata(os.fstat(self._ledger)) != _metadata(ledger):
            _refuse(Code.LEDGER)
        self._private_file(self._ledger, "ledger.jsonl", self._ledger_identity)

    def _source_hash(self, fd, expected, original=None):
        value = _regular(fd, self._uid, private=False)
        flags = fcntl.fcntl(fd, fcntl.F_GETFL)
        if (flags & os.O_ACCMODE != os.O_RDONLY or not 1 <= value.st_size <= 65536 or
                os.lseek(fd, 0, os.SEEK_CUR) != 0 or
                (original is not None and _metadata(value) != original)):
            _refuse(Code.SOURCE)
        digest = hashlib.sha256()
        offset = 0
        while offset < value.st_size:
            chunk = os.pread(fd, min(4096, value.st_size - offset), offset)
            if not chunk:
                _refuse(Code.SOURCE)
            offset += len(chunk)
            digest.update(chunk)
        if digest.hexdigest() != expected or _metadata(os.fstat(fd)) != _metadata(value):
            _refuse(Code.SOURCE)
        return _metadata(value)

    def _check_all(self):
        self._check_roots()
        self._check_holds()
        self._check_private()
        for fd, expected, metadata in self._sources.values():
            if fd is not None:
                self._source_hash(fd, expected, metadata)
        self._check_holds()
        self._check_roots()

    def _write_once(self, fd, data):
        # Any short write/exception/fsync uncertainty poisons further appends.
        self._poisoned = True
        if os.write(fd, data) != len(data):
            _refuse(Code.WRITE_UNKNOWN)
        os.fsync(fd)
        self._poisoned = False

    def _append(self, event, phase, receipt, code, observation=None, private_error=None):
        if self._poisoned or self._seq >= 64:
            _refuse(Code.LEDGER)
        self._check_private()
        body = {"seq": self._seq + 1, "prev": self._tip, "event": event,
                "phase": phase, "receipt": receipt, "code": code,
                "observation": _observation_fields(observation), "private_error": private_error}
        tip = hashlib.sha256(_encoded(body)).hexdigest()
        data = _encoded({**body, "hash": tip}) + b"\n"
        if len(data) > 4096 or self._ledger_bytes + len(data) > 65536:
            _refuse(Code.LEDGER)
        self._write_once(self._ledger, data)
        self._seq += 1
        self._ledger_bytes += len(data)
        self._tip = tip
        self._ledger_metadata = _metadata(os.fstat(self._ledger))
        # A directory fsync failure also forbids any subsequent append attempt.
        self._poisoned = True
        os.fsync(self._op)
        self._check_private()
        self._poisoned = False

    def _failure(self, code, error=None):
        if self._first_failure is not None:
            return
        self._first_failure = code
        if self._ledger is None or self._poisoned or self._closed:
            return
        try:
            self._check_private()  # no diagnostic formatting before safe ownership
            diagnostic = None
            # Exact builtin types only: never invoke caller-supplied __str__/repr.
            if type(error) in (OSError, FileNotFoundError, PermissionError, FileExistsError,
                               NotADirectoryError, IsADirectoryError, BlockingIOError,
                               InterruptedError, UnicodeDecodeError, json.JSONDecodeError,
                               ValueError, TypeError, AttributeError):
                detail = str(error)
                number = error.errno if type(error) in (OSError, FileNotFoundError, PermissionError,
                          FileExistsError, NotADirectoryError, IsADirectoryError, BlockingIOError,
                          InterruptedError) else None
                diagnostic = {"type": type(error).__name__, "errno": number,
                              "detail": detail[:256], "truncated": len(detail) > 256}
            self._append("FIRST_FAILURE", "", "", code.value, private_error=diagnostic)
        except BaseException:
            self._poisoned = True

    @contextmanager
    def _call(self):
        if self._busy:
            if self._first_failure is None:
                self._first_failure = Code.REENTRY
            _refuse(Code.REENTRY)
        if self._closed or self._complete or self._first_failure is not None:
            _refuse(Code.TERMINAL)
        self._busy = True
        try:
            self._check_all()
            yield
            if self._first_failure is not None:
                _refuse(self._first_failure)
            self._check_all()
        except BaseException as error:
            selected = _code(error)
            self._failure(selected, error)
            raise C1Error(selected) from None
        finally:
            self._busy = False

    def verify_source_fd(self, slot, fd):
        """Borrow a regular O_RDONLY start-position FD, duplicate host custody only.

        Caller must independently bind FD to reviewed public source and serialize
        it through this call. C1 cannot infer its path, provenance or executing code.
        """
        with self._call():
            if type(slot) is not SourceSlot or self._sources[slot][0] is not None:
                _refuse(Code.ORDER)
            _integer(fd, 0, 1048576)
            expected = self._profile.library_source_sha256 if slot is SourceSlot.LIBRARY else self._profile.profile_source_sha256
            metadata = self._source_hash(fd, expected)
            row = self._sources[slot]
            row[1], row[2] = expected, metadata
            row[0] = fcntl.fcntl(fd, fcntl.F_DUPFD_CLOEXEC, 0)
            self._source_hash(row[0], expected, metadata)
            self._append("SOURCE_PIN", slot.value, expected, "")

    def _sources_complete(self):
        return all(row[0] is not None for row in self._sources.values())

    def observe(self, observation, selected_cost_ns, external_receipt_digest):
        """Record arithmetic only; all guard calls require the original armed catch."""
        with self._call():
            _digest(external_receipt_digest)
            budget = validate_observation(self._epoch, self._last, observation, selected_cost_ns)
            self._append("OBSERVATION", "C1_EPOCH_RECORDS", external_receipt_digest, "", observation)
            self._last = observation
            return budget

    def begin_local(self, phase, external_receipt_digest):
        with self._call():
            if type(phase) is not LocalPhase or self._pending is not None or phase in self._done:
                _refuse(Code.ORDER)
            _digest(external_receipt_digest)
            self._append("LOCAL_INTENT", phase.value, external_receipt_digest, "")
            self._pending = (phase, external_receipt_digest)

    def finish_local(self, phase, external_receipt_digest):
        with self._call():
            _digest(external_receipt_digest)
            if type(phase) is not LocalPhase or self._pending != (phase, external_receipt_digest):
                _refuse(Code.ORDER)
            if phase is LocalPhase.SOURCE_PINS and not self._sources_complete():
                _refuse(Code.SOURCE)
            if phase is LocalPhase.EPOCH_RECORDS and self._last is None:
                _refuse(Code.EPOCH)
            self._append("LOCAL_COMPLETION_NOT_NATIVE", phase.value, external_receipt_digest, "")
            self._done.add(phase)
            self._pending = None

    def complete_local(self, observation, selected_cost_ns, external_receipt_digest):
        with self._call():
            if self._pending is not None or self._done != set(LocalPhase) or not self._sources_complete() or self._last is None:
                _refuse(Code.ORDER)
            _digest(external_receipt_digest)
            validate_observation(self._epoch, self._last, observation, selected_cost_ns)
            self._append("C1_LOCAL_COMPLETE_NOT_NATIVE", "", external_receipt_digest, "", observation)
            self._last = observation
            self._complete = True

    def _teardown(self):
        failed = False
        # Only host regular files/dirs ever enter these owned slots.
        for row in self._sources.values():
            fd = row[0]
            row[0] = None
            if fd is not None:
                try:
                    os.close(fd)
                except BaseException:
                    failed = True
        while self._holds:
            fd, _, _, _ = self._holds.pop()
            try:
                os.close(fd)
            except BaseException:
                failed = True
        for name in ("_ledger", "_marker", "_op"):
            fd = getattr(self, name)
            setattr(self, name, None)
            if fd is not None:
                try:
                    os.close(fd)
                except BaseException:
                    failed = True
        failed = self._private.close() or failed
        failed = self._control.close() or failed
        self._closed = True
        if failed and self._first_failure is None:
            self._first_failure = Code.CLEANUP
        return failed

    def close(self):
        """One-shot host teardown, including failed/completed objects; no retry."""
        if self._busy:
            if self._first_failure is None:
                self._first_failure = Code.REENTRY
            _refuse(Code.REENTRY)
        if self._closed:
            _refuse(Code.TERMINAL)
        self._busy = True
        selected = self._first_failure
        try:
            self._check_all()
            if not self._complete:
                self._failure(selected or Code.TERMINAL)
                selected = self._first_failure
        except BaseException as error:
            selected = selected or _code(error)
            self._failure(selected, error)
        finally:
            cleanup = self._teardown()
            self._busy = False
        if cleanup:
            _refuse(Code.CLEANUP)
        if selected is not None:
            _refuse(selected)
