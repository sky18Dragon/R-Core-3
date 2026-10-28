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
    def test_absolute_redirect_prepares_host_before_scheduling_and_keeps_downgrade_gate(self):
        source = (ROOT / 'main/utils.c').read_text()
        start = source.index('static esp_err_t fetch_prepare_redirect(', source.index('typedef struct {\n    esp_http_client_handle_t client;'))
        helper = source[start:source.index('static esp_err_t fetch_perform_download(', start)]
        self.assertLess(helper.index('esp_http_client_get_transport_type(client)'), helper.index('request_redirect_schedule_checked(&ops)'))
        self.assertIn('strncasecmp(url, "https://", 8)', helper)
        self.assertIn('return ESP_ERR_INVALID_RESPONSE', helper)
        self.assertIn('fetch_redirect_url, fetch_redirect_host, fetch_redirect_schedule', helper)
        self.assertNotIn('strstr(url,', helper)

    def test_initial_host_authority_is_checked_before_perform(self):
        source = (ROOT / 'main/utils.c').read_text()
        helper = source[source.index('static esp_err_t fetch_host_header('):source.index('static esp_err_t fetch_perform_download(')]
        self.assertIn('!strcasecmp(custom_key, "Host")', helper)
        self.assertIn('http_endpoint_host_header(url, host, sizeof(host))', helper)
        self.assertIn('fetch_put_header(client, "Host", host)', helper)
        main = source[source.index('static esp_err_t fetch_perform_download('):source.index('// Fetch the server-provided thumbnail')]
        self.assertLess(main.index('fetch_host_header(client, url, source->header_key)'), main.index('esp_http_client_perform(client)'))
        thumb = source[source.index('static bool fetch_download_thumbnail('):]
        self.assertLess(thumb.index('fetch_host_header(thumb_client, thumbnail_url, source->header_key)'), thumb.index('esp_http_client_perform(thumb_client)'))

    def test_singleton_response_headers_reject_duplicates_before_capture(self):
        source = (ROOT / 'main/utils.c').read_text()
        sent = source[source.index('case HTTP_EVENT_HEADERS_SENT:'):source.index('case HTTP_EVENT_ON_DATA:')]
        for header, field, capture in [('Location', 'location_header_seen', 'strdup('),
                                       ('X-Thumbnail-URL', 'thumbnail_header_seen', 'text_buffer_copy_exact('),
                                       ('ETag', 'etag_header_seen', 'text_buffer_copy_exact(')]:
            self.assertIn(f'ctx->{field} = false;', sent)
            block = source[source.index(f'strcasecmp(evt->header_key, "{header}")'):]
            guard = block[:block.index(capture)]
            self.assertIn(f'text_buffer_claim_once(&ctx->{field})', guard)
            self.assertIn('ctx->owner->failed = true;', guard)
        redirect = source[source.index('case HTTP_EVENT_REDIRECT:'):source.index('case HTTP_EVENT_HEADERS_SENT:')]
        self.assertLess(redirect.index('ctx->owner->failed'), redirect.index('fetch_prepare_redirect('))

    def test_download_endpoint_checks_precede_io_and_metadata_never_truncates(self):
        # Wiring assertions only: this does not execute the HTTP event loop.
        source = (ROOT / 'main/utils.c').read_text()
        main = source[source.index('static esp_err_t fetch_display_from_source('):]
        self.assertLess(main.index('http_endpoint_valid(url)'), main.index('fetch_perform_download('))
        self.assertLess(main.index('http_endpoint_valid(url)'), main.index('ESP_LOGI('))
        thumb = source[source.index('static bool fetch_download_thumbnail('):]
        self.assertLess(thumb.index('http_endpoint_valid(thumbnail_url)'), thumb.index('read_pico_upload_begin('))
        self.assertLess(thumb.index('http_endpoint_valid(thumbnail_url)'), thumb.index('ESP_LOGI('))
        for marker, target, size in [('X-Thumbnail-URL', 'thumbnail_url', '512'),
                                     ('ETag', 'etag', 'HTTP_ETAG_MAX_LEN')]:
            block = source[source.index(f'strcasecmp(evt->header_key, "{marker}")'):]
            checked = block[block.index('#if CONFIG_BOARD_DRIVER_READ_PICO'):block.index('#else')]
            self.assertIn(f'text_buffer_copy_exact(ctx->{target}, {size}, evt->header_value)', checked)
            self.assertIn('ctx->owner->failed = true;', checked)
            self.assertIn('return ESP_ERR_INVALID_RESPONSE;', checked)

    def test_request_config_snapshot_and_header_confirmation_precede_http_perform(self):
        source = (ROOT / 'main/utils.c').read_text()
        download = source[source.index('static esp_err_t fetch_perform_download('):source.index('static esp_err_t fetch_perform_download(') + 21000]
        self.assertLess(download.index('request_config_capture(&request_config)'), download.index('for (int retry'))
        self.assertLess(download.index('request_config_capture(&request_config)'), download.index('read_pico_upload_begin('))
        checked = download[download.index('request_header_ops_t header_ops'):download.index('err = esp_http_client_perform(client)')]
        self.assertIn('request_config_set_headers(&request_config, &header_ops)', checked)
        self.assertIn('esp_http_client_cleanup(client)', checked)
        self.assertIn('upload_file_abort(owner)', checked)
        self.assertIn('break;', checked)
        self.assertLess(download.index('request_config_set_headers('), download.index('esp_http_client_perform('))
        self.assertEqual(download.count('request_config_capture('), 1)
        self.assertEqual(download.count('request_config_release(&request_config)'), 3)
        core = (ROOT / 'main/request_config.c').read_text()
        self.assertIn('config_apply_run(capture, out)', core)
        self.assertNotIn('_get_defaults(', core)
        self.assertLess(core.index('color_palette_load(&palette)'), core.index('processing_settings_to_json('))
        self.assertIn('strcmp(readback, value)', core)
        self.assertIn('ops->remove(ops->context, key)', core)
        cmake = (ROOT / 'host_tests/CMakeLists.txt').read_text()
        self.assertIn('Header tests require reviewed fixed SDK source/headers', cmake)

    def test_all_initial_download_and_thumbnail_headers_stop_on_error(self):
        source = (ROOT / 'main/utils.c').read_text()
        download = source[source.index('static esp_err_t fetch_perform_download('):source.index('// Fetch the server-provided thumbnail')]
        for key in ('Authorization', 'X-Display-Width', 'X-Display-Height', 'X-Display-Orientation', 'X-Firmware-Version', 'If-None-Match', 'X-Battery-Percentage'):
            self.assertIn(f'FETCH_HEADER("{key}"', download)
        self.assertIn('FETCH_HEADER(header_key, header_value)', download)
        self.assertIn('if (header_error == ESP_OK)', download)
        check = download[download.index('if (header_error != ESP_OK)'):download.index('err = esp_http_client_perform(client)')]
        self.assertIn('upload_file_abort(owner)', check)
        self.assertIn('break;', check)
        thumbnail = source[source.index('static bool fetch_download_thumbnail('):source.index('static bool fetch_download_thumbnail(') + 7500]
        self.assertIn('fetch_put_header(thumb_client, "Authorization", thumb_auth)', thumbnail)
        self.assertIn('fetch_put_header(thumb_client, thumb_hk, thumb_hv)', thumbnail)
        guard = thumbnail[thumbnail.index('if (header_error != ESP_OK)'):thumbnail.index('esp_err_t thumb_err = esp_http_client_perform')]
        self.assertIn('upload_file_abort(owner)', guard)
        self.assertIn('return false;', guard)

    def test_redirect_strips_with_absence_confirmation_and_latches_before_next_hop(self):
        source = (ROOT / 'main/utils.c').read_text()
        redirect = source[source.index('case HTTP_EVENT_REDIRECT:'):source.index('case HTTP_EVENT_HEADERS_SENT:')]
        self.assertIn('if (ctx->redirect_error == ESP_OK)', redirect)
        self.assertLess(redirect.index('http_redirect_reference_valid(ctx->redirect_url)'),
                        redirect.index('fetch_strip_credentials('))
        self.assertIn('ctx->redirect_error = fetch_strip_credentials(evt->client, ctx->credential_header)', redirect)
        self.assertNotIn('esp_http_client_delete_header(', redirect)
        self.assertLess(redirect.index('fetch_strip_credentials('), redirect.index('fetch_prepare_redirect('))
        self.assertIn('ctx->owner->failed = true', redirect)
        self.assertIn('return ctx->redirect_error', redirect)
        sent = source[source.index('case HTTP_EVENT_HEADERS_SENT:'):source.index('case HTTP_EVENT_ON_DATA:')]
        self.assertNotIn('redirect_error =', sent)
        download = source[source.index('err = esp_http_client_perform(client)'):source.index('// 304 Not Modified:')]
        self.assertLess(download.index('err = ctx.redirect_error'), download.index('download_file_finish('))
        self.assertIn('if (err == ESP_OK && received == DOWNLOAD_FAILED)', download)
        thumbnail = source[source.index('esp_err_t thumb_err = esp_http_client_perform'):source.index('if (thumb_err == ESP_OK && thumb_status')]
        self.assertLess(thumbnail.index('thumb_err = thumb_ctx.redirect_error'), thumbnail.index('download_file_finish('))
        self.assertIn('if (thumb_err == ESP_OK && received != DOWNLOAD_COMPLETE)', thumbnail)

    def test_image_options_checked_reads_precede_publication_and_streams(self):
        settings = (ROOT / 'main/processing_settings.c').read_text()
        checked = settings[settings.index('static esp_err_t copy_options('):settings.index('dither_algorithm_t processing_settings_get_dithering_algorithm')]
        self.assertIn('config_apply_run(copy_options, out)', checked)
        self.assertIn('if (err != ESP_OK) return err;', checked)
        self.assertNotIn('processing_settings_get_defaults(', checked)
        self.assertLess(checked.index('processing_settings_load(&settings)'), checked.index('*(processing_options_t *)context = candidate'))
        http = (ROOT / 'main/http_server.c').read_text()
        display = http[http.index('static esp_err_t display_received_image('):http.index('static esp_err_t display_received_image(') + 1900]
        self.assertLess(display.index('processing_settings_copy_options(&options)'), display.index('display_flow_present_asset('))
        self.assertIn('return response == ESP_OK ? err : response;', display)
        utils = (ROOT / 'main/utils.c').read_text()
        owned = utils[utils.index('static esp_err_t fetch_present_owned('):utils.index('esp_err_t fetch_and_display_image_from_url(')]
        self.assertLess(owned.index('processing_settings_copy_options(&options)'), owned.index('display_flow_publish_asset('))
        self.assertNotIn('processing_settings_get_dithering_algorithm()', owned)
        image = (ROOT / 'main/image_processor.c').read_text()
        memory = image[image.index('esp_err_t image_processor_process_to_display('):image.index('esp_err_t image_processor_process_jpeg_file_to_display(')]
        self.assertLess(memory.index('processing_settings_copy_options(&options)'), memory.index('display_manager_begin_rgb_stream()'))
        file = image[image.index('esp_err_t image_processor_process(const char *'):image.index('esp_err_t image_processor_process(const char *') + 8000]
        self.assertLess(file.index('processing_settings_copy_options(&options)'), file.index('png_writer_open('))
        bridge = (ROOT / 'main/read_pico_photo_bridge.c').read_text()
        self.assertIn('esp_err_t processing_settings_copy_options(', bridge)
        self.assertNotIn('processing_settings_load(', bridge)
        self.assertNotIn('config_apply_run(', bridge)

    def test_checked_name_and_network_json_stop_before_unconfirmed_side_effects(self):
        source = (ROOT / 'main/utils.c').read_text()
        name = source[source.index('item = cJSON_GetObjectItem(root, "device_name")'):source.index('// Only the shape')]
        self.assertIn('config_manager_apply_device_name_checked(new_name, &operations, &result)', name)
        self.assertIn('result.name_accepted ?', name)
        self.assertIn('return name_error;', name)
        self.assertIn('return mdns_service_update_hostname_named(name)', source)
        self.assertIn('return wifi_manager_update_hostname_named(name)', source)
        start = source.index('wifi_network_t network_patch;')
        network = source[start:source.index('#else', start)]
        self.assertIn('wifi_network_json_patch(root,', network)
        self.assertIn('config_manager_patch_network_checked(network_fields, &network_patch)', network)
        self.assertIn('return network_error;', network)
        self.assertNotIn('config_manager_set_static_', network)

    def test_name_and_network_consumers_copy_before_json_or_sdk_work(self):
        source = (ROOT / 'main/http_server.c').read_text()
        start = source.index('static esp_err_t config_handler(httpd_req_t *req)')
        block = source[start:source.index('cJSON *root = cJSON_CreateObject();', start)]
        self.assertIn('config_manager_copy_device_name(', block)
        self.assertIn('config_manager_copy_network(', block)
        mdns = (ROOT / 'main/mdns_service.c').read_text()
        start = mdns.index('esp_err_t mdns_service_update_hostname(void)')
        block = mdns[start:]
        self.assertLess(block.index('config_manager_copy_device_name('), block.index('mdns_free();'))
        self.assertIn('mdns_register_services(name)', block)
        for path in ('main.c', 'ha_integration.c'):
            self.assertIn('config_manager_copy_device_name(', (ROOT / 'main' / path).read_text())

    def test_general_config_adapter_serializes_all_returns_and_busy_does_not_consume_errors(self):
        source = (ROOT / 'main/utils.c').read_text()
        start = source.index('esp_err_t apply_config_from_json(cJSON *root, bool from_remote)')
        block = source[start:source.index('typedef struct {', start)]
        self.assertIn('if (!cJSON_IsObject(root)) return ESP_ERR_INVALID_ARG', block)
        self.assertIn('return config_apply_run(apply_general_request, &request)', block)
        self.assertIn('return apply_config_values(root, from_remote)', block)
        http = (ROOT / 'main/http_server.c').read_text()
        start = http.index('if (apply_result == ESP_ERR_INVALID_STATE && !outcome.state.entered)')
        busy = http[start:http.index('#endif', start)]
        self.assertIn('HTTPD_503', busy)
        self.assertNotIn('utils_consume_', busy)

    def test_all_configuration_mutations_hold_control_through_revision_and_own_local_errors(self):
        utils = (ROOT / 'main/utils.c').read_text()
        local = utils[utils.index('static esp_err_t apply_local_values'):utils.index('// Context for HTTP event handler')]
        self.assertIn('memcpy(request->result->config_error, last_config_error', local)
        self.assertIn('memcpy(request->result->cert_error, last_cert_pin_error', local)
        self.assertIn('config_apply_transaction(apply_local_values, finish_local_config', local)
        self.assertIn('return config_manager_touch_config_checked()', local)
        remote = utils[utils.index('static esp_err_t apply_remote_group'):utils.index('#else\nstatic void fetch_apply_remote_config')]
        self.assertIn('config_apply_run(apply_remote_request, &request)', remote)
        self.assertNotIn('apply_config_from_json(', remote)
        http = (ROOT / 'main/http_server.c').read_text()
        self.assertIn('apply_local_config_checked(root, &outcome)', http)
        self.assertIn('const char *message = outcome.config_error[0]', http)
        self.assertIn('return sent == ESP_OK ? apply_result : sent', http)
        self.assertIn('if (!json_str) return ESP_ERR_NO_MEM', http)
        self.assertIn('if (outcome.state.applied && !outcome.state.finished)', http)
        self.assertIn('if (request.entered) return error;', http)
        for name in ('processing_settings', 'color_palette'):
            self.assertIn(f'return run_http_config_guarded(req, {name}_handler_values)', http)

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
        entry = entry.split("#else", 1)[1]  # Integration branch; R1 has a separate executable test.
        for unsafe in ('product_entry(', 'photoframe_app_main();', 'board_hal_init(',
                       'storage_init(', 'nvs_flash_init(', 'epd_init('):
            self.assertNotIn(unsafe, entry)
        self.assertIn('CONFIG_READ_PICO_PHOTOFRAME_INTEGRATION=n',
                      (ROOT / "boards/sdkconfig.defaults.read_pico").read_text())

    def test_read_pico_file_lifetime_wrappers_are_isolated_to_sd_profiles(self):
        cmake = (ROOT / "components/board_hal/CMakeLists.txt").read_text()
        calls = re.search(r'foreach\(FS_CALL ([^)]+)\)', cmake).group(1).split()
        self.assertEqual(set(calls), {'fopen', 'fdopen', 'fclose', 'open', 'close',
                                     'opendir', 'closedir', 'stat', 'mkdir', 'unlink', 'rename'})
        guard = cmake[cmake.rindex('if(CONFIG_BOARD_DRIVER_READ_PICO AND'):]
        for profile in ('CONFIG_READ_PICO_STORAGE_VALIDATION',
                        'CONFIG_READ_PICO_PHOTOFRAME_INTEGRATION',
                        'CONFIG_READ_PICO_PHOTO_VALIDATION'):
            self.assertIn(profile, guard)
        self.assertNotIn('CONFIG_READ_PICO_DISPLAY_VALIDATION', guard)
        self.assertIn('target_link_options(${COMPONENT_LIB} INTERFACE', guard)
        self.assertIn('src/read_pico_fs.c', cmake)

    def test_read_pico_snapshot_refuses_overwrite_and_keeps_uncertain_output(self):
        source = (ROOT / "main/display_manager.c").read_text()
        snapshot = source[source.index('static esp_err_t display_save_frame_epdgz('):
                          source.index('esp_err_t display_manager_push_rgb_column(')]
        self.assertIn('read_pico_upload_begin(&snapshot', snapshot)
        self.assertIn('upload_file_finish(&snapshot, snapshot.bytes)', snapshot)
        self.assertIn('upload_file_publish(&snapshot, snapshot.source, path)', snapshot)
        self.assertIn('upload_file_abort(&snapshot)', snapshot)
        adapter = (ROOT / 'main/read_pico_upload.c').read_text()
        self.assertIn('O_WRONLY | O_CREAT | O_EXCL', adapter)
        self.assertIn('return fsync(fd)', adapter)
        self.assertIn('read_pico_sd_note_file_close_failure()', adapter)
        self.assertIn('read_pico_sd_get_status(&status)', snapshot)
        self.assertRegex(snapshot, r'#if !CONFIG_BOARD_DRIVER_READ_PICO\s+unlink\(path\);')

    def test_read_only_file_check_observes_handle_lease_without_new_io_capabilities(self):
        source = (ROOT / "main/read_pico_storage_validation.c").read_text()
        check = source[source.index('static void run_file_check('):
                       source.index('typedef struct { probe_context_t errors;')]
        self.assertIn('.verify_lease = true', check)
        self.assertIn('sd_file_probe_check(session, &io)', check)
        for unsafe in ('probe_open_new', 'probe_write', 'probe_sync', 'sdcard_deinit(',
                       'read_pico_sd_unmount(', 'unlink(', 'rename('):
            self.assertNotIn(unsafe, check)
        self.assertIn('status.active_io == 1', source)
        self.assertIn('status.active_io == 0', source)
        self.assertIn('open(SD_FILE_PROBE_PATH, O_RDONLY)', source)

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
        self.assertLess(startup.index('config_manager_init()'), startup.index('storage_start != ESP_OK'))
        self.assertLess(startup.index('storage_start != ESP_OK'),
                        startup.index('ESP_ERROR_CHECK(config_manager_init())')) # Legacy order is unchanged.

    def test_config_read_failure_stops_before_network_and_never_panics_into_repair(self):
        source = (ROOT / "main/main.c").read_text()
        startup = source[source.index('void app_main(void)'):]
        start = startup.index('esp_err_t config_start = config_manager_init();')
        protected = startup[start:startup.index('#endif', start)]
        self.assertIn('if (config_start != ESP_OK)', protected)
        self.assertLess(startup.index('if (config_start != ESP_OK)'), startup.index('board_hal_init('))
        self.assertLess(startup.index('if (config_start != ESP_OK)'), startup.index('storage_init('))
        self.assertIn('return;', protected)
        for unsafe in ('ESP_ERROR_CHECK', 'esp_restart(', 'nvs_erase', 'storage_format(',
                       'wifi_manager_init(', 'display_manager_init(', 'board_hal_init(', 'storage_init('):
            self.assertNotIn(unsafe, protected)
        config = (ROOT / "main/config_manager.c").read_text()
        self.assertIn('cron_load_from_joined(rule); // Preview in RAM', config)
        self.assertRegex(config, r'#if !CONFIG_BOARD_DRIVER_READ_PICO\s+\{\s+nvs_handle_t erase_handle;')
        self.assertIn('return guard.error;', config)
        self.assertIn('config_read_string(g, err, out, capacity, *size)', config)

    def test_read_pico_ota_is_report_only_not_a_migration_shortcut(self):
        source = (ROOT / "main/ota_manager.c").read_text()
        start = source.index('#if CONFIG_BOARD_DRIVER_READ_PICO', source.index('esp_err_t ota_manager_init'))
        guarded = source[start:source.index('#else', start)]
        self.assertIn('ota_status.state = OTA_STATE_ERROR;', guarded)
        self.assertIn('return ESP_OK;', guarded)
        for unsafe in ('esp_https_ota_begin(', 'ota_load_status_from_nvs(', 'periodic_tasks_register(',
                       'esp_ota_mark_app_valid_cancel_rollback(', 'xTaskCreate('):
            self.assertNotIn(unsafe, guarded)
        for function in ('ota_check_for_update', 'ota_start_update'):
            start = source.index('#if CONFIG_BOARD_DRIVER_READ_PICO', source.index('esp_err_t ' + function))
            self.assertIn('return ESP_ERR_NOT_SUPPORTED;', source[start:source.index('#else', start)])
        entry = (ROOT / 'main/read_pico_integration_entry.c').read_text()
        entry = entry.split('#else', 1)[1]
        self.assertNotIn('product_entry();', entry)
        self.assertNotIn('photoframe_app_main();', entry)

    def test_wifi_text_import_is_complete_read_only_and_saved_errors_do_not_start_ap(self):
        source = (ROOT / 'main/storage.c').read_text()
        start = source.index('esp_err_t storage_read_wifi_config(')
        checked = source[start:source.index('#else', start)]
        for required in ('fstat(', 'fgetc(fp) != EOF', '!feof(fp)', 'ferror(fp)', 'fclose(fp)',
                         'read_pico_sd_note_file_close_failure()', 'errno != ENOENT', 'wifi_text_parse('):
            self.assertIn(required, checked)
        for forbidden in ('config_manager_set_device_name(', 'unlink(', 'rename(', 'nvs_'):
            self.assertNotIn(forbidden, checked)
        main = (ROOT / 'main/main.c').read_text()
        self.assertIn('import_result != ESP_OK && import_result != ESP_ERR_NOT_FOUND', main)
        self.assertIn('config_manager_set_device_name_checked(imported.device_name)', main)
        self.assertIn('possible partial NVS result', main)

    def test_wifi_recovery_retains_credentials_and_waits_for_this_session_not_old_nvs(self):
        source = (ROOT / 'main/main.c').read_text()
        start = source.index('static void recover_wifi_connection')
        checked = source[start:source.index('#else', start)]
        for forbidden in ('nvs_erase', 'nvs_commit', 'wifi_provisioning_is_provisioned('):
            self.assertNotIn(forbidden, checked)
        self.assertIn('http_server_stop()', checked)
        self.assertIn('while (!wifi_provisioning_session_complete())', checked)
        self.assertIn('wifi_provisioning_start_ap()', checked)
        provision = (ROOT / 'main/wifi_provisioning.c').read_text()
        self.assertIn('while (ret < req->content_len)', provision)
        self.assertIn('wifi_text_form_valid((const uint8_t *)buf, ret)', provision)
        self.assertIn('provisioning_session_commit(ticket)', provision)
        ap = (ROOT / 'main/wifi_provisioning_ap.c').read_text()
        self.assertIn('AP_REQUIRE(dns_server_start())', ap)
        self.assertIn('wifi_provisioning_ap_start(get_setup_ap_ssid(), register_provisioning_routes)', provision)
        self.assertIn('AP_REGISTER(&save_uri)', provision)
        manager = (ROOT / 'main/wifi_manager.c').read_text()
        self.assertIn('password && *password ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN', manager)

    def test_wifi_candidate_uses_checked_manager_not_stale_event_bits_or_driver_flash(self):
        source = (ROOT / 'main/wifi_manager.c').read_text()
        start = source.index('static esp_err_t checked_connect')
        checked = source[start:source.index('esp_err_t wifi_manager_test_credentials', start)]
        self.assertIn('esp_event_post(READ_PICO_WIFI_FENCE', checked)
        self.assertIn('current_link_matches()', checked)
        self.assertLess(checked.index('xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT'),
                        checked.index('candidate ? esp_wifi_connect() : esp_wifi_start()'))
        self.assertNotIn('ESP_ERROR_CHECK', checked)
        self.assertIn('cfg.nvs_enable = 0', source)
        self.assertIn('esp_wifi_set_storage(WIFI_STORAGE_RAM)', source)
        self.assertIn('IP_EVENT_STA_LOST_IP', source)
        self.assertIn('esp_err_t dns = apply_dns_override()', source)
        self.assertIn('s_link.ready = s_is_connected = false', source)
        self.assertIn('ret = ESP_ERR_INVALID_SIZE', source)
        provision = (ROOT / 'main/wifi_provisioning.c').read_text()
        self.assertIn('wifi_manager_test_network_owned(request->ssid, request->password, &request->network,', provision)
        self.assertIn('Candidate connection could not be verified; configuration not saved', provision)

    def test_wifi_credential_errors_are_not_missing_configuration_or_panic_recovery(self):
        source = (ROOT / 'main/main.c').read_text()
        self.assertIn('credential_check != ESP_OK && credential_check != ESP_ERR_NVS_NOT_FOUND', source)
        self.assertIn('if (credentials_failed) return;', source)
        self.assertIn('provision_state != ESP_OK && provision_state != ESP_ERR_NVS_NOT_FOUND', source)
        self.assertIn('no import/reprovisioning/erase/restart', source)
        credentials = (ROOT / 'main/wifi_credentials.c').read_text()
        self.assertIn('sr != ESP_OK || pr != ESP_OK', credentials)
        self.assertIn('config_read_string(&guard', credentials)
        self.assertNotIn('nvs_erase', credentials)
        cmake = (ROOT / 'main/CMakeLists.txt').read_text()
        self.assertIn('"wifi_credentials.c"', cmake)

    def test_online_startup_checks_current_address_before_logging_or_nvs(self):
        source = (ROOT / 'main/main.c').read_text()
        start = source.index('static void startup_online_work')
        checked = source[start:source.index('// Interactive boot whose connect timed out', start)]
        self.assertIn('char ip_str[16] = {0}', checked)
        self.assertIn('address != ESP_OK', checked)
        self.assertLess(checked.index('return;'), checked.index('Web interface available'))
        self.assertLess(checked.index('return;'), checked.index('nvs_open('))

    def test_candidate_parameters_are_temporary_and_group_save_is_checked(self):
        source = (ROOT / 'main/wifi_provisioning.c').read_text()
        start = source.index('wifi_network_t candidate_network, retained_network;')
        temporary = source[start:source.index('#else', start)]
        self.assertIn('wifi_network_copy(', temporary)
        self.assertNotIn('config_manager_set_', temporary)
        self.assertIn('provisioning_apply_run(&request, &ops, &context)', source)
        self.assertIn('wifi_manager_cancel_candidate(context.candidate)', source)
        self.assertIn('config_manager_save_provisioning_checked(request->ssid', source)
        manager = (ROOT / 'main/wifi_manager.c').read_text()
        self.assertIn('s_network = network', manager)
        self.assertIn('const char *dns = s_network.dns', manager)
        config = (ROOT / 'main/config_manager.c').read_text()
        start = config.index('esp_err_t config_manager_save_provisioning_checked')
        checked = config[start:config.index('#else', start)]
        self.assertIn('if (err == ESP_OK) err = nvs_commit(handle)', checked)
        self.assertLess(checked.index('nvs_close(handle)'), checked.index('memcpy(wifi_ssid'))
        self.assertNotIn('nvs_erase', checked)

    def test_provision_scan_uses_checked_results_and_json_library(self):
        source = (ROOT / 'main/wifi_provisioning.c').read_text()
        start = source.index('static esp_err_t provision_scan_handler')
        scan = source[start:source.index('// Extract one field', start)]
        self.assertIn('wifi_manager_scan_checked(ap_records, MAX_SCAN_RESULTS, &count)', scan)
        self.assertIn('if (scanned != ESP_OK)', scan)
        self.assertIn('cJSON_PrintUnformatted(array)', scan)
        self.assertIn('cJSON_AddStringToObject(entry, "ssid"', scan)
        self.assertIn('cJSON_free(json_buf)', scan)
        self.assertIn('return response;', scan)
        manager = (ROOT / 'main/wifi_manager.c').read_text()
        start = manager.index('esp_err_t wifi_manager_scan_checked')
        checked = manager[start:manager.index('int wifi_manager_scan(', start)]
        self.assertIn('take_radio_operation(true)', checked)
        self.assertIn('esp_wifi_clear_ap_list()', checked)
        self.assertNotIn('ESP_ERROR_CHECK', checked)
        cmake = (ROOT / 'host_tests/CMakeLists.txt').read_text()
        self.assertIn('RESOURCE_LOCK host_display_flow_storage', cmake)

    def test_wifi_initialization_checks_owned_resources_without_panic_helpers(self):
        source = (ROOT / 'main/wifi_manager.c').read_text()
        start = source.index('esp_err_t wifi_manager_init(void)')
        checked = source[start:source.index('#else', start)]
        self.assertIn('atomic_flag_test_and_set(&s_init_busy)', checked)
        self.assertIn('if (s_initialized)', checked)
        self.assertIn('esp_netif_new(&sta_config)', checked)
        self.assertIn('esp_netif_attach_wifi_station(s_sta_netif)', checked)
        self.assertIn('cleanup_initialization()', checked)
        self.assertNotIn('ESP_ERROR_CHECK', checked)
        self.assertNotIn('esp_netif_create_default_wifi_', checked)
        self.assertNotIn('esp_wifi_start', checked)
        self.assertNotIn('esp_event_loop_delete', source)
        main = (ROOT / 'main/main.c').read_text()
        self.assertEqual(main.count('esp_err_t wifi_init = wifi_manager_init();'), 2)
        self.assertIn('Wake WiFi initialization failed', main)
        self.assertIn('no provisioning/network restart', main)

    def test_dns_startup_binds_before_worker_and_stop_waits_for_owner(self):
        source = (ROOT / 'main/dns_server.c').read_text()
        checked = source[:source.index('#else')]
        start = checked.index('esp_err_t dns_server_start(void)')
        startup = checked[start:checked.index('esp_err_t dns_server_stop_checked')]
        self.assertLess(startup.index('bind(fd'), startup.index('xTaskCreate('))
        self.assertIn('SO_RCVTIMEO', startup)
        self.assertIn('SO_SNDTIMEO', startup)
        self.assertIn('htonl(0xc0a80401)', startup)
        self.assertIn('CAPTIVE_DNS_MAX_PACKET + 1', checked)
        self.assertIn('captive_dns_response(query', checked)
        stop = checked[checked.index('esp_err_t dns_server_stop_checked'):]
        self.assertIn('xEventGroupWaitBits', stop)
        self.assertNotIn('close(', stop)
        self.assertNotIn('vTaskDelete', stop)
        ap = (ROOT / 'main/wifi_provisioning_ap.c').read_text()
        stop = ap[ap.index('static esp_err_t stop_owned'):ap.index('esp_err_t wifi_provisioning_ap_start')]
        self.assertIn('error = dns_server_stop_checked();', stop)
        self.assertIn('if (error != ESP_OK) goto failed;', stop)
        self.assertIn('error != ESP_ERR_WIFI_NOT_STARTED', stop)
        self.assertLess(stop.index('dns_server_stop_checked'), stop.index('read_pico_http_stop'))
        self.assertIn('"captive_dns.c"', (ROOT / 'main/CMakeLists.txt').read_text())

    def test_provisioning_lifecycle_owns_sdk_objects_and_invalidates_stopped_sessions(self):
        ap = (ROOT / 'main/wifi_provisioning_ap.c').read_text()
        self.assertIn('atomic_flag_test_and_set(&s_control)', ap)
        self.assertIn('wifi_manager_provisioning_begin(&s_manager_owner, &netif)', ap)
        self.assertIn('ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED', ap)
        self.assertIn('provisioning_session_cancel()', ap)
        start = ap[ap.index('esp_err_t wifi_provisioning_ap_start'):]
        self.assertLess(start.index('AP_REQUIRE(dns_server_start())'),
                        start.index('provisioning_session_begin()'))
        self.assertLess(start.index('provisioning_session_begin()'), start.index('s_state = AP_ACTIVE'))
        self.assertNotIn('ESP_ERROR_CHECK', ap)
        for forbidden in ('nvs_', 'esp_restart', 'esp_netif_destroy', 'vTaskDelete'):
            self.assertNotIn(forbidden, ap)
        provision = (ROOT / 'main/wifi_provisioning.c').read_text()
        self.assertIn('wifi_provisioning_ap_active() && provisioning_session_ticket() == ticket', provision)
        self.assertIn('if (!wifi_provisioning_ap_active())', provision)
        self.assertIn('return wifi_provisioning_ap_stop();', provision)
        routes = provision[provision.index('static esp_err_t register_provisioning_routes'):
                           provision.index('esp_err_t wifi_provisioning_start_ap')]
        self.assertEqual(re.findall(r'\.uri\s*=\s*"([^"]+)"', routes), [
            '/', '/provision', '/assets/index.css', '/assets/index.js', '/assets/index2.js',
            '/assets/exif-reader.js', '/assets/browser.js', '/assets/__vite-browser-external.js',
            '/icon.svg', '/save', '/api/wifi/scan', '/api/keep_alive', '/hotspot-detect.html',
            '/generate_204', '/connecttest.txt'])
        self.assertEqual(routes.count('AP_REGISTER(&'), 15)
        self.assertIn('if (error != ESP_OK) return error;', routes)
        self.assertIn('return httpd_register_err_handler', routes)
        self.assertIn('"wifi_provisioning_ap.c"', (ROOT / 'main/CMakeLists.txt').read_text())
        main = (ROOT / 'main/main.c').read_text()
        self.assertEqual(main.count('esp_err_t ap_stopped = wifi_provisioning_stop_ap();'), 2)
        self.assertEqual(main.count('if (!wifi_provisioning_is_active())'), 2)
        recovery = main[main.index('static void recover_wifi_connection'):]
        recovery = recovery[:recovery.index('#else')]
        self.assertLess(recovery.index('wifi_provisioning_stop_ap()'), recovery.index('esp_restart()'))
        self.assertIn('if (ap_stopped != ESP_OK)', recovery)
        self.assertIn('no automatic restart', recovery)

    def test_radio_reservation_and_checked_stop_are_wired_into_existing_services(self):
        manager = (ROOT / 'main/wifi_manager.c').read_text()
        self.assertIn('s_ap_task != xTaskGetCurrentTaskHandle()', manager)
        self.assertIn('if (held) xSemaphoreGive(s_connection_lock)', manager)
        self.assertIn('s_ap_serial == UINT32_MAX', manager)
        quiesce = manager[manager.index('esp_err_t wifi_manager_provisioning_quiesce'):]
        quiesce = quiesce[:quiesce.index('#else')]
        self.assertLess(quiesce.index('cancel_link_locked()'),
                        quiesce.index('xSemaphoreTake(s_connection_lock, 0)'))
        ap = (ROOT / 'main/wifi_provisioning_ap.c').read_text()
        stop = ap[ap.index('static esp_err_t stop_owned'):ap.index('esp_err_t wifi_provisioning_ap_start')]
        self.assertLess(stop.index('wifi_manager_provisioning_quiesce'), stop.index('dns_server_stop_checked'))
        self.assertIn('WIFI_PROVISION_READY', ap)
        self.assertIn('WIFI_PROVISION_FAILED', ap)
        main = (ROOT / 'main/main.c').read_text()
        self.assertEqual(main.count('wifi_manager_stop_connecting_checked()'), 3)
        self.assertIn('no provisioning/restart', main)
        self.assertIn('no further wake work', main)

    def test_frame_http_routes_preserve_every_uri_method_and_auth_wrapper(self):
        source = (ROOT / 'main/http_server.c').read_text()
        routes = source[source.index('static esp_err_t register_http_routes'):
                        source.index('esp_err_t http_server_init')]
        self.assertEqual(re.findall(r'HTTP_REGISTER\("([^"]+)",\s*(HTTP_\w+)', routes), [
            ("/", "HTTP_GET"),
            ("/assets/index.css", "HTTP_GET"),
            ("/assets/index.js", "HTTP_GET"),
            ("/assets/index2.js", "HTTP_GET"),
            ("/assets/exif-reader.js", "HTTP_GET"),
            ("/assets/browser.js", "HTTP_GET"),
            ("/assets/__vite-browser-external.js", "HTTP_GET"),
            ("/icon.svg", "HTTP_GET"),
            ("/measurement_sample.jpg", "HTTP_GET"),
            ("/api/rotate", "HTTP_POST"),
            ("/api/current_image", "HTTP_GET"),
            ("/api/config", "HTTP_GET"),
            ("/api/config", "HTTP_POST"),
            ("/api/config", "HTTP_PATCH"),
            ("/api/debug/log", "HTTP_GET"),
            ("/api/debug/log", "HTTP_DELETE"),
            ("/api/crash/clear", "HTTP_POST"),
            ("/api/battery", "HTTP_GET"),
            ("/api/sensor", "HTTP_GET"),
            ("/api/sleep", "HTTP_POST"),
            ("/api/system-info", "HTTP_GET"),
            ("/api/time", "HTTP_GET"),
            ("/api/touch", "HTTP_GET"),  # CONFIG_READ_PICO_TOUCH only
            ("/api/time/sync", "HTTP_POST"),
            ("/api/ota/status", "HTTP_GET"),
            ("/api/ota/check", "HTTP_POST"),
            ("/api/ota/update", "HTTP_POST"),
            ("/api/ota/upload", "HTTP_POST"),  # CONFIG_READ_PICO_LOCAL_FRAME only
            ("/api/ota/local", "HTTP_GET"),
            ("/api/keep_alive", "HTTP_POST"),
            ("/api/format-storage", "HTTP_POST"),
            ("/api/display-image", "HTTP_POST"),
            ("/api/albums", "HTTP_GET"),
            ("/api/albums", "HTTP_POST"),
            ("/api/albums", "HTTP_DELETE"),
            ("/api/albums/enabled", "HTTP_PUT"),
            ("/api/images", "HTTP_GET"),
            ("/api/upload", "HTTP_POST"),
            ("/api/display", "HTTP_POST"),
            ("/api/delete", "HTTP_POST"),
            ("/api/image", "HTTP_GET"),
            ("/api/settings/processing", "HTTP_GET"),
            ("/api/settings/processing", "HTTP_POST"),
            ("/api/settings/processing", "HTTP_DELETE"),
            ("/api/settings/palette", "HTTP_GET"),
            ("/api/settings/palette", "HTTP_POST"),
            ("/api/settings/palette", "HTTP_DELETE"),
            ("/api/factory-reset", "HTTP_POST"),
            ("/api/calibration/display", "HTTP_POST"),
        ])
        self.assertIn('if (error != ESP_OK) return error;', routes)
        self.assertIn('.handler = auth_gate', source)
        self.assertIn('.user_ctx = (void *) handler', source)
        auth = source[source.index('static esp_err_t auth_gate'):source.index('// Register a route')]
        self.assertLess(auth.index('http_requests_ready(req)'), auth.index('http_auth_check(req'))
        self.assertIn('READ_PICO_HTTP_FRAME, req->handle', source)

    def test_shared_http_ownership_is_used_before_ap_radio_and_checked_main_startup(self):
        ap = (ROOT / 'main/wifi_provisioning_ap.c').read_text()
        start = ap[ap.index('esp_err_t wifi_provisioning_ap_start'):]
        self.assertLess(start.index('read_pico_http_claim'), start.index('wifi_manager_provisioning_begin'))
        stop = ap[ap.index('static esp_err_t stop_owned'):ap.index('esp_err_t wifi_provisioning_ap_start')]
        self.assertLess(stop.index('read_pico_http_stop'), stop.index('esp_wifi_stop'))
        self.assertLess(stop.index('esp_wifi_stop'), stop.index('read_pico_http_release'))
        source = (ROOT / 'main/http_server.c').read_text()
        self.assertIn('atomic_flag_test_and_set(&s_http_control)', source)
        self.assertIn('read_pico_http_claim(READ_PICO_HTTP_FRAME)', source)
        self.assertIn('read_pico_http_active(READ_PICO_HTTP_FRAME)', source)
        self.assertIn('read_pico_http_mark_ready(READ_PICO_HTTP_FRAME)', source)
        service = (ROOT / 'main/read_pico_http_service.c').read_text()
        self.assertIn('SERVICE_UNCERTAIN', service)
        self.assertIn('if (s_role != role)', service)
        for forbidden in ('nvs_', 'esp_restart', 'vTaskDelete', 'esp_wifi_'):
            self.assertNotIn(forbidden, service)
        main = (ROOT / 'main/main.c').read_text()
        self.assertEqual(main.count('if (ensure_http_server_running() != ESP_OK) return;'), 2)
        self.assertIn('no startup success or late WiFi task', main)
        self.assertIn('Online startup skipped: HTTP not ready', main)
        self.assertIn('"read_pico_http_service.c"', (ROOT / 'main/CMakeLists.txt').read_text())

    def test_background_creation_and_publication_are_checked_in_main_and_power(self):
        main = (ROOT / 'main/main.c').read_text()
        startup = main[main.index('esp_err_t background_start ='):]
        self.assertLess(startup.index('read_pico_background_add(READ_PICO_BACKGROUND_LATE_WIFI'),
                        startup.index('http_server_init()'))
        self.assertLess(startup.index('read_pico_background_activate(required, http_server_set_ready_checked)'),
                        startup.index('PhotoFrame started successfully'))
        self.assertIn('Background creation failed', startup)
        self.assertIn('esp_err_t http_exit = http_server_stop()', startup)
        wrapper = main[main.rindex('void app_main(void)'):]
        self.assertIn('photoframe_boot();', wrapper)
        self.assertIn('read_pico_background_cancel();', wrapper)
        self.assertIn('not whole-device cleanup', wrapper)
        power = (ROOT / 'main/power_manager.c').read_text()
        for role in ('SLEEP', 'ROTATION'):
            self.assertIn('read_pico_background_add(READ_PICO_BACKGROUND_' + role, power)
        self.assertEqual(power.count('if (!read_pico_background_running() || !http_server_is_ready()) return;'), 2)
        self.assertIn('esp_err_t power_start = power_manager_init()', main)
        self.assertIn('"read_pico_background.c"', (ROOT / 'main/CMakeLists.txt').read_text())

    def test_background_recovery_waits_before_radio_and_callbacks_return_to_owner(self):
        main = (ROOT / 'main/main.c').read_text()
        recovery = main[main.index('static void recover_wifi_connection'):main.index('static void startup_online_work')]
        self.assertLess(recovery.index('read_pico_background_cancel_and_wait'),
                        recovery.index('wifi_manager_stop_connecting_checked'))
        self.assertIn('if (background != ESP_OK)', recovery)
        late = main[main.index('static void late_wifi_task'):main.index('static void button_task')]
        self.assertIn('pdFALSE, pdFALSE, pdMS_TO_TICKS(100)', late)
        self.assertIn('if (!events) return;', late)
        self.assertIn('#if !CONFIG_BOARD_DRIVER_READ_PICO\n    vTaskDelete(NULL);', late)
        adapter = (ROOT / 'main/read_pico_background.c').read_text()
        self.assertEqual(adapter.count('vTaskDelete('), 1)
        self.assertIn('vTaskDelete(NULL)', adapter)
        for forbidden in ('nvs_', 'esp_restart(', 'esp_wifi_', 'free(', 'vTaskSuspend('):
            self.assertNotIn(forbidden, adapter)

    def test_declined_read_pico_sleep_does_not_fall_through_and_boot_exit_waits(self):
        main = (ROOT / 'main/main.c').read_text()
        wake = main[main.index('void deep_sleep_wake_main'):main.index('static void photoframe_boot')]
        self.assertEqual(wake.count('power_manager_enter_sleep();\n#if CONFIG_BOARD_DRIVER_READ_PICO\n        return;'), 4)
        self.assertIn('deep_sleep_wake_main(wakeup_src);\n#if CONFIG_BOARD_DRIVER_READ_PICO\n        return;', main)
        self.assertIn('power_manager_enter_sleep();  // Go back to sleep\n#if CONFIG_BOARD_DRIVER_READ_PICO\n        return;', main)
        wrapper = main[main.rindex('void app_main(void)'):]
        self.assertLess(wrapper.index('read_pico_background_cancel_and_wait'), wrapper.index('http_server_stop()'))
        self.assertLess(wrapper.index('if (exited != ESP_OK)'), wrapper.index('http_server_stop()'))
        self.assertIn('if (http_server_is_ready())', wrapper)

    def test_read_pico_periodic_uses_owned_timer_and_managed_worker_not_timer_business_io(self):
        source = (ROOT / 'main/read_pico_periodic.c').read_text()
        timer = source[source.index('static void timer_callback'):source.index('static void periodic_worker')]
        for forbidden in ('nvs_', 'vTaskDelay', 'ESP_LOG', 'periodic_tasks_check_and_run', 'esp_sntp_'):
            self.assertNotIn(forbidden, timer)
        self.assertIn('READ_PICO_BACKGROUND_PERIODIC, periodic_worker', source)
        self.assertIn('esp_timer_stop_blocking(s_timer, pdMS_TO_TICKS(100))', source)
        self.assertLess(source.index('esp_timer_stop_blocking'), source.index('esp_timer_delete(s_timer)'))
        self.assertNotIn('nvs_erase', source)
        self.assertNotIn('memset(s_tasks', source)
        self.assertIn('list(REMOVE_ITEM SOURCES "periodic_tasks.c")', (ROOT / 'main/CMakeLists.txt').read_text())

    def test_periodic_cancellation_is_before_recovery_and_boot_return_resource_cleanup(self):
        main = (ROOT / 'main/main.c').read_text()
        connect = main[main.index('static esp_err_t connect_to_wifi'):main.index('static void recover_wifi_connection')]
        self.assertNotIn('periodic_tasks_request_stop', connect)
        recover = main[main.index('static void recover_wifi_connection'):main.index('static void startup_online_work')]
        self.assertLess(recover.index('periodic_tasks_request_stop'), recover.index('read_pico_background_cancel_and_wait'))
        self.assertLess(recover.index('periodic_tasks_stop()'), recover.index('wifi_manager_stop_connecting_checked()'))
        wrapper = main[main.rindex('void app_main(void)'):]
        self.assertLess(wrapper.index('periodic_tasks_request_stop'), wrapper.index('read_pico_background_cancel_and_wait'))
        self.assertLess(wrapper.index('periodic_tasks_stop()'), wrapper.index('http_server_stop()'))
        self.assertIn('READ_PICO_BACKGROUND_BIT(READ_PICO_BACKGROUND_PERIODIC)', main)
        startup = main[main.index('esp_err_t periodic_start ='):]
        self.assertLess(startup.index('periodic_tasks_register'), startup.index('periodic_tasks_force_run'))

    def test_ntp_save_error_precedes_sync_and_legacy_setter_is_preserved(self):
        utils = (ROOT / 'main/utils.c').read_text()
        block = utils[utils.index('item = cJSON_GetObjectItem(root, "ntp_server")'):utils.index('// Clients PATCH')]
        self.assertIn('item && !cJSON_IsString(item)', block)
        self.assertLess(block.index('config_manager_set_ntp_server_checked'), block.index('periodic_tasks_run_now'))
        self.assertLess(block.index('return saved;'), block.index('periodic_tasks_run_now'))
        self.assertIn('NTP save not confirmed; synchronization not started', block)
        self.assertIn('#else\n        config_manager_set_ntp_server', block)

    def test_config_get_copies_ntp_before_json_and_checks_snapshot_failure(self):
        http = (ROOT / 'main/http_server.c').read_text()
        block = http[http.index('static esp_err_t config_handler'):http.index('static esp_err_t time_sync_handler')]
        self.assertLess(block.index('config_manager_copy_ntp_server'), block.index('cJSON_CreateObject'))
        self.assertIn('if (copied != ESP_OK)', block)
        self.assertIn('Configuration snapshot unavailable', block)
        self.assertIn('const char *ntp_server = ntp_snapshot;', block)
        self.assertIn('if (!cJSON_AddStringToObject(root, "ntp_server", ntp_server))', block)
        self.assertIn('cJSON_Delete(root);', block)
        main = (ROOT / 'main/main.c').read_text()
        sync = main[main.index('static esp_err_t sntp_sync_periodic_callback'):main.index('static esp_err_t connect_to_wifi')]
        checked = sync[:sync.index('#else')]
        self.assertIn('config_manager_copy_ntp_server(server, sizeof(server))', checked)
        self.assertNotIn('config_manager_get_ntp_server()', checked)

    def test_manual_sync_checks_atomic_run_and_sntp_owned_service_result(self):
        main = (ROOT / 'main/main.c').read_text()
        sync = main[main.index('static esp_err_t sntp_sync_periodic_callback'):main.index('static esp_err_t connect_to_wifi')]
        self.assertIn('read_pico_sntp_sync(server, sntp_allowed, periodic_tasks_is_active, &now)', sync)
        self.assertIn('if (synced != ESP_OK) return synced;', sync)
        self.assertIn('return ret;', sync)
        http = (ROOT / 'main/http_server.c').read_text()
        handler = http[http.index('static esp_err_t time_sync_handler'):http.index('static esp_err_t color_palette_handler')]
        self.assertIn('periodic_tasks_run_now(SNTP_TASK_NAME)', handler)
        self.assertLess(handler.index('if (sync != ESP_OK)'), handler.index('"success"'))
        self.assertIn('timestamp persistence not confirmed', handler)
        utils = (ROOT / 'main/utils.c').read_text()
        self.assertIn('esp_err_t sync = periodic_tasks_run_now(SNTP_TASK_NAME)', utils)
        self.assertIn('NTP configuration changed, but synchronization not confirmed', utils)
        response = http[http.index('static esp_err_t send_current_time_checked'):http.index('static esp_err_t time_handler')]
        self.assertIn('return send_current_time_checked(req, "success")', handler)
        self.assertIn('if (!json_str)', response)
        self.assertIn('return sent;', response)

    def test_timezone_save_reports_partial_application_and_does_not_apply_twice(self):
        utils = (ROOT / 'main/utils.c').read_text()
        block = utils[utils.index('item = cJSON_GetObjectItem(root, "timezone")'):utils.index('// Advanced network settings (#43):', utils.index('item = cJSON_GetObjectItem(root, "timezone")'))]
        self.assertIn('item && !cJSON_IsString(item)', block)
        self.assertIn('tz_err == ESP_ERR_NOT_FINISHED', block)
        self.assertIn('Time zone saved, but system application not confirmed', block)
        self.assertIn('#if !CONFIG_BOARD_DRIVER_READ_PICO\n        setenv', block)
        manager = (ROOT / 'main/config_manager.c').read_text()
        setter = manager[manager.index('esp_err_t config_manager_set_timezone'):manager.index('const char *config_manager_get_timezone')]
        self.assertLess(setter.index('nvs_commit(handle)'), setter.index('memcpy(tz_string, candidate'))
        self.assertLess(setter.index('nvs_close(handle)'), setter.index('err = apply_loaded_timezone()'))
        self.assertIn('time_config_control', setter)

    def test_remote_config_checks_stages_and_aborts_before_display_or_etag_claim(self):
        utils = (ROOT / 'main/utils.c').read_text()
        checked = utils[utils.index('static esp_err_t apply_remote_group'):utils.index('#else\nstatic void fetch_apply_remote_config')]
        self.assertIn('return apply_config_values(group, true)', checked)
        self.assertIn('return processing_settings_save(&settings)', checked)
        self.assertLess(checked.index('if (saved != ESP_OK) return saved'), checked.index('return image_processor_reload_palette()'))
        self.assertIn('return config_manager_touch_config_checked()', checked)
        self.assertIn('remote_config_apply(request->payload, &ops, &request->result)', checked)
        outer = utils[utils.index('bool thumbnail_downloaded = false;', utils.index('char fetched_url[')):utils.index('esp_err_t trigger_image_rotation')]
        self.assertLess(outer.index('if (applied != ESP_OK)'), outer.index('fetch_present_owned'))
        self.assertLess(outer.index('return applied;'), outer.index('config_manager_set_image_etag'))
        self.assertNotIn('unlink(', outer.split('#else', 1)[0])

    def test_remote_headers_refuse_truncation_duplicates_and_304_keeps_config(self):
        utils = (ROOT / 'main/utils.c').read_text()
        event = utils[utils.index('case HTTP_EVENT_HEADERS_SENT'):utils.index('case HTTP_EVENT_ON_DATA')]
        self.assertIn('ctx->config_header_seen = false', event)
        self.assertIn('remote_config_capture(ctx->config_payload, REMOTE_CONFIG_CAPACITY', utils)
        not_modified = utils[utils.index('if (err == ESP_OK && status_code == 304)'):utils.index('// Check if download was successful')]
        self.assertIn('*config_payload_out = config_payload_buffer', not_modified)
        outer = utils[utils.index('if (was_not_modified)'):utils.index('bool thumbnail_downloaded = false;', utils.index('if (was_not_modified)'))]
        self.assertLess(outer.index('fetch_apply_remote_config'), outer.index('*not_modified = true'))
        self.assertIn('if (applied != ESP_OK)', outer)

    def test_embedded_web_assets_revalidate_after_firmware_update(self):
        # Fixed asset names are embedded in the app image; without revalidation a
        # browser kept the old index.js after an app-only update (2026-10-25).
        for path in ('main/http_server.c', 'main/wifi_provisioning.c'):
            source = (ROOT / path).read_text()
            helper = source[source.index('static esp_err_t send_webapp_asset('):]
            helper = helper[:helper.index('\n}\n')]
            self.assertIn('httpd_resp_set_hdr(req, "Content-Encoding", "gzip")', helper, path)
            self.assertIn('httpd_resp_set_hdr(req, "Cache-Control", "no-cache")', helper, path)
            self.assertIn('if (err != ESP_OK) return err;', helper, path)
            self.assertLess(helper.index('"Cache-Control", "no-cache"'), helper.index('httpd_resp_send('), path)
            self.assertGreaterEqual(source.count('return send_webapp_asset(req, '), 8, path)
        http = (ROOT / 'main/http_server.c').read_text()
        self.assertEqual(http.count('"Content-Encoding", "gzip"'), 1)  # only through the helper
        self.assertIn('"Cache-Control", "public, max-age=3600"', http)  # image thumbnails keep caching

    def test_http_revision_and_palette_load_errors_precede_success(self):
        http = (ROOT / 'main/http_server.c').read_text()
        helper = http[http.index('static esp_err_t send_revision_error'):http.index('static esp_err_t config_handler')]
        self.assertIn('config_manager_touch_config_checked()', helper)
        self.assertIn('Configuration changed, but revision save not confirmed', helper)
        self.assertEqual(http.count('esp_err_t revision = touch_config_for_http(req);'), 5)
        palette = http[http.index('static esp_err_t color_palette_handler'):http.index('static esp_err_t register_http_routes')]
        # GET palette + GET gray ramp, POST reload, DELETE reload.
        self.assertEqual(palette.count('if (loaded != ESP_OK)'), 4)
        self.assertIn('color_palette_save_gray_levels(NULL)', palette)
        manager = (ROOT / 'main/config_manager.c').read_text()
        revision = manager[manager.index('esp_err_t config_manager_touch_config_checked(void)'):]
        self.assertIn('candidate = config_last_updated + 1', revision)
        self.assertIn('config_last_updated == INT64_MAX', revision)
        processing = (ROOT / 'main/processing_settings.c').read_text()
        self.assertIn('if (err != ESP_OK) goto close', processing)
        self.assertEqual(processing.count('SAVE_FIELD(nvs_set_'), 13)
        palette_source = (ROOT / 'main/color_palette.c').read_text()
        self.assertIn('if (err == ESP_OK) *palette = candidate;', palette_source)
        self.assertIn('size != sizeof(candidate)', palette_source)
        host = (ROOT / 'host_tests/CMakeLists.txt').read_text()
        self.assertIn('READ_PICO_TEST_FETCH_CJSON OR NOT EXISTS', host)
        self.assertIn('GIT_TAG b2890c8d76bbb64e710585ebc0a917196b9c67e7', host)
        self.assertIn('607e756460fa0de37d20a7a9181f2de29c97bfb7ce5a0e6c2f548243836cd852', host)
        self.assertIn('25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee', host)

    def test_configuration_http_uses_bounded_complete_documents_and_checked_group_converters(self):
        http = (ROOT / 'main/http_server.c').read_text()
        helper = http[http.index('static esp_err_t receive_config_document'):http.index('static esp_err_t send_revision_error')]
        self.assertLess(helper.index('req->content_len >= CONFIG_DOCUMENT_CAPACITY'), helper.index('malloc('))
        self.assertLess(helper.index('config_document_receive('), helper.index('config_document_parse('))
        self.assertIn('if (error == ESP_OK) error = config_document_parse', helper)
        self.assertEqual(http.count('receive_config_document(req,'), 3)
        self.assertIn('req->method != HTTP_GET && req->method != HTTP_POST', http)
        for prefix in ('processing_settings', 'color_palette'):
            start = http.index(f'static esp_err_t {prefix}_handler_values')
            block = http[start:http.index(f'static esp_err_t {prefix}_handler(', start)]
            self.assertIn(f'{prefix}_from_json_checked(json,', block)
            self.assertIn('return send_settings_json(req,', block)
            self.assertIn('return send_settings_failure(req,', block)
        remote = (ROOT / 'main/remote_config.c').read_text()
        self.assertIn('config_document_parse(payload, length, &root)', remote)
        utils = (ROOT / 'main/utils.c').read_text()
        block = utils[utils.index('static esp_err_t apply_remote_group'):utils.index('static esp_err_t finish_remote_config')]
        self.assertIn('processing_settings_from_json_checked(group,', block)
        self.assertIn('color_palette_from_json_checked(group,', block)

    def test_processing_load_only_missing_records_default_and_serializers_check_additions(self):
        source = (ROOT / 'main/processing_settings.c').read_text()
        load = source[source.index('esp_err_t processing_settings_load'):source.index('esp_err_t processing_settings_from_json_checked')]
        checked = load.split('#else', 1)[0]
        self.assertIn('if (err == ESP_ERR_NVS_NOT_FOUND)', checked)
        self.assertIn('if (err != ESP_OK) goto close', checked)
        self.assertIn('if (err == ESP_OK) *settings = candidate', checked)
        self.assertIn('if (compress > 1)', checked)
        self.assertIn('JSON_ADD(cJSON_AddStringToObject', source)
        palette = (ROOT / 'main/color_palette.c').read_text()
        self.assertIn('!cJSON_AddItemToObject(json, names[i], color)', palette)
        self.assertIn('cJSON_Delete(color); cJSON_Delete(json); return NULL', palette)

    def test_time_http_uses_checked_single_snapshot_and_preserves_legacy_getter(self):
        http = (ROOT / 'main/http_server.c').read_text()
        response = http[http.index('static esp_err_t send_current_time_checked'):http.index('static esp_err_t time_handler')]
        self.assertIn('config_manager_copy_local_time', response)
        self.assertIn('if (copied != ESP_OK) return send_time_unavailable(req)', response)
        self.assertIn('!cJSON_AddStringToObject(response, "timezone", timezone)', response)
        self.assertNotIn('config_manager_get_timezone()', response)
        config = http[http.index('static esp_err_t config_handler'):http.index('static esp_err_t time_handler')]
        self.assertIn('config_manager_copy_timezone(timezone_snapshot', config)
        self.assertIn('"timezone_applied", timezone_applied', config)
        self.assertIn('config_manager_get_timezone()', config) # Legacy only.
        sync = http[http.index('static esp_err_t time_sync_handler'):http.index('static esp_err_t color_palette_handler')]
        self.assertLess(sync.index('http_requests_ready(req)'), sync.index('periodic_tasks_run_now'))

    def test_owned_sntp_stop_checks_pending_resolution_before_network_cleanup(self):
        main = (ROOT / 'main/main.c').read_text()
        recover = main[main.index('static void recover_wifi_connection'):main.index('static void log_wall_clock', main.index('static void recover_wifi_connection'))]
        self.assertLess(recover.index('read_pico_sntp_request_stop()'), recover.index('read_pico_background_cancel_and_wait'))
        self.assertLess(recover.index('read_pico_sntp_stop()'), recover.index('wifi_manager_stop_connecting_checked()'))
        wrapper = main[main.rindex('void app_main(void)'):]
        self.assertLess(wrapper.index('read_pico_sntp_stop()'), wrapper.index('http_server_stop()'))
        self.assertIn('if (sntp_exit != ESP_OK)', wrapper)
        source = (ROOT / 'main/read_pico_sntp.c').read_text()
        self.assertIn('if (!atomic_load(&core_ack))', source)
        self.assertIn('result = ESP_ERR_NOT_FINISHED', source)
        self.assertIn('sntp_setservername(0, NULL)', source)
        self.assertNotIn('esp_sntp_init()', source)
        self.assertNotIn('esp_sntp_stop()', source)
        self.assertNotIn('free(', source)

    def test_sntp_correlation_applies_to_sdk_build_and_real_service_wiring(self):
        root = (ROOT / 'CMakeLists.txt').read_text()
        self.assertIn('if(CONFIG_BOARD_DRIVER_READ_PICO AND CONFIG_READ_PICO_PHOTOFRAME_INTEGRATION)', root)
        self.assertIn('target_compile_definitions(${read_pico_lwip_lib} PRIVATE SNTP_CHECK_RESPONSE=2 SNTP_COMP_ROUNDTRIP=0)', root)
        main = (ROOT / 'main/CMakeLists.txt').read_text()
        self.assertIn('"read_pico_sntp.c" "sntp_stamp.c"', main)
        self.assertIn('--wrap=sntp_get_system_time', main)
        self.assertIn('--wrap=udp_new_ip_type', main)
        source = (ROOT / 'main/read_pico_sntp.c').read_text()
        self.assertIn('void sntp_sync_time(struct timeval *tv)', source)
        self.assertIn('settimeofday(tv, NULL) == 0', source)
        self.assertIn('if (closed != ESP_OK) result = closed;', source)
        self.assertLess(source.index('close_owned_service();', source.index('cleanup:')), source.index('*confirmed = applied_time'))
        self.assertIn('ticket != atomic_load(&dns_ticket)', source)
        self.assertIn('reserved_pcb = __real_udp_new_ip_type(IPADDR_TYPE_ANY)', source)
        self.assertIn('return ESP_ERR_NO_MEM;', source)
        self.assertIn('if (!handing_pcb) return __real_udp_new_ip_type(type)', source)
        self.assertIn('#if SNTP_CHECK_RESPONSE < 2 || SNTP_COMP_ROUNDTRIP != 0', source)

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

    def test_ota_partition_table_appends_without_moving_factory_layout(self):
        def rows(name):
            lines = (ROOT / name).read_text().splitlines()
            return [[field.strip() for field in line.split(",")[:5]]
                    for line in lines if line.strip() and not line.startswith("#")]
        factory, ota = rows("boards/partitions.read_pico_factory.csv"), rows("boards/partitions.read_pico_ota.csv")
        self.assertEqual(ota[:len(factory)], factory)
        self.assertEqual([r[:3] for r in ota[len(factory):]], [
            ["otadata", "data", "ota"], ["ota_0", "app", "ota_0"], ["ota_1", "app", "ota_1"]])
        end = 0
        for name, _, _, offset, size in ota:
            start = int(offset, 16)
            self.assertGreaterEqual(start, end, name)
            end = start + int(size, 16)
        self.assertLessEqual(end, 16 * 1024 * 1024)
        defaults = (ROOT / "boards/sdkconfig.defaults.read_pico_local_frame").read_text()
        self.assertIn('CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="boards/partitions.read_pico_ota.csv"', defaults)

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


    def test_checked_upload_keeps_legacy_board_and_exclusive_new_paths(self):
        http = (ROOT / "main/http_server.c").read_text()
        checked = http[http.index("static esp_err_t parse_multipart_checked"):http.index("#endif", http.index("static esp_err_t parse_multipart_checked"))]
        self.assertIn("upload_stream_finish(parser)", checked)
        self.assertIn("discard_upload_files(result)", checked)  # own temporaries only
        self.assertIn('"415 Unsupported Media Type"', checked)
        start = checked.index("if (!ok && context.bad_type)")
        drain = checked[start:checked.index("free(parser); free(buffer);", start)]
        self.assertIn("esp_timer_get_time() < deadline", drain)  # bounded drain before replying
        self.assertNotIn("unlink(", checked)
        raw = http[http.index("static esp_err_t receive_raw_checked"):http.index("#else", http.index("static esp_err_t receive_raw_checked"))]
        self.assertIn("upload_file_finish(&file, req->content_len)", raw)
        self.assertNotIn("unlink(", raw)
        self.assertIn("++timeouts < 8", raw)
        self.assertIn("120000000", raw)
        publication = http[http.index("// Both parts are closed/synced"):http.index("#else", http.index("// Both parts are closed/synced"))]
        self.assertEqual(publication.count("upload_file_publish("), 2)
        self.assertNotIn("unlink(", publication)
        self.assertIn('"409 Conflict"', publication)
        adapter = (ROOT / "main/read_pico_upload.c").read_text()
        self.assertIn("O_WRONLY | O_CREAT | O_EXCL", adapter)
        self.assertIn("retry < 16", adapter)
        # The only deletion: this request's own hidden .part temporary, path-checked first.
        self.assertEqual(adapter.count("unlink("), 1)
        remover = adapter[adapter.index("static int remove_temp("):]
        remover = remover[:remover.index("\n}\n")]
        self.assertLess(remover.index("!owned_path(path) || !temp_name(path)"), remover.index("unlink(path)"))
        self.assertIn('"/.rc3-upload-"', adapter)
        self.assertIn('".part"', adapter)
        helper = http[http.index("static void discard_upload_files("):]
        self.assertIn("upload_file_discard(&result->files[i])", helper[:helper.index("\n}\n")])

    def test_owned_display_url_and_snapshot_never_return_to_fixed_slot_cleanup(self):
        flow = (ROOT / "main/display_flow.c").read_text()
        owned = flow[flow.index("esp_err_t display_flow_publish_asset"):flow.index("FILE *display_flow_open_current")]
        self.assertIn("upload_asset_publish(", owned)
        self.assertIn("image_processor_detect_format(image->source)", owned)
        self.assertNotIn("unlink(", owned)
        http = (ROOT / "main/http_server.c").read_text()
        self.assertIn("display_flow_present_asset(image_owner, thumbnail_owner", http)
        self.assertIn("*owner = file", http)
        url = (ROOT / "main/utils.c").read_text()
        checked = url[url.index("static esp_err_t fetch_present_owned"):url.index("#endif", url.index("static esp_err_t fetch_present_owned"))]
        for unsafe in ("unlink(", "rename(", "CURRENT_UPLOAD_PATH", "display_flow_read_file("):
            self.assertNotIn(unsafe, checked)
        self.assertIn("fetch_copy_new", checked)
        self.assertIn('"%s/download_%.*s%s"', checked)
        self.assertIn("download_file_finish(owner", url)
        self.assertIn(".disable_auto_redirect = true", url)
        self.assertIn("download_same_origin(image_url, thumbnail_url)", url)
        manager = (ROOT / "main/display_manager.c").read_text()
        self.assertIn("upload_file_publish(&snapshot, snapshot.source, path)", manager)
        self.assertIn("display_manager_copy_current_image", manager)
        self.assertIn("if (!create_image_link(record)) result = ESP_ERR_NOT_FINISHED", manager)

    def test_synthetic_upload_probe_not_boot_or_product(self):
        cmake = (ROOT / "main/CMakeLists.txt").read_text()
        self.assertIn('"upload_file.c" "read_pico_upload.c" "upload_probe.c"', cmake)
        probe = (ROOT / "main/upload_probe.c").read_text()
        self.assertIn("s->attempted = true", probe)
        self.assertIn("errno != EEXIST", probe)
        for unsafe in ("unlink(", "remove(", "format(", "nvs_", "epaper", "esp_wifi"):
            self.assertNotIn(unsafe, probe)
        constants = (ROOT / "main/upload_probe.h").read_text()
        for path in ("/storage/.RC3U0001.PART", "/storage/.RC3U0002.PART", "/storage/RC3U0001.BIN"):
            self.assertIn(path, constants)
        entry = (ROOT / "main/read_pico_storage_validation.c").read_text()
        startup = entry[entry.index("void app_main(void)"):entry.index("while (!stopped)")]
        self.assertNotIn("run_upload_probe(", startup)
        self.assertIn("probe_session.close_failed |= upload_session.close_failed", entry)

if __name__ == "__main__":
    unittest.main()
