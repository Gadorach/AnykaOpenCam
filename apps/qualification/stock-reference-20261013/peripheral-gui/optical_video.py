"""Opt-in RTSP/TCP -> bounded in-memory PNG preview; no camera control code."""
from dataclasses import dataclass
import ipaddress
import os
import selectors
import struct
import subprocess
import threading
import time
import uuid
import zlib

WIDTH, HEIGHT = 640, 360
MAX_PNG = 768 * 1024
READ_SIZE = 16384
STALE_SECONDS = 3.0
STARTUP_SECONDS = 12.0
FFMPEG = "/usr/bin/ffmpeg"
SIGNATURE = b"\x89PNG\r\n\x1a\n"


class VideoError(ValueError):
    """Only fixed, non-private messages may leave this module."""


def admitted_host(host, allow_loopback=False):
    if type(host) is not str or len(host) > 15:
        raise VideoError("Enter a numeric private IPv4 host only")
    try:
        address = ipaddress.IPv4Address(host)
    except ipaddress.AddressValueError:
        raise VideoError("Enter a numeric private IPv4 host only") from None
    # is_private alone admits link-local, unspecified and reserved addresses.
    private = any(address in ipaddress.IPv4Network(net) for net in
                  ("10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16"))
    if not private and not (allow_loopback and address.is_loopback):
        raise VideoError("Only RFC1918 IPv4 (or explicit loopback mock) is admitted")
    return str(address)


def duration_seconds(value):
    if type(value) is not int or not 1 <= value <= 300:
        raise VideoError("Session duration must be an integer from 1 to 300 seconds")
    return value


def decoder_argv(host, duration, allow_loopback=False):
    host = admitted_host(host, allow_loopback)
    duration_seconds(duration)
    return [FFMPEG, "-nostdin", "-hide_banner", "-loglevel", "error", "-xerror",
            "-max_alloc", "16777216", "-protocol_whitelist", "rtsp,tcp",
            "-rtsp_transport", "tcp", "-timeout", "2000000",
            "-probesize", "1048576", "-analyzeduration", "1000000",
            "-fpsprobesize", "0",
            "-threads", "1", "-i", f"rtsp://{host}:8554/live",
            "-map", "0:v:0", "-an", "-sn", "-dn", "-t", str(duration),
            "-vf", "select='isnan(prev_selected_t)+gte(t-prev_selected_t,0.2)',"
            "scale=640:360:force_original_aspect_ratio=decrease,"
            "pad=640:360:(ow-iw)/2:(oh-ih)/2",
            "-fps_mode", "passthrough",
            "-enc_time_base", "1:90000",
            "-flush_packets", "1",
            "-pix_fmt", "rgb24", "-c:v", "png", "-threads", "1",
            "-f", "image2pipe", "pipe:1"]


class PNGParser:
    """Incremental framing, chunk CRCs and exact output format; never resyncs."""
    def __init__(self):
        self.buffer = bytearray()
        self.offset = 8
        self.idat = False
        self.header = False
        self.inflate = None
        self.decoded = 0
        self.phys = False

    def feed(self, data):
        if len(data) > READ_SIZE or len(self.buffer) + len(data) > MAX_PNG + READ_SIZE:
            raise VideoError("Decoder output exceeded its bound")
        self.buffer.extend(data)
        frames = []
        while True:
            if len(self.buffer) < 8:
                return frames
            if self.buffer[:8] != SIGNATURE:
                raise VideoError("Invalid decoder PNG signature")
            if len(self.buffer) < self.offset + 8:
                return frames
            length, kind = struct.unpack_from(">I4s", self.buffer, self.offset)
            end = self.offset + 12 + length
            if end > MAX_PNG or not kind.isalpha():
                raise VideoError("Invalid decoder PNG chunk")
            if len(self.buffer) < end:
                return frames
            body = bytes(self.buffer[self.offset + 8:end - 4])
            crc = struct.unpack_from(">I", self.buffer, end - 4)[0]
            if zlib.crc32(kind + body) != crc:
                raise VideoError("Invalid decoder PNG checksum")
            if not self.header:
                if kind != b"IHDR" or body != struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0):
                    raise VideoError("Decoder PNG must be 640x360 RGB8")
                self.header = True
                self.inflate = zlib.decompressobj()
            elif kind == b"IHDR":
                raise VideoError("Duplicate decoder PNG header")
            elif kind == b"IDAT":
                self.idat = True
                expected = (WIDTH * 3 + 1) * HEIGHT
                try:
                    pixels = self.inflate.decompress(body, expected - self.decoded + 1)
                except zlib.error:
                    raise VideoError("Invalid compressed decoder PNG") from None
                if (self.decoded + len(pixels) > expected or self.inflate.unconsumed_tail or
                        self.inflate.unused_data):
                    raise VideoError("Decoder PNG pixels exceeded their bound")
                stride = WIDTH * 3 + 1
                first_filter = (-self.decoded) % stride
                if any(pixels[index] > 4 for index in range(first_filter, len(pixels), stride)):
                    raise VideoError("Invalid decoder PNG row filter")
                self.decoded += len(pixels)
            elif kind == b"IEND":
                if (body or not self.idat or not self.inflate.eof or
                        self.decoded != (WIDTH * 3 + 1) * HEIGHT):
                    raise VideoError("Invalid decoder PNG end")
                frames.append(bytes(self.buffer[:end]))
                del self.buffer[:end]
                self.offset, self.idat, self.header = 8, False, False
                self.inflate, self.decoded, self.phys = None, 0, False
                continue
            elif kind == b"pHYs":
                if self.idat or self.phys or len(body) != 9 or body[-1] not in (0, 1):
                    raise VideoError("Invalid decoder PNG pixel geometry")
                self.phys = True
            else:
                raise VideoError("Unexpected decoder PNG chunk")
            self.offset = end

    def finish(self):
        if self.buffer:
            raise VideoError("Truncated decoder PNG")


@dataclass(frozen=True)
class Preview:
    session: str
    ready: bool
    frame: bytes | None
    sequence: int
    reason: str


class Decoder:
    """Single worker, single latest-frame slot. Only our local child is reaped.

    Construction does nothing. start() is the only process/network entry. A
    terminal decoder is not reused; Connect must explicitly construct another.
    No worker calls GUI functions or writes frames/diagnostics to disk.
    """
    def __init__(self, *, popen=subprocess.Popen, clock=time.monotonic):
        self._popen, self._clock = popen, clock
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._thread = None
        self._process = None
        self._active = False
        self._frame = None
        self._sequence = 0
        self._last = None
        self._start = 0.0
        self._duration = 0
        self._reason = "Disconnected"
        self._retired = True
        self.session = uuid.uuid4().hex

    def start(self, host, duration=60, allow_loopback=False):
        argv = decoder_argv(host, duration, allow_loopback)
        with self._lock:
            if self._thread is not None:
                raise VideoError("Decoder sessions cannot reconnect automatically")
            self._active = True
            self._retired = False
            self._duration = duration
            self._start = self._clock()
            self._reason = "Waiting for first decoded frame"
            self._thread = threading.Thread(target=self._run, args=(argv,), daemon=True)
            self._thread.start()

    def _invalidate(self, reason):
        with self._lock:
            self._active = False
            self._frame = None
            self._reason = reason

    def snapshot(self):
        with self._lock:
            now = self._clock()
            alive = self._process is None or self._process.poll() is None
            ready = (self._active and alive and self._last is not None and
                     now - self._last <= STALE_SECONDS and
                     now - self._start < self._duration and not self._stop.is_set())
            reason = self._reason if not self._active else (
                "Live decoded preview" if ready else "No fresh decoded frame")
            return Preview(self.session, ready, self._frame if ready else None,
                           self._sequence, reason)

    def stop(self):
        self._stop.set()
        self._invalidate("Stopped — controls admission invalidated")

    def close(self):
        self.stop()
        if self._thread is not None:
            self._thread.join(4)
            if self._thread.is_alive():
                raise VideoError("Local decoder retirement not verified")
        if not self._retired:
            raise VideoError("Local decoder retirement not verified")

    def _run(self, argv):
        process = None
        reason = "Decoder ended — explicit Connect required"
        try:
            # No inherited FFREPORT, proxy, preload, credential or display variables.
            process = self._popen(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, shell=False, close_fds=True,
                                  bufsize=0, env={"PATH": "/usr/bin:/bin", "LC_ALL": "C"})
            with self._lock:
                self._process = process
            reason = self._pump(process)
        except Exception:
            reason = "Decoder failed — private diagnostics suppressed"
        finally:
            self._invalidate(reason)
            if process is not None:
                try:
                    self._reap(process)
                    self._retired = True
                except Exception:
                    self._invalidate("Local decoder retirement not verified")
            else:
                self._retired = True

    def _pump(self, process):
        parser = PNGParser()
        stderr_count = 0
        frame_count = 0
        with selectors.DefaultSelector() as selector:
            for stream, label in ((process.stdout, "video"), (process.stderr, "error")):
                os.set_blocking(stream.fileno(), False)
                selector.register(stream, selectors.EVENT_READ, label)
            while not self._stop.is_set():
                now = self._clock()
                with self._lock:
                    last = self._last
                if now - self._start >= self._duration:
                    return "Session duration ended"
                if last is None and now - self._start > STARTUP_SECONDS:
                    return "Startup deadline exceeded — no controls admission"
                if last is not None and now - last > STALE_SECONDS:
                    return "Decoder stalled — controls admission invalidated"
                if process.poll() is not None:
                    return "Decoder exited — controls admission invalidated"
                for key, _ in selector.select(0.1):
                    data = os.read(key.fileobj.fileno(), READ_SIZE)
                    if not data:
                        selector.unregister(key.fileobj)
                        if key.data == "video":
                            parser.finish()
                            return "Video ended — controls admission invalidated"
                        continue
                    if key.data == "error":
                        stderr_count += len(data)
                        # Store NO stderr text, not even its tail. Bound chatter too.
                        if stderr_count > 16384:
                            raise VideoError("Decoder diagnostic bound exceeded")
                        raise VideoError("Decoder reported an error")
                    for frame in parser.feed(data):
                        frame_count += 1
                        if frame_count > 5 * self._duration + 5:
                            raise VideoError("Decoder frame bound exceeded")
                        with self._lock:
                            if self._stop.is_set():
                                return "Stopped"
                            self._frame = frame
                            self._last = self._clock()
                            self._sequence += 1
            return "Stopped"

    @staticmethod
    def _reap(process):
        # Never process groups, PID discovery, camera helpers or remote processes.
        try:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=1)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=1)
        finally:
            process.stdout.close()
            process.stderr.close()
