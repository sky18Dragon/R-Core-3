"""Read Pico registration and build-only CLI guard tests; never invoke IDF."""

import json
import re
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
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

    def test_storage_validation_rejects_unsafe_or_mixed_profiles(self):
        valid = ["--board", "read_pico", "--step", "firmware",
                 "--read-pico-factory-diagnostic"]
        cases = [[], ["--board", "read_pico", "--step", "firmware"],
                 ["--step", "firmware", "--read-pico-factory-diagnostic"],
                 valid + ["--read-pico-display-validation"]]
        cases += [valid + [arg] for arg in ("flash", "monitor", "erase-flash", "--fullclean")]
        for arguments in cases:
            with self.subTest(arguments=arguments):
                result = subprocess.run(
                    [sys.executable, str(ROOT / "build.py"),
                     "--read-pico-storage-validation", *arguments],
                    cwd=ROOT, capture_output=True, text=True, check=False,
                )
                self.assertEqual(result.returncode, 2)
                self.assertNotIn("=== Building", result.stdout)

    def test_storage_profile_isolated_build_only(self):
        spec = importlib.util.spec_from_file_location("storage_build_under_test", ROOT / "build.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with patch.object(module.subprocess, "run") as run, patch.object(module.os, "makedirs"):
            module.build_firmware("read_pico", [], True, False, True)
        argv = run.call_args.args[0]
        self.assertEqual(run.call_count, 1)
        self.assertEqual(argv[-1], "build")
        self.assertIn("build-read-pico-storage", argv)
        self.assertIn("-DSDKCONFIG=.cache/sdkconfig.read_pico-storage", argv)
        defaults = next(arg for arg in argv if arg.startswith("-DSDKCONFIG_DEFAULTS="))
        self.assertTrue(defaults.endswith("sdkconfig.defaults.read_pico_storage_validation"))
        self.assertNotIn("sdkconfig.defaults.read_pico_display_validation", defaults)
        with self.assertRaises(ValueError):
            module.build_firmware("read_pico", [], True, True, True)

    def test_full_stack_integration_rejects_unsafe_and_mixed_profiles(self):
        valid = ["--board", "read_pico", "--step", "firmware",
                 "--read-pico-factory-diagnostic"]
        cases = [[], ["--board", "read_pico", "--step", "firmware"],
                 valid + ["--read-pico-display-validation"],
                 valid + ["--read-pico-storage-validation"]]
        cases += [valid + [arg] for arg in ("flash", "monitor", "erase-flash", "--fullclean")]
        for arguments in cases:
            with self.subTest(arguments=arguments):
                result = subprocess.run(
                    [sys.executable, str(ROOT / "build.py"),
                     "--read-pico-photoframe-integration", *arguments],
                    cwd=ROOT, capture_output=True, text=True, check=False)
                self.assertEqual(result.returncode, 2)
                self.assertNotIn("=== Building", result.stdout)

    def test_full_stack_build_retains_real_business_but_withholds_runtime(self):
        spec = importlib.util.spec_from_file_location("integration_build", ROOT / "build.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with patch.object(module.subprocess, "run") as run, patch.object(module.os, "makedirs"):
            module.build_firmware("read_pico", [], True, False, False, True)
        argv = run.call_args.args[0]
        self.assertEqual(run.call_count, 1)
        self.assertEqual(argv[-1], "build")
        self.assertIn("build-read-pico-integration", argv)
        self.assertIn("-DSDKCONFIG=.cache/sdkconfig.read_pico-integration", argv)
        defaults = next(arg for arg in argv if arg.startswith("-DSDKCONFIG_DEFAULTS="))
        self.assertTrue(defaults.endswith("sdkconfig.defaults.read_pico_photoframe_integration"))
        with self.assertRaises(ValueError):
            module.build_firmware("read_pico", [], False, False, False, True)
        with self.assertRaises(ValueError):
            module.build_firmware("read_pico", [], True, True, False, True)
        cmake = (ROOT / "main/CMakeLists.txt").read_text()
        self.assertIn('app_main=photoframe_app_main', cmake)
        for source in ('main.c', 'storage.c', 'image_processor.c', 'http_server.c'):
            self.assertIn('"' + source + '"', cmake)
        entry = (ROOT / "main/read_pico_integration_entry.c").read_text()
        for unsafe in ('product_entry(', 'photoframe_app_main();', 'board_hal_init(',
                       'storage_init(', 'nvs_flash_init(', 'epd_init('):
            self.assertNotIn(unsafe, entry)
        self.assertIn('CONFIG_READ_PICO_PHOTOFRAME_INTEGRATION=n',
                      (ROOT / "boards/sdkconfig.defaults.read_pico").read_text())

    def test_read_pico_business_nvs_and_sleep_fail_closed(self):
        main = (ROOT / "main/main.c").read_text()
        startup = main[main.index('void app_main(void)'):]
        self.assertIn('photoframe_nvs_init(false)', startup)
        self.assertLess(startup.index('photoframe_nvs_init(false)'),
                        startup.index('board_hal_init('))
        self.assertNotIn('nvs_flash_erase(', main)
        power = (ROOT / "main/power_manager.c").read_text()
        sleep = power[power.index('void power_manager_enter_sleep(void)'):]
        guard = sleep[:sleep.index('#endif')]
        self.assertIn('CONFIG_BOARD_DRIVER_READ_PICO', guard)
        self.assertIn('return;', guard)
        self.assertNotIn('storage_unmount(', guard)
        http = (ROOT / "main/http_server.c").read_text()
        reset = http[http.index('static esp_err_t factory_reset_handler('):]
        guard = reset[:reset.index('#else')]
        self.assertIn('CONFIG_BOARD_DRIVER_READ_PICO', guard)
        self.assertIn('501 Not Implemented', guard)
        self.assertIn('return ESP_ERR_NOT_SUPPORTED;', guard)
        self.assertNotIn('nvs_flash_erase(', guard)
        self.assertNotIn('xTaskCreate(', guard)

    def test_read_pico_storage_failure_stops_without_abort_or_fallback(self):
        main = (ROOT / "main/main.c").read_text()
        startup = main[main.index('void app_main(void)'):]
        block = re.search(r'#if CONFIG_BOARD_DRIVER_READ_PICO\n(.*?)#else\n(.*?)#endif',
                          startup[startup.index('Initializing storage subsystem'):], re.S)
        read_pico, legacy = block.group(1), block.group(2)
        self.assertEqual(read_pico.count('storage_init()'), 1)
        self.assertNotIn('ESP_ERROR_CHECK', read_pico)
        self.assertNotIn('abort(', read_pico)
        self.assertIn('if (storage_start != ESP_OK)', read_pico)
        self.assertIn('return;', read_pico)
        for unsafe in ('storage_format(', 'sdcard_format(', 'esp_restart(', 'read_pico_sd_mount(',
                       'memfs', 'littlefs', 'nvs_flash_erase('):
            self.assertNotIn(unsafe, read_pico)
        self.assertIn('ESP_ERROR_CHECK(storage_init());', legacy)
        self.assertLess(startup.index('storage_start != ESP_OK'),
                        startup.index('config_manager_init()'))

    def test_storage_entry_does_not_fall_through_to_product_or_write_paths(self):
        cmake = (ROOT / "main/CMakeLists.txt").read_text()
        block = re.search(r'if\(CONFIG_READ_PICO_STORAGE_VALIDATION\)(.*?)endif\(\)',
                          cmake, re.S).group(1)
        self.assertIn('SRCS "read_pico_storage_validation.c"', block)
        self.assertIn('return()', block)
        self.assertNotIn('"main.c"', block)
        main = (ROOT / "main/read_pico_storage_validation.c").read_text()
        self.assertEqual(main.count('read_pico_sd_mount()'), 1)
        self.assertIn('case READ_PICO_SD_COMMAND_MOUNT:', main)
        for unsafe in ('sdcard_format(', 'nvs_flash_init(', 'nvs_flash_erase(',
                       'esp_wifi_init(', 'fopen(', 'mkdir(', 'epd_init(', 'read_pico_init('):
            self.assertNotIn(unsafe, main)
        self.assertIn('CONFIG_READ_PICO_STORAGE_VALIDATION=n',
                      (ROOT / "boards/sdkconfig.defaults.read_pico").read_text())

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

    def test_display_profile_uses_exact_684_line_overlay_only(self):
        cmake = (ROOT / "CMakeLists.txt").read_text()
        self.assertIn('if("${SDKCONFIG_DEFAULTS}" MATCHES '
                      '"sdkconfig.defaults.read_pico_display_validation")', cmake)
        self.assertIn('"${CMAKE_SOURCE_DIR}/overlays/read_pico_epdiy/epdiy"', cmake)
        self.assertIn('"${CMAKE_SOURCE_DIR}/third_party/read_pico_firmware/components/epdiy"',
                      cmake)
        overlay_cmake = (ROOT / "overlays/read_pico_epdiy/epdiy/CMakeLists.txt").read_text()
        self.assertIn('${CMAKE_CURRENT_LIST_DIR}/../../..', overlay_cmake)
        self.assertNotIn('${CMAKE_SOURCE_DIR}/scripts/', overlay_cmake)

        spec = importlib.util.spec_from_file_location(
            "read_pico_epdiy_overlay",
            ROOT / "scripts/generate_read_pico_epdiy_overlay.py",
        )
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        source_root = ROOT / "third_party/read_pico_firmware/components/epdiy"
        before = {relative: (source_root / relative).read_bytes()
                  for relative in module.EXPECTED_SHA256}
        with tempfile.TemporaryDirectory() as temporary:
            render, lcd = module.generate(source_root, Path(temporary))
            render_text = render.read_text()
            lcd_text = lcd.read_text()
        self.assertIn("((epd_height() + 11) / 12) * 12", render_text)
        self.assertIn("#define BOUNCE_BUF_LINES 6", lcd_text)
        self.assertIn("((display_height + 11) / 12) * 12", lcd_text)
        self.assertEqual(((684 + 11) // 12) * 12, 684)
        self.assertEqual(684 % (2 * 6), 0)
        for relative, contents in before.items():
            self.assertEqual((source_root / relative).read_bytes(), contents)

    def test_exact_line_overlay_rejects_source_drift(self):
        spec = importlib.util.spec_from_file_location(
            "read_pico_epdiy_overlay_drift",
            ROOT / "scripts/generate_read_pico_epdiy_overlay.py",
        )
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        source_root = ROOT / "third_party/read_pico_firmware/components/epdiy"
        with tempfile.TemporaryDirectory() as temporary:
            temporary_root = Path(temporary)
            for relative in module.EXPECTED_SHA256:
                destination = temporary_root / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes((source_root / relative).read_bytes())
            (temporary_root / "src/render.c").write_text("drifted")
            with self.assertRaises(module.OverlayError):
                module.generate(temporary_root, temporary_root / "generated")

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


class PhotoBuildTests(unittest.TestCase):
    def test_read_pico_default_and_diagnostic_match_vendor_upright_portrait(self):
        board = (ROOT / "components/board_hal/include/board_read_pico.h").read_text()
        self.assertIn("#define BOARD_HAL_DISPLAY_ROTATION_DEG 180", board)
        bridge = (ROOT / "main/read_pico_photo_bridge.c").read_text()
        self.assertIn("BOARD_HAL_DISPLAY_ROTATION_DEG == 180", bridge)
        vendor = (ROOT / "third_party/read_pico_firmware/components/read_pico/read_pico_init.c").read_text()
        self.assertIn("epd_set_rotation(EPD_ROT_INVERTED_PORTRAIT)", vendor)

    def test_photo_profile_rejects_mixed_or_unsafe_actions(self):
        valid = ["--board", "read_pico", "--step", "firmware", "--read-pico-factory-diagnostic"]
        cases = [[], ["--board", "read_pico", "--step", "firmware"]]
        cases += [valid + [flag] for flag in ("--read-pico-display-validation", "--read-pico-storage-validation",
                  "--read-pico-photoframe-integration", "flash", "monitor", "erase-flash", "--fullclean")]
        for argv in cases:
            with self.subTest(argv=argv):
                result = subprocess.run([sys.executable, str(ROOT / "build.py"), "--read-pico-photo-validation", *argv],
                                        cwd=ROOT, capture_output=True, text=True, check=False)
                self.assertEqual(result.returncode, 2)
                self.assertNotIn("=== Building", result.stdout)

    def test_photo_build_isolated_compile_only(self):
        spec = importlib.util.spec_from_file_location("photo_build", ROOT / "build.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with patch.object(module.subprocess, "run") as run, patch.object(module.os, "makedirs"):
            module.build_firmware("read_pico", [], True, False, False, False, True)
        self.assertEqual(run.call_count, 1)
        argv = run.call_args.args[0]
        self.assertEqual(argv[-1], "build")
        self.assertIn("build-read-pico-photo", argv)
        self.assertIn("-DSDKCONFIG=.cache/sdkconfig.read_pico-photo", argv)
        defaults = next(x for x in argv if x.startswith("-DSDKCONFIG_DEFAULTS="))
        self.assertTrue(defaults.endswith("sdkconfig.defaults.read_pico_photo_validation"))
        for parameters in ((False, False, False, False), (True, True, False, False),
                           (True, False, True, False), (True, False, False, True)):
            with self.assertRaises(ValueError):
                module.build_firmware("read_pico", [], *parameters, True)

    def test_photo_entry_links_real_processing_without_product_or_auto_hardware(self):
        cmake = (ROOT / "main/CMakeLists.txt").read_text()
        block = re.search(r'if\(CONFIG_READ_PICO_PHOTO_VALIDATION\)(.*?)endif\(\)', cmake, re.S).group(1)
        for source in ("read_pico_photo_validation.c", "image_processor.c", "jpeg_metadata.c", "jpeg_stream_probe.c"):
            self.assertIn('"' + source + '"', block)
        for source in ("main.c", "config_manager.c", "display_manager.c", "http_server.c", "processing_settings.c"):
            self.assertNotIn('"' + source + '"', block)
        self.assertIn("return()", block)
        entry = (ROOT / "main/read_pico_photo_validation.c").read_text()
        startup = entry[entry.index("void app_main(void)"):]
        for unsafe in ("read_pico_sd_mount()", "read_pico_epaper_init(", "read_pico_epaper_clear_unverified(",
                       "image_processor_process_jpeg_file_to_display(", "nvs_flash_init(", "esp_wifi_init("):
            self.assertNotIn(unsafe, startup)
        self.assertIn("image_processor_init_defaults()", startup)
        for unsafe in ("unlink(", "rename(", "mkdir(", "sdcard_format(", "nvs_flash_erase("):
            self.assertNotIn(unsafe, entry)
        self.assertIn("CONFIG_READ_PICO_PHOTO_VALIDATION=n", (ROOT / "boards/sdkconfig.defaults.read_pico").read_text())


if __name__ == "__main__":
    unittest.main()
