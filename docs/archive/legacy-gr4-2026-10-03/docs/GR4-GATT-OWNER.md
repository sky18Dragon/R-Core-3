# GR IV 受控连接管理器与退出边界

日期：2026-10-02。本阶段实现一次性连接与**链路关闭**管理，不是完整设备入口。仍为 compile-only，禁止烧写；相机型号/版本及真实链路未验证。

当前更新（2026-10-03）：扫描→确认队列/host消费→Owner的同SDK延迟激活已实现，44组主机测试及真实SDK隔离ELF编译通过，见 [确认消息通道](GR4-CONFIRMATION.md)、[同SDK扫描会话](GR4-SCAN-SESSION.md)、[目标选择](GR4-TARGET-SELECTION.md)。SDK内部timer guard、独立watchdog和分阶段退出也已实现，无连接诊断首次真机生命周期PASS，见 [SDK生命周期](GR4-RADIO-LIFETIME.md)。实际人工输入/新动作/生产者退出和连接设备入口仍待开发，本阶段compile-only，不能直接沿用旧scan地址或自动确认。以下35/36组与“SDK timer未解决”描述是历史阶段，非当前整体进度。

## 已实现

`components/camera_gr4/io/gr4_gatt_owner.c` 接入 [身份读取操作层](GR4-GATT-IO.md)。调用方先明确选择一个广播候选并在 RAM 提供当次 public/random OTA 地址，`prepare()` 不做 SDK IO。专用、全新、单连接 host 的 sync 回调调用 `synced()`，只发起一次连接，不遍历设备、不重连、不沿用 bond。

连接成功后核对 SDK 描述符的连接句柄、OTA 地址和 Central 角色，再开始公开 Manufacturer/Model/Firmware 读取；地址匹配只证明连接目标一致，不证明 GR IV 身份。目标不符或句柄非法时不读取；无配对、通知、Power/WLAN/ACK、拍摄、升级或删除。

连接最多 5 秒，读取沿用整轮 30 秒/单步 5 秒，关闭最多 5 秒。100ms 应用 ESP timer 投递 NPL event 到 NimBLE 默认 host 队列，pump/GAP/定时操作串行执行；拒绝空队列，不能退回 timer-task 直接执行。迟到的成功连接事件自身也核对连接期限，不能在 host 队列延迟时绕过超时。新定时器的有界静默与检查式释放见 [应用定时器退出](GR4-HOST-TIMER.md)。

编译只允许单连接 Central、无 Peripheral/Broadcaster/SMP/NVS persist，强制 SDK 日志关闭与 ESP timer callout 路径。固定 SDK 的非 ESP timer 路径首次分配分支未设置 eventq，且 reset/stop 存在 `portMAX_DELAY` 等待，不满足本阶段假设；不修改 SDK 以绕过。

## 关闭证据与失败处理

- 先取消操作层会话，阻止后续 GATT 和旧回调，再取消连接尝试或终止已建立链路。
- `conn_cancel()` 返回 0 不算完成。即使 initiator 已 inactive，仍等待尝试的终态回调并确认目标连接不存在；缺少回调则关闭失败。
- 取消恰好遇到连接成功，只关闭该连接，不读 GATT；沿用原关闭截止，不额外延长 5 秒。
- `terminate()` 返回 0 / EALREADY 不算已断连。须匹配断连事件、确认 SDK 查不到该句柄且 initiator inactive；或者 ENOTCONN 与独立不存在查询一致。矛盾状态、超时、终止失败均 `link_closed=false` / `close_error=true`。
- host reset 不使用处于重置过程的 GAP 查询补造成功；仅停止会话、报告未知关闭状态。意外安全/重复配对事件拒绝并关闭，不删除 bond 或填写密码；无关句柄不关闭本链路。
- 完成只通知一次，已排队的旧 callout 不重启操作。目标地址在终态清零。三项身份仅在完整读取且链路关闭验证通过时保留在结果的私有有界 RAM 中，仍 `IDENTITY_UNVERIFIED`，不直接写日志；退出失败则清除。

`on_done` 必须有界且非阻塞，只复制结果/通知外部任务，不能在回调中启动新的 IO。对象、GATT tickets 和 timer/event 资源均为单次生命周期，不提前释放或重新 memset 复用。

## 历史退出边界：应用 timer 已补时的 SDK 缺口

固定 v6.1 `npl_os_freertos.c` 的 callout deinit 为 void：忽略或仅记录 ESP timer stop/delete 失败，然后释放 event/callout 内存。host run 返回只证明 host 执行者退出，不能证明另一 ESP timer task 没有正在进行的回调。直接释放可能使仍在使用的资源失效。

目前已移除 Owner 的 opaque NPL callout：应用定时器使用有返回值的 ESP timer 接口。`timer_retire_after_host_exit()` 在外部正常任务、确认 host exit 且队列/NPL 仍有效的前提下，先有界等待回调静默，再检查删除、移除排队 event、释放 event；失败封存并保留资源。旧 `callout_stop_after_host_exit()` 已删除，不要沿用。Owner/tickets 本身仍保留整次 boot。本模块仍**不调用 nimble_port_stop/deinit**，测试也不提供 NPL callout mock；不能将应用 timer 已退休理解为 SDK 所有 timer 已静默。

下一阶段设备 wrapper 必须补：SDK **内部**定时器/队列/transport 的静默策略、主任务独立 watchdog、SDK stop 独立任务/有界等待、明确 host exit、SDK/controller deinit 结果及 PMU/I2C 清理。固定 SDK 的 port_deinit 在 host timers 释放前先销毁默认队列；不能仅靠应用 timer 屏障放行。未完成前不得调用 port/NPL deinit 或直接接入 `app_main`。

`link_closed` 只表示本次尝试/链路在该终态时取得关闭证据，不表示射频停用、全局无连接、认证、相机电源状态或 `cleanup=1`；整个 SDK 的关闭必须另验。调度器阻塞也不能靠 host 队列中的定时器自证硬实时截止，外部 watchdog 尚未实现。

## 验证结果

最新定时器更新：全量36组主机与实际SDK组件/默认编译通过；本轮无设备/相机操作，见 [应用定时器退出](GR4-HOST-TIMER.md)。下列35组是此前 Owner 实现轮记录。

主机新增 `gr4_gatt_owner`，直接编译实际 owner 和 GATT 适配源码。SDK/NPL 使用合成 mock，覆盖一次连接/不重试、只在 host 执行者调用 SDK、读取后主动关闭、目标/角色错误、同步与异步拒绝、忙扫描/已有目标拒绝、空队列/定时器失败、连接期限与回绕、回调迟到、取消成功竞态、不延期、关闭超时/矛盾状态、终止失败、GATT 故障、意外断连、host reset、安全事件拒绝、错句柄、终态/旧回调及禁止自动释放。没有真实并发、射频、timer-task 静默或相机身份验证。

最终全量 CTest **35/35** ASan/UBSan 通过（38.38秒），固件门禁 **27 项**通过。真实固定 ESP-IDF v6.1/NimBLE owner 组件编译通过，只生成库、不生成专用应用；默认固件编译/门禁通过，359440 字节、SHA256 `02b2254cce5a3fb2c8a192908b1cb8f17d3d222b9ef01c4c3d603c4e4336a609`，未烧写。compile-only 显式拒绝烧写的门禁保持不变。

本轮没有串口连接、复位、烧写、相机连接/控制或 SD/NVS/PMU 校准操作；SDK/BSP 未修改。Pico 保持 bd74b761…扫描固件，产物门禁复核通过；显示回滚及 SD 库不变。
