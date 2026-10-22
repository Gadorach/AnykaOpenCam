#!/usr/bin/env python3
"""One22b operation lock/HOLD, without clearing the original failed radio HOLD."""
from contextlib import contextmanager
from pathlib import Path
import fcntl
import json
import os
import stat
import sys

ROOT = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ROOT / 'scripts'))
import qc07f_boot as b

EXPECTED = {'state': 'HOLD', 'operation': 'one_radio_native_load_RUN21a'}


@contextmanager
def operation(name):
    """Clear only this new subordinate marker after a positively completed phase.

    Any exception leaves its new marker sticky. The original global marker is
    locked and compared byte-for-byte but never written, truncated or cleared.
    """
    os.umask(0o077)
    path = b.CONTROL / 'peripheral-unattended22b-operation.lock'
    with (b.CONTROL / 'peripheral-gui-operation.lock').open('r+') as original:
        fcntl.flock(original, fcntl.LOCK_EX | fcntl.LOCK_NB)
        old = original.read()
        if json.loads(old) != EXPECTED:
            raise b.Refused('Original failed radio HOLD changed')
        fd = os.open(path, os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
        with os.fdopen(fd, 'r+') as subordinate:
            metadata = os.fstat(subordinate.fileno())
            if (not stat.S_ISREG(metadata.st_mode) or metadata.st_uid != os.getuid() or
                    metadata.st_nlink != 1 or stat.S_IMODE(metadata.st_mode) != 0o600):
                raise b.Refused('Subordinate operation file metadata refused')
            fcntl.flock(subordinate, fcntl.LOCK_EX | fcntl.LOCK_NB)
            if subordinate.read():
                raise b.Refused('Prior22b operation HOLD remains; no automatic retry')
            subordinate.write(json.dumps({'state': 'HOLD', 'operation': name}) + '\n')
            subordinate.flush()
            os.fsync(subordinate.fileno())
            try:
                yield
            finally:
                original.seek(0)
                if original.read() != old:
                    raise b.Refused('Original failed radio HOLD modified during operation')
            # Not reached on any exception, timeout, lost reply or unknown outcome.
            subordinate.seek(0)
            subordinate.truncate()
            subordinate.flush()
            os.fsync(subordinate.fileno())
