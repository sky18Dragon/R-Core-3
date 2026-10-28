# R1 本地相框：启动路径与运行门禁

范围：`--read-pico-local-frame` 构建（`CONFIG_READ_PICO_LOCAL_FRAME=y`）在 `main/read_pico_integration_entry.c` 的 `app_main` 直接调用 `photoframe_app_main`，其余 integration/诊断 profile 仍不启动产品。本文只描述代码路径与处置；真机行为未验收。2026-10-19 依据 `main/main.c` 源码核对，未运行设备。

## 启动顺序与 R1 处置

| # | 步骤（`photoframe_boot`） | 失败行为 | R1 处置 |
| --- | --- | --- | --- |
| 1 | `photoframe_nvs_init(false)`、`config_manager_init`、Wi-Fi 凭据对检查 | 返回，不擦 NVS、不修复 | 保留；旧 NVS 值不改写 |
| 2 | `board_hal_init` | `ESP_ERROR_CHECK`，会 abort 重启 | 保留；已知风险，见下 |
| 3 | `storage_init`（仅 SD，无回退） | 释放 HAL 后返回，不重试不格式化 | 保留 |
| 4 | `debug_log_init`、`crash_log_capture` | 无 | 保留 |
| 5 | 外部 RTC | Read Pico 无 RTC（`board_hal_rtc_*` 返回 NOT_SUPPORTED），每次启动 `time_restored=false` | 每次强制 SNTP |
| 6 | 周期任务（仅 SNTP，24h）及本次启动强制运行 | 初始化失败返回 | 保留 |
| 7 | `image_processor_init` | `ESP_ERROR_CHECK` | 保留 |
| 8 | `display_manager_init` + `read_pico_epaper_prepare_local_frame`（init → 清白一次 → 软件白参考） | 清理后返回，**不启动 Web 与轮播** | R1 新增；软件参考不是目视证据，见下 |
| 9 | `processing_settings_init`、`color_palette_init`、`power_manager_init` | 返回/abort | 睡眠由 `config_manager_get_deep_sleep_enabled()` 固定 false；轮播定时任务在此创建 |
| 10 | `ota_manager_init`、`album_manager_init` | 返回 | OTA 在 Read Pico 返回 NOT_SUPPORTED，不检查更新 |
| 11 | 唤醒源分支 | 冷启动走默认继续 | 睡眠/定时唤醒路径在 R1 不可达（睡眠禁用） |
| 12 | `wifi_manager_init`、`wifi_provisioning_init`、凭据 | 返回 | 无凭据：读 SD `wifi.txt` 导入或开设备 AP 配网，保存后重启 |
| 13 | `connect_to_wifi` | 成功：继续；超时：`late_wifi_task` 后台等待；失败：AP 重配网（凭据保留） | 保留 |
| 14 | `periodic_tasks_check_and_run`（SNTP）、mDNS | 旧行为：周期批次出错即返回，**HTTP 不启动** | **R1 已改**：时间同步失败只告警并继续（见下） |
| 15 | 按键任务、`http_server_init`、后台就绪门禁（PERIODIC+ROTATION+BUTTON[+LATE_WIFI]） | 取消后台并停 HTTP | 保留 |
| 16 | `startup_online_work`：地址日志、配网完成页、HA 通知、OTA 检查 | 跳过 | HA 地址在 R1 固定为空（通知无操作）；10 秒延时与 OTA 检查在 R1 已去除 |

## 本次发现并修复的阻断项

**时间同步失败使 Web 不启动。** 步骤 5–6 使每次启动都强制 SNTP；`periodic_tasks_check_and_run` 把回调错误原样返回，旧 `app_main`（以及 `late_wifi_task`）据此直接返回，HTTP 永远不发布。路由器未接外网或 NTP 不可达时，本地上传/相册也不可用，直接违反 R1“无云依赖”。修复：`CONFIG_READ_PICO_LOCAL_FRAME` 下该错误改为警告，后台任务与 HTTP 就绪门禁照常检查取消；强制请求保留，由每小时 worker 再试。非 R1 构建保持原行为。证据：`host_tests/test_read_pico_local_frame_build.py::test_time_sync_failure_does_not_block_local_web_startup`（源码接入检查）；未做真实断网实机验证。

## R1 范围外且已关闭的入口

| 入口 | 处置 |
| --- | --- |
| URL 图片源显示 | `fetch_and_display_image_from_url` 在分配/联网前返回 NOT_SUPPORTED |
| 远程配置 / 非空 `image_url`、`ha_url` / 非本地轮播 / 开启睡眠 | `apply_config_values` 在保存任何字段前拒绝 |
| HA 通知 | `config_manager_get_ha_url()` 返回空串 |
| 深睡眠 / PMU 睡眠 | 配置有效值固定 false；睡眠入口另有 PMU 策略门禁 |
| OTA | OTA 检查/升级入口在 Read Pico 返回 NOT_SUPPORTED，R1 不做延时检查 |
| 触摸 | 未实现，属路线图 |

## 已知限制与待实机核对

- **SD 卡格式：** 仅支持 FAT（FAT12/16/32）、MBR 或无分区表；固件的 FatFS 不含 exFAT，64GB 及以上卡出厂/相机格式化常为 exFAT，会挂载失败（FR_NO_FILESYSTEM）并安全停止，需用户换 FAT32 卡。2026-10-20 实机已遇到，见 VALIDATION。
- **启动时清白：** 启动期自动清白一次后才发布 Web（此前诊断清白约 13 秒）；Web 晚于上电可用。软件白参考只表示事务完成，不等于屏已全白。
- **abort 风险：** `board_hal_init`、`image_processor_init`、`processing_settings_init`、`color_palette_init` 失败仍 abort 重启；未在 R1 改动，若实机出现重启循环再处理。
- **轮播相位：** 轮播按 cron/墙钟对齐计算下一次；无外网时时钟停在启动值，间隔可用但相位与真实时间无关，SNTP 成功后重置定时。
- **同机显示与 SD 共存：** 显示租约与 SD 租约在同一产品启动内并存，主机侧为替身；真实供电/退出由 T3 验证。
- **分区：** 候选使用已核验的 `boards/partitions.read_pico_factory.csv`（factory 0x10000/0x400000），镜像 0x1d9dd0 字节，余 54%；app-only 写 0x10000 即可，不碰 NVS/PHY/storage。是否需迁移分区：R1 不需要。
