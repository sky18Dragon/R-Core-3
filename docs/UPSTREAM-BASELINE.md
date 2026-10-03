# 上游源码核对与移植差异

核对日期：2026-10-03。先通过 GitHub API/raw 文件核对，随后已将固定 PhotoFrame 引入根目录、固定 Read Pico 参考子模块，完成主机/Web 测试与原板 v6.1 编译。实际命令见 [开发说明](DEVELOPMENT.md)，结果见 [验证记录](VALIDATION.md)。P1 Read Pico 只读诊断已实现并编译；完整显示与实板验收未完成，下述接口/分区对照仍为固定上游依据。

## 已确认版本

| 仓库 | 本次 main 与报告候选 commit | 提交时间（UTC） |
| --- | --- | --- |
| aitjcize/esp32-photoframe | `f5e3ec974403b31174a3c8da1cf09385453f4abb` | 2026-09-30 10:21:37 |
| MindReset/read_pico_firmware | `28cde682a4468a581c278761922724f57d976418` | 2026-09-29 23:00:08 |

已核验 [PhotoFrame commit](https://api.github.com/repos/aitjcize/esp32-photoframe/commits/f5e3ec974403b31174a3c8da1cf09385453f4abb) 与 [Read Pico commit](https://api.github.com/repos/MindReset/read_pico_firmware/commits/28cde682a4468a581c278761922724f57d976418)，两者 API 均返回相同完整 SHA。本地实际锁定这两个版本；不跟随未来 main 自动升级。

## 构建依据

- PhotoFrame 开发文档要求 ESP-IDF v6.0 或更新；固件 CI 实际使用 `release-v6.0`，并先构建 webapp 和 splash。发布流程使用 Node 20，通用 CI 使用 Node 18；本地版本需在首次复现时记录。[DEV](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/docs/DEV.md)、[build workflow](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/.github/workflows/build.yml)、[CI](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/.github/workflows/ci.yml)
- Read Pico 官方要求 v6.1。已固定正式 tag 的 SDK commit 并编译 PhotoFrame 原板成功；保留原 v6.0 CI，尚未在本机复跑 v6.0。这不等于 Read Pico 已兼容。[官方 DIY](https://dot.mindreset.tech/docs/read_0/firmware)
- PhotoFrame `boards.json` 有 8 个板型，无 Read Pico。M5Paper 条目为原始 ESP32、960×540；不能作为 ESP32-S3/1216×684 的硬件配置直接复用。[boards.json](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/boards/boards.json)
- 图像依赖包括 `esp_jpeg ^1.3.1`、`libpng ^1.6.39`，另有 mDNS/cJSON/qrcode/LittleFS；LittleFS 使用通配版本。实际解析结果已保存根 dependencies.lock，版本表见 THIRD-PARTY，不将范围声明当作确定版本。[组件清单](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/main/idf_component.yml)
- 主机 CMake 使用 C++17、GoogleTest v1.14.0 与系统 libpng。源码存在 7 个测试可执行目标：cron、wake_schedule、network_backoff、http_auth、crash_record、image_pipeline、display_flow；这不是 7 个测试用例，也不是已通过结果。[CMake](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/host_tests/CMakeLists.txt)
- `make test` 还运行 process-cli orientation 测试，但当前 Makefile 未显式运行 wake_schedule_test；应补跑已注册的 CTest，避免遗漏。[Makefile](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/Makefile)

## 已核对的接口差异

| 部分 | 源码事实 | 项目需要补的工作 |
| --- | --- | --- |
| EPD | `epaper_init/display/clear/enter_deepsleep` 返回 void；配置结构以 SPI/IT8951 字段为主 | 独立并口 backend；明确错误传播/可查询状态、刷新串行化，不能将“调用结束”视为成功 |
| Board HAL | 已有电池/充电/USB、RTC/睡眠相关接口；prepare_for_sleep 文档预设随后调用 ESP deep sleep | 扩展外部 PMU 交接，保持其他板行为 |
| SDMMC | 当前 `slot_config.width=4`，挂载失败不自动格式化 | 参数化 1/4-bit；接卡检测/退出，继续禁止自动格式化 |
| SD 所有权 | 厂家 BSP 自带 SD 挂载，目录为 /sdcard；PhotoFrame 保留自己的存储体系 | 避免同时启用两套 SD 初始化；由 PhotoFrame 的存储适配统一拥有挂载 |
| 缓存/显示流程 | 上游已存在 display_flow 与其生命周期测试 | 先复核已有 current/thumbnail 文件，再决定新增缓存，避免另造重复机制 |
| 移植配置 | Read Pico 的总初始化还涉及显示、PMU、触摸、SD/加速度计 | 不能只当作无副作用的 I2C init；选择初始化组合并验证退出顺序 |

接口依据：[epaper.h](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/components/epaper/include/epaper.h)、[board_hal.h](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/components/board_hal/include/board_hal.h)、[SDMMC](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/components/sdcard/src/sdcard_sdio.c)、[Read Pico BSP](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/README.md)、[display_flow](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/main/display_flow.c)。

## 分区差异：首次移植前必须确定

以下来自源码静态分析，不是当前设备回读，也不是已生成的新板配置。

| 区域 | Read Pico Demo | PhotoFrame 公共布局 |
| --- | --- | --- |
| NVS | 0x9000，大小 0x5000 | 0x9000，大小 0x6000 |
| PHY | 0xE000，大小 0x1000 | 0x11000，大小 0x1000 |
| OTA 元数据 | 无对应项 | 0xF000，大小 0x2000 |
| 应用起始 | factory 0x10000 | ota_0 0x20000 |
| 其他 | FAT storage，大小 0x500000 | coredump 0x12000/0xE000；另含 ota_1，内部存储由配置决定 |

PhotoFrame 16 MB、关闭内部存储时，生成器计算每个 OTA 槽 0x780000，第二槽起点 0x7A0000；启用内部存储则采用另一套尺寸。Read Pico 是否开启内部相册存储尚未决定，不能照此直接烧写。[Demo 分区](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/partitions_16M.csv)、[PhotoFrame 生成器](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/scripts/generate_partitions.py)

旧 NVS/PHY 与新布局不完全相容，“不自动擦 NVS”不能靠只沿用旧地址保证。P0/P1 应先输出迁移方案：哪些设置要保留、如何备份/导出、初始化新分区的范围及如何恢复。ESP 的 Flash 备份也不是 PMU 固件或 PMU 校准区备份。

## 已执行的上游验证命令

以下命令已在独立未改上游目录执行，当前根目录也通过主机测试以及分步骤 Web/splash/firmware 构建：

```sh
make test
ctest --test-dir host_tests/build --output-on-failure
python3 build.py --board waveshare_photopainter_73
```

最后一条只验证上游已有板，产物不能用于 Read Pico。当前本地新增第 9 个板型 `read_pico`，仅可按 [P1 说明](READ-PICO-BRINGUP.md) 构建只读诊断，不嵌入 splash；显示/完整业务与实板分区迁移尚未完成。已增加可复跑脚本及 v6.1 基线 CI；环境与未覆盖项见 DEVELOPMENT。

## 来源与许可

PhotoFrame [LICENSE](https://github.com/aitjcize/esp32-photoframe/blob/f5e3ec974403b31174a3c8da1cf09385453f4abb/LICENSE) 声明 MIT；Read Pico 顶层声明 Apache-2.0；其 [EPDiy LICENSE](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/epdiy/LICENSE) 声明 LGPL-3.0-or-later，[波形 LICENSE](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/e0470_epaper_waveform/LICENSE) 声明 Apache-2.0。这里只记录原文件声明；集成时仍逐组件保留通知和修改来源。

固定树检索未发现 PhotoFrame AGENTS.md；Read Pico 根 AGENTS.md、ONBOARDING 和 HANDOFF 已读取。根目录保留本项目规则；子模块未修改，未向上游写入或操作设备。Git 遗留状态的备份/引入过程见 DEVELOPMENT。
