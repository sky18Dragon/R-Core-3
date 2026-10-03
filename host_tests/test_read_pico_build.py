"""Read Pico registration and build-only CLI guard tests; never invoke IDF."""

import json
import re
import importlib.util
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent.parent


class ReadPicoBuildTests(unittest.TestCase):
    def test_board_geometry_and_target(self):
        boards = json.loads((ROOT / "boards/boards.json").read_text())
        board = next(item for item in boards if item["id"] == "read_pico")
        self.assertEqual(board["resolution"], [1216, 684])
        self.assertEqual(board["display_type"], "gc16")
        self.assertEqual(board["target"], "esp32s3")
        self.assertEqual(len({item["id"] for item in boards}), len(boards))

    def test_unsafe_arguments_rejected_before_any_build(self):
        for factory in ([], ["--read-pico-factory-diagnostic", "--step", "firmware"],
                        ["--read-pico-factory-diagnostic", "--read-pico-display-validation",
                         "--step", "firmware"]):
            for argument in ("flash", "monitor", "erase-flash", "--fullclean", "-DREAD_PICO_FULL_APP=1"):
                with self.subTest(argument=argument, factory=factory):
                    result = subprocess.run(
                        [sys.executable, str(ROOT / "build.py"), "--board", "read_pico", *factory, argument],
                        cwd=ROOT,
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                    self.assertEqual(result.returncode, 2)
                    self.assertIn("no fullclean or extra commands", result.stderr)
                    self.assertNotIn("=== Building", result.stdout)

    def test_factory_profile_requires_explicit_board_and_firmware_only(self):
        for arguments in ([], ["--board", "read_pico"],
                          ["--board", "read_pico", "--step", "splash"],
                          ["--step", "firmware"]):
            with self.subTest(arguments=arguments):
                result = subprocess.run(
                    [sys.executable, str(ROOT / "build.py"),
                     "--read-pico-factory-diagnostic", *arguments],
                    cwd=ROOT, capture_output=True, text=True, check=False,
                )
                self.assertEqual(result.returncode, 2)
                self.assertNotIn("=== Building", result.stdout)

    def test_factory_profile_isolated_and_build_only(self):
        spec = importlib.util.spec_from_file_location("build_under_test", ROOT / "build.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with patch.object(module.subprocess, "run") as run, patch.object(module.os, "makedirs"):
            module.build_firmware("read_pico", [], True)
        self.assertEqual(run.call_count, 1)
        argv = run.call_args.args[0]
        self.assertEqual(argv[-1], "build")
        self.assertIn("build-read-pico-factory", argv)
        self.assertIn("-DSDKCONFIG=.cache/sdkconfig.read_pico-factory", argv)
        self.assertIn("-DSDKCONFIG_DEFAULTS=sdkconfig.defaults;boards/sdkconfig.defaults.read_pico;"
                      "boards/sdkconfig.defaults.read_pico_factory_diagnostic", argv)

    def test_factory_partition_protected_ranges(self):
        lines = (ROOT / "boards/partitions.read_pico_factory.csv").read_text().splitlines()
        rows = [[field.strip() for field in line.split(",")[:5]]
                for line in lines if line.strip() and not line.startswith("#")]
        self.assertEqual(rows, [
            ["nvs", "data", "nvs", "0x9000", "0x5000"],
            ["phy_init", "data", "phy", "0xe000", "0x1000"],
            ["factory", "app", "factory", "0x10000", "0x400000"],
            ["storage", "data", "fat", "0x410000", "0x500000"],
        ])

    def test_display_validation_requires_factory_and_firmware_only(self):
        for arguments in ([], ["--board", "read_pico", "--step", "firmware"],
                          ["--read-pico-factory-diagnostic", "--board", "read_pico"],
                          ["--read-pico-factory-diagnostic", "--step", "firmware"]):
            with self.subTest(arguments=arguments):
                result = subprocess.run(
                    [sys.executable, str(ROOT / "build.py"),
                     "--read-pico-display-validation", *arguments],
                    cwd=ROOT, capture_output=True, text=True, check=False,
                )
                self.assertEqual(result.returncode, 2)
                self.assertNotIn("=== Building", result.stdout)

    def test_display_validation_isolated_build_not_default_entry(self):
        spec = importlib.util.spec_from_file_location("display_build_under_test", ROOT / "build.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with patch.object(module.subprocess, "run") as run, patch.object(module.os, "makedirs"):
            module.build_firmware("read_pico", [], True, True)
        self.assertEqual(run.call_count, 1)
        argv = run.call_args.args[0]
        self.assertEqual(argv[-1], "build")
        self.assertIn("build-read-pico-display", argv)
        self.assertIn("-DSDKCONFIG=.cache/sdkconfig.read_pico-display", argv)
        defaults = next(arg for arg in argv if arg.startswith("-DSDKCONFIG_DEFAULTS="))
        self.assertIn("sdkconfig.defaults.read_pico_factory_diagnostic", defaults)
        self.assertTrue(defaults.endswith("sdkconfig.defaults.read_pico_display_validation"))
        self.assertIn("CONFIG_READ_PICO_DISPLAY_VALIDATION=n",
                      (ROOT / "boards/sdkconfig.defaults.read_pico").read_text())

    def test_display_renderer_profile_matches_vendor_without_clock_or_calibration_changes(self):
        overlay = (ROOT / "boards/sdkconfig.defaults.read_pico_display_validation").read_text()
        vendor = (ROOT / "third_party/read_pico_firmware/sdkconfig.defaults").read_text()
        for flag in ("CONFIG_COMPILER_OPTIMIZATION_PERF=y",
                     "CONFIG_ESP32S3_DATA_CACHE_64KB=y",
                     "CONFIG_ESP32S3_DATA_CACHE_LINE_64B=y"):
            self.assertIn(flag, overlay)
            self.assertIn(flag, vendor)
        flags = [line for line in overlay.splitlines() if line.startswith("CONFIG_")]
        self.assertEqual(len(flags), 4)
        self.assertNotIn("FLASHFREQ", overlay)
        self.assertNotIn("SPIRAM_SPEED", overlay)
        self.assertNotIn("HPM_ENA", overlay)
        base = (ROOT / "boards/sdkconfig.defaults.read_pico").read_text()
        self.assertIn("CONFIG_ESPTOOLPY_FLASHFREQ_80M=y", base)
        self.assertIn("CONFIG_SPIRAM_SPEED_80M=y", base)

    def test_scan_probe_is_display_only_and_preserves_locked_clear_contract(self):
        cmake = (ROOT / "components/epaper_driver_read_pico/CMakeLists.txt").read_text()
        self.assertIn('if(CONFIG_READ_PICO_DISPLAY_VALIDATION)\n'
                      '        list(APPEND READ_PICO_SOURCES "src/read_pico_scan_probe.c" '
                      '"src/read_pico_vendor_gray16.c")', cmake)
        self.assertIn('if(CONFIG_READ_PICO_DISPLAY_VALIDATION)\n'
                      '        target_link_options', cmake)
        self.assertIn('--wrap=epd_lcd_line_source_cb', cmake)
        self.assertIn('--wrap=epd_lcd_frame_done_cb', cmake)
        # The observation budget assumes the locked static clear's 78 frames.
        vendor = (ROOT / "third_party/read_pico_firmware/components/epdiy/src/render.c").read_text()
        self.assertIn('epd_clear_area_cycles(area, 3, clear_cycle_time)', vendor)
        for count in (10, 13, 3):
            self.assertIn(f'for (int i = 0; i < {count}; i++)', vendor)
        lut = (ROOT / "third_party/read_pico_firmware/components/epdiy/src/output_common/lut.h").read_text()
        self.assertIn('#define CLEAR_BYTE 0B10101010', lut)
        self.assertIn('#define DARK_BYTE 0B01010101', lut)
        main = (ROOT / "main/read_pico_display_validation.c").read_text()
        self.assertLess(main.index('read_pico_scan_probe_begin()'),
                        main.index('read_pico_epaper_clear_unverified()'))
        self.assertLess(main.index('read_pico_epaper_clear_unverified()'),
                        main.index('read_pico_scan_probe_end()'))
        self.assertIn('!summary.software_complete', main)

    def test_vendor_control_only_links_in_explicit_display_profile(self):
        cmake = (ROOT / "components/epaper_driver_read_pico/CMakeLists.txt").read_text()
        blocks = re.findall(r'if\(CONFIG_READ_PICO_DISPLAY_VALIDATION\)(.*?)endif\(\)',
                            cmake, re.S)
        helper = '"src/read_pico_vendor_gray16.c"'
        self.assertEqual(sum(helper in block for block in blocks), 1)
        stripped = re.sub(r'if\(CONFIG_READ_PICO_DISPLAY_VALIDATION\).*?endif\(\)',
                          '', cmake, flags=re.S)
        self.assertNotIn(helper, stripped)
        driver = (ROOT / "components/epaper_driver_read_pico/src/driver_read_pico.c").read_text()
        self.assertRegex(driver, r'#if CONFIG_READ_PICO_DISPLAY_VALIDATION\s+'
                                r'read_pico_refresh_result_t read_pico_epaper_display_vendor_gray16')
        render = (ROOT / "components/epaper_driver_read_pico/src/read_pico_vendor_gray16.c").read_text()
        self.assertIn('epd_fill_rect(rect, (uint8_t) (i * 17), frame)', render)
        for unsafe in ('epd_poweron(', 'epd_poweroff(',
                       'epd_set_vcom(', 'read_pico_pmu_vcom_set('):
            self.assertNotIn(unsafe, render)


if __name__ == "__main__":
    unittest.main()
