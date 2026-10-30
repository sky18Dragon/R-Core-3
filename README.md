# 小纸 Pico 墨水屏相框

基于 [aitjcize/esp32-photoframe](https://github.com/aitjcize/esp32-photoframe) 移植到 Read Pico。先完成墨水屏相框，再扩展理光 GR 相机连接。

目标是在本机实现Web管理、SD相册、GC16显示、触摸和PMU睡醒；交付按主机软件、固件编译、部署回读和真机验收分层判断。

**当前进度、下一步和候选身份只在 [TODO](TODO.md) 维护。** 主机/编译/部署/真机分别记录在 [VALIDATION](docs/VALIDATION.md)；[CHANGELOG](CHANGELOG.md) 与 [历史快照](docs/history/README.md) 保留过去轮次，不作为实时状态或新操作指令。

## 实施顺序

1. **第一阶段：PhotoFrame 移植。** 固定主工程与硬件参考版本，接入 Read Pico Board HAL、GC16 屏幕、SD 和现有 Web 相册，再完成触摸、PMU 睡眠/唤醒、图片处理与可靠性验证。
2. **第二阶段：GR 扩展。** 第一阶段验收通过后，增加独立相机图片源，按型号/固件版本验证 BLE、Wi-Fi、下载与同步。

第一阶段沿用 PhotoFrame 的相册、Web 管理、图片处理、Wi-Fi、轮播体系（OTA 已移除，固件经 USB 更新）；Read Pico 固件作为硬件适配参考。内置 Web 页面是相框的管理界面，项目交付物是设备固件。

## 从这里开始

- [TODO](TODO.md)：当前进度、任务顺序与完成条件（只含R1）。
- [测试策略](docs/TESTING.md)：日常 `scripts/test_quick.sh`，何时做编译与真机；[路线图](docs/ROADMAP.md)：R2及之后。
- [需求基线](docs/PROJECT-SPEC.md)：两个阶段的范围与验收。
- [技术架构](docs/ARCHITECTURE.md)：主工程与板级适配边界。
- [开发说明](docs/DEVELOPMENT.md)：源码引入、版本核对和构建准备。
- [验证记录](docs/VALIDATION.md)：里程碑汇总与候选台账。
- [文件索引](docs/README.md)：当前文件与历史资料的用途。
- [上游原始 README](README.upstream.md)：PhotoFrame 原有功能说明；其中通用烧写指引不适用于尚未完成移植的小纸 Pico。
- [官方硬件资料](docs/HARDWARE-REFERENCE.md)、[上游源码核对](docs/UPSTREAM-BASELINE.md)、[剩余信息清单](docs/OPEN-ITEMS.md)：开工依据与尚待实板/使用偏好确认的事项。

研究报告中的部分版本、接口与硬件参数已核对，伪代码仍不能当作现成实现。现有构建入口支持上游原板及 Read Pico 诊断；相框移植与 GR 扩展进度以 TODO 为准。
