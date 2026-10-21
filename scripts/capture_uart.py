#!/usr/bin/env python3
"""Receive-only UART capture. Requires preinstalled pyserial; never sends bytes."""

import argparse
from datetime import datetime, timezone
import math
from pathlib import Path
import sys
import time


def positive_float(value):
    try:
        result = float(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("must be a positive finite number") from exc
    if not math.isfinite(result) or result <= 0:
        raise argparse.ArgumentTypeError("must be a positive finite number")
    return result


def positive_int(value):
    try:
        result = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("must be a positive integer") from exc
    if result <= 0:
        raise argparse.ArgumentTypeError("must be a positive integer")
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="explicit OS port, e.g. /dev/ttyUSB0 or COM5; no auto-discovery")
    parser.add_argument("--baud", required=True, type=positive_int, help="explicit baud rate; 8 data bits, no parity, 1 stop bit")
    parser.add_argument("--seconds", required=True, type=positive_float, help="maximum capture duration")
    parser.add_argument("--log", required=True, type=Path, help="NEW raw binary log; an existing path is refused")
    args = parser.parse_args(argv)
    try:
        import serial
    except ImportError:
        print("error: pyserial is not installed. Install it separately through your approved local workflow; this helper installs nothing.", file=sys.stderr)
        return 2
    if not hasattr(serial, "Serial"):
        print("error: imported 'serial' is not the expected pyserial package.", file=sys.stderr)
        return 2
    connection = None
    count = 0
    interrupted = False
    try:
        # Exclusive creation happens before touching the UART port.
        with args.log.expanduser().open("xb") as output:
            connection = serial.Serial(port=None, baudrate=args.baud,
                                       bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
                                       stopbits=serial.STOPBITS_ONE, timeout=0.2,
                                       xonxoff=False, rtscts=False, dsrdtr=False)
            # These are requested while CLOSED. Some OS/adapter combinations may
            # still pulse lines on open/close: leave DTR/RTS/TX/VCC disconnected.
            connection.dtr = False
            connection.rts = False
            connection.port = args.port
            print("Connect only confirmed camera TX to adapter RX, and common GND.", file=sys.stderr)
            print("Adapter TX, DTR, RTS, and VCC must remain disconnected. Software cannot guarantee glitch-free control lines.", file=sys.stderr)
            print(f"Starting receive-only capture: {args.port!r}, {args.baud} 8N1, {args.seconds:g} s; {datetime.now(timezone.utc).isoformat()}", file=sys.stderr)
            connection.open()
            deadline = time.monotonic() + args.seconds
            try:
                while True:
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        break
                    connection.timeout = min(0.2, remaining)
                    chunk = connection.read(4096)
                    if chunk:
                        output.write(chunk)
                        output.flush()
                        count += len(chunk)
            except KeyboardInterrupt:
                interrupted = True
            finally:
                output.flush()
    except (OSError, ValueError, serial.SerialException) as exc:
        print(f"error: {exc}; any newly created partial log is retained", file=sys.stderr)
        return 2
    finally:
        if connection is not None:
            try:
                connection.close()
            except (OSError, serial.SerialException) as exc:
                print(f"warning while closing UART: {exc}", file=sys.stderr)
    print(f"Captured {count} raw bytes to {args.log}. No UART data was transmitted.")
    if count == 0:
        print("No received bytes does not establish a hardware fault; confirm power, voltage, port, baud, and ground separately.")
    return 130 if interrupted else 0


if __name__ == "__main__":
    sys.exit(main())
