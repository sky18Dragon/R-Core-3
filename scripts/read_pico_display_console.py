#!/usr/bin/env python3
"""Explicit command-only USB console; no flash, calibration, automatic clear or reconnect.

Use the locked ESP-IDF Python environment (pyserial + esptool). Keep a private log
via shell redirection. --reset is an explicit ESP reset, not a PMU reset.
"""

import argparse
import select
import sys

import serial

COMMANDS = frozenset(("clear-white", "white-confirm", "stop", "status",
                      "gc16-white", "gc16-black", "gc16-midgray", "gc16-gray16",
                      "gc16-parity", "gc16-corners", "gc16-gray16-labeled", "gc16-gray16-vendor", "gc16-gray16-page",
                      "pattern-confirm", "pattern-reject"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--reset", action="store_true")
    args = parser.parse_args()
    # Set modem levels BEFORE opening, avoiding pyserial's default asserted lines.
    port = serial.Serial(port=None, baudrate=115200, timeout=0.1, write_timeout=1)
    port.port = args.port
    port.dtr = False
    port.rts = False
    try:
        port.open()
        if args.reset:
            from esptool.reset import HardReset

            HardReset(port, uses_usb=True).reset()
        print("Console ready; no commands auto-sent. Allowed: " + ", ".join(sorted(COMMANDS)),
              flush=True)
        while True:
            data = port.read(4096)
            if data:
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
            if select.select([sys.stdin], [], [], 0)[0]:
                line = sys.stdin.readline()
                if not line:
                    break
                command = line.rstrip("\r\n")
                if command not in COMMANDS:
                    print("Rejected unknown command; nothing sent", flush=True)
                    continue
                payload = (command + "\n").encode("ascii")
                if port.write(payload) != len(payload):
                    raise serial.SerialTimeoutException("Partial command write; not retried")
                print("Sent explicit command: " + command, flush=True)
    except KeyboardInterrupt:
        print("Console interrupted; no command auto-sent", file=sys.stderr)
        return 0
    except (serial.SerialException, OSError) as error:
        print("Console stopped: " + str(error) + "; no reconnect/retry", file=sys.stderr)
        return 1
    finally:
        if port.is_open:
            port.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
