# 安全启动排查与产物门禁

最新状态：RAM 查询已完成主机测试/编译，CTest 17/17、新版 166656 字节已授权烧写并回读相符；三轮人工电源操作确认后 64 KiB 三模式 3/3 PASS，双核 Power on reset、查询后 running。下一阶段准备备用卡/SD/显示测试，更换固件须单独确认。见 [RAM 结果查询](PROBE-STATUS.md)。后文保留此前烧写/诊断历史。

恢复最新：用户人工断电/重连后，双核 Power on reset、ibreakenable=0、空闲路径/任务状态检查成功，检查后双核 running，JTAG 工具退出。此前恢复未知已解除，专用结果仍未确认；后续宜保存可查询 RAM 结果，不继续依赖 USB 重枚举期间断点。下面异常段保留为历史，详见上板记录的恢复小节。

当前注意：授权断点/受控复位实验遇到 USB-JTAG IO/OVERFLOW，后续重连清理未成功；实时状态及断点清理尚未确认，需要人工电源/USB 重连。专用结果未取得，不据进程退出码判成功。下面空闲路径证据来自本次复位前，不能当当前设备状态，详见上板记录最新小节。

实时 JTAG 最新证据：三次双核 PC 在 esp_cpu_wait_for_intr，返回地址在 FreeRTOS idle hook；完整任务列表只含 IDLE0/IDLE1/ipc0/ipc1，没有 main_task。当前不是停在 PSRAM 调校，软件采集的最后日志不代表当前 PC；专用 PASS/FAIL/ALLOC_FAIL 仍缺失，不反推 PASS。短暂停核后恢复双核 running，不请求复位、烧写或改时序，详见上板记录的实时 USB-JTAG 小节。

最新对照：用户授权软件复位后，30 秒日志 2495 字节，SPI 启动/应用加载确认，最后可见 PSRAM 调校（74 ms），无 APP_ENTRY/专用结果。Saved PC 对应复位前等待中断，不能定位当前卡点；没有重烧或改时序，串口已释放。新版启动门禁仍未通过，详情见上板记录的授权软件复位对照。

最新上板状态：用户确认就绪后，结果重播新版已烧写/回读相符（app 166512 字节），NVS/phy 保留；尚未验证新版启动/专用结果。下面“未烧写”是开发阶段历史，不代表当前设备。见第四轮 [上板记录](HARDWARE-RESULTS.md)。

2026-10-01 最新：修正版采集已获得 PSRAM 系统内存测试 OK、探针 heartbeat=1..4、PROBE_END 和 app_main 返回，证明应用已执行完。USB 重枚举期间缺失启动头/APP_ENTRY/PSRAM_64K_PASS，专用校验及完整冷启动门禁仍待补证。串口已释放，设备仍是第三轮探针，本轮没有再次烧写。详见 [上板记录](HARDWARE-RESULTS.md)；此前停于 PSRAM 的结果保留为历史，不能替代此次新证据。

## 已知与未知

原备份 app descriptor 为 `Read_Pico`、版本 `8836142`、ESP-IDF `v6.1`；本项目同样为 v6.1。测试 ELF 包含强 DRAM 符号 `spi_flash_hpm_enable_list`，不能把问题直接归咎于缺失 HPM 覆盖。此前最后可见的时序日志与后续 ROM 下载模式不足以证明精确根因；串口控制线/复位、冷启动和应用启动须分开验证。

不修改上游 120 MHz/HPM、驱动或校准。此次有效日志为 SPI_FAST_FLASH_BOOT，20 秒内停在 PSRAM 调校；原厂回滚后的串口复位也未取得应用就绪证据。尚不能归因于某一个配置项，必须区分 USB 重枚举/复位与完整冷启动。

## 离线镜像核对

新增 `tools/compare_boot_images.py`，只读取明确指定的四个本地 boot/app 文件，不访问串口、不复位、不写文件。仅支持 ESP32-S3 v1 镜像、附加 SHA256、无签名且尾部为擦除态的分区提取；检查段长度、XOR 校验、SHA256、描述符和头部，拒绝隐藏尾部/截断。完整性不是来源真实性或签名验证，未知格式直接拒绝。

```sh
python3 tools/compare_boot_images.py \
  --factory-boot <备份提取的boot文件> --factory-app <备份提取的app文件> \
  --candidate-boot build-boot-probe/bootloader/bootloader.bin \
  --candidate-app build-boot-probe/gr4_paper_frame.bin
```

2026-10-01 核对本次回滚备份与已测试探针产物：

| 核对项 | 离线结果 |
| --- | --- |
| 四个镜像 XOR/附加 SHA256 | 全部有效；原厂 boot/app 提取的擦除尾部不参与镜像长度比较 |
| 原厂/测试 boot 有效长度 | 均 21168 字节；原厂提取容器为 32768 字节 |
| boot 段地址、长度与代码 | 三段布局一致；两个 IRAM 段字节一致；首段仅编译时间字段的 11 个字节不同 |
| 排除 boot 描述符时间字段 | 全部加载段字节一致；工具仅屏蔽 `esp_bootloader_desc_t.date_time` 的 24 字节，不屏蔽代码/其他配置 |
| boot/app ROM Flash 头 | 均 DIO、16 MB、80 MHz；芯片/revision/引脚驱动字段一致 |
| 原厂 app / 探针 app | IDF 标签均 v6.1；有效长度分别 3018032 / 166496 字节，内容和入口不同，属于预期差异 |
| 探针生成配置 | Flash 120 MHz/HPM-DC 关闭、Octal PSRAM 120 MHz、CPU 240 MHz、XTAL 40 MHz；温度补偿未启用 |

80 MHz 是镜像启动头字段，不是应用运行频率；不能据此认定应用没有启用 120 MHz。原厂缺少配套 ELF/sdkconfig，原厂应用编译时序和精确 IDF commit 仍未知。现有对比不支持“bootloader 代码差异导致故障”的判断，也不证明混用 boot/app 安全或自动授权 app-only 烧写。

锁定 IDF 本地源码中，`mspi_timing_psram_tuning()` 在日志之后继续配置低速访问、写参考数据和选择时序点；240 MHz 核心时钟分支的 `get_working_pll_freq()` 存在无超时的 BBPLL 校准等待，温度补偿关闭时也可能使用这一分支。这里只能将它列为代码排查候选，不能从最后一行日志推断停在该循环；不能据此修改 IDF/BSP。官方 [v6.1 Flash/PSRAM 配置说明](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-guides/flash_psram_config.html) 说明共享内部时钟和 120 MHz DDR 的实验性，也不足以证明本次失败由温度导致。

对照实验应控制变量：原厂人工完整关机再开机与 USB 软件复位分开记录；原厂已有人工功能恢复确认，但缺少冷启动日志。第三轮新授权后，已备份并烧录同一探针，等待人工电源对照；未补写缺失的原厂冷启动证据。串口打开本身也可能影响 USB 控制线，不把自动重连接当成无干扰冷启动。若仍缺 APP_ENTRY，停止 SD/照片测试。上述离线核对阶段没有设备访问；第三轮烧录记录另见 HARDWARE-RESULTS。

工具的 9 项合成测试覆盖所有截断点/逐字节篡改、越界/头部、隐藏尾部、描述符、只读对比及仅编译时间排除，不依赖私人备份；已加入 CTest，总计 13/13 通过。

## 只读启动探针

探针模式在 app_main 最前面进入，绕过 NVS、BSP、PMU、屏幕、SD 和相机操作。新版输出 `APP_ENTRY`，在临时 PSRAM 分配的 64 KiB 缓冲中核验三种模式且仅测试一次；释放缓冲后每 500 ms 重播 `PSRAM_64K_PASS/FAIL/ALLOC_FAIL` 和 `PROBE_SUMMARY app_entered=1 result=...`，共 20 次，约 10 秒后输出携带明确结果的 `PROBE_END` 并返回。校验失败不继续后续模式，`patterns_checked` 仅统计完整通过的模式；分配失败同样重播明确结果，不误报 PASS。应用日志等级不变。

主机直接编译 `main/boot_probe.c`，以 fake heap/日志/FreeRTOS 时钟覆盖成功、分配失败、三个模式分别损坏、释放次数、结果不变、模拟晚接收与有界退出；不是设备内存或 USB 时序测试。CTest 16/16 通过，采集器 11 项循环测试识别新摘要与分配失败标记。新探针 v6.1 编译/门禁通过：166512 字节，SHA256 `55cee33ab0d9b05bab1c31213186c1b136277e7eb4bbbb3bb769905768d980d7`；尚未烧写，设备仍运行旧 166496 字节版本（五条心跳、结束标记不带结果）。新版本替换及真机重播验证需人工确认。

重播不能恢复遗漏的 ROM 复位原因，也不证明无 USB 干扰的冷启动；PMU 若提前断电仍可能截断日志。看到摘要中的结果才可核对专用测试，不能把旧版 PROBE_END 单独当 PASS。

它仍经过同一 IDF 早期 Flash/PSRAM 初始化，因此只能帮助区分“未到 app_main”和“应用已进入”；不能修复早期时序问题，也不是完整内存压力/温度测试。不会提供按钮界面或改变旧电子纸画面，不向 PMU 报告 ready；若 PMU 有启动时限，可能自行断电，不能把短时探针当成产品固件。

独立构建（不烧写）：

```sh
. .cache/esp-idf-v6.1/export.sh
idf.py -B build-boot-probe -D SDKCONFIG="$PWD/.cache/sdkconfig-boot-probe" -D FRAME_BOOT_PROBE=ON build
python tools/verify_firmware.py --build-dir build-boot-probe --mode boot-probe
```

探针与存储诊断构建开关互斥。使用各自独立 sdkconfig，防止沿用上次测试模式。下一次上板应先验证探针，不直接测试 SD；再次替换正常运行的原固件前取得用户确认，并保留原始回滚备份。

## 产物检查

`tools/verify_firmware.py` 不访问串口、不写文件。IDF 默认 build 和 flash 目标均依赖 `frame_firmware_check`：

- app/bootloader magic 和容量边界、app descriptor 项目名与 IDF 版本。
- Flash/PSRAM 120 MHz、HPM/USB 控制台配置，拒绝未验证的温度补偿。
- CMake 请求模式与实际 sdkconfig 相符，防止误烧写模式。
- 分区记录、MD5、固定 NVS/phy/factory/storage 布局，无新增擦写目标。
- ELF 中 HPM 强符号与当前模式入口存在。

这不是固件安全签名验证，也不验证真实启动、供电或断电可靠性。用户直接运行 esptool 或其他烧写程序不会经过这个 CMake 门禁。对原始回滚镜像也不运行本项目身份门禁，因为项目名不同。

主机 12 个门禁测试覆盖正常产物、错模式、弱 HPM/入口缺失、分区校验破坏/重算 MD5 后布局改变、额外 NVS 烧写目标、重复写入偏移、erase-all 选项、温度补偿、错误 IDF 和坏 bootloader。旧探针上板已见应用心跳/结束，但专用结果缺失；新版编译通过尚未上板，后续测试须先补齐启动门禁。

`tools/capture_pico_boot.py` 使用本机 IDF 中 esptool 5.4 的 CLI 等价连接、清除其标准 S3 易失 force-download latch 后 watchdog 复位，并在有界窗口内处理 USB 重枚举，保存不可覆盖的私有串口证据。该工具不烧写 Flash/SD，但会请求复位；只有用户授权的设备测试中使用，不是常规状态读取，也不替代物理冷启动。本次有效采集后已释放串口。

## 等待人工开机的采集

`tools/wait_pico_boot.py` 与主动复位工具独立。原版普通 pyserial 打开会更新 DTR/RTS 并自动清输入，不能作为无干扰冷启动证据。现已改为经过测试的 POSIX pyserial 3.5 专用子类：覆盖 `_update_dtr_state/_update_rts_state/_reset_input_buffer`，禁止 write，配置后清除 HUPCL（TCSANOW，不执行 flush）。保留 raw/115200/独占锁/有界读取，最多 45 秒并处理重新枚举；新版本不发 modem-control ioctl、不清输入、不请求复位、不访问 Flash/SD/PMU。未知库版本/后端/非 Darwin 或 Linux 环境直接拒绝，不回退普通串口。

这些保证限定到已核对的 Python 调用链。OS open/close、USB 枚举与底层驱动的控制线行为仍不受脚本控制；HUPCL 在串口配置后清除，不承诺设备打开/配置失败时零干扰。Linux 的运行时电气行为未验证，本轮在 macOS 测试。不能仅凭工具测试通过宣布真正冷启动。

```sh
# 使用已安装 esptool/pyserial 的 IDF Python，输出路径必须不存在。
python tools/wait_pico_boot.py --port <已确认的Pico串口> \
  --seconds 45 --output <私有缓存中的新日志文件>
```

先看到 `Transport open ... capture ready`（不是仅 Capture waiting），再由人操作完整关机和开机。按锁定 BSP 的 PMU 协议，EN=1 按住电源键至少 10 秒强制断电，松开后 EN=0 按住至少 1 秒开机；这是人工电源操作，不是仅拔 USB，也不要求拆电池。板上 PMU 版本/实际供电状态须记录，若实际行为不符停止并核对，不发送 PMU 出厂/校准命令。

报告只列字节数、连接次数、传输错误和观察到的探针标记；没有数据不等于启动失败，标记跨 USB 断开不拼接。原始日志仅保存私有路径；工具不自行宣布冷启动或门禁通过。输出满盘/短写立即停止，不错误地当串口断开重试；始终释放端口，日志文件不可覆盖。

10 项循环模拟测试验证控制线初值、等待枚举、重连接、分片标记、跨断开误匹配防护、有界退出、输出故障、清理以及打开成功后才通知就绪。另有 6 项真实 pyserial POSIX 后端测试，所有 OS 调用 mock：普通库基线副作用检测、修正版 open/超时重配置无 modem ioctl/flush 且 HUPCL 清除、三次重连、失败打开清理、拒绝 write、未知版本/后端/平台拒绝。没有 pyserial 会失败而非静默 skip。CTest 15/15 通过，IDF Python 的依赖测试 6/6 通过；没有真机采集或复位。

## 真实依赖与 ELF 核对（2026-10-01）

本机 IDF Python 的 pyserial 3.5，真实 `Serial.open()` 在所有 OS 调用被 mock（无设备访问）时，顺序为 `OS open → DTR update(False) → RTS update(False) → INPUT FLUSH`。仅提前设置 dtr/rts=False 不阻止打开时控制线更新；库自动调用 `_reset_input_buffer()`，与 [pyserial v3.5 POSIX 源码](https://raw.githubusercontent.com/pyserial/pyserial/v3.5/serial/serialposix.py) 一致。不能声称这证明了本次 USB 复位由哪次更新引起，或证明 flush 丢失了哪些实际字节。

本机 esptool 5.4 使用的 `esp_pylib.serial_reset.usb_jtag_bootloader_reset()` 明确以 DTR/RTS 转换触发 native USB-Serial/JTAG 下载复位。外置 UART 的 GPIO0/EN 电路规则不能直接套用于此板。OS/USB 控制状态和供电状态仍需要独立证据。

离线 ELF/反汇编核对：

- 日志 `Saved PC=0x403781ed` 对应 `spi_flash_hal_configure_host_io_mode` 内联的 `spimem_flash_ll_set_dummy()`，具体为 dummy cycle 计算，不是 PLL 等待；它是 USB 复位前的上下文，不是本次卡点的实时 PC。
- `0x4037965c..0x40379661` 对应实际已链接的 BBPLL 状态读取/回跳，来自 `get_working_pll_freq()` 恢复 PLL 后的等待，内联进 `s_select_best_tuning_config_dtr()`。无超时等待确实在 ELF 中，但无实时 PC，不能断定停在这里。
- `mspi_timing_psram_tuning=0x40379f04` 在 IRAM，最后日志后仍有参考数据写入、遍历/选择时序点和高速恢复；日志只定位到阶段，不定位具体指令。

本轮未修改采集实现、连接串口、复位、烧录或改 IDF/BSP/时序。下一步先修正并验证主机采集链路，再做受控电源实验；如仍有阻塞，再单独规划实时 PC/阶段证据。不能直接改成另一频率或混用原厂 boot/app 来猜修复。
