# Read Pico 单张原片显示诊断（读取优化版S1通过，已停止）

2026-10-05最新硬件：9f6d6f6f…/404,336字节读取优化版已获准factory部署、全长回读且4096字节分区保持。用户明确“刷白后直接执行S1显示”，本轮清白78/78软件/退出成功后发送协议white-confirm并一次photo-show；没有白屏目视反馈，不记人工白参考通过。S1处理56,161ms（读/解码/关闭41,491、几何/灰阶/抖动/绘制14,656），相同原片前轮114,252ms，本次下降约50.8%；36/36相398.130ms，刷新和电源退出正常，帧已释放。用户本次实拍确认“画面正常没有问题”，430701ms photo-confirm为VERIFIED；433457–433460ms显示/SD/HAL清理均ESP_OK并STOPPED/operation=cleanup=0。控制台EOF/exit0、串口无占用、原片保留，不重复刷新。仅计该张S1；仅本次许可跳过白屏等待，后续流程不自动沿用；下述62cac21b…通过版为历史结果。

2026-10-05最新硬件状态：方向修复版403,184字节/62cac21b…已factory应用更新，全长独立回读一致、4096字节分区表保持。清白78/78人工通过；唯一S1处理114,252ms、GC16为36/36相合计398.114ms，刷新和电源退出正常。用户实拍确认“正向显示正常没有问题”，photo-confirm接受为VERIFIED；随后显示/SD/HAL清理均ESP_OK，最终STOPPED、卡卸载、串口关闭。单张S1显示验收通过；其他样本、长期稳定及产品NVS迁移未由本结果覆盖。私有证据在`.cache/p3-photo-orientation-hardware-GxPcPw/`。

## 当前读取优化固件（已部署，S1视觉通过并正常退出）

404,336字节，应用SHA256 `9f6d6f6fc329344647f601b976d1869d70bd758b141f4db4f580f389fa2540a5`；ELF SHA256 `73b5d85440e41f547f5309629baf8344128a7b020e8ed7b0ba2038ac371969bd`。固定副本`.cache/p3-buffered-photo-candidate-kKXZq6/`，sdkconfig SHA256 `1055abe1df7e16e9270d1d807529e76514a728dd6420f37e7a993925d31f48ca`与方向通过版一致。IDF v6.1/S3、DIO/16MB/80MHz/MMU64KB、checksum/hash有效，secure version0。

仅改变文件JPEG读取缓存/介质查询节奏并增加分阶段日志；方向、EXIF/Cover、灰阶、波形、VCOM及频率不改。该固件仍是原受控单张S1入口，不启动完整产品/NVS/Web/OTA。PNG保护同时进入产品编译，但本次S1诊断不执行PNG路径。普通/ASan各474、Python38、方向6和六配置编译通过；5原片处理及源校验保持，真机性能尚未验收。

本次已核实身份、安全、完整旧应用及分区，只写应用`[0x10000,0x72b70)`，擦除上界`0x73000`，独立全长回读与固定输入一致、4096字节分区保持。私有证据目录`.cache/p3-buffered-photo-hardware-PAms8H/`，旧62cac21b…应用完整保留。本次一次挂载/清白/同一S1已经执行，未file-test/换片/写原片，不写bootloader/分区/NVS/PHY/内部storage/PMU校准，不开放产品迁移。当前等待用户照片反馈，然后photo-confirm或photo-reject及stop/卸载/关闭串口；不重刷。本次启动没有接收到PHOTO_MEMORY局部低水位条目，仅记录可见的处理前后free/largest，不推断峰值。

以下保留方向修复轮次及历史边界；涉及PNG输入的未完成描述已由本候选软件保护更新，完整消费者/并发仍未完成。

本入口只为打通已获准S1原片的“只读SD → 真实JPEG/EXIF/Cover/灰阶/抖动 → GC16”单张验收，不启动完整PhotoFrame、网络、NVS配置或相册轮播。没有第二套产品相册/状态系统；诊断只实现命令门禁与不发布配置/快照的显示sink，产品继续使用原display_manager。

## 候选与当前设备分开

2026-10-05方向修复候选403,184字节，应用SHA256 `62cac21b8cc7502fd6c79611f6b57d3f1c174d2549b2be41ee539f7827c0a554`，固定IDF v6.1/S3、DIO/16MB/80MHz/MMU64KB、checksum/hash有效，secure version0。本机固定副本位于忽略目录`.cache/p3-photo-orientation-candidate-0VzAUF/photo-app.bin`；ELF SHA `9b2d13c6c998d36676160525395f6ab0438063907821058e05f36d6c60a4fb38`，sdkconfig SHA `1055abe1df7e16e9270d1d807529e76514a728dd6420f37e7a993925d31f48ca`。

方向修复版已部署、回读及单张S1实屏通过，设备已正常STOPPED。上一版e7d5bd8d…/403,168字节的倒置照片为历史失败记录；旧应用完整回读副本保留在本轮私有证据目录。主机测试、编译、烧写回读和实屏分别记录。

实际写入仅factory应用范围`[0x10000,0x726f0)`、覆盖擦除块`[0x10000,0x73000)`；设备身份、安全/布局及旧应用完整回读、新应用全长回读和4096字节分区表保持均已核验。bootloader、分区表、NVS、PHY、内部storage或PMU校准未写。

### 首次候选真机结果与修正依据

上一候选e7d5bd8d…部署前后应用全长回读一致，4096字节分区表保持；清白78/78并获“整屏白色无异常”。一次S1照片处理记录6192×4128、EXIF6、1/4解码1548×1032、方向后1032×1548、Cover到684×1216，偏移(63,0)，处理约115秒；GC16为36/36相、约398毫秒扫描合计、刷新返回成功。随后实拍显示完整画面上下颠倒，用户判定“方向错误，上下颠倒，其他暂无问题”，因此发送photo-reject，不标记视觉通过。

源码核对表明图像处理的portrait输出把逻辑`(x,y)`映射到native`(1215-y,x)`，厂家在机壳正向初始化使用`EPD_ROT_INVERTED_PORTRAIT`，即逻辑`(x,y)`映射到native`(y,683-x)`；两者正差native 180°，与实拍一致。修正将Read Pico板级默认方向设为180°；隔离诊断因绕过Paint，另在4bpp封装层应用相同180°变换。EXIF、Cover、灰阶、gamma、VCOM、频率、波形及厂家子模块不改。完整产品若NVS已有`disp_rot_deg=0`覆盖值，仍需迁移策略和产品真机验收，不因默认值修正自动宣告完成。

## 构建与软件证据

激活[固定SDK](DEVELOPMENT.md)后，只编译：

```sh
python3 build.py --board read_pico --step firmware \
  --read-pico-factory-diagnostic --read-pico-photo-validation
```

专用`build-read-pico-photo/`及`.cache/sdkconfig.read_pico-photo`。与display/storage/integration互斥，CLI拒绝flash/monitor/erase/fullclean和额外参数；默认Read Pico配置关闭。复用已验收精确684行外层覆盖、18MHz传输及CPU/APB事务PM锁、PERF/64KB/64B与80MHz Flash/PSRAM，不改厂家源码/波形/VCOM或频率。

普通/ASan各459项、Python38项通过；照片/完整集成/显示/SD/只读factory/原板六配置编译成功。新增四角合成用例把隔离诊断输出逐点对照厂家inverted portrait映射，并由源码保护检查板级默认、诊断桥和厂家初始化保持一致；既有会话、完整4bpp、真实JPEG和输出保护覆盖继续通过。5张获准原片普通和ASan诊断CLI都输出415,872字节/831,744像素，原片最终SHA一致。主机外部TJPGD/替身sink不代替S3 ROM、SD或实屏；SDK入口没有主机模拟硬件运行。本轮未跑Web/远端CI或其他板型完整矩阵。

## 明确流程与人工关口

固定只读路径`/storage/RC3P0001.JPG`，预期为此前S1字节一致测试副本。文件不存在、损坏、超限或介质异常就失败，不列目录/选其他文件，不创建、修复、覆盖、删除或重编码。卡上内容仍须人工确认，先前大小/ROM解码通过不是全文件SHA验收。

1. 获准本候选更新并完成独立回读/分区保持后，新启动检查factory布局、≥8MiB PSRAM、控制台独占、PMU/VCOM只读状态。开机不挂载、不开屏。
2. 明确`mount`一次；检查present/mounted/media_valid/exit_failed。失败消耗机会，只允许status/stop，不自动重试或格式化。
3. 明确`clear-white`一次；正常卡状态才初始化显示/静态清白，检查78帧软件覆盖和独立退出电源位。返回不等于全白通过。
4. **等待用户查看本次启动整屏白色。** 确认无异常后才发送`white-confirm`；不跨启动复用白参考。异常用`photo-reject`锁存并stop，不补刷。
5. 仅一次`photo-show`：真实文件JPEG完整源/EOF/关闭后，EXIF方向、居中Cover、原PhotoFrame灰阶/CDR与Floyd-Steinberg产生完整4bpp帧。诊断固定portrait与原上游调色板默认值，不读取/写入NVS调色板，不新增gamma调整；这套默认图像校准在Read Pico的视觉效果尚未验收。再次检查有效介质后才一次GC16，返回后释放帧。
6. **等待用户照片/目视结果。** 核对方向、裁切/铺满、不拉伸、无旧几何连续块/异常黑带和灰阶细节，再明确`photo-confirm`或`photo-reject`。软件completed不自动计视觉通过；确认后也不能再显示第二张。
7. 明确`stop`一次：释放诊断帧、显示deinit、SD卸载，再HAL退出；保留主错误与退出错误。文件关闭失败锁存时拒绝正常卸载/HAL拆除，清理失败终止为FAILED，不重试不确定句柄。正常STOPPED/console卸载后由主机EOF关闭串口；只在完整卸载后人工插拔卡。

控制台必须显式选择`--profile photo`，精确白名单为status/mount/clear-white/white-confirm/photo-show/photo-confirm/photo-reject/stop；不接受路径参数、其他profile命令、前后空白、控制字节或超长行。控制台不自动发命令/复位/重连或重试部分写入；默认display白名单不变。

## 测量、恢复及未完成范围

记录图像处理耗时、GC16软件返回/36相完整性及相位min/max/合计，独立退出IOE位、SD状态及STOPPED。图片帧完成之前没有显示调用；sink逐行验证16灰阶理论值和完整行列顺序，标准PhotoFrame偶数像素高nibble，再由原生驱动转换到厂家低nibble。

SDK局部内存低水位监测覆盖处理→刷新→释放，记录内部/PSRAM free、largest及local minimum和起始差值。它包含系统并发分配，且多heap低水位并不等于精确单任务峰值；本机最大单次RGB分配4,792,608字节不是实际总PSRAM峰值。ROM解码、卡读取和显示共存的真实余量/性能待本候选实测。

同步JPEG/SD与固定EPDiy内部等待没有硬取消保证；发送stop不能中断正在执行的调用。沿用已有单张GC16挂起风险，须在本候选硬件许可时明确接受；错误保留、不自动reset/清白/重试/调波形或校准。

Read Pico图像输出现只以O_EXCL新建，不覆盖已有路径，失败不删除路径并保留可能的部分输出；PNG解码也忽略release_source以保护持久原片。格式探测和PNG写入关闭错误已接锁存，但旧内存JPEG、PNG输入关闭/提交时序、其他业务消费者/临时文件生命周期和并发、原子发布/NVS/OTA迁移仍未完成。此诊断不走PNG文件输出或这些旧消费者；单张通过也不开放完整产品入口、不计P3 MVP/P5完成。
