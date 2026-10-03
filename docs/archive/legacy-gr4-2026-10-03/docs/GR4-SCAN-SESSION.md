# GR IV 同一蓝牙生命周期：扫描与延迟只读身份诊断

2026-10-03当前：独立USB输入/现场TTY新动作、有界退出与单次身份设备入口/专用门禁已实现，48组主机/37项门禁及实际SDK编译通过，未烧写/未有相机证据；见 [只读身份诊断](GR4-IDENTITY-DIAGNOSTIC.md)。本文隔离SCAN_SESSION_COMPILE_ONLY仍仅ELF验证，无.bin且门禁拒绝。下方43组/旧摘要和未接通道描述为历史。

日期：2026-10-02。本阶段实现 host 内的扫描/确认消费/Owner 激活及生命周期扩展，**仍为 compile-only，没有现场确认输入或相机连接设备入口**。没有烧写、相机连接、真实 GATT 或型号/版本认证证据，不计 Gate A。

## 实现与调用边界

`io/gr4_scan_session.c` 使用 [受控目标选择器](GR4-TARGET-SELECTION.md)，绑定 `io/gr4_radio_lifetime.c` 同一次启动创建的专用 SDK/host。不拼接旧扫描入口，不销毁后重建 SDK，不从日志恢复地址。便携核心不增加 ESP-IDF 依赖。

| 阶段 | 执行者与动作 | 不允许的动作 |
| --- | --- | --- |
| READY | 启动前 prepare 数据；radio start_session 接受未使用的 IDLE Owner，sync 调用扫描适配 | prepare 不做 IO，不自动准备连接目标 |
| SCANNING | host 被动 legacy 扫描；独立应用 timer 每100ms向同一host队列投递事件 | 无主动 scan request、无 RSSI 选优、无连接/配对 |
| CONFIRM | host 禁收报告→取消/查询 inactive→重新取时钟 seal；仅暴露当次 epoch/id/services | 不能读取地址到日志、将 count1 视为同意或沿用上次确认 |
| READING | host 消费新鲜确认，立即 prepare 并激活 Owner；最多一次尝试 | 不重新初始化、不排队延迟连接、不自动重试/选另一个目标 |
| DONE/清理 | 先 fence 扫描/选择/GATT，再沿已有SDK清理；host退出、controller停用后退休扫描timer和Owner timer | 不提前释放、强杀/复用工作任务或以终态代替清理证据 |

调用方 prepare `frame_gr4_scan_session_t`，取得静态 ops 后调用 `frame_gr4_radio_start_session(lifetime, idle_owner, ops, session)`。ops/对象/时钟参数/Owner 全启动期存活；不能借用临时栈对象，也不能重新 memset 复用。主任务仍调用 radio wait/close；sync/reset/cancel/确认/view 均仅在 host 执行者调用，retire 仅由确认 host 退出后的外部清理任务调用。

`frame_gr4_scan_session_confirm()` 是host内部交接接口。当前可在SDK启动前绑定静态确认队列，由host定时事件消费，但**没有实际输入驱动或提示界面**。未来驱动必须从新的人工操作生成绑定当前nonce/世代/id的不可变事件；不能另一个任务直接读取/修改选择器、Owner或调用本接口，不能固定传 true。世代也不是身份认证；实际生产者需避免跨启动旧消息、按键保持态或排队旧事件重放，合成 epoch42 不是产品默认值。

## 扫描期限与停止证据

SDK命令设31秒兜底；应用观察窗口在命令成功返回后开始，仍严格30秒。这样分配/命令等待不侵占完整观察窗口，也避免SDK自然到期与应用起点的微小差异被误判提前结束。两服务交替广播不进行 duplicate filtering；interval160/window80，不改变板卡时序/驱动。

独立应用timer到期由host禁收，再调用 `ble_gap_disc_cancel()`，核对返回0或EALREADY且 `ble_gap_disc_active()==0`。固定NimBLE取消成功会重置扫描状态，不发布正常 DISC_COMPLETE；不等待一个取消接口不承诺的回调。自然 DISC_COMPLETE 只有 reason0且应用完整窗口已到才可 seal。提前结束、异常 reason、超出停止宽限、取消失败或EALREADY仍活动都拒绝交接。

取消/查询之后**重新取时钟**，核对选择器30–31秒停止宽限及最后报告≤2秒新鲜度，不用命令前时刻补造及时停止。多个地址、类型变化或格式错误报告立即失败；过期/撤销清除未消费私有地址。到期之后到来的报告与确认后的旧扫描回调不修改选择。

100ms host事件并非硬实时保证。阻塞的host无法及时执行30秒取消；主任务另有从host创建前起算的 **85秒全流程期限**（含同步、扫描、确认、连接、读取、关闭），非事件唤醒不重新计时，普通非扫描Owner仍45秒。超过期限转入有界停止，不能宣称30秒时无线已经停用；SDK/队列阻塞时保留资源并报告未清理，不强杀任务。

## 激活与结果处理

radio session activate 仅接受已同步的当前lifetime、正确done回调/参数且已prepare的Owner；reset、host exit、SESSION_DONE、外部关闭栅栏或之前激活都拒绝。主任务在停止工作任务创建/排队前设置关闭栅栏，关闭请求先发生时，迟到确认不能启动SDK连接。地址推断错误/不支持的类型消耗本轮激活机会并拒绝，不重试。

激活返回true只表示已交给Owner，不等于连接成功。Owner仍重新核对无扫描/initiator/已有目标、地址/角色/句柄，并只动态发现/读取 Manufacturer、Model、Firmware；结果仍 `IDENTITY_UNVERIFIED`。无SMP/配对/CCCD/Power/Operation Mode/WLAN/ACK/拍摄/升级/删除。

radio wait返回OK也只表示会话终止，可能是没有候选或其他失败；必须分别检查扫描阶段原因、Owner原因/链路关闭、整个SDK及板卡清理。正常Owner完成后调用close，不把扫描适配的清理误记为“用户取消”；Owner自身失败仍由Owner结果单独分类。host reset/重复sync会禁收并reset Owner、清除此前私有结果；即使读取已经完成，reset后也不允许发布旧身份。

SDK退出沿用 [SDK生命周期与timer guard](GR4-RADIO-LIFETIME.md)：取得stop完成和host退出，controller先disable，再退休扫描应用timer、Owner timer及SDK timers，预排空事件后deinit host/controller。扫描timer退休失败封存/保留并停止后续释放；不以晚事件移除或stop返回代替timer callback静默。

## 编译与验证（2026-10-02阶段记录）

新增 `FRAME_GR4_SCAN_SESSION_COMPILE_ONLY` 必须与既有 GATT compile-only 开关同时显式开启；配置标记与显式选项不一致拒绝，不能使用无连接/旧Observer/照片入口。独立配置仅添加Observer角色和内部SCAN_SESSION，保留单Central/GATT Client、无SMP/缓存/持久化/自动重试/隐私/DTM及校准NVS持久化。SDK/BSP不改。

```sh
idf.py -B build-gr4-scan-session-compile -D SDKCONFIG="$PWD/build-gr4-scan-session-compile/sdkconfig" -D FRAME_GR4_GATT_IO_COMPILE_ONLY=ON -D FRAME_GR4_SCAN_SESSION_COMPILE_ONLY=ON reconfigure
ninja -C build-gr4-scan-session-compile gr4_paper_frame.elf
```

实际固定v6.1组件/ELF编译链接通过，ELF SHA256 `8600923810a1e6ba00304f42f500969a900f6e0e715120eba1e6d715bd785213`。nm确认实际扫描/取消/inactive、session prepare/view/confirm、radio deferred start/activate以及Owner交接均保留，不是空条件编译。扫描对象直接SDK依赖仅扫描/取消、扫描与initiator活性查询、NPL函数表，连接仍交给Owner。没有.bin；真实门禁明确拒绝 Deferred scan session compile-only，不通过esptool绕过。

主机新增 `gr4_radio_deferred` 与 `gr4_scan_session`，全量 **43/43 ASan/UBSan，40.88秒**（初轮41.26秒）。前者直接编译真实lifetime，以合成SDK/会话hooks覆盖旧33场景及11个延迟场景，含85秒无事件期限、单次激活/错误回调、关闭栅栏、重复sync/reset、Owner完成后reset、地址推断错误及额外timer退休失败。后者直接编译真实扫描适配及便携选择器，以合成SDK/timer/Owner交接覆盖被动参数、命令延时/完整窗口、回绕、空/歧义/陈旧候选、取消失败/仍活动、取消耗时后重新取时钟、提前/异常完成、错确认/过期、激活失败/旧事件、重置不查询GAP、正常清理不误分类及失败退休。不是整个SDK/真实Owner并发的一次端到端运行。

初次延迟mock把同步期间发生reset的start错误预期为成功，测试拒绝后已修正期望，再全量回归；未放宽实现。门禁 **33项**通过，新增SCAN_SESSION即使缺IO/镜像仍拒绝、无连接版拒绝session/start/activation符号；缺父compile-only选项的真实CMake配置在SDK初始化前拒绝。

默认编译/门禁通过：359568字节、b5fe671e…未变/未烧。无连接诊断重编/门禁通过：417296字节、SHA256 `dac733a08bd932923eb5373fb14cb7c13b961ae1f2cf1d55aa012d80fb61f07e`，未烧、不能套用此前09101d54…或设备792c5409…的运行证据。当前设备仍已验证的792c5409…无连接诊断版，旧私有回读及显示/扫描/原厂回滚保留。

本轮无串口/主动复位/烧写、相机连接/操作、SD/NVS/PMU校准操作；固定SDK、NimBLE和锁定BSP tracked-clean。下一步现场确认事件通道、独立设备入口/日志与输入生产者退出屏障、入口故障测试/新专用门禁，再受控上板。当前无需开关机或操作相机。
