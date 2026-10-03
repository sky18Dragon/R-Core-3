# GR IV 应用定时器静默与退出

日期：2026-10-02。本阶段只解决受控连接 Owner **自己的定时器**，仍是 compile-only，不接设备入口、不烧写、不计 Gate A 或整机清理通过。

后续阶段已新增独立生命周期及SDK内部timer guard，见 [无线栈退出与无连接诊断](GR4-RADIO-LIFETIME.md)。本文36组和“未完成”描述是应用timer阶段历史；Owner连接仍无设备入口，不能把后续无连接诊断当身份读取成功。

## 实现

`components/camera_gr4/io/gr4_host_timer.c` 替代 Owner 的 opaque NPL callout。直接创建 ESP_TIMER_TASK 一次性定时器，回调只向非空的 NimBLE host 默认队列投递一个 NPL event；不读取 Owner 状态、不调用 GAP/GATT、不自行重启。event/queue 指针在定时器存活期间不变。host 消费事件后才重启下一次 100ms；禁止覆写仍活动的定时器，也不从 host 并发移除 timer task 正在投递的事件。

完成时 `request_stop()` 是非阻塞、建议性停止；ESP_OK 或“已 inactive”不证明回调退出，排队事件仍可抵达，只由 Owner 终态拒绝后续工作。其他停止错误会标记 `close_error` 并清除私有身份结果，不伪装清理成功。

`retire_after_host_exit()` 必须由正常外部任务调用：已确认 host run 返回、所有重启调用方停止、SDK 默认队列和 NPL 仍有效。不是从 host/timer 回调或 ISR 调用；函数拒绝 ISR，其他执行者与 host-exit 条件仍是未来 wrapper 必须提供的契约，不能凭传入超时自动证明。

顺序必须是：

1. 封存定时器，禁止此后重启/初始化。
2. 调用固定 IDF 的 `esp_timer_stop_blocking()`，有界等待回调退出；超时上限 1000ms，拒绝零值、零 tick 与无限等待。检查结果为 ESP_OK。
3. 检查 `esp_timer_delete()` 成功；其删除内存由 timer task 延后执行，接收成功后立即放弃该 handle，不再查询、停止或重复删除它。
4. 移除已排队的应用 event，核对已不在队列，再 deinit event 并核对其指针清零。
5. 标记仅该应用定时器已 retired。

stop-blocking 或 delete 失败不删除 event、不释放 queue/Owner/tickets，也不让 SDK/NPL 进入下一步销毁；封存后可由同一外部执行者重试，不能重新启用定时器。若 delete 已成功但 event 移除/释放证据不足，保留 event/queue，重试时不再使用已删除的 timer handle。重复成功退休不会再操作 SDK。

固定 SDK 的 `ble_npl_eventq_put()` 在普通任务使用 `portMAX_DELAY`，队列满时 timer task 可能阻塞；1000ms 屏障超时必须保留资源，不强杀或复位。内部 NPL event 分配代码会 assert，合成 mock 的空分配防御测试不证明真实 SDK 能软失败返回。

## 本地 SDK 依据与未完成项

核对固定 ESP-IDF v6.1 commit `fff9895c82d744c7237be8847347bdd1b07c6643`，不修改 SDK：

- `components/esp_timer/include/esp_timer.h` / `src/esp_timer.c`：正常任务的 stop-blocking 在锁内解除定时器，并有界等待 FL_CALLBACK_IS_RUNNING 清除；回调自身上下文的 ESP_OK **不**提供相同的结束证明，故不允许在 timer 回调退休。delete 将删除事件入队，不能在成功后继续使用 handle。
- `components/bt/host/nimble/nimble/porting/npl/freertos/src/npl_os_freertos.c`：NPL callout stop/deinit 隐藏 ESP timer 返回值；eventq_put 可无限等待，event 对象需要延后释放。本模块不读取 opaque callout 布局、不调用其释放接口。
- `.../porting/nimble/src/nimble_port.c`：port_stop 内部两次无限 sem 等待；其 stop callback 不传播底层 status。port_deinit 中默认队列 deinit 在 ble_hs_deinit 前。
- `.../nimble/host/src/ble_hs.c` / `ble_hs_stop.c`：SDK 自己仍有 host/stop callout，后续 deinit 经同一个 void 接口释放；Owner 定时器退休不能替它们证明静默。

因此还不能直接复制旧 Observer 的成功退出流程到 Central/GATT wrapper，或在应用定时器退休后立即调用 port/NPL deinit。下一步：独立 watchdog、SDK stop 工作任务与有界主任务等待、host-exit 证据、**SDK 内部 timer/队列/transport 的完整静默策略**、controller 返回状态及 PMU/I2C 清理。身份读取、实际机型版本、配对、Power/Operation 与功耗仍未验证。资源保留只避免提早释放，不意味着无线已停。

## 验证

新 `gr4_host_timer` CTest 直接编译实际适配源码，ESP timer/NPL/FreeRTOS 使用确定性的合成 mock：验证只投递不在 timer task 运行 Owner、host 单次重启、活动计时拒绝覆盖、排队与退休竞态、正在投递时屏障等待、超时保留、所有 stop/delete 错误、部分释放重试、空初始化/创建失败、零 tick/ISR 拒绝、重复退休。Owner 原有连接/GATT/竞态测试改为连实际新定时器，新增停止失败清除身份；不提供 opaque NPL callout mock，误用会链接失败。

最终主机全量 36/36 ASan/UBSan 通过（34.61秒），门禁27项；实际固定 SDK 组件编译通过，包含真实 `esp_timer_stop_blocking` 引用。默认固件编译/门禁通过，359440字节、SHA256 `02b2254cce5a3fb2c8a192908b1cb8f17d3d222b9ef01c4c3d603c4e4336a609`，仍关闭 BLE/IO，未烧写。隔离构建只有组件库，无 GATT 应用镜像，烧写门禁不变。合成 interleaving 不是真实线程并发/ESP timer task/射频或设备退出验证。

本轮无串口/复位/烧写/相机连接或控制、SD/NVS/PMU 校准操作；SDK/BSP 工作树保持未改，Pico 扫描固件及显示回滚不变。
