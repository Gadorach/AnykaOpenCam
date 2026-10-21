#!/usr/bin/env python3
"""FreeSimpleGUI desktop bench. No actuation on startup or status refresh."""
import argparse
import datetime as dt
import fcntl
import json
import math
import os
from pathlib import Path
import threading
import time

import FreeSimpleGUI as sg
from backend import Bench, BenchError, ROOT
from ssh_backend import SSHBench, DEFAULT_PROFILE, FAULT, tone_parameters


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--smoke-ui", action="store_true", help="display briefly; no UART/SSH/profile reads")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--ssh-profile", type=Path, help="parent-owned mode600 controlled stock SSH epoch")
    mode.add_argument("--uart", action="store_true", help="explicit legacy recorder mode (never SSH)")
    args = parser.parse_args()
    # Smoke constructs neither backend and reads no private profile or manifest.
    ssh_mode = not args.uart
    bench = None
    held = None
    if not args.smoke_ui:
        if ssh_mode and not (args.ssh_profile or DEFAULT_PROFILE).exists():
            raise SystemExit("SSH profile missing; parent preparation required. No UART fallback; legacy requires --uart.")
        held = (ROOT / "work/qc-uart-live/peripheral-gui-instance.lock").open("a")
        try:
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise SystemExit("The peripheral GUI is already running; use that window.")
        if ssh_mode:
            bench = SSHBench(args.ssh_profile or DEFAULT_PROFILE)
        else:
            manifest = Path(__file__).with_name("helper.json")
            sealed = json.loads(manifest.read_text()) if manifest.exists() else {"sha256": "0" * 64}
            session = ROOT / "work/peripheral-gui-sessions" / (dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ") + f"-{os.getpid()}")
            bench = Bench(sealed["sha256"], session)
    sg.theme("DarkBlue3")
    layout = [
        [sg.Text("QC07F — Peripheral and audio bench", font=("Sans", 17, "bold"))],
        [sg.Text("User-observed directions; ±32-step nudges only. Software counts are not calibrated physical positions.")],
        [sg.Text("LED ON lasts at most 5 seconds, then returns to 0. GPIO2, homing and full sweeps are excluded.")],
        [sg.Text("SSH: motors/LEDs disabled — not prepared in this minimal stock epoch; no providers will be loaded."
                 if ssh_mode else "Legacy UART recorder mode; audio7 tone/ambient require a parent-prepared SSH epoch.")],
        [sg.Frame("Motor0 — Left/right", [[sg.Button("Counter-clockwise −32", key="motor0-minus"), sg.Button("Clockwise +32", key="motor0-plus"),
                               sg.Text("Direction confirmed by user", key="M0", size=(32, 1))]])],
        [sg.Frame("Motor1 — Up/down", [[sg.Button("Toward edge −32", key="motor1-minus"), sg.Button("Toward center +32", key="motor1-plus"),
                               sg.Text("Direction confirmed by user", key="M1", size=(32, 1))]])],
        [sg.Text("Tilt labels describe the observed range, not a center-seeking command. No homing or position feedback.")],
        [sg.Frame("White LED", [[sg.Button("ON — 5 seconds", key="white-on"), sg.Button("OFF", key="white-off")]]),
         sg.Frame("IR LED", [[sg.Button("ON — 5 seconds", key="ir-on"), sg.Button("OFF", key="ir-off")]])],
        [sg.Frame("Microphone activity — sample levels only, no audio saved", [
            [sg.Button("Monitor mic — 10 seconds", key="mic-start"), sg.Button("Stop monitoring", key="mic-stop", disabled=True)],
            [sg.ProgressBar(80, orientation="h", size=(42, 18), key="MIC_BAR"), sg.Text("Not monitoring", key="MIC_LEVEL", size=(45, 1))],
            [sg.Text("Updates after each bounded 1s sample. Late-half AC RMS removes DC and reduces startup-transient influence.")]
        ])],
        [sg.Button("Speaker — short quiet test tone", key="speaker-tone"),
         sg.Text("Expected stock DAC0; audible output needs your confirmation. No IR-cut/MX6208 control.")],
        [sg.Frame("Camera-generated tone — SSH audio7; no host waveform/audio transfer", [
            [sg.Text("Frequency Hz"), sg.Input("1000", key="TONE_HZ", size=(8, 1)),
             sg.Text("Analog gain code"), sg.Combo(list(range(6)), default_value=3, readonly=True, key="TONE_GAIN", size=(3, 1)),
             sg.Text("Digital volume %"), sg.Input("25", key="TONE_VOLUME", size=(5, 1)),
             sg.Text("Duration s (0..5)"), sg.Input("0.25", key="TONE_SECONDS", size=(6, 1))],
            [sg.Text("Frequency maximum unknown — Read status first", key="TONE_LIMIT", size=(68, 1))],
            [sg.Checkbox("I confirm UART signal wires are physically disconnected", default=False, key="UART_ACK", enable_events=True)],
            [sg.Button("Play one tone", key="tone"), sg.Text("Audibility unknown. Zero duration = no effects, not played.")]
        ])],
        [sg.Frame("Ambient — manual one-shot only", [
            [sg.Button("Read ambient once", key="ambient"), sg.Text("Not read", key="AMBIENT_VALUE", size=(48, 1)),
             sg.ProgressBar(2999, orientation="h", size=(25, 16), key="AMBIENT_BAR")],
            [sg.Text("0..2999 stock-scaled, uncalibrated; higher = darker (not lux). ADC may hang forever IRQ-disabled.")]
        ])],
        [sg.Button("Read status / Connect", key="refresh"), sg.Button("Both LEDs OFF", key="lights-off", button_color=("white", "firebrick")),
         sg.Button("Close")],
        [sg.Text("Disconnected — click Read status. No command has been sent.", key="STATE", size=(100, 2))],
        [sg.Text("On uncertain stop/close/link: switch bulb power OFF and leave OFF. No automatic recovery or DMA kill.")],
        [sg.Multiline("", key="LOG", size=(105, 15), disabled=True, autoscroll=True, expand_x=True, expand_y=True)],
        [sg.Text("FreeSimpleGUI 5.3.0.post1 (LGPLv3); library notices remain in the local venv. Local-only / no camera stream.")],
    ]
    window = sg.Window("QC07F peripheral bench", layout, finalize=True, resizable=True,
                       enable_close_attempted_event=True)
    controls = ["motor0-minus", "motor0-plus", "motor1-minus", "motor1-plus", "white-on", "white-off", "ir-on", "ir-off", "lights-off", "mic-start", "speaker-tone", "tone", "ambient"]
    busy, connected, closing, fault = False, False, False, False
    pulse_refresh = 0.0
    monitor_until = 0.0
    busy_kind = ""

    def enabled():
        for key in controls:
            window[key].update(disabled=busy or not connected or fault or monitor_until > 0 or
                               (ssh_mode and key in controls[:9] + ["speaker-tone"]) or
                               (not ssh_mode and key in ("tone", "ambient")) or
                               (key == "tone" and (not window["UART_ACK"].get() or bench is None or bench.rate is None)) or
                               (key == "speaker-tone" and (bench is None or "speaker" not in bench.audio)) or
                               (bench is not None and time.monotonic() < bench.pulse_until and key not in ("white-off", "ir-off", "lights-off")))
        window["mic-stop"].update(disabled=not (monitor_until or busy_kind == "mic-sample"))
        window["refresh"].update(disabled=busy or fault or monitor_until > 0)

    def dispatch(key, inputs=None):
        nonlocal busy, busy_kind
        busy = True
        busy_kind = key
        enabled()
        window["STATE"].update("Running " + key + " — please observe the device")

        def work():
            try:
                if key == "refresh":
                    result = bench.status()
                elif key in controls[:4]:
                    result = bench.motor(key)
                elif key == "lights-off":
                    result = bench.lights_off()
                elif key == "mic-sample":
                    result = bench.audio_test("mic")
                elif key == "speaker-tone":
                    result = bench.audio_test("speaker")
                elif key == "tone":
                    result = bench.tone(inputs["TONE_HZ"], inputs["TONE_GAIN"], inputs["TONE_VOLUME"],
                                        inputs["TONE_SECONDS"], inputs["UART_ACK"])
                elif key == "ambient":
                    result = bench.ambient()
                else:
                    kind, action = key.split("-")
                    result = bench.led(kind, action == "on")
                error = "GUI_BLOCKED" in result or "RETAINED" in result
                item = (key, result, error)
            except Exception as exc:
                item = (key, str(exc), True)
            if not closing:
                window.write_event_value("-RESULT-", item)

        threading.Thread(target=work, daemon=True).start()

    enabled()
    smoke_end = time.monotonic() + 2
    while True:
        event, values = window.read(timeout=100)
        if args.smoke_ui and time.monotonic() >= smoke_end:
            break
        if event in (sg.WIN_CLOSED, sg.WINDOW_CLOSE_ATTEMPTED_EVENT, "Close"):
            monitor_until = 0
            if busy:
                window["STATE"].update("Wait for the bounded operation to finish before closing")
                continue
            if ssh_mode and bench is not None and bench.uncertain_running():
                window["STATE"].update("Unverified remote retirement: operation lock retained even if SSH exited — power OFF and leave OFF; parent recovery required before closing")
                continue
            # A timed LED child always restores independently, even if no link.
            if connected and time.monotonic() < bench.pulse_until and not fault:
                dispatch("lights-off")
                window["STATE"].update("Turning LEDs off. Click Close again after completion.")
                continue
            break
        if args.smoke_ui:
            continue
        if event == "-RESULT-":
            key, text, error = values[event]
            busy = False
            busy_kind = ""
            if error:
                monitor_until = 0
                fault = True
                connected = False
                window["STATE"].update(FAULT if ssh_mode else "BLOCKED / ERROR — do not repeat blindly; ask the assistant or power off if needed")
            else:
                connected = True
                message = ("No movement requested: the software limit/margin would be crossed."
                           if "LIMIT_BLOCKED" in text else
                           "Completed. Check the actual physical result; GPIO/software receipts are not movement proof.")
                window["STATE"].update(message)
            window["LOG"].update(f"[{dt.datetime.now().strftime('%H:%M:%S')}] {key}\n{text.replace(chr(0), '')}\n\n", append=True)
            for line in text.splitlines():
                if line.startswith("{\"record\":"):
                    row = json.loads(line)
                    if "motor" in row:
                        pos = row.get("software_position", row.get("software_position_after", "?"))
                        window[f"M{row['motor']}"].update(f"Software count {pos}; direction user-mapped")
                    if row.get("record", "").startswith("mic-level-"):
                        ac = row.get("half_ac_rms", [row["rms"], row["rms"]])[-1]
                        db = max(-80., 20 * math.log10(max(1, ac) / 32768))
                        window["MIC_BAR"].update(max(0, min(80, 80 + db)))
                        window["MIC_LEVEL"].update(f"Late AC RMS {ac} | {db:.1f} dBFS* | clipped {row['clipped']}")
                        window["STATE"].update("Digital sample amplitude (*S16LE candidate), not calibrated sound pressure or intelligibility proof.")
                    if row.get("record") == "stock-speaker-tone-v3" and not error:
                        window["STATE"].update("Tone test stopped and closed. Did you hear the short beep? Report the result before repeating.")
                    if row.get("record") == "peripheral-status-v7":
                        window["TONE_LIMIT"].update(f"1..{row['frequency_max_hz']} Hz; calculated DAC {row['calculated_DAC_rate']} Hz; not physically measured")
                        window["STATE"].update("Read-only status; no PCM opened, gain set or ambient conversion requested.")
                    if row.get("record") == "stock-ambient-control-v1":
                        window["AMBIENT_VALUE"].update(f"{row['value']} / 2999 — higher = darker; uncalibrated")
                        window["AMBIENT_BAR"].update(row["value"])
                    if row.get("record") == "stock-tone-control-v7" and not error:
                        window["STATE"].update("Zero duration: no effects; no tone played." if row["duration_ms"] == 0 else
                                               "Tone stopped/closed and original gain/mux/latch restored. Physical audibility UNKNOWN — report heard/not heard.")
            if time.monotonic() < bench.pulse_until:
                pulse_refresh = bench.pulse_until
            enabled()
        elif event == "mic-stop":
            monitor_until = 0
            window["STATE"].update("Monitoring stopping after the current bounded sample; no new capture will start.")
            enabled()
        elif event == "mic-start" and connected and not busy and not fault:
            monitor_until = time.monotonic() + 10
            enabled()
        elif event == "UART_ACK":
            enabled()
        elif event == "refresh" and not busy and not monitor_until and not fault:
            dispatch(event)
        elif event in controls and connected and not busy and not fault:
            if monitor_until or (ssh_mode and event in controls[:9] + ["speaker-tone"]) or (not ssh_mode and event in ("tone", "ambient")):
                continue
            if event == "tone":
                try:
                    tone_parameters(values["TONE_HZ"], values["TONE_GAIN"], values["TONE_VOLUME"], values["TONE_SECONDS"], bench.rate)
                    if not values["UART_ACK"]:
                        raise BenchError("Confirm physical UART signal disconnection first; no command sent")
                except BenchError as exc:
                    window["STATE"].update(str(exc))
                    continue
            dispatch(event, dict(values))
        if pulse_refresh and time.monotonic() >= pulse_refresh and not busy:
            pulse_refresh = 0
            dispatch("refresh")
        if monitor_until and not busy:
            if time.monotonic() + 3 >= monitor_until:
                monitor_until = 0
                window["STATE"].update("10-second microphone check finished; capture is stopped between samples.")
                enabled()
            else:
                dispatch("mic-sample")
    closing = True
    window.close()
    if held is not None:
        held.close()


if __name__ == "__main__":
    main()
