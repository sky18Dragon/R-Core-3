# Pico 有界被动蓝牙扫描诊断

更新时间：2026-10-02。仅用于 GR IV 链路第一步采证，不是配对或照片同步功能。

当前状态：修正版bd74b761…两次人工启动扫描流程PASS，均候选1、30秒独立截止/inactive1/stats1/code0/cleanup1、全部20次一致结果及应用返回。第二次用户仅关相机电源，服务位2；首次待配对服务位1。真正无线关闭负对照仍未执行，服务位不判电源/身份；启动头/APP_ENTRY/STARTED缺失，不计独立掉电/射频功耗或Gate A。关机广播官方说明与用户参考源码见 [参考核对](GR4-BLE-REFERENCE-NOTES.md)。旧99c…code9仍为失败。

## 实现契约

独立模式 `FRAME_GR4_SCAN` / `CONFIG_FRAME_GR4_SCAN`，入口 `main/gr4_scan.c`。与所有图片/SD/启动探针模式互斥。默认固件不启用 Bluetooth。便携解析见 [广播候选基础](GR4-ADVERTISING.md)，无线 IO 留在主板适配层。

单次启动只允许一个会话：NVS初始化失败直接停止、不擦除；使用现有引脚配置创建I2C、仅PMU握手与HOST_READY，不初始化BSP、SD或EPD，不读写VCOM、不刷新屏幕。PMU退出时移除设备/释放I2C，诊断不是正常按键菜单程序。

NimBLE仅Observer角色，Central/Peripheral/Broadcaster/SMP/GATT连接能力关闭；不建立连接、配对、广播，不发送主动scan request，没有WLAN控制、远端ACK、拍摄、升级、删除或HTTP调用。PHY仍执行SDK所需射频初始化/校准，但关闭其校准结果的NVS持久化；不修改PMU/VCOM校准或SDK/板卡驱动。

扫描参数固定30秒、100ms间隔/50ms接收窗口、普通非白名单扫描。关闭控制器重复过滤，以免漏掉同一地址交替出现的两个服务UUID；应用最多8候选、饱和计数。候选不证明相机型号/固件/可信身份，GR IV / HDF / Monochrome必须分别验证。

## 生命周期与失败

先创建结果事件组，再初始化SDK并检查host任务创建结果；等待同步最多5秒。适配层用FreeRTOS单调tick独立计时30秒，处理tick回绕和非事件唤醒，不依赖SDK结束回调。到期禁收后续报告，显式取消发现并查询 `ble_gap_disc_active()`，必须inactive=1才可能成功；EALREADY也必须确认inactive。SDK正常完成须reason0且已达到期限，提前完成、复位或host退出均失败。不自动重扫，保留首错，然后独立停止任务调用SDK stop。

固定SDK `nimble_port_stop` 内部有无期限等待，故主任务只等待停止结果和host退出最多10秒；只有两者确认且stop成功才deinit。不得强杀SDK任务、在其仍运行时删除事件组或主动复位。如果stop超时/任务创建失败/deinit失败，保留仍可能被引用的静态对象，输出cleanup=0，不读并发候选表、不清其地址，也不宣称无线已停止；须人工完整关机后处理。初始化失败的SDK嵌套回滚错误无法独立核实，也输出cleanup=0。

上述时间限制是应用事件等待与正常扫描的预算，不保证所有SDK/I2C底层调用的端到端期限；若SDK/host失去响应，传统扫描的主机定时结束也不能保证执行。对此只判失败，不包装为正常省电或可靠退出。

只有host确认退出后才输出候选临时编号/服务位与聚合计数，并清空私有地址表。不记录相机名称/地址/序列号/原始广播，PMU与BLE初始化可能输出标识的SDK日志标签关闭，NimBLE日志NONE。只输出阶段、数量、错误分类，不提供GATT访问能力。

| code | 含义 |
| --- | --- |
| 0 | 同步、正常扫描结束及清理完成；候选为0也是成功观察，不是相机连接成功 |
| 1–3 | NVS / I2C / PMU或HOST_READY失败 |
| 4–6 | 事件组 / SDK初始化 / host任务创建失败 |
| 7–9 | 同步超时或复位 / 扫描启动失败 / 提前完成、复位、host提前退出或非正常结束 |
| 10–12 | 取消发现 / stop任务创建 / stop或deinit失败或超时 |
| 13 | PMU或I2C退出失败 |

`GR4_SCAN_STARTED`、`GR4_SCAN_CANDIDATE`、`GR4_SCAN_COUNTS`、`GR4_SCAN_END`，最后20次间隔500ms的 `GR4_SCAN_RESULT` 为同一结果重播，不是20轮扫描。`cleanup=1`只在所请求清理返回成功时输出，并非独立射频功耗测量。单一错误码保留首错，cleanup单独报告退出是否确认。

结果重播增加 `end=deadline/sdk/fault/not-started`、`elapsed_ms`、`inactive`、`stats` 与完整聚合计数。stats=1仅在host退出且SDK反初始化成功后复制统计，不读并发会话表；晚采可以获得相同聚合结果。cleanup=1但code10/inactive0仍是失败，不能认为取消已确认；也不把stats0的零计数当实际空扫描。elapsed_ms是应用tick预算证据，不是独立射频时间测量。

## 构建、测试与产物

使用固定ESP-IDF v6.1。它的BT链接片段处理会拆分含空格的SDK真实路径，符号链接仍会被解析回原路径；本机使用同commit的无空格临时克隆副本，未修改SDK代码。推荐在无空格的真实SDK安装路径构建：

```sh
idf.py -B build-gr4-passive-final -D SDKCONFIG="$PWD/build-gr4-passive-final/sdkconfig" -D FRAME_GR4_SCAN=ON build
```

SDKCONFIG与CMake选项必须匹配；错误模式/角色/日志/持久化配置或混入连接/写入/显示/存储入口的产物由 `tools/verify_firmware.py --mode gr4-scan` 拒绝，失败目录不能烧写。初期缺少BT配置与路径空格失败均被门禁/构建阻止，未烧写那些产物。

修正版最终主机CTest32/32（ASan/UBSan，35.62秒），实际扫描入口28种模拟生命周期覆盖无SDK结束回调、空扫描、SDK完成/提前完成、host提前退出、取消后仍活动、EALREADY仍活动、tick回绕、非事件唤醒、禁收晚报告及全部20次重播相同。不是实际线程竞争或无线验证。固件门禁26项、采集器17项通过；门禁新增取消及inactive符号必需项。默认及扫描专用v6.1编译/门禁通过。

修正版扫描应用391568字节，SHA256 `bd74b761f79c99a84f7b62f17d26d4aa0dcc265086ab65f1407f956ebdd704d3`。默认应用359440字节，SHA256 `2fc5a12739de6e48ba0ce801bbba2da5d9e94928a76c878acbd4c321c2e3e7b9`，未烧。SDK及板卡子模块未修改。修正版首次人工启动扫描流程通过，完整烧写/运行记录见 [上板记录](HARDWARE-RESULTS.md)。

## 真机验证方法

### 第二次人工启动：相机仅关机，仍有候选

用户澄清“只关闭了相机电源”，不是菜单关闭蓝牙。120秒 `manual-power-gr4-deadline-radio-off-01.log` 4054字节、2连接/273传输错误，SHA256 `56c494bd92b9cc5fb1d305c3712c19e5e4e94f2ad196464bd01923afd35f91c6`。30659ms候选id1/services2，30660ms COUNTS reports1581/invalid0/matches50/overflow0/candidates1及END code0/cleanup1；30660–40161ms20次一致正常deadline结果elapsed_ms30000/inactive1/stats1，同一统计，40661ms应用返回。扫描流程第二次通过，不计无广播对照；前轮service1与此轮service2不构成稳定身份或开关机映射。采集结束释放串口，无重烧/主机复位/相机操作，启动头仍缺失。官方无线开启时关机广播说明与只读GATT状态设计见 [参考核对](GR4-BLE-REFERENCE-NOTES.md)。

### 修正版首次人工启动：独立截止/退出通过

120秒采集4054字节、2连接/223传输错误（含人工断开/重枚举），私有 `manual-power-gr4-deadline-01.log` SHA256 `cf8942d84a6416b84fc0819a9da60a507e5612292d8cf3e2d593cd9e9102e964`。30662ms候选id1/services1；30663ms COUNTS reports1618/invalid0/matches55/overflow0/candidates1及END code0/cleanup1，30663–40164ms全部20次一致结果 end=deadline/elapsed_ms30000/inactive1/stats1，同一统计；40664ms应用返回。用户确认人工开机，采集结束释放串口，本轮不复位/重烧/操作相机。启动头/APP_ENTRY/STARTED未捕获；依据代码与清理后结果核实本轮扫描流程通过，不补造完整启动/独立电气掉电或射频功耗测量。详见 [上板记录](HARDWARE-RESULTS.md)。后续须相机关闭蓝牙对照，候选不证明属于本台相机/型号身份。

### 首次人工启动：候选收到，正常结束失败

用户确认“已开机”及相机“待配对”。首次采集120秒0字节/1连接/0错误，USB节点时间晚于窗口，不能凭空日志判启动失败。无复位补采120秒1569字节/1连接/0错误：35656ms候选id1/services1半行；36707–45207ms18次一致code9/cleanup1；45707ms应用返回。未捕获启动头/APP_ENTRY/STARTED/完整COUNTS/END，也不能声称20次全部捕获。半行的编号和服务位完整，支持至少一个协议候选，不证明型号或相机稳定身份；cleanup1对应清理API成功，不是射频功耗测量。记录见 [上板结果](HARDWARE-RESULTS.md)。本轮无复位/烧写/JTAG/相机控制，两次采集已释放串口。

### 固定SDK问题与适配层修复

本地固定commit `fff9895c82d744c7237be8847347bdd1b07c6643`：`nimble_opt_auto.h` 的NIMBLE_BLE_CONNECT为Central或Peripheral；本版均关闭，仅Observer。`ble_hs.c` 中 `ble_hs_timer_exp` 的GOOD分支把 `ble_gap_timer()` 放在NIMBLE_BLE_CONNECT条件内，故Observer-only不处理传统扫描期限。`ble_gap.c` 原本在该定时器到期时关闭发现并发出reason0的DISC_COMPLETE，而本版等不到它，35秒等待后报告code9并取消/清理。这是源码与约35秒日志吻合的定位，不是相机拒绝配对，也不是配对成功。

已仅在本项目适配层增加独立30秒截止，显式取消并核实发现不再活动，再确认host/SDK退出；仍关闭连接角色，不修改SDK。异常复位、取消失败、host提前退出或缺失清理保持失败，未简单吞掉code9。补充无SDK结束回调的真实入口mock及细分/晚采结果，主机/构建/烧写回读及修正版首次人工启动扫描流程通过。旧22场景自动完成mock未覆盖该SDK编译条件，历史真机code9仍保留为失败。

### 修复后复验流程（首轮已通过，对照/重复待执行）

先确认相机蓝牙启用/待配对，放在Pico附近；先开启不复位的串口采集，再按提示完整关机/开机Pico。核实本版APP_ENTRY、30秒扫描、候选服务位/计数、END及20次一致结果；正常结果应code0/cleanup1/end=deadline或sdk/elapsed_ms≥30000/inactive1/stats1。若只捕获部分重播，记录实际次数及缺失启动证据，不补造完整捕获或独立掉电证明。扫描无候选时先核对相机是否广播，不进行自动配对或猜测写值。

成功观察后安排真正无线关闭/不广播对照：由用户在菜单关闭无线通信工作/动作模式，不能用电源关机或“关机时传输”开关替代；另验重复有界会话、同地址交替服务、正常与失败退出/功耗。当前两次正常停止通过不替代失败/功耗验证；Gate A型号/版本读取与配对另行开发。此诊断不需要读卡器，不清SD，也不替代先前指定三图显示验收。
