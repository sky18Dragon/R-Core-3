> **历史快照：2026-10-08文档整理前。** 来源 `docs/READ-PICO-BRINGUP.md`，原文字节SHA256 `9371373333d8fa545a82f08d264021d116dffa003a74f9a8eae2fc29d20a8d13`。以下“当前/最新/下一步/许可”只描述当时；不作实时状态或新操作指令。原文完整保留，仅相对Markdown链接为快照目录重定位。当前任务见 [TODO](../../../../TODO.md)。

# Read Pico P1：只读板级诊断

本文描述普通只读入口，不是相框。factory 诊断已实机启动，PMU/VCOM/PSRAM 通过；P2b 后端在此入口仍不运行。另有明确授权的 [受控显示入口](READ-PICO-DISPLAY.md)，已完成一次清白/人工确认/显式退出，但尚未验收 GC16。所有入口均未启用 SD/触摸/Wi-Fi/OTA/睡眠/完整业务；厂家驱动、波形和校准保持原样。

诊断入口先核验 `init(false)` 被拒绝及 16 像素 nibble 内存交换，再执行 P1 观测；不启动 EPDiy 或真实刷新。已链接原生显示代码用于检查依赖，存在符号不代表运行；见 [显示适配说明](READ-PICO-DISPLAY.md)。

## 构建与隔离

激活 DEVELOPMENT 中固定的 ESP-IDF v6.1.0 后：

```sh
python build.py --board read_pico --step firmware
```

输出到 `build-read-pico/`，配置到 `.cache/sdkconfig.read_pico`，与原板 `build/` / `sdkconfig` 分开。诊断 main 不嵌入 Web/splash，因此不必先生成这些资源。共享的生成分区表目前仍是原版 16 MB 双 OTA 编译布局，**不是已经批准的实板迁移方案**。

本设备已核实为 factory 布局。对应专用编译命令为 `python build.py --board read_pico --step firmware --read-pico-factory-diagnostic`；输出 `build-read-pico-factory/`，配置 `.cache/sdkconfig.read_pico-factory`，使用固定 `boards/partitions.read_pico_factory.csv`，不重新生成共享 OTA 表。factory 选项只能与该板和单独 firmware 步骤使用，同样禁止额外命令/fullclean。用户放弃完整备份并授权仅 app 烧写，实测证据及保护范围见 [烧写维护](DEPLOYMENT.md)。

板型信息为 ESP32-S3、16 MB Flash、8 MB Octal PSRAM、1216×684 GC16。P1 配置仅使用 80 MHz Flash/PSRAM 进行编译诊断，参照厂家 sdkconfig.ci 的边界；不引入 HPM，也不冒充厂家 120 MHz 显示产品配置。编译成功不能证明实际容量或时序正确。

诊断配置显式禁用 Flash coredump，避免继承原相框的启动检查/崩溃写入路径；保留生成分区表的 coredump 区域不代表会写入。NVS、SD 和业务初始化均未启用；这不是对任意 bootloader、已有设备配置或直接烧写的无副作用承诺。

`build.py` 对该板拒绝 fullclean 和额外参数（包括 flash、monitor、erase）；这是防误操作限制，不阻止用户直接运行其他工具。IDF 编译后自动打印的烧写示例不能照用。源码编译约束检查 SDK 6.1.0，其他 SDK 先重新验证。

## 当前执行路径

`read_pico_bringup → board_hal_init → I2C 只读观测 → 脱敏日志 → 释放设备/总线 → 返回`

- Read Pico 分支编译诊断 main、检查式 display_manager 与 utils；只有诊断 app_main 被执行，未引用业务路径由链接 GC 移除。ELF 包含 read_pico_epaper_init/display、epd_init/poweron，但诊断只调用 false 初始化 guard；无 nvs_flash_init/erase、sdcard_init、esp_wifi_init、Flash coredump 写入入口。
- HAL 创建并拥有唯一 I2C0，SDA39/SCL40，400 kHz；PMU 0x2A 与 IOE 0x24 句柄由 HAL 创建/释放。启动 init 为单任务；运行时快照/IO 共用递归锁，新增显示租约阻止借用期间释放总线。显示锁→总线锁顺序已实现，主机互斥测试不证明实际 RTOS 调度正确。
- IOE 只读 CFG0/OUT0，不改方向、MODE、XOE、SY_EN、VCOM_EN，不读取/清空事件队列。SY 仅探测 0x62；未上电时 NACK 正常，不为探测拉高电源，不调用 SY power_on/设置寄存器，也不擅自关闭既有电源轨。
- 不调用厂家 read_pico_init 或 read_pico_pmu_init：前者包含显示/触摸/SD 等副作用，后者会 HOST_READY 并 drain/ACK 事件。当前只复用固定协议头的常量/布局依据，不修改第三方实现。
- PMU IDENTITY/STATUS 检查完整长度、CRC、PMU1、协议 1.1、device ID、frame size 与非零 session。未知协议拒绝，固件版本只记录实际值，不把宏 1.0.8 当设备版本。
- STATUS CRC 错误最多重新读取 3 次，重读前退避 2/5 ms：固定厂家 parser 已记录影子更新可撕裂。只处理 CRC，不重试 IO 错误/未知身份，不发布混合样本；持续错误失效并保持显示关闭，尚未发送 GET。实板遇过一次拒绝，不把新增重读的测试当作所有物理 CRC 原因都已解决。
- VCOM 只发 0x0510 GET。序号从有效 STATUS.last_sequence+1 恢复并跳过零；响应校验 magic/header/kind/protocol、sequence/opcode/status/session、长度和 valid/range/10 mV 对齐。GET 在线上需要写命令帧，会改变命令序号/响应缓存，但不写校准、电源策略或持久配置。
- GET 后再次读 IDENTITY，session 改变则不发布混合快照。响应最多读 5 次，不换序号重发；读失败有界返回，初始化失败按逆序清理。释放失败保留未释放句柄，可重试 deinit，不能冒充已释放。

## 状态与限制

帧校验/状态映射在便携 read_pico_pmu_codec 中，不依赖 ESP-IDF；I2C 与资源生命周期在 driver_read_pico。

电池值只有有效标志和能力位均存在时才发布；SOC 超过 1000‰ 或无效值保持未知，不估算或夹到 100%。charging 是 -1/0/1，电源来源是 unknown/present。NOT_CHARGING、FULL_INFERRED、UNKNOWN、FAULT 不用来断言 USB 是否存在；状态与标志矛盾保持未知。

快照 5 秒过期，刷新失败立即失效；读取失效快照返回错误及未知哨兵值。VCOM 未标定、响应拒绝/不匹配均保留 vcom_valid=false、mV=-1，不用默认值替代；诊断可打印其余状态，显示始终关闭。

通用 HAL 的 bool USB/充电接口不能完整表达 unknown。当前完整相框 main 被隔离，诊断使用 richer snapshot；P4 接完整电源策略前必须扩展可用性/外部电源契约，不能使用 false 判定“正在电池供电”。

不发 HOST_READY，可能留下 BOOT_WAIT 超时故障位。厂商协议写明超时只置位、不断电；实板行为仍需确认。不要把短诊断入口当作正常待机固件。

## 后续与上板条件

SD 唯一所有者确定为 PhotoFrame 存储 backend，保留 `/storage`；不调用厂家 `/sdcard` 挂载。P1 当前不挂载任何卡，1-bit 参数化及介质测试在 P3 完成。

外层 EPDiy/GC16、SY/IOE 电源和所有权通过本地测试/编译；用户另行允许受控显示测试和首次清白，已实机执行并人工确认全白。软件 PGOOD/故障与退出检查、显示/HAL deinit 成功，但 GC16/温度/异常恢复、连续稳定性和 120 MHz/HPM 仍未验收。普通入口仍不启屏，受控显示授权不扩展为完整迁移或校准写。

小纸 Pico ROM 识别为 S3 v0.2、16 MB Flash/8 MB PSRAM，factory 专用诊断通过本机启动；PMU 1.0.9/hw1、VCOM 有效 1580 mV、IOE CFG0=0x64/OUT0=0x81，SY 断电 NACK 符合诊断边界。PMU fault bit0 是协议定义的 BOOT_WAIT 超时粘滞位，本轮未清除；无法据此推定屏电源故障或所有 PMU 功能正常。电池/USB 来源只是瞬时报告，unknown 不等于无 USB。

PCB 修订/改装、完整相框分区迁移与恢复能力仍需核验。用户放弃完整备份，历史两个废纸篓备份路径已失效。默认 OTA metadata 与旧 Demo PHY/app 范围重叠，只有已核验的专用 factory app-only 流程允许写 `0x10000`，不能把它泛化为完整 PhotoFrame 烧写方式。

结果及产物摘要见 [VALIDATION](../../../VALIDATION.md)。主机模拟不是 PMU/显示/功耗的真机通过证据。
