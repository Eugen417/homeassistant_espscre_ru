"""Capture ESP32 boot/runtime diagnostics. Requires pyserial (in ESPHome's venv)."""
import argparse
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument('--port', default='/dev/cu.usbserial-130')
parser.add_argument('--seconds', type=int, default=90)
parser.add_argument('--reset', action='store_true')
args = parser.parse_args()
with serial.Serial(args.port, 115200, timeout=0.2) as port:
    if args.reset:
        port.dtr = False
        port.rts = True
        time.sleep(0.1)
        port.rts = False
    end = time.monotonic() + args.seconds
    while time.monotonic() < end:
        print(port.read(4096).decode('utf-8', errors='replace'), end='', flush=True)
