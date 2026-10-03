# Read Pico 官方硬件与接口资料

核对日期：2026-10-03（北京时间）。官方产品参数与固定源码用于移植设计，不等于手头设备已经核验。型号/修订、PMU 实际版本、校准值和运行状态留待只读采集。

## 产品规格

| 项目 | 官方资料 |
| --- | --- |
| 产品型号 | Read Pico development board (CNC)，RDP-G01-W |
| 主控/内存 | ESP32-S3 双核 LX7；512 KB SRAM、8 MB PSRAM、16 MB Flash |
| 显示/输入 | 4.7 英寸、原生 16 灰阶；双点触摸、3 个触摸键 |
| 电池/供电 | 可更换 2050 mAh LCO 电池包；额定输入 5V⎓1.8A；USB-C |
| 存储 | microSD，产品页标称最高 256 GB；新固件的文件系统/卡兼容性仍需验证 |
| 无线 | 802.11b/g/n，BLE 5.0，产品页标注 2 Mbps PHY |
| 关键芯片 | CW32L010 PMU、SY7636ARMC 显示电源、SC7A20HTR 加速度计 |
| 外形 | 117.5×66.2×7.3 mm，91 g±2 g |

来源：[Read Pico 官方产品页](https://dot.mindreset.tech/docs/read_0)。256 GB 是产品规格，不直接承诺移植版支持某种 exFAT/FAT32 配置；额定输入不等于运行电流或充电电流。

## 板级接线（固定 BSP）

| 功能 | GPIO/连接 |
| --- | --- |
| I2C SCL / SDA | 40 / 39，默认 400 kHz |
| EPD D0–D14 / D15 | GPIO4–18 / GPIO45 |
| EPD XLE / XSTL / XCL / SPV / CKV | 3 / 46 / 21 / 47 / 48 |
| FCA9555 INT# | GPIO41；BSP 明确仅作为浅睡唤醒来源 |
| CST836U INT# | GPIO43；不能把该引脚当作空闲 UART TX 使用 |
| SC7A20H INT1 / 蜂鸣器 | GPIO1 / GPIO2 |
| SDMMC CLK / CMD / D0 | 38 / 42 / 44，1-bit；D1–D3 不接 |

GPIO 定义见 [read_pico_board.c](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/read_pico_board.c)、[board.h](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/include/read_pico_board.h)、[Kconfig](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/Kconfig)、[SD 实现](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/read_pico_sd.c)；加速度计/蜂鸣器见 [官方 pinout](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/README.md#pinout)。

FCA9555 Port-0：P0.0 MODE、P0.1 XOE、P0.2 CW_INT、P0.3 SY_EN、P0.4 SY_VCOM_EN、P0.5 SY_PGOOD、P0.6 SD_CD、P0.7 TP_RST。P0.2/5/6 为输入；卡检测低表示在位。见 [板定义](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/read_pico_board.c)。

I2C 参考地址：CST836U 0x15、SC7A20H 0x19、CW32 0x2A、SY7636A 0x62；FCA9555 默认 0x24。地址来自 [BSP 设备识别](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/read_pico_board.c) 与 [FCA9555 头文件](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/fca9555/include/fca9555.h)。SY7636A 在 EN 低时不应答，触摸芯片睡眠时也可能不应答；不能要求所有电源状态下扫描全在线。

## 显示、频率与 VCOM

目标为 1216×684、16-bit LCD 并口驱动的裸屏，不能复用 IT8951 的控制器通信实现。厂家 EPD 扫描 helper 给出的 PCLK 范围为 12–24 MHz；其上限说明是 DMA 供给限制，并非面板规格。改变 PCLK 必须同步重新计算行消隐与 CKV 等参数，不能仅调用切频函数。[时序源码](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/include/read_pico_epd_timing.h)

参考 `sdkconfig.defaults` 为 Flash/Octal PSRAM 120 MHz，并含 Zbit HPM 与温度补偿限制说明；`sdkconfig.ci` 的 PSRAM 80 MHz 用于编译检查。80 MHz 不自动成为已验证的产品方案，120 MHz 也不能不核对实物便照用。[默认配置](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/sdkconfig.defaults)、[CI 配置](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/sdkconfig.ci)

ESP-IDF 文档说明 Flash/PSRAM 共用内部时钟，120 MHz DDR 为实验配置，温度变化可能影响可靠性。本项目频率选择需连同吞吐、DMA 和温度测试一起确定。[ESP-IDF v6.1 配置说明](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-guides/flash_psram_config.html)

面板 VCOM 存于 PMU，厂家提供 `read_pico_pmu_vcom_get()` 获取；P1 因其初始化有 HOST_READY/drain 副作用，改以独立只读适配发送同一 VCOM_GET，见 [P1 说明](READ-PICO-BRINGUP.md)；不能把底层初始化常量作为校准缺失时的替代值。首次刷新前验证读取成功、标定有效并已应用。官方 DIY 文档确认 VCOM 不应由用户修改。[官方开发说明](https://dot.mindreset.tech/docs/read_0/firmware)、[PMU API](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico_pmu/README.md)

## PMU 协议与睡眠

已找到完整的 [中文协议正文](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico_pmu/docs/llms-full_zh_cn.md)、[协议头](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico_pmu/include/read_pico_pmu_protocol.h) 和 [v1.0.8 PDF 入口](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico_pmu/docs/read_pico_pmu_datasheet_v1.0.8_zh_cn.pdf)。本轮阅读的是文本/头文件，未对 PDF 另做排版或逐页核验。

- 头文件声明协议 1.1、固件 1.0.8、HW revision 1；这些是参考定义，不是本机读取结果。
- 命令使用序号、会话和 CRC；状态读取与命令不能简化为任意寄存器写。新鲜快照与 `*_ok` 标志决定读数是否有效。
- 启动需读取身份并完成 HOST_READY 交接；RUNNING 才能进入 soft sleep。SOFT_SLEEP 后 PMU 关闭 ESP EN，后续运行从重新启动开始。
- 闹钟支持关闭/单次/循环，相对秒数范围 10–2,678,400。闹钟状态在 RAM，PMU 复位后需要重建；UTC 校时和漂移需要测试。
- 电量为千分比，未知值要与 0 区分。充电状态包含 UNKNOWN、NOT_CHARGING、CHARGING、FULL_INFERRED；不能以“未充电”直接断言没有外部供电。
- 本机是否能可靠识别充电器/充满/PC USB，需要基于上述状态与实测定义映射；报告中的 `board_hal_is_usb_connected()` 仍属设计草案。

以上依据为 [PMU 正文](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico_pmu/docs/llms-full_zh_cn.md) 与 [主机组件说明](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico_pmu/README.md)。相框必须先结束存储/显示/网络操作，再交给 PMU；状态确认不能仅看命令返回码。

CST836U 自身的 deep sleep、ESP light sleep 和 PMU 切 EN 是三种不同状态。触摸控制器深睡需经 FCA9555 复位唤醒，不能由此推导“触摸可唤醒整机断电”。[板级接口](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/read_pico/include/read_pico_board.h)、[官方睡眠实现](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/main/sleep.c)

## 官方诊断与恢复入口

[Official Demo](https://dot.mindreset.tech/docs/read_0/start) 提供屏幕灰阶/刷新、触摸、I2C、PMU、电源、睡眠和存储诊断，可作为同板对照。页面里存在格式化等操作，本项目不自动执行。

[DIY Your System](https://dot.mindreset.tech/docs/read_0/firmware) 给出从官方源码构建/烧写 Demo 的入口。[CrossMux 说明](https://dot.mindreset.tech/docs/read_0/crossmux) 给出阅读固件安装路线，但不是本项目依赖，也不证明旧用户配置能恢复。当前查阅页面未得到可直接锁定并验证的原厂二进制摘要；稳定恢复文件仍须另行核验。

本轮浏览的官方页面和固定仓库树中没有找到整板原理图/BOM/PCB 修订对照文件。已有源码引脚与 PMU 协议可用于开工；如实板不匹配，再向厂商补取，不能据此断言厂商未公开。

## 本机待填字段

| 字段 | 当前状态 | 取得方式 |
| --- | --- | --- |
| 实物型号/PCB 修订/是否改装 | 未重新确认 | 外观标识或用户已有资料；无需为建档自行拆机 |
| Flash JEDEC ID、PSRAM 实际配置 | 旧记录有容量，当前待读 | 首次启动/只读设备信息 |
| PMU 固件、协议、硬件版本 | 未读 | 官方身份/扩展信息接口 |
| VCOM 有效标记/值 | 未读 | 官方只读接口；私有日志保存 |
| SD 型号、容量、文件系统 | 未核验 | 独立测试卡建档，不格式化现有卡 |
| 当前固件/分区与可用备份 | 仅有历史停止记录 | 首次上板前核验 |
| GC16 耗时、静态/刷新/睡眠电流 | 无新方案测量 | 固定测试图和相同供电条件测量 |
