# 开发与构建

## 主机验证

2026-10-03 USB接收修正：49/49 CTest（46.67秒，C目标ASan/UBSan）、门禁37项通过。变更该适配必须配置 `-DFRAME_AUDITED_IDF_DIR=<锁定tracked-clean SDK绝对路径>`，否则普通便携构建只有48项、不覆盖实际SDK VFS。新增测试直接编译原SDK open/read/availability和实际RX适配，替换FIFO/连接/锁，不是实机USB/RTOS证明；源码摘要或commit不符即失败。专用433232字节d14d1093…编译/门禁通过；默认dd5ebff0…未变、无连接5c63fedc…改变且未烧，隔离ELF通过且无.bin/门禁拒绝。下方旧数量/“未改接收”状态为历史，最新烧写/人工测试以TODO和 [身份诊断](GR4-IDENTITY-DIAGNOSTIC.md) 为准。

2026-10-03当前：48/48 CTest（C目标ASan/UBSan）、37项门禁、真实SDK只读身份专用/默认/无连接编译通过。新增USB生产者10种/设备入口22种合成启动、便携严格命令与TTY人工工具8项（含实际pyserial OS mock）；不替代真实USB/相机/SDK并发。软件操作见 [只读身份诊断](GR4-IDENTITY-DIAGNOSTIC.md)。新专用d4f2ccaf…未烧；默认359440字节dd5ebff0…/无连接417296字节e167e55f…为新重编、均未烧，旧基线/旧真机证据不能自动移用。下文44组及缺入口描述为历史。

2026-10-03 当前：44/44 ASan/UBSan、门禁33项、实际SDK隔离ELF和正式默认/无连接重编通过。确认固定队列接入host定时消费，包含pthread合成队列与关闭竞态测试；不是ESP-IDF真实双核/实际人工输入或相机证据。仍缺输入驱动/新动作/退出与专用设备入口，compile-only禁止烧写，见 [确认消息通道](GR4-CONFIRMATION.md)。下文43组及更早为历史。

2026-10-02 当前进度：43/43 ASan/UBSan、固件门禁33项、真实固定SDK隔离ELF及默认/无连接固件编译通过。同SDK扫描、确认API消费、一次只读Owner激活与85秒独立期限已接入；现场确认输入和连接设备入口仍未实现，compile-only禁止烧写。无连接诊断此前仅一次真机生命周期PASS；本轮未烧写或连接相机，见 [同SDK扫描会话](GR4-SCAN-SESSION.md)。下文“最新”及测试数量均为历史阶段记录。

2026-10-02 最新timer：36/36 ASan/UBSan、门禁27项、实际SDK组件/默认固件编译通过；应用timer新增host exit后的有界静默与检查式退休，失败封存保留。SDK内部timer/队列/transport退出和独立watchdog/设备入口仍未完成，仍compile-only禁止烧写；见 [应用定时器退出](GR4-HOST-TIMER.md)。下文34组等为历史。

2026-10-02 最新：主机全量34/34 ASan/UBSan、固件门禁27项通过；新增实际NimBLE操作层合成mock测试，不访问设备或相机。真实SDK组件编译和默认固件编译通过，但GATT无设备连接入口、compile-only禁止烧写，见 [GATT操作层](GR4-GATT-IO.md)。下文测试数量为此前阶段。
2026-10-02 最新进展：全量 CTest 25组（新增设备 JPEG 存储和实际诊断入口测试），固件门禁 Python 22项；默认／JPEG 专用 ESP-IDF v6.1 编译通过。已有 RAM 缓存 SD 显示三图实机／视觉验收通过，设备 JPEG 新版另行验收，见 [JPEG 诊断](JPEG-CACHE-SAMPLES.md)。下文数量及待启动状态保留历史。

最新全量 CTest 20/20：包含实际 g4 像素回调映射、严格已有库恢复、目录检查、产物模式门禁、120 秒采集边界，以及 RAM 状态解码／实际 Tcl fake target 测试。需要 Tcl/tclsh（本机已有，不安装或跳过）；C 目标 ASan/UBSan。RAM 64 KiB 和严格已有 SD fixture 恢复分别完成三轮人工电源操作后 PASS；三图显示诊断已编译／授权烧写／回读，等待启动／视觉验收。见 [显示测试](DISPLAY-SAMPLES.md) 与 [上板记录](HARDWARE-RESULTS.md)。下述八组／16 项保留此前覆盖说明，不代表最新数量。

完整测试还需要 POSIX Python 与 pyserial 3.5，本机 IDF Python 已有。CMake 缓存的 Python 不一定与 shell 中 python3 相同：配置时可添加 `-DPython3_EXECUTABLE=<含pyserial的Python绝对路径>`。本轮首次 CTest 因旧缓存解释器缺 serial 明确失败，改为已安装依赖的 IDF Python 后重新验证；没有安装软件或静默跳过。真实串口适配测试不访问设备。

```sh
git submodule update --init
cmake -S . -B build-host -G Ninja -DFRAME_HOST=ON -DFRAME_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

CTest 的八组测试覆盖：状态/删除意图/所有日志尾部截断点/CRC 修改/ACK 缺失条件/路径注入；多照片交错恢复、不确定提交、序列冲突和 5000 条容量停入；库绑定、双快照回退、全局序号、5000 条快照与主机文件往返；POSIX 文件适配重开/同步/检查点/回收/锁及不安全路径保护；介质接口逐个 IO 故障与 FatFS 映射主机测试；EXIF 1–8 的镜像旋转、Fit、奇数 stride 与缩放；自有合成 JPEG 解码、截断拒绝、内存失败和 EXIF 越界；命令行端到端转换、输出尺寸与拒绝覆盖已有文件。

ASan/UBSan 是主机内存检查，不能替代 SD 断电、SPI/PSRAM 时序、实机功耗或屏幕测试。默认主机构建 Debug，测试使用 CHECK，不受 Release 的 NDEBUG 影响。

当前 CTest 共 16 项，包括产物门禁、导入事务/CLI、流式恢复、镜像对比、采集循环、真实 pyserial 后端 OS mock 和实际探针源码的结果重播测试。探针以 fake heap/时钟覆盖成功、分配失败、三种模式损坏与晚接收，只证明应用逻辑，不证明真机启动。新增存储测试覆盖 IO 故障、部分文件保留、重开、防重入、PFU1 校验、分配失败保留 JPEG 和纯本地恢复；边界见 [照片导入事务](PHOTO-IMPORT.md)。可选私人样片工具另测用户三张原图（不加入公共 CTest），见 [黑白样片测试](IMAGE-TESTS.md)。串口库测试验证调用行为，不验证 USB 电气状态，见 [启动排查](BOOT-PROBE.md)。

## 固件构建

仅独立 `FRAME_GR4_IDENTITY_DIAGNOSTIC=ON` 可使用 `--mode gr4-identity` 专用门禁。它要求新现场USB确认才允许一次公开身份读取，禁止写入/配对/SD/显示；普通双compile-only开关仍不生成获准.bin。使用真实固定SDK与独立sdkconfig，命令/配置契约见 [只读身份诊断](GR4-IDENTITY-DIAGNOSTIC.md)，不能用旧隔离产物绕过门禁。默认Demo 0门禁名称是 `jpeg`，不是 `default`。

同SDK扫描会话使用 `FRAME_GR4_SCAN_SESSION_COMPILE_ONLY=ON` 与 `FRAME_GR4_GATT_IO_COMPILE_ONLY=ON` 双开关，仅隔离ELF编译、不生成获准烧写的应用；现场确认输入和专用入口尚未接入。命令、严格配置及验证边界见 [同SDK扫描会话](GR4-SCAN-SESSION.md)。

GATT操作层只允许 `FRAME_GR4_GATT_IO_COMPILE_ONLY=ON` 隔离组件构建，不生成批准的应用入口，门禁显式拒绝该配置烧写；不要用默认/扫描模式门禁伪装，也不要直接esptool绕过。具体configure/ninja命令及Owner契约见 [GATT操作层](GR4-GATT-IO.md)。

独立无线诊断已提供 `FRAME_GR4_SCAN`：只做有界被动Observer，默认仍不启用蓝牙；主机32组/默认及专用编译通过，已烧写但真机运行待验。该SDK的BT链接片段要求真实SDK路径无空格，符号链接不足；使用无空格SDK副本/安装路径，详见 [扫描构建与契约](GR4-PASSIVE-SCAN.md)。下面“源码未初始化BLE”指早期默认Demo及历史阶段，不适用于显式扫描模式。

使用 ESP-IDF v6.1。激活环境后确认 `idf.py --version`，再执行 `idf.py build`。工程读取上游 `sdkconfig.defaults` 的板卡时序，加 `sdkconfig.frame` 的本项目覆盖；配置用于对应 Read Pico 板，不能移植为任意 ESP32-S3 通用配置。

构建生成 `build/gr4_paper_frame.bin`、bootloader、分区表与 flash_args；源码未初始化 Wi-Fi/BLE，因此不能称为相机直连固件。

本机准备的隔离 IDF checkout 在被忽略的 `.cache/esp-idf-v6.1`，基线 tag v6.1。若使用它：

```sh
. .cache/esp-idf-v6.1/export.sh
idf.py --version
idf.py build
```

该缓存不是仓库发布依赖；其他电脑使用自行安装的相同 IDF。Read Pico 子模块通过 Git gitlink 固定。修改子模块 HEAD 后用 `git add third_party/read_pico_firmware` 更新父仓库记录，不提交上游构建生成物。

## 设备测试顺序

1. 选择可识别、≤4 Mi 像素的自有 baseline JPEG，复制到 SD `/frame/test.jpg`。
2. 先完成 BSP 自检/PMU 校准核对；本项目只读 VCOM，不提供出厂校准写入。
3. 核对 flash_args 的地址后，按维护文档烧写并查看串口启动日志。
4. 记录刷新结果、方向、残影、温度、内部 RAM 与 PSRAM 空闲量。
5. 填入 TODO.md，分别标记已编译、已烧写、已目视验证。

P0 尚未测试 sRGB/Adobe RGB 转换，当前按解码出的 RGB 使用固定亮度权重；推荐 sRGB 输入。文件严格要求尾部 EOI，带额外尾随数据的 JPEG 可能被拒绝，此限制后续由样本兼容测试调整。
