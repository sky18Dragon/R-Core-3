# GR IV GATT 操作层：有界发现与身份读取

日期：2026-10-02。本文记录**操作层阶段**及其调用契约；独立的相机连接设备入口仍未启用，不具备连接烧写测试条件，不计 Gate A。后续总进度见下段。

当前总进度（2026-10-03）：Owner、SDK生命周期/内部timer guard和无连接首次真机诊断已推进；便携受控目标选择、无IO交接、同SDK延迟激活及固定确认队列/host消费完成，44组主机/实际SDK隔离ELF通过，见 [确认消息通道](GR4-CONFIRMATION.md)、[同SDK扫描会话](GR4-SCAN-SESSION.md)、[目标选择](GR4-TARGET-SELECTION.md)、[SDK退出](GR4-RADIO-LIFETIME.md)。实际人工输入/新动作/生产者退出与连接设备入口仍未实现，本阶段compile-only；下文34/36组及未完成SDK退出的描述为历史。人工确认只许可一次未认证诊断，不是手动设置为已验证，不放行Power/Operation或写入。

后续更新：受控连接Owner/host队列与链路级关闭已实现，应用timer另补有界静默与检查式退休，全量36组及真实SDK组件编译通过，见 [Owner与退出边界](GR4-GATT-OWNER.md) 和 [应用定时器](GR4-HOST-TIMER.md)。SDK内部timer/整个SDK退出和设备wrapper仍未完成；下文34组等为操作层阶段记录。

## 实现范围

便携会话 `components/camera_gr4/gr4_gatt_session.c` 只管理状态和数据。独立适配 `components/camera_gr4/io/gr4_gatt_nimble.c` 仅调用三个 NimBLE 客户端 API：发现全部主服务、逐服务发现特征、长特征读取。现有 [五字段核心](GR4-GATT-CORE.md) 完整校验后，顺序读取 Manufacturer、Model、Firmware，最终停在 `IDENTITY_UNVERIFIED`。

本阶段不接受“手动设置为已验证”的布尔放行，不提供 Power/Operation Mode 读取入口；没有型号/实际版本白名单或配对接口。公开 UUID 与三个合法字符串都不自动证明身份。相机真实值及长度/编码/权限仍待受控采证，合成测试字符串不是支持矩阵依据。

## 约束与状态

- 整轮最多 30 秒，每个操作最多 5 秒，数据到达不延期。无符号毫秒差支持时钟回绕；调用方必须使用单调时钟并持续调度，不能停止轮询后宣称超时已执行。
- 最多 16 服务、128 特征、20 个请求（1 次服务发现 + 16 次特征发现 + 3 次读取），超限停止，不截断。不依赖服务/特征发现顺序。
- 每次 SDK 操作使用独立且不复用的回调 ticket，包含会话世代、连接句柄、请求号/类型/范围/字段。旧请求、错误句柄、错误世代及终态回调不修改结果。连接句柄 0 合法，0xffff 无效。
- 完整长读取严格核对特征句柄、连续偏移和字段限额，复制 mbuf 链而不转移其所有权。仅成功结束后解析，部分响应不计已读；零字节尾片允许，但整字段为空仍失败。
- SDK 同步拒绝、异步错误、空/异常对象、复制失败、范围或读取错误、超时、断连、取消均停止后续操作，清除已读身份、临时值与计划。正常完成也清除发现表和计划，只保留有界身份文本供调用方核对；断连/取消后再清除文本。
- 无自动重试、配对、安全请求、通知订阅、GATT 缓存、bond/NVS 写入、私密字段读取或状态写入；没有原始响应日志。API 白名单之外的 SDK 函数没有进入适配对象的外部依赖。

## 调用方契约（部分已实现）

`frame_gr4_nimble_begin()` 仅接受**已建立且由外部受控管理**的连接和非零世代，不自行确认连接活性。`pump()` 启动当前未提交的操作，SDK 回调只更新会话，下一操作由后续 pump 发起，不在回调内递归调用下一 SDK 操作。

所有 pump、时钟、SDK 回调、disconnect/reset/cancel 必须在同一 host 执行者串行调度；不能直接从另一个 FreeRTOS 主任务同时访问会话。本模块不带互斥锁，也没有实现队列调度器。

存储应静态/长期存在并零初始化，每对象只允许一次 begin；不能通过重新 memset 来复用，也不能在超时后立即释放。所有 ticket 在该 SDK host 生命周期结束前保持地址和内容稳定；单次 SDK 回调返回并不证明再无回调。

Owner 必须转发与其世代匹配的 GAP 断连事件；host reset 也须取消会话。即使操作层超时或停止，SDK 仍可能持有进行中的操作或连接；必须关闭链路、核实 host 停止并有界清理，失败时保留被引用资源。**终态不等于无线已停或 cleanup=1。**现有独立Owner完成连接/链路级关闭及应用timer退休，SDK内部timer静默与整个SDK退出仍未完成，不得直接接入 `app_main` 或烧写。

## 构建与门禁

默认关闭 `CONFIG_FRAME_GR4_GATT_IO`。新增 `FRAME_GR4_GATT_IO_COMPILE_ONLY` 只供隔离组件编译，不能与扫描、显示或存储诊断组合；配置要求 Central/GATT Client 开启，SMP、缓存、其他角色与持久化关闭。仅编译库，不生成可测试的应用。

激活固定 ESP-IDF v6.1（无线相关使用真实无空格 SDK 路径），在全新隔离目录配置：

```sh
idf.py -B build-gr4-gatt-io-compile -D SDKCONFIG="$PWD/build-gr4-gatt-io-compile/sdkconfig" -D FRAME_GR4_GATT_IO_COMPILE_ONLY=ON reconfigure
ninja -C build-gr4-gatt-io-compile esp-idf/camera_gr4/libcamera_gr4.a
```

本轮使用私有隔离 sdkconfig，重新配置并确认 IO/GATT Client/Central 实际启用；核心与真实 NimBLE 适配对象编译通过，不是空的条件编译文件。对象外部依赖核对仅含三项发现/读取 API、会话函数、memcpy/memset 与 mbuf 复制。

产物门禁显式拒绝 `CONFIG_FRAME_GR4_GATT_IO=1`，即使 BT 标志/链接符号缺失也拒绝；已有 flash 目标依赖门禁。不要调用完整 build/flash 或通过直接 esptool 绕过拒绝，本阶段没有批准的设备入口。

## 验证与设备状态

新增 `gr4_gatt_io` CTest 直接编译实际 NimBLE 适配源码，SDK 使用合成 mock；不存在连接、写入、安全/配对或订阅符号，意外调用无法链接。覆盖正常三项分段读取、最大 16/128 元数据与 20 请求、16/32/128 位 UUID 转换、同步拒绝、异步错误、空对象、范围/容量/缺字段、mbuf 复制失败、异常偏移/超限/空值/NUL、所有阶段超时与时钟回绕、整轮独立截止、数据流不延期、重复 pump、旧请求/世代/连接、断连/取消清零及单次会话拒绝重用。未执行真实并发或真实 mbuf 链传输。

最终全量主机 **34/34** ASan/UBSan 通过（34.10 秒）；固件门禁 **27 项**通过（新增 compile-only 拒绝用例）。CMake 双模式冲突实测拒绝 GATT compile-only 与扫描同时启用，在配置阶段停止，不生成应用。默认 v6.1 固件编译及门禁通过，359440 字节，SHA256 `02b2254cce5a3fb2c8a192908b1cb8f17d3d222b9ef01c4c3d603c4e4336a609`，摘要相较旧默认版改变，不能称原镜像未变；该默认配置仍关闭 BLE/IO 适配，未烧写。

既有扫描产物门禁复核通过，391568 字节、bd74b761…未变。本轮没有连接串口、复位、烧写、SD/NVS/PMU 校准操作或相机连接/控制；Pico 保持该扫描固件。下一步独立受控连接入口、host 串行调度、GAP 世代隔离、退出/异常清理及模拟测试；完成新专用固件门禁后再真机读取身份，再建立机型/版本能力档案，Power/Operation Mode、配对与功耗另验。
