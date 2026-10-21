"""Offline fixtures only: no FFmpeg execution, sockets, desktop or private files."""
import struct
import subprocess
import threading
import unittest
from unittest.mock import Mock, patch
import zlib

from optical_video import (Decoder, MAX_PNG, PNGParser, READ_SIZE, SIGNATURE,
                           VideoError, admitted_host, decoder_argv, duration_seconds)


def chunk(kind, body):
    return (struct.pack(">I", len(body)) + kind + body +
            struct.pack(">I", zlib.crc32(kind + body)))


def png(value=0, *, pixels=None):
    raw = pixels if pixels is not None else b"\x00" + bytes([value]) * (640 * 3)
    if pixels is None:
        raw *= 360
    return (SIGNATURE + chunk(b"IHDR", struct.pack(">IIBBBBB", 640, 360, 8, 2, 0, 0, 0)) +
            chunk(b"pHYs", struct.pack(">IIB", 1, 1, 0)) +
            chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


class Clock:
    def __init__(self, now=100.0):
        self.now = now

    def __call__(self):
        return self.now


def fake_process():
    proc = Mock()
    proc.poll.return_value = None
    proc.stdout.fileno.return_value = 11
    proc.stderr.fileno.return_value = 12
    return proc


class VideoTests(unittest.TestCase):
    def test_host_exact_private_and_opt_in_loopback(self):
        for host in ("10.1.2.3", "172.16.0.1", "172.31.255.254", "192.168.12.2"):
            self.assertEqual(admitted_host(host), host)
        for host in ("", "camera", "localhost", "127.0.0.1", "169.254.1.1", "0.0.0.0",
                     "192.0.0.1", "172.32.0.1", "8.8.8.8", "224.0.0.1", "::1",
                     "10.1.2.03", "10.1.2.3 ", "10.1.2.3:8554", "u:p@10.1.2.3",
                     "10.1.2.3/live?x", "rtsp://10.1.2.3", None, 123):
            with self.subTest(host=host), self.assertRaises(VideoError):
                admitted_host(host)
        self.assertEqual(admitted_host("127.0.0.1", True), "127.0.0.1")

    def test_duration_strict(self):
        for duration in (0, 301, True, 1.0, "30", None):
            with self.assertRaises(VideoError):
                duration_seconds(duration)
        self.assertEqual(duration_seconds(300), 300)

    def test_fixed_tcp_decoder_command(self):
        args = decoder_argv("10.1.2.3", 60)
        self.assertEqual(args[0], "/usr/bin/ffmpeg")
        self.assertEqual(args[args.index("-i") + 1], "rtsp://10.1.2.3:8554/live")
        self.assertEqual(args[args.index("-rtsp_transport") + 1], "tcp")
        self.assertEqual(args[args.index("-protocol_whitelist") + 1], "rtsp,tcp")
        self.assertEqual(args[-1], "pipe:1")
        self.assertIn("-an", args)
        self.assertNotIn("-y", args)
        self.assertNotIn("fps=5", args[args.index("-vf") + 1])
        self.assertIn("select=", args[args.index("-vf") + 1])
        self.assertEqual(args[args.index("-fps_mode") + 1], "passthrough")
        self.assertEqual(args[args.index("-enc_time_base") + 1], "1:90000")
        self.assertEqual(args[args.index("-fpsprobesize") + 1], "0")
        self.assertEqual(args[args.index("-flush_packets") + 1], "1")

    def test_parser_fragmented_and_coalesced_frames(self):
        expected = [png(0), png(30), png(255)]
        stream = b"".join(expected)
        for size in (1, 7, 19, READ_SIZE):
            parser, got = PNGParser(), []
            for offset in range(0, len(stream), size):
                got.extend(parser.feed(stream[offset:offset + size]))
            parser.finish()
            self.assertEqual(got, expected)

    def test_parser_negative_paths(self):
        header = chunk(b"IHDR", struct.pack(">IIBBBBB", 640, 360, 8, 2, 0, 0, 0))
        invalid = [b"notapng!", SIGNATURE + struct.pack(">I4s", MAX_PNG, b"IDAT"),
                   png()[:-1] + b"X", SIGNATURE + chunk(b"IDAT", b"bad"),
                   SIGNATURE + header + header,
                   SIGNATURE + header + chunk(b"IEND", b""),
                   SIGNATURE + header + chunk(b"IDAT", b"bad"),
                   SIGNATURE + header + chunk(b"tEXt", b"private"),
                   png(pixels=b"short"), png(pixels=b"\x05" + b"\0" * 1920 * 360),
                   png(pixels=b"\0" * (691560 + 1))]
        for data in invalid:
            with self.subTest(length=len(data)), self.assertRaises(VideoError):
                parser = PNGParser()
                for offset in range(0, len(data), READ_SIZE):
                    parser.feed(data[offset:offset + READ_SIZE])
                parser.finish()
        with self.assertRaises(VideoError):
            PNGParser().feed(b"x" * (READ_SIZE + 1))
        parser = PNGParser()
        parser.feed(png()[:-1])
        with self.assertRaises(VideoError):
            parser.finish()

    def test_construct_and_stop_never_spawn(self):
        spawn = Mock(side_effect=AssertionError("No child allowed"))
        decoder = Decoder(popen=spawn)
        self.assertFalse(decoder.snapshot().ready)
        decoder.stop()
        decoder.close()
        spawn.assert_not_called()

    def test_snapshot_invalidates_immediately(self):
        clock = Clock()
        decoder = Decoder(clock=clock)
        decoder._active = True
        decoder._start = 100
        decoder._duration = 60
        decoder._last = 100
        decoder._frame = png()
        self.assertTrue(decoder.snapshot().ready)
        clock.now = 103.001
        self.assertFalse(decoder.snapshot().ready)
        clock.now = 101
        decoder._process = fake_process()
        decoder._process.poll.return_value = 1
        self.assertFalse(decoder.snapshot().ready)
        decoder._process.poll.return_value = None
        decoder.stop()
        self.assertFalse(decoder.snapshot().ready)
        self.assertIsNone(decoder.snapshot().frame)

    def pump(self, events, *, duration=60):
        clock = Clock()
        decoder = Decoder(clock=clock)
        decoder._active, decoder._start, decoder._duration = True, 100, duration
        proc = fake_process()
        selector = Mock()
        selector.__enter__ = Mock(return_value=selector)
        selector.__exit__ = Mock(return_value=False)
        reads = []
        def select(timeout):
            clock.now += timeout
            if not events:
                return []
            event = events.pop(0)
            if callable(event):
                event(decoder, proc, clock)
                return []
            label, data = event
            key = Mock(data=label, fileobj=proc.stdout if label == "video" else proc.stderr)
            reads.append(data)
            return [(key, 1)]
        selector.select.side_effect = select
        with patch("optical_video.selectors.DefaultSelector", return_value=selector), \
             patch("optical_video.os.set_blocking"), \
             patch("optical_video.os.read", side_effect=lambda *_: reads.pop(0)):
            result = decoder._pump(proc)
        return decoder, proc, result

    def test_pump_single_latest_slot_and_eof(self):
        first, second = png(1), png(2)
        seen = []
        def inspect(decoder, proc, clock):
            seen.append(decoder.snapshot())
        decoder, _, result = self.pump([("video", first + second), inspect, ("video", b"")])
        self.assertEqual(seen[0].sequence, 2)
        self.assertEqual(seen[0].frame, second)
        self.assertEqual(decoder._sequence, 2)
        self.assertIn("ended", result)
        self.assertFalse(hasattr(decoder, "queue"))

    def test_pump_stall_duration_exit_and_stop(self):
        self.assertIn("Startup deadline", self.pump([])[2])
        self.assertIn("duration", self.pump([], duration=1)[2])
        def exit_child(decoder, proc, clock):
            proc.poll.return_value = 1
        self.assertIn("exited", self.pump([exit_child])[2])
        def stop(decoder, proc, clock):
            decoder.stop()
        self.assertEqual(self.pump([stop])[2], "Stopped")

    def test_startup_grace_never_marks_no_frame_ready_or_weakens_live_staleness(self):
        def startup(decoder, proc, clock):
            clock.now += 5
            self.assertFalse(decoder.snapshot().ready)
        decoder, _, reason = self.pump([startup, ("video", png()), ("video", b"")])
        self.assertEqual(decoder._sequence, 1)
        self.assertIn("ended", reason)
        def stall_after_frame(decoder, proc, clock):
            clock.now += 3.01
            self.assertFalse(decoder.snapshot().ready)
        _, _, reason = self.pump([("video", png()), stall_after_frame])
        self.assertIn("stalled", reason)

    def test_pump_errors_never_expose_diagnostics(self):
        for event in (("video", b"badframe"), ("error", b"rtsp://secret PRIVATE"),
                      ("video", png()[:-1]), ("video", b"")):
            if event[1] == b"":
                self.assertIn("ended", self.pump([event])[2])
            else:
                with self.assertRaises(VideoError) as error:
                    self.pump([event, ("video", b"")])
                self.assertNotIn("secret", str(error.exception))

    def test_worker_failure_and_reap_only_owned_child(self):
        proc = fake_process()
        spawn = Mock(return_value=proc)
        decoder = Decoder(popen=spawn)
        with patch.object(decoder, "_pump", side_effect=RuntimeError("rtsp://secret")):
            decoder._run(decoder_argv("10.1.2.3", 1))
        self.assertFalse(decoder.snapshot().ready)
        self.assertNotIn("secret", decoder.snapshot().reason)
        proc.terminate.assert_called_once_with()
        proc.wait.assert_called_once_with(timeout=1)
        proc.kill.assert_not_called()
        kwargs = spawn.call_args.kwargs
        self.assertFalse(kwargs["shell"])
        self.assertTrue(kwargs["close_fds"])
        self.assertEqual(kwargs["env"], {"PATH": "/usr/bin:/bin", "LC_ALL": "C"})
        proc.stdout.close.assert_called_once_with()
        proc.stderr.close.assert_called_once_with()

    def test_reap_escalates_only_local_decoder(self):
        proc = fake_process()
        proc.wait.side_effect = [subprocess.TimeoutExpired("local", 1), 0]
        Decoder._reap(proc)
        proc.kill.assert_called_once_with()
        self.assertEqual(proc.wait.call_count, 2)

    def test_retirement_failure_suppressed_and_close_denied(self):
        proc = fake_process()
        decoder = Decoder(popen=Mock(return_value=proc))
        decoder._retired = False
        with patch.object(decoder, "_pump", return_value="Ended"), \
             patch.object(decoder, "_reap", side_effect=RuntimeError("private")):
            decoder._run([])
        with self.assertRaises(VideoError):
            decoder.close()

    def test_start_is_worker_owned_one_shot(self):
        spawn = Mock(side_effect=RuntimeError("private"))
        decoder = Decoder(popen=spawn)
        decoder.start("10.1.2.3", 1)
        decoder._thread.join(1)
        self.assertFalse(decoder.snapshot().ready)
        with self.assertRaises(VideoError):
            decoder.start("10.1.2.3", 1)
        decoder.close()


if __name__ == "__main__":
    unittest.main()
