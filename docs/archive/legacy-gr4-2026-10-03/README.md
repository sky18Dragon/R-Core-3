# GR IV 墨水屏黑白相框

项目已于2026-10-03按用户要求终止开发和调试。源码、依赖、工具、构建配置和产物已移至废纸篓 `/Users/zhanghuilong/.Trash/R-Core-3-2-code-U2TFQd`，可恢复；当前目录仅保留文档和Git元数据，不再可构建。测试图片、日志和辅助数据统一归档至 [测试资料](</Users/zhanghuilong/Documents/ChatGPT/R-Core-3-2-测试资料/README.md>)，112份固件备份已删除至独立废纸篓，可恢复。下文状态、旧备份路径和操作步骤全部为历史，不继续执行；停止边界见 [TODO.md](TODO.md)。本次整理未恢复或擦除设备固件。

当前修正（2026-10-03）：USB无驱动接收改为独立有界适配，补直接编译实际SDK分支的回归；49项主机测试、专用固件编译/门禁通过，d14d1093…待烧写。SDK/BSP未改，确认期限不变、不自动确认。真机接收和相机身份仍待验证；烧写及重测状态见 [只读身份诊断](docs/GR4-IDENTITY-DIAGNOSTIC.md) 和 [上板记录](docs/HARDWARE-RESULTS.md)。下方旧设备摘要、未实现或待烧写均为历史阶段，当前以TODO顶部为准。

最新真机（2026-10-03）：d4f2ccaf…首次人工启动无确认负对照/退出路径通过：18条一致EXPIRED、身份无结果、cleanup/sdk/input/board全1及应用返回。不是身份读取/型号认证；完整提示与启动头缺失。采集结束/串口释放，下一轮需用户在Mac交互式终端现场新确认，命令见 [只读身份诊断](docs/GR4-IDENTITY-DIAGNOSTIC.md)。下方“待人工启动”属于此前阶段。

当前设备（2026-10-03）：只读身份诊断d4f2ccaf…已完整备份后烧写，整片16MiB回读/三镜像/保护区域核对通过；设备留下载器，未运行新应用。下一步先人工启动做“无确认不连接”及退出验证，之后才现场新确认仅读身份。屏幕不刷新，旧画面保留；不计身份认证/Gate A。以下“尚未烧写/USB未连接/设备792c…”为此前记录，见 [上板记录](docs/HARDWARE-RESULTS.md)。

最新进度（2026-10-03）：USB本轮人工确认、独立只读身份诊断入口及专用烧写门禁已实现；48组主机测试、37项门禁、真实固定SDK编译通过。新固件d4f2ccaf…尚未烧写；Pico USB当前未枚举，需要重新连接。只读连接必须现场新确认，不自动配对/写入，不计身份认证或Gate A。操作及边界见 [只读身份诊断](docs/GR4-IDENTITY-DIAGNOSTIC.md)；下文44组等是此前阶段记录。

基于小纸 Pico（Read Pico）的本地影像终端。目标是 GR IV 系列相机 → BLE/Wi-Fi 直连 → JPEG 展示副本 → 16 灰阶电子纸，日常使用无需手机或云端。

当前为 **0.1.0 开发原型**：已有可运行的主机 JPEG 转换工具、状态/日志/渲染核心，以及板卡 SD JPEG → GC16 的 Demo 0 入口。BLE 配对、预约自动同步、持久化照片库与用户菜单尚未接入。详细状态见 [TODO.md](TODO.md)。

当前设备：无连接无线栈诊断792c5409…首次人工启动生命周期PASS，17条一致code0/cleanup1/sdk1/board1及应用返回；启动头/阶段/前3条未捕获，不补记完整20条或射频功耗证据。屏幕不刷新，没有相机连接；身份/配对与Gate A仍未验证。主机40组和编译沿用实现轮，详见 [上板记录](docs/HARDWARE-RESULTS.md)。

最新开发（2026-10-03）：人工确认固定队列适配已接入同SDK扫描/host消费，启动nonce与当次世代/id核对，拒绝/过期/关闭后不激活；静态队列保留，迟到输入不触及NPL。44组ASan/UBSan、33项门禁、实际SDK隔离ELF/正式默认与无连接重编通过；实际输入驱动、界面/新动作验证、生产者退出及连接设备入口尚未实现，仍compile-only。默认b5fe671e…/无连接dac733a0…未变/未烧，设备仍已验证792c5409…，本轮无设备/相机操作。见 [确认消息通道](docs/GR4-CONFIRMATION.md)、[同SDK扫描与只读会话](docs/GR4-SCAN-SESSION.md)；以下为历史。

此前开发：Owner应用定时器改用检查式ESP timer退出，host exit后有界等待回调结束，再删除/移除事件，失败封存保留；主机36/36、门禁27项、实际SDK组件/默认编译通过。见 [应用定时器与退出边界](docs/GR4-HOST-TIMER.md)。

最新开发（2026-10-02）：有界GATT会话与实际NimBLE身份读取操作层完成，单步5秒/整轮30秒、分段读取和旧回调隔离；主机34/34、门禁27项、真实SDK组件/默认固件编译通过。没有连接/退出Owner或设备入口，compile-only门禁拒绝烧写；仅停在身份未验证，不读电源或配对。本轮未连接设备/相机，Pico仍扫描版，见 [GATT操作层](docs/GR4-GATT-IO.md)。下文33组及此前阶段为历史。

最新开发（2026-10-02）：便携GATT五字段动态读取计划/服务归属校验、严格文本与电源/工作模式参考解析完成，全量主机33/33 ASan/UBSan、默认组件编译/门禁通过，默认镜像未变。无IO或运行时调用，不计真实GATT/型号身份通过；本轮未连接设备或烧写，Pico保持bd74b761…扫描版。下一步独立有界只读适配，见 [GATT核心](docs/GR4-GATT-CORE.md)。下文均为此前阶段记录。

最新进展：扫描版bd74b761…两次人工启动均30秒停止/inactive1/code0/cleanup1、20次一致结果及应用返回。第二次用户仅关相机电源，仍1个Bluetooth Control候选；不是无线关闭负对照，不按服务位判电源或身份。官方关机广播行为及用户Live-View源码已只读核对，后续将分离发现/身份/电源/工作模式，见 [参考核对](docs/GR4-BLE-REFERENCE-NOTES.md)。本轮不改固件/不连接配对，真正无线关闭/功耗/Gate A待验；启动头缺失，下文旧状态为历史。

当前最新（2026-10-02）：Pico独立30秒被动蓝牙扫描诊断已实现；主机32/32 ASan/UBSan、默认/专用v6.1编译与门禁通过，已直接烧写并扩展回读一致。设备待人工启动采证，不自动连接/配对或操作相机；服务匹配只表示候选，不证明型号。显示固件回滚和SD库保留，默认构建不启用蓝牙。见 [被动扫描诊断](docs/GR4-PASSIVE-SCAN.md)。下文显示版与31组进展为历史阶段。

最新进展（2026-10-02）：主机 CTest 30/30（ASan/UBSan）、默认/专用ESP-IDF v6.1编译通过。指定三图设备SD JPEG解码/Cover生成/缓存索引提交→严格只读人工启动恢复→已有生成缓存SD加载/GC16→本轮用户实屏验收闭环通过：比例/铺满/竖条/残影确认回复“没有问题”。整轮mask7/code0、20次一致结果及应用返回；首图单张日志不完整，与旧RAM缓存视觉验收分别记录。BLE/HTTP相机链路、正式产品导入、首次源创建耐久性和压力仍待验，旧竖条/缓存损坏根因未确定。见 [上板记录](docs/HARDWARE-RESULTS.md)、[生成缓存显示](docs/JPEG-CACHE-DISPLAY.md)。下文早期阶段记录保留为历史。

广播识别进展（2026-10-02）：最新主机CTest **31/31**（ASan/UBSan），新广播候选解析/会话表与默认ESP-IDF v6.1组件编译通过。尚无运行时调用或真机扫描，不连接/配对/写相机；本轮不烧写，设备显示固件不变。见 [广播识别基础](docs/GR4-ADVERTISING.md)。以上30组为此前显示阶段结果。

## 快速开始：电脑上验证

本轮开发：独立 `/frame/cache-test` 三图缓存准备／提交／SD 暂存读回／GC16 诊断已实现，CTest 22/22、v6.1 编译及门禁通过；真实 SD 执行与实屏待人工开机。只在卡上保存测试缓存，不证明设备 JPEG 解码或源图耐久性。见 [SD 三图诊断](docs/SD-CACHE-SAMPLES.md)、[Cover 缓存](docs/COVER-CACHE.md)。

最新状态：Cover 三图本轮实屏通过，第三张保持；旧竖条本轮未复现，不宣称永久修复。下一阶段为独立 Cover 缓存布局与 SD 读图显示闭环；设备 JPEG／压力／写中断／相机尚未验收。见 [显示测试](docs/DISPLAY-SAMPLES.md)、[严格恢复](docs/STORAGE-RECOVERY.md)。

需要 Git、CMake ≥3.20、Ninja 和 C11 编译器。首次取得代码：

```sh
git submodule update --init
cmake -S . -B build-host -G Ninja -DFRAME_HOST=ON -DFRAME_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

转换自己的 baseline JPEG：

```sh
./build-host/host/frame_convert input.jpg output.g4 preview.pgm
```

工具输出 1216×684 灰阶预览 PGM，以及 415872 字节的原始 g4。默认完整构图 Fit、16 级量化，解析 EXIF 方向 1–8。输出使用独占创建，已有文件不覆盖。g4 暂无文件头/CRC，是开发原始数据，不能冒充最终持久化缓存。

2026-10-01 用户将展示要求更新为全屏等比居中裁切、不拉伸。工具兼容旧 Fit 默认，显式追加 `--cover` 生成全屏；可用 `--crop x y width height` 指定 EXIF 纠正后解码图 ROI（不是原 JPEG 坐标）。设备 Demo 0 已改 Cover；旧 layout=1 导入／恢复缓存仍 Fit，主机显式 --cover 使用 layout=2，不能混用。三图样片显式去白框后 Cover，见 [显示测试](docs/DISPLAY-SAMPLES.md)。

## 小纸 Pico 固件

必须使用 **ESP-IDF v6.1**，先核验 `idf.py --version`；目录名不代表真实版本。

```sh
idf.py build
```

将测试 baseline JPEG 放入 microSD 的 `/frame/test.jpg`。默认原型启动后读出厂 PMU VCOM、读取测试图、Cover/量化并执行一次 GC16；没有卡或 VCOM 不可读时停止，不格式化、不修改校准。SD 三图诊断为独立显式构建，不需要电脑复制到卡。相机网络操作保持禁用。烧写会替换设备应用，设备验证与维护方式见 [开发说明](docs/DEVELOPMENT.md)。

## 文档

- [需求文档](docs/PROJECT-SPEC.md)：PRD v2.0 原始副本。
- [技术方案](docs/ARCHITECTURE.md)：协议证据、架构、可靠性与 Demo Gate。
- [GATT只读诊断核心](docs/GR4-GATT-CORE.md)：动态句柄计划、严格值解析、参考映射及IO接入边界。
- [GATT操作层](docs/GR4-GATT-IO.md)：有界发现/分段身份读取、ticket隔离、Owner调用契约与禁止烧写边界。
- [受控连接与退出边界](docs/GR4-GATT-OWNER.md)：一次连接、host队列、链路关闭证据与尚未完成的timer/SDK退出。
- [开发计划](TODO.md)：计划、当前实现和验证结果。
- [存储核心进展](docs/STORAGE-CORE.md)：多照片日志与介质适配边界。
- [存储格式](docs/STORAGE-FORMAT.md)：库绑定日志、双快照恢复与故障测试。
- [主机文件参考适配](docs/FILE-STORE.md)：文件落盘、双槽回读、日志回收和平台边界。
- [照片导入事务](docs/PHOTO-IMPORT.md)：意图、版本/CRC 缓存、恢复与离线 `frame_import` 工具。
- [Cover 缓存与只读加载](docs/COVER-CACHE.md)：布局隔离、恢复／读取门禁及 SD 显示接入边界。
- [黑白样片测试](docs/IMAGE-TESTS.md)：指定三张原图、显式缩放、待渲染后本地恢复与私有预览。
- [私人三图实屏测试](docs/DISPLAY-SAMPLES.md)：嵌入 g4，无需电脑复制到卡；Cover 三图本轮全屏／比例／无明显竖条验收通过。
- [设备存储适配](docs/DEVICE-STORAGE.md)、[第一轮上板验证](docs/HARDWARE-GATE.md)：FatFS 诊断模式和人工验证步骤。
- [只读启动探针与产物门禁](docs/BOOT-PROBE.md)：回滚后启动排查、禁止错误模式/分区产物烧写。
- [协作规范](AGENTS.md)、[显示原则](DESIGN.md)、[组件约定](docs/COMPONENT-GUIDELINES.md)。
- [开发与构建](docs/DEVELOPMENT.md)、[设备维护](docs/DEPLOYMENT.md)、[版本记录](CHANGELOG.md)。
- [BLE 固件分析](docs/protocol/gr4-v111-bluetooth-firmware-analysis.md)、[HTTP 固件分析](docs/protocol/gr4-v111-http-api-analysis.md)。

## 实现边界

默认固件/XS JPEG 限制仍为 8 MiB 文件、4 Mi 像素、每边 8192。主机文件工具显式支持至多 32 Mi 源像素，通过原生缩放保持灰度中间图不超过 4 Mi 像素，保存的原 JPEG 不变。仅接受解码器支持的 baseline JPEG；不支持 RAW/HEIF/视频或 progressive JPEG。固定 profile 是线性灰度量化基线，尚未经过实屏影调评审；完整 EXIF、色彩空间转换、长期设备事务和省电调度仍待实现。

三款 GR IV 系列是目标范围，不是已经验证的兼容列表。路径构造仅接受已知 `100RICOH/R0000001.JPG` 风格，未知命名 fail closed。不会发送相机删除、拍摄、升级或猜测的 BLE 控制指令。

## 上游与许可

Read Pico 官方代码通过子模块锁定在 `28cde682a4468a581c278761922724f57d976418`，保持上游源码不变。主体、EPDiy、波形及 JPEG 等各自许可证见 [第三方说明](docs/THIRD-PARTY.md)。本项目新增源码的发布许可尚未选定；引用上游不自动为新增代码授权。
