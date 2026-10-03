# GR IV 无线栈生命周期与无连接真机诊断

日期：2026-10-02。连接 Owner 仍是 compile-only；本轮仅放行独立的 **无扫描、无相机目标、无连接/GATT** 无线栈诊断。它验证同一 Central SDK 的启动/停止基础，不是相机身份、配对、同步或功耗验收。

当前扩展：同SDK host扫描/一次延迟Owner激活已补，独立85秒全流程期限及关闭栅栏、reset后拒绝旧身份、host退出后退休额外扫描timer；43组主机/实际SDK隔离ELF通过。现场确认输入/连接设备入口仍未启用，见 [同SDK会话](GR4-SCAN-SESSION.md)。重编无连接417296字节dac733a0…未烧、设备仍此前已验证792c5409…；下段41组/09101d54…及尚无延迟激活描述为历史。

后续开发：受控候选选择与无IO Owner交接已测试/编译，但此lifetime尚无同SDK扫描后的延迟激活入口。全量41组/32项门禁通过，重编无连接app417296字节SHA256 `09101d5469577f15074cb3dab9360eda7908a3e85fffb0477cc5e7f37f0d659e`未烧，不能套用旧792c5409…真机PASS。设备及旧私有回读证据仍为792c5409…，本轮无设备操作。详见 [候选选择与待集成契约](GR4-TARGET-SELECTION.md)。
## 独立生命周期管理

`components/camera_gr4/io/gr4_radio_lifetime.c` 放在 ESP-IDF 适配层，不进入便携核心。静态零初始化、全启动期存活、单次使用；不用超时强杀 SDK 任务、不自动重置或复用对象。

主任务独立等待：初始化 10 秒、同步 5 秒、从 host 创建前起算会话 watchdog 45 秒、停止 10 秒、清理 20 秒。watchdog 使用 FreeRTOS tick/回绕安全差值，不依赖 host timer 或 GATT 回调；非事件唤醒不重新计时。SDK 内部无限等待放入专用工作任务，主任务超时即报告失败，工作任务及其引用继续保留。它不是硬件看门狗，不能保证 CPU/中断失效或底层 assert/panic 后仍有软件响应。

停止工作任务先向 host 默认队列投递取消事件，host 串行禁收 Owner/GATT 并确认取消已处理，才进入 `nimble_port_stop()`。同时取得 stop 成功与 host run 返回两项证据，且 timer guard 无故障，才允许清理工作任务继续。SDK stop callback 不传播所有内部 status；不以单个返回值替代这些证据。

清理顺序：

1. `esp_bt_controller_disable()` 成功且 controller 为 INITED，先停止 transport 生产。
2. 有 Owner 时，退休其独立应用 timer；无目标诊断没有该 timer。
3. 封存并静默所有捕获的 SDK callout，队列仍有效时移除排队事件。
4. 移除/核对/释放应用取消事件。
5. `esp_nimble_deinit()` 成功，核对 SDK timer 已接受删除且 callout 指针清零。
6. `esp_bt_controller_deinit()` 成功且 controller 为 IDLE。

不调用先销毁默认队列、再关闭 controller 的 `nimble_port_deinit()`。失败停在当前步骤，不继续破坏仍在使用的资源；部分已释放资源不可回滚。只有整个成功路径之后才对外复制私有身份，仍是 IDENTITY_UNVERIFIED。事件组、lifetime、Owner/tickets 即使成功也保留至本次启动结束，避免工作任务尾部引用竞态。

## SDK 内部 timer guard

`io/gr4_sdk_timer_guard.c` 使用固定 SDK **公开的** `nimble/npl_freertos.h` / `ble_npl_callout_freertos` 类型，不猜 opaque 内存偏移，不用 timer 名称找对象。这与此前应用 timer 不依赖内部 callout 类型是两个不同层次：本适配明确依赖公开 NPL ABI，必须锁定 SDK 和 NimBLE submodule，不能随意升级。

GNU `--wrap=npl_freertos_funcs_init` 在 SDK 初始化函数表后、创建 callout 前安装运行时 init/reset/deinit/is_active guard；`--wrap=esp_timer_delete` 检查实际删除返回值。SDK/BSP 源码未修改。最多 16 个不可重置的生命周期 ticket，静默前至少捕获 host/stop 两个 callout；未知对象、重新初始化活对象、封存后 reset、容量溢出或 SDK 错误均置粘性失败。

SDK host 在停止期间可能自行 deinit host timer。guard 在原 void deinit **之前** 用正常任务 `esp_timer_stop_blocking(≤1000ms)` 等待回调退出，再移除排队事件，防止 SDK 先释放 event/callout。清零后的 active 查询返回 false，避免 SDK 后续调度空指针。退出时再次封存/静默所有剩余 ticket，并在 SDK 销毁默认队列之前预先排空；之后 deinit 不能再访问已经销毁的队列。

stop 屏障失败不会调用原 deinit。原 SDK void deinit 若隐藏 delete 错误，guard 会记录失败，但不能撤销 SDK 已经执行的内存释放：此时 timer 已经静默/不再重启，仍不得宣称 SDK 全部清理成功。资源保留不等于射频已停。原 SDK callout stop/其他并发行为未被完整改写，测试也不构成任意线程并发安全证明。

## 无连接板上入口

`main/gr4_radio_selftest.c`：NVS 初始化失败直接停止、不 erase/retry；仅现有 I2C/PMU HOST_READY 握手。PMU 初始化失败也执行检查式清理，I2C 删除失败保留资源。不初始化 BSP/SD/电子纸，不访问 VCOM，不创建照片、不改相机或卡上数据。

`frame_gr4_radio_start_idle()` → 同步 → 等待 3 秒 → 检查 host 状态 → 分阶段关闭。专用编译条件不引用 Owner/地址推断/GAP/GATT操作；普通连接入口在此配置中拒绝调用，并由链接回收。SDK Central/GATT client 代码仍是被测试的底层实现，不能把“无相机操作”说成 Bluetooth 代码不存在。

关闭 Observer/Peripheral/Broadcaster/SMP/GATT cache/持久化绑定/扩展广播，单连接容量；另显式关闭 SDK 默认的自动连接重试、隐私地址和 DTM 模式。初版 ELF 因默认重试保留 SDK `ble_gap_connect`，未烧写；最终独立配置不含该启动符号。PHY 校准属于 SDK 的射频初始化，关闭其 NVS 持久化，不等同于测量证明没有任何射频活动。

日志只输出阶段和分类，不输出目标/地址/序列号；`GR4_RADIO_END` 后每 500ms 重播同一 `GR4_RADIO_RESULT`，共20次，不是20轮测试。

| code | 含义 |
| --- | --- |
| 0 | 请求的生命周期和板卡清理成功 |
| 1 / 2 / 3 | NVS / I2C / PMU握手失败 |
| 4 / 5 / 6 | 无线栈启动 / host状态等待 / 停止或清理失败 |
| 7 | PMU/I2C清理失败（保留此前首错） |

`sdk` / `board` / `cleanup` 独立报告，错误 code 保留首错；初始化工作任务一旦提交，不能仅凭启动失败就声称 SDK 清理成功。部分初始化/失去响应时 `sdk=0 cleanup=0`，即使 SDK 自己声称已回滚，也不自动信任。成功日志应 `code=0 cleanup=1 sdk=1 board=1 start=0 wait=0 close=0 scan=0 connect=0 pair=0 gatt=0`。这仍不是独立射频/电流/掉电证据。

## 构建门禁与验证

```sh
idf.py -B build-gr4-radio-final -D SDKCONFIG="$PWD/build-gr4-radio-final/sdkconfig" -D FRAME_GR4_RADIO_SELFTEST=ON build
python3 tools/verify_firmware.py --build-dir build-gr4-radio-final --mode gr4-radio-selftest
```

必须是专用空构建目录，修改 defaults 后不能沿用已持久化旧选项的 sdkconfig。门禁核对独立 CMake 开关、全部配置、必需/禁用符号、分区和 BSP profile。连接 Owner/任意 NimBLE应用操作、扫描/连接启动、相机写入、SD/显示/erase、旧整体 port_deinit 均拒绝；普通 GATT compile-only 标记仍拒绝烧写。SDK commit `fff9895c82d744c7237be8847347bdd1b07c6643`、NimBLE `139cada0ae932957fa06ba37d17e3c9c2c95c773` 与 tracked-clean 状态必须一致；反汇编真实 ELF，核对 esp_nimble_init 与 NPL deinit 的实际 wrapper 调用，不只核对包装函数是否存在。门禁不是二进制安全证明或独立构建溯源认证。

主机40/40 ASan/UBSan（38.58秒）；SDK guard24场景、普通生命周期33场景、专用idle28场景（不定义任何Owner/GAP调用mock）、真实入口13场景、门禁31项、采集标记18项。测试覆盖独立任务期限/队列阻塞/晚投递/隐含删除失败/状态及顺序/部分释放保留/20次一致重播；mock/setjmp/fork是确定性合成执行，不是真实FreeRTOS线程、ESP timer或射频验证。最初入口测试的 GOOD 场景计数期望错误已修正，随后全量回归通过；不把失败运行计入通过。

真实固定SDK默认/专用编译和门禁通过：专用417296字节 SHA256 `792c54093a6a115dda12db21ce7b4e144f89315639ab7710bfee5a94ec1519db`；默认359568字节 `b5fe671e71fa11dc63b3c25bf570fd39e82f79baa757045cbb88339462344219`，BLE/IO关闭、未烧。旧扫描391568字节bd74b761…与已验收显示13117988…回滚门禁保持通过。普通GATT隔离构建仅组件/ELF链接检查，无新bin/连接设备入口。

## 真机步骤与边界

最终门禁修订后全量40/40 ASan/UBSan再跑通过（37.75秒）；门禁31项/采集18项通过。下述烧写与回读已完成，不等于真机执行：完整16MiB备份SHA256 `2c4f6224523cb5e6183eb165ee126369515535471a9866d85e4a1918f17443fa`，当前扫描/历史尾部/显示与原厂回滚核验通过；专用792c5409…三个镜像Hash verified，0x410000回读SHA256 `3904b6161115bfc6740c1336b67ae1774c96fc8dc65cae07aa6989c24bae113c`，全部镜像逐字节相符，0x6000–0x7fff/0x8000–0xffff/0x76000–0x40ffff与备份相同。设备留下载器，串口无占用，尚未运行；人工启动与SDK真实退出待验。详见 [上板记录](HARDWARE-RESULTS.md)。

先核对当前应用、完整16MiB备份/分区和原厂及已验收显示回滚。按既有直接烧写授权仅替换boot/table/app，以no-reset写入/回读核对完整0x410000范围及保护字节；不擦NVS/storage/SD或旧尾部，不主机自动运行新应用。

随后必须先开启只读采集、再由用户完整关机/开机Pico并重连。相机无需操作，也不需要待配对；屏幕不刷新，保持旧画面是预期，不能用屏幕判断新程序运行。至少捕获同一启动的完整或明确次数结果/返回，才记录 SDK/板卡生命周期是否通过。缺日志不等于启动失败；SDK/stop/cleanup失败必须保留资源，人工完整关机后处理，不能继续尝试相机连接。

首次人工启动无连接生命周期PASS：5164–13164ms捕获17条完全一致code0/cleanup1/sdk1/board1/start0/wait0/close0的结果，13664ms应用返回。120秒2145字节、2连接/226重连传输错误，日志SHA256 `66a8fc8dfa93004294f5d7838e78633c833308742bf0b78b28db0491b9d5c7ad`；采集结束、串口释放。APP_ENTRY/STARTED/END/前3条及ROM头缺失，不补造完整20条、完整启动时序、独立电气掉电/射频功耗证据；本轮仅采证/文档，不重新编译或烧写，详见 [上板记录](HARDWARE-RESULTS.md)。

真实目标选择/单次身份读取、GR IV各型号版本、配对/Power/Operation、无线关闭负对照、关闭后电流、WLAN/HTTP和照片同步均未完成。无连接生命周期成功不能自动启用连接流程。
