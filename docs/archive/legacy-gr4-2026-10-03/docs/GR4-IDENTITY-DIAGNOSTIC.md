# GR IV 单次只读身份诊断

当前接收修正（2026-10-03）：独立 `io/gr4_usb_rx.c` 已改用固定SDK的无驱动预读取路径，O_RDONLY、每轮最多32次单字节读取，禁止driver_install/use_driver混用。49/49主机CTest（46.67秒，C目标ASan/UBSan）及实际SDK专用编译/门禁通过；app433232字节、SHA256 `d14d10936aac5bb4f312f13ac247437216645c5c4f63ad3c567e790e740ecd5f`。尚未计真机接收/身份读取通过，备份烧写状态见 [上板结果](HARDWARE-RESULTS.md)。SDK/BSP源码未改，确认期限和现场新动作要求不变。此前confirm-04到期不是相机身份/连接失败；旧无确认清理证据不证明接收可用。

最新真机记录（2026-10-03）：首次人工启动不发送确认，补采18条一致code11/flow6(EXPIRED)/lengths0,0,0/cleanup-sdk-input-board全1及应用返回，无确认负对照/退出路径通过。完整提示/APP_ENTRY/END/前2条未捕获，不计确认工具端到端或身份读取。原120秒采集0字节，45秒补采3207字节，仅同一次用户启动，见 [上板记录](HARDWARE-RESULTS.md)。下一轮必须现场TTY新确认，不能重用本轮提示或代发y。下文“待首次人工启动”为历史。

## 下一轮本机操作

先保持Pico现状，确认附近只有本次要测试的自己的GR IV系列相机，蓝牙开启；不需要执行配对。用户在Mac交互式终端执行下列命令（本轮日志文件尚不存在；不要同时开启其他串口软件）：

```sh
/Users/zhanghuilong/.espressif/python_env/idf6.1_py3.14_env/bin/python \
  '/Users/zhanghuilong/Documents/ChatGPT/R-Core-3 2/tools/confirm_gr4_identity.py' \
  --port /dev/cu.usbmodem2101 \
  --output '/Users/zhanghuilong/Documents/ChatGPT/R-Core-3 2/.cache/pico-gr4-usbfix-dIGslm/manual-identity-confirm-05.log'
```

终端显示“USB已打开；可人工启动Pico”后，完整关机/开机Pico并重连USB；等约30秒扫描。只有终端出现完整本轮候选提示且自己核对唯一相机时，4秒内新输入y+Enter同意一次只读身份连接；n+Enter拒绝。没有新提示就不输入、不自动补发，过期/断开/短写不重试。保持工具运行至120秒结束，随后回复结果以核对日志；发送成功或未见报错不能代替读取/清理验证。仍不会刷新屏幕。

最新设备状态（2026-10-03）：d4f2ccaf…已完整备份/回滚核验后按既有授权烧写，整片16MiB回读通过，全部保护区域与备份相同；设备留下载器、新应用未运行、串口已释放。先人工启动进行不发送确认的负对照/退出验证，之后另次启动才用TTY现场新确认读身份。记录见 [上板结果](HARDWARE-RESULTS.md)。下文未连接/未烧写为实现阶段历史，不能误作当前状态。

日期：2026-10-03。新增专用设备入口、USB 输入生产者、人工终端确认工具及专用产物门禁。默认仍关闭 BLE；普通 GATT/扫描会话隔离构建仍不可烧写。

本模式只验证一次受控的公开身份读取和退出，不是正式同步固件。只支持 GR IV 系列目标范围；候选 UUID、唯一候选、用户确认和三字段读取成功均不构成型号认证或版本兼容验收。暂不输出身份原文，不能据长度推断具体型号/固件版本。Gate A 仍未通过。

## 入口和时序

`FRAME_GR4_IDENTITY_DIAGNOSTIC=ON` 唯一调度 `main/gr4_identity.c`，排斥其他诊断模式与 compile-only 开关。静态对象全启动期保留，只运行一次。

1. NVS 初始化，不 erase/retry；仅 I2C/PMU HOST_READY 握手，不初始化 BSP/SD/EPD，也不读取或写入 VCOM。
2. 在启用无线前通过 `esp_fill_random` 取得非零 nonce64/epoch32；零值立即停止，不使用固定回退。此时 SDK RNG 可能依赖此前种子，不保证真随机、绝对不重复或密码安全。它只用于降低旧消息误匹配风险。
3. prepare 会话、初始化并绑定两条深度1静态确认队列，启动独立 USB 输入任务。
4. 同一 SDK host 被动观察30秒，禁收/取消/inactive/完整窗口唯一候选及新鲜度均核对成功，才发布一次提示。没有提示、没有本轮回复、多个候选、拒绝或超时均不连接。
5. host 每100ms最多消费一条回复，先过期检查，再核对 nonce/epoch/id/services；ACCEPT 才允许单次 Owner 激活。5秒确认窗口不可延期。
6. 只发现服务/特征并分段读取三项公开身份；单步5秒、GATT整轮30秒，连接/关闭另有期限。停在 `IDENTITY_UNVERIFIED`，随后严格关闭链路。
7. 主任务85秒独立会话期限；输入任务≤1000ms退出屏障，随后依现有无线生命周期分阶段停止/退休 timer/核对 SDK 清理，最后 PMU/I2C 清理。失败不强杀、复用或释放仍被引用的对象。
8. 只在 host 已退出后提取结果元数据，清零局部身份副本；Owner 静态私有存储依既有契约保留。输出 END 和20条相同 RESULT（500ms间隔），不是20轮测试。

禁止配对/SMP、持久化绑定、CCCD/通知、GATT写入、MTU交换、Power/Operation Mode、WLAN/HTTP、ACK、拍摄、升级和删除原片。SDK/BSP/驱动/波形/校准源码未修改。

## USB 输入契约

`io/gr4_confirmation_usb.c` 通过独立 `io/gr4_usb_rx.c` 使用专用 native USB console 的 `/dev/usbserjtag`，独立 FD、O_RDONLY，每50ms最多32次单字节读取。固定SDK无驱动 rx_func 立即读取FIFO，空时返回EWOULDBLOCK；不设O_NONBLOCK才会进入这一预读取分支。设置该flag反而只查询未安装驱动的ringbuffer可读数0，导致始终不读取FIFO。此策略不能推广到其他SDK/安装驱动模式，也不承诺内部锁/调度硬实时。

打开前及每个字节前核对驱动未安装；stop、驱动混用或断开使整批局部输入丢弃。没有stdin/其他输入消费者，不安装或卸载USB驱动、不撤销VFS；完整配置不得并发安装驱动或修改该VFS的读取模式，运行时检查不构成任意并发安装的原子保证。专用门禁要求新RX符号，并拒绝driver_install/use_driver符号，旧接收产物不能再通过新门禁。

收到提示后先丢弃早到/局部输入，观察 RX 空，才显示当前 token。固定 SDK 的 EIO 表示 USB host 不在/断开：丢弃半条命令，重新等待 RX 空；不会延长 host 截止。提示日志 `ttl_ms=5000` 是名义上限，不是剩余时间承诺。

命令严格48个 ASCII 字符（不含 LF；也接受 CRLF）：

```text
GR4_ACCEPT <16小写hex nonce> <8小写hex epoch> <8小写hex id> <2小写hex services>
GR4_REJECT <16小写hex nonce> <8小写hex epoch> <8小写hex id> <2小写hex services>
```

控制字符/NUL/非ASCII/大小写不符/零字段/未知服务位/超长行全部拒绝并丢弃到下一 LF。便携解析器只做语法，不验证时效或人为来源；队列入队成功不证明同意有效。任何串口发送者仍可伪造命令，本诊断不是 USB 身份认证协议，应仅在可信本机、独占端口条件下运行。

停止使用原子 stop 标记和事件组屏障，不强制删除任务或由主任务并发关闭 worker 的 FD。事件确认表示最后一次 IO/通道访问已结束，不承诺任务 TCB 已回收；事件/队列/对象永久保留。超时保留资源并报告失败，不把静态队列安全当硬件静默证明。

## 现场人工确认工具

`tools/confirm_gr4_identity.py` 必须由用户在本机交互式终端运行；拒绝非 TTY/管道和非 canonical 输入。沿用已核对 POSIX pyserial 3.5 的 open 适配，禁止库层 modem 更新和 USB 输入自动清空，清 HUPCL。OS/USB 打开仍可能有电气影响，不称零干扰冷启动。

提示到达时只清空人的终端输入队列，不清 USB RX；必须现场确认附近唯一候选相机就是自己的 GR IV 系列，随后4秒内新输入 `y` + Enter（只读连接）或 `n` + Enter（拒绝）。旧聊天室“允许”、开机和预输入不会自动转换成当轮确认。

同一工具运行最多一次写尝试，在实际写之前封存；短写/错误不重试。重复 token 不刷新时间，断开丢弃局部行和提示，已见 token 不重放；超过8种 token 封存。日志延迟可能使提示已过期，固件最终复核仍决定是否连接。发送成功不代表读取/认证通过。原始日志仅写到用户指定的新私有文件，绝不覆盖。

```sh
# 在当前项目目录；日志目录必须先存在，且文件尚不存在。
/Users/zhanghuilong/.espressif/python_env/idf6.1_py3.14_env/bin/python \
  tools/confirm_gr4_identity.py --port /dev/cu.usbmodem2101 \
  --output .cache/<本轮私有目录>/manual-identity-01.log
```

示例端口不是自动设备选择，实际须先核对。工具打开 USB 后，再按现场指导人工完整关机/开机 Pico 并重连。屏幕不刷新、旧画面保持是预期，不凭屏幕判断新程序运行。工具120秒后释放端口。

## 结果解释

`GR4_IDENTITY_RESULT` 的 code：0只读流程结构完整；1 NVS，2 I2C，3 PMU，4 RNG零值，5准备/通道，6输入启动，7无线启动，8等待，9无线清理，10输入清理，11目标选择/确认/身份失败，12板卡清理。保留首错。

成功要求 code0、cleanup/sdk/input/board全1，start/wait/close全0，flow0、Owner `IDENTITY_UNVERIFIED`、三项长度1..28，verified/pair/write全0。cleanup1 + code11 可以是正常拒绝/未确认/无候选，不是身份读取成功。身份内容、相机地址和序列号不打印；长度不是认证证据。若 cleanup0，停止后续连接测试，人工完整断电处理，不能自动复位重试。

## 构建与验证

接收适配变更必须额外执行实际SDK分支回归：

```sh
cmake -S . -B build-host -G Ninja -DFRAME_HOST=ON -DFRAME_SANITIZERS=ON \
  -DCMAKE_BUILD_TYPE=Debug -DFRAME_AUDITED_IDF_DIR=/private/tmp/gr4-idf-ZjUILh/sdk
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

该SDK路径是本机缓存；其他机器指定相同锁定commit的tracked-clean checkout。普通便携主机构建不依赖SDK，此选项显式增加 `gr4_usb_audited_sdk`，缺源码/摘要不符即失败，不跳过。测试直接编译原SDK open/read/availability函数和实际接收适配，替换FIFO/连接/锁；复现旧100次EWOULDBLOCK/0消费，验证32+17字节命令解析、空队列/驱动/停止/断开/无效参数/跨轮CRLF。SDK函数只在原open体范围豁免未使用参数警告，项目实现仍-Werror。USB worker从10增至12种合成启动，补驱动已安装/提示后混用及每tick读取次数。不是真实USB/RTOS或相机证据。

```sh
idf.py -B build-gr4-identity -D SDKCONFIG="$PWD/build-gr4-identity/sdkconfig" \
  -D FRAME_GR4_IDENTITY_DIAGNOSTIC=ON build
python3 tools/verify_firmware.py --build-dir build-gr4-identity --mode gr4-identity
```

门禁核对显式独立 CMake 模式、配置/分区/BSP profile、锁定 SDK/NimBLE tracked-clean、实际 timer wrapper 调用和 app_main→identity 调度；要求 USB/扫描/确认/单次连接/只读符号，禁止无连接入口、写入/MTU/SMP/旧 port_deinit/SD/显示/NVS erase。门禁不是完整二进制安全或射频证明。

本阶段软件证据：

- 最终48/48主机CTest，C目标ASan/UBSan，42.95秒（前次58.92秒）；新增便携命令解析、实际USB worker（10种合成启动）、身份入口（22种合成启动）、人工确认工具（8项，含实际pyserial OS mock）。只读采集标记测试19项，不把确认入队或code11/cleanup1当PASS。入口首次timer stub缺类型声明、终端工具首次mock期望缺self参数，已修正后回归，不将失败运行计PASS。
- 37项产物门禁测试通过；真实固定 SDK 身份专用编译/门禁通过。初始 app432992字节，SHA256 `d4f2ccaf519e55d913cd36c095eb88d0d0ef457618bf9df2796fc58c06c29380`；若后续重编改变摘要，烧写必须以最终复核产物为准。
- 合成 pthread/SDK 替身不证明真实双核、USB驱动、整个Owner+SDK链路、相机读取或射频退出。本版尚未烧写，设备此前最后核验是792c5409…无连接诊断，旧回读/显示/原厂回滚均保留。

最终专用重编/门禁摘要不变，432992字节d4f2ccaf…；隔离ELF重编通过且无.bin，真实门禁分别拒绝身份模式借用及compile-only，身份+compile-only混用在SDK初始化前被CMake拒绝。默认Demo0使用门禁`--mode jpeg`（误用`default`已被CLI拒绝，随后按正确模式通过），本次359440字节SHA256 `dd5ebff0703e9b450a319f1e3e505239134b6a67a0692fb10b4df68e70eb788c`；无连接本次417296字节SHA256 `e167e55f67ae96616031d67a322b53c9d8afee08d5b4575602b9a390aa706ce7`。这两项不同于此前本地基线，均未烧写，不移用旧版真机证据。SDK/NimBLE/BSP tracked-clean复核通过。

当前实际人工边界：本机端口枚举一度出现usbmodem2101，随后已不存在，最终两次枚举均没有Pico USB。没有打开串口或进行任何设备操作，不能备份/烧写；请先人工重新插好Pico，相机无需操作。

下一步核对最终产物与现接 Pico，完整16MiB备份、验证当前镜像/分区/回滚后按既有直接烧写授权写 boot/table/app，并回读及核对保护字节；不自动运行新应用。之后需要人启动 Pico 并在交互式终端作本轮新确认，才能获得实际只读身份证据。型号/版本核验、真正无线关闭负对照及功耗另验。
