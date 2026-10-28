"""R1 build selection tests. Subprocess is mocked; no SDK or device operation."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("local_frame_build", ROOT / "build.py")
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class LocalFrameBuildTests(unittest.TestCase):
    def test_product_display_preparation_is_separate_from_manual_confirmation(self):
        driver = (ROOT / "components/epaper_driver_read_pico/src/driver_read_pico.c").read_text()
        start = driver.index("esp_err_t read_pico_epaper_prepare_local_frame(void)")
        body = driver[start:driver.index("esp_err_t read_pico_epaper_deinit(void)", start)]
        self.assertLess(body.index("read_pico_epaper_init(true)"), body.index("read_pico_epaper_clear_unverified()"))
        self.assertLess(body.index("read_pico_epaper_clear_unverified()"), body.index("accept_white_reference()"))
        self.assertNotIn("read_pico_epaper_confirm_white_reference()", body)
        self.assertIn("if (err == ESP_OK) err = accept_white_reference();", body)
        self.assertIn("read_pico_epaper_deinit()", body)
        main = (ROOT / "main/main.c").read_text()
        startup = main[main.index("esp_err_t display_start = display_manager_init();"):]
        self.assertLess(startup.index("return;"), startup.index("power_manager_init()"))

    def test_runtime_guards_precede_network_and_config_side_effects(self):
        # Wiring evidence only; config getter behavior is executed by the C++ target.
        source = (ROOT / "main/utils.c").read_text()
        config = source[source.index("static esp_err_t apply_config_values("):]
        guard = config[:config.index("// General")]
        self.assertIn("#if CONFIG_READ_PICO_LOCAL_FRAME", guard)
        self.assertIn("if (from_remote) return ESP_ERR_NOT_SUPPORTED", guard)
        self.assertIn('{"image_url", "ha_url"}', guard)
        self.assertIn('"rotation_mode"', guard)
        self.assertIn('"deep_sleep_enabled"', guard)
        fetch = source[source.index("esp_err_t fetch_and_display_image_from_url("):]
        self.assertLess(fetch.index("return ESP_ERR_NOT_SUPPORTED"), fetch.index("malloc("))
        self.assertIn("if (not_modified) *not_modified = false;", fetch[:fetch.index("malloc(")])

    def test_time_sync_failure_does_not_block_local_web_startup(self):
        # Read Pico has no external RTC, so SNTP is forced at every boot. A LAN without
        # Internet must still reach HTTP readiness in the R1 local frame.
        main = (ROOT / "main/main.c").read_text()
        marker = "esp_err_t periodic = periodic_tasks_check_and_run();"
        start = 0
        hits = 0
        while (start := main.find(marker, start)) != -1:
            window = main[start + len(marker):start + 900]
            if window.lstrip().startswith("#if CONFIG_READ_PICO_LOCAL_FRAME"):
                hits += 1
                local = window[window.index("#if CONFIG_READ_PICO_LOCAL_FRAME"):window.index("#else")]
                self.assertIn("ESP_LOGW", local)
                self.assertNotIn("return;", local)
            start += len(marker)
        self.assertEqual(hits, 2)  # boot path and late-WiFi path (wake path is unreachable in R1)
        online = main[main.index("static void startup_online_work(void)"):]
        online = online[:online.index("static void late_wifi_task")]
        guard = online.index("#if CONFIG_READ_PICO_LOCAL_FRAME\n    // OTA is unsupported")
        self.assertLess(guard, online.index("vTaskDelay(pdMS_TO_TICKS(10000))"))
        self.assertLess(guard, online.index("ota_check_for_update(NULL, 0)"))

    def test_card_diagnostic_is_read_only_and_local_frame_only(self):
        source = (ROOT / "components/sdcard/src/sdcard_sdio.c").read_text()
        start = source.index("#if CONFIG_READ_PICO_LOCAL_FRAME\n// Read-only explanation")
        diag = source[start:source.index("esp_err_t sdcard_init(")]
        for forbidden in ("sdmmc_write_sectors", "sdmmc_erase_sectors", "sdcard_format",
                          "esp_vfs_fat_sdcard_format", "f_mkfs", "f_mount", "f_open"):
            self.assertNotIn(forbidden, diag)
        self.assertIn("sdmmc_read_sectors", diag)
        call = source[source.index("Failed to mount filesystem"):]
        call = call[:call.index("} else if")]
        self.assertIn("#if CONFIG_READ_PICO_LOCAL_FRAME\n            diagnose_unmountable_card", call)
        self.assertFalse("format_if_mount_failed = true" in source)

    def test_local_frame_photo_refresh_flashes_white_first(self):
        driver = (ROOT / "components/epaper_driver_read_pico/src/driver_read_pico.c").read_text()
        draw = driver[driver.index("static int draw_frame("):driver.index("static int draw_gc16(")]
        local = draw[draw.index("#if CONFIG_READ_PICO_LOCAL_FRAME"):draw.index("#else", draw.index("#if CONFIG_READ_PICO_LOCAL_FRAME"))]
        self.assertIn("read_pico_white_first_gc16(&hl, target_fb", local)
        self.assertIn("untouched_white", local)
        self.assertNotIn("epd_hl_update_screen_full", local)
        cmake = (ROOT / "components/epaper_driver_read_pico/CMakeLists.txt").read_text()
        self.assertIn('if(CONFIG_READ_PICO_LOCAL_FRAME)\n        list(APPEND READ_PICO_SOURCES "src/read_pico_white_first.c")', cmake)

    def test_separate_output_and_ordered_defaults(self):
        with patch.object(build.subprocess, "run") as run:
            build.build_firmware("read_pico", [], read_pico_local_frame=True)
        argv = run.call_args.args[0]
        self.assertIn("build-read-pico-local-frame", argv)
        self.assertIn("-DSDKCONFIG=.cache/sdkconfig.read_pico-local-frame", argv)
        defaults = next(arg for arg in argv if arg.startswith("-DSDKCONFIG_DEFAULTS="))
        self.assertTrue(defaults.endswith("boards/sdkconfig.defaults.read_pico_local_frame"))
        self.assertLess(defaults.index("read_pico_factory_diagnostic"),
                        defaults.index("read_pico_photoframe_integration"))
        self.assertEqual(argv[-1], "build")
        run.assert_called_once()

    def test_direct_mixed_profiles_rejected_before_build(self):
        for flag in ("read_pico_factory_diagnostic", "read_pico_display_validation",
                     "read_pico_storage_validation", "read_pico_photoframe_integration",
                     "read_pico_photo_validation"):
            with self.subTest(flag=flag), patch.object(build.subprocess, "run") as run:
                with self.assertRaises(ValueError):
                    build.build_firmware("read_pico", [], read_pico_local_frame=True,
                                         **{flag: True})
                run.assert_not_called()
        with self.assertRaises(ValueError):
            build.build_firmware("waveshare_photopainter_73", [], read_pico_local_frame=True)

    def test_cli_refuses_device_actions_and_mixed_profiles(self):
        for extra in (["flash"], ["monitor"], ["erase-flash"], ["--fullclean"],
                      ["--read-pico-factory-diagnostic"], ["--read-pico-photo-validation"],
                      ["--read-pico-photoframe-integration"], ["--step", "webapp"],
                      ["--board", "m5stack_m5paper_v11"]):
            with self.subTest(extra=extra):
                result = subprocess.run(
                    [sys.executable, str(ROOT / "build.py"), "--board", "read_pico",
                     "--step", "firmware", "--read-pico-local-frame", *extra],
                    capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn("Running:", result.stdout)

    def test_cli_dispatches_local_profile(self):
        argv = ["build.py", "--board", "read_pico", "--step", "firmware",
                "--read-pico-local-frame"]
        with patch.object(sys, "argv", argv), patch.object(build, "build_firmware") as call:
            build.main()
        self.assertEqual(call.call_args.args, ("read_pico", [], False, False, False,
                                               False, False, True))


if __name__ == "__main__":
    unittest.main()
