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
    def run_console(self, text="", reset=False, port=None, profile=None):
        port = port or FakePort()
        reset_action = Mock()
        constructor = Mock(return_value=port)
        argv = ["console", "--port", "fake-port"] + (["--reset"] if reset else [])
        if profile is not None:
            argv += ["--profile", profile]
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
            "gc16-gray16-labeled", "gc16-gray16-vendor", "gc16-gray16-page",
            "gc16-gray16-labeled-continuous", "pattern-confirm", "pattern-reject"})

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

    def test_storage_profile_has_only_explicit_commands_and_no_auto_command(self):
        self.assertEqual(console.STORAGE_COMMANDS, {"status", "mount", "file-test", "file-check", "upload-test", "photo-check", "stop"})
        result, port, reset = self.run_console(profile="storage")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [])
        reset.assert_not_called()
        result, port, _ = self.run_console("status\nmount\nfile-test\nfile-check\nphoto-check\nstop\n", profile="storage")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [b"status\n", b"mount\n", b"file-test\n", b"file-check\n", b"photo-check\n", b"stop\n"])

    def test_storage_profile_rejects_display_format_prefix_and_control(self):
        result, port, _ = self.run_console(
            "clear-white\nwhite-confirm\ngc16-gray16\nformat\n mount\nmount x\n"
            "mount\x00\nmount\t\nstatus\n", profile="storage")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [b"status\n"])
        result, port, _ = self.run_console("mount\nfile-test\nfile-check\nphoto-check\n", profile="display")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [])

    def test_file_test_path_prefix_controls_and_auto_retry_are_rejected(self):
        result, port, _ = self.run_console(
            "file-test /storage/a\nfile-test;stop\nFILE-TEST\nfile-test\x00\n"
            "file-test\t\n file-test\nfile-test\n", profile="storage")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [b"file-test\n"])

    def test_storage_partial_write_is_not_retried(self):
        port = FakePort()
        port.partial = True
        result, port, _ = self.run_console("mount\nstop\n", port=port, profile="storage")
        self.assertEqual(result, 1)
        self.assertEqual(port.writes, [b"mount\n"])
        self.assertTrue(port.closed)

    def test_file_check_only_exact_command_no_paths_or_control_characters(self):
        result, port, reset = self.run_console(
            "file-check /storage/a\nfile-check;stop\nFILE-CHECK\nfile-check\x00\n"
            "file-check\t\n file-check\nfile-check\r\n", profile="storage")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [b"file-check\n"])
        reset.assert_not_called()


    def test_photo_check_only_exact_command_no_paths_or_control_characters(self):
        result, port, reset = self.run_console(
            "photo-check /storage/a\nphoto-check;stop\nPHOTO-CHECK\nphoto-check\x00\n"
            "photo-check\t\n photo-check\nphoto-check\r\n", profile="storage")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [b"photo-check\n"])
        reset.assert_not_called()


    def test_photo_profile_is_exact_and_eof_sends_nothing(self):
        expected = {"status", "mount", "clear-white", "white-confirm", "photo-show", "photo-confirm", "photo-reject", "stop"}
        self.assertEqual(console.PHOTO_COMMANDS, expected)
        result, port, reset = self.run_console(profile="photo")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [])
        reset.assert_not_called()
        result, port, reset = self.run_console("\n".join(sorted(expected)) + "\n", profile="photo")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [(x + "\n").encode() for x in sorted(expected)])
        reset.assert_not_called()

    def test_photo_profile_rejects_other_profiles_paths_and_control(self):
        result, port, _ = self.run_console("file-test\nphoto-check\ngc16-gray16\nphoto-show x\n"
                            "photo-show\t\nphoto-show\x00\n/photo.jpg\n photo-show\nPHOTO-SHOW\nstatus\n", profile="photo")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [b"status\n"])


    def test_upload_probe_only_explicit_storage_command(self):
        result, port, _ = self.run_console(
            "upload-test /storage/a\nupload-test;stop\nUPLOAD-TEST\nupload-test\x00\n"
            "upload-test\t\n upload-test\nupload-test\r\n", profile="storage")
        self.assertEqual(result, 0)
        self.assertEqual(port.writes, [b"upload-test\n"])
        for profile in ("display", "photo"):
            result, port, _ = self.run_console("upload-test\n", profile=profile)
            self.assertEqual(result, 0)
            self.assertEqual(port.writes, [])

if __name__ == "__main__":
    unittest.main()
