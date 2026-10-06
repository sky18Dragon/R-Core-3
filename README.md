# 小纸 Pico 墨水屏相框

基于 [aitjcize/esp32-photoframe](https://github.com/aitjcize/esp32-photoframe) 移植到 Read Pico。先完成墨水屏相框功能，再扩展理光 GR 相机连接。

更新时间：2026-10-05（北京时间）。

最新部署：读取优化版404,336字节/9f6d6f6f…已仅factory更新、全长回读一致且分区保持。按用户本轮“刷白后直接执行S1显示”，清白软件成功后一次S1处理56.2秒（前轮114.3秒，单次缩短约50.8%）、GC16 36相398.130毫秒，刷新/电源退出正常。用户实拍确认“画面正常没有问题”，确认后正常stop、显示/SD/HAL清理均成功，当前STOPPED、SD卸载、串口关闭、原片保留。本轮不计白屏人工验收，不推广为其他照片、长稳或产品完成；下述62cac21b…为上一轮状态。

最新进展：方向修复版62cac21b…已完成部署/回读、清白、原片读取/EXIF/Cover/灰阶/GC16及S1实屏闭环。用户确认“正向显示正常没有问题”，验收登记后正常停止、卸载SD并关闭串口。首次e7d5bd8d…倒置保留为失败历史；完整产品仍禁止运行。见 [单张照片诊断](docs/READ-PICO-PHOTO-VALIDATION.md)。

最新软件证据：4KiB JPEG读取缓存与分阶段计时、PNG关闭后才提交显示及文件尾验证；普通/内存检查各474项、Python38项、方向6项与六固件配置通过，5原片处理结果及源校验保持。本次同一S1处理实测114.3秒→56.2秒，视觉待反馈；不重复软件测试或推广为所有照片稳定通过。

## 当前状态

上一轮方向通过版为403,184字节/`62cac21b…`：factory应用完整回读一致、4096字节分区表保持。清白78/78及S1正向实屏人工通过；photo-confirm后正常stop，显示/SD/HAL退出均成功，STOPPED、卡卸载、串口关闭、原片保留。照片处理约114秒、36相扫描合计398毫秒。当前读取优化版状态见页首，上一轮单张结果不自动覆盖新镜像、其他照片、连续运行或完整产品验收。

此前file-test仅独占新建RC3T0001.BIN，每启动一次、同名拒绝；375普通/375 sanitizer和30 Python检查及实板写/关闭/重开通过。后续file-check软件普通/ASan各388项、Python31项和SD/集成编译通过，本轮另完成部署及ESP重启/重挂读取验收，不重建该文件，不把ESP复位当SD冷断电。详见 [SD说明](docs/READ-PICO-STORAGE.md)。

原片检查：此前photo-check仅丢弃像素，S1另获ROM真机读取/解码通过。随后软件轮次已接入Read Pico文件JPEG的分块输入、≤5MiB缩放RGB、EXIF方向、居中Cover及既有灰阶/抖动处理，不整体分配压缩文件。普通/ASan各433、Python32和集成/SD/原板编译通过；5张获准原片本机业务流程通过、源SHA不变。未烧录/刷新，内存输入JPEG及PNG等旧路径、并发/关闭处理和峰值内存仍待审查，不计照片实屏或完整产品通过。见 [照片检查](docs/READ-PICO-PHOTO.md)。

PM锁候选`6f2a4ea…`已仅写factory并完成305,104字节全长回读，分区表写前/写后逐字节不变；bootloader、NVS、PHY、storage和PMU校准未写。它保持精确684行、18MHz、factory VCOM、默认36相、gamma和80MHz存储，只在显示事务内持有CPU/APB max锁，固定厂家子模块保持干净。

上述是此前已验收显示镜像，本轮写前回读再次匹配；已被SD诊断替换，不代表当前SD固件可显示图案。屏幕保留旧画面不作为SD运行证明。

根因是项目开启自动DFS时，EPDiy直接LL RMT/APB时钟和CPU feeder没有事务PM锁。旧GC16每相30.5–48.5ms、HL draw 1.206–1.686s，浅灰被过驱到白；加锁后唯一官方局部页36/36相、11.090–11.112ms/相、合计399.327ms，回到官方量级。静态清白获用户确认，实拍16阶中心亮度严格单调，旧05–15并白消失；图案确认后显示/HAL正常释放，最终STOPPED。

16灰阶故障根因已经闭环，但P2整体仍未完成：重复性、其他标准图案、照片解码链路、历史欠载/黑带与长期稳定继续按计划验收。固定上游同步绘制内部仍使用`portMAX_DELAY`，本轮未触发挂起。

P3已接入唯一 `/storage` SD适配、只读卡检测与安全卸载租约，新增独立手动SD入口；开机不挂载、不自动格式化，卸载异常锁存到重启。普通/ASan各335项、Python23项和原板/四Read Pico配置编译通过。完整产品入口、实体卡与照片链路待验收；只读/显示入口不操作SD，本轮没有烧写或刷新屏幕。见 [SD说明](docs/READ-PICO-STORAGE.md) 与 [TODO](TODO.md)。

后续业务层集成已完成软件检查：完整PhotoFrame栈独立编译链接、运行入口默认禁止；SD业务无自动fallback，Read Pico禁止NVS自动擦除与整分区Web reset，显示元数据不再截断长路径。普通/ASan各356项、Python26项、原板及五Read Pico配置编译通过。尚需测试卡及人工上板验收，未开放产品启动/迁移、未烧写。见 [集成说明](docs/READ-PICO-INTEGRATION.md)。

方案和边界见 [16灰阶验证方案](docs/GRAY16-VALIDATION-PLAN.md)、[验证记录](docs/VALIDATION.md)。仅更新 factory 应用；不写 bootloader、分区表、NVS、PHY、FAT 或 PMU 校准。原板 `build/` 与双 OTA `build-read-pico/` 不可直接烧入本设备。用户放弃完整备份，不能保证移植前旧应用完整恢复。

## 实施顺序

1. **第一阶段：PhotoFrame 移植。** 固定主工程与硬件参考版本，接入 Read Pico Board HAL、GC16 屏幕、SD 和现有 Web 相册，再完成触摸、PMU 睡眠/唤醒、图片处理与可靠性验证。
2. **第二阶段：GR 扩展。** 第一阶段验收通过后，增加独立相机图片源，按型号/固件版本验证 BLE、Wi-Fi、下载与同步。

第一阶段沿用 PhotoFrame 的相册、Web 管理、图片处理、Wi-Fi、轮播和 OTA 体系；Read Pico 固件作为硬件适配参考。内置 Web 页面是相框的管理界面，项目交付物是设备固件。

## 从这里开始

- [TODO](TODO.md)：当前进度、任务顺序与完成条件。
- [需求基线](docs/PROJECT-SPEC.md)：两个阶段的范围与验收。
- [技术架构](docs/ARCHITECTURE.md)：主工程与板级适配边界。
- [开发说明](docs/DEVELOPMENT.md)：源码引入、版本核对和构建准备。
- [验证记录](docs/VALIDATION.md)：主机、编译、真机分别记录。
- [文件索引](docs/README.md)：当前文件与历史资料的用途。
- [上游原始 README](README.upstream.md)：PhotoFrame 原有功能说明；其中通用烧写指引不适用于尚未完成移植的小纸 Pico。
- [官方硬件资料](docs/HARDWARE-REFERENCE.md)、[上游源码核对](docs/UPSTREAM-BASELINE.md)、[剩余信息清单](docs/OPEN-ITEMS.md)：开工依据与尚待实板/使用偏好确认的事项。

研究报告中的部分版本、接口与硬件参数已核对，伪代码仍不能当作现成实现。现有构建入口支持上游原板及 Read Pico 诊断；相框移植与 GR 扩展进度以 TODO 为准。
