#!/usr/bin/env python3
"""Dedicated optical GUI. Import/startup never connects or loads legacy controls."""
import argparse
import fcntl
from pathlib import Path

from optical_control import ACTIONS, AIM_ACTIONS, ALL_ACTIONS, ADAPTER_PENDING, Controls
from optical_video import Decoder, VideoError, admitted_host, duration_seconds

INSTANCE_LOCK = (Path(__file__).resolve().parents[4] /
                 'work/qc-uart-live/peripheral-gui-instance.lock')


def run(sg, *, driver=None, epoch=None, decoder_factory=Decoder, allow_loopback=False):
    """Injected driver/epoch are for parent integration and offline GUI fixtures.

    An optional private UART epoch selects the explicit parent bridge, never fallback.
    Every GUI call stays on this calling thread; workers expose bounded slots.
    """
    sg.theme("DarkBlue3")
    layout = [
        [sg.Text("QC07F — bounded optical preview", font=("Sans", 16, "bold"))],
        [sg.Text("Numeric private IPv4 host"),
         sg.Input(epoch.host if epoch is not None else "", key="HOST", size=(18, 1),
                  disabled=epoch is not None),
         sg.Text("Session seconds (1..300)"), sg.Input("300", key="SECONDS", size=(5, 1))],
        [sg.Button("Connect preview", key="connect"), sg.Button("Stop preview", key="stop", disabled=True),
         sg.Button("Read controls status", key="status")],
        [sg.Image(key="FRAME", size=(640, 360))],
        [sg.Text("Disconnected — no network or hardware operation on startup", key="VIDEO", size=(90, 1))],
        [sg.Button("A — filter OUT / IR-pass", key="pulse-a", disabled=True),
         sg.Button("B — filter IN / IR-block", key="pulse-b", disabled=True)],
         [sg.Button("IR ON — 4.9 s", key="ir-on", disabled=True),
          sg.Button("IR OFF", key="ir-off", disabled=True),
          sg.Button("White ON — 4.9 s", key="white-on", disabled=True),
          sg.Button("White OFF", key="white-off", disabled=True)],
        [sg.Text("Motor0 / pan"), sg.Button("−4 steps", key="motor0-minus", disabled=True),
         sg.Button("+4 steps", key="motor0-plus", disabled=True),
         sg.Text("Untested; position unknown", key="MOTOR0_META", size=(38, 1))],
        [sg.Text("Motor1 / tilt"), sg.Button("−4 steps", key="motor1-minus", disabled=True),
         sg.Button("+4 steps", key="motor1-plus", disabled=True),
         sg.Text("Untested; position unknown", key="MOTOR1_META", size=(38, 1))],
        [sg.Text(ADAPTER_PENDING, key="CONTROL", size=(95, 2))],
        [sg.Text("User-observed A/night IR-pass, B/day IR-block; fixed nominal10ms. Emitters remain separate.")],
        [sg.Text("Raw signs, no calibrated position/homing/sweeps. Target light timers are nominal4.9s; no auto repeat.")],
        [sg.Text("Unknown control completion: do not repeat; parent retirement/recovery required.")],
        [sg.Text("FreeSimpleGUI notices retained in existing local venv; FFmpeg is a separate system executable.")],
        [sg.Button("Close")],
    ]
    window = sg.Window("QC07F optical bench", layout, finalize=True, resizable=True,
                       enable_close_attempted_event=True)
    controls = Controls(driver, epoch)
    decoder = None
    host = ""
    shown = None
    try:
        while True:
            event, values = window.read(timeout=50)
            # Refresh freshness BEFORE interpreting any queued click/completion.
            preview = decoder.snapshot() if decoder is not None else None
            if preview is not None:
                controls.observe(preview, host)
                window["VIDEO"].update(preview.reason)
                marker = (preview.session, preview.sequence) if preview.ready else None
                if marker != shown:
                    if preview.ready:
                        window["FRAME"].update(data=preview.frame)
                    else:
                        window["FRAME"].update()  # documented FreeSimpleGUI clear
                    shown = marker
            controls.poll()
            if event in (sg.WIN_CLOSED, sg.WINDOW_CLOSE_ATTEMPTED_EVENT, "Close"):
                if decoder is not None:
                    decoder.stop()  # Only the owned LOCAL decoder may be stopped.
                    controls.observe(decoder.snapshot(), host)
                if not controls.close_allowed():
                    controls.message = "Pending/UNKNOWN control retained; ask parent for retirement/recovery"
                else:
                    break
            elif event == "connect" and decoder is None:
                try:
                    host = admitted_host(values["HOST"], allow_loopback)
                    text = values["SECONDS"]
                    if type(text) is not str or not text.isascii() or not text.isdecimal() or len(text) > 3:
                        raise VideoError("Session seconds must be an integer from 1 to 300")
                    seconds = duration_seconds(int(text))
                    decoder = decoder_factory()
                    decoder.start(host, seconds, allow_loopback)
                    shown = None
                except (ValueError, VideoError):
                    decoder = None
                    window["VIDEO"].update("Invalid endpoint/duration; no connection started")
            elif event == "stop" and decoder is not None:
                decoder.close()
                controls.observe(decoder.snapshot(), host)
                decoder = None
                shown = None
                window["FRAME"].update()
                window["VIDEO"].update("Stopped — manual Connect and controls-status required")
            elif event == "status" or event in ALL_ACTIONS:
                controls.dispatch(event)
            window["CONTROL"].update(controls.message)
            if controls.last_receipt is not None and controls.last_receipt['schema'] == 'optical-control-v2':
                for motor in controls.last_receipt['motors']:
                    text = ('Untested; position unknown' if motor['health'] == 'UNTESTED' else
                            f"Last: {motor['issued_steps']} issued; off-verified; position unknown")
                    window[f"MOTOR{motor['axis']}_META"].update(text)
            for action in ALL_ACTIONS:
                window[action].update(disabled=not controls.allowed(action))
            window["status"].update(disabled=controls.busy or controls.fault or
                                    (epoch is not None and not controls.allowed("status")))
            window["connect"].update(disabled=decoder is not None or controls.busy)
            window["stop"].update(disabled=decoder is None)
            window["HOST"].update(disabled=decoder is not None or epoch is not None)
            window["SECONDS"].update(disabled=decoder is not None)
    finally:
        if decoder is not None:
            decoder.close()
        window.close()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--loopback-mock", action="store_true",
                         help="allow explicit 127/8 preview fixtures; never control admission")
    parser.add_argument("--uart-profile", type=Path,
                        help="explicit parent-qualified private epoch; no serial open or fallback")
    args = parser.parse_args(argv)
    driver = epoch = None
    if args.uart_profile is not None:
        if args.loopback_mock:
            parser.error("Loopback fixtures cannot admit camera controls")
        from optical_uart import UARTDriver, NATIVE_SCHEMA_REVIEWED
        from optical_control import ControlError
        if not NATIVE_SCHEMA_REVIEWED:
            raise SystemExit("Optical control transport not yet admitted; no command sent")
        try:
            driver = UARTDriver(args.uart_profile)
            epoch = driver.epoch
        except (ControlError, OSError, ValueError, KeyError, TypeError):
            raise SystemExit("No usable parent-qualified optical epoch; no command sent") from None
    held = INSTANCE_LOCK.open('a')
    try:
        try:
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise SystemExit("A peripheral/optical bench window already owns this instance") from None
        import FreeSimpleGUI as sg  # Existing venv only; no package install.
        run(sg, driver=driver, epoch=epoch, allow_loopback=args.loopback_mock)
    finally:
        held.close()


if __name__ == "__main__":
    main()
