"""Command-only console regression; fake serial transport, never opens hardware."""

import importlib.util
import io
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import Mock, patch


class SerialError(Exception):
    pass


fake_serial = types.SimpleNamespace(
    Serial=Mock(), SerialException=SerialError, SerialTimeoutException=SerialError)
spec = importlib.util.spec_from_file_location(
    "read_pico_console", Path(__file__).resolve().parents[1] / "scripts/read_pico_display_console.py")
console = importlib.util.module_from_spec(spec)
with patch.dict(sys.modules, {"serial": fake_serial}):
    spec.loader.exec_module(console)


class FakePort:
    def __init__(self):
        self.is_open = False
        self.writes = []
        self.closed = False
        self.partial = False
        self.read_error = None
        self.open_count = 0

    def open(self):
        assert self.dtr is False and self.rts is False
        self.is_open = True
        self.open_count += 1

    def read(self, size):
        if self.read_error:
            raise self.read_error
        return b""

    def write(self, data):
        self.writes.append(data)
        return len(data) - 1 if self.partial else len(data)

    def close(self):
        self.is_open = False
        self.closed = True


class ConsoleTest(unittest.TestCase):
    def run_console(self, text="", reset=False, port=None):
        port = port or FakePort()
        reset_action = Mock()
        constructor = Mock(return_value=port)
        argv = ["console", "--port", "fake-port"] + (["--reset"] if reset else [])
        reset_module = types.SimpleNamespace(HardReset=reset_action)
        with patch.object(console.serial, "Serial", constructor), \
                patch.object(sys, "argv", argv), \
                patch.object(sys, "stdin", io.StringIO(text)), \
                patch.object(sys, "stdout", io.StringIO()), \
                patch.object(sys, "stderr", io.StringIO()), \
                patch.object(console.select, "select", side_effect=lambda *a: ([sys.stdin], [], [])), \
                patch.dict(sys.modules, {"esptool.reset": reset_module}):
            result = console.main()
        constructor.assert_called_once_with(
            port=None, baudrate=115200, timeout=0.1, write_timeout=1)
        return result, port, reset_action

    def test_exact_whitelist(self):
        self.assertEqual(console.COMMANDS, {
            "clear-white", "white-confirm", "stop", "status", "gc16-white", "gc16-black",
            "gc16-midgray", "gc16-gray16", "gc16-parity", "gc16-corners",
            "gc16-gray16-labeled", "gc16-gray16-vendor", "gc16-gray16-page", "pattern-confirm", "pattern-reject"})

    def test_eof_never_sends_or_resets(self):
        result, port, reset = self.run_console()
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [])
        self.assertTrue(port.closed)
        reset.assert_not_called()

    def test_exact_commands_sent_once_including_reject(self):
        commands = sorted(console.COMMANDS)
        result, port, reset = self.run_console("\n".join(commands) + "\n")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [(c + "\n").encode("ascii") for c in commands])
        self.assertEqual(port.open_count, 1)
        reset.assert_not_called()

    def test_unknown_prefix_and_control_are_never_sent(self):
        result, port, _ = self.run_console(
            "gc16-all\n gc16-gray16\ngc16-gray16 x\ngc16-gray16\t\n"
            "GC16-GRAY16\ngc16-gray16\x00\n\nstatus\n")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [b"status\n"])

    def test_partial_write_stops_without_retry(self):
        port = FakePort()
        port.partial = True
        result, port, _ = self.run_console("gc16-gray16\ngc16-corners\n", port=port)
        self.assertEqual(result, 1)
        self.assertEqual(port.writes, [b"gc16-gray16\n"])
        self.assertTrue(port.closed)

    def test_read_disconnect_never_reconnects_or_sends(self):
        port = FakePort()
        port.read_error = SerialError("disconnected")
        result, port, _ = self.run_console("gc16-gray16\n", port=port)
        self.assertEqual(result, 1)
        self.assertEqual(port.writes, [])
        self.assertEqual(port.open_count, 1)
        self.assertTrue(port.closed)

    def test_keyboard_interrupt_closes_without_command(self):
        port = FakePort()
        port.read_error = KeyboardInterrupt()
        result, port, _ = self.run_console("stop\n", port=port)
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [])
        self.assertTrue(port.closed)

    def test_reset_only_when_explicit(self):
        result, port, reset = self.run_console(reset=True)
        self.assertEqual(result, 0)
        reset.assert_called_once_with(port, uses_usb=True)
        reset.return_value.reset.assert_called_once_with()
        self.assertEqual(port.writes, [])


if __name__ == "__main__":
    unittest.main()
