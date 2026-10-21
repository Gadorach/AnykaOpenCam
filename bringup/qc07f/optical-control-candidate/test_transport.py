#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Mock-only transport/source assertions. Never opens an actual device."""
import errno
from pathlib import Path
import unittest
from unittest.mock import patch
import optical


class Transport(unittest.TestCase):
    def endpoint(self):
        endpoint = optical.Optical.__new__(optical.Optical)
        endpoint.fd = 123
        return endpoint

    def sample(self, _fd, _cmd, data, _mutate):
        data[:] = optical.LAYOUT.pack(1, 1, 0, 7, 0, 0, 0, 0, 7, 1, 1,
                                     0, 0, 2000, 10000, 4900, 17, 999999)

    def test_layout_and_positive_receipt(self):
        self.assertEqual(optical.LAYOUT.size, 80)
        with patch.object(optical.fcntl, "ioctl", side_effect=self.sample) as call:
            result = self.endpoint().action("status")
        self.assertTrue(result["ioctl_completed"])
        self.assertEqual(result["state"]["release_verified"], 1)
        self.assertEqual(result["state"]["sequence"], 17)
        self.assertEqual(result["optical_position"], "unknown")
        self.assertEqual(call.call_count, 1)

    def test_status_error_is_not_completion(self):
        with patch.object(optical.fcntl, "ioctl", side_effect=OSError(errno.EIO, "model")):
            result = self.endpoint().action("status")
        self.assertFalse(result["ioctl_completed"])
        self.assertIsNone(result["state"])
        self.assertEqual(result["snapshot_errno"], errno.EIO)

    def test_no_effect_retry_on_status_loss(self):
        with patch.object(optical.fcntl, "ioctl",
                          side_effect=[0, OSError(errno.EIO, "model")]) as call:
            result = self.endpoint().action("pulse-a")
        self.assertTrue(result["ioctl_completed"])
        self.assertIsNone(result["state"])
        self.assertEqual(result["snapshot_errno"], errno.EIO)
        self.assertEqual(call.call_count, 2)
        self.assertEqual(call.call_args_list[0].args[1], optical.COMMANDS["pulse-a"])
        self.assertEqual(call.call_args_list[1].args[1], optical.STATUS)

    def test_failure_still_snapshots_without_retry(self):
        def fake(fd, cmd, *args):
            if cmd != optical.STATUS:
                raise OSError(errno.EAGAIN, "model cooldown")
            return self.sample(fd, cmd, *args)
        with patch.object(optical.fcntl, "ioctl", side_effect=fake) as call:
            result = self.endpoint().action("pulse-b")
        self.assertFalse(result["ioctl_completed"])
        self.assertEqual(result["errno"], errno.EAGAIN)
        self.assertEqual(result["state"]["release_verified"], 1)
        self.assertEqual(call.call_count, 2)

    def test_fixed_actions_only(self):
        with patch.object(optical.fcntl, "ioctl") as call:
            for bad in ("gpio6", "pulse-a 100", "; reboot", "", "IR_ON"):
                with self.assertRaises(ValueError):
                    self.endpoint().action(bad)
        call.assert_not_called()

    def test_close_once_no_ioctl_or_sleep(self):
        endpoint = self.endpoint()
        with patch.object(optical.os, "close") as close, patch.object(optical.fcntl, "ioctl") as ioctl:
            endpoint.close()
            endpoint.close()
        close.assert_called_once_with(123)
        ioctl.assert_not_called()

    def test_open_fixed_nofollow_cloexec(self):
        with patch.object(optical.os, "open", return_value=123) as open_:
            self.assertEqual(optical.Optical().fd, 123)
        self.assertEqual(open_.call_args.args[0], "/dev/qc07f-optical")
        self.assertTrue(open_.call_args.args[1] & optical.os.O_CLOEXEC)
        self.assertTrue(open_.call_args.args[1] & optical.os.O_NOFOLLOW)

    def test_source_no_direct_mmio_store_or_ownership_bypass(self):
        text = (Path(__file__).parent / "qc07f_optical.c").read_text()
        for prohibited in ("writel(", "writeb(", "writew(", "iowrite", "regmap_write(",
                           "regmap_update_bits(", "gpiochip_request_own_desc(",
                           "gpio_device_get_base(", "gpio_to_desc(", "gpiod_set_config("):
            self.assertNotIn(prohibited, text)
        self.assertIn("GPIOD_ASIS", text)
        self.assertIn("gpiod_direction_output(pins[i], 0)", text)
        self.assertIn("udelay(1000)", text)
        self.assertIn("gpiod_cansleep(pins[i])", text)
        self.assertIn(".mode = 0600", text)
        callback = text.split("static enum hrtimer_restart ir_expired", 1)[1].split("static void put_pins", 1)[0]
        self.assertNotIn("mutex_", callback)
        self.assertNotIn("schedule", callback)


if __name__ == "__main__":
    unittest.main(verbosity=2)
