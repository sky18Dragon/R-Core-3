# 小纸 Pico 墨水屏相框

基于 [aitjcize/esp32-photoframe](https://github.com/aitjcize/esp32-photoframe) 移植到 Read Pico。先完成墨水屏相框功能，再扩展理光 GR 相机连接。

更新时间：2026-10-04（北京时间）。

## 当前状态

边缘供数探针候选daf0b7bf…已仅写factory并全长回读一致，分区表前后不变；bootloader、NVS、PHY、storage和PMU校准未写。唯一静态清白78/78帧完整且用户确认整屏正常；同启动只运行一张`gc16-gray16-page`，GC16绘制完成、36/36相完整。每相696次source回调，12条目标行齐全，六组成对轨迹相等、五个边界全部可区分，丢弃/孤立/晚到/未完成均为0。新实拍仍复现左侧宽连续区：灰条、黑块和细线缺少预期40像素白边，棋盘左侧约前120逻辑像素并成连续色块，而u≈120后的40像素节距及右侧布局基本正常。本轮明确不通过，证据将问题收窄到LCD source回调之后、可见像素之前。已部署分析器的688/CPU1启动预填误判已在离线候选5069a6d0…修正，普通和ASan/UBSan全量各300/300及固定IDF 6.1编译通过但未部署。会话已正常`STOPPED`，屏上保留测试图；不做官方GC16铺白，不改gamma、VCOM、频率或波形。P2未完成。

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
