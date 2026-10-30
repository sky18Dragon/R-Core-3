# Read Pico 单张原片显示诊断契约

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-PHOTO-VALIDATION.md)。

## 入口与构建

照片profile只用于固定S1副本的“只读SD→JPEG/EXIF/Cover/灰阶/抖动→GC16”单张诊断，sink不发布产品NVS/快照，不运行完整PhotoFrame、网络或轮播。

激活 [固定SDK](DEVELOPMENT.md) 后只编译：

```sh
python3 build.py --board read_pico --step firmware \
  --read-pico-factory-diagnostic --read-pico-photo-validation
```

输出为`build-read-pico-photo/`及`.cache/sdkconfig.read_pico-photo`，与display/storage/integration互斥。采用精确684行、18MHz、CPU/APB事务PM锁与80MHz Flash/PSRAM；不改固定厂家源码/波形/VCOM。产物身份和最后已知设备分别见TODO，编译不能代替部署。

## 已验收与未验收

2026-10-05方向修复及读取优化候选分别获S1正向实屏确认并正常退出。读取优化版S1处理56,161ms、36相合计398.130ms；属于单次测量，无精确局部内存低水位日志，不推算总峰值。后续设备已换为SD诊断，该历史结果不代表实时照片固件或活动白参考。

倒置首轮、其余旧拒绝及“等待反馈”阶段保留在VALIDATION/历史快照；后续明确确认不倒改早期失败。其他照片、真实峰值、重复性和完整产品运行仍待验收。

## 明确流程与人工关口

固定只读路径`/storage/RC3P0001.JPG`，预期为此前S1字节一致测试副本。文件不存在、损坏、超限或介质异常就失败，不列目录/选其他文件，不创建、修复、覆盖、删除或重编码。卡上内容仍须人工确认，先前大小/ROM解码通过不是全文件SHA验收。

1. 获准本候选更新并完成独立回读/分区保持后，新启动检查factory布局、≥8MiB PSRAM、控制台独占、PMU/VCOM只读状态。开机不挂载、不开屏。
2. 明确`mount`一次；检查present/mounted/media_valid/exit_failed。失败消耗机会，只允许status/stop，不自动重试或格式化。
3. 明确`clear-white`一次；正常卡状态才初始化显示/静态清白，检查78帧软件覆盖和独立退出电源位。返回不等于全白通过。
4. **等待用户查看本次启动整屏白色。** 确认无异常后才发送`white-confirm`；不跨启动复用白参考。异常用`photo-reject`锁存并stop，不补刷。
5. 仅一次`photo-show`：真实文件JPEG完整源/EOF/关闭后，EXIF方向、居中Cover、原PhotoFrame灰阶/CDR与Floyd-Steinberg产生完整4bpp帧。诊断固定portrait与原上游调色板默认值，不读取/写入NVS调色板，不新增gamma调整；这套默认策略已有S1单张证据，其他照片/重复性仍需验收。再次检查有效介质后才一次GC16，返回后释放帧。
6. **等待用户照片/目视结果。** 核对方向、裁切/铺满、不拉伸、无旧几何连续块/异常黑带和灰阶细节，再明确`photo-confirm`或`photo-reject`。软件completed不自动计视觉通过；确认后也不能再显示第二张。
7. 明确`stop`一次：释放诊断帧、显示deinit、SD卸载，再HAL退出；保留主错误与退出错误。文件关闭失败锁存时拒绝正常卸载/HAL拆除，清理失败终止为FAILED，不重试不确定句柄。正常STOPPED/console卸载后由主机EOF关闭串口；只在完整卸载后人工插拔卡。

控制台必须显式选择`--profile photo`，精确白名单为status/mount/clear-white/white-confirm/photo-show/photo-confirm/photo-reject/stop；不接受路径参数、其他profile命令、前后空白、控制字节或超长行。控制台不自动发命令/复位/重连或重试部分写入；默认display白名单不变。

## 测量、恢复及未完成范围

分别记录处理阶段耗时、GC16返回/36相完整性、IOE退出位、SD/清理状态和人工画面。SDK局部heap低水位包含并发分配，单次RGB最大分配不等于整机峰值。

同步JPEG/SD与EPDiy内部等待没有安全硬取消契约，stop不能中断已执行调用。错误保留，不自动reset/清白/调波形或校准。文件关闭失败保留锁存，产品配置/并发/空间与OTA迁移按TODO继续；单张诊断不计P3 MVP/P5完成。
