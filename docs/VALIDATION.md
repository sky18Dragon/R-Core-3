# 新方案验证矩阵与记录

## 2026-10-08 · 提交前完整软件快照复验

按用户要求整理可验证增量提交。此前尚未提交的存储/图像、配置、网络和后台适配共用 CMake、测试替身及调用入口，作为一个完整软件快照保存，不将历史开发轮次反推成可独立构建的中间提交。本轮未修改固件源码。

当前源码重新构建后，普通 CTest **1142/1142、10.61秒**，ASan/UBSan **1142/1142、46.89秒**；Python 构建保护 **68/68**、控制台 **17/17**。固定 SDK v6.1 下重新执行原板及 Read Pico 集成、照片、SD、显示、factory 六配置构建，全部 exit 0；六份应用的长度与 SHA256 均与下方“P3主图/缩略图初始请求头检查”记录一致，集成仍为 **1,923,152字节 / `8e67153054ef4dd400a71e423949e629803c40119c8f304d8c26391dd75697a8`**。

复验日志位于本机忽略目录 `.cache/progress-baseline-*`。Git 待提交集合展开后为196个源码、测试和文档文件，常见私钥、GitHub Token、AWS Access Key 模式扫描未发现匹配；该扫描不代替所有敏感信息审查。构建生成物、私人原片及本机日志均不入库。

烧写/回读：未执行。真机验收：未执行。远端 CI：未执行。产品、OTA 与 PMU 睡眠运行门禁保持。随后文档整理以独立提交记录，不改变本软件快照的运行能力或硬件结论。

当前 PM 时钟锁候选 305,104 字节、SHA256 `6f2a4ea106f7aab2c1d8ff2fd2605f70b0aefafaa5c437b0d688106a12055255` 已仅 factory 部署并全长回读一致，分区表前后不变。候选在显示事务内同时固定 CPU 240 MHz/APB 80 MHz；静态清白获人工确认，官方局部 16 灰阶页 36 相合计 399.327 ms，实拍 16 阶中心亮度严格单调，确认后显示/HAL 正常释放并完整 `STOPPED`。

旧失败样本的 30.5–48.5 ms/相和 05–15 并白，与加锁后的 11.090–11.112 ms/相和 16 阶恢复构成同机、同页面、同波形/VCOM/PCLK 的单变量 A/B。因此“自动 DFS 下 EPDiy 直接 LL RMT 与 CPU feeder 缺少事务 PM 锁”已确认为本轮持续白偏的具体根因。P2 整体仍未完成：单张标准页不替代重复性、其他图案、照片解码链路或长期稳定验收。

下方按日期记录历史验证，不将旧产物测试或实屏结果转移给新产物。

更新时间：2026-10-04。本文件仅记录 PhotoFrame 移植。P0 完成；16 灰阶长期白偏根因已通过 PM 锁候选的真机时序与实拍共同关闭。历史四角、684 行几何、GC16 铺白和连续高压结果仍按日期保留，不反向改写。16 灰阶重复性、其他标准图案、照片链路、历史欠载/黑带机制及连续稳定性仍待验收；标准 SY 易失性启动允许，校准写/完整迁移不批准。

## 验证矩阵

### 2026-10-04 · C阶段连续高压实屏仍白偏，已拒绝并停止

用户明确允许上一节本地候选的factory app-only更新、回读/分区复核、新启动清白及人工全白后的单次连续事务，并知悉固定EPDiy同步等待没有安全硬取消。只读核验仍为同类ESP32-S3 v0.2、16MB/3.3V Flash，Secure Boot/Flash Encryption关闭；本地镜像image-info为S3/DIO/80MHz/16MB/MMU64KB且checksum有效。写前4096字节分区表与生成3072字节前缀及1024字节FF尾部一致。

仅写factory应用`[0x10000,0x5a510)`，扇区擦除止于`0x5b000`；304,400字节全长回读与本地逐字节一致，SHA256均为`ec917aafe57fea0bd601182560368021868f377fd9dfd13e9d5082d70a3d9dc1`。写前/写后4096字节分区表逐字节一致，SHA256均为`f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。未写bootloader、分区、NVS、PHY、storage或PMU校准；不使用完整flash_args。

新启动核对ELF前缀`cbc71841f`、ESP-IDF v6.1、PERF/65536/64、80MHz Flash/PSRAM、PSRAM自检及PMU 1.0.9/VCOM只读1580mV。显式`status`后唯一静态`clear-white`：12.901秒，78/78帧、每帧696次source回调，coverage/tail/padding/pulse/core及退出观察全部无错。用户确认“整屏白色无异常”，同启动`white-confirm`后只发送一次`gc16-gray16-labeled-continuous`。

连续事务一次prepare/ON=1温度采样/stop，主动31°C、PGOOD=1；两次HL绘制分别约1.713/1.724秒，外层4.174秒。`completed=1/frames=2/stage=0/frame_error=0/operation=0/cleanup=0/faulted=0`，内存从两输入分配后的PSRAM 5,846,932恢复到6,682,524；退出`input0=0x81/out0=0xe5`。相位记录registered/started/done/stored=72/72/72/72、errors/dropped=0，index 0–71连续；第36相前约59ms是两次同步HL调用间的CPU差分/缓冲准备间隔，不是下电或人工等待。

用户照片为私人附件，未复制入库；本机SHA256 `7c9a4a3b799ccaf0effc74d534e5150ffc5427cdea554d5f644184fadcc43690`。照片中00–15编号、条带边界和整屏几何完整，00–03/04有明显变化，但约05后快速趋白、后半大面积合并；曝光/反光不允许精确L*结论，但足以判定16阶未通过，连续HV没有可见改善。发送`pattern-reject`后FAULT_LATCHED/drawn=0x40/confirmed=0；随后`stop/status`，LCD、DISPLAY_DEINIT、HAL_DEINIT均ESP_OK，最终两次完整STOPPED/operation=0。控制台Ctrl-C因管道退出1，端口无占用；不是设备错误。

独立解析私有日志确认命令恰为`status → clear-white → white-confirm → gc16-gray16-labeled-continuous → pattern-reject → stop → status`，连续入口仅一次、相位索引恰72条，无其他GC16、补刷或重试。私有日志/回读为`.cache/continuous-gray-*`，均忽略。静态清白、B阶段GC16白后下电和C阶段连续HV均未解决白偏；下一变量需重新论证和授权，不自动调整VCOM、gamma、频率或waveform。P2仍未完成。

用户随后明确要求“先记录当前验证结果，不再继续执行验证”。本轮至此关闭：不再发送串口命令、复位、烧写、清白、刷新或测试其他变量。设备最后确认状态为`STOPPED/operation=0/pending=invalid/drawn=0x40/visually_confirmed=0`，控制台已关闭且串口无占用；物理屏保留本次未通过的编号灰阶图。后续若恢复，必须重新取得用户指令并建立新的当次白参考，不能沿用本轮会话状态。

### 2026-10-04 · C阶段连续高压对照：离线实现、全量测试与固定IDF编译

在B阶段无改善后，仅进行源码、主机测试和本地固件编译；没有连接串口、复位、烧写或刷新设备。新增精确白名单指令`gc16-gray16-labeled-continuous`，必须在本启动静态清白经人工确认且尚未绘制任何图案时执行。它预先生成并校验白帧与同一编号灰阶帧，在同一个显示锁、一次SY prepare/PGOOD、一次ON=1主动温度采样和一次stop内顺序执行两次普通GC16；第一帧失败不进入第二帧，第二帧结束后立即关电，过程中不等待人工输入。最终图仍进入原有WAIT_PATTERN_CONFIRM，只有事务结束后才接受人工确认或拒绝。

便携结果增加`frames_completed`并新增双帧API；输入长度、空指针、转换/重叠均在上电前失败。故障注入覆盖第二输入非法、第一/第二绘制失败、退出失败、缺少第二帧不启动高压轨、单次温度采样和同一锁/电源区间。扫描相位容量从48扩到72，新增精确72相成功及74相溢出拒绝测试；固件要求`completed=1`、`frames_completed=2`、72/72相且无错误，否则会话锁存失败。固定Read Pico gitlink仍为`28cde682…`且工作树干净，overlay只在主工程构建目录生成，未改厂家波形/校准源码。

| 检查 | 结果 |
| --- | --- |
| 普通完整CTest | 309/309，通过 |
| ASan/UBSan完整CTest | 309/309，通过 |
| 控制台与构建门禁 | 8/8 + 12/12，通过，无硬件访问 |
| 固定ESP-IDF | v6.1 / `fff9895c…`，显示配置编译成功 |
| app | 304,400字节；SHA256 `ec917aafe57fea0bd601182560368021868f377fd9dfd13e9d5082d70a3d9dc1` |
| ELF / sdkconfig | `cbc71841fb836f130c6515345feb6995e79a885eec28b4903016949152f53acb` / `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5` |

本候选逻辑上限定两帧且所有**已经返回**的失败路径都会执行stop；但固定EPDiy同步绘制在`render_lcd.c`/`render.c`内部仍有`portMAX_DELAY`等待，没有安全的异步取消接口。因而不能把它称为可在驱动卡死时硬中止的超时实现，官方8秒idle规则也不能冒充绘制超时。该限制与既有单帧GC16同源，但连续轮包含两次绘制，必须在新的具体硬件许可中明示。当前设备仍运行已回读的300,096字节`81ac6974…`，最后完整状态STOPPED；新产物没有回读或实屏结论。

### 2026-10-04 · B阶段GC16全白预处理无改善，已拒绝并停止

用户明确“允许”既定B阶段。沿用已部署、全长回读一致的300,096字节`81ac6974…`，未刷写、改代码或调参；精确684行、18MHz、factory VCOM 1580mV、DEFAULT 36相、leading-skip关闭、80MHz存储、gamma及每次刷新后下电流程全部不变。启动核对同一ELF前缀`b8a1dea58`、ESP-IDF v6.1、PSRAM自检、PMU 1.0.9及只读VCOM门禁通过。

显式`status`后唯一静态`clear-white`：初始化79ms、清白12.889秒，78/78帧、每帧696次source回调，coverage/tail/padding/pulse/core和退出观察全部无错。用户确认“整屏白色无异常”后`white-confirm`。随后唯一`gc16-white`：completed=1，stage/frame/operation/cleanup/faulted均0，2.396秒；36/36相完整、31°C/ON=1/PGOOD=1，内存恢复。用户再次确认“整屏白色无异常”，因此`pattern-confirm`登记white位0x1。

确认后只运行一张`gc16-gray16-labeled`：completed=1，所有错误0，2.407秒；36/36相完整、31°C/ON=1/PGOOD=1，内存恢复。私人照片未复制入库，本机附件SHA为`d5d2ab07…`。照片中00–15标签、条带边界和整屏范围完整，但04后快速接近白，约05–15仍难逐级区分；与铺白前照片的光线/曝光并非严格校准，不能量化L*差值，但肉眼没有可见改善，故不通过。

发送`pattern-reject`后状态`FAULT_LATCHED/operation=0/pending=gray16-labeled/drawn=0x41/visually_confirmed=0x1`；随后`stop`和`status`，LCD deinitialized、DISPLAY_DEINIT/HAL_DEINIT均ESP_OK，最终两次完整`STOPPED/operation=0/pending=invalid/drawn=0x41/visually_confirmed=0x1`。Ctrl-C结束，端口无占用。没有补清白、重试、额外图、复位、烧写或校准写；私有日志`.cache/exact684-gc16-white-compare-console-private.log`被忽略。

B阶段只证明“静态清白→GC16全白→下电→人工确认→再次上电绘图”未改善浅阶；它没有复现官方铺白后保持高压并立即绘图的连续事务。下一步先离线实现C阶段有界连续操作、超时和任一子步骤失败清理，经主机故障注入及固定IDF编译后再请求新的app-only/单次硬件许可。仍不改VCOM、gamma、频率、waveform或校准。

### 2026-10-04 · 精确684行编号灰阶仍白偏，已拒绝并停止

沿用已部署并回读一致的300,096字节`81ac6974…`，没有刷写、代码或参数改动。保持精确684行、18MHz、factory VCOM 1580mV、DEFAULT 36相、leading-skip关闭、80MHz存储、gamma及每次操作后下电流程不变。新启动核对ELF前缀`b8a1dea58`、ESP-IDF v6.1、PSRAM自检、PMU 1.0.9及只读VCOM门禁通过。

显式`status`后唯一一次`clear-white`：显示初始化79ms，清白12.875秒；78/78帧、每帧696次source回调，coverage、tail、padding、pulse、core和退出观察全部无错。用户明确回复“整屏白色无异常”，同启动`white-confirm`接受。随后只运行一张`gc16-gray16-labeled`：主动温度31°C、ON=1/PGOOD=1；completed=1，stage/frame/operation/cleanup/faulted均0，外层2.412秒；36/36相完整，errors/dropped均0，内存释放回绘制前水平。

用户提供的私人照片未复制入库，附件SHA仅在本机核对为`52efaf6d…`。照片中00–15标签顺序、条带边界和整屏范围完整，没有裁切、错位或此前左侧宽连续区；但仅00–03差异明显，04后迅速接近白，约05–15难以逐级区分。照片受曝光、反光与透视影响，这是一项定性失败结论，不冒称L*或精确可见级数测量。软件36相不同也不能替代光学验收。

按视觉失败发送`pattern-reject`，会话进入`FAULT_LATCHED/operation=0/pending=gray16-labeled/drawn=0x40/visually_confirmed=0`；随后`status`及`stop`，LCD deinitialized、DISPLAY_DEINIT/HAL_DEINIT均ESP_OK，最终`STOPPED/operation=0/pending=invalid/drawn=0x40/visually_confirmed=0`。Ctrl-C结束控制台，端口无占用；没有补清白、重试、第二张图、复位、烧写或校准写。私有日志`.cache/exact684-gray16-labeled-console-private.log`被忽略。

本轮证明精确684行修复的空间几何不能同时解释灰阶白偏。下一步按方案B，以同固件新启动建立人工白参考，只增加一张`gc16-white`作为GC16全白预处理，人工确认后再显示同一编号图；现有逐张门禁已经支持该序列，不需新增固件。该轮只隔离起始光学状态，不等同官方连续高压流程；保持VCOM、gamma、频率、waveform及校准不变，需新的硬件许可。

### 2026-10-04 · 精确684行候选部署与同图几何通过

用户回复“继续”，允许按上一轮明确范围制作精确684行候选，离线门禁通过后仅更新factory app、完整回读与分区复核，再执行一次清白和一张相同`gc16-gray16-page`。实现位于主工程构建覆盖，不修改固定子模块：生成器要求固定`render.c` SHA `f288db9c…`及`lcd_driver.c` SHA `7287dffa…`，哈希或唯一锚点漂移即失败；两个bounce buffer由4行改为6行，renderer与LCD对齐单位由8改为12，684=57×12。18MHz、factory VCOM、DEFAULT 36相、leading-skip关闭、80MHz存储、测试图和电源流程不变。

构建保护12/12、普通与ASan/UBSan串行全量各300/300通过。固定ESP-IDF v6.1 `fff9895c…`编译成功，实际编译对象来自构建目录`generated/render.c`与`generated/lcd_driver.c`，未编译固定子模块的同名源；运行日志显示`using resolution 152x684`。app 300,096字节，SHA256 `81ac6974935202c3acbf3b40dc8f019893b19ef9320aa4f8717ab727eff1e63d`；ELF SHA256 `b8a1dea58c1676294754d30d57f732c05513075ef950447a03adcd013cf68751`；sdkconfig仍为`e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`。image-info为ESP32-S3/DIO/80MHz/16MB/MMU64KB，checksum与validation hash有效。

写前重新核对同一设备、ESP32-S3 v0.2/8MB PSRAM/16MB Flash/3.3V、安全启动与Flash加密关闭。实际分区仍为NVS `0x9000/0x5000`、PHY `0xe000/0x1000`、factory `0x10000/0x400000`、storage `0x410000/0x500000`；候选表前3072字节一致，尾1024字节全`0xFF`。仅写factory `[0x10000,0x59440)`，工具擦除范围止于`0x5a000`；300,096字节独立全长回读逐字节/SHA一致。写前/写后4096字节分区表逐字节一致，SHA均为`f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。未写bootloader、分区、NVS、PHY、storage或PMU校准。

新启动ELF前缀`b8a1dea58`、IDF v6.1、PERF/65536/64，PSRAM自检通过，PMU 1.0.9/VCOM只读1580mV。`status`为WAIT_CLEAR_COMMAND后唯一一次清白：显示初始化80ms，清白12.873秒；78/78帧、每帧696次软件回调、tail/padding/脉冲/覆盖/退出及所有观察错误通过。用户明确确认“整屏白色无异常”，同启动`white-confirm`接受。

随后仅一张`gc16-gray16-page`：`native_equal=1/from_white=1`，GC16 completed=1，stage/frame/operation/cleanup/faulted均0，事务2.035秒；36/36相完整。边缘观察每相696次软件回调、12行齐全，missing/unequal/indistinguishable/dropped/orphan/late/unfinished/wrong_core均0；`core1_start_prefill=156`为13帧×12次允许的同步预填。用户提供的私人照片未复制入库；照片中40像素边距恢复，灰条、黑块、棋盘和细线不再出现共同宽连续区，棋盘从首格起规则交替。按该视觉证据发送`pattern-confirm`，drawn/visually_confirmed均为`0x100`。

随后`stop`：LCD deinitialized、DISPLAY_DEINIT和HAL_DEINIT均ESP_OK，最终`STOPPED/operation=0/pending=invalid/drawn=0x100/visually_confirmed=0x100`；控制台关闭且端口无占用。该结果支持684→688填充参与此前相位间gate错位，但只有一次同图A/B，不能替代示波器级因果证明、长期复现或16个灰阶L*测量。下一步保持精确684行、VCOM/gamma/频率/waveform不变，单独量化灰阶可分辨性。

### 2026-10-04 · 18MHz单变量部署与同图A/B失败

用户明确允许本轮仅更新factory app、完整回读并核对分区表，然后在新启动完成一次清白和一张相同`gc16-gray16-page`。写前确认Secure Boot与Flash Encryption关闭、16MB Flash、分区为nvs `0x9000/20KiB`、phy `0xe000/4KiB`、factory `0x10000/4MiB`、storage `0x410000/5MiB`；构建分区表前3072字节与设备一致，设备剩余1024字节全为`0xFF`。

仅写factory范围`[0x10000,0x59430)`，扇区擦除`[0x10000,0x5a000)`。300,080字节全长独立回读与候选逐字节相等，SHA256均为`66425c9c6340ffad20963d7c0771725c11f8662a8835d504b1a6015a0d2a6167`；写前/写后4096字节分区表逐字节相等，SHA256均为`f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。未写bootloader、分区表、NVS、PHY、storage或PMU校准。

新启动为ESP-IDF v6.1、ELF前缀`6c09113cb`，PSRAM自检通过，PMU 1.0.9/VCOM只读1580mV。`status`确认`WAIT_CLEAR_COMMAND`后唯一一次`clear-white`：LCD PCLK 18MHz，solver输出行周期16.0µs/288 cycles、CKV高13.4µs、89.15Hz，显示初始化80ms；清白14.003秒、78/78帧、每帧696次source回调，coverage/tail/padding/edge/core及退出检查全部通过。用户明确确认“整屏均匀白色无异常”，同启动`white-confirm`被接受。

随后只发送一次`gc16-gray16-page`：主动温度32°C、ON=1/PGOOD=1、`native_equal=1/from_white=1`；GC16 completed=1，stage/frame/operation/cleanup错误及faulted均0，外层2.117秒，HL diff/draw/buffer/total为27/1286/13/1326ms。相位registered/started/done/stored均36，errors/dropped均0；每相696次source回调，12条目标行齐全，missing_rows/unequal_pairs/indistinguishable/dropped/orphan/late/unfinished/wrong_core均为0，六对轨迹组内相等且五个相邻组可区分。

用户随后提供的私人照片未复制入库。照片仍显示预期40像素左白边缺失：灰条、实心黑块与细线从可视区左缘开始，棋盘左侧约三格形成连续黑/白带，之后才恢复规则40像素节距；与12MHz失败形态基本一致。因此18MHz单变量没有修复黑白几何，本轮不进入灰阶级数/L*、GC16铺白、VCOM或waveform实验。发送`pattern-reject`后状态进入FAULT_LATCHED，再发送`stop`；LCD deinit、DISPLAY_DEINIT、HAL_DEINIT均ESP_OK，最终`STOPPED/operation=0/pending=invalid/drawn=0x100/visually_confirmed=0`，控制台正常中断。下一步转向source回调之后的DMA首段及LE/STV/CKV物理边沿取证。

### 18MHz实拍后的684/688行只读假设核对

停止设备后只读核对固定`epdiy`源码：面板定义高度为684，但renderer与LCD驱动都按8行向上对齐为688；两个bounce buffer各4行。每相位的逻辑684–687行由renderer写成hold，`epd_lcd_start_frame()`仍按688 active lines启动扫描。源码同时明确记载：曾经截短帧时，帧首STV脉冲不能可靠把gate拉回第0行；该注释只证明作者遇到过复位依赖，不证明本机当前就是同一故障。

在忽略目录编译实际默认36相GC16表，做了一个仅CPU的地址漂移敏感性模型：假设每相位末尾多出的4行没有被STV完全复位，使物理行在第`f`相收到逻辑`u+4f`附近的动作。白参考15→目标页的模拟会把40像素左白边驱成内容色，并在36相内把混合范围扩展到约140行，与照片约112–120逻辑像素的量级相近；灰条、黑块与棋盘会同时受影响。模型输出也会随漂移方向出现模糊或回卷，不能逐像素复现照片，更没有模拟gate模拟电压、STV采样窗口或实际DMA。因此它只把“684/688 + 相位间gate复位”提升为优先假设，不是根因结论或修复证明。

离线模型、文本与预览保存在被忽略的`.cache/gate_drift_model.*`，未包含私人照片、未改厂家源码。下一步有两种可证伪路径：一是跨GC16相位边界同时测GPIO47/STV、GPIO48/CKV、GPIO3/LE、GPIO46/XSTL-DE及GPIO21/PCLK，确认每相位gate复位与有效行数；二是先离线制作一个不改VCOM/gamma/waveform、只把LCD传输周期改成精确684行的单变量候选，再经新的明确许可做同一白参考/单图A/B。后一候选必须让renderer与LCD行数同步，并处理双bounce-buffer边界；不能只删4行或只改一侧。本节没有连接设备、烧写、复位或刷新。

### 2026-10-04 · 新指导说明核对与18MHz单变量候选

用户提供第二份外部指导说明并要求继续解决16灰阶问题；全文只作为参考。其“固定相机/L*、一次只改一个变量、PCLK改变必须用solver重算”的方法可用，但“立即覆盖VCOM、切换/修改waveform、把约11级视为成功”与当前黑白空间故障及项目校准边界不相容。本轮不接受这些高风险或会混淆因果的步骤，不复制附件入库。

固定上游源码确认产品初始化使用18MHz并开启leading-skip，板级SY示例固定1290mV；当前设备PMU实测factory VCOM为1580mV。为只比较扫描工作点，本地只把`driver_read_pico.c`中初始化及apply的PCLK从12MHz改为18MHz，仍由固定`read_pico_epd_scan()`同步计算blanking/CKV。leading-skip保持关闭，factory VCOM 1580mV、DEFAULT 36相、FULL scan、prefill 32、80MHz Flash/PSRAM、静态清白和同一官方页面均不变；没有runtime VCOM override、波形选择或phase mutation。

普通及ASan/UBSan串行全量各300/300通过；`ReadPicoNative`普通/ASan各26/26再次通过，Python构建保护与控制台合计18/18。首次固件命令遗漏必需的factory诊断显式标志，被`build.py`在编译前安全拒绝；补齐`--read-pico-factory-diagnostic --read-pico-display-validation`后，固定ESP-IDF v6.1编译成功。镜像300,080字节，SHA256 `66425c9c6340ffad20963d7c0771725c11f8662a8835d504b1a6015a0d2a6167`；ELF SHA256 `6c09113cb40f7d6780f9961daed67784310dc3ebd1472c53b4a38ae5a7b2c678`；sdkconfig仍`e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`。image-info为ESP32-S3、DIO/80MHz/16MB、MMU 64KB、secure version 0，checksum和validation hash有效。本段记录当时的离线状态；随后部署结果见上一节。

该候选同时包含先前离线完成的696行/CPU1启动预填分析器修正。后续已按限定范围完成硬件A/B且18MHz仍失败，因此不进入灰阶L*或waveform标定，改做source-edge之后的DMA首段及LE/STV/CKV物理证据。

### 2026-10-04 · 驱动差异报告后的官方页面重测

用户提供外部驱动差异报告并要求重新发起16灰阶验证；报告只作为分析依据，其中建议先判定黑白几何、再讨论GC16光学等级的顺序与现有证据一致。本轮沿用已部署的299,952字节/`daf0b7bf…`诊断应用，不刷写、不改gamma、VCOM、PCLK、leading-skip或厂家waveform。显式USB复位进入新会话，启动为ESP-IDF v6.1、ELF前缀`55d5514b4`、80MHz Flash、12MHz LCD、FULL scan、prefill 32、leading skip关闭，PMU 1.0.9/VCOM只读1580mV。

先发送`status`确认`WAIT_CLEAR_COMMAND`，再唯一一次`clear-white`：显示初始化80ms，静态清白13.993秒，78/78帧、每帧696次source回调，tail/padding/覆盖与退出检查通过；用户明确确认本启动整屏白色。`white-confirm`后仅发送一张`gc16-gray16-page`：主动温度32°C、ON=1/PGOOD=1，厂家原生绘图与目标全帧相等`native_equal=1`，36/36相完成，每相696行，绘制事务2.064秒，12条目标行齐全、六组成对轨迹相等、五个边界轨迹互异，missing/unequal/indistinguishable/dropped/orphan/late/unfinished均为0。

已部署分析器仍把部分CPU1供数归为`wrong_core=280`，因此`GC16_EDGE complete=0`并按安全策略进入`FAULT_LATCHED`；这是分析器判定失败，不能把画面标记为通过。用户随后提供的私人照片未复制入库。照片再次显示预期40像素左白边缺失：灰条、实心黑块和细线均从有效显示区左缘开始，棋盘约前三格宽形成连续区域，之后恢复规则节距；这与前两轮空间异常一致。本轮明确不通过，且纯黑白元素共同失败，不能归因于灰阶waveform或浅阶反射率。

观察后只发送`stop`；LCD deinit、DISPLAY_DEINIT、HAL_DEINIT均ESP_OK，最终`STOPPED/operation=0/pending=invalid`，控制台由人工中断正常关闭。没有pattern-confirm/reject、第二张图、自动重试、GC16铺白、升频、波形或校准写入。下一步应使用纯黑白source-edge图和回调后DMA首段/LE/STV/CKV证据定位固定偏移；没有这类证据前不进入B/C或36/48相光学A/B。

### 2026-10-03 边缘探针候选部署与白参考门

用户确认的范围为仅factory应用更新、独立回读、分区表前后验证和一次静态清白；不得写bootloader/NVS/校准，不得自动发送`white-confirm`或GC16图案。旧会话先以`pattern-reject`进入FAULT，随后打开串口的新启动回到`WAIT_CLEAR_COMMAND`，不沿用旧白参考。

镜像本地校验为ESP32-S3/DIO/80MHz Flash/16MB/MMU64KB、checksum和validation hash有效。仅写`[0x10000,0x593b0)`，擦除`[0x10000,0x5a000)`；299,952字节全长回读与候选逐字节一致，SHA256均为`daf0b7bffc8f9f723ebe9e861bb0222b6248c995a20d37e14ef55a91d1692eef`。烧写前后4096字节分区表逐字节相等，SHA256均为`f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`；分区仍为nvs 0x9000/20KiB、phy 0xe000/4KiB、factory 0x10000/4MiB、storage 0x410000/5MiB。Secure Boot和Flash Encryption保持关闭，未写其他区域。

新启动ESP-IDF v6.1、ELF SHA前缀`55d5514b4`、PSRAM自检通过、PMU 1.0.9、VCOM只读1580mV。唯一一次`clear-white`初始化79ms、清白13.918秒；78/78帧均为696次source回调，tail/padding采样、固定脉冲序列、注册/完成与所有错误计数均通过，`EXIT_OBSERVATION`为ESP_OK/input0=0x81/out0=0xe5。用户回复“整屏白色没有问题”，同启动`white-confirm`被接受，进入REFERENCE_KNOWN。

随后只发送一次`gc16-gray16-page`：主动温度32°C/ON=1/PGOOD=1、`native_equal=1/from_white=1`；GC16 completed=1、stage/frame/operation/cleanup错误均0、faulted=0，外层2.062秒，HL full diff/draw/buffer/total为26/1231/13/1270ms。退出ESP_OK/input0=0x81/out0=0xe5，内存恢复到绘制前。相位registered/started/done/stored均36，errors/dropped均0。边缘记录每相696次，12行齐全，missing_rows/unequal_pairs/indistinguishable/dropped/orphan/late/unfinished均0；六对轨迹依次为944535c6、869817be、216fd492、aef07d8a、a845cec7、71695b22，组内相同且五个相邻组均不同。

已部署分析器仍输出`complete=0/wrong_core=256`并使会话进入FAULT_LATCHED/operation=264。只读源码核对表明GC16两条厂家renderer任务分别固定CPU0/CPU1，命中trigger的任务在`epd_lcd_start_frame`启动DMA前同步填两个4行bounce buffer；256恰为32帧×8次，强烈一致于CPU1触发帧的启动预填。旧二进制只聚合core计数，不能回溯每次具体行号，因此该解释记录为源码与计数的一致性推断；目标行556–683及其指纹已完整记录。离线修正要求696次，并只允许CPU1的row0–7归入`core1_start_prefill`，后续CPU1回调仍失败。新增旧688计数与row100跨核故障注入；定向19/19、串行全量普通300/300（6.87秒）及ASan/UBSan 300/300（33.97秒）通过。固定IDF v6.1增量编译通过，app 300,080字节/SHA256 5069a6d0…、ELF db964513…，image checksum/hash有效；未烧写。

用户随后提供同尺寸新实拍。以u≈120之后稳定棋盘格定标，每格约40逻辑像素；照片中灰条、实心黑块和细线均贴到物理左边缘，缺少设计的40像素白边，棋盘交替行约前120逻辑像素形成连续色块，之后40像素节距恢复；右侧空框、垂直分区和后续格距基本正确。该表现与上一张照片同类，不是源图预期、整幅缩放或单纯灰阶波形现象。由于本轮关键边界在source回调处均可区分，当前根因边界收窄到该回调之后、可见像素之前；照片不能继续区分DMA、PCLK、CKV/STV或物理gate。没有发送`pattern-confirm`、GC16铺白或第二张图；显式`stop`后LCD deinitialized、DISPLAY_DEINIT/HAL_DEINIT均ESP_OK，状态STOPPED/operation=0，屏上保留测试图。

### 2026-10-03 局部图边缘供数探针离线候选

为区分framebuffer之后的renderer/LCD供数与物理gate落点，显示验证profile新增仅用于`gc16-gray16-page`的组合探针。它保留原36相注册/启动/完成时间戳，同时选取原生landscape行y={683,644,643,607,606,604,603,570,569,564,563,556}，对应逻辑portrait u={0,39,40,76,77,79,80,113,114,119,120,127}；仅在这些行的source回调完成后对304字节二位脉冲数据计算指纹。六对行在测试页上应分别具有完全相同的内容，五个相邻组边界应在36相中至少有一相可区分。每相仍要求688次source调用、12行样本齐全，缺失、溢出、晚到、错误core、成对不等或边界全程不可区分均失败锁存。

新增两个主机用例：正常36相/688行完整通过；把u=80–113组复制为前一组时检出不可区分边界，把同组第二条行改写时检出成对不等。扫描探针相关18/18普通与ASan/UBSan通过；串行全量普通299/299（7.26秒）及ASan/UBSan 299/299（33.06秒）通过。固定ESP-IDF v6.1 `fff9895c…`显示配置增量编译通过，app 299,952字节，SHA256 `daf0b7bffc8f9f723ebe9e861bb0222b6248c995a20d37e14ef55a91d1692eef`；IRAM目标对象的`traced_line`仅引用DRAM状态及IRAM/FreeRTOS临界区入口，未引入rodata跳表。`git diff --check`通过。

该证据设计只观察软件回调供数及顺序：它既不证明这304字节已经被DMA发送，也不测量LCD时钟、CKV/STV、高压或实际gate落点。候选未连接设备、未复位、未烧写、未清白、未GC16；已部署3a39dff3…、实拍失败和最后已知WAIT_PATTERN_CONFIRM状态均不变。若后续获准真机轮次，应先处理旧待确认会话并建立新启动人工白参考，只运行一次同局部图；`GC16_EDGE complete=1`且六对轨迹吻合/五边界可区分时，问题才进一步收窄到回调之后，仍不自动进入GC16铺白。

### 2026-10-03 官方局部图案与GC16相位观测开发

最新进展：用户确认本启动“整屏白色，无明显异常”，white-confirm接受后仅一次gc16-gray16-page。completed1/stage0/frame_error0/operation0/cleanup0/faulted0，2.246秒，主动32°C/ON1/PGOOD1，native_equal1/from_white1，退出ESP_OK/input81/oute5，内存恢复一致。36相registered/started/done/stored均36，errors/dropped0；准备共17,990µs，启动至完成共1,394,299µs，相间空档共877µs，合计1,413,166µs，与HL draw1416ms接近。单相启动至完成约36.903–39.487ms，额外耗时主要位于软件启动至完成区间，不是相间空档；仍不能仅凭此证明电气扫描异常或白偏因果。用户随后提供实拍并明确本轮不通过，没有自动确认、重刷或第二张。

实拍与预期布局复核：固定官方页面的逻辑竖屏坐标为684×1216。灰条应覆盖u=40–631、v=176–415，共16条、每条37像素；u=0–39与u=632–683为白。黑块为u=40–331、v=472–671；空框为u=352–643。棋盘覆盖u=40–679、v=712–871，16×4格、每格40像素；例如首行应为“40像素白边→40像素黑→40像素白→40像素黑”，次行应为“80像素白→40像素黑→40像素白”。细线为u=40–643、v=902+6i。旋转后的原生坐标是(x,y)=(v,683-u)。

照片透视估算以未受影响的右侧空框和后续棋盘格为尺度：正常40逻辑像素约为照片46–48像素；左侧连续区域约132–140照片像素，即约112–120逻辑像素。与预期逐段比较，约前80逻辑像素取成了后续区域的同色，直到u≈120才恢复正常40像素节距；灰条暗平台、实心黑块越过40像素白边、交替棋盘行的宽同色块和细线越过白边互相印证。透视/JPEG不支持把“80像素”当电气定值，但足以排除预期图案本来就有该连续区。右侧空框位置、后续棋盘节距、垂直分区和细线间距基本符合预期，不像整幅统一缩放或旋转错误。

软件侧继续核对：实际厂家 `epd_fill_rect` 结果与独立便携参考的415,872字节完整framebuffer相等；新增主机断言明确检查40像素左边距、37像素灰条边界、两行棋盘前160像素、黑块和细线边界，普通与ASan/UBSan下目标8/8通过。串行全量回归分别297/297通过（7.28秒、40.98秒）。一次同时并行启动两套会共享 `pf_display_checked`/`pf_storage` 的CTest产生DisplayFlow临时文件互扰，串行复跑通过，不计代码失败。仅改主机断言与文档，未重编译/烧写固件，部署产物不变。

高层from-white路径会强制全684行、608列为dirty；renderer提交688行，其中684行为面板数据、4行为显式hold填充，源码没有预期的80行裁切或复制。36相记录只证明每相启动/完成，不证明逐行数据与物理gate对齐。因此当前证据把问题定位到已验证framebuffer之后、光学灰阶判定之前的行映射/扫描边界，尚不能在照片基础上断言具体LCD/DMA/CKV/STV根因。暂不进入官方GC16铺白对照，不改gamma、VCOM、频率或波形；先用边缘定位证据隔离该约80像素区域。设备状态未通过串口重新读取，最后已知仍为WAIT_PATTERN_CONFIRM；本轮没有发送pattern-confirm、pattern-reject或stop。

四配置（显示、factory只读、双OTA只读、原板）编译均通过，日志为 .cache/gray-page-{display-final,factory-build,readonly-build,original-build}.log。获准 app-only 范围内核验同一设备、安全状态及分区兼容后，仅写 [0x10000,0x56d80)，扇区擦除 [0x10000,0x57000)。290,176字节完整回读逐字节相等；前后4096字节分区表相等，SHA256 f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20。不写保护区及校准。

新启动 ELF 5b6a60f87…/PMU1.0.9/VCOM有效1580mV，sticky faults=0x1保留。status后仅一次clear-white，初始化80ms、清白13.979秒；78帧/每帧696次/覆盖完整，补齐非零、边缘、脉冲、丢帧等计数全0。EXIT_OBSERVATION ESP_OK/input0=0x81/out0=0xe5；内存前后internal_free240487/largest196608/PSRAM6682524一致。当前WAIT_WHITE_CONFIRM/operation0/drawn0/confirmed0，等待人工白屏观察，未发white-confirm或GC16。串口保持打开，不跨启动复用旧确认。私有证据 .cache/gray-page-console-private.log、gray-page-readback.bin、gray-page-partition-{before,after}.bin。

新增gray16-page/ID8/位0x100，实际厂家epd_fill_rect按官方竖向几何绘制16×37×240灰条，保留黑块/空框/棋盘/细线，省略字体/页眉/菜单。独立便携参考映射到1216×684并与原生目标全长比较；临时旋转恢复，首次人工白参考门禁保留。原有八图ID不变。厂家源码/波形/频率/标准启动配置未改。

新增互斥的GC16相位观察，帧注册/启动入口/完成回调记时，48条上限、期望36相，行数据回调原样转发。输出准备、启动至完成与相间间隔；缺失/重复/顺序错误/溢出禁止验证通过。打印位于退出观察之后，无ISR日志和动态分配。新增7项主机测试：实际厂家图案/非法参数2、原生后端成功及错帧2、相位36帧/缺失重复/溢出3；扩展现有白参考和白名单测试。

296/296完整CTest通过（7.06秒），159/159 ASan/UBSan子集通过（11.32秒），10构建保护+8控制台通过；日志 `.cache/gray-page-host-*-final.log`、`gray-page-asan-*.log`、`gray-page-python-tests.log`。显示编译通过，本地app290,176字节，SHA256 `3a39dff3299e81030cb9582cf5675cc3c609159dd029ccf8fe49f3cb2e57ee30`，ELF `5b6a60f870945f3c8c9a2a4d174f2630008692a57a6ccee171719aea71f845ac`，sdkconfig仍e16d333e…；反汇编确认GC16供数路径调用IRAM的启动wrapper，timer也位于IRAM。软件计时不证明脉冲实际持续时间，观测开销待真机评估。其余配置及上板记录后续单列。

### 2026-10-03 最新部署与方案复核

用户重新连接并允许更新，身份与此前Pico一致（标识仅私有日志），S3 v0.2/8MB PSRAM/16MB Flash 5e4018，安全启动/加密关闭。旧应用头ELF与0f69c987…匹配，新镜像S3/revision/DIO80MHz/16MB/MMU64KB/checksum/hash核验通过。仅写 `[0x10000,0x55810)`，擦除 `[0x10000,0x56000)`；284,688字节全长回读逐字节相等，SHA eea2743296c97656da0d63ee3196c4e96cfeb63980dcba41a0cfec1eb8419248。前后4096字节分区表相等，SHA f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20。保护区/校准未写，无完整旧应用备份。

新启动ELF21cfb088d…/IDFv6.1/PSRAM自检/PERF65536/64通过，PMU1.0.9/VCOM有效1580mV，sticky faults=0x1保留。唯一clear-white初始化80ms、总14.017秒；78帧、每帧696次/seenff、边缘55/AA/00、补齐全0、coverage完整，各异常计数0。退出input0=0x81/out0=0xe5/ESP_OK，非显示位保留；不能套用此前out0=0x81整字节值判断故障。用户确认全白，同启动white-confirm后唯一gc16-gray16-vendor：completed1/stage0/frame_error0/operation0/cleanup0/faulted0，2346ms，主动32°C/ON1/PGOOD1，native_equal1/from_white1。内存before/after internal_free243911、largest196608、PSRAM6682524一致。

HL日志diff27ms/draw1408ms/buffer13ms/total1448ms；FULL日志16µs行/89.15Hz/36相预测404ms。两者含义不同，需要GC16相位实测，不能宣称时序错误或确定白偏因果。该图没有收到目视通过/拒绝，confirmed仍0；用户转向重新查询方案后，显式stop、DISPLAY_DEINIT/HAL_DEINIT ESP_OK，随后完整STOPPED/operation0/pendinginvalid/drawn80/confirmed0。Ctrl-C正常exit0，lsof无串口占用，白参考清除，屏上保留该图。指令仅status、clear-white、status、white-confirm、gc16-gray16-vendor、stop、status；无pattern-confirm/reject、补刷或第二张。

私有证据：`.cache/vendor-gray-device-console-private.log`、`vendor-gray-app-flash-private.log`、`vendor-gray-app-readback.bin`、`vendor-gray-partition-{before,after}.bin`及同前缀核验日志，均忽略。没有修改固件/重新编译，沿用同SHA的289CTest/152sanitizer/10构建保护/8控制台/四配置证据。

研究复核：官方产品/Demo/DIY文档及固定app_refresh/display/波形/UI/板级/renderer源码核对；git ls-remote确认HEAD/main仍28cde682…。新增 [16灰阶验证方案](GRAY16-VALIDATION-PLAN.md)，明确GC16铺白/连续HV/局部布局尚未复现；下轮先官方图案与GC16相位观测，之后逐项电源/扫描对照。未查得已验证适用于本机的现成修复。研究阶段只结束旧会话，没有再次烧写或刷新；新方案尚未实现/实屏。

| 阶段 | 主机/静态检查 | 固件编译 | 真机验收 | 当前状态 |
| --- | --- | --- | --- | --- |
| P0 基线 | 118 CTest + 6 方向 + 60 Web 通过 | 原板/v6.1，独立目录及当前根目录通过 | 未操作设备 | 已完成；远端 CI 未运行 |
| P1 板级 | HAL/PMU、10 构建保护及 8 控制台测试通过 | factory/双 OTA 只读、factory 显示及原板回归通过 | 扫描诊断回读/分区表不变、启动/PMU/VCOM/清白退出及显示/HAL 清理通过 | PCB/产品时序/连续电源待验收；放弃完整备份 |
| P2 显示 | PM锁候选310/310，构建/控制台20/20通过 | `6f2a4ea…`/305,104字节仅factory部署并全长回读一致 | 清白通过；局部页36相399.327ms，照片16阶严格单调，确认后STOPPED | 16灰阶根因关闭；硬取消、温度精度、重复性、其他图案/照片链路与长期稳定待验收 |
| P3 相册 | 图片处理、路径、容量/错误返回 | 完整业务集成 | Web 上传→SD→显示、重启、轮播 | 未开始 |
| P4 输入/电源 | 手势、并发请求、睡眠顺序/失败 | PMU/Touch 及其他板型回归 | 左滑、电池、按键/定时唤醒、实测电流 | 未开始 |
| P5 完整验收 | 格式/OOM/缓存完整性、回归 | 发布配置与产物身份 | 满卡/拔卡/写中断、Web/REST/OTA、长稳 | 未开始 |
| G1–G3 相机 | 协议/路径/恢复与能力矩阵 | 独立相机模块集成 | 分型号/固件的连接、下载与同步 | 后置 |

第一阶段稳定性初始目标：100 次 GC16 轮换、连续左滑 200 次、至少 50 次定时睡醒、USB 运行 24 小时及电池睡醒 100 轮。记录实际次数与失败情况，不用目标次数替代结果。功耗以同板参考固件及真实测量为依据，暂不虚构绝对指标。

## 每次验证的记录字段

- 日期、任务 ID、变更范围、主工程/BSP/SDK commit、配置与产物 SHA256。
- 主机测试：命令、样本/故障覆盖、通过/失败数；无适用测试时说明原因。
- 固件编译：板型、SDK、命令、产物与结果。
- 烧写/回读：备份与恢复位置、参数依据、核验结果。
- 真机：板卡版本、供电/SD/温度条件、步骤、日志位置、实屏或测量结论。
- 限制：未覆盖项、失败项、下一步及对应 TODO 状态。

只记录与本轮产物一致的证据；私人样片和可识别设备的信息留在私有资料目录，公共记录脱敏。

## 2026-10-03 · 文档重整

本轮仅整理文档、计划及引用：核对 43 份历史文档归档前后 SHA256、研究报告副本、当前文档本地链接与 Git index 保留情况。核对结果见 [归档说明](archive/README.md)。

结果：43/43 历史文件摘要一致；研究报告副本与下载原件摘要一致；15 份当前/索引文档的 46 个本地链接全部可解析；Git index 仍为原有两项，模式与对象 ID 均未改变。

未运行主机代码测试、未编译固件、未烧写或验证真机。旧工程的 49 项测试以及显示/蓝牙实验不计入新方案。

## 2026-10-03 · 官方资料与固定源码核对

- 官方产品、Demo、DIY、CrossMux 说明已查阅；PMU 协议文本、板定义、SD、时序、配置和 PhotoFrame 关键接口/测试入口已核对，来源链接见 HARDWARE-REFERENCE.md 与 UPSTREAM-BASELINE.md。
- GitHub API 核验两个候选 commit 返回 200，完整 SHA 与各自本次 main 一致；未引入当前主工程或执行构建。
- 文档检查：18 份当前/索引文档的 65 个本地链接全部可解析；43/43 历史归档摘要仍一致；原 Git index 两项的模式和对象 ID 保持不变。
- 本轮发现并加入 TODO：显示接口错误传播、SD 唯一挂载所有权、NVS/PHY/OTA 分区迁移、遗漏主机目标的 CTest 补跑，以及 PMU 闹钟/供电状态验证。
- 未安装依赖、未运行主机测试、未编译固件、未读取串口或改变设备。官方规格、源码声明和本机验证继续分别记录。

## 2026-10-03 · P0 源码引入与原版基线验证

### 变更与固定身份

- PhotoFrame 根目录及上游历史：`f5e3ec974403b31174a3c8da1cf09385453f4abb`，当前分支 `codex/read-pico-port`。未修改上游 main/components/boards/算法/驱动。
- Read Pico 子模块：`28cde682a4468a581c278761922724f57d976418`，gitlink 固定，工作树干净；仅作参考，未链接进本轮固件。
- ESP-IDF 正式 tag v6.1：`fff9895c82d744c7237be8847347bdd1b07c6643`，header 6.1.0，递归子模块就绪、SDK 工作树干净。
- 环境：macOS 15.7.7 arm64，Python 3.14.6，Node 22.23.3/npm 10.9.9，CMake 4.3.4，Ninja 1.13.2，主机 libpng 1.6.58；固件 Xtensa GCC 15.2.0（esp-15.2.0_20251204），idf-component-manager 3.1.2，esptool 5.4.0，qrcode 8.2，librsvg 2.63.2。
- 为构建安装了 Homebrew librsvg/Node 22 及其依赖（部分既有依赖由 Homebrew 升级）；Node 22 未全局 link，未修改 shell 配置。SDK 使用独立 Python 环境，旧 6.2 开发 SDK 不动。
- 根依赖锁 SHA256：`4639f56e78af56730ceed80dcb2c64aa3678647d4be4092f5ab77656b9283514`；与独立原版解析结果一致。npm 锁文件无修改；SDK Python 约束文件 SHA256：`e86ce7492aed3cd9f4c72473d74e19eff11e3cf379e7fcfd0cb85922e996c8dd`。

### 主机与 Web

| 命令/覆盖 | 结果 | 执行位置 |
| --- | --- | --- |
| make test（6 个 C/C++ 可执行程序及方向测试） | 通过；方向 6/6 | 独立未改上游、当前根目录；根目录另用 Node 22 复跑 |
| ctest --test-dir host_tests/build --output-on-failure | 118/118；7 个测试目标，包含 Makefile 漏跑的 wake_schedule | 独立未改上游及当前根目录 |
| npm --prefix webapp ci / test | 2 个文件、60/60 测试通过 | 当前根目录，Node 22 |
| npm --prefix webapp run build | 生产构建通过 | 当前根目录，Node 22 |
| bash scripts/test_baseline.sh | exit 0；118 CTest + 6 方向 + 60 Web 及生产构建通过 | 当前根目录，Node 22 |

没有把不同执行目录的重复用例累加为新增覆盖。主机测试不证明 Read Pico 显示、SD、PMU 或功耗。

### 编译与产物

独立目录 `/Users/zhanghuilong/Documents/ChatGPT/R-Core-3-2-baseline-FFl8Vm/photoframe`：执行原始 `python build.py --board waveshare_photopainter_73`，Web → splash → firmware 全步骤成功，源码工作树保持原样。

当前根目录：`npm --prefix webapp ci` / `npm --prefix webapp run build` 后，固定 SDK 下执行 `python build.py --board waveshare_photopainter_73 --step splash --step firmware`，1236 个主构建步骤通过，生成应用大小 `0x1f6680`（2,057,856 字节）。完整基线脚本重建 Web 后再增量编译成功，产物摘要不变。已实际验证路径含空格可构建。

配置为 `sdkconfig.defaults;boards/sdkconfig.defaults.waveshare_photopainter_73`；原板 ESP32-S3/16 MB，无内部图片存储，两槽 OTA 各 0x780000。生成 sdkconfig SHA256：`357941c70d4dee6b5d5a1e17c2cb3141c5b38ccb89e048f8f305956554acaee2`。

| 当前根目录产物 | SHA256 |
| --- | --- |
| build/esp32-photoframe.bin | `9ac1d6fc6b7317238189cbb5b448383766de329a83bbe0bde0650f4097c7342c` |
| build/bootloader/bootloader.bin | `bc12246d3a3159c551f4923dc86ac3685e125b69a7ea749497337e95c5c8bcad` |
| build/partition_table/partition-table.bin | `861b9d8dd765e3d088fe0094a653f7fdf69a4208937d8af921a480ae3b7265e7` |

这是原板基线，不能烧入 Read Pico。未执行 flash、erase、monitor、OTA、PMU 写入或 SD 操作。编译工具输出的烧写示例未执行，设备当前状态未知。

### 告警、失败尝试与边界

- 最初使用默认 Python 运行 splash，缺 qrcode 而失败；改用独立 SDK Python 并安装 qrcode 8.2 后成功。
- 默认 Node 23 产生 Vitest engines 告警；最终用 Node 22 重跑测试/生产构建。未将不受支持环境作为最终前端基线。
- 原版主机编译有 image_processor.c 中 size_t 使用 `%u` 的格式告警；固件有原字体数组截断 NUL 告警和 SDK Wi-Fi 私有 include 依赖告警；Web 有浏览器 externalization/chunk size 告警。保留原码并记录，不宣称零告警。
- 新增 port-baseline.yml 已做本地结构检查，GitHub Actions 未运行；原有 v6.0/8 板编译矩阵及 format-check 未复跑。当前版本未有 Read Pico 移植固件。

本机日志（忽略，不入库）：`.cache/p0-host-root.log`、`.cache/p0-host-node22.log`、`.cache/p0-ctest-root.log`、`.cache/p0-webapp-tests.log`、`.cache/p0-webapp-build.log`、`.cache/p0-firmware-root.log`、`.cache/p0-firmware-final.log`、`.cache/p0-baseline-script.log`；独立原版日志在基线目录 `p0-upstream-build.log`。保留 Git/文档备份，未删除原资料。

收尾静态检查：19 份当前/索引/上游 README 的 77 个本地链接可解析；43/43 历史归档摘要不变，研究报告副本与下载原件摘要一致，README.upstream.md 与上游文件摘要一致；git diff --check、脚本 bash 语法、CI YAML 结构及依赖锁结构检查通过。独立上游、SDK、Read Pico 子模块均无工作树修改。历史两条废纸篓备份路径本轮已不存在，原因未知，已修正索引并纳入首次烧写前风险。

备份 SHA256：Git 元数据 `24bad475610b00cade594c25fc966a4924c725436a638080175bce417beeb607`；文档 `8f8cd3b49f6ab2af142d531ee13bda0782129ec37ed45203af3e74bfa4e5e39c`。下一步：P1 注册 Read Pico、筛选组件、只读 PMU/VCOM、SD 挂载所有权及首次烧写前分区/备份方案。

## 2026-10-03 · P1 只读板级诊断实现与编译

### 范围与身份

固定 PhotoFrame、Read Pico 与 SDK commit 沿用 P0；Read Pico 子模块工作树保持干净，组件锁摘要仍为 `4639f56e78af56730ceed80dcb2c64aa3678647d4be4092f5ab77656b9283514`。新增第 9 个板型、独立诊断 main、HAL I2C 适配与便携 PMU 编解码；不链接厂家驱动库，不启动完整相框。详见 [诊断边界](READ-PICO-BRINGUP.md)。

诊断以 80 MHz Flash/Octal PSRAM 编译；产品 120 MHz/HPM、实物容量与器件未验收。Flash coredump 显式禁用。PMU 仅 IDENTITY/STATUS/VCOM GET，IOE 只读 CFG0/OUT0，SY 仅探测；GET 有命令帧写入但不写校准。未 HOST_READY、drain/ACK、设置电源、挂载 SD、初始化/擦除 NVS 或烧写设备。

### 主机与静态检查

| 检查 | 实际结果 |
| --- | --- |
| `bash scripts/test_baseline.sh`（Node 22） | exit 0；146 CTest、6 方向、60 Web、2 构建防误操作测试及 Web 生产构建通过 |
| 新增 PMU codec 测试 | 19/19，CRC、协议/身份、未知电池/充电、序号、VCOM 会话/响应及范围 |
| 新增模拟 I2C 板级测试 | 9/9，资源/错误路径、快照失效/过期、PMU 重启、VCOM 缺失、清理失败重试 |
| 新增 28 项 ASan/UBSan | 28/28，无 sanitizer 错误；不是全部 146 项的 sanitizer 覆盖 |
| 构建防误操作 | 2 unittest；其中 5 个子场景拒绝 flash/monitor/erase/fullclean/配置覆盖，并在构建前退出 |
| ELF 已定义符号检查 | 诊断 app_main/快照入口存在；无 nvs_flash_init/erase、epaper_init/display、sdcard_init、esp_wifi_init、esp_core_dump_write |
| 静态/资料保护 | git diff --check、bash 语法、CI YAML/SDK/双板命令检查通过；23 份文档的 87 个本地链接可解析，历史 43/43 摘要及研究报告原件/副本一致 |
| 新增 C/C++ 格式 | clang-format 20 dry-run 通过；未复跑上游 clang-format 18 全项目 format-check |

没有累加同一用例的重复执行数。模拟 IO 与 sanitizer 不能证明硬件通信或波形正确。新基线 CI 双板编译、原 CI 主机测试递归子模块配置已补入；远端 CI 未运行。

### 固件编译与本轮最终产物

在 P0 同一固定 SDK/工具环境运行：`python build.py --board read_pico --step firmware` 与 `python build.py --board waveshare_photopainter_73 --step firmware`，均 exit 0。Read Pico 应用 `0x371a0`（225,696 字节），原板回归应用 `0x1f66e0`（2,057,952 字节）。原板产物含新增 Web 板型元数据，因此与 P0 摘要不同；不是产品发布。

| 产物/配置 | SHA256 |
| --- | --- |
| build-read-pico/esp32-photoframe.bin | `f1dfb882220b76ca60ded16c4f5818c1b1dd78bfa16fd37d3a5aa7d855d63824` |
| build-read-pico/bootloader/bootloader.bin | `78f11c92712a956992941b068c4b16e2b2c7a348c5c2131db5a1e567495a89ed` |
| build-read-pico/partition_table/partition-table.bin | `861b9d8dd765e3d088fe0094a653f7fdf69a4208937d8af921a480ae3b7265e7` |
| .cache/sdkconfig.read_pico | `45f6b010e56bbdaf5d2840e42e8f7345f15bc036ddf503a1294c4828c482bcc0` |
| build/esp32-photoframe.bin（原板回归） | `ff274e0e19515b18a62f5b37a12e1078821bf2e79f11537ef69506fc1429980e` |
| sdkconfig（原板回归） | `db6422487dbc591fb09aad61de553b57d57bcdfbe2ca8c85f362add757b10a9a` |

共享分区生成器仍输出原版双 OTA 布局；诊断的独立 sdkconfig 已参与分区生成，但迁移方案未批准，不能照用构建日志的烧写示例。原板 sdkconfig 未切换为 Read Pico。

### 修复尝试、限制与证据位置

首次诊断编译缺 esp_psram 头：修正 IDF 早期依赖扫描阶段尚不可见 CONFIG 的依赖声明后编译通过。新增测试初版有枚举列表推导及 macOS reboot 同名编译错误，修正后完整测试与 sanitizer 通过。最终重编译已包含格式化、SDK 6.1.0 编译约束与禁用 Flash coredump；不引用更早产物摘要。

原版字体/格式及 Web 打包告警边界沿用 P0，不宣称全项目零告警。fast-context 因缺本机 Windsurf 凭据未能使用，改用 rg 与源码阅读；没有提取或输出凭据。

本机日志（忽略、不入库）：`.cache/p1-baseline-regression-final.log`、`.cache/p1-ctest-final.log`、`.cache/p1-asan.log`、`.cache/p1-read-pico-final.log`、`.cache/p1-original-board-final.log`。本轮没有 Git 提交/推送、读取串口、flash/monitor/erase、OTA、实屏/功耗验证。P1 仅部分完成；下一步 P2 外层 GC16 显示适配，产品时序、完整显示电源、有效备份和分区迁移仍需落实。

## 2026-10-03 · P2a 像素格式与刷新事务核心

### 实现边界

新增 `components/read_pico_display/` 便携组件：高 nibble PhotoFrame → 低 nibble EPDiy、顺时针四种旋转、独立 stride/长度与溢出/重叠校验；0 黑/15 白不隐式反色。新增同步 GC16 事务及结构化结果，区分 gate/帧错误、prepare/draw/stop 原生错误，失败后清理，不把返回 void 当成功。

真实 transport、epaper_* 后端及业务结果传播尚未连接。诊断 main 仅增加 16 像素内存自检，不初始化 EPDiy、不启用屏电源。厂家 board init 的 I2C/电源副作用已核对，未直接复用。契约、缓冲所有权与待实现条件见 [P2a 说明](READ-PICO-DISPLAY.md)。

### 实际验证

| 命令/检查 | 结果及限制 |
| --- | --- |
| CMake 主机构建 + 全 CTest | 173/173（原有 146 + 新增 27），exit 0 |
| 新增格式测试 | 14/14；全灰阶、奇偶像素、四旋转、stride、奇数尺寸、边界/溢出/重叠、源图保留；实际根 GUI_Paint 原码生成全帧并逐像素核验 831,744 像素 |
| 新增模拟刷新测试 | 13/13；gate 关闭、BUSY/重入拒绝、错误长度、锁覆盖整个事务、部分 prepare/绘制/清理失败与双错误保留；未证明真实 RTOS 并发/PGOOD/扫描 |
| ASan/UBSan 新显示目标 | 27/27，无 sanitizer 错误，包含实际 GUI_Paint；不是全部 173 项的 sanitizer 覆盖 |
| `bash scripts/test_baseline.sh`（Node 22） | exit 0；173 CTest、6 方向、60 Web、2 构建保护测试与 Web 生产构建通过 |
| 固定 v6.1 双板编译 | read_pico 诊断及 waveshare_photopainter_73 回归均 exit 0，两个便携 C 源文件均实际编译 |
| 诊断 ELF 已定义符号 | app_main、read_pico_frame_convert 存在；无 epd_init/poweron、epaper_init/display、SD/Wi-Fi/NVS 初始化/擦除、Flash coredump 写入入口 |
| 文档/源码保护 | 24 份文档 97 个本地链接可解析；历史 43/43 摘要及研究报告原件/副本一致；厂家子模块与 GUI_Paint 未修改，组件锁未变化；git diff --check 和新增 C/C++ clang-format 20 dry-run 通过 |

主机首次仅核心测试 25 项通过，随后补实际 GUI_Paint 与重入测试并最终复跑 27 项；以最终 173/27 计数，不累加重复执行。新目标编译原 GUI_Paint 出现已有 const char 与 224 比较告警（GUI_Paint.c:804），未改动原码；P0 Web 打包告警仍存在。未复跑 clang-format 18 全项目 format-check 或远端 CI。

### 本轮最终产物

固定 commit/工具链沿用 P0/P1。Read Pico 应用大小 `0x37540`（226,624 字节），原板回归仍为 `0x1f66e0`；原板应用摘要与 P1 相同，新核心无原板运行时调用。Read Pico 配置、bootloader 与分区摘要均未变化，分区迁移仍未批准。

| 产物/配置 | SHA256 |
| --- | --- |
| build-read-pico/esp32-photoframe.bin | `3144446ddb1c3a7e385a2758d2b47e6f53a4c5b3dc9d13cfab90aedab56cc08b` |
| build-read-pico/bootloader/bootloader.bin | `78f11c92712a956992941b068c4b16e2b2c7a348c5c2131db5a1e567495a89ed` |
| build-read-pico/partition_table/partition-table.bin | `861b9d8dd765e3d088fe0094a653f7fdf69a4208937d8af921a480ae3b7265e7` |
| .cache/sdkconfig.read_pico | `45f6b010e56bbdaf5d2840e42e8f7345f15bc036ddf503a1294c4828c482bcc0` |
| build/esp32-photoframe.bin（原板回归） | `ff274e0e19515b18a62f5b37a12e1078821bf2e79f11537ef69506fc1429980e` |

日志（忽略、不入库）：`.cache/p2a-host.log`、`.cache/p2a-asan.log`、`.cache/p2a-baseline.log`、`.cache/p2a-read-pico.log`、`.cache/p2a-original.log`。本轮未烧写/读串口/真机内存自检/显示/功耗/SD/OTA，未提交或推送。下一步 P2b：共享 I2C 与显示电源所有权、PGOOD/失败锁存、厂家时序/GC16 transport、上层错误传播；P2 实屏完成条件仍未达到。

## 2026-10-03 · P2b 原生后端与检查式业务结果

### 实现和运行边界

新增独立 Read Pico 后端，借用 HAL 单一 I2C0，显示租约阻止借用期间释放总线；显示→递归总线锁顺序、IOE 掩码/读回保留触摸与输入信号、拒绝接管已有高压轨。外层 SY 适配保留厂家标准寄存器值与启动顺序，补失败返回与可重试资源清理；30 次有界 PGOOD 轮询，不把读取错误当高电平。固定厂家源码/波形/校准未改。

选入厂家 EPDiy、波形、SY 故障解码器及原位编译的扫描时序；16-bit 原 LCD 引脚、12 MHz/FULL 扫描、原生横向、GC16 全刷、关闭 leading-skip。使用 PMU 读取的 VCOM 和实时 SY 温度/故障状态；异常数字预检不启动高压轨，绘制/退出错误锁存到重启。首次白屏参考须人确认，void 清屏返回不是成功证据；厂家 renderer/LCD 的 assert/abort 与真实 DMA 风险尚未真机验证。

新板 display_manager 使用检查式刷新，失败不发布链接/快照/当前图，不清除 NVS；顺序/随机轮播失败不保存 last_image 或更新索引。新增 checked 轮播接口将错误交回 utils，原板保留 void 调用行为。Read Pico CMake 编译实际 display_manager/utils，但未启动完整业务。

**用户已明确允许厂家标准 SY 易失性启动配置，禁止写回 PMU/NVS 校准。** 该授权不包括烧写、任意调参或波形改动。诊断仍只执行 `read_pico_epaper_init(false)` 和内存自检/PMU 观测，真实显示代码已链接但不启动；无 SD/Wi-Fi/NVS 业务初始化。实板身份、USB/备份位置和分区仍待用户补充/核验；不能把编译布局直接用于烧写。

### 实际验证

| 检查 | 结果和覆盖边界 |
| --- | --- |
| 最终主机构建及 CTest | 215/215，exit 0；较 P2a 新增 42 项，不累加重复执行 |
| 共享 HAL 新增用例 | 4 项：显示租约/退出保护、递归锁和跨线程排斥、IOE 保留触摸/输入、现有轨与读回错误拒绝；板级共 13 项 |
| SY 适配 | 15 项：被动初始化/地址、VCOM 范围和标准值、STOP/read、实际厂家故障解码、签名温度、PGOOD 超时/IO、逐项部分写失败、退出首错保留、设备释放重试；模拟 I2C/延时，不是电气测量 |
| 实际 display_manager | 11 项：文件/RGB/流式/灰阶/清屏/轮播失败不提交状态、BUSY 返回、锁释放、成功白屏 0xff/单次检查式刷新；GUI_Paint/RawBuffer 为实际源码，解码/NVS/屏幕边界为替身 |
| 实际外层原生后端 | 12 项：false guard 无副作用、PMU/VCOM/内存拒绝、拒接现有轨、白屏确认 gate、重入 BUSY、真实 SY/便携核心、灰阶 nibble、温度/故障预检、VCOM 变化、PGOOD 读错、prepare/draw/stop 失败与故障锁存、释放重试；HAL/LCD/RTOS/heap 是替身，各场景用子进程模拟独立启动 |
| ASan/UBSan | 78/78（13 板级 + 27 核心 + 15 SY + 11 业务 + 12 后端），无 sanitizer 报错；后端子进程退出不作为完整产品长期泄漏证明，主要释放路径另核对分配计数 |
| 基线脚本（Node 22） | exit 0；215 CTest、6 方向、60 Web、2 构建防误操作及 Web 生产构建通过；最终增量修改后另复跑 215 CTest/78 sanitizer 与双板编译 |
| 固定 v6.1 双板编译 | Read Pico 诊断/原生后端/实际 display_manager/utils、原板回归均 exit 0；后端代码实际链接，不只生成无引用静态库 |
| 诊断 ELF | app_main、read_pico_epaper_init/display、epd_init/poweron 存在；无 nvs_flash_init/erase、sdcard_init、esp_wifi_init、厂家总 BSP/PMU init、SY set_* 校准入口或 Flash coredump 写入入口；符号存在不等于执行 |
| 资料与来源 | 厂家子模块干净、43/43 归档摘要不变、根锁摘要不变、20 份当前入口/索引文档 90 本地链接可解析；新增 C/C++ clang-format 20、git diff --check 和 bash 语法通过 |

保留原字体/GUI_Paint 与 Web externalization/chunk size 告警；未运行远端 CI、v6.0/其他板矩阵、clang-format 18 全项目检查。最初新公共 HAL 头的 I2C 依赖是 private，导致诊断缺头编译失败；已改为 public REQUIRES 并完成最终双板回归。没有把首次失败当作通过。

### 本轮最终产物

固定主工程/gitlink/SDK/工具链沿用 P0。Read Pico 应用 `0x43170`（274,800 字节），原板 `0x1f6770`（2,058,096 字节）。配置、bootloader、分区与根依赖锁未变；原板因公共业务适配编译变化，不声称二进制不变。所有产物均未烧写。

| 产物/配置 | SHA256 |
| --- | --- |
| build-read-pico/esp32-photoframe.bin | `fe8c38452f6b8617f50c1aeb82349b91ecf9118c6864543fbe7817ce530d19e0` |
| build-read-pico/bootloader/bootloader.bin | `78f11c92712a956992941b068c4b16e2b2c7a348c5c2131db5a1e567495a89ed` |
| build-read-pico/partition_table/partition-table.bin | `861b9d8dd765e3d088fe0094a653f7fdf69a4208937d8af921a480ae3b7265e7` |
| .cache/sdkconfig.read_pico | `45f6b010e56bbdaf5d2840e42e8f7345f15bc036ddf503a1294c4828c482bcc0` |
| build/esp32-photoframe.bin（原板） | `c3bd5346024941d87f0be5c331076aed9914a989da878e89c38df12113455104` |

日志（忽略、不入库）：`.cache/p2b-host.log`、`.cache/p2b-sy-test.log`、`.cache/p2b-commit-test.log`、`.cache/p2b-native-test.log`、`.cache/p2b-asan.log`、`.cache/p2b-baseline.log`、`.cache/p2b-firmware.log`、`.cache/p2b-original.log`。本轮未连接设备、读串口、烧写/回读、实屏、SD/OTA/功耗验证或提交/推送。下一步依照 [首次迁移审查](DEPLOYMENT.md) 取得实板与备份证据，再进行受控验收；P2 完成条件仍未达到。

## 2026-10-03 · factory 只读诊断首次上板

### 用户选择、只读识别与布局

用户确认“小纸 Pico，已连接 Mac，不需要备份”，随后明确批准核验兼容性后仅覆盖 factory 应用，用于不启动屏电源的 PMU/VCOM 诊断。未取得完整 Flash/旧应用恢复副本；本次批准不扩展到完整布局迁移、OTA 或显示测试。

在 `/dev/cu.usbmodem2101` 使用固定环境 esptool 5.4.0、`--chip esp32s3 --no-stub --connect-attempts 2`，先执行 ROM `flash-id`、`get-security-info`，再只读取 0x8000/4096 字节分区表和 0x10000/4096 字节旧 app 头。日志脱敏，未读 NVS/PHY/FAT 内容。ROM 操作会复位 ESP；不声称运行中的旧 app 没有自己的副作用。

- ESP32-S3 QFN56 v0.2，晶振 40 MHz，8 MB AP 3.3 V PSRAM，USB Serial/JTAG；Flash 0x5e/0x4018，16 MB。
- Secure Boot/Flash Encryption 均关闭，Crypt Count=0；未写 eFuse 或读取密钥内容。
- SDK 分区解析器验证表成功：NVS 0x9000/0x5000，PHY 0xe000/0x1000，factory 0x10000/0x400000，FAT 0x410000/0x500000。
- 旧应用描述符：`gr4_paper_frame 0.1.0`，IDF v6.1，编译 2026-10-03 08:04:01，64 KB MMU，ESP32-S3/DIO/80 MHz 头；这不是厂家 Demo 应用身份，也没有完整镜像校验。其描述符 ELF SHA 为 `2b7dca6366eda7d777da6e8ae388a298b4e89fa327590c45adc468f31a9cca0a`，不等于 app bin 摘要。
- 对 4 KiB 截断头尝试 esptool image-info 因 segment EOF 失败；随后依据固定 SDK `esp_app_desc.h` 验证 0xe9/0xabcd5432 并解析本地描述符。没有为了该错误读取或备份完整旧 app。

### 新配置、编译和范围核验

新增 `boards/partitions.read_pico_factory.csv` 与 defaults overlay，以及 `--read-pico-factory-diagnostic` 编译选项：只允许 read_pico/单独 firmware 步骤，独立 build/config 目录，跳过共享双 OTA 表生成。构建脚本仍拒绝 flash/monitor/erase/fullclean/额外参数，未增加自动设备操作。固定 commit/SDK/锁沿用 P2b，厂家源码未改。

本次切换原板/Read Pico 配置时，组件管理器重写锁文件：原板摘要 `4639f56e…`，选入厂家 SY/波形的 Read Pico 摘要 `c499f738912ad362e66884b6190edc8c1428174fca47d6685e9dad2262997d65`。diff 仅增加直接 idf 要求并改变 manifest_hash，两个 dependencies 对象完全一致；保留生成的当前锁，未手工回退或升级包。P2b 的“锁未变”是其原板最后构建后的历史状态，不是所有配置都生成同一摘要的承诺。

- factory 编译 exit 0，app 273,680 字节（0x42d10）；image-info 校验和/hash 有效，S3、v0.0–0.99、64 KB MMU、DIO/80 MHz、16 MB，符合设备 v0.2/eFuse v1.4 与应用空间。
- 新生成表 3072 字节与设备表相应前缀逐字节一致，设备剩余 1024 字节全 0xff。
- ELF 包含诊断 app_main/显示后端，但无 NVS 初始化/擦除、SD/Wi-Fi 初始化或 Flash coredump 写入入口。仅 false guard/只读观测实际执行。
- CTest 215/215 与扩展后 5/5 构建保护测试通过；factory、原板和默认双 OTA 诊断三个配置均编译 exit 0。上轮 78 sanitizer/6 方向/60 Web 结果保留，本次未重复 Web/sanitizer，未修改图像/状态/电源 C 代码。
- CI 添加 factory 配置编译，远端未执行，不计 CI 通过。

### app-only 写入与回读

人工授权后实际执行（固定 SDK Python）：

```sh
python -m esptool --chip esp32s3 --port /dev/cu.usbmodem2101 --no-stub \
  --connect-attempts 2 --after no-reset write-flash --no-compress \
  --flash-mode keep --flash-freq keep --flash-size keep \
  0x10000 build-read-pico-factory/esp32-photoframe.bin
```

工具记录 erase `[0x10000, 0x53000)`，写入 273,680 字节 `[0x10000, 0x52d10)`，写后 hash 验证成功。没有使用 `flash_args` 或新 bootloader/分区表，没有 erase-all/NVS 擦除、SD 格式化或 PMU 校准写。

保持 ROM 状态，用 `--before no-reset --after no-reset` 回读 app 实际长度，cmp 与本地镜像一致；再回读 0x8000/4096 分区表，cmp 与写入前一致。保护数据区域依据写入/擦除范围未触及，但未对所有 NVS/PHY/FAT 内容做前后完整校验，不把表不变当作所有用户数据 checksum 已验证。

| 产物/读数 | SHA256 |
| --- | --- |
| factory app 与设备全长回读 | `28d83305c39d8c4509783545f03d633dc33d8a577c7d8f9939771797c30db93e` |
| factory 生成的 3072 字节分区表（未烧写） | `205e21ddff471bce79c29a846bd04a48854cb42d406c784405d4c8e3ec1e6061` |
| factory sdkconfig | `b71f7f5acf66a8d1b24b8da81ff3a757ae15756ae976def1978163045cc39293` |
| 设备 4096 字节分区表（写前/写后相同） | `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20` |
| 旧应用 4096 字节头（不是完整 app 备份） | `c732e5f839e9f2bb28b23d4a3695805d983858a69b77f164babd560fac0b891c` |

### 真机首次启动

通过固定 esptool HardReset 在已打开的只读串口上复位并捕获日志，20 秒有界等待，诊断结束后再观测 2 秒。保留的旧 bootloader 日志为 IDF v6.1、编译 2026-10-03 08:04:07、DIO/80 MHz/16 MB，从 factory 0x10000 成功加载本次 ELF（前缀 `2e420dd23`）。

- PSRAM 8 MB/80 MHz，SPI SRAM memory test OK；应用报告 PSRAM=8,388,608 字节。
- 16 像素 4bpp 内存自检通过；panel remains disabled。
- PMU fw=1.0.9、hw=1、state=3（RUNNING）、faults=0x1。固定协议规定 bit0 为 BOOT_WAIT 超时粘滞位，可能早已置位，本轮未清除/未发 HOST_READY，不判定为屏电源故障，也不宣称所有 PMU 功能已验收。
- 电池瞬时 4091 mV、100%、charging=0、external_power=unknown；unknown 不代表 USB 未接或电池独立供电。
- IOE CFG0=0x64、OUT0=0x81，SY responding=0；EN/VCOM_EN 未启用时 NACK 属于诊断预期，不验证真实 SY 温度/PGOOD/电源转换。
- Factory VCOM valid=1、mV=1580；这是 PMU 标定值读取，不是模拟电压实测，不写回 PMU/NVS。
- 诊断结束、HAL 清理无错误日志，main_task Returned from app_main；捕获窗口无 panic/复位循环。不是长期稳定性或实屏验证。

私有忽略日志/二进制在 `.cache/device-*-private.log`、`.cache/device-*.bin`；构建/回归日志 `.cache/factory-diagnostic-build.log`、`.cache/factory-original-regression.log`、`.cache/factory-ota-diagnostic-regression.log`、`.cache/factory-host-regression.log`。未提交/推送，未修改厂家驱动/波形/校准。下一步需要用户安排并确认受控首次清白，白屏实视确认后才进入 GC16 图案验收；完整相框/OTA 迁移未批准。

## 2026-10-03 · 受控首次清白、人工确认与退出

### 范围、实现与回归

用户另行确认允许受控首次清白，沿用小纸 Pico、放弃完整备份、仅 factory app 写入，以及允许厂家标准 SY 易失性启动但禁止 PMU/NVS 校准写的边界。固定 PhotoFrame/Read Pico/SDK commit 沿用前述记录；第三方驱动、扫描时序、波形及校准源码未修改。新入口不初始化 NVS/SD/Wi-Fi，不执行 GC16 或完整相框业务。

新增独立显示编译配置、便携白屏会话与 48 字节 ASCII 命令解析、命令白名单 USB 控制台。启动仅预检，只有显式 clear-white 执行一次未验证清白；人工确认后才能 white-confirm。失败锁存，无自动确认/重试/绘制。stop 成功终止本会话并清除逻辑参考，不能因屏幕仍白就跨会话沿用。

| 检查 | 本轮实际结果 |
| --- | --- |
| 全部 CTest | 228/228；较上轮新增 10 会话/命令、3 HAL CRC 用例 |
| ASan/UBSan | 91/91，板级/格式/事务/SY/业务提交/原生后端/会话；非全部 228 项覆盖 |
| 构建防误操作 | 7/7；显示选项要求 Read Pico + factory + firmware-only，拒绝其他动作 |
| 固件编译 | 固定 v6.1，原板、只读双 OTA、只读 factory、显示 factory 四配置均 exit 0 |
| 方向/Web | 沿用上轮 6/60，本轮未复跑；远端 CI、v6.0/全板矩阵未运行 |

### CRC 拒绝与有界读取

首个显示应用（275,472 字节，SHA256 `520f3a738127c5aa6f297aec865a8e5730d9ec6d3d1867e24d1bb69496d651f5`）启动预检遇 STATUS `ESP_ERR_INVALID_CRC`，app_main 退出，未初始化屏电源、未发送 clear-white。固定厂家 PMU 源码已注明 STATUS 影子更新可能导致读撕裂；HAL 增加最多 3 次 STATUS 读取，仅 CRC 失败时以 2/5 ms 退避重读。IO 错误立即返回、持续 CRC 错误使快照失效；有效 STATUS 前不 GET，不重发 GET，不改变校准/session。

新增模拟用例覆盖两次 CRC 后恢复、持续 CRC 使旧快照失效、CRC 后 IO 失败立即退出。最终应用实板启动获得有效 CRC（未观察到本次触发重读）；这支持安全预检可运行，不能证明首次 CRC 的实际根因或重读已消除全部硬件错误。

### 最终应用与 app-only 核验

| 对象 | SHA256 / 范围 |
| --- | --- |
| build-read-pico-display/esp32-photoframe.bin，275,600 字节 | `2d64b0722365afedfa8530f297f1ce32beaf79ba5ddf8302ae06e37fab16c767` |
| 全长应用回读 .cache/white-crc-app-readback.bin | 同上，cmp 一致 |
| 显示配置 .cache/sdkconfig.read_pico-display | `804fd90ecdaefc95dcb88c5e7e9eb2a79c939ccfbb4357804ff72eff7e7422cf` |
| ELF | `a0e9e8513439b4a58c8b061195ed0739e51208e915b7f79531e70c21f09eaa9a` |
| 实板分区表，最终写入前/后各 4096 字节 | `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`，一致 |
| 最终写入 / 擦除范围 | `[0x10000, 0x53490)` / `[0x10000, 0x54000)`，均在 factory 内 |

Image-info 校验通过：ESP32-S3、IDF v6.1、DIO/80 MHz、16 MB、64 KiB MMU。保留既有 bootloader、NVS `0x9000/0x5000`、PHY `0xe000/0x1000`、factory `0x10000/0x400000`、FAT `0x410000/0x500000`；生成分区表 3072 字节与实板前缀及 FF 填充匹配。不写 bootloader/分区/NVS/PHY/FAT；NVS/FAT 没有全量内容摘要检查，保护依据是实际擦写范围。旧应用无完整备份，不能保证回滚。

### 同一次启动的清白与人工证据

USB Serial/JTAG 启动有效 PMU 1.0.9、VCOM valid/1580 mV，停在 WAIT_CLEAR_COMMAND。只发送一次 clear-white，观察如下：

| 步骤 | 实际结果 |
| --- | --- |
| 初始化前 | internal_free 336551、internal_largest 245760、psram_free 8384996 字节 |
| DISPLAY_INIT | ESP_OK，98 ms |
| 初始化/清白后 | internal_free 279643、internal_largest 200704、psram_free 6682524 字节 |
| CLEAR_RETURN | ESP_OK，含初始化 12266 ms，faulted=0；清白本身约 12168 ms |
| 独立退出观察 | ESP_OK，input0/out0=0x81；SY_EN/VCOM_EN/XOE 均低、PGOOD 低 |
| 人工目视 | 用户回复“整屏白色，无明显异常” |
| white-confirm | 同一 ESP 启动内显式发送，接受 WHITE_REFERENCE_CONFIRMED |
| stop | 显式发送，DISPLAY_DEINIT/HAL_DEINIT 均 ESP_OK，接受 STOPPED |
| 主机控制台 | 随后关闭，exit 0；无自动命令、复位、重连/重发 |

PMU faults=0x1 是协议 BOOT_WAIT timeout 黏滞标志，可能早于本轮；未清除该标志或发送 HOST_READY，不能将其解释为屏电源故障。LCD init 暂时将 12 MHz 限为 6 MHz 的厂家提示后，原始扫描配置设为 12 MHz/FULL；并不据此宣告产品时序已验收。没有对高压轨/VCOM 作电压实测，也未执行 GC16、16 阶/方向/照片、连续刷新、120 MHz/HPM、SD/网络业务或完整迁移。

本轮首次清白以软件电源退出检查与人工实屏确认共同计通过，不仅凭 void 清屏返回。当前 STOPPED，屏幕仍白但 deinit 已清除逻辑参考；后续 GC16 测试必须在新会话重新建立参考。私有日志 `.cache/white-console-private.log`（首次 CRC 拒绝）、`.cache/white-crc-console-private.log`（最终启动/清白/确认/退出）；回归/编译记录 `.cache/white-*.log`，回读二进制 `.cache/white-*.bin` 均忽略、不入库。无提交/推送，完整业务与 GR 尚未开始。

收尾复核：228/228 CTest、91/91 sanitizer、7/7 构建保护再次通过；控制台脚本 py_compile、CI YAML/四配置命令、新增及本次修改 C/C++ 的 clang-format 20 dry-run、git diff --check 通过。22 份当前/索引文档的 102 个本地链接可解析；Read Pico 子模块工作树干净，串口无主机进程占用。本次收尾未再刷机、复位或刷新屏幕。

## 2026-10-03 · GC16 标准图案与逐张确认（离线）

### 范围与实现

用户要求继续开发直到需要人工介入。本轮实现既有显示 profile 的六种便携标准图案及受控会话扩展，没有读取串口、复位、刷写、启动屏电源或调用实板 GC16。固定 PhotoFrame/BSP/SDK/依赖版本沿用前述，Read Pico 子模块原样；只在自有后端补充实时温度/VCOM gate 日志，不修改厂家驱动/波形/校准。

白/黑/中灰全帧、16×76px 灰阶竖带、三段 1px 竖/横/棋盘格、带 TL/TR/BL/BR 及 1/2/3/4 点的四角图，均为标准高 nibble 优先 4bpp。适配层按需临时分配 415,872 字节 PSRAM，经现有格式转换/新鲜 PMU 与实时温度 gate/GC16/电源退出；记录结果、主/退出错误、耗时、内存和独立 IOE/PGOOD 观察，随后释放输入帧。实板时延/内存峰值尚无本轮测量。

会话要求本次启动先 clear-white/人工白屏/white-confirm；每张图案只能显式绘制一次。成功后 WAIT_PATTERN_CONFIRM，只有显式人工 pattern-confirm 才允许下一张；pattern-reject 锁存 FAULT，软件失败同样禁止后续绘制，不自动清白/确认/重试/轮播。drawn 与 visually_confirmed 位分开，stop 后仅为本轮历史，不转移到新启动。旧白屏会话可不提供 draw callback，仍拒绝全部图案。

### 主机与编译证据

| 检查/命令 | 结果与覆盖 |
| --- | --- |
| cmake --build host_tests/build；ctest --test-dir host_tests/build --output-on-failure | 246/246，exit 0；较上轮新增 18 项，不累加重复执行 |
| 新图案测试 | 8 项：非法输入不写、首尾 guard/可重复、白黑灰全帧、16 阶逐像素、三段逐像素、四角标签/点数/边框、真实转换/nibble、名称与非法 ID |
| 新会话测试 | 8 项：本次白参考前拒绝、无回调白屏模式、逐张等待/一次性、六指令映射/人工确认位、失败锁存、人工拒绝、等待时 stop、精确 ASCII 白名单 |
| 新原生集成测试 | 2 项：六图案经真实转换/外层后端/GC16/电源退出并逐张人工 gate；模拟 draw 错误同时锁存会话及后端未知参考 |
| sanitizer 子集（开发说明命令） | 109/109，exit 0；含上述 18 新用例，不是全部 246 项覆盖 |
| python3 host_tests/test_read_pico_build.py | 7/7，无新增构建动作/烧写功能 |
| python3 host_tests/test_read_pico_console.py | 8/8，假 serial，无硬件；EOF 不发命令、不默认复位、精确白名单、未知/控制字节拒绝、短写/断连不重试、Ctrl-C 关闭、显式 reset |
| 固定 v6.1 四配置 | 原板、只读双 OTA、只读 factory、显示 factory 均编译 exit 0 |
| 静态核验 | clang-format 20 dry-run、Python py_compile、基线脚本 bash -n、git diff --check 通过；显示 ELF 有图案/会话/GC16 入口，无已定义 NVS init/erase、SD init、Wi-Fi init、Flash coredump write |

主机原生集成使用真实自有转换/后端/SY 外层与固定厂家故障解码器，但 renderer/LCD/RTOS/heap/总线为替身，不是实屏/模拟电压证据。方向 6 项、Web 60 项沿用上轮，本轮未复跑全部基线脚本/生产 Web 构建；远端 CI、v6.0/全板矩阵未运行。基线脚本已加入 8 控制台用例。

### 新产物身份（未部署）

| 对象 | SHA256 |
| --- | --- |
| build-read-pico-display/esp32-photoframe.bin，288,224 字节（0x465e0） | `ef3925f4f7b1ed7348b7bb844b4a3ed70c2f14b1bb97cb8c8c7f8e271e28aff5` |
| 新显示 ELF | `535c2f310ee63c1b9e7c6694729ecf2c4913cdf26315f4a8cb6262b88c9c9594` |
| 显示 sdkconfig | `804fd90ecdaefc95dcb88c5e7e9eb2a79c939ccfbb4357804ff72eff7e7422cf`，与前轮显示配置相同 |
| 原板回归应用 | `c3bd5346024941d87f0be5c331076aed9914a989da878e89c38df12113455104`，与前轮相同 |
| 只读双 OTA 应用 | `df749d1fbf655c8481833826b300ce954c2e6e07ccc27d60ae33b326f46f3d49` |
| 只读 factory 应用 | `5231430edf9c9ecf1ae5916a10af1ce539687b71e439ad7a57121f08ab7959fc` |

新显示 image-info checksum/validation hash 均 valid，ESP32-S3/IDF v6.1、DIO/80 MHz、16 MB、64 KiB MMU；factory 表不变。增量 app_desc 时间仍为 14:14:58，不将旧时间当作本轮构建身份，使用摘要核验。设备上次已验证应用仍为 275,600 字节、SHA256 `2d64b0722365afedfa8530f297f1ce32beaf79ba5ddf8302ae06e37fab16c767`，上次已 stop；本轮没有重新观察实时设备状态，也没有把新产物回读上板。

按新大小，拟 app 写入 `[0x10000,0x565e0)`、扇区擦除 `[0x10000,0x57000)`，均在原 factory 内，**尚未执行**。下一步需用户明确允许本轮 app-only 更新、重新建立白参考和标准 GC16 图案测试，并为每次实屏观察提供反馈；保留 bootloader/分区/NVS/PHY/FAT，沿用禁止 PMU/NVS 校准写及不完整迁移的边界。用户已放弃完整旧应用备份，不新增不可保证的恢复承诺。

日志在忽略目录 `.cache/gc16-host-*.log`、`.cache/gc16-asan-*.log`、`.cache/gc16-display-build-final.log`、`.cache/gc16-original-build.log`、`.cache/gc16-readonly-*-build.log`；该离线阶段没有串口/硬件操作。后续获准真机测试见下节；历史产物/结果不追改为当前状态。

## 2026-10-03 · 首次 GC16 四角欠载，已退出；性能候选未部署

### 实板更新与身份

用户“确认”批准本轮 factory app-only 更新、重新清白和逐张标准 GC16 测试，沿用放弃完整备份与禁写 PMU/NVS 校准边界。重新核对同一设备（身份仅私有比对）、ESP32-S3 v0.2、16 MB Flash（0x5e/0x4018）、8 MB AP PSRAM、Flash 3.3 V、安全启动/Flash 加密关闭。原 v6.1 DIO/80 MHz bootloader 未写入。实际分区仍为 NVS 0x9000/0x5000、PHY 0xe000/0x1000、factory 0x10000/0x400000、FAT 0x410000/0x500000。

已部署镜像为上一节图案 app：288,224 字节，SHA256 `ef3925f4f7b1ed7348b7bb844b4a3ed70c2f14b1bb97cb8c8c7f8e271e28aff5`，ELF `535c2f310ee63c1b9e7c6694729ecf2c4913cdf26315f4a8cb6262b88c9c9594`，当时 sdkconfig `804fd90ecdaefc95dcb88c5e7e9eb2a79c939ccfbb4357804ff72eff7e7422cf`。ROM/no-stub 写入 `[0x10000,0x565e0)`、擦除 `[0x10000,0x57000)`，Hash verified；应用全长回读 cmp/SHA 一致。分区表前后各 4096 字节 SHA256 同为 `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`，生成 3072 字节表及 FF padding 与实板匹配。没有 bootloader/分区/NVS/PHY/FAT 写入，不将范围隔离当作所有数据完整校验。未做完整备份。

### 一次清白、人工参考、一次失败与退出

启动 IDF v6.1、PMU 1.0.9、VCOM valid/1580 mV、faults=0x1，等待指令，不自动启屏。一次 clear-white：DISPLAY_INIT ESP_OK/98 ms，CLEAR_RETURN ESP_OK/13,145 ms（含初始化），退出 input0/out0=0x81。初始化前 internal_free/largest=334,231/241,664，PSRAM free=8,384,996；初始化/清白后为 277,323/200,704，PSRAM=6,682,524。

用户本轮回复“整屏白色，无明显异常”。同一 ESP 启动内显式 white-confirm 接受、REFERENCE_KNOWN；随后仅一次 gc16-corners：

| 观测 | 结果 |
| --- | --- |
| 输入帧分配后 PSRAM free | 6,264,728，释放后恢复 6,682,524；内部 free/largest 不变 |
| gate | STATUS 首读 CRC 拒绝，最多 3 次的有界重读后有效；GET 成功，PMU VCOM=1580 mV、实时 SY 温度原始解码=0°C |
| renderer | `line buffer underrun occurred!`；diff 37 ms、draw 140 ms、buffer update 18 ms、total 195 ms |
| GC16_RETURN | completed=0、stage=6、frame_error=0、operation=1024（0x400）、cleanup=0、elapsed=922 ms、faulted=1 |
| 独立退出观察 | ESP_OK，input0/out0=0x81；源帧释放 |
| 会话 | FAULT_LATCHED，drawn/visually_confirmed=0，未发送 pattern-confirm/其他图案 |
| 显式 stop/status | 观测 LCD deinitialized；随后 status 接受、STOPPED/operation=0、pending invalid、两个验收掩码为 0 |

USB 日志没有各条 DISPLAY_DEINIT/HAL_DEINIT ESP_OK，不能补造；成功终止依据随后 STOPPED/operation=0 及 stop callback 必须返回 0 的契约。控制台 Ctrl-C 关闭 exit 0，无后续复位/刷写/清白/GC16。失败后的物理四角图未知；即使出现部分图案也不计成功，HL 失败后会改 back_fb，逻辑参考必须作废。

固定 EPDiy `epdiy.h` 明确 0x400 为 EPD_DRAW_EMPTY_LINE_QUEUE（波形查表生产未跟上扫描），与 renderer 报错一致；不是帧格式 gate 或清理错误。原始温度 0°C 在允许数字范围，不等于环境温度已校验，不推断它造成欠载；PMU faults=0x1 仍为 BOOT_WAIT timeout 黏滞标志，未 HOST_READY/清除。CRC 这次有界重读恢复不能证明所有 CRC 根因已解决。

### 离线候选与回归

对照固定厂家 sdkconfig：部署 app 为 DEBUG(-Og)/32 KB 数据缓存/32 B 行；厂家为 PERF(-O2)/64 KB/64 B，同时使用产品 120 MHz/HPM。候选只对显式显示 overlay 对齐前三项，保留 Flash/PSRAM 80 MHz、12 MHz/FULL 扫描/32 行预填充、1K LUT、原波形/leading-skip=false；不改厂家源码、不调校准。不宣称这就是唯一根因或已修复，80 MHz 吞吐/调度等仍需实板证据。

显式迁移本地忽略 sdkconfig 的三个既有选择；入口编译保护拒绝陈旧显示配置。首轮 -O2 构建暴露自身 `first_image` 的 strncpy truncation 编译错误，以同容量 snprintf 保证 NUL 终止，不关闭警告。新增 511 字节最长适配路径的顺序 wrap 用例，确认传入解码及链接路径完整；现有 current_image/last_image 63/255 字符元数据限制未扩展，P3 仍需统一路径容量。新增测试最初误期望元数据完整而失败，修正为验证实际契约后全套复跑通过；不掩盖初次失败。

| 最终检查 | 结果 |
| --- | --- |
| CTest | 247/247，exit 0，新增最长 wrap 路径 1 项 |
| ASan/UBSan 子集 | 110/110，exit 0，不等于全部 247 项覆盖 |
| Python 构建保护/假串口 | 各 8/8，通过；新增 profile 与不升频/不校准保护 |
| 固定 SDK v6.1 四配置 | 显示候选、原板、双 OTA 只读、factory 只读编译均 exit 0；没有执行打印的完整烧写命令 |
| 镜像 | ESP32-S3/v6.1/DIO/16 MB/80 MHz/64 KB MMU，checksum/hash valid |
| 隔离/静态 | 显示 ELF 有原生显示/图案入口，无已定义 NVS init/erase、SD/Wi-Fi init、Flash coredump write；clang-format 20 dry-run、git diff --check 通过，第三方 clean |

方向/Web 沿用此前 6/60，未复跑完整基线脚本或远端 CI；原有字体/Web 告警未作为新问题消除。

### 最新本地产物（全部未部署）

| 产物 | SHA256 |
| --- | --- |
| build-read-pico-display/esp32-photoframe.bin（275,680 字节） | `7ced0efd9feb7d84699d5a5c4e4a623165634707bf4f8cb1f9a943dfdc4cee80` |
| 显示 ELF | `7bf59f2c4be0edcfc822250f284a1111c719a3e59ae3accf57d598eed3f6d702` |
| .cache/sdkconfig.read_pico-display | `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5` |
| build/esp32-photoframe.bin（原板） | `3e11ab3bf9b9abb69ed1740626d79d8898d4f13052e4e3a4f158bde620ed17a3` |
| build-read-pico/esp32-photoframe.bin（双 OTA 只读） | `df749d1fbf655c8481833826b300ce954c2e6e07ccc27d60ae33b326f46f3d49` |
| build-read-pico-factory/esp32-photoframe.bin（factory 只读） | `5231430edf9c9ecf1ae5916a10af1ce539687b71e439ad7a57121f08ab7959fc` |

候选拟写 `[0x10000,0x534e0)`、擦除 `[0x10000,0x54000)`，尚未执行。下一步安排候选 app-only 受控验证、应用回读/分区复核、重新清白/人工全白确认、单张四角刷新及人工观察；故障即停，不自动升频或刷白。P2 未完成，完整业务/OTA 迁移与 GR 不启动。

私有硬件日志 `.cache/gc16-device-*-private.log`、`.cache/gc16-app-*-private.log`、二进制 `.cache/gc16-*.bin`；离线最终日志 `.cache/gc16-perf-*-final.log` 及四配置 `.cache/gc16-perf-*-build.log` 均忽略不提交，避免泄露设备身份。该阶段没有提交/推送、校准写、第三方修改或失败后硬件重试；后续另获准候选测试见下节。

## 2026-10-03 · 性能候选上板；清白后边缘黑线，未登记参考，已退出

### 授权、镜像及保留范围

用户另行“确认”允许仅 factory 更新性能候选、重新清白/人工确认后单张四角复测，仍不备份完整 Flash。没有新增校准/波形/120 MHz/完整迁移权限。本轮没有代码改变或重新编译，沿用前节 247 CTest/110 sanitizer/8 构建保护/8 控制台/四配置成功结果，非再次执行计数。

候选 app 275,680 字节，SHA256 `7ced0efd9feb7d84699d5a5c4e4a623165634707bf4f8cb1f9a943dfdc4cee80`，ELF `7bf59f2c4be0edcfc822250f284a1111c719a3e59ae3accf57d598eed3f6d702`，sdkconfig `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`。image-info checksum/hash valid，ESP32-S3/v6.1/DIO/16 MB/80 MHz/64 KB MMU。重新核对同一设备（身份私有比对）、v0.2/8 MB AP PSRAM、Flash 0x5e/0x4018/16 MB/3.3 V、Secure Boot/Encryption Disabled、CryptCount=0。前置实际表与既有实板/生成表及 FF padding 一致。

ROM/no-stub 仅写 `[0x10000,0x534e0)`，擦除 `[0x10000,0x54000)`；日志 Hash verified，写入 4.1 秒。全长回读 SHA/字节一致；前后 4096 字节分区表一致，SHA `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。当前设备即此候选，前轮 288,224 字节 DEBUG 图案 app 已覆盖；不把前轮摘要当当前应用。未写 bootloader/分区表/NVS/PHY/FAT，不将区域保留当作全部数据完整校验。

### 本次启动与一次清白

显式 ESP reset 后 ELF prefix `7bf59f2c4`，实机 `Render profile: PERF dcache_bytes=65536 line_bytes=64`；PSRAM 80 MHz、LCD 12 MHz/FULL，原波形未变。PMU 1.0.9、VCOM valid/1580 mV、faults=0x1；STATUS 首读 CRC 拒绝后有界重读有效。部分启动 WAIT 提示未被 USB 日志捕获，显式 status 得到 WAIT_CLEAR_COMMAND/operation=0 后才发清白，不将未捕获提示当作初始化失败或伪造它已出现。

仅一次 clear-white：

| 观测 | 结果 |
| --- | --- |
| 初始化前内部 free/largest，PSRAM free | 306,579 / 245,760；8,384,996 字节 |
| DISPLAY_INIT | ESP_OK，80 ms |
| 初始化/清白后内部 free/largest，PSRAM free | 249,671 / 204,800；6,682,524 字节 |
| CLEAR_RETURN | ESP_OK，14,332 ms（含初始化），faulted=0 |
| 退出观察 | ESP_OK，input0/out0=0x81 |
| 会话 | WAIT_WHITE_CONFIRM/operation=0、pending invalid、drawn/visually_confirmed=0 |

用户回复“有一边有黑色线条”并附照片；照片可见下沿深色窄线，中央大面积浅色，但受边框/阴影/反光及拍摄角度影响，不能定位到最后几行像素或判断残影/硬件损坏。**本轮白参考未验收**，没有 white-confirm、pattern-confirm 或任何 gc16-*；前轮已确认白屏不能代替本次观察。

收到异常反馈后显式 stop；本轮完整观测 `LCD deinitialized`、DISPLAY_DEINIT ESP_OK、HAL_DEINIT ESP_OK、STOPPED/operation=0，两验收掩码为 0。随后显式 status 再确认 STOPPED；Ctrl-C 关闭控制台 exit 0，串口已释放。没有自动清白/重试/升频、复位或再次刷写，当前无已知白参考。性能候选 GC16 欠载是否恢复仍未知，不能计通过。

### 只读排查与人工关口

fast-context 因缺少 Windsurf API key 不可用，未提取密钥，回退 rg/本地固定源码。外层 `read_pico_epaper_clear_unverified` 调用厂家 epd_clear；后者使用 epd_full_screen（0,0,实际宽,实际高）。LCD 清白使用静态行填充，区域条件 `y <= line < y+height`，不经过 GC16 波形 LUT/行队列生产；没有发现外层主动裁掉底行的证据，不能把前轮 0x400 直接解释成本轮黑线。void 清白没有可传播的完整扫描结果，软件 ESP_OK 只证明电源/退出检查，不能保证全部像素已清白。

下一步需要用户确认照片中深色线是否位于可显示像素内、在首次固件操作前是否存在，并提供更近的下沿照片（尽量正对、避免反光，可用手指在边框外指示，不按压屏面）。若是物理边界，需明确再决定重新建立参考；若是有效区残留，需制定新的受控恢复验证，不凭现有照片自动改扫描/波形/校准或重刷。P2/完整业务继续未验收，GR 后置。

忽略私有资料：`.cache/gc16-perf-device-*-private.log`、`.cache/gc16-perf-app-*-private.log`、`.cache/gc16-perf-partition-*-private.log`、对应 `.bin`。照片保留会话附件，不复制进仓库/不公开。无提交/推送/凭据输出/厂家源码改动。

## 2026-10-03 · 新近照及刷写前历史确认：按下沿新黑带异常处理

用户明确回复“刷固件前不存在”，并提供第二张更正面的近照；下沿有连续深色窄带，中央大面积浅色。结合用户历史反馈，不能再按正常已有边框/仅阴影解释而放行；本次清白实屏不通过。照片不能精确数出异常行，也不能确定永久损坏、可恢复残影或具体扫描机制；没有要求拆机/按压屏幕。前轮人工全白反馈按历史保留，不由本轮照片反推前轮已存在黑带。

设备沿用前节候选及 STOPPED 状态。本次没有打开串口、读取设备、复位、刷写、清白或 GC16，也没有代码改动/编译/测试重跑。仅只读源码与公开资料检索、更新 TODO/状态；此前 247/110/8/8 和四配置结果不新增计数。

fast-context 仍缺 Windsurf key，未提取密钥，回退 rg；未匹配的猜测文件路径已改为以 rg 实际定位，未把路径错误当作源码缺失结论。固定源码核对：

- E0470_DISPLAY 实际宽/高为 1216/684；外层 clear 调用 full_screen，不主动裁掉底边，静态 clear 行选择为 `[0,684)`。
- LCD `assign_lcd_parameters_from_config` 将高向上补齐到 8 的倍数，即 688；这是 DMA/bounce 缓冲设计，不是“刚好漏 4 行”的证据。GC16 也使用相同向上补齐，不直接关联前轮欠载与这条新黑带。
- `epd_lcd_start_frame` / VSYNC ISR 控制最后一批/帧结束，清白没有可传播的完整扫描状态；应优先核验有效末行、填充行与帧结束/电源关闭的对齐，而非改图片旋转/nibble 或提升清白次数。
- 厂家 board_poweroff 在撤 XOE 后有显式 1 ms 延时，外层 stop_rails 没有该额外延时，但双方 SY shutdown 均有 500 ms 序列；该差异尚未证明会产生黑带，实际 I2C/CKV/STV/高压轨先后需测量，未据此擅自改变时序。

公开核对 [固定 LCD 驱动](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/epdiy/src/output_lcd/lcd_driver.c) 与本地固定源码一致；官方入口/仓库检索没有得到可直接解释本机黑带并已验证适用的修复，不用不相关的其他 Pico/LCD 结果套用。厂家 ONBOARDING 中 PAGE/FULL 留边及欠载恢复提示属于 Demo 场景；本次为 full_screen 静态清白，且本项目禁止自动降频/刷白，不照搬其恢复策略。

下一步为补可测试的末行/帧完成观察及退出契约核验，先在外层诊断/适配层寻求证据，固定厂家驱动/波形/校准不直接修改。新增镜像或时序/恢复动作需重新核验范围并安排受控硬件验证；已停止的会话不可继续用旧确认。没有成立的白参考前不 GC16，不通过反复刷新掩盖新异常。

收尾：22 份当前/索引文档的 102 个本地链接可解析，CI YAML 与四配置命令检查通过；新生成 3072 字节分区表与前轮实板 4096 字节记录的前缀及 FF 填充相符（不是本轮设备回读）。最终构建后再次核对 image-info/ELF 入口及摘要；子模块工作树干净，新增 C/C++ 格式与 git diff --check 通过。

收尾：21 份当前入口/索引文档的 82 个本地链接可解析，git diff --check、CI YAML 解析及最终 5 项构建保护测试通过，厂家子模块工作树干净。原板/默认诊断 app 摘要仍与 P2b 一致；当前设备 app 摘要与 factory 镜像一致。没有更换设备正在运行的诊断版本。

## 2026-10-03 · 静态清白扫描诊断：离线实现与验证，未上板

### 变更与观测范围

新增便携 `read_pico_scan_trace` 分析与显示 profile 专用 `read_pico_scan_probe` 注册 link-wrap；固定 PhotoFrame/Read Pico/SDK commit 沿用。仅显式 clear-white 开始记录，保留原回调/参数/返回值；帧完成原回调在临界区外执行，可以注销注册。IRAM 回调写内部 DRAM，不打印/分配/IO/延时，不改电源、频率、扫描参数、波形或校准。

逐帧保存 source 回调数量与逻辑 680–687 行的首/尾字节，固定静态 clear 预期 78 帧（3×10 dark/13 white/3 no-op）。分析要求有效末行对应 0x55/0xaa/0x00 脉冲、补齐行 0、首尾相等，注册/完成/保存数一致且无观测故障；软件检查失败则会话失败锁存，不允许 white-confirm。退出 IOE 只读检查在逐帧日志之前进行。最多存 96 帧，溢出显式计数；不依赖动态分配。

这不是黑带修复或根因结论：仅采样每行首/尾，不观测中间字节、电气 DMA 完成、CKV/STV、高压轨或屏上像素。回调可能预取到 688 行以后，不能由计数推断送屏行数。观察器两份 2336 字节 capture 和状态约增加 4.8 KB 内部静态数据，同时增加回调开销；真实时序影响未测，软件 coverage_complete 不代替人工确认。

### 主机、保护与固件编译

| 检查 | 本轮结果 |
| --- | --- |
| cmake --build host_tests/build；ctest --test-dir host_tests/build --output-on-failure | 260/260，exit 0；新增扫描观察/分析 13 项 |
| ASan+UBSan 对应 Read Pico/业务/图案/扫描子集 | 123/123，exit 0；命令见 DEVELOPMENT |
| python3 host_tests/test_read_pico_build.py | 9/9，exit 0；含显示 profile 专用链接、begin/clear/end 顺序与厂家 78 帧/脉冲常量保护 |
| python3 host_tests/test_read_pico_console.py | 8/8，exit 0；没有连接真实串口 |
| 固定 v6.1 的原板、双 OTA 只读、factory 只读、factory 显示 build.py --step firmware | 四配置 exit 0；仅编译，未 flash/monitor |

13 项扫描用例覆盖原回调原样转发、busy/错误核心、完整 78 帧/返回值、缺末行/未完成、96 帧边界、源回调期间 frame done、孤立/迟到、冻结快照、坏参数、填充/首尾/缺样本、缺帧/多帧/所有计数故障、覆盖未完成注册，以及“白相位末行输出 no-op，虽有全部回调也拒绝通过”。模拟不是实际 RTOS/DMA 验收。

本轮未重跑方向/Web，沿用 6/60；远端 CI、120 MHz/HPM、完整业务仍未验证。首次回归误用了未注册板名 `waveshare_7in5_v2`，构建脚本参数校验 exit 2，未启动硬件操作；改用已注册 `waveshare_photopainter_73` 后四配置通过，未改板列表绕过。增量编译没有新增源码编译告警；不宣称固定第三方/旧配置所有告警消失。

### 新候选产物与链接核验

- `build-read-pico-display/esp32-photoframe.bin`：282,896 字节，SHA256 `89c5f1739ed58bbb6c9130793b1e9a0e3d0333a4da5dbb6407f6f3a8272bd861`。
- ELF SHA256 `7717b54c8fb5b682af709ae428b4a3b5a0f94fbc162d2df68a9c7e27ae67902a`；sdkconfig SHA256 `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`。
- image-info：ESP32-S3/v6.1、DIO/16 MB/80 MHz、64 KB MMU、checksum/hash valid。实际 epd_push_pixels_lcd 反汇编引用两注册 wrapper；wrapper/traced_line/traced_done 在 IRAM，capture/frozen/current 在内部 DRAM；-O2 优化的短复制调用 ROM memcpy（0x400011f4），不调用 flash memcpy。
- 普通 factory ELF 没有 scan probe/wrapper 定义；显示 ELF 无 nvs_flash_init/erase、sdmmc_card_init、esp_wifi_init 或 esp_core_dump_write 定义。未改 bootloader/分区文件，未在设备执行上述操作。
- 原板应用 SHA256 `3e11ab3bf9b9abb69ed1740626d79d8898d4f13052e4e3a4f158bde620ed17a3`；双 OTA 只读 `df749d1fbf655c8481833826b300ce954c2e6e07ccc27d60ae33b326f46f3d49`；factory 只读 `5231430edf9c9ecf1ae5916a10af1ce539687b71e439ad7a57121f08ab7959fc`。三者均与扫描诊断加入前一致。

### 设备状态与下一关口

本轮未打开串口、读取设备、复位、烧写、清白或 GC16；没有新的应用回读或实屏扫描数据。设备仍为 275,680 字节性能候选 `7ced0efd9feb7d84699d5a5c4e4a623165634707bf4f8cb1f9a943dfdc4cee80`、STOPPED、无已确认白参考，下沿新黑带仍未解决。

下一步需针对已出现的新异常明确确认：核验兼容性后 app-only 更新新诊断候选，重新回读/比对分区，显式一次 clear-white 后人工观察；不连带 GC16，不批量确认，不自动重试。若仍有黑带或软件记录不完整则 stop；不能把此前授权/旧白确认跨镜像复用。若仅凭软件证据无法定位，需进一步人工电气测量。用户仍不要求备份，不承诺原应用完整回滚。

本地日志 `.cache/scan-probe-*-final.log`、各配置构建日志、build-guards/console-tests 和 ISR 反汇编均被忽略。厂家子模块工作树干净，无提交/推送或凭据输出。

收尾检查：21 份当前入口/文档的 85 个本地链接可解析，CI YAML 解析成功；新增/修改扫描 C/C++ 的 clang-format --dry-run --Werror、git diff --check 均 exit 0，厂家子模块仍干净。

## 2026-10-03 · 扫描诊断 factory 上板：单次清白及人工全白通过

### 本轮许可与身份、布局核验

用户直接回复 `yunxu`，按“允许”上一条所列 factory app-only 更新及一次带诊断清白处理；用户仍放弃完整备份，不扩展到 GC16、完整迁移、波形/频率调整或校准写。串口开始前无其他占用。ROM/no-stub 重新读取安全信息、Flash ID 和 4096 字节分区，MAC 仅私有比对与前轮设备相同，不输出识别值。

实机 ESP32-S3 QFN56 v0.2/8 MB AP_3v3 PSRAM；Flash 5e/4018、16 MB、3.3 V；Secure Boot/Flash Encryption disabled，SPI_BOOT_CRYPT_CNT=0。实际表校验通过：NVS 0x9000/0x5000、PHY 0xe000/0x1000、factory 0x10000/0x400000、FAT 0x410000/0x500000；与候选生成表及 FF 填充一致，4096 字节 SHA 与前轮相同。候选应用/ELF/config 摘要仍与上节一致，没有重新构建或修改代码。

### 仅应用烧写与回读

`esptool 5.4.0 --chip esp32s3 --port /dev/cu.usbmodem2101 --baud 115200 --no-stub --before no-reset --after no-reset write-flash --flash-mode dio --flash-size 16MB --flash-freq 80m 0x10000 build-read-pico-display/esp32-photoframe.bin`，只有一个 app 目标，无全区擦除/flash_args。首次安全读取显式进入 ROM 下载态，其他核验/写入/回读保持下载态，回读完成后由控制台显式 USB ESP reset 启动。

| 项目 | 本轮结果 |
| --- | --- |
| 应用大小与 SHA256 | 282,896 字节；`89c5f1739ed58bbb6c9130793b1e9a0e3d0333a4da5dbb6407f6f3a8272bd861` |
| ELF / sdkconfig SHA256 | `7717b54c8fb5b682af709ae428b4a3b5a0f94fbc162d2df68a9c7e27ae67902a` / `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5` |
| 写入 / 擦除范围 | `[0x10000,0x55110)` / `[0x10000,0x56000)`，均在 factory 内 |
| 写入校验 | 4.0 秒，Hash of data verified |
| 全长回读 | 282,896 字节逐字节 cmp 一致，SHA 同候选 |
| 分区表前后 | 4096 字节 cmp 一致，SHA 均 `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20` |

当前设备是本节扫描诊断镜像，历史 275,680 字节性能候选已覆盖。不写 bootloader/分区表/NVS/PHY/FAT，不把保留区域当作全部内容逐字节完整校验；没有原 app 完整回滚保证。未备份原 app，回读副本是验证新 app。

### 新启动与唯一一次 clear-white

新启动 ELF prefix `7717b54c8`、IDF v6.1、PERF/65536/64；PSRAM memory test OK。PMU 1.0.9、VCOM_valid=1/1580 mV、faults=0x1（沿用已记录的 BOOT_WAIT sticky，未清除），本轮启动日志未出现 CRC 拒绝。启动布局/内存与 PMU 门禁通过，未在 boot 自动启屏。显式 status 接受 WAIT_CLEAR_COMMAND/operation=0 后，仅一次 clear-white。

| 观测 | 本轮结果 |
| --- | --- |
| 初始化前内部 free/largest、PSRAM free | 300,819 / 241,664；8,384,996 字节 |
| DISPLAY_INIT | ESP_OK，80 ms |
| 初始化及清白后内部 free/largest、PSRAM free | 243,911 / 196,608；6,682,524 字节 |
| 退出观察 | ESP_OK，input0=0x81、out0=0x81 |
| 帧注册 / 完成 / 保存 | 78 / 78 / 78，coverage_complete=1、software_only=1 |
| 每帧 source 回调数量 / seen | min=max=696；78 帧全部 696 / 0xff |
| 末行及填充 | 680–683 行首尾逐相位匹配 0x55/0xaa/0x00；684–687 行首尾全 0 |
| 观测异常 | missing_tail/missing_padding/padding_nonzero/unequal_edges/unexpected_pulse/dropped/orphan/late/unfinished/wrong_core 全 0 |
| CLEAR_RETURN | ESP_OK，14,018 ms（含初始化，不含后续逐帧打印），faulted=0 |
| 清白后会话 | WAIT_WHITE_CONFIRM/operation=0、pending invalid、drawn/visually_confirmed=0 |

独立解析私有日志，确认 78 条 CLEAR_FRAME 连续 index=0..77、各周期脉冲及零填充完全匹配。696 是预取软件回调数量，不是声称实际传输了 696 行。未采样每行中间字节/CKV/STV/高压轨；观测器内部内存与时序开销仍未量化，不能用本轮完整记录证明黑带因果。

### 人工实屏与正常退出

用户针对本轮回复“黑带消失，整屏白色无明显异常”。该观察计为本轮单次全白通过，不倒改前轮新黑带失败，也不代表长期稳定/永久修复。收到人工回复后同启动显式 white-confirm，REFERENCE_KNOWN/operation=0 接受；该确认仅更新 RAM 参考，没有刷新或校准写。

随后 stop：LCD deinitialized、DISPLAY_DEINIT ESP_OK、HAL_DEINIT ESP_OK、STOPPED/operation=0、pending invalid、drawn/visually_confirmed=0。再发 status，日志确认 STOPPED/operation=0；末尾 visually_confirmed 字段在关闭控制台前未完整捕获，完整掩码以此前 stop 日志为依据，不补造字段。Ctrl-C exit 0、lsof 无占用，串口释放；参考随 deinit 清除，不能跨会话复用。

全过程命令仅 status、一次 clear-white、收到人工回复后的 white-confirm、stop、status；没有 gc16-*、pattern-confirm/reject、自动清白/重试、降频或额外波形/电源调整。前轮 0x400 恢复仍未知。黑带消失可能与累计清白、观察器开销或其他因素有关，尚无证据区分；不宣称面板永久损坏或已确定根因。

本轮没有代码修改、测试/编译重跑；沿用上节同一产物的 260/123/9/8/四配置结果。fast-context 仍缺 Windsurf key，未提取密钥，回退 rg 核对实际 confirm/deinit 源码；猜测路径未找到不当作接口证据，确认函数以真实 driver_read_pico.c 为准。

当前安全停止。下一步需另行确认单张四角 GC16 复测；新会话必须重新清白/人工确认，不因本轮全白直接跨会话放行，不自动连刷。完整相框/产品时序/长期运行及 GR 均未验收。

私有日志/回读：`.cache/scan-probe-device-*-private.log`、`.cache/scan-probe-app-*-private.log`/readback.bin、`.cache/scan-probe-partition-*-private.log`/before.bin/after.bin；不入库、不公开 MAC，无提交/推送、厂家源码或校准改动。

收尾：应用与回读摘要再次一致，实际命令序列及 stop/控制台退出日志独立核对通过，私有日志/二进制均命中 ignore；21 份当前文档 85 个本地链接可解析、CI YAML 解析通过、git diff --check 通过，厂家子模块干净。

## 2026-10-03 · 已获单张四角 GC16 复测许可：新启动清白通过，待本轮人工确认

用户直接回复“允许”新启动清白、人工确认后仅一张四角 GC16 的请求。没有烧写/擦除/校准写、代码变更或编译测试重跑；当前应用/上轮回读 SHA 均仍 `89c5f1739ed58bbb6c9130793b1e9a0e3d0333a4da5dbb6407f6f3a8272bd861`，同一 282,896 字节扫描诊断镜像。串口无占用后显式控制台 `--reset` 新启动，ELF prefix `7717b54c8`、IDF v6.1、PERF/65536/64、PSRAM memory test OK、PMU 1.0.9/VCOM valid/1580 mV、faults=0x1。启动布局/PMU 门禁通过，启动不自动启屏。

显式 status=WAIT_CLEAR_COMMAND/operation=0 后仅一次 clear-white。DISPLAY_INIT ESP_OK/80 ms；初始化前内部 free/largest=300,819/241,664、PSRAM=8,384,996，初始化/清白后=243,911/196,608、PSRAM=6,682,524。CLEAR_RETURN ESP_OK/14,018 ms/faulted=0，退出 input0/out0=0x81。

本轮 registered/done/stored=78/78/78、coverage_complete=1/software_only=1；每帧 696 次 source 回调、seen=0xff，680–683 首尾符合 dark/white/no-op，684–687 首尾全 0，全部观测异常计数 0。独立解析本轮 78 条逐帧日志再次核对通过；软件回调/预取不等于实际送屏证明，观测开销未测。

目前仅发送 status、一次 clear-white；WAIT_WHITE_CONFIRM/operation=0、pending invalid、drawn/visually_confirmed=0。已询问本轮实屏是否全白无黑带/灰条/残影，等待人工回复；尚未 white-confirm 或 gc16-corners，不沿用上轮已经清除的参考。串口保持连接，不再复位/刷新。人工确认全白才执行获准的一张四角；异常则 stop，无自动重试/批量图案。

私有日志 `.cache/gc16-scan-retest-device-console-private.log`，当前控制台会话 9277 保持运行以承接本次人工关口。授权、软件清白、人工白参考、GC16 返回、图案实屏及退出分别记录，不预填未发生结果。

## 2026-10-03 · 本轮单张四角 GC16 软件完成，待图案人工确认

用户回复本轮重新清白“整屏白色，无明显异常”。仍在上节同一启动，串口由原控制台持有，没有断电/复位/重连/新烧写。显式 white-confirm 接受 REFERENCE_KNOWN/operation=0 后，仅发送一次获准的 gc16-corners。

| 观测 | 本轮结果 |
| --- | --- |
| 刷新前内部 free/largest、PSRAM free | 243,911 / 196,608；6,682,524 字节 |
| 分配标准 4bpp 图案后 PSRAM free | 6,264,728 字节，减少 415,872 |
| GC16_RETURN | corners、completed=1、stage=0、frame_error=0、operation=0、cleanup=0、elapsed_ms=2140、faulted=0 |
| 退出观察 | ESP_OK，input0/out0=0x81 |
| 图案释放后内存 | 内部 243,911 / 196,608；PSRAM 6,682,524，回到图案前读数 |
| 会话 | WAIT_PATTERN_CONFIRM/operation=0、pending=corners、drawn=0x20、visually_confirmed=0 |

独立核对日志仅一条 GC16_BEGIN，命令序列为 status/clear-white/white-confirm/gc16-corners，无 pattern-confirm/reject、其他 GC16 或重试。扫描观察器只包围此前静态清白，不在 GC16 feeder 记录；未改 PERF/64 KB/64 B、80 MHz/12 MHz FULL/1K LUT/原扫描/波形/校准。软件 completed/退出成功不代替像素方向、灰阶或残影验收。本次没有复现历史 0x400，仅计单次结果，不证明根因或长期稳定。

fast-context 仍无 Windsurf key，未提取密钥，回退读取实际 read_pico_pattern.c：本图为白底/外围 2px 黑框、TL/TR/BL/BR 及 1/2/3/4 点。细黑框是图案定义，不与历史清白粗黑带混同；多余粗黑带/灰条/残影仍属异常。已请求本轮四角位置/清晰度及异常的人工观察，可附照片；尚未收到图案验收，未 pattern-confirm/reject，串口保持，暂不复位/断电或发送其他图案。

## 2026-10-03 · 四角图人工验收通过，正常停止与串口关闭

用户针对上节唯一四角图回复“四角位置正确，无明显异常”。按本次人工反馈显式 pattern-confirm，REFERENCE_KNOWN/operation=0、pending invalid、drawn=0x20/visually_confirmed=0x20 接受；确认只登记验收位，无刷新/新白参考/校准写。该结果计本轮单张四角方向/标记/可见异常检查通过，非 16 阶灰阶、像素或照片效果验收。

随后显式 stop：LCD deinitialized、DISPLAY_DEINIT ESP_OK、HAL_DEINIT ESP_OK、STOPPED/operation=0、pending invalid、drawn/visually_confirmed=0x20。再发 status，**完整**收到相同 STOPPED/operation=0/两掩码=0x20；确认完整日志后 Ctrl-C exit 0，lsof 无占用，串口关闭。白参考随 deinit 清除，掩码仅是会话历史，不能作下一启动参考；不自动补刷全白，所以物理屏保留本次四角图。

独立解析日志确认全部命令恰为 status → clear-white → 人工全白后的 white-confirm → 一次 gc16-corners → 人工图案通过后的 pattern-confirm → stop → status；仅一条 GC16_BEGIN，无其他图案/重试/复位/烧写/校准写。本次 2.140 秒软件/退出/实屏均通过，没有复现历史 0x400；不以此断言欠载根因已修复或连续稳定。软件/配置同前节，厂家代码/波形/校准与 80 MHz 不变，未重新编译或运行主机测试。

P2 下一项为 16 阶灰阶单张验收，需另行允许新启动清白及人工确认后测试；后续仍保留逐张目视关口，错误/拒绝停止，不因本次四角通过批量放行。完整照片/相册/产品时序与长期运行未完成，GR 后置。

收尾：完整实际命令序列、单张 GC16 返回、人工验收位与 STOPPED/控制台关闭独立核对通过；21 份当前入口/文档 85 个本地链接可解析、CI YAML 解析与 git diff --check 通过，厂家子模块干净、私有日志被忽略，候选应用摘要仍与已回读副本一致。未提交/推送或公开设备标识。

## 2026-10-03 · 已获单张 16 阶灰阶许可：本轮清白软件通过，待人工全白

用户直接回复“允许”重新清白、人工确认后仅一张 16 阶灰阶的请求。串口开始前无占用，本轮不刷写/改代码/调参/校准写/重跑测试编译。应用与已回读副本 SHA 均仍 `89c5f1739ed58bbb6c9130793b1e9a0e3d0333a4da5dbb6407f6f3a8272bd861`（282,896 字节）。控制台显式 `--reset` 新启动，ELF prefix `7717b54c8`、IDF v6.1、PERF/65536/64、PSRAM memory test OK、PMU 1.0.9/VCOM valid/1580 mV、faults=0x1；启动门禁通过，boot 不自动启屏。

显式 status=WAIT_CLEAR_COMMAND/operation=0 后一次 clear-white：DISPLAY_INIT ESP_OK/80 ms，CLEAR_RETURN ESP_OK/14,022 ms/faulted=0（含初始化，不含逐帧打印），退出 input0/out0=0x81。初始化前内部 free/largest=300,819/241,664、PSRAM=8,384,996，初始化/清白后=243,911/196,608、PSRAM=6,682,524。

registered/done/stored=78/78/78、coverage_complete=1/software_only=1；每帧 696 次 source 回调、seen=0xff、680–683 行首尾脉冲匹配各相位、684–687 填充首尾全 0，全部观测故障 0。独立解析 78 条逐帧日志通过；软件回调不是送屏电气证明，观测开销未知。

当前仅 status、一次 clear-white，WAIT_WHITE_CONFIRM/operation=0、pending invalid、drawn/visually_confirmed=0；已询问本轮全白/黑带/灰条/残影，等待人工回复。尚未 white-confirm/gc16-gray16，不跨会话沿用旧参考；本轮只允许一张灰阶，异常停止、不重试/其他图案。私有日志 `.cache/gc16-gray16-device-console-private.log`，控制台会话 93521 保持连接以承接人工关口。

## 2026-10-03 · 本轮单张 16 阶灰阶软件完成，待实屏确认

用户回复本轮清白“整屏白色，无明显异常”。同一启动显式 white-confirm 接受 REFERENCE_KNOWN/operation=0 后仅一次 gc16-gray16；没有新复位/烧写/调参/其他图案。

GC16_RETURN gray16 completed=1、stage/frame_error/operation/cleanup=0、elapsed_ms=2138、faulted=0；退出 input0/out0=0x81/ESP_OK。刷新前内部 free/largest=243,911/196,608、PSRAM=6,682,524，标准图案分配后 PSRAM=6,264,728，释放后全部回到刷新前读数。WAIT_PATTERN_CONFIRM/operation=0、pending gray16、drawn=0x8/visually_confirmed=0；只计软件/退出完成，不计实屏验收。

独立解析本轮日志：命令恰为 status/clear-white/white-confirm/gc16-gray16，一条 GC16_BEGIN gray16，两次 EXIT_OBSERVATION ESP_OK，无重试/其他 GC16/图案确认。本次未出现 0x400，不声明根因或连续稳定。

固定图案按 1216/16=76 像素分成 16 个等宽竖条，标准灰阶从左到右 0 黑→15 白；竖条本身不是异常条带。已请求黑到白顺序、灰阶区分及额外横条/粗黑带/残影的人工观察，可附照片。当前尚未图案人工确认/pattern-confirm/reject，串口会话 93521 保持，暂不复位/断电或发其他图案。

## 2026-10-03 · 灰阶实屏未通过：拒绝、停止与只读离线核对

上述等待阶段已结束。用户反馈“白色偏多”并附照片；照片可见深色集中一端，大段浅灰接近白色。按实屏未通过处理，不发送 pattern-confirm、不将 completed=1 等同灰阶正确。附件为私人资料，未复制进仓库；手持竖向照片不能作为方向错误证据，缺少编号/固定拍摄条件，不能推算全部灰码、反射率或本机实际可分辨级数。

### 拒绝与安全退出

同一启动发送 pattern-reject，FAULT_LATCHED/operation=0、pending gray16、drawn=0x8/visually_confirmed=0；视觉失败与 transport 错误分开。随后 stop：LCD deinit、DISPLAY_DEINIT/HAL_DEINIT ESP_OK；完整 status=STOPPED/operation=0、pending invalid、drawn=0x8/visually_confirmed=0。收到完整 status 后 Ctrl-C exit 0、lsof 无端口占用，串口关闭。白参考随退出清除，物理屏仍保留本次灰阶图；没有补刷全白。

独立私有日志核对命令恰为 `status → clear-white → white-confirm → gc16-gray16 → pattern-reject → stop → status`；唯一一条 GC16_BEGIN，无 pattern-confirm/其他图案/重试/校准写。本轮仅获准本张灰阶，没有追加硬件操作、刷写或调参；历史四角单次人工通过仍保留，不转移到本轮灰阶。私有日志 `.cache/gc16-gray16-device-console-private.log`，控制台会话 93521 已关闭。

### 只读代码与波形核对

fast-context 当前缺少服务 API key，不提取凭据，使用 rg/固定源码直接核对；未读取的接口不计验证。已核对本项目图案/转换、原生后端、SY 读取及固定厂家 highlevel/render/LUT/波形生成路径：

- gray16 每条 76 像素，标准码 0–15；实际转换 ROTATE_0/不反色、仅交换 nibble，没有 gamma/阈值/亮度压缩。
- 后端 `E0470_WAVEFORM` 默认 GC16 36 相、1K LUT、`epd_hl_update_screen_full(...MODE_GC16,temp)`；全帧差分打包，并未选 GRAY8/DU。未验证 ESP PIE 汇编的实际输出或电气扫描，所以不能宣称后端全链路已经排除故障。
- 厂家[固定头文件](https://github.com/MindReset/read_pico_firmware/blob/28cde682a4468a581c278761922724f57d976418/components/e0470_epaper_waveform/include/e0470_epaper_waveform.h) 声明完整 48 相 GC16/GL16 表约 11 级可分辨；默认裁剪保留灰阶尾段。此为厂家能力声明，非本机测量；16 输入码与 16 可见灰阶不能混同。没有据此降低原目标，也不能据此解释本机大段白偏。
- `live_SY_temperature=0°C` 是本轮实际 gate 日志，适配层读取 SY 寄存器 8；转换就绪/有效性未证明，不认定为环境温度。厂家波形仅一个 0–50°C 温区，0 和 25 不选不同表；该事实不证明温度正确/白偏因果，不用固定 25°C 替换。

### 本轮主机验证及边界

`ctest --test-dir host_tests/build --output-on-failure -R 'ReadPicoPattern|ReadPicoFrame'`：22/22、exit 0；日志 `.cache/gray16-frame-pattern-host-tests.log`。本轮重跑子集，不累加为新覆盖；完整 260/123 sanitizer/9 构建保护/8 控制台及四配置编译、方向/Web 6/60 沿用同产物历史结果，未再完整执行。

另在忽略目录用 `cc -std=c11 -Wall -Wextra` 编译未改的 `e0470_epaper_waveform.c` / `e0470_waveform_trim.c`，配合真实类型头与空 IO 头文件桩，运行纯 CPU 白参考 15 → 各目标 0–15 审计。默认 36 相与完整 48 相（仅离线对照、未选入设备）分别有 16 个不同时间有序的软件相位序列，所有动作值在 0–2；部分码的动作数量相同，但序列不同。序列数不等于光学级数，不能替代面板测量/厂家声明。程序/构建/结果分别在 `.cache/gray16-waveform-audit.c`、`.cache/gray16-waveform-audit-build.log`、`.cache/gray16-waveform-audit.log`，编译及执行 exit 0，没有修改厂家/生产源码。

当前 app 仍为 282,896 字节/SHA256 `89c5f1739ed58bbb6c9130793b1e9a0e3d0333a4da5dbb6407f6f3a8272bd861`，无固件代码变更、未重编译或刷写，80 MHz/12 MHz FULL/1K LUT/原始波形/PMU 校准不变。仅临时离线审计和文档修正，不改 VCOM、不切完整表/GRAY8、不升频，也未运行完整业务/GR。

下一步：先制定带编号、固定拍摄条件的灰码→可见级数/质量测量，以及 SY 温度转换有效性检查，补入 I14/I15 和 P2 TODO。需追加刷新/更新诊断固件/电气测量时，明确范围与人工介入；当前不继续标准图案序列或自动重试。原 16 灰阶需求保留，是否调整能力目标须由用户决定。

收尾：21 份当前入口/文档的 87 个本地链接、CI YAML 解析和 git diff --check 通过；厂家子模块固定 commit/工作树干净，私有日志及临时审计被忽略；本地 app 与既有回读副本均 282,896 字节且 SHA 一致。串口无占用，未提交/推送或公开设备标识/私人照片。

## 2026-10-03 · 主动温度采样与编号灰阶：离线实现及编译

用户要求继续开发。本轮仅源码/测试/文档与主机编译，不访问串口、不刷写/复位/显示；设备保持上一节 STOPPED。主工程/BSP/SDK 固定 commit 与配置沿用，厂家的驱动/时序/波形/校准工作树原样；未引入 GR、SD/完整业务或校准写。

### 确认的采样问题与实现

根据 [Silergy 编写的 SY7636A Rev.0.9B 数据手册（分销站公开副本）](https://uploadcdn.oneyac.com/attachments/files/brand_pdf/silergy/81/0E/SILERGY-SY7636ARMC.pdf) 第 4/12/21 页：EN 低会清报告寄存器，温度寄存器复位为 0，温度转换在 ON=1 工作。旧移植层在待机 ON=0 读寄存器 8 并赋温度有效，属于明确的采样状态问题，不能将历史 0°C 日志当环境温度。厂家板定义的温度入口也要求 rails_on。本轮检索 fast-context 因缺服务 key 不可用，使用 rg/直接读取固定源码；没有提取凭据。

便携事务新增可选 `ops.read_temperature`：NULL 保留旧预验证温度 gate；配置时先验证 PMU VCOM/输入帧，标准 prepare 成功后同锁主动采样，检查 0–50°C，再 draw/stop。回调 IO 错误、未写样本或越界返回独立 TEMPERATURE_FAILED，禁止 draw 并始终清理；两种错误分别保留。Read Pico 预检只检查待机 OFF/故障，原标准 power_on/PGOOD/5 ms 稳定后读取 ON=1 样本，另核验 ON/PG/故障及独立 PGOOD；异常锁存、清除参考、不重试。真实 active 0°C 不被武断拒绝；无固定 25°C、无新寄存器配置/单独探测上电。

手册转换时序参考与主动源状态不等于本机 NTC 测量已可靠；新读数/环境对照待实板验证。当前单温区仍相同，所以不宣称这个修正解释或改善了白偏，灰阶拒绝与原 16 灰阶目标保持。

### 编号图与验证

追加 `READ_PICO_PATTERN_GRAY16_LABELED` / `gc16-gray16-labeled`，原六图 ID/掩码保留，新图 0x40；既有白参考和逐张人工确认/拒绝门禁沿用，旧设备尚无新指令。中央 y=72–611 为 16 条各 76×540 像素标准灰码 0–15；上下白区各绘 00–15，短刻线只在白区，标签不改测量灰码、无 gamma。原未编号满屏图不变。

新增 16 项：7 个便携主动采样顺序/预检/失败/范围/未写样本/合法 0/50 用例，5 个原生 EN 低清温度→ON 后采样/IO失败/范围/合法0/PGOOD错误与清理用例，3 个编号图测量区/重复二进制编号/真实 nibble 转换用例，1 个编号图人工门禁/拒绝用例；现有 parser/控制台白名单和全图会话回归同步扩展。

| 验证 | 结果 | 日志 |
| --- | --- | --- |
| cmake --build host_tests/build -j 6；完整 CTest | 276/276，exit 0 | .cache/active-temperature-host-build.log / active-temperature-host-tests.log |
| build-asan 重编译；DEVELOPMENT 的原有八组筛选 | 139/139，exit 0；不是全部 276 项 | .cache/active-temperature-asan-build.log / active-temperature-asan-full-subset.log |
| python3 host_tests/test_read_pico_build.py / test_read_pico_console.py | 9/9、8/8，无硬件 | 本轮工具输出 |
| 固定 IDF v6.1 显示、factory 只读、双 OTA 只读、原板 --step firmware | 四配置编译成功，未 flash | .cache/active-temperature-{display-build-fixed,factory-build,readonly-build,original-board-build}.log |

第一次 SDK 激活未指定已配置的 Python 环境，IDF 在构建前以解释器不匹配退出 2；未 fullclean/擦除或改配置，显式激活原 Python 后重跑成功。完整测试脚本/Web/方向未重跑，既有 6/60 结果沿用；sanitizer 全构建出现上游 image_processor `%u`/size_t 警告，非本轮改动，不将其称为零警告。新核心/后端编译无报错。

临时主机预览直接调用同一 C 图案渲染器，以 code×17 写灰度 PNG，已目视核对标签/白区/刻线及 16 等宽条。`.cache/numbered-gray-preview.png` SHA256 `010871b91e4d960f60fe90c6d7a2b3cf41b4a0934cf4e663b427d2f855826243`；预览仅是输入码示意，不是面板反射率预测，未拷贝私人照片。

### 新候选身份与下一硬件关口

新显示 app 283,792 字节，SHA256 `03be87b6a65ffd555940a9beef36c33dd209f895940f9bc27ba539b8a8dc47a6`；ELF `0f69c9874b4c11792fbce4caa9459ef1c7e55e22dc17b49f9ccfdeba71c83d25`。sdkconfig SHA 仍 `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`，PERF/64 KB/64 B、80 MHz、12 MHz FULL、1K LUT/默认波形不变。factory/双 OTA 只读 app 最终摘要分别为 `d1c7c7c60e22af111dc624d0c926288b0ffebad472b7a311ab0d1349aa4f4e98` / `1d4912599ccaa4d57b1e5c2aa2d3bd031ec2e8ba4870539c01a0a446b4c7fc56`；原板 `3e11ab3bf9b9abb69ed1740626d79d8898d4f13052e4e3a4f158bde620ed17a3`，不能烧入 Pico。

设备没有更新，已回读的旧 app 仍 282,896 字节/89c5f173…；新本地构建不能复用旧回读/实屏结论。下一步需明确本轮 app-only 更新和单张编号图许可，先核验设备/分区/安全/兼容，保留 bootloader/分区/NVS/PHY/FAT，仅 factory 应用写入回读；新启动一次清白必须人工全白确认后才编号 GC16，错误立即拒绝/stop，不补刷。人工记录逐码/相邻区分、额外条带/残影，并对照环境温度与主动采样。新版同时改变采样状态和测试布局，只提供诊断证据，不是能隔离白偏因果的 A/B 实验；原 16 可见灰阶目标不自动降低。

## 2026-10-03 · 主动温度/编号灰阶上板：软件完成，等待人工观察

用户明确“允许”本轮 factory app-only 更新、新启动一次清白、人工全白确认后仅一张编号灰阶。沿用不完整备份决定，不扩大到分区迁移、bootloader/数据写入、波形/校准/频率调整或重复刷新。本轮无代码变更，不重新计主机测试或编译；沿用上述同 SHA 产物的 276/139/9/8 与四配置编译证据。

### 写入与独立回读

- 私下比较历史身份读数，核验仍为同设备；ESP32-S3 QFN56 v0.2、8 MB PSRAM，16 MB Flash 5e/4018/3.3 V；Secure Boot/Flash Encryption 禁用。公开记录不含 MAC/UID。
- 写前 4096 字节分区表与生成表填充 0xff 后一致：NVS 0x9000/0x5000、PHY 0xe000/0x1000、factory 0x10000/0x400000、FAT 0x410000/0x500000；各分区 flags=0。
- 镜像 S3/chip ID 9、revision/MMU 64 KB、DIO/80 MHz/16 MB、checksum/hash/大小与安全版本核验通过；app SHA `03be87b6a65ffd555940a9beef36c33dd209f895940f9bc27ba539b8a8dc47a6`、ELF `0f69c9874b4c11792fbce4caa9459ef1c7e55e22dc17b49f9ccfdeba71c83d25`、配置 `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`。
- 固定 IDF Python/esptool 仅对 0x10000 写 283,792 字节：写入 `[0x10000,0x55490)`、扇区擦除 `[0x10000,0x56000)`，全部位于 factory。CLI hash 校验后独立全长回读逐字节/SHA 一致；写前后分区表逐字节一致，SHA `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。
- 不使用完整 flash_args、不写 bootloader/分区/NVS/PHY/FAT，不读完整旧 app 备份；受限写入范围与分区对照不替代受保护数据的全量内容校验，也不保证旧 app 可回滚。

私有忽略证据：`.cache/numbered-gray-{security-private,flash-id-private,image-info,app-flash-private,app-readback-private,partition-before-private,partition-after-private}.log`，`.cache/numbered-gray-app-readback.bin`、`.cache/numbered-gray-partition-{before,after}.bin`。

### 新启动、清白与单张编号图

显式新启动核对 ELF 前缀 `0f69c9874`、IDF v6.1、PSRAM memory test OK、PERF/65536/64；只读 PMU 1.0.9/VCOM valid/1580 mV 与布局门禁通过。PMU sticky BOOT_WAIT `faults=0x1` 保留，不清故障/写校准。先 status=WAIT_CLEAR_COMMAND，随后唯一 clear-white：init ESP_OK/80 ms，clear ESP_OK/14,022 ms/faulted=0；退出 input0/out0=0x81/ESP_OK。

78 帧软件记录独立核对 index=0–77/calls=696/seen=0xff，末行首尾同相且只为 55/AA/00、填充行首尾全 00；registered/done/stored=78、coverage_complete=1，missing/unequal/unexpected/dropped/orphan/late/unfinished/wrong_core 均 0。初始化前 internal free/largest=300819/241664、PSRAM=8384996；初始化及清白后 243911/196608、PSRAM=6682524。仅软件回调证据，不等于电气/光学验收。

WAIT_WHITE_CONFIRM 后请求本轮实屏观察，用户回复“整屏白色，无明显异常”；同启动唯一 white-confirm 接受，状态 REFERENCE_KNOWN。随后仅一次 gc16-gray16-labeled：预检 VCOM=1580 mV、温度延迟到 ON=1；主动温度 28°C、ON=1/PGOOD=1。GC16 completed=1/stage=0/frame_error=0/operation=0/cleanup=0/faulted=0，2,137 ms；退出 input0/out0=0x81/ESP_OK。绘制前后 internal free/largest=243911/196608、PSRAM=6682524，转换临时内存释放。

刷新后先保持 WAIT_PATTERN_CONFIRM/operation=0、pending=gray16-labeled、drawn=0x40/visually_confirmed=0，请求 00–15 相邻区分/白偏/额外条带/残影及照片。用户回复“还是明显白色偏多，请参考小纸官方代码中的示例里的16灰阶测试”，随后附编号图照片：深阶可见、后段大面积浅白，反光/透视使严格可见级数与反射率不可量化。附件为私人资料，未复制入库；实屏明确未通过，不 pattern-confirm。

唯一 pattern-reject 接受/FAULT_LATCHED/operation=0；随后 stop 和 status 均记录完整 STOPPED/operation=0、pending=invalid、drawn=0x40/visually_confirmed=0；DISPLAY_DEINIT/HAL_DEINIT 均 ESP_OK，参考清除。此后控制台报 `Device not configured; no reconnect/retry`，会话 74536 已结束/exit 1，原 USB 端口不存在；不是 exit 0 正常关闭，也不推断人工拔线/供电变化原因。所有拒绝/stop/status 完整日志先于断连，不复位、重连、补清白或再次 GC16。

私有日志 `.cache/numbered-gray-device-console-private.log`。28°C 是单次主动采样证据，不证明温度精度/NTC 一致性；新图仍白偏，不宣称温度修正修复白偏，也不由非控制拍摄作严格因果排除。历史失败不倒改。

### 固定官方示例对照与本轮离线检查

按用户要求读取固定厂家 `main/apps/app_refresh.c`（draw_pattern_page/demo_touch）、`main/display.c`、`read_pico_init.c`、`read_pico_board.c`、`read_pico_epd_timing.c`、`epdiy.c`、`highlevel.c` 与 render 前导跳过实现；重新核验 GitHub 固定源码及官方产品页。未修改 gitlink/厂家源码。fast-context 缺 Windsurf Key，未提取/配置凭据，改用限定路径 rg 和完整函数读取；搜索命中不算接口验证。

官方灰条是 i*17 的 8bit 颜色，厂家 draw_pixel 取高 4 位，得到 0–15；当前转换后原生码同样 0–15。官方 16 灰按钮仍默认 GC16/36 相/1K LUT，未使用额外 gamma/完整 48 相；from_white 先 memset back_fb=ff 再调用 full，当前本轮人工白参考路径等价，不能仅换 API 名声称修复。图案布局/覆盖面积、GC16 清白 vs static clear、HV 8 秒保持 vs 操作后退出、18 MHz/120 MHz vs 12 MHz/80 MHz、前导跳过开启 vs 关闭仍存在差异；无单变量电气或光学证明。详见 [官方对照表](READ-PICO-DISPLAY.md#官方-16-灰阶示例对照)。

重跑 `ctest --test-dir host_tests/build --output-on-failure -R 'ReadPicoFrame|ReadPicoPattern'`，25/25 通过（原 22 + 编号图新增 3），日志 `.cache/numbered-gray-official-format-tests.log`。没有新算法/硬件代码修改，不重新编译或新增候选；原同 SHA 的 276/139/9/8/四配置保留。本轮 app-only/一清白/一编号图权限已执行完，下一对照需离线准备、明确单变量与追加硬件许可，不刷带总 BSP/NVS/SD/标定副作用的官方整包，不降灰阶目标。

收尾检查：21 份当前/索引文档 108 个本地链接可解析，3 份工作流 YAML 可解析，git diff --check 通过；厂商 gitlink 固定且 clean，app/readback/sdkconfig/分区表 SHA 与上述一致，私人日志及回读均被忽略。逐条核对只有 status/clear-white/white-confirm/gc16-gray16-labeled/pattern-reject/stop/status 七条命令；一次清白/一次图案，完整拒绝/退出状态先于 USB 断连。不提交/推送。待补人工基线：同一台设备此前是否实际运行官方 16 灰阶示例且正常、是否有可比较照片；未获得该信息前不把源代码标称或别的面板结果当成本机光学基线。

## 2026-10-03 · 官方绘图/from-white 隔离控制（仅离线）

用户“继续”。本轮仅开发和离线检查，不把此前已完成的一清白/一编号图权限扩展到新镜像或刷新。fast-context 因缺 Windsurf Key 无法使用，未获取凭据；限定路径 rg 和实际函数读取作为回退。PhotoFrame/厂家/SDK commit 均沿用固定基线，厂家工作树 clean、不修改波形或校准。

### 变更与测试边界

- 追加图案 ID7/位0x80/指令 gc16-gray16-vendor，原 ID0–6 不变；便携目标与旧无编号全屏灰条逐字节一致，非编号图也非完整官方 UI。
- 显示诊断单独链接 helper，厂家真实 epd_fill_rect(i*17) 在现有 difference_fb 暂存区生成原生帧；与转换目标全长 memcmp 相等后复制到 front_fb，走实际 epd_hl_update_screen_from_white。错帧拒绝 HL、清理并锁存；不新分配帧缓冲。
- 会话 drawn=0 与后端 untouched_white 双门禁：只能在本启动清白/人工全白确认后的第一张使用。其他图案后不能假造白参考；温度错误不绘制，HL 错误保留并禁重试，stop/deinit 清除参考。标准 prepare/主动温度/退出、静态清白、12 MHz LCD/80 MHz Flash及PSRAM/PERF/默认36相/1K LUT 均不变。
- 新增 13 CTest：图案1、会话2、原生边界5、真实厂家CPU源码5。真实 epdiy.c/highlevel.c 原样编译，测试全屏像素/8bit取高nibble/奇偶顺序/非法参数不改帧/from-white与白参考full的差分及dirty标志。IO、板绑定、renderer/时钟与差分汇编为替身，不验证 ASM/LUT/DMA/电气/光学；原生后端边界测试本身也仍为模拟 HAL/RTOS。

### 本轮实际执行

| 检查 | 结果 | 忽略的本机日志 |
| --- | --- | --- |
| CMake configure/build；格式化后重编译、完整 CTest | 289/289，通过，最终 6.89 秒 | .cache/vendor-control-host-{config,build,tests}.log；vendor-control-host-{build,tests}-final.log |
| ASan/UBSan 九个目标、DEVELOPMENT 中含 ReadPicoVendor 的子集 | 152/152，通过，10.55 秒；不是全部289 | .cache/vendor-control-asan-{config,build,tests}.log |
| python3 -m unittest host_tests/test_read_pico_build.py | 最终10/10通过；首跑旧断言固定源文件列表导致1失败，更新诊断新增源文件预期并加隔离保护后通过，不是固件运行失败 | .cache/vendor-control-build-guards.log；vendor-control-build-guards-final.log |
| python3 -m unittest host_tests/test_read_pico_console.py | 8/8通过，无串口访问 | .cache/vendor-control-console-tests.log |
| 固定IDF/Python build.py --board read_pico --step firmware，加factory及display两开关 | 显示配置通过 | .cache/vendor-control-display-build.log |
| 同命令仅factory开关、无开关、原板 waveshare_photopainter_73 | 三配置分别 Project build complete，无error/FAILED/Traceback | .cache/vendor-control-{factory,readonly,original-board}-build.log |
| esptool --chip esp32s3 image-info，仅本地文件/无port | S3/DIO16MB/80MHz/MMU64KB/secure version0；checksum/hash有效 | .cache/vendor-control-image-info.log |
| 三种Read Pico链接map核对 | helper与诊断API仅显示配置有链接，双OTA/factory只读均无 | build-read-pico*/esp32-photoframe.map（忽略） |

方向/Web 6/60 沿用此前，未重跑完整 test_baseline.sh、远端CI或v6.0矩阵。构建日志没有新的 warning:；不将主机模拟/编译成功计为真机通过。

### 新本地身份与设备边界

- 显示 app **284,688 字节**，SHA256 `eea2743296c97656da0d63ee3196c4e96cfeb63980dcba41a0cfec1eb8419248`；ELF `21cfb088d507ecd34eaa139d62c0caf8217d79a6bce52abbe421538ec8c022ca`。
- sdkconfig SHA 不变：`e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`；factory分区布局不变。factory只读 app 274,304 字节/`a357412344b36f1e1e63e27a3a907792637c675a18210baa8fbef2607bca01d2`；双OTA只读275,424字节/`a330dcb90f097b9d12b6d06432f0490963015b26feee136179b423e376d7af48`；原板2,058,112字节/`3e11ab3bf9b9abb69ed1740626d79d8898d4f13052e4e3a4f158bde620ed17a3`，不能烧入Pico。
- 本轮只列举 `/dev/cu.usbmodem*`，数量0；没有USB打开/复位/读取设备/烧写/刷新。设备仍为上一轮回读的283,792字节/03be87b6…，最后完整STOPPED、参考已清除、编号图白偏拒绝。旧回读SHA不能拿来验证新本地镜像，厂家绘图入口本次没有上板。
- 新候选拟写 `[0x10000,0x55810)`、扇区擦除 `[0x10000,0x56000)`，均在factory内，未执行。下一步需人工恢复USB并明确本轮app-only/一清白/人工全白后单张gc16-gray16-vendor；先重新核验身份/安全/分区/兼容，再全长回读/分区不变。异常或拒绝即stop，不自动补刷、升频、改波形/PMU或NVS校准，不运行官方总BSP/SD/NVS初始化。

灰码/入口已在CPU层证明一致，这个控制验证并不承诺解决白偏；GC16清白/HV保持/前导跳过/18MHz及120MHz产品时序仍是后续独立变量，需要另外方案和风险审查。缺少同设备官方正常运行/可比照片与电气、光学测量基线，I14/I15与P2不关闭，不降低原16灰阶目标。

收尾：21份当前/索引文档108个本地链接、3份工作流YAML及 git diff --check 通过；系统python3缺PyYAML后改用已有固定IDF Python完成检查，未安装依赖。固定SDK分区解析器核对本地生成表与已保存实板表的4个逻辑条目一致，生成文件与4096字节设备回读不是同一原始二进制，不冒称新一轮设备表回读。旧应用回读仍03be87b6…、设备表前后仍f078383d…，本地显示app仍eea27432…/配置e16d333e…；厂家gitlink固定且clean，构建/日志/私人回读均被忽略。无提交、推送或设备操作。

收尾：21 份当前入口/文档的 87 个本地链接、CI YAML 解析及 git diff --check 通过；sdkconfig 摘要不变、SDK 指向固定 v6.1，厂家子模块固定 commit/工作树干净；临时预览/审计/构建与私人日志均被忽略。没有提交/推送、新硬件操作或公开私人照片/设备标识。

## 2026-10-04 · 官方示例独立克隆与真机烧录

用户明确要求新建文件夹、拉取并烧录官方 `MindReset/read_pico_firmware`。该新指令将本轮范围扩展到官方完整 `flash_args`，但不授权整片擦除、SD 格式化、VCOM/PMU 标定或其他页面操作。仓库独立克隆到主工程旁的 `/Users/zhanghuilong/Documents/ChatGPT/read_pico_firmware_official`，HEAD/main 为 `28cde682a4468a581c278761922724f57d976418`，与固定参考基线一致；源码保持 clean。

烧录前核对官方入口、ONBOARDING、启动流程、NVS、VCOM 与 SD 格式化路径。SD 格式化需要页面内两次人工确认；VCOM 写入只存在于未标定拦截页。本机现有 NVS 只读备份 20 KiB，解析为 5 页、13 个 written、0 invalid、各页 CRC OK，空余充足；备份 SHA256 `63ad8691100bb218f04dff2af1409b6c6226ea0cf59c1b04fe5143e19ce8ac89`，保存在官方克隆的忽略目录。设备分区表前 3072 字节与官方生成表一致，SHA256 均为 `205e21ddff471bce79c29a846bd04a48854cb42d406c784405d4c8e3ec1e6061`。Flash 为 Zbit `5e/4018`、16 MB、3.3 V；未公开 MAC/UID。

ESP-IDF v6.1 默认 `sdkconfig.defaults` 编译成功：bootloader `2a875d61daf2b05640020188e1cc0eca2fd76a266c067a45f85c7d08bd519eb2`，app `be4959a234d0f1cddbe6b895edd67800b82e9cd62bc90326c21aa87eb66eb721`，应用大小 `0x2efd20`。按生成的 `flash_args` 写 bootloader `0x0`、分区表 `0x8000`、app `0x10000`，三段均 `Hash of data verified`；没有 erase-flash，也不写 NVS/PHY/storage。该120 MHz产品配置从 `0x10000` 载入并识别Zbit HPM与AP 8 MB PSRAM，但两次均停在 `MSPI Timing: Enter psram timing tuning`，未进入 `app_main`，不记为启动通过。

随后不改源码，使用官方仓库自带 `sdkconfig.ci` 的80 MHz安全配置重新构建并按同一地址表烧录。当前 bootloader SHA256 `c7f769530b03d105c6563d535482507db0cda0c57ebf208bbc2acc85afe5081a`，app SHA256 `24e52ec860e4e5e7ceccc67ec0fe392117244411360515df44e6f7f5731de2d9`，应用大小 `0x2ef150`；三段写入和哈希校验通过。启动日志确认：factory app从 `0x10000` 加载、ESP-IDF v6.1、8 MB PSRAM 80 MHz及内存测试通过、I2C扫描4设备、PMU 1.0.9、VCOM从PMU读取为1580 mV、SY轨电压对应VCOM -1580 mV、SD已挂载，最终 `app_loop: UI ready on 概览 Overview`；随后屏电源轨按8秒空闲策略关闭。串口已释放。

本轮没有进入 `EPD Refresh` 或触发16灰阶；未执行SD格式化、NVS/PMU/VCOM校准写、整片擦除或网络功能。官方源码中的NVS异常恢复分支仍存在，但写前分区完整且有充足空位，本次没有观察到初始化异常；不能将80 MHz CI配置冒充官方120 MHz产品时序验收。当前设备运行官方提交的80 MHz构建并停在概览页；需要人工目视确认屏幕内容后，才可把显示结果记为实屏通过。

## 2026-10-04 · 官方独立灰阶通过后的根因收敛：项目刷新缺少 DFS 时钟锁

### 同机基线与已排除差异

从固定官方仓库抽出的独立 16 灰阶固件已烧入同一设备，应用 831,728 字节，SHA256 `8b7483bbbdee59702f9fb13d345467f14592d4f13af67c0f83437f8e1246523e`，全长回读一致。该固件先 `epd_hl_set_all_white → GC16 full`，再以官方 `epd_fill_rect(i * 17)` 页面执行 `epd_hl_update_screen_from_white`；18 MHz、FULL、1K LUT、默认裁剪 GC16 36 相、PMU factory VCOM 1580 mV、25°C 均来自官方路径。用户实拍确认局部灰条可见连续阶次、40 像素边距/棋盘/黑白块几何正确。它证明同一块面板、同一 VCOM 和同一官方波形能够正常工作，项目长期白偏不是面板“不支持16阶”的结果。

按“先刷白、再展示16灰阶”复验后的新实拍仍为官方独立页（页眉明确显示 `Standalone`）：16个灰条槽位完整，从0黑到15白方向正确，肉眼未见项目旧结果中约05–15大面积并白。对JPEG中各灰条中心等面积区域取亮度中位数得到 `55, 58, 65, 78, 85, 100, 113, 124, 139, 145, 152, 161, 165, 170, 175, 180`，15个相邻差均为正，因而这张照片提供了单调性的辅助证据。相机曝光、环境光和JPEG处理未经标定，这组数值不作为反射率或严格16级光学计量；它也不能替代下述项目PM锁候选的真机验证。

项目侧此前已经逐字节证明：官方 `i*17` 经高 nibble 得到的码正是 0–15；厂家 `epd_fill_rect` 页面与便携参考完全相等；from-white 入口、波形对象、36相选择、FULL扫描、18 MHz及factory VCOM均一致。精确684行修复恢复了黑白几何，但普通、GC16铺白后下电及连续高压三种流程都留下约05–15趋白。因此灰码、图案、gamma、VCOM、白参考启停和页面几何不再是解释3–4倍刷新时差的首要候选。

### 决定性差异与机制

官方独立固件的 `sdkconfig` 没有启用 `CONFIG_PM_ENABLE`，CPU固定240 MHz。项目显示诊断配置则为 `CONFIG_PM_ENABLE=y`、`CONFIG_PM_DFS_INIT_AUTO=y`、最高240 MHz、最低由40 MHz XTAL确定；旧显示适配层从未在刷新期间申请 PM lock。ESP-IDF 6.1 的ESP32-S3功耗管理表明确：没有锁时CPU/APB处于配置的最低频点；`ESP_PM_CPU_FREQ_MAX`请求240/80 MHz，`ESP_PM_APB_FREQ_MAX`请求APB 80 MHz。

这对当前 EPDiy 路径不是无关配置：

- LCD PCLK显式选择PLL240M，所以日志仍会显示正确的18 MHz，不能据此证明整条扫描链时钟稳定。
- EPDiy没有调用新版RMT驱动，而是通过 `rmt_compat.c` 直接写LL寄存器；ESP32-S3的 `RMT_CLK_SRC_DEFAULT` 是 `SOC_MOD_CLK_APB`，此兼容层只启用时钟源，不申请PM锁。
- ESP-IDF 6.1自己的非DMA RMT驱动会创建 `ESP_PM_CPU_FREQ_MAX`，源码注释说明CPU降频会使供数/编码不稳定；EPDiy的CPU renderer/feed任务同样没有自行加锁。
- 项目真机日志直接在 `epd_lcd_start_frame` 到frame-done之间量得：官方页36相为30.492–34.492 ms/相；编号页为43.484–48.489 ms/相；连续两帧大多43–48 ms/相。相间空档仅约22 µs，所以额外时间发生在每个物理扫描相位内部，而不是相位间业务代码。
- 18 MHz扫描求解值是约11.2 ms/相、36相约404 ms；官方同机页面显示16灰刷新约399 ms。项目HL draw为1.206–1.686 s，恰为约3–4倍。探针会增加少量软件开销，但无法解释稳定的每相30–48 ms以及无探针早期同量级draw时长。

综合以上证据，当前最具体且能解释全部现象的根因候选是：**项目开启DFS，却让绕过官方驱动的APB-RMT和CPU供数链在无PM锁状态下刷新；GC16每相被拉长约3–4倍，改变了面板实际驱动剂量，使多个浅灰码过驱并塌缩到接近白色。** 这是已由代码、SDK文档和旧相位测量支持的因果模型；在新固件真机恢复约11 ms/相及灰阶之前，仍标为“待硬件证实”，不倒推为已闭环。

### 单变量修复与离线验证

`epaper_driver_read_pico` 现于初始化创建CPU/APB两把PM锁，在静态清白和每个GC16事务上电前同时持有，直到rails安全关闭后释放；初始化失败、prepare失败、绘制失败、温度失败和deinit均走同一释放/删除路径。这样保持项目的自动DFS策略，但在显示事务内复现官方固件的稳定240/80 MHz条件。候选另通过ESP32-S3时钟树API记录加锁前后的CPU/APB实频；该API在此芯片上直接读取当前CPU/APB硬件频率，不是配置上限。未修改gamma、VCOM、频率上限、18 MHz PCLK、leading-skip、波形、灰码、页面或固定第三方子模块。

| 检查 | 结果 |
| --- | --- |
| 新PM锁主机替身与生命周期断言 | 清白和GC16内部必须同时持有2把锁；每次事务后acquire计数归0；deinit后锁对象归0 |
| 完整主机CTest | 310/310通过，7.02秒 |
| 构建/控制台Python检查 | 20/20通过 |
| 固定ESP-IDF v6.1显示诊断编译 | 成功；`esp_pm`真实组件编译并链接 |
| 本地候选 | 305,104字节；SHA256 `6f2a4ea106f7aab2c1d8ff2fd2605f70b0aefafaa5c437b0d688106a12055255`；ELF `44ec27ed6eb7d2783415a5629f296dfe6be2da9a30892731b415c9bcf4983545` |
| 配置 | DIO/80 MHz/16 MB；CPU 240 MHz；DFS仍开启；64 KB cache/64 B line |

本节没有串口打开、复位、烧写、清白或GC16；设备仍是已回读一致的官方独立固件。下一步只需一次严格A/B：app-only写入factory并回读，分区表前后不变；新启动执行一次静态清白，人工确认全白后只显示一次官方局部页。软件通过门槛为36/36相且单相回到约11 ms、HL draw接近0.4 s；光学门槛为16条单调、相邻可分且无旧白偏。若时间恢复而灰阶不恢复，则DFS只解释性能而非白偏；若时间仍为30–48 ms，则本候选被否证并转查探针开销/renderer调度和外部CKV测量。未经这次真机证据不标记P2通过。

## 2026-10-04 · PM时钟锁候选真机A/B（时序与16阶光学通过）

用户明确回复“执行”后开始严格单变量验证。写前候选仍为305,104字节、SHA256 `6f2a4ea106f7aab2c1d8ff2fd2605f70b0aefafaa5c437b0d688106a12055255`，image-info为ESP32-S3、16 MB、DIO/80 MHz、ESP-IDF v6.1，ELF摘要`44ec27ed6eb7d2783415a5629f296dfe6be2da9a30892731b415c9bcf4983545`。设备为ESP32-S3 v0.2、Zbit `5e/4018`、16 MB/3.3 V，Secure Boot与Flash Encryption关闭。

写前读取完整4096字节分区扇区，逻辑布局仍为NVS `0x9000/0x5000`、PHY `0xe000/0x1000`、factory `0x10000/0x400000`、storage `0x410000/0x500000`；前3072字节与候选生成表逐字节一致，尾1024字节全`0xFF`。仅写factory应用 `[0x10000,0x5a7d0)`，工具报告写入305,104字节并验证数据哈希；全长独立回读与候选逐字节/SHA一致。写前后4096字节分区表逐字节相等，SHA均为`f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。未写bootloader、分区、NVS、PHY、storage或PMU校准。

新启动从factory加载正确候选，应用身份、ELF摘要和编译时间匹配；DIO/80 MHz、16 MB Flash及8 MB AP PSRAM 80 MHz内存测试通过。启动仅进入`WAIT_CLEAR_COMMAND`，没有自动清白或GC16。显式一次`clear-white`后：扫描配置为18 MHz、288 cycles/line、16.0 µs/line、理论89.15 Hz/约404 ms GC16；锁日志为CPU `240→240 MHz`、APB `80→80 MHz`（命令任务在取锁前已经唤醒CPU，锁的作用是阻止扫描等待期间再次降频）。静态清白78/78帧、每帧696次软件回调，padding/边缘/欠载/孤儿/迟到/跨核等计数全0，关轨观察`ESP_OK`，总耗时1.630秒、faulted=0。此前同类静态清白为12.873秒，本轮时长显著恢复，但静态清白不是GC16光学验收，不能单独关闭根因。

会话当前停在`WAIT_WHITE_CONFIRM`，尚未发送`white-confirm`或任何灰阶命令。下一步必须先由用户目视确认本启动整屏均匀白色，随后只发送一次`gc16-gray16-page`并同时核对36相计时和光学阶次；若白屏异常则拒绝图案并停止。

用户随后明确确认“整屏白色无异常”。同一启动发送一次`white-confirm`后只执行`gc16-gray16-page`，实际命令序列截至当前严格为`status → clear-white → white-confirm → gc16-gray16-page`，无其他GC16、重试或调参。ON态温度27°C、PMU VCOM 1580 mV，锁日志仍为CPU 240 MHz/APB 80 MHz；厂家控制页再次确认`i*17 epd_fill_rect native_equal=1 from_white=1`。

GC16软件结果为completed=1、operation/cleanup/fault均0，36/36相全部启动并完成，leading-skip=0、prefill=32；单相`start_to_done`最小11,090 µs、最大11,112 µs、平均11,092.42 µs、中位11,091 µs，36相合计399.327 ms。HL记录diff 27 ms、draw 419 ms、buffer copy 13 ms。相间gap除首相外约22–36 µs；12条边缘目标行配对/边界全部可区分，696行、欠载、错误、dropped/orphan/late/unfinished/wrong-core均为0，关轨观察`ESP_OK`。这与波形常量11,090 µs/相及官方同机约399 ms完全同量级，而旧项目为30.5–48.5 ms/相、HL draw 1.206–1.686 s；因此“缺少PM事务锁导致扫描时钟/供数在DFS下失稳”对**时序异常**的因果关系已经由真机单变量A/B证实。

用户随后提供本轮实拍。对16条灰带中心等面积区域取照片亮度中位数，依次为`48, 53, 60, 68, 78, 86, 91, 99, 115, 121, 133, 148, 156, 162, 165, 169`；15个相邻差为`5, 7, 8, 10, 8, 5, 8, 16, 6, 12, 15, 8, 6, 3, 4`，全部为正。三个纵向采样窗也保持单调；40像素白边、灰条、黑白块、棋盘和细线几何正确，没有旧样本约05–15大面积并白。JPEG、环境光和透视不构成校准反射率测量，但足以支持本轮同照片内的顺序与旧故障对比。

收到照片后发送一次`pattern-confirm`，再发送`stop → status`。最终`drawn=0x100/visually_confirmed=0x100`，`LCD deinitialized`、DISPLAY_DEINIT/HAL_DEINIT均`ESP_OK`，状态为`STOPPED/operation=0`；控制台关闭后串口无占用。完整实际命令序列为`status → clear-white → white-confirm → gc16-gray16-page → pattern-confirm → stop → status`，仅一条GC16_BEGIN/RETURN，无重试、其他图案、调参或校准写。

最终回归：`ctest --test-dir host_tests/build --output-on-failure`为310/310通过（7.22秒），构建/控制台Python检查20/20通过。时序恢复和光学恢复同时发生，且唯一功能变量是显示事务CPU/APB PM锁，因此本轮持续白偏的具体根因已闭环。该结论只关闭16灰阶故障调查，不把单张结果扩展为P2重复性、照片链路或长期稳定全部通过。

## 2026-10-04 · P3 SDMMC 1/4-bit 软件准备（无设备操作）

用户要求继续 R-Core-3 开发，本轮完成独立于 P2 剩余硬件验收的 SD 软件准备。优先上下文搜索因缺少 Windsurf Key 不可用，回退限定目录 rg 与真实函数读取，未提取或绕过凭据。固定 SDK 仍为 v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`；Read Pico gitlink `28cde682…` 与其工作树保持原样。

PhotoFrame 原 SDMMC backend 固定4-bit，现通过 `sdcard_config_t.bus_width` 显式选择1/4-bit。1-bit 强制 D1/D2/D3 为 `GPIO_NUM_NC`，原 WaveShare 初始化显式填写4-bit；Read Pico 板级静态定义 CLK38/CMD42/D0=44、D1–D3未连接、bus_width=1。既有 Kconfig 符号 `SDCARD_DRIVER_SDIO` 保留，菜单说明改为 SDMMC。

新增配置检查拒绝空配置、空挂载路径、超过31字节的路径、不支持的总线宽度与缺失必要引脚；过长路径不再出现挂载完整路径但卸载截断路径的不一致。重复挂载在调用IDF前拒绝；挂载失败清除card handle，成功后才保存挂载路径。`format_if_mount_failed=false`保持，显式 `sdcard_format()` 接口仍保留；本轮所有mount/unmount/format调用均为主机替身，无实体介质IO。卸载错误行为沿用上游，不将其视为故障恢复已验收。

| 检查 | 结果 |
| --- | --- |
| 新增真实backend主机替身测试 | 8/8；无效配置、1-bit引脚屏蔽、4-bit传递、挂载失败、成功但无handle、重复初始化、显式format及正常卸载 |
| 普通CTest全量 | 318/318，7.20秒 |
| ASan/UBSan全量 | 318/318，34.13秒，无sanitizer报告 |
| 格式整理后受影响测试 | 普通与ASan/UBSan各8/8；其余源码未变 |
| 构建/控制台Python检查 | 20/20，1.857秒，无串口访问 |
| 固定IDF原板编译 | `waveshare_photopainter_73 --step firmware`成功；SD backend与原板初始化实际编译，最终增量16步骤成功 |
| 固定IDF Read Pico只读编译 | `read_pico --step firmware`成功；1197步骤完成 |
| 固定IDF Read Pico显示诊断编译 | factory/display两个显式开关，13步骤成功；未烧写 |
| 诊断隔离 | 只读和显示配置均未启用SDIO/SPI；只读ELF仅命中app_main，未定义sdcard_init/format、SDMMC mount、NVS init/erase或Wi-Fi init |
| 空白与来源检查 | `git diff --check`通过，固定厂家子模块clean；组件版本/component_hash未升级 |

首次编译因未指定隔离Python环境，IDF检测到与构建缓存路径不符并在源码编译前退出；尝试把IDF_TOOLS_PATH指向隔离目录时又因缺少约束缓存而未激活。最终沿用DEVELOPMENT中的 `IDF_PYTHON_ENV_PATH` 指向既有隔离环境，默认工具/约束缓存路径不变，双板和显示配置通过；未运行fullclean或重装SDK。另一次编译后连接ELF检索的整体命令exit 1来自rg没有命中禁止入口，构建自身明确成功，不能误记为编译失败。仍有固定SDK的Kconfig/include通知、上游字体及厂家代码历史告警，SD修改源无新增编译告警。

原板与Read Pico配置会生成不同的直接依赖manifest元数据。最终保留本轮Read Pico解析得到的锁状态（直接idf、manifest_hash `5be05836…`）；所有版本/component_hash保持既有锁定，未自动升级依赖。

| 本地产物 | 字节 | SHA256 |
| --- | --- | --- |
| 原板 build/esp32-photoframe.bin | 2,058,368 | `f2e3e896bc329e53b7156c2540b6299832fb15b0c0d8ad5c6723dd91e5686a2b` |
| Read Pico只读 build-read-pico/esp32-photoframe.bin | 276,448 | `2b8c946edb7708bca65d37d8cee611a05168831e8202c307ee7274e87a8dbde4` |
| Read Pico显示诊断 build-read-pico-display/esp32-photoframe.bin | 305,104 | `6f2a4ea106f7aab2c1d8ff2fd2605f70b0aefafaa5c437b0d688106a12055255` |

编译记录位于忽略的 `build/log/idf_py_stdout_output_29051`（原板最终）、`build-read-pico/log/idf_py_stdout_output_21828`（只读）及 `build-read-pico-display/log/idf_py_stdout_output_30619`（显示），另有对应stderr；测试结果来自本轮工具输出。生成物和日志不提交。

烧写回读：本轮未执行。真机SD挂载、读写/回读、无卡、卡供电/退出与照片读取：均未验收。现有Read Pico诊断不选择SD driver、不调用厂家总BSP或/storage挂载；本轮没有打开串口、复位、清白、GC16、格式化或校准写，设备部署与上轮STOPPED状态没有被本轮操作改变。下一项是独立产品入口的唯一/storage接线和故障处理，再进入实体卡验收；P2重复性与照片显示仍按原计划保留。

## 2026-10-04 · P3 单一SD适配与手动隔离入口（无设备操作）

用户继续开发，本轮实现SD检测、所有权和退出适配，并编译独立手动入口；没有恢复灰阶验证或操作实体卡。固定厂家 `read_pico_sd.c` 确认1-bit/40MHz/内部上拉，`read_pico_board.c` 的 `read_pico_sd_present` 确认P0.6低有效；映射中没有独立SD供电控制，不借用显示电源位、不调用厂家总BSP/默认挂载。语义搜索仍缺配置，使用限定目录源码核对，无凭据提取。

`read_pico_board_sd_present` 在共享递归锁内只读方向/输入，拒绝输出模式并区分IO错误和无卡；`read_pico_sd` 独占PhotoFrame `/storage`，挂载前/后核对CD、取得HAL存储租约、只调用一次backend。backend上拉为显式opt-in，Read Pico开启，原板保持false/4-bit。挂载期间拔卡清理后不发布可用状态；挂载后观察到拔卡/CD错误使media_valid锁存，不隐式卸载或重挂，未来消费者须先关文件。

实际读取固定IDF v6.1 `components/fatfs/vfs/vfs_fat_sdmmc.c`：`esp_vfs_fat_sdcard_unmount` 先释放context，`unmount_card_core` 可先free(card)再返回VFS unregister错误。因此不能简单保留指针重试。本轮backend清指针并锁存卸载错误，后续deinit返回原错且不再次调用SDK，init/format拒绝；Read Pico外层保留HAL租约和exit_failed，防止失败后正常HAL退出。这是保守软件策略，不计真机拔卡/故障恢复通过；其他板既有睡眠业务未改写。

`--read-pico-storage-validation` 必须与factory诊断同用，与display互斥；独立build/sdkconfig目录，CLI拒绝flash/monitor/erase/fullclean。开机仅HAL身份/VCOM GET/CD，不挂载；精确status/mount/stop命令有界解析，超长/二进制/控制字符整行拒绝。mount只一次，stop先卸载再HAL退出，成功STOPPED/失败FAILED均终止命令处理。不开屏/NVS/网络、不写文件、不格式化。仍使用同步SDK，未新增可取消硬超时；PMU无HOST_READY的诊断启动超时仍为实板限制。完整PhotoFrame入口与NVS安全迁移尚未接入。

| 检查 | 结果 |
| --- | --- |
| 新增主机测试 | 17项：backend上拉/卸载锁存2、真实HAL检测/租约3、SD适配及有界命令12；底层IO为替身 |
| 普通CTest全量 | 335/335，9.12秒 |
| ASan/UBSan全量 | 335/335，48.46秒，无sanitizer报告 |
| Python构建/控制台保护 | 23/23；新增storage隔离/非法参数/安全入口3项，无串口访问 |
| 固定IDF编译 | 原WaveShare、Read Pico只读、factory只读、display、storage五配置均成功 |
| 链接隔离 | 只读/factory/display均无SD init/mount；storage链接唯一sdcard_init/SDMMC mount和外层mount/unmount，无sdcard_format、NVS init/erase、Wi-Fi init、epd_init或厂家read_pico_init |
| 配置隔离 | 仅storage启用SDIO，四Read Pico配置均禁内部Flash storage；显示与SD各自专用 |
| CI | 增加第五个编译profile；远端未运行，不计Actions通过 |
| 来源/空白 | 固定厂家子模块clean，git diff --check通过，组件版本/hash未升级 |

| 本地产物（未烧写） | 字节 | SHA256 |
| --- | --- | --- |
| build-read-pico-storage/esp32-photoframe.bin | 320,624 | `3168a7cc4da8d8afc3d11097fe80c813959e559f0b4b16c959df7ecf251cd13d` |
| build/esp32-photoframe.bin | 2,058,400 | `34aef1db9540003e429f13bbdc55b3625dd705d39ed3b14dd7671c84b0c8b59a` |
| build-read-pico/esp32-photoframe.bin | 276,464 | `b510dd8ee43da3f24c2c23760f043e42c1302b6b7fd043cec90c67e72eb1cbdb` |
| build-read-pico-factory/esp32-photoframe.bin | 275,344 | `af5b36f2efb1e1c5acfa54ce529205a0a6d6ca94efb65e8ec02abf090f58b334` |
| build-read-pico-display/esp32-photoframe.bin | 305,120 | `76ae17a3c653b6456a75da84ef22e233e0e1bbd5ae92adaa5886fdcb5ad820d8` |

本轮最终编译日志为忽略目录 `.cache/p3-{storage,original,readonly,factory,display}-build.log`；SDK仍有既有Kconfig/include/上游字体告警，本轮SD/HAL源无新增编译错误。新显示构建受公共HAL变化影响身份已不同，不能替代设备已验收镜像。设备仍为上轮305,104字节/`6f2a4ea…`；没有串口、复位、烧写、回读、实体卡IO、清白、GC16、校准或PMU电源写，本轮也未提交/推送。锁文件最终保持Read Pico解析元数据，所有版本/component_hash不变。

烧写回读：未执行。真机验收：未执行。完整产品入口、实际卡供电与断电、有卡/无卡、明确测试文件读写回读、读图及照片显示均待后续；当前只完成软件准备，不标记P3 MVP通过。使用契约见 [SD说明](READ-PICO-STORAGE.md)。

## 2026-10-04 · P3 完整业务集成编译及数据保护（人工介入前的软件检查）

用户要求继续开发直到需要人工介入。本轮沿用真实PhotoFrame架构，完成其Read Pico业务编译链接与存储/NVS/路径安全适配，没有运行设备或扩大既有烧写授权。fast-context服务缺少配置，代码定位使用限定目录rg，源码接口均实际读取后使用；研究/归档不作为新执行许可。

新增独立 `--read-pico-photoframe-integration`，必须Read Pico/factory/firmware-only，与显示/SD profile互斥。真实main重命名为photoframe_app_main并以volatile引用保留整个业务链路；新app_main只报告INTEGRATION_ONLY，不调用真实产品、不初始化板卡、SD、NVS、显示或网络。使用既有精确684行外层覆盖、PERF/cache，厂家子模块/波形/VCOM/gamma/PCLK及80MHz存储设置不变。旧共享splash/Web资源仅作编译输入，不计新板画面运行通过，也不计OTA分区可用。

真实 `storage.c` 在Read Pico只调用唯一SD适配，一次初始化失败返回原错，不fallback到Flash/MemFS；持久存储查询检查新鲜CD/挂载/介质有效/退出状态。检查式卸载用所有权而非可用状态决定清理，失效/拔卡仍能退出，失败保持占用并拒绝新挂载。Read Pico格式化禁用；wifi.txt无有效卡时不读文件/写设备名。尚无文件引用计数/业务停止协调，不计并发或热拔卡恢复完成。

NVS启动在新板板卡/存储业务之前初始化，错误直接返回；新板bootstrap构建排除整个自动擦除分支，即使误传legacy opt-in也不可启用。完整栈链接审计还发现上游Web factory-reset整分区擦除，Read Pico改为501/NOT_SUPPORTED、不擦除/重启；原板行为保留。真实NVS初始化成功不意味着旧Demo/产品配置已迁移，产品仍会写配置，所以运行门禁未开放。

current_image/last_image统一512字节含NUL；文件及流式display_name/save_path/fallback_name在刷新前校验，超长返回INVALID_SIZE并释放流锁，不刷新/发布/NVS提交。511字符解码、链接、当前状态和NVS记录不截断，快照失败后的同容量fallback保持完整；没有自动修复旧截断数据，也不宣称所有HTTP/相册API具有相同上限。

首轮完整栈O2编译暴露NC按键负值位移和配网/缩略图strncpy告警：GPIO mask便携化，负值和>=64拒绝位移；同容量HTTP名字完整拷贝、STA有终止字符串、AP按显式SSID长度允许满32字节。Read Pico睡眠入口在任何关网络/卸载/ESP deep sleep动作前返回，等待PMU/wake策略验收；这不是低功耗实现。JPEG分配失败日志size_t格式修正，无图像算法改动。

| 检查层 | 本轮结果 |
| --- | --- |
| 新增主机用例 | 21：通用NVS5、新板硬禁擦除2、真实storage7、实际隔离启动1、GPIO mask3、显示长路径/拒绝/回退3；新板NVS测试不提供erase mock，链接不能依赖擦除 |
| 普通CTest | 356/356，7.61秒 |
| ASan/UBSan | 356/356，40.29秒，无sanitizer报告；实体IO均替身 |
| Python构建/控制台 | 26/26，最终复跑1.882秒；18构建保护+8控制台，无串口访问 |
| Web回归 | Node22下60/60，2测试文件；未操作设备浏览器 |
| 固定IDF编译 | 原WaveShare、Read Pico只读、factory只读、display、storage、integration共六配置成功；原板及integration在最终业务修改后复跑 |
| 完整栈链接 | app_main与photoframe_app_main、storage_init/read_pico_sd_mount、image_processor_init/http_server_init均保留；无nvs_flash_erase、sdcard_format或厂家read_pico_init；bootstrap对象仅未定义引用nvs_flash_init |
| 原隔离入口 | 四种诊断入口保持原用途；只读/factory/display无SD挂载，storage无显示/NVS/Wi-Fi/文件操作启动 |
| CI | 加入第六个编译配置；远端未运行，不计Actions通过 |
| 来源/数据 | 固定厂家28cde682…子模块clean，组件version/hash未升级；不提交构建资源/凭据/私人照片，保留原有工作树修改 |
| 烧写/回读 | 未执行 |
| 真机/SD/照片/睡醒验收 | 未执行，不标记P3/P4/P5通过 |

| 本地产物（未烧写） | 字节 | SHA256 |
| --- | --- | --- |
| build-read-pico-integration/esp32-photoframe.bin | 1,972,096 | `5893eb3cacb6a7073f8b3ec27ce5c1a721cf54b205928b6e7e04b419505e7ced` |
| build/esp32-photoframe.bin | 2,058,592 | `031c74cc988611395d247ffc0f5f98c0fec31b236345e42a7cee954ada75b7bd` |

本轮四个原诊断profile的字节/SHA与上节相同，integration不是可烧写产品入口。日志在忽略目录 `.cache/p3-product-{host,asan,python,web-tests}.log`（Web为web-tests）、`.cache/p3-product-{original,readonly,factory,display,storage,integration}-build.log`。完整栈初次失败已修复后复编成功，不隐藏为首次即通过；固定上游既有字体/Kconfig告警不等于本轮错误。原板/完整栈与诊断解析可能生成不同manifest/direct_dependencies元数据，但组件版本/hash不变；最后重编SD诊断保持原Read Pico解析形态。

下一步真实依赖：用户提供已备份、无重要数据的FAT32测试卡，确认设备/供电及本轮仅factory应用更新的SD诊断许可，分会话进行无卡/有卡检测、单次挂载和正常卸载/HAL退出。挂载不是硬件只读模式，不拿原片卡实验，不自动格式化/写文件。其后再验证限定测试文件、真实照片与产品迁移/显示/PMU策略。设备最后已知仍为305,104字节/6f2a4ea…、显示/HAL STOPPED；本轮无串口、复位、烧写、回读、清白、GC16、SD介质或PMU写，也未提交/推送。详见 [集成说明](READ-PICO-INTEGRATION.md)。

## 2026-10-04 · P3 SD隔离诊断 app-only部署与有卡一次挂载（无卡待人工取卡）

用户对上轮“准备已备份且无重要数据的FAT32测试卡，仅factory SD诊断更新，执行无卡/有卡、一次挂载及卸载；不格式化/主动写文件、不改NVS/PMU校准，挂载不是硬件只读模式”的关口回复“确认”。本轮仅在该范围执行，不扩展到照片文件读写、产品运行/迁移、显示或PMU配置。

### 产物与写前核对

既有SD诊断320,624字节/`3168a7cc4da8d8afc3d11097fe80c813959e559f0b4b16c959df7ecf251cd13d`，ESP32-S3/DIO/16MB/80MHz/MMU64KB、secure version0、校验和/hash有效，镜像描述符ELF SHA为`ce105b4d1c655694eb0961daff297cd0e7b2b650e33307483c1038d43364005a`。配置明确storage=y、显示/integration未启用，使用本设备固定factory表；完整业务集成镜像没有烧写。

端口枚举只有一个USB JTAG/serial debug unit。固定SDK Python/esptool5.4.0以115200/ROM/no-stub核对同一设备标识（不公开MAC），S3 v0.2、8MB AP_3v3 PSRAM、16MB Flash/5e4018、Secure Boot和Flash Encryption关闭、crypt count0。写前完整读取4096字节分区扇区：NVS0x9000/0x5000、PHY0xe000/0x1000、factory0x10000/0x400000、内部storage0x410000/0x500000；生成3072字节表与其前缀逐字节一致，其余1024字节全FF。

写前按上轮已核实长度305,104字节读取当前app，SHA与`6f2a4ea106f7aab2c1d8ff2fd2605f70b0aefafaa5c437b0d688106a12055255`逐字节/摘要匹配。它仅是上轮显示诊断应用的本地副本，不是完整Flash/移植前旧app备份；没有读取NVS/PHY/内部FAT内容或PMU密钥/校准备份。

### 烧写与回读

镜像复制到忽略目录固定本轮输入。ROM状态下只写`0x10000`一个应用目标，keep flash参数、no-compress，无flash_args/erase-all/bootloader/分区/数据区写入；工具报告写入320,624字节、4.1秒并Hash verified。应用写入范围`[0x10000,0x5e470)`、扇区擦除`[0x10000,0x5f000)`，完全位于factory。

| 核对 | 实际结果 |
| --- | --- |
| app全长独立回读 | 320,624字节，cmp与固定输入一致，SHA均3168a7cc4da8d8afc3d11097fe80c813959e559f0b4b16c959df7ecf251cd13d |
| 分区写前/后 | 两份4096字节cmp一致，SHA均f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20 |
| 保护范围 | 写入/擦除未触及bootloader、分区表、NVS、PHY、内部FAT；没有对这些数据区做全部内容前后校验 |

### 实板有卡会话与退出

在已打开串口上显式ESP USB reset捕获新启动，不复位PMU/发HOST_READY。保留bootloader加载新app，ELF前缀ce105b4d1…匹配；8MB PSRAM/80MHz、SPI SRAM memory test OK。SD入口报告无自动挂载/格式化/文件写/显示/NVS/网络；启动完成后 `SD observation=ESP_OK present=1 mounted=0 media_valid=0 exit_failed=0`，READY后才手动发送命令。

执行顺序：

1. 明确一次`mount`：真实SDMMC width=1、CLK38/CMD42/D0=44、D1–D3=-1；SDK成功挂载`/storage`，返回`Mount=ESP_OK (one attempt)`。卡报告SDHC、40.00MHz limit40MHz、59638MB；没有主动创建/打开/写入业务文件或格式化。这不证明文件系统格式、所有卡容量/型号支持或读写吞吐，FAT挂载也不是硬件只读。
2. 明确`status`：新鲜CD读取，`ESP_OK present=1 mounted=1 media_valid=1 exit_failed=0`；未观察到掉卡/读错误。
3. 明确`stop`：真实backend卸载、存储租约释放与HAL退出；完整日志`Unmount=ESP_OK`、`HAL cleanup=ESP_OK`、`STOPPED; command processing ended`，随后app_main返回。没有补发挂载/重试或再次初始化。
4. 日志确认终态后控制台EOF退出，exit0/串口关闭；不把等待人工期间的存储资源保持挂载。

真机通过范围仅本卡此次有卡CD、一次挂载、正常卸载/HAL退出。无卡检测尚未执行，需要用户此时取出测试卡并回复，再重新启动隔离会话；不在挂载时拔卡。读写/回读、坏卡/满卡/热拔、照片显示、产品迁移及长期稳定仍未验收，不标记P3 MVP完成。新SD固件不刷新屏幕，旧画面不能作为新固件运行证据，没有活动白参考。

### 本轮控制台保护与留档

为现有手动控制台新增显式`--profile storage`，仅status/mount/stop；默认display白名单保留。两种profile互相拒绝对方操作，格式化/未知/前缀/控制字节拒绝；EOF不发送指令或复位，部分写入不重试、断连不重连。新增3项替身测试，Python构建/控制台总29/29（1.913秒），没有测试调用真实串口。未修改C驱动/固件，沿用上轮356普通/356 sanitizer和六配置编译证据；不能将本轮Python回归当成重跑所有C测试。

私有设备/烧写/回读/控制台日志与镜像存于忽略目录 `.cache/p3-sd-hardware-EdGEy4/`，设备标识/卡信息不提交。固定厂家子模块不改；主机脚本及文档之外无源码修改、无提交/推送。当前应用已是320,624字节/3168a7cc… SD诊断且完整STOPPED；后续显示需重新指定镜像/许可并建立白参考。待人工取卡后继续无卡验证，见 [SD说明](READ-PICO-STORAGE.md)。

## 2026-10-04 · P3 SD隔离诊断无卡检测与单次挂载拒绝

用户在此前已成功卸载/HAL退出并关闭串口后回复“已取出”。本轮仅继续已获准的无卡分会话验证，不烧写、不读写测试文件/照片、不格式化、不刷新屏幕、不运行产品入口或网络/NVS业务；没有PMU配置/校准写或HOST_READY。

使用现有storage白名单控制台、同一USB端口，显式ESP USB reset开启新会话。boot报告ELF前缀`ce105b4d1…`匹配已部署SD镜像，PSRAM memory test OK；不将本次启动日志当成新的镜像全长回读或分区校验。

| 阶段 | 本轮实板证据 |
| --- | --- |
| 启动 | 593ms：SD observation=ESP_OK，present=0、mounted=0、media_valid=0、exit_failed=0；594ms READY，无自动挂载 |
| 明确status | 11852ms：新鲜CD读取ESP_OK，四个状态仍为0 |
| 明确mount一次 | 21397ms：Mount=ESP_ERR_NOT_FOUND (one attempt)；随后状态仍为0；整份日志没有sdcard_sdio挂载/SDMMC初始化记录，与挂载前CD拒绝路径相符 |
| 明确stop | 29008ms Unmount=ESP_OK（无挂载清理），29009ms HAL cleanup=ESP_OK，29010ms STOPPED，29011ms app_main返回 |
| 主机退出 | 确认STOPPED后EOF，控制台exit0/串口关闭；不发送额外挂载、重试或显示指令 |

通过范围：取卡后的新启动无卡CD、单次挂载拒绝以及无挂载状态的HAL安全退出。结合上轮有卡记录完成本轮有卡/无卡挂载门禁及正常退出闭环；这是卸载后取卡，不是挂载中热拔，不能据此宣称掉卡/坏卡/满卡/错误恢复、文件读写/回读、照片链路或P3 MVP通过。

本轮只修改状态文档，无代码或配置更改，未重跑主机测试/固件编译；沿用此前分层证据，不把实板无卡验证计为软件回归。日志在忽略目录 `.cache/p3-sd-hardware-EdGEy4/no-card-console-private.log`，不提交原始设备日志。设备仍是3168a7cc… SD隔离诊断，最终STOPPED、卡已取出、串口关闭，无活动白参考。下一步需明确限定测试文件及读写范围，现有入口只有status/mount/stop，不自动扩大操作。无提交/推送。

## 2026-10-04 · P3 限定文件写入/回读软件候选（未烧写）

用户对“在已备份测试卡上新建专用测试文件，不覆盖现有文件、操作原片或格式化”回复“允许”。本轮实现其必要的独立诊断代码并验证，没有将许可扩大为已有文件覆盖/删除、完整分区迁移、显示或NVS/PMU配置写。当前卡已取出，需要下一次人工操作；本轮未打开串口、复位、烧写或访问实卡。

### 实现与边界

新增便携`main/sd_file_probe.c/.h`，以有界同步IO回调驱动；POSIX/VFS与新鲜介质检查留在独立SD入口。新增精确file-test命令，设备解析器16字节和主机storage白名单均不接受路径参数/前缀/控制字节，默认display白名单保持原样。Kconfig说明更新，完整PhotoFrame集成不编入文件测试核心或执行此入口。

固定8.3文件`/storage/RC3T0001.BIN`：O_WRONLY|O_CREAT|O_EXCL独占新建4096字节，256字节分块内容由绝对offset确定。已读取锁定IDF v6.1的fat_mode_conv映射O_CREAT|O_EXCL→FA_CREATE_NEW；fsync→f_sync，close→f_close/file_cleanup，未猜测SDK接口。不存在truncate、append、unlink、format或目录枚举/用户路径选项，不扫描私人文件。冲突报告CREATE_FAILED/EEXIST，不读取或更改原内容。

每启动最多一次文件尝试，含无卡、无挂载、冲突或错误；不自动挂载/重试。每个操作前检查CD/mounted/media_valid/exit_failed；写完fsync并关闭，随后只读重开，完整逐字节核对及EOF，再关闭/最终状态检查。正常短传输处理有界，零/负/越界返回、缺字节、损坏或多尾字节均拒绝；所有已打开句柄在返回前close一次。主错误与额外关闭失败分开报告；close错误另锁存、阻止stop正常卸载/HAL拆除，FAILED终止命令，租约保留直到另行受控重启，不重试句柄或伪报STOPPED。

新建文件无论成功/失败均保留（部分写入也保留），不清理或更换名字绕过冲突。只验证本次fsync/关闭/重新打开路径，不声称卸载重挂、重启/断电持久性、满卡/热拔、所有容量、照片链路或并发消费者通过。同步卡驱动没有新增硬取消/超时契约，不承诺stop能打断未返回的IO。设备开机仍不自动挂载或文件IO，不启屏/NVS/网络，不写PMU校准或HOST_READY。

### 分层证据

| 检查层 | 本轮结果 |
| --- | --- |
| 新增主机用例 | 19项：完整字节/EOF、正常短传输、无卡/冲突、一次性门禁、坏写/读、sync/close/reopen失败、损坏/短文件/额外尾字节、37个介质失效点、非法回调，以及实际主机POSIX独占创建/冲突保留；不访问实体SD |
| 普通CTest | 375/375，7.78秒 |
| ASan/UBSan | 375/375，37.83秒，无sanitizer报告 |
| Python构建/控制台 | 30/30，1.874秒；新增1项file-test参数/前缀/控制字符拒绝，现有storage/默认display用例同步扩展；使用替身，无串口访问 |
| 固定SDK编译 | SD诊断、完整业务隔离集成、原WaveShare成功；本轮未复跑其余三个Read Pico profile或Web测试 |
| 厂家来源 | 子模块clean，仍28cde682a4468a581c278761922724f57d976418；驱动、波形、频率、VCOM不改 |
| 镜像离线检查 | ESP32-S3/DIO/16MB/80MHz/MMU64KB、secure version0、checksum/hash有效；这不是实机烧写回读 |
| 烧写/分区回读 | 未执行，本轮设备当前应用仍为320,624字节/3168a7cc… |
| 实板文件IO | 未执行；此前有卡/无卡挂载证据不转移为file-test验收 |

新SD候选322,848字节，SHA256 `6b85769bc32b65a4fb6051651062c43498dd3c463b85dfcb95c004034d2a9b77`，ELF SHA `b970e4596eb3eb36a2982a246a84f10502be4bd6721052245f4dea08047589f7`，sdkconfig SHA `bbc74531c50d30d1eef3a94b7ebc3681c54b90cd012f9bf87288c369e18984b1`。本次原板与integration产物SHA分别仍为031c74cc…与5893eb3c…，integration从未烧写、不开放产品启动。

初次SD编译因新系统头文件误置于源码末尾失败，移至文件顶部后重新编译成功，再做复核构建；不将修复前失败隐藏为首次即通过。日志保留于忽略目录`.cache/p3-file-probe-*`，包括host/asan配置构建和测试、python、firmware-build初次与final/reviewed、storage-final-build、integration/original-build和image-info；不提交构建产物或私有日志。未修改Git index、无提交/推送。

下一步人工关口：重新插入已备份、没有重要数据的测试卡，明确本轮仅factory诊断应用更新与全长回读/分区保持；再新会话status→单次mount→单次file-test→status→stop，观察所有句柄关闭、卸载/HAL退出，终态后关闭串口。若发现同名文件即拒绝，不自行删除/覆盖或改名重试。详见 [SD说明](READ-PICO-STORAGE.md)。

## 2026-10-04 · P3 限定文件诊断部署与4KiB实板回读通过

用户回复“已插入，确认允许更新”，承接此前已允许的新建专用测试文件及仅factory诊断应用更新/回读范围。本轮仅更新该应用，并在新启动执行一次挂载/file-test/退出；不扩展到已有文件覆盖/删除、格式化、照片、显示、完整产品/分区迁移或NVS/PMU配置写。

### 写前核对与应用更新

复制上一轮已编译/测试的322,848字节候选到本轮忽略目录作为固定输入，SHA256仍为`6b85769bc32b65a4fb6051651062c43498dd3c463b85dfcb95c004034d2a9b77`。image-info确认ESP32-S3、DIO/16MB/80MHz、MMU64KB、secure version0、checksum/hash有效，ELF SHA为`b970e4596eb3eb36a2982a246a84f10502be4bd6721052245f4dea08047589f7`。配置storage=y、factory表、80MHz PSRAM，显示/integration未启用。本轮没有重编译或修改固件源。

确认单一USB设备端口、串口无占用；固定SDK Python/esptool5.4.0在ROM/no-stub核对身份，私有MAC与上轮一致（两份记录各一条，不公布标识）。S3 v0.2/QFN56、8MB AP_3v3 PSRAM、Flash16MB/5e4018、Secure Boot和Flash Encryption Disabled、crypt count0。生成分区表3072字节与实机回读4096字节的前3072字节经明确复制后cmp/SHA一致；整扇区与上轮partition-after完全一致，SDK解析仍为NVS0x9000/0x5000、PHY0xe000/0x1000、factory0x10000/0x400000、内部FAT0x410000/0x500000。

按已知旧应用长度320,624字节整长读取，cmp及SHA与3168a7cc…旧SD诊断一致，保留本地恢复副本；不是完整Flash或移植前原应用备份，不承诺完整产品回滚。

仅write-flash目标0x10000，keep flash参数/no-compress，无全片擦除、flash_args、bootloader或分区/数据区写入。工具报告322,848字节/4.0秒、Hash verified；应用写入范围`[0x10000,0x5ed20)`、扇区擦除`[0x10000,0x5f000)`。

| 核对层 | 实际证据 |
| --- | --- |
| 独立应用整长回读 | 322,848字节，cmp与固定输入完全一致；两份SHA均6b85769bc32b65a4fb6051651062c43498dd3c463b85dfcb95c004034d2a9b77 |
| 分区表前/后 | 4096字节cmp一致；SHA均f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20 |
| 保护范围 | 写入/擦除仅factory应用，未触及bootloader/分区/NVS/PHY/内部FAT；没有对所有这些数据区做完整内容前后备份校验 |

### 新启动与单次文件验证

显式ESP USB reset（不是PMU reset）后，保留bootloader加载新app，ELF前缀b970e4596…匹配，PSRAM memory test OK。隔离入口声明不开机挂载/文件IO/格式化、无显示/NVS/网络；594ms启动CD观察ESP_OK、present=1/mounted=0/media_valid=0/exit_failed=0，596ms READY，之后才明确发送命令。

| 明确操作 | 实板结果（时间均自本次ESP启动，不是操作耗时） |
| --- | --- |
| status | 9149ms新鲜CD仍present=1，尚未挂载 |
| mount一次 | 10391ms初始化既有SDMMC，width1、CLK38/CMD42/D0=44、D1–D3=-1；10448ms挂载/storage成功，10450ms Mount=ESP_OK，10452ms状态present/mounted/media_valid=1、exit_failed=0 |
| file-test一次 | 19013ms File test=OK、created=1、written=4096、verified=4096、close_failed=0、errno=0、close_errno=0；19015ms报告保留/storage/RC3T0001.BIN；19017ms介质状态仍有效 |
| status | 29177ms新鲜CD/挂载/介质有效，exit_failed=0 |
| stop | 30422ms真实backend卸载，30424ms Unmount=ESP_OK，30425ms HAL cleanup=ESP_OK及STOPPED，30426ms app_main返回 |
| 控制台退出 | 确认终态后EOF退出，exit0，串口关闭；无补发/重试/重新挂载，端口占用检查为空 |

结合已读取的同一固件实现及成功结果，此次单张专用文件完成独占创建、4096字节写、fsync、关闭、只读重开、逐字节比对与EOF长度检查、再次关闭和最终介质检查。没有目录枚举、私人文件内容读取、已有文件覆盖/删除、自动改名或格式化。测试文件留在卡上，卡仍插入但已卸载；不自动清理，即使后续请求遇到同名也应拒绝。

通过范围只限本卡此次4KiB关闭/重开完整性与正常退出。不能代替卸载重挂/重启/断电持久性、满卡/坏卡/热拔恢复、不同卡型号/容量、真实照片/并发消费者或P3 MVP验收。旧3168a7cc…无卡证据仍属旧镜像，不移作当前文件镜像的无卡验收。当前SD固件不刷新屏幕、没有活动白参考，旧保留画面不证明显示运行；完整PhotoFrame集成镜像未烧写。

### 留档与后续

本轮只有状态文档更新，不重跑软件测试/编译；此前软件轮次375普通/375 ASan/UBSan、30 Python、SD/集成/原板编译证据保留，不将此次实板结果冒充软件重跑。厂家固定子模块不改，无提交/推送，保留原有工作树修改。

固定输入、旧应用副本、整长回读、分区前/后与前缀、image-info、设备安全/身份及控制台私有日志存于忽略目录`.cache/p3-file-hardware-7lH0bJ/`，不提交原始设备标识或卡信息。当前应用为6b85769b…/322,848字节，最终STOPPED/串口关闭。下一步先设计只读重挂/重启核对已保留专用文件的入口；现有file-test不能用于读回已有文件，不能通过删除/覆盖同名文件绕过门禁。之后另明确照片测试样本和路径，未在本轮执行。

## 2026-10-04 · P3 file-check只读持久性核对候选（未部署，需人工确认）

继续“开发直到需要人工介入”的目标。上一轮属于明确进展：已获准部署6b85769b…并完成实卡4KiB文件写/关闭/重开及安全退出；本轮重新读取TODO、当前源码、前轮整长回读摘要和完整STOPPED日志，确认下一依赖是保留文件在新启动/重新挂载后的核对，不把原有关闭/重开冒充持久性通过。没有依据归档报告或旧许可自动操作新候选。

### 只读能力与保护

新增`sd_file_probe_check`，独立`sd_file_probe_read_io_t`仅提供media_ready/open_read/read/close，根本不传递创建、写入或sync回调。实际隔离入口固定O_RDONLY打开/storage/RC3T0001.BIN，不接受任意路径、不扫描其他文件；新增精确file-check命令及storage主机白名单，默认display白名单不变。16字节解析器继续拒绝超长/前缀/控制字节/参数。

与原file-test共享逐字节/EOF核对核心，保留已有内容公式及4096字节格式，未新增不同格式或事后修复。验证结果只在内容完整、EOF正常、关闭成功且最终介质有效时OK；文件缺失READ_OPEN_FAILED/errno、短文件/坏读READ_FAILED、损坏MISMATCH、尾字节EXTRA_DATA均失败，不创建或重建。即使verified已达4096，EOF读取错误也失败。

attempted与check_attempted分别限制每种命令每启动一次，包含失败；无自动挂载/重试。关闭失败共用close_failed锁存，阻止后续两类文件操作和stop正常卸载/HAL拆除，主错与关闭错不相互覆盖。实际文件消费者仍同步且句柄在返回前close一次；没有新增后台热插拔或安全硬取消，不承诺可中断卡驱动挂起。新入口不开机挂载/文件IO、不启屏/NVS/网络、不写PMU/VCOM校准、不开放完整产品。

### 软件与构建证据

| 层次 | 本轮实际结果 |
| --- | --- |
| 新增主机用例 | 13项只读核心用例：完整内容/EOF且无写、缺文件不创建、无卡、重复命令、损坏不修复、短/多尾文件、短/零/负/越界读、全部19个介质失效点、EOF错误、close失败、主读/close双错、写close错误阻止读、非法回调；既有实际POSIX测试扩展新会话只读核对已有文件 |
| 原写测试 | 原19项继续覆盖同一共享核心，独占创建/37个介质失效点/关闭回读行为通过，不因重构删除原保护 |
| 普通CTest最终 | 388/388，7.85秒；补EOF错误用例前387/387也通过 |
| ASan/UBSan最终 | 388/388，39.19秒，无sanitizer报告；补用例前387/387、38.18秒通过 |
| Python构建/控制台 | 31/31，1.898秒；新增1项file-check参数/前缀/控制字节拒绝，既有两profile互斥及无自动指令用例同步扩展；均替身，不打开串口 |
| 固定IDF v6.1编译 | SD诊断和完整业务隔离集成成功，SD最终再编译通过；本轮未复跑原板/其余Read Pico profile、Web或远端CI |
| 固定上游 | 厂家28cde682a4468a581c278761922724f57d976418工作树clean，驱动/波形/扫描/VCOM/频率不改；依赖锁无新增版本/hash修改 |
| 离线镜像检查 | 323,424字节，ESP32-S3/DIO/16MB/80MHz/MMU64KB、secure version0，checksum/hash有效；不是实板回读 |
| 实板动作 | 无串口连接/复位/烧写/挂载/文件IO或显示；前轮实卡结论不转移为当前候选 |

新只读候选SHA256 `d62fd8d2fddc564eb3a5b9201e5d9d0385c22e9f49c56d8176393957a605fea0`，ELF SHA `69f3f64fd29575ec174ab8d3e11582c0cdb48dda7dee969de978e873bf1c9233`，sdkconfig仍`bbc74531c50d30d1eef3a94b7ebc3681c54b90cd012f9bf87288c369e18984b1`。集成镜像SHA仍5893eb3c…，未烧写且运行门禁保持关闭；实际设备最后核验仍6b85769b…/322,848字节，前轮回读副本摘要重新核对一致，日志终态为STOPPED，无活动白参考。

软件/构建/image-info日志在忽略目录`.cache/p3-file-check-*`，最终388项日志为host-tests-final/asan-tests-final；首次及补用例前结果亦保留。不提交构建生成物/私有日志，无提交/推送，保留原有工作树修改。

### 人工关口与自主开发完成边界

已完成本阶段全部可自主执行的实现、便携失败/状态测试、固件编译、镜像身份与状态文档核对，没有仍在运行或等待观察的工具句柄。下一关键验证需要把新d62fd8d2…应用写入真实设备、ESP复位和FAT重挂；此前用户确认仅针对旧6b85769b…单次更新/文件测试，不推导为长期任意诊断更新许可。因此已到“继续开发直到需要人工介入”的边界，需本轮明确仅factory更新/整长回读/分区保持及一次只读核对会话确认；不是R-Core-3、P3或持久性已完成。

获准后拟status→mount一次→file-check一次→status→stop，验证前轮保留文件在ESP重启/重挂后完整，不发送file-test，不写/删除/修复测试文件或读取私人照片。ESP USB reset不等于SD卡冷断电；断电持久性须在完整卸载后另行人工操作。若只读核对失败，保留原文件和错误证据，不自动重建掩盖失败。照片样本、显示门禁、完整产品/NVS/OTA迁移和PMU策略仍按原TODO分阶段验收，不跨越本轮许可。

## 2026-10-04 · P3 file-check部署及ESP重启/重挂只读核对通过

用户本轮“允许”明确对应“仅更新factory诊断应用，再执行一次重启后的只读核对；不重写测试文件或刷新屏幕”。重新读取TODO、适用协作规则、实际控制台及固定O_RDONLY接口，唯一USB端口空闲后执行。此许可不推导为完整产品迁移、冷断电、照片或追加刷新授权。

### 软件/编译与镜像身份

本轮没有固件源或配置更改，不重跑主机测试/固件编译；沿用上一节实际388/388普通、388/388 ASan/UBSan、31/31 Python以及SD/隔离集成编译证据，与本节部署/真机证据分开记录。复制已编译候选作为不可变输入，323,424字节，SHA256 `d62fd8d2fddc564eb3a5b9201e5d9d0385c22e9f49c56d8176393957a605fea0`，ELF SHA `69f3f64fd29575ec174ab8d3e11582c0cdb48dda7dee969de978e873bf1c9233`，sdkconfig SHA仍`bbc74531c50d30d1eef3a94b7ebc3681c54b90cd012f9bf87288c369e18984b1`。

本轮image-info确认ESP32-S3、DIO/16MB/80MHz、MMU64KB、secure version0、checksum/hash有效；storage启用、display/integration未启用，完整集成应用未烧写。未修改固定厂家驱动/波形/校准。

### 烧写与独立回读

| 检查 | 实际结果 |
| --- | --- |
| 设备身份 | 与前轮私有记录同机匹配；ESP32-S3 QFN56 rev0.2、8MB AP_3v3 PSRAM、16MB Flash，manufacturer5e/device4018，不公开MAC或卡标识 |
| 安全状态 | Secure Boot/Flash Encryption disabled，cryptcnt=0 |
| 写前旧应用 | 0x10000整长读取322,848字节，与6b85769bc32b65a4fb6051651062c43498dd3c463b85dfcb95c004034d2a9b77固定输入cmp一致；保留该诊断app副本，不是完整Flash备份 |
| 分区兼容 | 写前4096字节表与前轮一致，前3072字节与本次构建表一致；factory仍0x10000/0x400000，其余nvs/phy/storage布局不变 |
| 写入 | 仅应用`[0x10000,0x5ef60)`，工具实际擦除`[0x10000,0x5f000)`；323,424字节、Hash verified、exit0；keep模式/频率/容量，无stub，无全量flash |
| 独立应用回读 | 再读取323,424字节，cmp与固定输入完全一致，两份SHA均d62fd8d2fddc564eb3a5b9201e5d9d0385c22e9f49c56d8176393957a605fea0 |
| 分区保持 | 写后4096字节与写前cmp一致，SHA均f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20 |

未写bootloader/分区表/NVS/PHY/内部FAT或PMU校准，不宣称逐字节核验了全部未写分区。端口相关操作串行进行；烧写/读取完成后才开启控制台，不因等待而重复烧写。

### 真机新启动单次只读核对

使用storage profile显式ESP USB reset；启动ELF前缀69f3f64fd…匹配，PSRAM识别8MB，595ms READY且present=1/mounted=0/media_valid=0/exit_failed=0，没有自动挂载或文件操作。显式命令仅status→mount→file-check→status→stop：

| 本启动日志时间 | 动作与结果 |
| --- | --- |
| 20120ms | status：ESP_OK、present=1/mounted=0/media_valid=0/exit_failed=0 |
| 21397–21457ms | 唯一1-bit挂载，CLK38/CMD42/D044，Mount=ESP_OK；present=1/mounted=1/media_valid=1/exit_failed=0 |
| 29986–29991ms | 唯一file-check：OK、created=0/written=0/verified=4096、close_failed=0/errno=0/close_errno=0，逐字节/EOF核对及关闭成功；最终介质有效 |
| 38763ms | status：ESP_OK、present=1/mounted=1/media_valid=1/exit_failed=0 |
| 40048–40052ms | backend卸载、Unmount=ESP_OK、HAL cleanup=ESP_OK、STOPPED，app_main返回 |

终态后主机EOF关闭，控制台exit0、端口无占用。测试卡仍插入且已卸载，RC3T0001.BIN保留。没有file-test、文件创建/写入/修复/删除、扫描私人目录、SD格式化、NVS/网络/显示/PMU配置写。应用核对只以O_RDONLY打开固定文件，但FAT挂载不是硬件写保护，不宣称元数据绝无变化。

本轮证明前轮创建的专用文件在**ESP重启及FAT重新挂载**后仍完整；ESP USB reset没有切断SD卡供电，不计SD冷断电持久性、热拔/坏卡恢复、照片闭环、全部卡容量或P3 MVP完成。冷断电须完整卸载后另安排人工；未来照片样本/测试路径与产品迁移仍按TODO独立门禁，不自动继续。

固定输入、旧应用副本、应用回读、分区前/后/前缀、image-info及身份/安全/控制台私有日志保留在忽略目录`.cache/p3-file-check-hardware-wNdZWm/`，不提交原始设备标识、卡信息或构建产物。本轮仅同步状态文档，保留其他工作树修改，无提交/推送。当前应用d62fd8d2…/323,424字节，STOPPED/串口关闭，无活动白参考。

### “开发直到需要人工介入”目标完成审计

后续目标检查将上一轮归为进展：实际更新了设备应用并取得新启动读取证据，不是仅重复状态。重新核对TODO/需求基线/集成门禁、固定输入与实板整长回读cmp/SHA、分区前后cmp，以及私有日志中4096字节核对、卸载/HAL成功、STOPPED和app_main返回；端口无占用，当前没有烧写或控制台进程可继续等待。既有388普通/388 sanitizer、31 Python及两个固件编译成功日志仍可查，此次未重跑，未把它们当冷断电或产品验收。

当前阶段已实现、测试、编译并部署所需只读入口，继续验证冷断电必须人工让SD卡真正断电；ESP复位不能替代该操作。照片链路尚需用户明确可使用的样本/路径，完整产品入口仍需数据迁移、显示参考和PMU策略关口。不自选私人文件、不擅自操作供电或启用产品来绕过依赖。因此已达到原目标的“需要人工介入”终点，仅完成本次自主开发目标，不宣称P3、第一阶段或整个项目完成；本次审计没有新增设备动作。

## 2026-10-04 · P3 单次人工取卡断电/重插后保留文件只读核对通过

用户同意继续冷断电验证后，先重新核对上次Unmount/HAL均ESP_OK、STOPPED、app_main返回及端口无占用，再要求将测试卡完全取出、重新插入以脱离供电，不修改RC3T0001.BIN。用户回复“已重新插入”后，本轮仅执行已说明的新启动/单次挂载/只读核对/安全退出，不把该回复扩大为烧写、照片、显示或完整产品许可。

断电动作由人工按上述要求操作并确认，固件没有测量SD供电电压或断电持续时间；不把单独ESP复位当卡断电。只能据此记录本测试卡单次取卡断电后数据保持，不宣称整机冷启动、写入中异常断电、任意卡容量、热拔恢复、长期保持或P3 MVP通过。

### 镜像、软件与烧写边界

实际使用此前已部署并全长回读的d62fd8d2…/323,424字节只读诊断，不更改固件源/配置、不编译、不烧写或再次ROM回读；本启动ELF日志前缀69f3f64fd…匹配已部署镜像，8MB PSRAM识别正常。本轮只有状态文档更新，不重跑此前388普通/388 sanitizer、31 Python及SD/隔离集成编译，也不将此前回归或回读称为本轮重做。保留完整产品/显示/NVS/网络/PMU配置写门禁。

### 真机顺序与结果

已核验USB端口空闲，storage profile显式ESP USB reset，只手动发送status→mount→file-check→status→stop。开机595ms READY，present=1/mounted=0/media_valid=0/exit_failed=0，无自动挂载或文件操作。

| 本启动日志时间 | 实际结果 |
| --- | --- |
| 16051ms | status：ESP_OK、present=1/mounted=0/media_valid=0/exit_failed=0 |
| 17306–17366ms | 唯一1-bit挂载，CLK38/CMD42/D044，Mount=ESP_OK；present=1/mounted=1/media_valid=1/exit_failed=0 |
| 24525–24530ms | 唯一file-check：OK、created=0/written=0/verified=4096、close_failed=0/errno=0/close_errno=0；逐字节/EOF核对及关闭成功，最终介质有效 |
| 33749ms | status：ESP_OK、present=1/mounted=1/media_valid=1/exit_failed=0 |
| 34989–34993ms | backend卸载、Unmount=ESP_OK、HAL cleanup=ESP_OK、STOPPED，app_main返回 |

STOPPED后主机EOF退出，控制台exit0，端口无占用。专用文件保留，测试卡仍插入且已卸载。没有file-test、文件创建/写入/修复/删除、扫描私人目录、格式化、NVS/网络/显示或PMU配置写。核对只以O_RDONLY打开固定文件；FAT挂载不是硬件写保护，不宣称元数据绝无变化。没有自动重试或第二次核对。

私有日志保存在忽略目录`.cache/p3-card-power-cycle-PkxJjQ/console-private.log`，不提交设备或卡标识。TODO及当前状态同步更新，保留用户其他修改，无提交/推送。下一人工关口为明确获准照片样本、测试路径及照片链路验证范围；本轮不自选私人照片或开放完整集成应用，不进行整机/异常断电试验。

## 2026-10-04 · P3 获准相机原片只读解码候选（未部署）

用户明确指定本机样图目录，并随后说明这些照片均为相机导出原片。本轮只在该授权目录读取5张JPEG，不改写/删除、重压缩、嵌入或提交原片，不推断相机型号/固件，不开启相机协议。读取TODO/需求/架构、实际JPEG处理与锁定解码器、SD入口和命令/测试源码后实施；fast-context按要求优先尝试，但无Windsurf API key而失败，改为限定rg及逐段实际源码核对，不抽取凭据或将搜索结果当作接口已验事实。

### 样本审查与直接原片内存边界

5张均为baseline RGB JPEG，字节数为9,464,714/16,702,316/10,483,728/14,582,954/11,867,635；尺寸分别6192×4128/5935×3957/6192×4128/6192×4128/6192×4128。精确EXIF方向为6/1/6/6/6：值6需要90°方向处理，不是早期file工具概述带来的镜像判断。Pillow实际load全部5张通过；它不是设备解码器，不将此计为板端通过。

当前PhotoFrame文件处理先整体分配压缩文件，再分配缩放RGB；全部样本压缩文件本身即超过8MiB PSRAM。因此不能直接承诺旧产品路径支持这些原片，不凭缩放RGB尺寸忽略压缩副本内存。当前main/image_processor.c没有EXIF解析，正式方向/裁切/灰阶/照片呈现未验收。本轮建立只读分块解码探针，不通过主机重压缩或降尺寸替代原片支持，也不宣称已解决整个产品大图路径。

本机按文件名排序编号S1–S5，仅私有清单保存源路径/文件名。原片5份SHA256在实际检查前后cmp一致；S1是最小文件，准备到忽略目录`.cache/p3-photo-sample-G0r1fc/RC3P0001.JPG`，与原片全长cmp一致，保留其原始编码/EXIF，未放入SD或固件。

### 实现及可验证范围

新增jpeg_stream_probe.c/.h，read/media_ready回调无创建/写入能力；分块输入和跳过段均实际读，最大256字节，短读可继续、负/超界/介质错误锁存。固定3100字节工作池及有界栈对象，不分配整压缩文件或RGB/framebuffer。输出只计矩形范围、块/像素数及block-order RGB CRC，像素随即丢弃，不是row-major图像指纹或视觉验证。

新增jpeg_file_probe.c/.h：open_read/file_size/read/close固定来源接口、每启动一次（无卡/缺文件/工作池失败也消耗机会）；非空≤32MiB、源边≤16384且≤64M像素，固定1/4缩放。解码后继续读完源文件剩余字节并检查EOF，再close一次、最终介质核对。tail/EOF/关闭错误不能成功，关闭失败单独锁存并保留主错。上述边界是拒绝保护，不是已支持的所有图片大小/格式上限。

storage隔离入口新增精确photo-check，只以O_RDONLY打开/storage/RC3P0001.JPG，不接受任意路径、不扫描其他文件；工作池内部堆分配、每128次读回调让出调度。与4KiB探针共享关闭失败保护，失败时stop拒绝正常卸载/HAL退出，不自动修复/重试。默认display主机白名单不变，storage白名单新增photo-check并拒绝参数/前缀/控制字节。photo-check只验证读取及解码，不执行EXIF/Cover/灰阶/dither/显示/NVS/网络，不进入完整PhotoFrame业务。

板端使用固定IDF v6.1 ESP32-S3 ROM TJPGD；nm确认jd_prepare=0x40000858、jd_decomp=0x40000864。主机使用已解析esp_jpeg 1.3.1外部TJPGD，来自固定组件repository commit746e83ddbea0db9c3d24993a87c4c737a60337ae；没有修改依赖源。可选主机目标默认OFF，显式开启后检查版本及三个源码/头文件SHA：tjpgd.c=1286f5e32e7a4750ee36fd57d2c442c99a74ffed0e81cb5d2fbf92fccf64a84d、tjpgd.h=53d1156fd9fcc97d346e5ee8c16ae39c34d71c691dcad3c524424bb91519cd24、tjpgdcnf.h=6a2134a4aae53bc361a7c4f1bfc8ef0107452191d6c4ae26bd1030ca441459ae，漂移即拒绝，不自动下载JPEG/照片。保留ChaN许可/来源，未复制或编辑第三方源码。

### 主机真实解码、回归与编译（与真机分开）

| 检查 | 实际结果 |
| --- | --- |
| 主机外部TJPGD读取/解码5原片 | 全部file=OK/decode=OK/decoder=0，完整bytes_read=file_size、EOF及关闭成功；S1/S3/S4/S5输出1548×1032/pixels1597536，S2输出1483×989/pixels1466687；仅首幅缩放像素，不验证尾部/多帧结构 |
| S2完整源与codec分离 | codec_read=7,406,080而file_size/bytes_read=16,702,316；补读尾部及EOF，不能把剩余字节说成已解码，不猜测尾部内容 |
| 新增21便携/实际外部解码用例 | 4种缩放、奇数尺寸、短读指纹一致、无效参数/无卡/负数及超界/中途掉卡/截断/零读；生命周期成功/一次门禁/缺文件/工作池与尺寸失败、正常及失败close锁存、主读错误不被close错覆盖、来源尺寸限界、尾部/EOF失败；自动测试只用已有非私人fixture，不读私人目录 |
| 普通CTest | 409/409、9.90秒；显式READ_PICO_JPEG_STREAM_TESTS=ON，包含原有388和新增21 |
| ASan/UBSan CTest | 409/409、48.84秒，同样开启真实外部解码目标，非硬件替代证明 |
| 获准5原片ASan/UBSan实际CLI | 各自file/decode=OK、完整源读取/EOF/关闭成功；无AddressSanitizer或runtime error诊断，日志另存samples-asan-private.log；私人样本不加入自动测试/CI |
| Python构建/控制台 | 32/32、1.945秒；新增photo-check精确白名单拒绝用例，均fake serial，无设备操作 |
| SD诊断编译 | 固定ESP-IDF v6.1成功，storage=y/display关闭、CONFIG_JD_USE_ROM=y，probe实链；本轮仅编译，不执行打印的全量flash命令 |
| 完整集成编译 | 成功、镜像SHA仍5893eb3cacb6a7073f8b3ec27ce5c1a721cf54b205928b6e7e04b419505e7ced；未烧写、产品入口仍关闭 |

本轮没有重跑原板/其余Read Pico配置、Web或远端CI。最终存储构建保留SDK自动解析的dependencies.lock：只增加direct_dependencies中的idf并将manifest_hash更新为d87481ea…，组件版本和component_hash均无变化，不手工伪造锁记录。主机外部TJPGD与板端ROM不同，不把主机通过/CRC当ROM行为、内存峰值、SD读取或实屏成功；解码器不是严格JPEG/EOI/多帧结构验证器，SDK同步阻塞也没有安全硬取消保证。

### 未部署镜像与下一人工关口

新候选325,904字节，SHA256 `d760371b5e6da1e07df2a285bca31945213e937a58f243dfee7c3dca660f65d5`，ELF SHA `93c03a6c595731cc2cab9236c0d4b4cb85da09d7080bfc44fc8957fb50657c03`，sdkconfig SHA仍bbc74531c50d30d1eef3a94b7ebc3681c54b90cd012f9bf87288c369e18984b1。image-info确认S3/DIO/16MB/80MHz/MMU64KB、secure version0/checksum/hash有效。拟写应用[0x10000,0x5f910)、擦除[0x10000,0x60000)，尚未执行。设备仍是前轮已回读d62fd8d2…/323,424字节、STOPPED/串口关闭，端口无占用。

下一步需人工将S1字节一致副本复制到已安全卸载的测试卡根目录RC3P0001.JPG（同名先报告、不覆盖/删除），安全弹出并插回，再明确允许本轮新factory应用更新/独立全长回读/分区保持及一次只读解码。拟status→mount一次→photo-check一次→status→stop，不发送file-test、不清白/刷新/改校准或产品启动；错误保留，不重建文件或自动重试。不复用旧镜像的单次更新许可。

全部样本路径/哈希/副本、像素指纹及清单在忽略目录；软件日志为.cache/p3-jpeg-{host,asan}-build/tests-final.log、python-final.log、firmware-build-final.log、storage-last-build.log、integration-build.log、image-info.log及samples-final-private.log。README/TODO/资料索引及当前状态同步；本轮无串口、复位、烧写/回读、真实SD操作、显示、NVS或PMU配置写，无提交/推送，保留用户其他修改。详见 [照片检查](READ-PICO-PHOTO.md)。

## 2026-10-04 · P3 S1原片只读解码诊断部署与ROM单次真机验收

用户确认“已复制，允许更新诊断固件”，承接上一轮S1副本复制/插卡及明确单次factory更新、独立回读、一次只读photo-check范围。本轮没有源码/配置变更，不重跑或将此前409/409普通与ASan、32/32 Python及两配置编译改记为本轮结果。主机外部TJPGD与本次S3 ROM实机证据分开；完整产品仍禁止启动。

### 写前核对、仅应用写入与独立回读

端口首次检查无占用；新ROM读取的唯一设备身份与此前私有flash-id日志精确匹配（各一条身份记录，不公开标识），ESP32-S3 QFN56 rev0.2/8MB AP_3v3 PSRAM、16MB Flash manufacturer5e/device4018一致。Secure Boot/Flash Encryption均Disabled，SPI_BOOT_CRYPT_CNT=0。

从当前构建复制到本轮独立忽略目录的固定输入为325,904字节，SHA256 `d760371b5e6da1e07df2a285bca31945213e937a58f243dfee7c3dca660f65d5`，ELF SHA `93c03a6c595731cc2cab9236c0d4b4cb85da09d7080bfc44fc8957fb50657c03`；sdkconfig SHA仍bbc74531c50d30d1eef3a94b7ebc3681c54b90cd012f9bf87288c369e18984b1。再次image-info确认S3/DIO16MB/80MHz/MMU64KB、secure version0及checksum/hash有效。

写前完整读取旧323,424字节应用，与此前固定d62fd8d2…输入逐字节一致，保留previous-app.bin供恢复；这只是应用副本，不是整Flash备份。读取0x8000起4096字节分区表，与此前记录一致，前3072字节也与此次生成表一致：factory仍0x10000/0x400000，内部storage仍0x410000/0x500000。

仅write-flash 0x10000该固定应用，保留flash mode/size/freq且不压缩；实际写入325,904字节，工具hash verified。写范围`[0x10000,0x5f910)`，扇区擦除`[0x10000,0x60000)`。随后独立read-flash同地址全长325,904字节，cmp输入一致、SHA同d760371b…。再读完整4096字节分区表，cmp写前一致，前后SHA均`f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。不写bootloader/分区/NVS/PHY/内部storage/PMU校准，未使用构建器的全量flash建议。

### 新启动单次SD/照片检查与停止

使用storage控制台显式ESP USB reset，不是PMU或整机断电；控制台没有自动发送指令/重连/重试。实际发送顺序严格为status→mount→photo-check→status→stop，每项一次。新启动ELF前缀93c03a6c5…和8MB PSRAM匹配本次应用，开机不挂载/不读写文件/不启屏/NVS/网络。

| 新启动日志时间 | 实际结果 |
| --- | --- |
| 594ms | SD observation=ESP_OK，present=1/mounted=0/media_valid=0/exit_failed=0；READY含photo-check |
| 17645ms | 首次显式status同上，尚未挂载 |
| 26808–26880ms | 一次1-bit挂载，CLK38/CMD42/D0=44；Mount=ESP_OK，present/mounted/media_valid=1，exit_failed=0 |
| 114566–114572ms | 唯一photo-check完成：Photo check=OK/stream=OK/decoder=0；file_size=bytes_read=codec_read=9,464,714，source6192×4128、output1548×1032、pixels1,597,536/blocks199,692；close_failed=errno=close_errno=0，随后自动状态仍有效 |
| 125908ms | 最后显式status：ESP_OK、present/mounted/media_valid=1，exit_failed=0 |
| 132777–132781ms | backend卸载，Unmount=ESP_OK、HAL cleanup=ESP_OK、STOPPED，app_main返回 |

结果返回前没有再次发送photo-check或复位；检查为同步等待，未记录独立开始时间，不把挂载到结果的间隔称为精确解码耗时。STOPPED后主机EOF退出，控制台exit0、lsof端口无占用。测试卡仍插入且已卸载，RC3P0001.JPG保留，RC3T0001.BIN本轮未打开或重建。

本轮只有O_RDONLY打开固定照片、分块读/ROM缩放解码、完整文件与EOF读取及close一次，没有文件创建/写入/修复/删除、目录扫描或其他原片访问。用户复制到卡的来源确认与此前本机副本cmp保持为不同证据：此次未计算SD文件完整SHA，不把大小和解码成功当SD副本逐字节一致证明。FAT挂载不是硬件写保护，不宣称元数据绝无变化。

仅计S1在该卡/该设备的一次完整源读取、ROM 1/4解码与正常退出；像素统计后立即丢弃，没有EXIF=6旋转、Cover、灰阶/dither/framebuffer/清白或GC16，没有照片实屏验收。解码器结果不是严格JPEG EOI/多帧结构验证，也不证明全部5原片、所有大小/卡容量、损坏/热拔恢复、长期性能或产品大图链路通过。当前旧PhotoFrame整文件缓冲仍不能承诺支持大于8MiB的样本，后续软件需真实流式接入；完整产品/NVS/OTA迁移、新固件/显示须另行确认。

私有日志、固定输入及旧/新应用与分区回读保存在`.cache/p3-photo-hardware-ronKMZ/`，不提交设备/卡标识或私人照片。TODO/README/部署、存储、照片与相关当前状态同步；保留历史未部署/失败记录及用户其他修改。本轮无提交/推送，不自动追加硬件验证。

## 2026-10-04 · P3 文件JPEG EXIF/Cover/灰阶业务接入（仅软件，未部署）

用户“继续”后，在既有PhotoFrame目录和组件契约内继续照片软件开发。读取TODO/需求/架构、实际image_processor/display_flow、SD生命周期和固定TJPGD源码；fast-context优先尝试但因缺Windsurf API key不可用，改为限定rg及实际源码核对，不提取凭据。未启用相机协议、恢复旧自建相册或修改第三方驱动/波形。

### 软件交付与边界

- 新增便携`jpeg_metadata.c/.h`：只读有界随机读取，baseline SOF0/8-bit灰或三分量，解析APP1 TIFF II/MM IFD0方向，缺省1；非法类型/计数/偏移/重复方向及渐进JPEG拒绝。上限32MiB源、4MiB头、4096标记/IFD项、边16384/源像素64M是拒绝保护，不是支持能力验收。未访问缩略图/GPS/MakerNotes，不是严格EOI/多帧验证。
- `jpeg_stream_decode_rgb`复用探针的≤256字节压缩输入和3100字节工作池，将紧凑MCU tile拷贝到调用方行优先RGB；缓冲长度在decomp前检查，保留奇数尺寸floor行为。原probe仍只统计/丢弃像素；RGB业务函数不生成block CRC。
- Read Pico文件JPEG选择全/半/四分之一/八分之一中符合5MiB RGB预算的最小缩放，PSRAM保存该缩放RGB而非整压缩文件。EXIF 1–8由采样坐标映射处理，不增加旋转副本；方向后固定居中Cover，再复用既有重采样、CDR/灰阶/误差扩散与行列显示sink，不调整gamma、VCOM、频率或物理波形。
- 新文件解码读取完整源/剩余尾部并检查EOF，close一次及最终介质有效后才开始显示；失败不发布像素、不自动重试。显示提交前释放RGB，JPEG文件分支不受release_source=true影响而删除原片。JPG→PNG拒绝相同路径及常规硬/符号链接别名，但不是并发路径变化安全保证，也未验证FAT inode语义/原子输出。
- 新SD关闭失败钩子原子锁存到重启，阻止正常卸载/重挂，保留SDK挂载与HAL租约；不IO/重试不确定句柄。主机直接调用钩子验证，不宣称真实FAT fclose错误注入通过。完整集成启动门禁保持关闭。

### 主机回归与实际原片（不访问设备）

| 检查 | 实际结果 |
| --- | --- |
| 新增24项用例 | 元数据10：全部8方向/双字节序、独立非对称raster映射、坏EXIF/截断/读失败/渐进/头部限界/缩放预算；RGB3：短读像素一致、长度/空缓冲、奇数尺寸边界；业务10：全部8方向×2显示方向、9MiB压缩源无整文件分配、Cover覆盖FIT、坏图/无卡/中途介质失效/分配失败/32MiB限界、别名保护、JPG→PNG；SD1：关闭锁存拒绝卸载/重挂且不释放租约 |
| 普通CTest | 433/433，19.82秒，READ_PICO_JPEG_STREAM_TESTS=ON |
| ASan/UBSan CTest | 433/433，73.47秒，同样包含实际外部TJPGD及完整image_processor/display_flow替身测试 |
| Python构建/控制台 | 32/32，1.840秒，fake serial，不访问串口 |
| 5张获准原片普通及ASan业务CLI | 全部ESP_OK、替身1216×684输出，portrait/Cover/16灰阶码检查通过；只读输入、不生成私人照片输出文件 |
| 单次像素分配 | S1/S3/S4/S5：4,792,608字节（1548×1032×3）；S2：4,400,061字节（1483×989×3），不持有整压缩源副本 |
| 原片保持 | 5份源SHA在处理前/普通后及最终ASan后cmp一致；未重编码、删除、提交或网络上传 |

自动测试只使用既有非私人JPEG fixture，EXIF合成/大文件和坏图在测试临时目录创建，私人样本不加入CI。主机实际调用外部TJPGD，板端仍链接ESP32-S3 ROM；主机替身显示不是屏幕刷新，最大单次分配不是总峰值PSRAM。每128次压缩读回调让出调度，不承诺同步SDK/ROM硬取消或硬超时。

### 固件编译/链接（未烧写）

固定ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`，未改厂家子模块/组件版本；dependencies.lock本轮无差异。

| 配置 | 字节数 | 应用SHA256 | 结果 |
| --- | --- | --- | --- |
| Read Pico完整集成 | 1,978,080 | `cf0e40bcc43600444c0169151edad096742564898baeac922367530b1200e56a` | 编译成功，保留真实PhotoFrame入口但不调用产品 |
| Read Pico独立SD诊断 | 326,128 | `e05fc2ed60214350aecadb82ec34182e8329221c5fbc4328eaeec09c37e57537` | 编译成功，不含新照片显示命令 |
| 原WaveShare PhotoPainter 7.3 | 2,058,624 | `9edef163b3111e163d5805bd55087829e54232ac45d77ff125b43d3500d3c50d` | 原板编译成功 |

集成ELF SHA `d90edf7dc3c00c61f960b4ee763f61cbfd30098d49ed5ffdd926256b95327220`，sdkconfig SHA `884d6fbdb3a0faa365852520c0fd637a7928665c6acfe50819469ae173fa43c5`。image-info checksum/hash有效、S3/v6.1/MMU64KB/secure version0；nm确认新元数据/RGB/文件显示及SD锁存函数与photoframe_app_main实链，jd_prepare=0x40000858、jd_decomp=0x40000864仍为ROM地址。不把链接当运行或ABI真机验证。集成保留已知Read Pico guard下http_server.c:2229未使用restart_task告警，无新增图像处理告警。本轮未重跑其他诊断/板型矩阵、Web或远端CI。

### 硬件状态、剩余门禁与证据

本轮没有串口、复位、挂载/实卡IO、烧写/回读、启屏/清白/GC16、NVS/网络或PMU写。设备仍是上一轮已全长回读的d760371b…/325,904字节，STOPPED、卡插入且卸载、串口关闭；新增326,128字节SD构建未替换设备。前轮S1 ROM只读解码结论不转移为EXIF/Cover/灰阶或照片实屏通过。

当前完成范围仅Read Pico文件JPEG软件分支；旧内存输入JPEG、PNG压缩文件路径与格式探测/PNG输出/其他消费者关闭及并发控制仍待审查。新文件别名检查不能代替路径并发安全、消费者所有权或原子发布；不能称整个产品大图/损坏图/断电恢复已完成。下一步准备受控单张照片显示诊断，再取得新候选factory更新/清白/单张显示许可；不得直接烧写完整集成应用或复用此前单次许可。真机峰值内存、方向、裁切、SD读图性能和实屏结果分别验收，产品/NVS/OTA迁移门禁不开放。

日志位于忽略目录`.cache/p3-photo-pipeline-`前缀：full.log、asan-full.log、python.log、integration-build.log、storage-build.log、original-board-build.log；5原片普通/ASan与SHA日志后缀private，不提交私人路径/照片/哈希。TODO、照片/集成/存储/部署说明、架构/README/CHANGELOG已同步，git diff --check通过；保留原有未提交修改，本轮不提交或推送。

## 2026-10-04 · P3 单张原片显示诊断候选与输出保护（未部署）

用户要求继续开发直到需要人工介入。本轮仅软件实现、获准本机原片只读处理及构建，不将“继续开发”推导为新候选烧写/复位/挂载/显示或完整产品迁移许可。代码探索工具因缺少配置无法使用，已说明并以有限rg和实际源码核对；不尝试提取凭据。保留既有未提交修改，不修改固定厂家子模块，不提交或推送。

### 软件实现范围

- 新增隔离照片profile及构建/控制台保护，显式互斥display/storage/integration。启动仅板级只读观察、无NVS默认调色板与独占console，不挂载/不开屏、不进入完整PhotoFrame/网络/相册。
- 便携会话明确一次mount→clear-white→本启动人工white-confirm→一次photo-show→人工确认/拒绝→stop；错误锁存、拒绝重试/重入，主错误与退出错误分开。解析32字节有界ASCII白名单，非法/超长整行拒绝；清理只尝试一次，不重用不确定句柄。
- 诊断sink按完整1216×684行或反向portrait列封装415,872字节标准4bpp，预检16灰阶理论码/完整顺序，拒绝缺行/混合方向/坏调色板及发布。它不调用硬件或发布配置/快照，只在完成后由显式入口刷新一次。
- 单张入口固定只读`/storage/RC3P0001.JPG`（S1副本），复用真实文件JPEG/EXIF/Cover/Floyd-Steinberg及原PM锁GC16；检查78帧清白、36相、独立退出电源位，并记录SDK局部内存低水位。文件/帧失败不启屏，刷新后释放帧，再等待人工视觉反馈。
- image_processor默认调色板无NVS初始化；正常初始化传播加载错误。无效抖动参数提前拒绝；新板格式探测检查新鲜介质、读取/关闭错误，PNG输出关闭错误接SD原子锁存。输出以O_EXCL独占新建，既有路径不覆盖、错误清理不删除路径，可能部分输出保留；PNG输入release_source也不删除原片。固定SDK FAT源码实际O_EXCL→FA_CREATE_NEW映射已核对，但不计真实FAT故障/并发验收。

不改厂家源/波形/校准、VCOM/gamma/频率；诊断使用原PhotoFrame默认调色板，Read Pico图像校准效果尚未实屏验收。旧内存JPEG/PNG输入关闭与提交顺序、其他业务消费者/临时文件生命周期和并发、原子发布及产品/NVS/OTA迁移仍未完成。

### 主机与原片软件检查

| 检查 | 结果与边界 |
| --- | --- |
| 普通CTest | 458/458，21.09秒；较上一轮新增25项（会话/封装14、真实业务诊断桥6、参数/格式/输出与原片保护5） |
| ASan/UBSan CTest | 458/458，79.24秒；实际锁定外部TJPGD及替身业务，不是S3 ROM/硬件 |
| Python构建/控制台 | 37/37，2.422秒，fake serial、不访问串口 |
| 5张获准原片普通/ASan诊断CLI | 各自全部ESP_OK、415,872字节/831,744像素完整帧，仅读源、不输出照片文件/硬件IO |
| 单次像素分配 | S1/S3/S4/S5：4,792,608字节；S2：4,400,061字节，不计总峰值PSRAM |
| 原片保持 | 最终5份源SHA与处理前cmp一致，无删除/重编码/提交/上传 |

自动用例使用非私人fixture；5原片日志/哈希只保留忽略目录。SDK照片入口未用主机模拟运行，主机测试覆盖便携门禁/完整帧和真实图像算法，硬件入口只经源码审查与固件编译。局部内存低水位包含系统并发分配，多heap最低值差不是精确单任务峰值；ROM+SD+显示共存余量与性能待真机。同步SDK/ROM及固定EPDiy内部portMAX_DELAY无法硬取消，stop不能中断正在执行的刷新；新硬件许可须明确此风险。

### 固件编译、固定候选及未部署状态

固定ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`，六配置编译成功；不是完整板型/Web/远端CI矩阵。

| 配置 | 应用字节数 | 结果 |
| --- | --- | --- |
| 隔离单张照片诊断 | 403,168 | 最终e7d5bd8d…候选，只编译 |
| 完整PhotoFrame集成 | 1,978,208 | `43d8c1c8833e9fc76d136f4392f8252b76d03a39046ac6304000abaa88c34c4b`，运行门禁关闭 |
| 原显示诊断 | 305,120 | 只编译，没有照片命令 |
| 原SD诊断 | 326,128 | 只编译，没有照片显示 |
| factory只读诊断 | 275,344 | 只编译，不挂载/启屏 |
| 原WaveShare PhotoPainter 7.3 | 2,058,752 | `fe8d9b3f1c028209a8af6e721bdb56092bd640d8d434e38f9685cc7209a73c72`，原板回归编译 |

SDK解析自动将dependencies.lock的direct_dependencies加入idf，manifest_hash由e1f28437…变为`5be05836a36c08b4777d6ca3a942a7ec9f08fed48f6aa52ffe2828295482dd04`；组件版本/源码哈希未变，保留解析结果。新照片初次构建保留既有fontCN三字节UTF-8/NUL告警，最终增量无新增告警；集成保留已知restart_task未使用告警，无新增图像处理告警。

最终app SHA256 `e7d5bd8d1c338e448829fab7c241002f93b5c5fef0c304dd9b1a0d1c20b39ee2`；ELF SHA `432f32e6ce4d649ddc3e371575245d632f76d9b697b9d523728c1a861a69a5a2`；sdkconfig SHA `1055abe1df7e16e9270d1d807529e76514a728dd6420f37e7a993925d31f48ca`。固定副本`.cache/p3-photo-candidate-D1ffHt/`与构建输入cmp一致，含本机候选说明，不提交构建物。image-info确认checksum/hash有效、S3/DIO16MB80MHz/v6.1/MMU64KB、secure version0。ELF链接真实文件JPEG、诊断桥/会话、GC16和局部内存监测，jd_prepare=0x40000858/jd_decomp=0x40000864仍是ROM；保留符号中无产品入口、wifi初始化、NVS初始化/擦除/写值、PNG写入或校准保存函数。不把符号或镜像头当实际运行/硬件ABI验收。

拟更新仅factory应用`[0x10000,0x726e0)`，覆盖擦除块`[0x10000,0x73000)`，尚未执行或取得本候选硬件许可。不写bootloader/分区/NVS/PHY/内部storage/PMU校准、不改照片。更新前须重新核验身份、安全/布局与当前app完整回读，更新后独立全长回读及4096字节分区表保持核对。具体流程见 [单张照片诊断](READ-PICO-PHOTO-VALIDATION.md)。

本轮无串口、复位、挂载/实卡IO、烧写/回读、清白/GC16/NVS/网络或PMU写；设备仍为此前d760371b…/325,904字节，最后STOPPED/卡插入已卸载/串口关闭。新的主机与编译证据不替代旧设备状态，也不转移为照片实屏/P3完成。已到人工确认新候选与单张流程的边界，未自动继续硬件操作。

证据在忽略目录`.cache/p3-photo-diagnostic-`：host-tests-final.log、asan-tests-final.log、python-final.log、candidate.log、integration-final.log、original-board-final.log、display.log、storage.log、factory.log、symbols.log；5原片samples-private.log/samples-asan-private.log和前后SHA只保留本机。README、TODO、架构、照片/集成/SD/部署说明与CHANGELOG同步，11份当前文档的105个本地文件链接检查无缺失（不核验锚点），git diff --check通过；固定厂家目录干净，候选app/ELF/sdkconfig与构建输入再次cmp一致，不承诺远端CI。

## 2026-10-05 · P3 首张照片真机倒置拒绝与方向修复候选

### 首次候选部署与真机结果

用户此前明确允许e7d5bd8d…照片候选app-only更新及受控单张流程。更新前同设备ESP32-S3 rev0.2、16MiB Flash/8MiB PSRAM、安全启动/Flash加密关闭；写前factory应用325,904字节全长回读匹配d760371b…，4096字节分区表SHA256 `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`，布局仍nvs/phy/factory/storage。仅写403,168字节应用`[0x10000,0x726e0)`，擦除块止于`0x73000`；写后全长回读匹配`e7d5bd8d1c338e448829fab7c241002f93b5c5fef0c304dd9b1a0d1c20b39ee2`，分区表写前后逐字节相同。未写bootloader、NVS、PHY、内部storage或PMU校准。

新应用启动识别固定IDF v6.1、PMU 1.0.9、VCOM 1580mV、卡存在；一次1-bit/40MHz挂载成功。clear-white记录78/78帧、退出电源位`input0=0x81/out0=0x81`，用户确认“整屏白色无异常”。white-confirm后仅一次photo-show：S1文件6192×4128/EXIF6，ROM 1/4输出1548×1032，方向后1032×1548，Cover为684×1216、偏移(63,0)；源完整读取/EOF/关闭无错，处理约115,082ms，RGB分配4,792,608字节。GC16主动温度32°C、36/36相、单相约11.057–11.086ms、扫描合计398.136ms、入口返回约1,140ms；内部堆起始差4,044字节、PSRAM低水位差5,294,936字节为系统包含值，不是精确任务峰值。

实拍画面完整但上下颠倒；用户反馈“方向错误，上下颠倒，其他暂无问题”。因此不发送photo-confirm，显式photo-reject锁存后stop；Display cleanup、Unmount、HAL cleanup均ESP_OK，最终STOPPED、卡卸载、串口关闭。屏幕保留倒置静态画面不是应用仍在运行。该轮只证明白屏、文件链路、处理和刷新返回，照片视觉验收失败；“其他暂无问题”只记录本张目视范围，不把裁切、灰阶或长期稳定独立标记通过。私有硬件证据在`.cache/p3-photo-display-hardware-ll28WM/`，不提交照片或设备标识。

### 根因边界与软件修正

图像处理portrait输出的逻辑到native映射为`(x,y) → (1215-y,x)`；固定厂家`components/read_pico/read_pico_init.c`为机壳正向设置`EPD_ROT_INVERTED_PORTRAIT`，等价于`(x,y) → (y,683-x)`。二者恰差native 180°，与实拍上下颠倒一致。Read Pico板级默认方向因此改为180°，使完整产品经Paint转换时选择厂家物理正向；隔离照片诊断绕过Paint，故在标准4bpp封装时仅对native坐标应用同一180°。新增四角非对称合成测试逐点验证诊断输出等于厂家inverted portrait映射，并以源码检查锁住板默认、诊断桥与厂家初始化。EXIF6、Cover、灰阶/CDR/抖动、gamma、VCOM、频率、波形、PM锁和固定厂家子模块均未改。

完整产品配置允许NVS中的`disp_rot_deg`覆盖板级默认；若旧NVS已有0值，本轮没有读取、迁移或写回。因此修正候选只保证默认路径与隔离诊断一致，不宣称完整产品历史配置自动迁移完成。

### 软件复核、固件与新候选

| 检查 | 结果与边界 |
| --- | --- |
| 普通CTest | 459/459，21.50秒；含新增四角厂家映射回归 |
| ASan/UBSan CTest | 459/459，80.47秒，无sanitizer错误 |
| Python构建/控制台 | 38/38，2.346秒；新增源码一致性保护，fake serial、不访问设备 |
| 5张获准原片普通/ASan诊断CLI | 全部ESP_OK、415,872字节/831,744像素；前后源SHA逐字节清单一致，不生成/修改照片 |
| 固定SDK六配置 | 照片403,184、集成1,978,208、原板2,058,752、显示305,120、SD326,128、factory275,344字节，全部编译成功 |
| 符号保护 | 新照片ELF仍由ROM地址`jd_prepare=0x40000858`/`jd_decomp=0x40000864`，无产品入口、Wi-Fi初始化、NVS初始化/擦除/写值、PNG写入或校准保存符号 |

首次固定SDK调用未带已配置的Python环境，IDF在构建前拒绝并未修改配置/源码或执行硬件动作；显式使用固定`idf6.1_py3.14_env`后六配置全部成功。image-info确认新照片应用为ESP32-S3、DIO/16MB/80MHz、IDF v6.1、MMU64KB、secure version0且checksum/hash有效。普通/ASan日志分别为`.cache/p3-photo-orientation-host-tests.log`、`.cache/p3-photo-orientation-asan-tests.log`，Python为`.cache/p3-photo-orientation-python.log`，六配置和image-info/符号日志同前缀保留在忽略目录；5原片日志后缀private。

新候选应用403,184字节、SHA256 `62cac21b8cc7502fd6c79611f6b57d3f1c174d2549b2be41ee539f7827c0a554`；ELF `9b2d13c6c998d36676160525395f6ab0438063907821058e05f36d6c60a4fb38`；sdkconfig `1055abe1df7e16e9270d1d807529e76514a728dd6420f37e7a993925d31f48ca`。固定副本`.cache/p3-photo-orientation-candidate-0VzAUF/`与构建输入cmp一致。拟写范围`[0x10000,0x726f0)`、擦除块`[0x10000,0x73000)`；**本候选尚未部署，未访问设备**。下一步须取得新的app-only更新、清白/人工白参考及同一S1一次显示许可，并再次做身份/安全/分区/旧应用全长回读与写后独立回读。同步JPEG/显示调用仍无硬取消保证，不能复用e7d5bd8d…许可。

## 2026-10-05 · P3 方向修复版部署、白参考及单张照片软件完成（视觉待验收）

用户明确“允许”后，仅factory更新既有方向修复候选。写前ESP32-S3 rev0.2、8MB PSRAM/16MB Flash 5e4018、安全启动/Flash加密关闭，与此前设备一致；403,168字节旧应用全长回读匹配e7d5bd8d…，完整4096字节分区表与前轮逐字节一致，前3072字节匹配编译布局，尾1024字节均FF。旧应用读取以ROM/no-stub约139.5秒完成；中间主机调用返回但原读取进程仍在运行，另一次打开端口因独占锁而拒绝，原读取随后完整结束。该端口打开拒绝没有向设备发送指令或重复读取。最终以独立cmp/SHA为核验结果。

仅写403,184字节应用`[0x10000,0x726f0)`，实际擦除`[0x10000,0x73000)`，工具hash verified。随后使用esptool RAM flasher stub独立全长读取，新应用逐字节/SHA匹配`62cac21b8cc7502fd6c79611f6b57d3f1c174d2549b2be41ee539f7827c0a554`；写后4096字节分区表与写前一致，SHA均`f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。bootloader/分区/NVS/PHY/内部storage和PMU校准未写，本轮不修改源码或重跑既有459/459/38及六配置软件检查。

隔离photo控制台新启动ELF前缀9b2d13c6c…、IDF v6.1和8MB PSRAM匹配；PMU 1.0.9、VCOM1580mV、卡存在。启动与显示初始化各出现一次STATUS CRC拒绝，均在既有有界只读重读内恢复有效，不重发PMU GET/写校准/清故障。status后唯一mount为1-bit/40MHz成功，唯一clear-white记录78/78、退出电源位`input0=out0=0x81`，状态WAIT_WHITE。用户随后确认“整屏白色无异常”，本启动white-confirm被接受为READY，才发送唯一photo-show。

| 本启动日志时间 | 结果 |
| --- | --- |
| 117417ms | white-confirm接受，READY、operation=0/cleanup=0 |
| 122387ms | 唯一photo-show开始，内部free226231、PSRAM free6678644字节 |
| 221975ms | S1完整源/EOF确认，6192×4128/EXIF6/1/4、RGB4792608字节 |
| 221986ms | 方向后1032×1548→684×1216 portrait Cover，偏移(63,0)，与上一轮处理参数一致 |
| 236640ms | PHOTO_PROCESS=ESP_OK、114252ms、无错误原因 |
| 236820ms | 主动温度32°C、ON=1/PGOOD=1，CPU/APB240/80MHz锁内 |
| 237782–237783ms | 36/36相、min11057/max11081/total398114us；GC16 completed=1、stage/frame/operation/cleanup均0，入口1140ms |
| 237786ms | 独立电源退出检查ESP_OK，input0=out0=0x81 |
| 237787–237791ms | 局部低水位internal222187/PSRAM1383708、系统包含差4044/5294936字节；帧释放后PSRAM6678668、WAIT_VISUAL、operation=cleanup=0 |

照片处理和刷新软件完成，方向修正的视觉结论尚未登记；未发送photo-confirm/reject或stop。当前SD仍挂载、串口会话保持，等待用户核对方向、裁切及画面效果，再登记反馈和正常清理。不自动重复显示、换照片或清白。日志、旧/新应用及分区回读保存在忽略目录`.cache/p3-photo-orientation-hardware-GxPcPw/`；私人照片/设备标识不入库，无提交/推送。

## 2026-10-05 · P3 方向修复版S1人工通过与正常退出

用户附实拍并明确“正向显示正常没有问题”。附件可见机壳按键位于底部，建筑/地面为正常朝向，与上一轮机壳上下颠倒的展示状态不同；实拍作为本次视觉证据，不复制照片到仓库。根据本启动唯一S1反馈发送photo-confirm，509942ms记录VERIFIED、operation=0/cleanup=0，验收登记不触发刷新。仅计本设备/本卡/本张原片/本配置的一次白参考、读取/EXIF/Cover/灰阶/GC16和正向显示闭环通过，不将未标定照片推为光学灰阶计量或长期连续稳定。

随后显式stop一次：518345ms LCD deinitialized；518848ms Display cleanup=ESP_OK、SD开始卸载；518849ms Unmount=ESP_OK；518850ms HAL cleanup=ESP_OK；518851ms最终STOPPED、operation=cleanup=0，随后app_main返回。主机EOF结束控制台、exit0，端口占用检查为空；卡仍插入且已卸载，照片保留。当前应用仍为403,184字节/62cac21b…，屏幕保留已验收画面；本轮无再次刷新、烧写、换文件或源码/配置变更，不重跑既有软件测试。

方向问题已由同一S1的失败→坐标修正→部署/回读→新白参考→正向实拍确认闭环。剩余工作包括约114秒原片处理性能、其他样本/重复性、旧内存JPEG/PNG及全消费者生命周期、产品NVS方向覆盖迁移和完整PhotoFrame入口；S1单张通过不等于P3 MVP或第一阶段完成。TODO/README及照片、部署、架构/集成当前状态同步，私有证据继续存于`.cache/p3-photo-orientation-hardware-GxPcPw/`。

## 2026-10-05 · P3 有界JPEG读缓存、分阶段计时与PNG提交保护（仅软件）

用户要求继续开发至需要人工介入。本轮没有串口打开、复位、挂载、烧写或刷新；设备仍为方向通过版403,184字节/62cac21b…、STOPPED/SD卸载/串口关闭。探索性fast-context因缺少Windsurf凭据失败，随后用限定本地源码检索继续，没有提取或设置凭据。

实际源码确认`jpeg_stream_probe`将输入调用限制为256字节，文件业务每次读经`read_pico_sd_get_status`触发Board HAL读取IOE方向/输入寄存器。新`jpeg_buffered_source`为便携无分配适配，业务额外提供4096字节内部堆，物理读取前后检查live介质，缓存命中不重复查询。已知源长约束、短读继续及提前EOF/非法读/介质错误锁存保留；缓存移除检测最多延后一个块，必须完整源/物理EOF/关闭/最终介质成功才允许显示。JPEG调度由128调用改为约32KiB实际输入，与旧最大256字节调用的量级保持，不因4096字节读放大等待间隔。固定TJPGD、方向、EXIF/Cover、灰阶/gamma、波形、VCOM及频率不改。

增加设备`JPEG read/decode/close`和`JPEG geometry/gray/dither/paint`耗时及物理读次数/字节；主机时间为零，不作为速度证据。原114,252ms的约99.6秒读解码/14.7秒处理仅为原日志粗分，尚未证明新缓存改善多少或占比。

PNG输入检查非空普通文件≤32MiB、有界分块读、EOF、关闭及最终介质；fallback仍保留整份压缩PNG，32MiB不是内存支持保证。处理后PNG在验证IEND、补读文件附尾、关闭和介质成功前不刷新/发布；关闭失败原子锁存SD退出。流式PNG补验Cover裁切外IDAT行/IEND，行sink错误不忽略。主机关闭注入先真实释放句柄再报告失败，不冒充真实FAT故障；未验收全部消费者/并发、原子发布或产品运行。

| 检查 | 本轮结果 |
| --- | --- |
| 普通CTest | 474/474，24.24秒；新增15项，原459项保留 |
| ASan/UBSan CTest | 474/474，97.10秒；无报告，日志`.cache/p3-buffered-asan-tests.log` |
| Python构建/控制台保护 | 38/38，2.407秒，均为无设备替身 |
| Node22方向测试 | 6/6，4.822秒 |
| 解码一致性 | 固定外部TJPGD各缩放CRC、像素数、消耗字节保持；既有EXIF1–8及横/竖Cover逐帧比较通过 |
| 5获准原片 | 普通/ASan各5张均ESP_OK，415,872字节/831,744像素及16桶直方图与前轮一致；源SHA保持。直方图不证明逐像素/光学一致，不计ROM/实屏 |
| 固定IDF v6.1编译 | 照片、完整集成、显示、SD、factory、原板六配置成功，无本轮编译告警 |
| 设备更新/回读 | 未执行，不转移旧候选回读证据 |
| 优化版真机性能/视觉 | 未执行，需新候选部署许可和人工白参考/照片反馈 |

构建产物分别为照片404,336字节、完整集成1,980,592、显示305,120、SD326,128、factory275,344、原板2,058,976。照片应用SHA256 `9f6d6f6fc329344647f601b976d1869d70bd758b141f4db4f580f389fa2540a5`，ELF SHA256 `73b5d85440e41f547f5309629baf8344128a7b020e8ed7b0ba2038ac371969bd`；sdkconfig SHA256 `1055abe1df7e16e9270d1d807529e76514a728dd6420f37e7a993925d31f48ca`与方向通过版一致。集成SHA256 `e7567542ceba7c103055077855f2cc05ab009e3f957459699600d6ec71f2a0d6`、原板SHA256 `cbc8654af77e1cbab6f4f4b46f5e9fac8b23c20ed3daa466f4aeb2929231a4b9`。固定候选副本`.cache/p3-buffered-photo-candidate-kKXZq6/`，本地image-info检查IDF6.1/S3、DIO/16MB/80MHz/MMU64KB、checksum/hash有效、secure version0。

下一关键步骤为允许仅该factory候选及同一S1一次复测：重新核实身份/安全/旧应用与分区，写应用`[0x10000,0x72b70)`、擦除上界0x73000，独立全长回读且分区保持；新启动挂载/清白等待人工确认，再一次photo-show测量阶段耗时/内存/GC16及画面，确认或拒绝后stop。不可重用上一启动白参考/照片确认，不更新原片、不写bootloader/分区/NVS/PHY/内部storage/PMU校准，不开放完整业务。完整消费者审查仍是独立未完成事项，不能由本轮软件保护宣告P3 MVP。

日志统一`.cache/p3-buffered-*.log`，样本文件名/源hash只保留private日志；固定厂家子模块状态保持干净，用户原有修改保留。TODO、README、CHANGELOG及照片/集成/架构说明已同步；本轮无提交/推送。

## 2026-10-05 · P3 读取优化版部署与直接S1显示（视觉待验收）

用户明确“允许更新，刷白后直接执行S1 显示”。本次仅该固定候选factory app-only更新，以及新启动单次挂载/清白成功后直接显示同一S1；用户本次明确跳过白屏等待，white-confirm只作为协议推进指令，不伪造本次白屏目视确认。此特例不改变后续默认人工门禁，不跨启动使用旧白参考；仍等待照片视觉反馈，不自动photo-confirm或刷新第二次。

写前检查同一ESP32-S3 rev0.2、16MB Flash、Secure Boot/Flash Encryption均Disabled；前轮403,184字节62cac21b…应用独立全长读取匹配并保留，4096字节分区表与此前逐字节相同、前3072字节匹配本配置。只写404,336字节`[0x10000,0x72b70)`，实际擦除`[0x10000,0x73000)`；工具hash verified，随后独立全长读取逐字节/SHA256匹配`9f6d6f6fc329344647f601b976d1869d70bd758b141f4db4f580f389fa2540a5`，写后4096字节分区表与写前一致，SHA256 `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。bootloader/分区/NVS/PHY/内部storage及PMU校准未写；并未核对这些数据区的全量内容或进行完整Flash备份。

隔离photo控制台显式ESP reset一次，启动ELF前缀73b5d8544…、固定IDF v6.1、8MB/80MHz PSRAM内存自检和factory布局匹配，PMU1.0.9/VCOM1580mV只读有效，启动faults=0x1原样记录，不写清故障。初始present=1/mounted=0。唯一mount为1-bit/40MHz正常，卡介质有效；没有file-test/file-check/photo-check/换片/创建/覆盖/删除/NVS/网络或相机操作。

| 本启动日志时间 | 实际结果 |
| --- | --- |
| 30448–30451ms | mount=ESP_OK，present/mounted/media_valid=1、exit_failed=0，WAIT_CLEAR |
| 39968–41596ms | 唯一clear-white；78/78、complete=1、ESP_OK，退出input0/out0=0x81，WAIT_WHITE；没有目视白屏验收 |
| 68594ms | 按本次用户许可white-confirm接受为READY、operation/cleanup=0 |
| 70038ms | 唯一photo-show，internal free226231/largest180224，PSRAM free6678644/largest6553600字节 |
| 111532–111534ms | 完整JPEG源/EOF、EXIF6、6192×4128、1/4、RGB4792608；读/解码/关闭41491ms、物理2311次/9464714字节、4096缓存、ESP_OK |
| 111544–126200ms | 方向后1032×1548→684×1216 portrait Cover，偏移(63,0)，原灰阶/CDR与Floyd-Steinberg；绘制阶段14656ms、ESP_OK |
| 126202ms | PHOTO_PROCESS=ESP_OK、elapsed56161ms、错误原因为空 |
| 126291–126381ms | CPU/APB240/80MHz锁内；主动温度31°C，ON=1/PGOOD=1 |
| 126840–127344ms | HL draw419ms；36/36相完整，min11057/max11086/total398130us；GC16 completed=1，stage/frame/operation/cleanup均0，入口1140ms |
| 127347–127351ms | 独立电源退出ESP_OK/input0=out0=0x81；帧释放后internal free226143/largest180224、PSRAM free6678668/largest6553600，WAIT_VISUAL |
| 184644–184646ms | 显式status确认SD仍present/mounted/media_valid=1、exit_failed=0，WAIT_VISUAL、operation/cleanup=0 |

本次同一S1处理相比方向通过版114252ms减少58091ms（约50.8%，约2.03倍），而图像处理约14.7秒和GC16约398毫秒保持前轮量级。前轮读解码约99.6秒是日志差的粗略值，本次41491ms为新阶段计时；只计同一设备/卡/原片/配置的一次改善，不推广为所有照片或长期性能保证。源读取/EXIF/Cover/灰阶软件和GC16返回通过，不等同本候选视觉通过。本次控制台没有接收到PHOTO_MEMORY局部低水位条目，不能登记精确峰值或补用上一候选峰值；仅记录上述实际收到的free/largest。

当前没有发送photo-confirm/reject或stop；照片尚待用户核对方向、铺满/裁切和画面细节。串口会话保持、SD挂载，显示高压退出正常且照片帧已释放。后续收到视觉反馈后登记确认或拒绝，再stop/显示与SD/HAL清理、EOF关闭串口；不重复显示/清白或自动换片。私有证据`.cache/p3-buffered-photo-hardware-PAms8H/`，固定输入`.cache/p3-buffered-photo-candidate-kKXZq6/`。本次无源码/配置变更或重新主机测试/编译，复用既有474/474/38/6及六配置软件证据；更新TODO/README/CHANGELOG及相关当前状态，无提交/推送。

## 2026-10-05 · P3 读取优化版S1人工通过与正常退出

用户附本次实拍并明确“画面正常没有问题”，仅登记本启动唯一S1的视觉通过。430701ms发送photo-confirm后返回VERIFIED、operation=cleanup=0，登记不触发刷新；照片不复制到仓库。本次按用户许可跳过白屏人工等待，仍不补记白参考目视通过，不把本张照片作为16阶光学计量、其他样本或长期稳定性验收。

随后显式stop一次：432954ms LCD deinitialized；433457ms Display cleanup=ESP_OK且SD开始卸载；433458ms Unmount=ESP_OK；433459ms HAL cleanup=ESP_OK；433460ms最终STOPPED、operation=cleanup=0；433461ms命令处理结束，433462ms app_main返回。主机EOF结束控制台、exit0，端口占用检查为空。卡仍插入且已卸载，屏幕保留已验收S1，原片保留；未再次清白、显示、重置、烧写、换片或运行完整产品。

当前应用仍为404,336字节/9f6d6f6f…，同一S1单次处理56,161ms较前轮114,252ms缩短约50.8%，软件、部署/回读、单张视觉和正常退出证据分别保留。精确局部内存峰值仍未取得，不补造数据；其他样本、重复性、全文件消费者/并发与NVS/OTA迁移保持未验收。TODO、README、CHANGELOG及照片/部署/架构/集成当前状态已同步。本反馈轮仅验收登记与正常退出，无源码/配置变更、重新测试/编译、提交或推送；私有证据仍为`.cache/p3-buffered-photo-hardware-PAms8H/`。

## 2026-10-07 · P3 内存JPEG与文件入口保护（仅软件）

用户要求继续。先读取TODO/需求/架构/索引及适用AGENTS；fast-context因Windsurf凭据缺失不可用，改用限定main/host_tests本地搜索，未提取或配置凭据。本轮未打开串口、复位、烧录、挂载实体SD、刷新或读取私人原片；最后已知设备应用仍为10月5日404,336字节/9f6d6f6f…、S1人工通过并正常STOPPED。新编译产物不代替设备身份或回读证据。

源码确认旧内存JPEG忽略esp_jpeg_get_image_info返回值，依靠未验证尺寸/长度分配RGB，并且不走文件JPEG的EXIF/预算/Cover契约。Read Pico改用同一便携metadata和固定流式解码器：≤32MiB源、边≤16384且≤64M源像素、baseline/EXIF1–8、最小可用缩放及≤5MiB RGB，实际输出尺寸匹配后才开始显示。内存回调不查询SD、读取按边界限制、约32KiB让出调度；调用方保持压缩字节有效/不可变，不增加压缩副本或旋转RGB。工作区/RGB任一分配或解码失败均释放，不显示；RGB预算不是全部堆/相机原片支持保证。显示发布/快照仍可能访问存储，保留display_manager契约，未证明所有消费者或并发安全。

另确认display_flow_stream_file的旧通用分支会在解码器拒绝格式前依release_source删除源；Read Pico现在只允许JPEG/PNG检查式入口，无效路径/算法及其他格式在文件读/删除前拒绝。其他板不改变，BMP/EPD_GZ的专用入口不增加/取消支持。该修复不处理全部stage/retire/current-slot/HTTP/URL文件所有权或原子发布，因此产品启动门禁不解除。未改固定组件、显示方向、灰阶/gamma、VCOM、频率或波形。

| 检查 | 本轮结果 |
| --- | --- |
| 普通CTest | 486/486，29.96秒，`.cache/p3-memory-jpeg-host-tests.log` |
| ASan/UBSan CTest | 486/486，109.44秒，无报告，`.cache/p3-memory-jpeg-asan-tests.log` |
| 新增用例 | 9项内存JPEG、2项文件入口原片保护；8种EXIF×横/竖显示与文件JPEG逐帧一致，既有定向PNG对照保留 |
| 输入/失败保护 | 源字节不变、9MiB输入无额外压缩分配；无卡/SD关闭锁存与内存解码隔离；非法元数据/渐进/超限长度及像素、损坏/截断、RGB与工作区分配失败均不发布；误用release_source不删除源 |
| Python保护 | build23/23（2.440秒）、console16/16（0.005秒），合计39，均为不接触硬件的替身测试 |
| 固件编译 | 固定IDF v6.1（fff9895c…）集成/照片/原板三配置成功；最后集成增量编译记录现有http_server.c restart_task未使用警告，未修改该无关代码，无本轮图像代码警告 |
| 烧写/回读/真机/性能 | 未执行，不转移旧S1/ROM/51%性能或实屏反馈为新增内存路径验收 |

编译输出：完整集成1,979,216字节、SHA256 `9aa41dc045b1005998902d3320c6c028b71cf79161578f19b81af152e9cf6bc7`；照片404,720字节、`bb4e42b7d260807970d3cd48b6230c4b8454ebf6a9db1d3278a1a97a364ccb66`；原板2,058,976字节、`849535e81c9e92ad993a5fd3c6dc5570c2b5d6593d1521633a63110396716be9`。照片sdkconfig仍为`1055abe1df7e16e9270d1d807529e76514a728dd6420f37e7a993925d31f48ca`，与已验收历史配置相同，不意味新应用已部署。对应编译/SDK/Python日志`.cache/p3-memory-jpeg-*.log`；固定Read Pico子模块保持干净。未复跑私人5样本、Node方向/Web测试、其余诊断固件配置或远端CI，不计这些为本轮通过。

TODO/README/CHANGELOG及照片/架构/集成说明同步。后续继续审查全文件消费者关闭/并发、临时文件所有权和原子发布，再准备受控产品流程及NVS/OTA迁移许可；完整集成固件不得因本轮编译成功直接烧录运行。用户原有内容保留，本轮无提交/推送。

## 2026-10-07 · P3 SD IO占用与快照关闭保护、只读候选（未部署）

上轮完成内存JPEG与文件入口保护，属于真实软件进展。本轮按同一开发目标继续核对现有TODO/主工程；fast-context仍缺Windsurf凭据，使用限定main/board_hal/epaper_src/host_tests的本地搜索。未提取凭据、操作设备、读取私人原片或执行相机功能。

实际源码显示SD卸载原先只有“所有消费者先关闭”的注释契约，没有在打开/关闭过程中保持占用；标准文件消费者散布于图片/GUI、HTTP/URL、相册/目录及日志。新增read_pico_sd_io_begin/end：在同一Board锁内确认挂载/介质并增加原子计数，unmount在有占用时拒绝且不失效正常介质，end仅原子结束，不匹配结束锁存。read_pico_fs在三个SD相关profile链接包装11个标准入口，32槽、无堆分配、登记临界区不IO/日志，fdopen转移已有占用，真实关闭成功后释放；关闭错误保留占用/Board租约，已知不确定句柄不自动重试。元数据操作也在调用期间占用，其他路径/socket/console透传。此保护不等于数据发布原子性、SD硬超时或所有IO别名/所有消费者错误传播已审查。

初次普通503项回归中并发用例出现fclose=-1、结束399/400、残留占用1，暴露libc复用旧FILE键但旧关闭还在记账的窗口；当时ASan503通过不能否定这个竞态。修正过期键识别及live-owner选择，增加SD/非SD复用的确定性用例。旧完整失败日志后来被复跑覆盖；实际工具输出摘录整理为`.cache/p3-fs-lease-initial-race-summary.txt`，不伪称保留原日志。修正后的最终全量及重复测试见表。

display_manager快照在新板改O_CREAT|O_EXCL，只独占创建衍生目标；已有文件/符号链接目标不截断。检查flush/fsync、close和最终介质状态；关闭错误接SD锁存，失败/部分衍生文件保留、不unlink可能由其他消费者替换的路径。显示已完成但保存失败继续返回ESP_ERR_NOT_FINISHED/fallback语义；不是“失败就未显示”。新增完整gzip→提交帧比对、已有文件/链接保留、关闭失败和关闭后介质失效用例。其他板快照行为保留；未完成将部分快照隐藏至最终发布或所有上传/日志关闭错误传播，产品仍禁止运行。

| 检查 | 本轮最终结果 |
| --- | --- |
| 新增CTest | 24项：4个SD生命周期、15个标准句柄/元数据/复用、5个快照保护 |
| 普通CTest | 510/510，28.71秒，`.cache/p3-fs-lease-host-tests.log` |
| ASan/UBSan | 510/510，112.09秒，无报告，`.cache/p3-fs-lease-asan-tests.log` |
| 并发重复 | 8线程×50次独立打开/关闭的用例额外100次（40,000次），4.26秒，全部通过，`.cache/p3-fs-lease-concurrency.log`；非实际RTOS/实卡 |
| Python保护 | build26/26（2.469秒）、console16/16（0.006秒），合计42；新增3项配置/快照/只读观测源码保护，均不触及硬件 |
| 固件编译 | 固定IDF v6.1：集成、照片、SD、显示、factory、原板六配置成功；新FS最后修正后重编三个SD相关profile，SD只读观测最后更新后再次编译 |
| 链接检查 | 集成build.ninja含全部11个wrap选项，map将对应__wrap符号绑定到read_pico_fs；storage compile_commands使用ESP_PLATFORM；不替代实际libc/FAT调用/计数验收 |
| 依赖 | 与`.cache/p3-fs-lease-dependencies.before.lock`比较，dependencies块的版本/source/component_hash逐字相同；生成的manifest_hash和direct_dependencies中idf标记更新，idf依赖条目仍6.1.0，外部SDK commit仍固定 |
| 设备更新/回读/视觉 | 未执行；最后已知仍10月5日9f6d6f6f…S1通过/正常STOPPED，不转移为新包装的实卡/GC16/性能结论 |

初次配置受沙箱网络及SDK psutil进程枚举限制；经受控主机编译后完成，不禁用组件管理或改SDK绕过。完整重配置编译出现既有中文字体数组和GUI字符比较告警、集成restart_task未使用告警；最终增量未见新FS/SD/快照代码告警。锁文件的生成元数据更新保留，不伪称整份锁完全不变，也不接受组件版本升级。固定厂家子模块仍干净，用户原有编辑保留。

本轮最终编译：集成1,981,600字节/SHA256 `8f0f9c72e81ba5738038148020a8f2fcb333ca21ec2feff0262061642fa20185`；照片406,256字节/`e2b25a4bd574f565344b9c9a1b01368367ef0a5037fc6aee114c5cf5537b6445`；SD328,016字节/`7861965cb49015a35ed254886008ee30a77fb907a40ad4fa14b9abdc6a5b3aa9`；显示305,120、factory275,344、原板2,058,976字节（原板SHA256 `75313357dfbcdaeff0185d0b082a0efdf904fe027d5e29b131d5668bfade8422`）。以上都是编译产物，不是部署证明。

已冻结仅SD候选`.cache/p3-fs-lease-storage-candidate-T7Skvt/`，与最终build/sdkconfig一致；ELF SHA256 `b1e99213b67b85b6e7c5f3f9c89af801969d37c3f3e549af2cd6e2fc3fd27411`、sdkconfig `116fab2bb11b0d29d45b016756fae15289f73a1026609a11912429984c03afe1`。本地image-info确认S3/IDFv6.1/DIO/16MB/80MHz/MMU64KB/secure version0、checksum/hash有效。没有解锁完整产品入口。

新file-check只读能力不变，仍只打开此前专用RC3T0001.BIN、核对4096字节/EOF。隔离入口新增打开active_io=1、关闭0和介质有效的观测条件；不在持有句柄时尝试实卡卸载，不注入故障/热拔，不创建、改写、修复或读取私人照片。基本实卡证据取得前不把共享标准库保护当产品运行已验证；下一步需要用户新的仅factory更新许可及测试卡确认，计划写`[0x10000,0x60150)`/擦除`[0x10000,0x61000)`，先核实设备/安全/分区并保留当前应用，再完整回读/分区保持、status→mount→file-check→status→stop。不清白/显示，不file-test/photo-check，不写bootloader/NVS/PHY/内部storage/PMU校准。

当前仍需上传成功/失败反馈、全部临时文件/原片所有权、发布原子性和NVS/OTA迁移；新基础适配的人工上板关口不代表这些工作完成。TODO/README/CHANGELOG与存储/架构/集成状态同步，本轮无提交/推送。主机/编译/私有候选日志`.cache/p3-fs-lease-*`，新真机证据尚不存在。

## 2026-10-07 · P3 SD句柄保护获准部署与只读实卡核对

用户明确“允许”，随后要求继续。本轮许可仅覆盖固定SD隔离候选factory app-only更新、新启动status→一次mount→一次file-check→status→stop；不继承旧file-test/照片刷新/完整产品许可。没有源码或配置修改、重新编译/主机回归，复用上一软件轮510/510/42及六配置证据，不能计为本轮重新通过。

写前私下比对与此前设备身份一致，ESP32-S3 rev0.2、16MB Flash，Secure Boot/Flash Encryption均Disabled。旧404,336字节应用独立完整回读匹配`9f6d6f6fc329344647f601b976d1869d70bd758b141f4db4f580f389fa2540a5`并保留。写前4096字节分区表与此前相同，前3072字节匹配本配置。仅写328,016字节`[0x10000,0x60150)`，实际擦除`[0x10000,0x61000)`，工具hash verified；另行独立全长回读逐字节/SHA256匹配固定输入`7861965cb49015a35ed254886008ee30a77fb907a40ad4fa14b9abdc6a5b3aa9`。写后4096字节分区表与写前逐字节相同，SHA256 `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。未写bootloader/分区/NVS/PHY/内部storage/PMU校准；未全量核对这些数据分区，也不是完整Flash备份。

新隔离控制台显式ESP reset一次，启动ELF前缀b1e99213b…、固定IDF v6.1、8MB/80MHz PSRAM自检正常、CPU240MHz。初始595ms SD present=1、mounted=media_valid=exit_failed=active_io=0，开机不挂载/文件IO/显示。此SD启动没有新的PMU/VCOM GET数值日志，不套用历史值为本轮观测。

| 本启动日志时间 | 实际结果 |
| --- | --- |
| 120178ms | 显式status：present=1、未挂载、active_io=0 |
| 242360–242420ms | 唯一mount；1-bit、40MHz，ESP_OK；present/mounted/media_valid=1、exit_failed=active_io=0 |
| 1338561ms | 唯一file-check打开：lease observation=ESP_OK、active_io=1、expected=1、valid=1 |
| 1338583ms | 成功关闭：lease observation=ESP_OK、active_io=0、expected=0、valid=1 |
| 1338585–1338588ms | File check=OK、verified=4096、created=written=0、close_failed=errno=close_errno=0；固定内容/EOF通过，卡仍有效、active_io=0 |
| 1441412ms | 显式最终status：present/mounted/media_valid=1、exit_failed=active_io=0 |
| 1553961–1553965ms | 唯一stop：Unmount=ESP_OK、HAL cleanup=ESP_OK、STOPPED、app_main返回 |

最终status及stop已完成，SD卸载/HAL清理ESP_OK、STOPPED；主机EOF关闭控制台exit0，授权主机lsof检查无输出/exit1（串口无占用）。卡仍插入且卸载，本轮未刷新屏幕。操作证据`.cache/p3-fs-lease-storage-hardware-ed4twS/`，固定输入`.cache/p3-fs-lease-storage-candidate-T7Skvt/`。主机授权调用延迟期间未重复发送命令。未file-test/photo-check、创建/改写/修复/删除、读取私人照片、启产品/网络/相机或改校准；FAT挂载不是硬件写保护，不宣称所有元数据零变化。

实际libc/FAT打开/关闭占用的单次只读路径通过，不等于真实关闭故障、热拔、RTOS并发、照片/快照/上传各调用入口或完整产品生命周期全部验收。产品/NVS/OTA门禁保持关闭，后续仍需上传错误传播、临时文件/原片所有权及原子发布审查。

TODO/README/CHANGELOG及SD/部署/照片/架构/集成页已同步当前设备与历史照片镜像边界。本轮只增加证据和文档，不提交/推送；固定厂家子模块及原有源码改动保留。基本SD实卡关口已关闭，后续软件审查不自动授予下一轮部署、显示或产品运行许可。

## 2026-10-07 · P3 有界上传、FAT非覆盖发布与合成探针（仅软件）

按“继续开发，直到需要人工介入”为止继续软件审查。fast-context因缺少Windsurf凭据不可用，改限定目录搜索，未获取凭据。沿用PhotoFrame HTTP/相册目录与契约，不改固定厂家源码，不执行相机功能。此轮没有打开串口、更新设备、刷新屏幕或读私人原片；设备最后已知仍为上一节7861965c…只读诊断/STOPPED。

新增便携upload_stream/upload_file和板级read_pico_upload。Read Pico multipart采用有界二进制流：boundary≤70、header≤1024、filename≤119字节，跨块分隔符/零字节保持，重复或未知字段、危险名称、截断与尾部垃圾失败。每文件≤5MiB、multipart≤10MiB+4096；连续8次接收超时或120秒接收阶段预算失败，但同步SDK调用本身没有硬取消，不承诺严格实时上限。独占新建隐藏暂存，短写/零写、fsync、一次close和介质检查均决定成功；关闭不确定锁存、不重试或删除，事务绑定创建时源路径。

相册PNG/EPDGZ主图及JPEG缩略图完整接收/同步/关闭后，缩略图先、主图后发布；同名不预删除，不宣称两文件原子性或FAT掉电安全，部分发布失败保留结果并报告。魔数检查仅识别格式。直接显示后处理/current-slot回收、URL下载及其余消费者/迁移仍待审查，完整产品门禁不解除。详见[上传保护](READ-PICO-UPLOAD.md)。

核对固定IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`实际源码：vfs_fat.c:979在FAT锁内调用f_rename，ff.c:5305起的不同既存对象冲突返回FR_EXIST，vfs_fat.c:348映射EEXIST。只在Read Pico拥有的/storage FAT路径使用；不能以主机POSIX rename覆盖语义替代。主机适配测试使用FAT式非覆盖替身，不是实卡FatFS验收。

新upload-test只在SD隔离profile精确命令下每启动一次，开机不执行；控制台不在display/photo转发。独占创建固定4096字节图样并发布RC3U0001.BIN，两次完整读取/EOF；另创建16字节0xA5暂存，向同名发布必须EEXIST且BIN内容不变。预存目标/暂存或任何失败停止，不换名、删除、修复或重跑。使用256字节块及复用已关闭事务，降低现有6144字节主任务栈占用；未测真机栈峰值。合成探针不是HTTP/Wi-Fi/照片/完整产品验证。

| 检查 | 本轮最终结果 |
| --- | --- |
| 新增CTest | 40项：11流解析、15文件事务、14实际适配替身/合成探针；二进制分块/截断、短写、关闭、介质、源绑定、同名保留与错误覆盖检测 |
| 普通CTest | 550/550，33.47秒，`.cache/p3-upload-host-tests.log` |
| ASan/UBSan | 550/550，122.91秒，无报告，`.cache/p3-upload-asan-tests.log` |
| Python保护 | build28/28（2.620秒）、console17/17（0.007秒），合计45；新增3项保护，不触及硬件 |
| 固件编译 | 固定SDK六配置成功：SD、集成、照片、显示、factory、原WaveShare；最终源绑定/栈调整后重编SD/集成 |
| 来源/工作树 | 固定Read Pico gitlink28cde682…且子模块干净，git diff --check通过；原有用户改动保留，无提交/推送 |
| 依赖 | 最终dependencies.lock相对HEAD仅manifest_hash/direct_dependencies的idf标记生成差异，dependencies版本/source/component_hash未变；保留生成元数据，不称整份锁不变 |
| 烧写/回读/实卡/视觉 | 未执行，不把旧file-check或S1验收当作新上传/发布证据 |

初次SDK环境路径指向基线目录导致constraints缺失，改用已有安装工具与固定Python环境后成功，未安装/修改SDK。首次适配主机链接缺C接口extern C保护，修复后最终全量通过。构建曾出现既有http_server.c restart_task未使用警告；最后SD/集成增量无新增上传代码警告。中间较少用例的回归不代替上表最终550项；未复跑私人5照片、Node/Web或远端CI。日志`.cache/p3-upload-*.log`，不保留/提交私人样本及构建生成物。

| 最终固件 | 大小（字节） | SHA256（已核对项） |
| --- | ---: | --- |
| SD合成探针 | 332304 | c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3 |
| 完整集成（禁止运行） | 1984496 | 727ffed23f9658cd4c7fe95513d9e23fdcfadadeda540d8909f78b6b4caf8bc4 |
| 照片诊断 | 406256 | 本轮仅编译，不部署 |
| 显示诊断 | 305120 | 本轮仅编译，不部署 |
| factory只读 | 275344 | 本轮仅编译，不部署 |
| 原WaveShare | 2058976 | e398b6a16853c303e717d5cb1292235e7445e02d390101b3c72e2756c0227cd6 |

SD候选固定副本`.cache/p3-upload-storage-candidate-EuBiLz/`：ELF SHA256 `21d0f9b5ed5707ff1c07886d1c7bb4c90e29ee47528957ff49c5c82900650d67`；sdkconfig `116fab2bb11b0d29d45b016756fae15289f73a1026609a11912429984c03afe1`，与此前SD配置一致；编译分区产物 `205e21ddff471bce79c29a846bd04a48854cb42d406c784405d4c8e3ec1e6061`。本地image-info确认S3/IDFv6.1/DIO/16MB/80MHz/MMU64KB/secure version0及checksum/hash有效，未连接设备。

现在停在新的人工关口：确认仍是已备份测试卡，并允许仅factory app-only更新及一次合成文件写入/发布测试。计划写`[0x10000,0x61210)`、擦除`[0x10000,0x62000)`，授权后仍需核实设备/安全/分区、保存当前完整应用，独立全长回读与4096字节分区保持，再status→mount一次→upload-test一次→status→stop。成功应created=2、written=4112、published=1、verified=8192、conflict_rejected=1、close_failed=0、打开/关闭占用1→0及正常退出；成功保留4096字节BIN和16字节PART，失败保留可能的部分文件。未获新许可前不执行；不附带旧file-test/file-check/photo-check、显示、热拔/故障注入或完整产品/NVS/OTA/校准操作。TODO及架构/部署/存储/集成说明同步。

## 2026-10-07 · P3 合成文件非覆盖发布获准部署与单次实卡通过

用户回复“仍使用测试卡，并允许更新”，对应上一轮明确提出的仅factory诊断更新及一次合成文件创建/发布测试。本轮复用固定332,304字节SD候选，不修改固件源码/配置、不重新编译或运行主机测试；550普通/550ASan、45Python及六配置仅属上一软件轮证据。fast-context仍缺凭据，限定本地源码核对，无凭据提取、照片/相机操作或完整产品运行。

写前授权主机检查串口无占用；私下设备身份与此前一致，ESP32-S3 rev0.2、Flash 5e4018/16MB，Secure Boot/Flash Encryption均Disabled。4096字节实板分区与此前逐字节/SHA相同，前3072字节匹配编译布局。首次主机cmp -n比较较短编译文件报告EOF，受控脚本在读取旧应用/唯一写指令之前exit1；改用head -c3072截取后逐字节比较exit0，复用已读分区并复核全部前置条件，不跳过分区门禁、不重复烧写。

旧328,016字节应用独立完整回读匹配`7861965cb49015a35ed254886008ee30a77fb907a40ad4fa14b9abdc6a5b3aa9`并保留。唯一写332,304字节`[0x10000,0x61210)`，实际擦除`[0x10000,0x62000)`，工具hash verified；另行独立全长回读逐字节/SHA匹配`c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3`。写后4096字节分区表与写前逐字节一致，SHA均`f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。ROM核对身份/安全，实际Flash读写使用RAM flasher stub；未写bootloader/分区/NVS/PHY/内部storage或PMU校准，也未全量校验这些数据区域，不是完整Flash备份。

控制台仅一次显式--reset调用。捕获先有一段到esp_image segment3的USB_UART_CHIP_RESET启动，再有完整同原因启动；前段未到app_main，不写成两次app测试，也不猜第二段原因。后一完整启动ELF21d0f9b5e…、IDFv6.1、8MB/80MHz PSRAM自检正常、CPU240MHz，596ms卡存在但未挂载/无占用，597msREADY，无启动自动文件IO。未增加额外复位；本启动没有新的PMU/VCOM GET数值日志，不套用旧值为本轮观测。

| 本启动日志时间 | 实际结果 |
| --- | --- |
| 123188ms | 显式status：present=1、mounted/media_valid/exit_failed/active_io=0 |
| 237230–237290ms | 唯一mount：1-bit、40MHz、ESP_OK；present/mounted/media_valid=1，exit_failed/active_io=0 |
| 338398→338447ms | 独占新建第一份4096字节合成文件，create占用1/valid1，sync/close成功后占用0/valid1 |
| 338458→338481ms | 发布后第一次只读打开BIN占用1、关闭0，介质均有效；4096字节/EOF正确 |
| 338489→338504ms | 独占新建第二份16字节合成PART，sync/close成功，占用1→0/valid1 |
| 338512→338535ms | 同名发布拒绝后第二次只读打开BIN、4096字节/EOF及内容不变，占用1→0/valid1 |
| 338537–338541ms | Upload probe=OK、stage=complete、created2/written4112/published1/verified8192/conflict_rejected1/close_failed0；卡有效、占用0 |
| 445576ms | 显式最终status：present/mounted/media_valid=1，exit_failed/active_io=0 |
| 549940–549944ms | 唯一stop：Unmount=ESP_OK、HAL cleanup=ESP_OK、STOPPED、app_main返回 |

完整probe成功要求FAT实际rename同名冲突为EEXIST、没有发布第二份，且两次复读逐字节/EOF正确；不是仅stat预检查或主机替身结果。本轮只新建两份合成文件，第一份`.RC3U0001.PART`发布为`RC3U0001.BIN`4096字节，第二份`.RC3U0002.PART`16字节因冲突保留；不删除、覆盖、修复、换名或重跑，不打开旧RC3T0001.BIN或私人照片。文件同步/关闭和同一挂载内重开核对通过，不计卡断电/ESP重启后持久性、写中异常掉电、真实关闭故障/热拔或并发/任意卡长期稳定。

最终SD卸载/HAL退出正常、STOPPED；主机EOF结束控制台exit0，授权主机lsof检查无输出/exit1（串口无占用）。测试卡仍插入且卸载，屏幕未刷新；没有file-test/file-check/photo-check/清白/显示、网络/HTTP/产品、数据分区/校准或相机指令。本轮新FAT子路径通过不解锁完整PhotoFrame入口，直接显示后处理/current-slot回收、URL/其余消费者、暂存清理与NVS/OTA继续软件审查。

固定输入`.cache/p3-upload-storage-candidate-EuBiLz/`，私有硬件/完整旧新应用/分区/控制台证据`.cache/p3-upload-storage-hardware-fAeGCh/`。授权主机调用等待期间未重复发送命令；首次前置比较失败及修正见本目录SESSION.md，不伪造原完整日志。TODO/README/CHANGELOG与上传/部署/存储/架构/集成/照片页已同步；原有用户修改、固定厂家子模块保持，无提交/推送，证据/标识/构建生成物不入库。

### 2026-10-07 · P3直接显示、URL下载与隐藏快照发布保护（仅软件）

在上节实卡退出后继续安全软件开发，未继承为新设备操作许可。当前仍332,304字节/c876599f…SD隔离应用、STOPPED、测试卡已卸载；未开串口、复位、烧录、读写测试卡、读取原片、刷新或运行业务网络/完整产品。源码探索的fast-context因缺Windsurf Key不可用，使用限定文件rg及完整相关函数核对，不提取凭据。

直接显示raw/multipart保持完整接收的`upload_file_t`，新增便携`upload_asset`对闭合所有权/隐藏名/格式/预览检查，先预览后主图发布为独立格式文件，不再删除固定current图片。所有失败产物保留；魔数验证不等于解码通过。URL复用独占事务，非200正文隔离、已知长度/SDK完整接收/阶段预算/写入/同步/关闭/最终介质检查，304关闭空暂存并跳过刷新。固定SDK ON_DATA忽略事件回调返回值的源码已核对，因此另有owner失败锁存；完成时即使perform成功也拒绝失败文件。

重定向元数据按请求清空、Location有界，在安排下一请求前处理跨站源凭据，去除失败则不跳转；跨站缩略图不携带源认证/配置自定义头，缩略图采用配置CA。Downloads仍用标准可见相册名、JPEG预览和EPDGZ快照，不重新引入自建相册/状态系统；复制长度/EOF/关闭/同步受检查，32KiB调度让出，原输入保留。快照独占隐藏暂存完成/关闭后FAT非覆盖发布，失败不留下半写入的最终EPDGZ。

当前图片复制复用显示锁，记录写入/flush/fsync/关闭/介质及匿名记录清除失败传播，HTTP/URL明确已经显示而保存未完成。不把ESP_ERR_NOT_FINISHED转换为普通成功，也不假称屏幕回滚。持久`.current.lnk`仍原位写入，不保证元数据掉电原子性或重启恢复；旧记录/NVS/OTA继续审查，完整产品门禁保持关闭。

新增27项CTest：12项所有权/预览/碰撞/部分发布/介质/边界，5项下载正文/状态/截断/超时/304，2项复制长度/EOF/关闭与失败保留，2项源站/相对跳转/端口/用户信息及超长URL，2项真实显示管理器记录关闭故障/受锁复制，4项实际固定JPEG解码器的独占发布→显示替身/对应预览/魔数不符/关闭故障/坏图保留。主机发布IO模拟FAT非覆盖语义，不能拿POSIX默认rename替代；显示替身不计实屏。

最终普通CTest **577/577，30.13秒**；ASan/UBSan **577/577，119.03秒**。Python构建保护29项（新增1项）及控制台17项，合计**46/46**；git diff --check通过。第一次Python回归因旧快照静态断言仍要求在最终路径O_EXCL失败，随后改为检查共享适配的独占/sync/关闭锁存及隐藏暂存finish/publish，重跑29项成功；不是删掉保护或抹去失败。第一次集成编译因受限网络无法查组件清单，在源码编译前失败；主机授权环境继续固定SDK/组件后最终通过，不更换SDK/依赖版本。

最终六配置均编译成功：

| 配置 | 应用字节 | 证据范围 |
| --- | ---: | --- |
| Read Pico完整集成 | 1,985,056 | 真实HTTP/URL业务已编译链接，入口仍禁止运行 |
| Read Pico照片诊断 | 406,256 | 仅编译，不转移旧S1实屏结果 |
| Read Pico SD诊断 | 332,304 | SHA仍c876599f…，未重新部署/执行探针 |
| Read Pico显示诊断 | 305,120 | 仅编译，未清白/刷新 |
| Read Pico只读工厂诊断 | 275,344 | 仅编译，未运行 |
| 原PhotoPainter板 | 2,059,248 | 保留原文件策略，仅编译 |

完整集成SHA256 `947458be9720c7349c567e6ed142a38e6ac502a5e33fc93993f95fa276bc241f`；SD SHA256 `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3`。本次增量编译改动源无warning/error；其他固定组件历史告警边界不扩大。依赖锁只保持已存在的manifest/direct-idf标记差异，组件版本未变；未覆盖用户修改、固定厂家代码、提交/推送或纳入私有资料/构建生成物。

本机日志`.cache/p3-asset-{host,asan}-{build,tests}.log`、`p3-asset-python-{build,console}.log`及`p3-asset-idf-{integration,photo,storage,display,factory,legacy}.log`均忽略。下一安全开发项为其余消费者/并发、旧持久元数据/NVS/OTA和暂存空间策略。真实浏览器HTTP/URL/重定向/TLS、BMP/EPDGZ完整失败路径、重启恢复、满卡/热拔/掉电/长稳与完整产品验收仍待完成；本轮软件成功不替代这些结果，也不触发自动删除/再烧录/再显示。

### 2026-10-07 · P3只读严格配置加载和OTA门禁（仅软件）

继续完整目标的软件开发，上一上传/URL轮为实际进展而非等待/阻塞。本轮重新读取TODO、需求/架构/文件索引、真实main/config/OTA和对应测试；fast-context仍缺密钥，用限定rg及相关函数核对，不提取Keychain/Windsurf凭据。未操作设备、串口、测试卡、原片、显示、业务网络/NVS/分区或相机；最后验收会话是c876599f…SD诊断/STOPPED，本轮不作新的在线状态轮询。

定位到旧配置加载将密码读取错误当认证关闭、证书读取/分配错误当无证书，并自动持久化legacy cron迁移/删除quiet-hours键；OTA初始化还自动查询/保存状态/注册检查和PENDING_VERIFY启动即确认，资产匹配仅按文件名。当前核验factory布局没有otadata/OTA槽，不可据此启产品或上游升级。

新板启动先不擦除恢复的NVS init，再真实`config_manager_init`只读加载；失败在board/storage/display/network前返回，不panic/重试/自动修复。便携`config_read`锁存第一个非缺失错误，检查字符串报告长度/完整NUL、标量范围和已知枚举；密码/证书类型/长度/IO/分配故障拒绝启动，CA≤4096、两次读长度一致，证书第二读消失也拒绝。默认值完整复位，不复用前次合成启动的密码/CA，未挂载的新板仍默认必需SD，不误用上游MemFS/URL默认。有效0/180角度不改写，不证明语义已迁移；旧cron只在RAM预览、旧键保留，存储cron解析失败/超条数/过长拒绝加载。

新板OTA只初始化构建/不可用状态和状态锁，检查/更新NOT_SUPPORTED、daily=false、不做任务/网络/NVS/分区/确认回滚/重启。原板完整实现和加载迁移/密码读错误策略保持。临时门禁不等于OTA功能实现或迁移完成，不降低第一阶段最终要求；真实integration入口仍不调用`photoframe_app_main`。

新增23项CTest：便携读取保护5项、真实Read Pico配置管理源码13项（包含全已读字符串错误循环、标量/类型/cron/证书/内存故障、缺失/重复合成启动、旧键保留、挂载前默认），原板真实配置管理2项，真实OTA门禁3项。OTA主机目标无NVS/网络/分区/periodic/task/restart替身符号，误链接任何相关操作会失败；NVS测试通过替身计数核对Read Pico无RW打开/写/erase/commit与正确关闭。并非读取设备上的真实设置或真实NVS闪存故障注入，也不证明运行时并发重载、所有配置消费者或多键持久原子性。

最终普通CTest **600/600，29.69秒**；ASan/UBSan **600/600，111.52秒**。Python构建保护31项（新增2项）和控制台17项，共**48/48**；git diff --check通过。初次主机编译暴露config_manager依赖SDK传递包含stdlib的缺陷，补显式stdlib后通过，不用放宽编译标准掩盖；第一版启动错误后检查卸载/清理，最终前移加载至板级/SD之前，新断言核对更早门禁，旧板仍保持存储→配置顺序。最终又强化NVS_RW打开计数断言并完整重跑主机/ASan/Python成功；测试修改不改固件输入，六配置/ELF证据对应同一最终生产源码。

六配置最终均编译通过，修改源无新增warning/error，固定SDK的既有Kconfig NOTE边界保留。Read Pico完整集成1,893,808字节，SHA256 `fa2a3f94cabc13bfe8bb02ec6c1a36a2bdc730f3f6afa83b582fc4236fffbffa`，仅编译、未部署；照片406,256、SD332,304、显示305,120、工厂只读275,344、原板2,059,248字节。SD SHA仍`c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3`，没有重写或重跑探针。集成变小主要由于新板未验收OTA实现不再链接，不作为内存/功能完成或实机性能指标。

最终ELF存在真实产品、config_manager及ota_manager/check/start API；Read Pico无`ota_check_task`、`ota_update_task`、`esp_https_ota_begin/perform/finish`或`esp_ota_mark_app_valid_cancel_rollback`符号，原板均保留。此检查只证明本ELF选定路径/符号的链接保护，不扩大为SDK全部Flash写能力已移除。现有本机构建rollback配置关闭，设备旧bootloader本轮未读取，不能宣称已确认其回滚能力。

新增[迁移审查](READ-PICO-MIGRATION.md)：列出旧NVS/PHY/factory/FAT与上游默认布局冲突、未实际读取的配置组、方向覆盖/旧链接边界及OTA镜像身份/备份/回滚条件。没有生成/选择/接入新活动分区、擦数据、写厂校准或启业务。用户原修改/固定厂家组件保留，SDK/组件版本未变，依赖锁维持既有manifest/direct-idf标记差异；无提交/推送或私有材料/生成物入库。

本机日志为`.cache/p3-config-{host,asan}-{build,tests}.log`、`p3-config-focused-tests.log`、`p3-config-python-{build,console}.log`、`p3-config-idf-{integration,photo,storage,display,factory,legacy}.log`及两个ELF符号日志，均忽略。下一安全开发项是其他NVS/文件消费者、Wi-Fi失败/导入、元数据与空间回收策略；实际配置/分区迁移、重启/TLS/HTTP/SD/显示/PMU和OTA恢复仍需后续明确权限及验收，本轮不是新的部署或产品运行许可。

### 2026-10-07 · P3 Wi-Fi完整导入、表单和保留凭据重配（仅软件）

上一配置/OTA轮是实际进展。本轮重新检查TODO和真实storage/wifi_manager/provisioning/main/HTTP停止代码；fast-context仍缺Windsurf Key，限定rg定位并读取相应函数，不提取凭据。没有串口、复位、设备NVS/卡/原片/显示/实际网络或相机操作；最后实卡会话仍c876599f…SD诊断/STOPPED，本轮不做新的实时状态确认。

旧文本读取会截断SSID/密码、在读完/关闭前保存设备名，未检查文件结尾/关闭；表单也只单次接收并接受错误百分号/截断字段。新板新增便携wifi_text：位置文本≤1024字节、LF/CRLF、两字段及可选名称，空/注释语义保留、开放密码用明确双引号标记，字节长度/密码形态及整份NUL/控制/额外字段拒绝，输出只在完整解析成功后变化。Read Pico C容量33/65支持32字节SSID/64位十六进制PSK，原板32/64不变；SSID/密码首字符#的文本表示、WEP/企业认证等限制见Wi-Fi说明，不宣称任意格式/网络均支持。

真实storage只读按优先目录打开，仅ENOENT允许根路径后备；有效SD、普通非空≤1KiB、长度/EOF/ferror/关闭/最终介质核对之后才复制值，关闭错误锁存SD。读入不写名字/NVS、不删文件；主机临时文件包括优先坏文件、目录/错误父路径、过长/坏格式、关闭故障/关闭时失介质及完整wire字段。此处SD状态/关闭故障为替身，不计实卡读wifi.txt、热拔或真实关闭失败验收。

新板main对非缺失读取错误/凭据或名称保存错误停止此分支，不当作无配置AP/重启/成功；名称新增检查式set→commit→RAM提交。已成功的凭据键或名称set可能留在NVS，不伪称多键回滚。完整配置组、网络参数上游void保存、所有错误退出资源收敛仍待审查，源码return不等于整机已正常停止。

新板配网表单≤511字节完整接收，连续8次超时失败，先检查已知字段重复/容量及正确百分号编码/控制字符，再沿用字段提取；没有全程硬取消/严格总时限保证。连接/保存前验证凭据，station按全长复制，开放网络用OPEN；原板解析/容量/连接规则保留。实际SDK连接事件/测试候选/网络配置及并发尚未完整验证，不能把字符串测试当无线/TLS验收。

连接失败分支新板不擦SSID/password，停止当前重连并检查原HTTP停止，进入原AP恢复，等待本次新保存/名称/响应完成后重启；失败不立即擦除/重启或重复创建服务。正常OOBE也不以旧SSID/中间NVS写入判完成。易失性原子provisioning_session ticket只在当前完整成功后commit，旧ticket/重复/未完成不认成功，完成后不再发新保存ticket。不是第二套产品状态/相册系统，也不替代NVS事务、客户端确认或已开始写入回滚。新板AP模式/配置/启动/IP/DHCP/HTTP注册/路由/DNS检查并清理，HTTP停止错误保留所有权；实际AP开放、后台任务和SDK完整失败/退出仍待验收，产品入口继续禁止运行。

新增26项CTest：wifi_text/凭据9项、表单3项、真实storage/主机文件7项、完成ticket4项、真实config_manager名称保存3项；首次针对文本/storage的26项集合含原7项，不能当新增26项，最终新增数由全量600→626核对。最终普通CTest **626/626，30.73秒**，ASan/UBSan **626/626，126.98秒**；Python构建33项（新增2）与控制台17项，共**50/50**；git diff --check通过。

六配置最终编译均成功。第一次扩展公共SSID/密码C容量使原板原snprintf触发format-truncation编译错误，改为仅Read Pico扩展、原板保留32/64后重跑全部配置；没有禁用-Werror或修改原板截断规则来掩盖。新wifi/storage/session代码无编译告警；集成日志仍有既有crash_log未使用TAG/save_record和Read Pico禁用factory-reset后未使用restart_task告警，未隐去或当作全工程零告警。

| 最终配置 | 应用字节 | 部署范围 |
| --- | ---: | --- |
| Read Pico完整集成 | 1,896,816 | 仅编译，门禁不启动真实main |
| Read Pico照片 | 406,256 | 仅编译，旧S1结果不转移 |
| Read Pico SD | 332,304 | SHA仍c876599f…，未重写/重跑 |
| Read Pico显示 | 305,120 | 仅编译，未刷新 |
| Read Pico只读工厂 | 275,344 | 仅编译，未运行 |
| 原PhotoPainter | 2,059,280 | 仅编译，原策略保持 |

集成SHA256 `e42ca71083918e9d65a0bcda8587fd1ce8e483d773dc9bd2ded35feac2cd3c1c`，SD仍`c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3`。SDK/组件版本、固定厂家代码、实际分区/波形/VCOM和现存用户修改保持；无提交/推送、真实凭据/私人照片或构建生成物入库。测试所有文件/配置值为合成数据，临时目录为主机测试自建范围。

源码、TODO、README、CHANGELOG、架构/存储/集成/迁移与[Wi-Fi说明](READ-PICO-WIFI.md)已同步；日志`.cache/p3-wifi-{host,asan}-{build,tests}.log`、`p3-wifi-focused-tests.log`、`p3-wifi-python-{build,console}.log`和六个`p3-wifi-idf-*.log`忽略。下一安全开发项为连接事件/其他Wi-Fi与NVS消费者、后台任务/错误退出、完整多键保存及元数据/空间策略；真实AP/NVS/重启/热拔/失败恢复和完整产品仍需后续明确权限，测试卡factory更新许可不授权广播OPEN AP、改设备配置或新的显示。

### 2026-10-07 · P3成对凭据、候选/当前连接与SDK错误核对（仅软件）

上一轮确有软件进展；本轮核对当前文件与已完成的运行句柄/日志，完成连接消费者审查。fast-context缺Windsurf Key，改用限定本地rg及读取实际函数，不提取密钥。没有串口/复位/烧录、设备凭据、卡/原片/显示/业务网络或相机操作。最后已知设备仍为c876599f…SD诊断/STOPPED，本轮没有新的设备状态采样。

新`wifi_credentials`抽出既有两键NVS契约，Read Pico先局部成对只读：类型、报告长度、唯一末尾NUL及凭据格式均检查，句柄关闭后才同时输出；两键均缺失/均空为未配置，部分缺失或异常为损坏，不发布半份凭据。main硬件启动前检查该对，普通/唤醒入口读错不走网络拒绝/自动修复，配网状态也区别缺失与错误；旧板原读取顺序/容量保留。明确保存仍是既有set/set/commit，不能推导为两键/名称/其他网络参数原子性。

Read Pico driver初始化关闭其NVS配置缓存并检查RAM模式；失败候选不继续。真实manager单连接锁、先停止旧station（候选保留AP）、默认事件队列私有固定大小消息栅栏、设置目标并在start/connect前清位，避免快速新事件被后续清零。关联事件核对目标SSID/BSSID及驱动现场；IP须匹配station接口、当前非零地址和当前关联。丢IP/stop/断线取消旧就绪，查询也复核现场，不靠旧bit。便携wifi_link不是新产品状态系统；SDK无应用尝试ID，这组保护不证明所有延迟、同目标快速重连或真实任务竞态均已消除。

DHCP停/地址清零/静态设置/启动错误传播，即使ALREADY_STOPPED也覆盖保留旧地址。DNS覆盖或静态网关默认DNS失败不发布成功，候选返回原SDK错误并尝试恢复AP；后台/启动connect失败设置失败位。新板性能设置检查每次驱动返回，避免跨重连的缓存假成功；完整地址查询失链/读取/容量错不修改输出，真实在线启动先检查该返回值才日志/NVS/在线通知。普通非认证超时保留后台继续契约；失败清理若SDK再错只日志，不假称无线或资源已退出。

新增33项CTest：真实凭据源码6、便携关联5、真实manager+合成SDK/事件队列22；manager目标不提供NVS保存/擦除符号，误链接会失败。覆盖旧成功位/排队旧IP、错误SSID/BSSID/接口/地址/长度、快速回调、候选AP保留/恢复、栅栏丢失及SDK模式/设置/启动/连接/DHCP/DNS/省电故障、认证有限重试、stop/丢IP、完整32/64wire字段、过期/容量不足地址与继续重连失败。NVS/无线都为替身，等待即时推进队列；不是实际FreeRTOS调度、无线时间测量、掉电/NVS实机故障或并发整栈验收。新增3项Python核对候选/driver RAM门禁、main读错分支及在线地址返回检查；不能以词法断言代替运行证据。

最终普通CTest **659/659，30.77秒**，ASan/UBSan **659/659，124.21秒**；Python构建36、控制台17，共 **53/53**；git diff --check通过。中途背景重连新用例只注入一次断线却断言已耗尽重试，真实实现未耗尽而测试失败；改为明确注入六次断线后全量通过，不改生产重试策略迎合测试。651/657为中间轮次，最终659包含DNS/地址/省电扩展。

六配置编译均成功；最后地址调用方修改后，又使用同一固定SDK环境的现有Ninja构建重编集成及原板，并核对对象时间晚于源码，不复用先前main对象。四个隔离profile不链接改动的业务main/Wi-Fi源，产物身份保持。最终应用：集成 **1,898,896** 字节/SHA256 `8adbdec6e0fb86c2c7cadd4f468d05a440c731e17dd234e2e900ee4cc2a40b7f`；照片406,256、SD332,304、显示305,120、工厂275,344、原板2,059,264字节。SD SHA仍`c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3`。修改源最终编译无新增warning/error，固定SDK既有Kconfig NOTE仍存在，不宣称全部工程/历史构建零告警。

固定厂家子模块`28cde682a4468a581c278761922724f57d976418`工作树无修改；SDK/组件、分区、波形/VCOM、现存用户修改保留。集成仍只保留真实产品入口的链接，不调用它；没有OTA/产品启动许可、提交/推送或私有资料/生成物入库。日志`.cache/p3-connection-{host,asan}-{build,tests}.log`、`p3-connection-focused-tests.log`（中间目标集合）、`p3-connection-python-{build,console}.log`、六个`p3-connection-idf-*.log`及集成/原板`-final.log`忽略。

本轮未解决静态IP/DNS在配网测试前通过void多键保存的问题，不宣称失败候选NVS完全不变；部分初始化清理/ESP_ERROR_CHECK、全配置保存、后台生命周期及真实无线/AP/重启/断网恢复继续审查。下一安全开发项为临时候选网络参数与检查式保存及其消费者；完整产品/NVS/分区迁移、硬件与网络验收仍需明确范围和人工条件。

### 2026-10-07 · P3临时候选网络、显式组保存与过期清理（仅软件）

上一轮实际完成659项回归和连接检查。本轮复核TODO与真实provisioning/config_manager/wifi_manager接口，fast-context仍缺Key，使用限定rg读取，不提取凭据。没有串口、复位、烧录、卡/原片/设备NVS/显示/业务网络或相机操作；设备最后已知仍为c876599f…SD诊断/STOPPED，不做新的实时状态确认。

Read Pico AP不再在候选连接测试前调用网络参数void持久setter。新增便携wifi_network，验证IP模式/有界四段IPv4，DHCP保留此前未使用静态字段；没有发明子网/网关连通或Internet可达验收。manager将候选复制为本次IP/DNS快照，测试不改config_manager缓存/应用NVS；配置getter改变或调用方修改原请求也不改变随后DNS与保存的候选。SDK实际转换及driver RAM门禁保留，旧板仍使用原提前保存和连接分支。

便携provisioning_apply只是既有同步HTTP处理器的顺序帮助函数：复制请求，当前ticket→测试→当前ticket→保存→当前ticket→响应→完成。测试错误/测试中会话过期不进入保存；保存错误不发成功页或完成，保存中ticket变更单独报告“已保存但未完成”，不假称未保存。响应失败不重复已完成保存，最后原子ticket commit失败不自动重启；客户端收到成功页后仍可能会话变化，无法收回响应，不把发送成功当客户端确认。函数不拥有新的产品配置、状态或相册系统。

新板config_manager_save_provisioning_checked复制SSID/密码/名称/网络输入；一个NVS句柄检查七个字符串set及模式set，共八键，再一次commit与关闭，全部成功才逐字段更新RAM。错误保留旧RAM值并如实报可能的NVS部分持久化，不擦除或自动修复；输入别名/IO替身修改原输入不会变更所存快照。不是NVS多键/掉电事务，也不是多任务RAM原子发布。普通Web setter和其他配置消费者还未统一此契约；旧板新API为NOT_SUPPORTED，原路径保持。

manager成功候选返回本次不复用的应用连接所有权ID。后续保存/响应/会话失败只尝试取消匹配ID的候选station并保持AP；晚到旧处理器不能关闭后来同SSID新候选。模式恢复失败返回/记录未确认，不以逻辑取消宣称station真已关，也不反复尝试或自动重启。该ID用于清理所有权，不伪造SDK事件尝试ID；无线RF/服务并发和生命周期仍须真机验证。

新增 **32项CTest**：wifi_network语法/容量/不发布输出6；阶段顺序10（每阶段失败、测试/保存中会话过期、请求改变和保存后不重试/回滚）；真实新板config_manager组保存7（完整组/完整wire字段、打开/逐个八键/commit错误、RAM不提前发布、输入快照与别名）、旧板无操作1；真实manager临时候选/配置快照/失败恢复AP5及所有权取消/过期清理/清理SDK错误3。合成SDK事件、NVS立即在set落入主机map及故障注入不是实体Flash/FAT或FreeRTOS/RF并发证明；HTTP阶段的socket/全服务整栈未运行。新增1项Python检查生产接线/无提前setter/保存返回与清理所有权；词法检查不是实际处理器运行证据。

最终普通CTest **691/691，31.70秒**，ASan/UBSan **691/691，128.83秒**；Python构建37/控制台17共 **54/54**；git diff --check通过。64项聚焦集合是增加阶段帮助函数前的中间证据，不当最终新增数；689为增加保存输入快照和保存中过期门禁之前的全量，中间值不覆盖691。

首次受限环境Ninja因CMake源码列表变化重新生成，组件索引连接失败；进程已终止后在主机环境使用相同锁定SDK/配置编译六种profile成功，不更换或绕过依赖版本。第一次六配置构建与最后会话门禁源码修改时间相差数秒，随后补编集成和原板，用对象时间核对最终输入；未复用旧provisioning_apply/handler对象。集成日志仍显示既有Read Pico禁止reset后restart_task未使用告警和固定SDK Kconfig NOTE，不掩盖或宣称全工程零告警；新增网络/保存帮助代码编译无新增告警。

| 最终配置 | 应用字节 | 本轮范围 |
| --- | ---: | --- |
| Read Pico集成 | 1,901,120 | 仅编译，真实main启动仍被门禁拦住 |
| Read Pico照片 | 406,256 | 仅编译，未刷新/读原片 |
| Read Pico SD | 332,304 | 仅编译，c876599f…身份保持 |
| Read Pico显示 | 305,120 | 仅编译，未执行显示 |
| Read Pico工厂 | 275,344 | 仅编译，未运行 |
| 原PhotoPainter | 2,059,264 | 仅编译，原保存/连接分支保留 |

最终集成SHA256 `8ab2513d646ac21aed479af8ad080f477da37b199c24f218a13a1a144ecd1b2f`；SD仍`c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3`。固定厂家子模块28cde682…无修改，SDK/组件版本、实际分区、波形/VCOM及现存用户修改保持。依赖锁只保留此前manifest_hash/direct-idf标记差异，未改变组件解析版本；无提交/推送、凭据/私人照片或构建生成物入库。

日志为`.cache/p3-network-{host,asan}-{build,tests}.log`、`p3-network-focused-tests.log`、`p3-network-python-{build,console}.log`、六个`p3-network-idf-*.log`及集成/原板`-final.log`，均忽略。TODO/README/CHANGELOG/架构/索引和[Wi-Fi说明](READ-PICO-WIFI.md)已同步；下一安全工作为其他Web配置setter与消费者、部分初始化/扫描/后台生命周期及元数据空间策略。完整产品/NVS/分区迁移、真实AP/NVS保存/异常断线/重启、HTTP/显示/PMU仍需后续明确权限和人工条件，不扩大测试卡factory许可。

### 2026-10-07 · P3检查式AP扫描、响应与测试资源互斥（仅软件）

按TODO继续审查真实wifi_manager/provisioning扫描。fast-context仍缺服务Key，使用本地限定搜索；不提取凭据。固定SDK esp_wifi.h核对：成功get_ap_records释放整个扫描列表，读取失败需要clear_ap_list；既有cJSON实际API及失败所有权也已核对，没有猜测接口。没有串口/复位/烧录/SD/原片/NVS/射频/AP或相机操作；最后已知设备仍为c876599f…SD诊断/STOPPED，非本轮实时观察。

新板scan_checked仅允许空闲AP，容量1–20项，连接锁拒绝扫描/候选重入和活动连接。检查模式/切换/扫描启动/数量/记录，局部结果完整NUL及数量检查；清理自己的列表并恢复AP成功后才发布记录和count，失败两份输出不变。拒绝的scan_start没有取得列表所有权，不clear别人的列表；成功get_records按SDK契约不重复释放。零热点也需列表清理/恢复成功，不能将SDK错误当空列表。清理失败保留首错、记录不确定并关闭原子driver就绪门禁、取消目标；后续检查式扫描/候选拒绝复用，不自动重试/重启或声称射频已关。

HTTP新板分支传播扫描错误；热点名改用已依赖cJSON构建/转义，检查分配、字段/数组加入、打印以及类型/响应发送，正确释放所有权。移除固定80字节/项和未检查snprintf累加路径。旧板扫描/响应保留，新检查式API旧板返回NOT_SUPPORTED。仅C字段完整/JSON转义保护，不宣称任意二进制SSID、浏览器或实际HTTP/RF已验收；manager锁不覆盖所有直接SDK调用、AP/HTTP启停、后台休眠或部分/重复初始化。

新增12项真实manager+合成SDK测试：上限/成功恢复、空结果、非法参数、活动候选/STA拒绝、扫描/连接重入、模式/启动/数量/记录故障、未终止SSID/超量返回、恢复/清理失败阻止复用。SDK替身即时返回、事件按合成队列推进，不是实际阻塞扫描耗时、FreeRTOS调度或无线验收。新增1项Python仅核对检查式调用、cJSON/返回和测试资源锁，非HTTP运行证明。

第一次普通-j8、ASan-j4并行全量各有1/2项旧DisplayFlowTest失败：同一构建目录pf_storage被多个case的SetUp/TearDown删除/重建，出现mkdir=-1与应保留文件消失；失败日志保留。串行重跑普通 **703/703、31.00秒**，ASan/UBSan **703/703、139.32秒**通过。未改产品显示行为；在该fixture的gtest_discover_tests添加CTest RESOURCE_LOCK，防止同目录用例互相清理。加锁后普通-j8 **703/703、9.93秒**，ASan/UBSan-j4 **703/703、35.45秒**通过。其他套件/独立构建目录仍可并行，不宣称任意外部进程共享工作目录的全局锁。

Python构建38/控制台17，共 **55/55**通过。六配置使用固定IDF v6.1/fff9895c…和既有依赖完成编译，未新增组件解析或更改SDK/组件版本；最终集成/原板wifi_manager和provisioning对象时间均晚于最终源码。集成 **1,901,552字节**、SHA256 `3c2bf6dfd5946036b4c077d5e4339647150df133a3c7b52503a85774daff6bc3`；原板 **2,059,264字节**、`3cef497902e3bdf94f48998554729a61ae87e1eeccda55600a24057424b1d82c`。照片406,256、SD332,304、显示305,120、工厂275,344字节保持；SD SHA仍c876599f…，四个隔离profile不链接改动业务源。最终本轮编译日志无新增warning/error；中间C label声明扩展警告已移除，不把整个历史工程称为零告警。

固定厂家子模块28cde682a4468a581c278761922724f57d976418干净；实际分区、波形/gamma/VCOM、产品入口门禁和用户修改保持，无提交/推送或私有材料/生成物入库。日志`.cache/p3-scan-{host,asan}-{build,tests}.log`、focused、`*-serial-tests.log`、`*-locked-{build,tests}.log`、Python与六配置idf日志忽略。TODO/README/CHANGELOG/架构/索引和Wi-Fi说明同步；下一安全开发为部分/重复初始化/错误退出、后台与其他配置消费者，不以本轮扫描保护代替完整产品或迁移验收。

### 2026-10-07 · P3检查式Wi-Fi初始化与发布前失败退出（仅软件）

上一轮703项扫描软件验证已完成。本轮按TODO继续真实manager/main调用方；fast-context仍缺Key，限定本地读取，不提取凭据。核对固定SDK wifi_default.c/esp_wifi_default.h：默认创建器含assert/ESP_ERROR_CHECK；attach先保存静态netif并可能创建未发布driver，错误后适配层无法证明释放；clear接口API可能已毁driver后返回错误；SDK默认回调清理、driver销毁内部也有忽略错误的调用。固定SDK protocol_examples_common/wifi_connect.c退出范例确认为deinit→clear driver/handlers→netif destroy，沿用其顺序，不修改SDK/厂家来源。

新板初始化原子互斥，首次同步句柄按需创建且稳定保留；部分分配失败显式重试补缺失，不反复替换/增长对象。共享netif核心/默认事件循环应用生命周期保留，不删除其他消费者的基础设施；已有默认循环可借用，已有默认STA/AP拒绝接管。成功重复init不分配/注册、不重置已连接目标；运行态扫描清理关闭ready后，重复init不能绕过锁存。

新板驱动init禁用NVS缓存，RAM存储成功后以检查式new/attach创建接口，不使用abort默认创建器；检查默认回调、hostname、五个本模块回调和初始模式，完整后才发布ready，不start射频/AP/连接。发布前失败保留首错，反序注销成功注册的回调，再退出确认自有驱动并检查接口解绑/销毁；公开API返回成功的路径允许显式重试，不自动循环。driver init错误未取得可靠所有权，不deinit可能属于别人或未知部分状态的driver；attach错误保留其静态指针/接口/driver，任何注销/deinit/clear错误也锁存，不重复释放或声称零泄漏。回调在ready未发布时忽略，预初始化stop/keep/disconnect不碰空锁或射频。

main普通启动和唤醒入口检查初始化返回，错误不进入之后配网/在线分支，也不因ESP_ERROR_CHECK触发panic重启；旧板分支保留。这不是此前板级/SD/周期任务/全部业务对象的退出或运行态deinit；默认SDK帮助函数内部忽略错误的细节也不以公开API成功推断为每个内部资源均已故障注入验收。稳定RTOS句柄/共享核心是有界应用生命周期选择，不是假称所有失败资源归零。产品启动门禁保持。

新增 **28项CTest**，真实manager配合合成SDK/事件/RTOS替身，覆盖成功重复且保留已连接目标、借用默认循环/拒绝外部接口、三处分配/两处netif/五个回调逐点错误、core/loop/driver/RAM/STA/AP attach/默认回调/hostname/模式、注销/driver/接口清理错误、重入、预初始化调用及运行态锁存不可通过init清除。原RAM失败用例改为真正第一次初始化失败，不用先成功再假设第二次init重置；原省电失败仍独立保留。CTest每项新进程模拟设备新启动，不把不可恢复锁存在人为重置同进程后假称产品可恢复。对象标记/调用计数和即时替身不是真实SDK堆、射频、FreeRTOS并发/故障验收。新增1项Python源码接入断言，不能代替实际main启动。

针对性 **70/70、0.48秒**；最终普通CTest-j8 **731/731、11.25秒**，ASan/UBSan-j4 **731/731、38.05秒**；Python构建39、控制台17，共 **56/56**。保留上一轮显示fixture资源锁，并行全量本轮没有失败。源码diff检查通过，本轮最终host/固件日志未出现新增warning/error。

固定IDF v6.1/fff9895c…及既有依赖下六配置编译通过；首次编译后删除旧板分支内失效的Read Pico条件，再完整跑现有六配置Ninja构建。最终集成/原板manager与main对象时间晚于最终源码；集成map的wifi_manager_init实际段0x3c3及初始化状态符号、对象中的新失败日志确认新路径编入，不只凭bin大小推断。最终集成 **1,901,520字节**/SHA256 `dfcf1fc0d0fd4aa1ecd7f17b6cd127fdca86c764a304ebc8df58c0243e3cc4db`；原板 **2,059,264字节**/`5dc6fbab004b5fb943b4ae112fb804314f0dfeaea6740516b21f0df2d9788ffb`。照片406,256、SD332,304、显示305,120、工厂275,344字节保持；隔离profile不链接改动业务源，SD仍c876599f…。

没有串口/复位/烧录/SD/原片/设备配置/NVS/射频/AP/相机操作，无提交/推送；最后已知设备c876599f…SD诊断/STOPPED不是本轮实时确认。固定厂家28cde682a4468a581c278761922724f57d976418干净，分区/波形/gamma/VCOM/SDK组件版本及用户修改保持。日志`.cache/p3-init-{host,asan}-{build,tests}.log`、focused、Python、首次idf及六配置`-final.log`忽略，文档索引/状态同步。下一安全开发为AP/HTTP/后台启动失败和退出、其他SDK/配置消费者；真实初始化/无线/HTTP/NVS/重启和完整产品仍需具体权限及人工验收，不扩大测试卡factory许可。

### 2026-10-07 · P3配网DNS报文边界与端口所有权（仅软件）

按TODO继续检查配网服务。fast-context不可用后限定本地源码搜索；原DNS在异步bind之前返回成功，且将最多512字节原输入复制到512字节输出后再追加16字节答案，存在越界；跨任务close与100毫秒等待没有端口退出确认。只为Read Pico增加分支，旧板保持旧实现，不宣称旧板缺陷本轮已修复。核对固定SDK lwipopts.h确认收发超时支持，FreeRTOS tasks.c确认xTaskCreate允许NULL输出句柄。协议布局/限制参考[RFC1035](https://www.rfc-editor.org/rfc/rfc1035)和[RFC6891](https://www.rfc-editor.org/rfc/rfc6891.html)，受限实现边界明确如下。

便携captive_dns有界解析单问题IN、非压缩标签/≤255字节wire名称、头部计数/标记和可选单个EDNS OPT，失败输出不变。A返回固定IPv4，其他IN类型零答案；响应不冒充递归/认证。OPT校验完整长度和选项边界、不回显未知选项，声明512字节、保留DO，未知版本返回BADVERS。不是通用DNS或全RFC符合性：压缩/多问题/非IN/畸形报文忽略，不实现全部标准错误响应、TCP、DNSSEC或缓存。512字节合法EDNS输入可缩短构造响应而非原长度追加；513及更大输入拒绝。

真实新板适配同步检查socket/250毫秒收发超时/仅AP地址192.168.4.1:53的bind/任务创建，再报告成功；512+1接收缓冲检测超大报文，每次重置客户端地址长度。任务创建前调用方拥有端口，之后仅后台关闭；临时收发错误丢当前报文/继续，致命或短发送结束后台。停止原子控制请求、等待2秒完成位并检查close结果；超时仍持有资源、禁止start，后续显式stop可等真正完成，关闭不确定锁存且不双重关闭/强删。完成位是端口关闭尝试和最后共享状态的确认，不是idle已回收任务堆的证明；稳定事件组按应用生命周期保留。AP停止先检查DNS，再HTTP和射频停止错误，不假称完整退出。AP自身初始radio/DHCP停止及服务互斥仍未处理。

新增 **29项CTest**：便携报文12、真实DNS源码/合成RTOS/socket17。覆盖精确响应、ROOT/非A、255/256名称、所有截断前缀/错误计数/标签/类型、EDNS/BADVERS/512上限及10,000组确定性随机输入；端口/超时设置/任务创建错误、同步提前后台故障、停止超时后确认、关闭失败锁存、临时/致命/短发送、地址长度复位与重入。不创建真实主机UDP socket；替身等待即时执行后台，不能当实际2秒时间测量、FreeRTOS并发或无线验收。每项CTest新进程运行锁存用例。新增1项Python源码接入/调用顺序断言，不能代替实际AP运行。

针对性 **29/29、0.27秒**；最终普通CTest-j8 **760/760、10.41秒**，ASan/UBSan-j4 **760/760、35.25秒**；Python构建40、控制台17，共 **57/57**。首次主机link缺少替身esp_err_to_name，补齐后通过；初次沙箱固件CMake重配无法读取组件注册信息，未算成功，随后授权主机环境固定SDK六配置编译成功，不使用失败缓存日志冒充完成。最终构建日志无新增warning/error，diff检查通过。

最终集成 **1,902,752字节**/SHA256 `66245d1f920b61a398af3b015e141cda4b2dbc20c1ec801bebcc44c7f1874982`；原板 **2,059,264字节**/`0a898e1795b9bc174e656cf4e6677a76cc287fe626c7655c534b7e4ab7406c0d`（重编后身份变化，不用大小不变推断hash保持）。照片406,256/fc202285…、SD332,304/c876599f…、显示305,120/44f691bd…、工厂275,344/1541861d…保持。集成map含真实dns_server_stop_checked/captive_dns_response实际段及调用引用，三个改动业务对象时间晚于源码；原板DNS对象也新编译。固定IDF v6.1/fff9895c…、组件版本、厂家28cde682a4468a581c278761922724f57d976418干净状态保持；依赖锁既有manifest/direct依赖差异未新增版本变动。源码索引/README/TODO/架构/变更/Wi-Fi说明同步，忽略日志为`.cache/p3-dns-*`。

本轮没有串口、复位、烧写回读、显示/SD/原片、设备NVS/真实配置、射频/AP、相机或提交推送操作；最后已知设备c876599f…SD诊断/STOPPED不是实时核查。完整产品/OTA/分区门禁保持。下一安全软件工作为AP/HTTP自身启动失败/退出与后台资源、其他SDK/Web配置消费者和元数据空间策略；真实DNS/端口占用/HTTP/无线/重启/NVS与完整产品验收需另定范围，不扩大测试卡诊断许可。

### 2026-10-07 · P3配网服务所有权、取消与重启前退出（仅软件）

上一轮760项DNS验证为实际软件进展。本轮先读TODO/需求/架构/索引，fast-context仍缺Key，限定本地源码搜索、不提取凭据。检查发现AP初始radio stop及DHCP stop未检查，关闭后旧保存ticket未取消，start/stop可重入并替换服务，main保存完成直接重启不经过AP退出。读取固定IDF httpd_main.c：httpd_start仅成功才写输出handle；httpd_stop失败位于释放之前，成功等待THREAD_STOPPED后删除对象，但其信号量portMAX_DELAY和等待循环没有整体时限。核对esp_netif DHCP接口异常码，不修改SDK或厂家来源。

保留PhotoFrame原配网入口、15个路由和404处理器，提取Read Pico SDK生命周期到wifi_provisioning_ap，不引入另一个产品/相册系统。原板抽出共享路由注册函数，但仍忽略注册错误、保留原ESP_ERROR_CHECK/HTTP失败继续/DNS void启动与停止行为，不宣称旧板本轮已获得新保护。新板控制入口原子重入门禁、活动或失败状态不重复建服务；SSID1–32字节先复制快照。manager只在检查式ready时返回稳定自有AP接口，未准备或已故障不按ifkey接管/停止其他driver；这个getter不预留所有无线操作的锁。

同步检查初始radio stop（仅OK/NOT_STARTED接受）、模式/配置/启动、DHCP stop（仅OK/ALREADY_STOPPED接受）、静态IP/DHCP start、HTTP句柄/所有路由/404、DNS启动。全部成功才begin本次ticket、发布活动状态；此前HTTP资产可响应，扫描/保存受门禁拒绝。初始stop出错未取得AP所有权，不自动第二次stop；HTTP异常发布句柄或成功空句柄视为SDK契约不确定，不盲目释放，锁存到受控恢复。正常启动失败保留首错并按已取得所有权退出，不以清理成功覆盖原错误。

便携provisioning_session增加取消态，generation保持不复用（两位状态，最多0x3fffffff代）；stop/失败使活动/已完成旧ticket失效，下一start生成新ticket。退出检查DNS→HTTP→radio，确认释放才清对象，不跨任务强删或丢弃未确认句柄；空闲stop不调用radio API，避免停止后来独立station。DNS超时允许显式再次等实际关闭确认；DNS close失败锁存不靠重试清除。HTTPstop失败保留相同句柄供后续显式stop检查，不自动循环；radio失败不重复已确认关闭的DNS/HTTP。取消不回滚已开始的NVS保存或响应，不是全栈/多键/掉电原子或客户端ACK证明。

main恢复配网及首次配置两个等待点在AP取消后返回，不永远等待旧保存；保存成功后保留3秒响应延时，实际调用APstop、检查成功才esp_restart，退出失败返回、不自动重启。首次配置setup-complete仍沿用原NVS写法，未在本轮修复所有setter或元数据事务。SDK HTTPstop仍可能无界等待，既有manager/普通HTTP/其他直接SDK消费者尚未与AP控制门禁形成整体互斥；后台任务和整栈退出继续审查，不把本轮作为第一阶段完成。

新增 **24项CTest**：真实AP适配+合成SDK20、真实manager接口准备1、便携session取消3。覆盖完整SDK调用顺序/固定IP及OPEN AP参数、32字节SSID/SDK调用期间原缓冲改变、未初始化拒绝、各radio/DHCP/HTTP/路由/DNS启动错误、初始/SDK契约不确定、清理首错、DNS超时/close失败、HTTP同句柄重试/radio重试、关闭顺序、重入/重复、空闲stop不触radio、已完成ticket取消。取消/commit竞争用真实主机线程1000次核对，不能推广为FreeRTOS/无线并发验收。SDK/HTTP/DNS/延时为替身，无实际端口/网络；不可恢复锁存用例按CTest新进程运行，不人为清除产品状态。路由处理器及真实main只由Python接入/顺序和固件编译验证，未在主机运行HTTP或main业务。

针对性最终 **88/88、0.14秒**（过滤为AP/session/非参数化manager名称，全部参数化用例仍纳入全量）；普通CTest-j8 **784/784、10.19秒**，ASan/UBSan-j4 **784/784、36.69秒**；Python构建41、控制台17，共 **58/58**。收紧DNS close故障测试，不把现实中不可清除的锁存替身改成下一次成功；最终全量重跑通过。Python确认全部15个URI及注册门禁、两个main退出调用和取消等待，不能把源码断言当真实响应/重启验收。diff检查通过，最终host/固件日志无新增warning/error。

固定IDF v6.1/commit `fff9895c82d744c7237be8847347bdd1b07c6643`主机环境六配置编译成功；接入main退出调用后使用同一环境缓存Ninja重新编译集成/原板，相关对象时间晚于最终源码。首次集成map显示公开APstop只在discarded列表，不能假称其实际保留；接入main后最终map实际.text含wifi_provisioning_stop_ap、wifi_provisioning_ap_stop、stop_owned、cancel、manager getter及真实photoframe_app_main，证明调用链进入集成链接，不证明已运行。最终集成 **1,903,392字节**/SHA256 `b3a89b263f17a12c537f7dd9952d05703b0c683209b44da85a3af286ec4a07bb`；原板 **2,059,216字节**/`91594deef1c8536b9ec158e22985b0344fe71e719cd76c8c3234a13d4654c5b8`。共享路由提取使原板产物身份/大小改变，不推断字节保持或网络行为实机回归。

四个隔离配置不链接本轮业务源，身份保持：照片406,256/fc202285…、SD332,304/c876599f…、显示305,120/44f691bd…、工厂275,344/1541861d…；Read Pico integration app_main仍仅保留业务指针、拒绝调用，产品/OTA/迁移门禁不变。固定厂家28cde682a4468a581c278761922724f57d976418干净，组件版本/分区/波形/gamma/VCOM及用户修改保持，dependencies.lock仅既有manifest/direct依赖差异。日志`.cache/p3-ap-{host,asan}-{build,tests}.log`、focused、Python、六配置`-host.log`与集成/原板`-final.log`忽略，TODO/README/索引/架构/变更/Wi-Fi说明同步。

无串口/复位/烧录回读/设备网络/AP/NVS/SD/原片/显示/相机/提交推送操作；最后已知c876599f…SD诊断/STOPPED不是本轮实时核查。下一安全工作为manager/AP/普通HTTP与其他SDK消费者的共享无线生命周期、后台任务/Web setter/元数据空间策略；真实HTTP/DNS/无线/重启和整栈验收另定范围，不扩大测试卡诊断许可。

### 2026-10-07 · P3配网与manager共享无线控制（仅软件）

接续784项生命周期轮次，按TODO审查AP与manager共享无线操作。fast-context缺Key，限定本地源码搜索、不提取凭据；已核对固定FreeRTOS mutex/task API及既有SDK契约。不改固定厂家驱动、SDK、波形/gamma/VCOM、设备分区或产品门禁。

Read Pico复用既有连接mutex，AP begin/quiesce在当前调用取得控制，finish检查owner和当前task并在同任务释放，不跨服务API返回持有mutex。服务保持不复用uint32 owner和阶段，溢出拒绝，不引入另一套产品业务状态。普通station连接/断开/停机/IP设置不得接管AP；稳定ACTIVE允许候选/扫描/省电/hostname但仍与其他控制调用串行。候选/扫描保留IDLE直接API兼容，扫描仍查真实AP模式，不宣称所有未知调用者都已纳入服务lease。

退出先取消保存ticket；manager policy锁下取消旧目标/后台重连并设置失败位，再尝试非阻塞连接锁。候选/扫描busy时不关DNS/HTTP/radio、不give别人的锁；故障finish只释放自己持有的控制，保留服务owner和资源。候选结束自身清理后，后续显式stop才可取锁确认退出，无自动循环/强删。旧owner或另一task不能释放正在执行的AP控制调用；正常停止资源全部确认才清服务owner。候选arm前重新检查阶段，扫描在清理/模式恢复后、policy锁内检查阶段再复制结果。app回调沿用policy锁并检查目标/阶段，取消后不重连旧目标，不使回调等待连接mutex而死锁连接等待。已开始SDK调用仍可能完成，不承诺瞬时硬取消。

新增检查式station stop，接受OK/NOT_STARTED，其他SDK错误关闭ready、禁止自动复用/通过init绕过；main恢复、唤醒凭据错误和网络不可用三处检查其返回，错误不继续后续网络/重启工作。旧板保留原void行为，新板旧void wrapper记录错误。重复init/getter仅在ready且IDLE/ACTIVE允许，不把STARTING/STOPPING/FAILED当准备完毕；native AP改用begin授予控制权，不凭getter操作SDK。

新增 **16项CTest**：真实manager+合成SDK/事件/RTOS12，真实AP适配+合成manager协调接口4。覆盖旧回调不重连、启动所有mutator拒绝且不释放锁、活动服务候选/扫描可用与普通接管拒绝、跨task/旧owner/非法outcome、故障lease/新owner、候选busy先取消但不give候选锁、连接wait嵌套操作、fence期间取消不得arm、scan期间取消输出不变且列表退出、station stop失败/NOT_STARTED，以及AP begin忙不触SDK/旧ticket、quiesce忙保留全部对象、READY发布失败清理与释放、启动清理失败释放自身mutex但保留owner。两套分别运行真实实现验证接口契约，不是单次真实AP+manager+SDK端到端或真实RTOS/RF/HTTP测试；task标识/即时等待为替身。故障锁存case按CTest新进程，未人为恢复产品不可恢复状态。

首次主机编译缺新task替身头和AP协调替身链接，补齐后编译通过；首次798项全量有1项失败：故障后再次busy quiesce阶段变STOPPING，但重复init仍返回OK。修正为仅IDLE/ACTIVE允许重复成功，而非放宽测试；继续发现扫描取消后结果发布缺阶段检查，补代码和2项用例后全量 **800/800** 通过。普通CTest-j8 **10.17秒**，ASan/UBSan-j4 **36.71秒**。新增1项Python接入/控制顺序与main三处调用断言，构建42+控制台17，共 **59/59**；源码断言不证明main真实启动/错误退出已运行。diff检查通过，最终host/六配置固件日志未出现新增warning/error（SDK既有Kconfig NOTE仍保留，不宣称整个工程无诊断）。

固定IDF v6.1/commit `fff9895c82d744c7237be8847347bdd1b07c6643`和既有依赖下六配置编译成功，最终带失败即停的六配置Ninja复跑exit0。集成manager/AP/main与原板manager/main对象时间均晚于最终源码及头。集成map实际.text保留provisioning_begin/quiesce/finish、stop_connecting_checked与photoframe_app_main，不把discarded段当链接成功；入口仍只保留业务指针并输出INTEGRATION_ONLY，不调用产品、板级/SD/显示/NVS/网络。

| 配置 | 字节 | 最终应用SHA256 |
| --- | ---: | --- |
| Read Pico集成（未运行） | 1,905,472 | `558ce60d6c43de90fa51cc7993321c382cbbcbce5d84a87634ba58b97e03bcac` |
| 原板 | 2,059,216 | `519e1d8b192dae9b096f560cb340bb5df0461bb021f5d2579cbb54deca82c9db` |
| 照片隔离诊断 | 406,256 | `fc20228582b2dbd06cc8b0dbbb84d38859dd793b767945c611e9b73dd2338aab` |
| SD隔离诊断 | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示隔离诊断 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| 工厂隔离诊断 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

四个隔离profile不链接本輪业务源且身份保持；原板重编身份变化不以大小不变推断hash保持。固定厂家28cde682a4468a581c278761922724f57d976418工作树干净；dependencies.lock仅既有manifest/direct依赖差异、组件版本保持。普通HTTP生命周期/后台/其他SDK消费者、Web setter和元数据空间策略继续审查。SDK HTTPstop底层仍可无界等待，不宣称整栈互斥/有界退出、SDK内部资源故障覆盖或完整产品验收；取消不回滚已开始SDK/NVS/响应。

烧写/回读：**未执行**；真机验收：**未执行**。没有串口/复位/设备配置/无线AP/业务网络/NVS/测试卡/原片/显示/相机或提交推送；最后已知c876599f…SD诊断/STOPPED不是实时核查。不扩展测试卡更新许可，完整产品/OTA/迁移门禁保持。同步TODO/README/架构/索引/CHANGELOG/Wi-Fi说明；忽略日志`.cache/p3-radio-{host-build,ctest,asan-build,asan,python-build,python-console,sdk-env}.log`和六配置`*-build.log`/`*-final.log`。当前无必须人工介入的软件阻塞，继续上述安全软件审查；真实配网/迁移/整机运行须另定范围和验收。

### 2026-10-08 · P3普通HTTP与配网共用资源、检查式启动（仅软件）

上一轮800项共享无线控制已完成。本轮跨10月7/8日继续普通HTTP与main入口，fast-context仍缺Key，限定本地源码查找，不提取凭据。发现普通HTTP直接覆盖全局句柄、注册返回被忽略、ready停机后未撤销，main HTTP失败仍可能panic或继续报可用。核对固定IDF httpd_main.c：只有成功才输出handle；stop失败在释放之前，成功等待THREAD_STOPPED，但信号量/循环无总时限。未把公开返回成功推导为SDK内部每个socket/线程释放都已被证明。

新增Read Pico HTTP SDK适配，两入口保留原PhotoFrame业务而共享端口80占用和确切自有句柄，显式FRAME/PROVISION角色只供已接入入口，不是业务状态库或认证机制。原子控制拒绝重入；claim先预约，start检查SDK和全部注册，完整才ACTIVE；stop先关闭ready，确切handle关闭成功才清除；仍保留占用至release确认无句柄/无不确定状态。异常失败输出非空/成功输出空锁存，不盲目close未知对象。注册失败保留确切句柄，调用方检查cleanup但保留原错误；stop错误同句柄可显式重试，不自动循环或覆盖start。

普通HTTP入口自身start/stop控制，已ACTIVE重复init不创建/覆盖；新claim失败不清理别人的角色。保留原46个URI/方法、handler和可选Basic认证，Read Pico逐注册检查；原板通过相同注册帮助函数但继续忽略错误、保留原ready/stop语义，不能声称真实原板网络已验收或产物字节保持。Read Pico所有注册路由先检查服务/ready/请求SDK句柄，未就绪503，再进入原认证/handler。旧handler业务ready点也检查本次请求。新start不继承旧ready；退出失败的残留服务不处理新配置/上传。但已经通过检查或正在运行的handler未被撤销，不是SDK/NVS/文件回滚、响应ACK或全程硬取消。

AP在manager begin/改射频之前预约HTTP；FRAME占用时不触无线/manager/原ticket。manager未取得控制则释放自己尚未start的HTTP预约，不凭零owner猜无线控制。仍保留原15路由/404、DNS与本次保存session；退出DNS→HTTP→radio检查，radio未确认之前不release端口，避免普通服务进入未完成退出的AP阶段。idle AP stop不释放FRAME预约，不重复已确认关闭的HTTP；已有不确定SDK对象/失败资源保留。

main普通启动和wake两处检查HTTP init/ready及相关mDNS返回，不panic或报该分支启动成功；HTTP不就绪不进入之后的late Wi-Fi创建或在线通知。mDNS未实现完整失败清理，之前已启动button/periodic/板级/SD等对象不由这些return自动退出，其他SDK消费者、后台创建/生命周期仍待审查。未把main源码return当整机退出成功。Read Pico无调用的legacy restart_task改为仅旧板编译，保留新板factory-reset禁止门禁，消除新编HTTP源码触发的unused告警，不启用擦除。

新增 **13项CTest**：真实共享HTTP适配+合成SDK11，真实AP适配+真实HTTP适配+合成manager/radio/DNS/SDK2；原24项APcase也改用真实HTTP适配。覆盖预约/角色、重入、唯一创建、显式ready及请求句柄、非法参数、注册错误、SDK契约异常、不确定锁存、同句柄停止重试、AP先查HTTP再改radio、radio退出后才release；manager原case补STARTING拒绝init/getter及ACTIVE重复init断言，不新增计数。两项Python核对原46个完整URI/方法及auth指向、就绪顺序、AP/main接入；普通巨大HTTP模块/实际路由及main未在主机运行，不是端到端HTTP/真实FreeRTOS/RF时序验收。

针对性最终 **37/37**；首次普通812项通过，补外部角色不能撤销ready测试后813项通过。一次ASan发现文件解析错误，另一次清单有896登记：同目录两个构建重叠，使GTest生成HTTP清单写入损坏、manager83个case重复登记。它们不是新用例或有效唯一覆盖；未手改生成文件来伪造通过。重编真实测试来源、由构建器重新发现，检查manager清单83条、CTest库存总813，再跑完整唯一用例。最终普通-j8 **813/813、11.59秒**；ASan/UBSan-j4 **813/813、40.09秒**。Python构建 **44**、控制台 **17**，共 **61/61**；diff检查通过。本轮最终验证避免同目录同时构建/生成清单，不宣称防护所有外部进程并发构建。

首次固件因CMake新增源码触发重配，受限环境不能读取组件registry，exit1未计成功；既有运行请求保留、等待主机环境许可返回后，同一固定SDK六配置构建exit0，没有因观察等待重复启动同一主机请求。最终新板/原板相关源及头后再次六配置缓存Ninja、失败即停循环exit0；最终日志无新增warning/error，保留SDK既有Kconfig NOTE。固定IDF v6.1/fff9895c82d744c7237be8847347bdd1b07c6643，组件版本不变；重配规范化dependencies.lock后当前与HEAD一致，未手工改版本/替换SDK。固定厂家28cde682a4468a581c278761922724f57d976418工作树干净。

最终集成 **1,907,456字节**/SHA256 `69432b505036b782ce520981ba2871c3774b6c5c640e537aedeef846cb94ba58`；原板 **2,059,376字节**/`858513f015b41b5ef4663c6c83e2203d7abea59f35fdf7d2640a4ca01e4319f3`。四隔离profile身份保持：照片406,256/fc202285…、SD332,304/c876599f…、显示305,120/44f691bd…、工厂275,344/1541861d…。集成HTTP/main/AP/service及原板HTTP/main对象时间晚于相关最终源码；集成map实际.text保留HTTP claim/start/stop/release、accepts/ready、register_http_routes和set_ready_checked，不是只读取discarded列表。Read Pico集成入口仍不调用业务指针，产品/OTA/迁移门禁保持。

烧写/回读：**未执行**；真机验收：**未执行**。无串口/复位/设备网络或配置/AP/NVS/SD/测试卡/原片/显示/相机/提交推送操作，最后已知c876599f…SD诊断/STOPPED非实时核查。HTTP停止仍可能无界等待，已开始业务不回滚，普通页面/AP/断线/重启/真实后台和整机失败退出仍待另定范围及验收。下一安全开发是后台创建/生命周期与就绪发布、其余Web配置/文件/空间策略；不扩大测试卡许可。

同步TODO/README/架构/索引/CHANGELOG，新增HTTP说明、Wi-Fi说明交叉引用。忽略日志`.cache/p3-http-*`保留首次registry失败、ASan清单错误/重复登记、主机六配置、最终唯一CTest/库存、Python和产物编译证据；原有用户修改/私有资料保持，生成物不入库。

## 2026-10-08 · P3四类后台创建/发布门禁与合作退出（仅软件）

本轮继续软件开发，不继承测试卡更新许可为产品或网络运行。开工核对TODO、需求/架构/索引及现有main/power接口；fast-context仍缺Windsurf Key，不提取或配置凭据，改限定rg/源码核对，未把未读接口当事实。固定SDK task.h确认xTaskCreate的pdPASS契约、栈字节单位和vTaskDelete(NULL)/idle回收语义；本轮不改厂家驱动/波形/校准、VCOM或频率参数。

Read Pico新SDK包装`read_pico_background`只管理原四类任务的固定槽，不引入另一个产品/相册系统。原名称、4096/16384/8192/8192栈字节及优先级5保留；逐次检查创建，任务先等待，main所需角色齐备及HTTP ready发布成功后才放行。创建/发布控制拒绝重入，取消锁存，本次启动不复用槽位；创建失败使先前等待任务自行退出，不猜句柄、跨任务强删或自动重建。发布与取消竞争不能重新激活，HTTP就绪由外层检查式清理，不假称取消回滚外部ready。

power睡眠/轮播和button在安全循环边界检查门禁/HTTP，late Wi-Fi改100ms有限event等待、空事件组/当前连接检查；Read Pico业务函数return后由包装层自删除。在线启动入口/延迟OTA前复查门禁；电源及其后OTA/相册/配网初始化检查式返回，原板保留旧创建/自删除/等待分支。main包装启动函数，返回未活动则取消并最多2秒确认其他自有回调返回，确认后才关闭仍ready的FRAME HTTP；退出busy/超时不继续清理。联网恢复同样先取消/有限等待，只豁免确实正在执行的自有调用者，未确认不改变射频/关闭HTTP。

另修正Read Pico睡眠门禁返回的流程：四处条件提前睡眠不再继续轮播；CLEAR和timer/rotate唤醒函数返回不再落入普通冷启动。未启用PMU睡眠、未重做电源策略；wake配置窗口原有HTTP可用行为不是完整四任务启动，窗口/失败返回后只尝试上述检查式退出。已有唤醒工作不回滚，不宣称实机睡醒验收。

新增 **18项CTest**，真实包装C接合成FreeRTOS创建/排队/时间/自删除：非法参数、必需角色/重复、创建后等待、取消后不复用、首个/后续创建失败、ready失败、发布中取消、重入、创建/发布中busy取消、不强删/有界超时/显式再确认、当前执行者豁免且不漏其他任务、完成句柄复用及tick回绕。每个CTest独立进程，静态故障状态无测试reset。新增 **3项Python源码断言**核对main/power真实接入、顺序、回调return/旧板分支、恢复/启动返回先等待、睡眠拒绝不继续推进；巨大main/power业务未在主机执行，合成调度不是实RTOS并发/RF/HTTP时序证明。

主机：针对性 **18/18，0.16秒**；完整普通-j8 **831/831，8.92秒**；ASan/UBSan-j4 **831/831，37.85秒**，库存确认831且新组18、无上一轮重复登记问题。最后的sleep/boot-return修改只改main及源码断言，适配C/18项CTest源码未再变；Python最终 **47项，2.715秒**，控制台 **17项**，合计 **64/64**。diff检查通过，各目录不并发重复构建。

固件：首次受限SDK重配不能读取组件索引exit1，未计成功；同一固定SDK在获准主机环境重配后六配置exit0，不烧录。后续main修改后最终再串行六配置Ninja、失败即停循环exit0；最终日志无新增warning/error，固定SDK的Kconfig NOTE不当源码错误。IDF v6.1/`fff9895c82d744c7237be8847347bdd1b07c6643`不变，dependencies.lock与HEAD一致；厂家`28cde682a4468a581c278761922724f57d976418`工作树干净。未改版本或通过凭据变量绕过限制。

最终产物：集成 **1,909,264字节**/SHA256 `8de840e5a154178ff695cbcc638bdcc07cf471be141695bba3ff499f9eade651`；原板 **2,059,376字节**/`2b0479598cd60c400f0ca39bd4ac86b2d8021e9dd0f8132905fc66580808ce39`。四隔离profile身份保持：照片406,256/fc202285…、SD332,304/c876599f…、显示305,120/44f691bd…、工厂275,344/1541861d…。main/power/background对象晚于最终相关源码；集成map实际.text保留background preparing/running/cancel/add/activate/cancel_and_wait、photoframe_boot/photoframe_app_main、button/late和power入口，不是仅discarded项。集成启动仍只保留业务指针并记录INTEGRATION_ONLY，不调用产品，产品/OTA/迁移门禁保持。

烧写与回读：**未执行**。真机验收：**未执行**。没有串口、复位、设备/业务网络或AP、NVS/SD/测试卡/原片/显示/相机/提交推送操作。最后已知c876599f…SD诊断/STOPPED仅历史证据，不是实时核查。

合作等待确认只证明这四类其他回调返回，不证明idle已回收TCB/栈、所有timer/HTTP/SDK退出或整机零泄漏；取消不硬中断或回滚已开始的文件/NVS/HA/显示操作。HTTPstop仍可能无界等待；periodic timer/SNTP未纳入该门禁，mDNS/板级/SD及其他消费者仍未完整清理。下一安全软件审查是periodic_tasks重复init/timer失败所有权、SNTP并发与NVS时间戳错误，其余Web消费者/空间策略；真实恢复/断线/重启和完整产品仍需另定范围与人工验收。

同步TODO/README/架构/索引/CHANGELOG和HTTP交叉说明，新增后台说明。忽略的`.cache/p3-background-*`保留初始registry失败、主机/最终六配置、CTest唯一清单、普通/ASan及Python证据；生成物不入库，用户原有修改和私人材料保留。

## 2026-10-08 · P3周期timer、串行回调与完成时间戳（仅软件）

上一目标回合属于progress：四类后台代码/测试/六配置与文档已修改并验证。本轮从当前TODO及周期任务源码继续，不将历史测试卡授权扩大为网络或产品运行。核对需求/架构/索引、main/power/HTTP/utils/OTA调用方；fast-context仍缺Windsurf Key，未提取/设置凭据，改限定rg和实际源码。固定SDK esp_timer.h/c明确共享TASK派发、stop_blocking用户任务确认及delete异步回收；lwIP sntp.c明确esp_sntp_init/stop为void tcpip_callback入队，未将其当退出ACK或真机证据。

Read Pico保留PhotoFrame periodic接口/日同步业务，CMake只在新板改选`read_pico_periodic.c`；旧板继续原periodic_tasks.c。新板重复成功init不清注册表、不覆盖timer或再创建worker；逐步检查timer创建/每小时启动/受管worker创建，失败保留主错误并检查确切自有timer退出。成功空句柄或错误带句柄的SDK异常锁存不确定，不猜close；stop/delete错误保留句柄、不用init绕过，后续可显式stop确认。

共享timer回调仅原子接纳检查/合并due，无业务/NVS/网络/日志/等待。新增第五后台角色PERIODIC，8192字节/优先级5，普通启动把它加入必需角色；worker等HTTP发布后运行。批次忙时保留due再尝试，不直接阻塞共享timer；原18后台用例只把非法mask改为按COUNT计算，未减少覆盖或重置静态故障状态。

注册/批次/force/run_now/时间戳读写用零等待控制串行，回调中重入报错不改表或双跑；15字节NVS键/16槽校验，满表可更新已有任务、interval0禁用保持。只NOT_FOUND当缺失，IO/类型/无效记录不触发业务；clock有效性、回退/未来及安全差值检查，输出成功且接纳仍有效才发布。force为本次启动易失请求，不erase/commit；run_now一次控制强制并执行指定任务，不把别人的消费或无关批次当本请求成功。批次失败不继续下一个任务。

回调错误传播、不写完成时间；成功后检查有效时钟及NVS open/set/commit，记录未确认则本任务锁存，不由重复init/register/force重跑已发生外部操作。真实NVS失败可能已部分保存，不保证旧记录未变/掉电原子，取消不回滚已改系统时钟、RTC、NVS或网络。main在注册后再请求RTC缺失的强制同步，三处批次与power漂移检查结果；恢复/启动返回先关闭周期接纳、取消/有限等待受管回调，再确认timer退出，未确认不继续射频/HTTP清理。手动HTTP批次仍忙时stop保护保留资源。

Read Pico SNTP通知标记改atomic，执行循环复查接纳/连接，取消或断线不必等满10秒；结束会尝试SDK异步stop，但无ACK，不宣称服务已关闭，通知仍没有应用请求号。有效时钟和RTC更新失败不确认本次任务。手动Web时间同步检查run_now失败，503 JSON不发送成功时间；成功路径检查clock/localtime/格式化、JSON各分配及发送。配置消费方检查同步结果，但NTP setter旧void保存/可变RAM仍未完成检查式持久化/并发；日志只说明配置可能已变化、不假称回滚。

新增 **36项CTest**：真实周期适配C+真实后台包装C接合成timer/FreeRTOS/NVS/time，覆盖重复/失败所有权、SDK异常输出、唯一worker、timer只排队、取消/重入、停止/删除失败与确切重试、键名/满表、缺失/IO/类型/无效记录、输出不变、无效clock/强制bootstrap/回退/极值、易失force/单任务、回调及open/set/commit失败、故障不能重复init/注册/force绕过、回调/读取中取消与批次失败不继续。每CTest独立进程，SDK对象/调度为替身，无假称实FreeRTOS/RF/浏览器/NVS/掉电验收。

新增 **3项Python**源码断言核对CMake新板选择、timer无重业务、main/power/HTTP/utils接入、恢复/启动返回关闭顺序与原子通知；同时防止把request_stop误接在普通connect。开发中一次大patch临时误接该位置，源码检查时发现并移到恢复入口，未编译部署该错误版本，最终断言证明connect无该调用。巨大main/HTTP/真实SNTP回调未在主机运行，JSON/RTC/SDK服务仅源码与固件编译证据，不将这些断言当行为测试。

主机：最终针对性 **54/54、0.38秒**（36周期+18后台）；完整普通-j8 **867/867、10.53秒**；ASan/UBSan-j4 **867/867、38.78秒**。ASan库存总867，新组36，未重复登记；目录各自单构建，无上一轮重叠发现问题。最后修改只涉及HTTP/配置消费者的错误/响应检查和Python断言，适配C/Cpp已测来源未再改。Python最终构建 **50、2.550秒**、控制台 **17**，共 **67/67**；diff检查通过。

固件：首次受限CMake重配因组件索引不可达exit1，未计成功；保留同一主机授权请求并等待活句柄，不因观察等待重启请求。获准主机环境后同一固定SDK六配置、失败即停循环exit0；最终相关源/头在编译前完成，无新增warning/error。IDF v6.1/`fff9895c82d744c7237be8847347bdd1b07c6643`、厂家`28cde682a4468a581c278761922724f57d976418`不变且厂家工作树干净，dependencies.lock重配暂有manifest规范化差异，最终与HEAD一致，组件版本不改、未手动覆盖锁。

最终集成 **1,910,864字节**/SHA256 `090c43e2f7942edfa77281b2b1411c1c3bc132891883ab9bbdb7ca36cfde5504`；原板 **2,059,376字节**/`9542a6dcbfbde2a6cd1f319a98fd87103c999a79834e34a37a29c2189cec2282`。四隔离profile保持：照片406,256/fc202285…、SD332,304/c876599f…、显示305,120/44f691bd…、工厂275,344/1541861d…。main/power/utils/HTTP/periodic/background对象晚于最终相关源/头；map实际.text保留timer_callback/periodic_worker、is_active/request_stop/init/stop/register/force/run_now/check_and_run及SNTP/main/time_sync入口，未把discarded的未用查询API当实际业务链接。集成启动仍INTEGRATION_ONLY、不调用业务指针，产品/OTA/迁移门禁保持。

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/真实网络/AP、NVS/SD/测试卡/原片/显示/相机/提交推送操作，不改频率参数/固定驱动/波形/VCOM校准。最后已知设备c876599f…SD诊断/STOPPED仅此前证据，不是实时观察。

未闭环：SDK SNTP所有权、void入队失败/异步关闭确认、无请求号的延迟回复关联、NTP及其他配置setter保存/RAM并发、其他Web/文件消费者/长期空间策略和整机资源退出。合作回调确认/timer关闭不保证idle已回收全部内存，HTTPstop仍可能无界等待；完整产品/RF/恢复/重启/PMU睡醒需要另定范围与人工验收。下一步优先SNTP SDK服务与配置消费者的软件审查，保留完整开发目标，不将本适配通过标成P3/P5完成。

同步TODO/README/架构/索引/CHANGELOG，新增周期说明并标明后台说明四类/831为历史。忽略日志`.cache/p3-periodic-*`保留首次registry失败、获准六配置、最终Python、唯一库存和普通/ASan证据；生成物不入库，原有用户修改和私人材料保留。

## 2026-10-08 · P3检查式NTP保存、独立副本与SDK审查（仅软件）

上一回合为progress，本轮按TODO继续检查NTP保存/SDK退出，不将旧测试卡许可扩大到联网或产品运行。先读需求/架构/索引；fast-context缺Key，未提取或设置凭据，改限定rg与实际源码。固定SDK esp_netif_sntp.h/c及lwIP sntp.c只读核对：setservername借用指针，异步stop没有ACK；stop不取消DNS，晚到DNS回调忽略hostname/arg并使用全局当前服务器。netif deinit为void且不检查TCPIP停止返回，释放存储，不能直接当作完整退出保证。本轮未改SDK/厂家驱动或实际联网复现。

新板NTP检查式setter在非阻塞控制内校验/复制完整请求，检查NVS open/set/commit，关闭后仅成功才发布RAM。拒绝超长截断、空白/控制/非ASCII，空串仍保存为默认回退；不宣称完整域名/IP语法、可达性/信任验证。失败RAM保留，但NVS set/commit可能已经生效，测试刻意保留此模型而不假造回滚；后续显式保存可再次核对。void兼容入口在新板记录未确认，旧板原行为保留。

新板副本读取返回调用方拥有的值，容量/忙错误不改输出且无NVS IO；检查式读写及启动加载共用原子控制，加载失败不发布部分NTP值。此控制不覆盖其他配置/旧借用getter，init仍是启动专用，不是全配置在线reload。utils先确认本字段保存再触发同步，失败立即返回；已保存而同步失败明确配置已变化、不假称多字段回滚。GET先获取NTP副本，忙响应503、检查JSON对象/NTP字段分配；没有主机执行巨大HTTP/utils/main或证明所有字段分配/请求路径已安全。

新增 **17项CTest**（16新板、1旧板）：真实config_manager C与合成NVS，覆盖默认无IO副本、成功保存单commit/关闭、open/set/commit失败不发RAM、已部分持久化、输入在存储调用中被修改、保存/加载重入、真实双线程忙拒绝、独立副本/精确容量/输出不变、空串/最大长度/超长/null/文本字符、失败reload保留最后确认NTP、畸形持久值与void兼容；旧板检查原截断/RAM先写行为未改变。实际并发用例只验证这些检查式入口，不用未保护getter参加测试或宣称全局竞争已解决；ASan/UBSan不是TSan。

主机最终：17项针对性各重复20次，**17/17唯一、340次执行通过，3.37秒**；完整普通-j8 **884/884、10.43秒**；ASan/UBSan-j4 **884/884、39.18秒**。库存884无重复登记。新增2项Python源码断言，构建 **52/52、2.620秒**、控制台 **17/17**，共 **69/69**。两主机目录分别完整构建后运行，没有同目录重叠构建；日志没有新增编译warning/error。

固件最终：固定IDF v6.1六配置失败即停循环exit0；均为最终C/header源码后编译，无新增warning/error；dependencies.lock与HEAD一致、厂家子模块干净。无需新的主机网络授权（本轮增量构建成功）。集成map实际.text保留config_manager_init/set_ntp_server_checked/copy_ntp_server及config_handler，非仅discarded条目；集成启动仍INTEGRATION_ONLY，不调用业务入口。

| 配置 | 字节 | 最终SHA256 |
| --- | ---: | --- |
| 集成 | 1,911,520 | `b68e48b54f9bdf008b9bdf8c17bcaab42de0d5b196b03f245558c8998ce67d5e` |
| 原板 | 2,059,392 | `417a2d1ed94f65bf5ee38e15cb27ba54070c149253874e222414575f0bb62f4b` |
| 照片 | 406,256 | `ce07b6989408db79741ec261e12099158af3d98f9c707d879f74930e1b867e47` |
| SD | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| 工厂 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写与回读：**未执行**。真机验收：**未执行**。无串口/复位/设备网络/AP、真实NVS/SD/测试卡/照片/显示/相机/提交推送操作；不改频率、VCOM、波形或固定厂家源码。设备最后已知c876599f…SD诊断/STOPPED为历史，不是本轮实时核查。

剩余重要限制：main/SNTP仍用借用可变缓存，检查式保存不保证SDK读它时无竞争；晚到DNS/回复归属、服务所有权、退出确认均未修复。仅更换async stop或netif deinit不足以解决，需要继续审查应用拥有的解析/服务生命周期及失败保留；时区/其余setter、其他文件/Web消费者/空间策略/整机退出也待完成。产品/OTA/迁移门禁保持，不把这些软件检查记作P3/P5或完整网络验收。

同步TODO/README/架构/索引/CHANGELOG及周期说明；`.cache/p3-ntp-*`保存构建、唯一库存、重复/全量测试与Python记录。所有生成物忽略，不提交凭据/原片或覆盖既有用户修改。

## 2026-10-08 · P3自有DNS身份、回复关联和检查式SNTP服务（仅软件）

上一回合为progress，NTP真实保存/测试/六配置与文档完成。本轮按TODO检查固定SDK DNS/SNTP/core IPC、调用方、需求/架构/索引；fast-context仍缺Key，未提取或设置凭据，改限定rg/实际源码。SDK sntp_setservername借用pointer、stop不取消pending DNS、dns_found无请求身份；netif deinit为void不能作为退出确认。因此未仅替换成该包装或在异步stop后释放临时字符串。

新板沿用PhotoFrame日同步/手动请求/RTC/轮播业务，main先复制NTP，read_pico_sntp再持有完整副本。带不回绕ticket的外层DNS在core解析，使用原SDK默认地址类型，缓存/异步结果均检查；只匹配ticket/PENDING发布。SDK仅设置数字地址，不发内部DNS、不借用活RAM。超时/断线/取消保留pending静态槽，晚到结果不能启动同步，NOT_FINISHED不复用；退出还core栅栏确认已结束派发，不清全局DNS缓存/外来对象。

单服务零等待控制，request_stop永久关闭本启动接纳/时钟应用。拒绝外来已启用/已配置SNTP，不先stop或覆盖。SDK启动前真实分配器预约准确UDP PCB，null返回NO_MEM、尚未配置；当前同步init独占消耗，其他UDP/DNS透传。异常未消费只清自己PCB并锁存。SDK启动后enabled检查、结束先关闭clock接纳，再core停止自有服务、检查disabled后清引用；IPC返回+callback执行ACK均需确认。忙/未确认保留，故障不由重复sync/显式stop成功自动消除。

外层CMake仅Read Pico集成给固定lwIP启用SNTP_CHECK_RESPONSE=2/关闭RTT补偿；未改SDK/厂家源码。链接包装SDK取请求时间，本启动严格单调唯一、相同wall-clock或回退不复用；SDK原编码并核对来源地址/端口/Originate。synthetic timestamp只是回声关联，不作为clock/RTT校时；无效/耗尽锁存。SDK通过关联后自有强apply hook检查首次接纳、非阻塞周期gate、2020–2099/微秒和settimeofday返回；退出及连接/接纳复查后才输出confirmed time，main再检查RTC/轮播前门禁。已发生clock/RTC/NVS不可回滚。

main恢复/启动返回先请求SNTP和周期停机、等受管回调、检查周期timer和SNTP退出，然后才无线/HTTP；pending/busy/fault不继续。旧板原同步/通知路径保留，SDK编译没有新关联宏。强hook是新板唯一客户端契约：不接管/停止外来服务，但外来hook不会更新clock，不能当多客户端共享支持。

新增 **54项CTest**：5便携时间戳、40真实服务C接合成DNS/core/RTOS/clock、9真实服务C与**固定SDK原始lwIP sntp.c**接合成UDP/pbuf/timeout。覆盖副本/参数/默认地址类型、DNS缓存/异步/失败/错误地址/ticket、未完成不复用、晚到结果不启动、core nominal success却无ACK、启动/资源不足/自有关闭失败保留、外来不覆盖、timeout/断线/取消/late-hook/clock失败/无效时间/重复/票据时间戳耗尽与tick回绕。实际SDK报文用例直接运行原parser，验证正确/旧Originate(含clock回退)、错误来源/端口/长度/模式/KoD、重复/取消回复、OOM预约避实际null断言、PCB/timer退出，不是模型重写报文处理。

两主机目录显式READ_PICO_TEST_IDF_PATH指固定SDK；CMake校验原文件SHA256 `8695155c80c807d7ac949801b023d4e2f27db7975f78bcbba726bcdc012020e7`，不下载/复制SDK。没有此参数默认主机构建**不含9项SDK测试**，不是相同覆盖。替身startup延迟100ms/地址模型简化/内存与IO非真实，ASan不是TSan，也未主机执行巨大main/HTTP/RTC及完整SDK网络时序。

最终主机：普通-j8 **938/938、11.58秒**；ASan/UBSan-j4 **938/938、43.35秒**；库存938无重复。54项针对性每项重复20次，**1,080次执行/54唯一，8.39秒**。新增2项Python核对main服务/退出顺序、配置副本、root/main CMake/clock/包装/失败接入，保留原测试覆盖并调整此前旧通知断言；最终Python构建 **54/54、3.059秒**、控制台 **17/17、0.007秒**，总 **71**。主机两个目录各自构建后测试，无同目录重复构建。开发初次macOS timeval窄化错误及缺少SDK替身声明已修正，不能计首次失败为通过。

固件：首次受限重配组件索引失败exit1，未计成功；同一主机授权handle继续等待至确认SDK会话，未因观察timeout重启。主机授权固定SDK集成/六配置失败即停循环exit0。最后DNS完成栅栏及默认地址类型/RTC门禁发生在其中一次集成编译之后，按对象时间发现旧源，已等原循环完整退出，再逐配置增量复编；最终source 11:27:06/07，对象11:27:13，六配置exit0、无新增warning/error。

编译命令实含SNTP_CHECK_RESPONSE=2/COMP_ROUNDTRIP=0；SDK对象有last_timestamp_sent/last_server_address，最终ELF SDK send_request调用__wrap_sntp_get_system_time，init调用__wrap_udp_new_ip_type。实际.text的read_pico_sntp_sync/stop/request_stop、包装及强sntp_sync_time均来自main适配；SDK弱apply在discarded，不把discarded条目当接入。原板compile_commands无这两个宏。固定IDF `fff9895c82d744c7237be8847347bdd1b07c6643`、厂家`28cde682a4468a581c278761922724f57d976418`均干净；锁重配临时manifest hash变化，最终dependencies.lock与HEAD一致，版本不改、没有手动覆盖锁。集成入口仍INTEGRATION_ONLY，不调用产品。

| 配置 | 字节 | 最终SHA256 |
| --- | ---: | --- |
| 集成 | 1,913,968 | `126a5b7a700330e58ec4a5295a9f1b19678b3a3f20419e7814318197ed300c55` |
| 原板 | 2,059,392 | `aa04afaba467bf1bd1cb28fd7cadc2e31696463644c472ed9d6c2351a737ef0c` |
| 照片 | 406,256 | `ce07b6989408db79741ec261e12099158af3d98f9c707d879f74930e1b867e47` |
| SD | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| 工厂 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/真实RF/DNS/AP/NVS/SD/测试卡/照片/显示/相机/提交推送操作；不改频率、VCOM、波形或固定厂家源码。最后已知c876599f…/STOPPED只是历史，不是实时检查。

重要边界：10秒预算只约束应用等待，SDK IPC/连接检查/clock IO仍可能阻塞；回复关联不是NTS/可信时间或完整NTP规则，SDK级3/4注释中LI/版本/stratum/Root Dispersion尚未全部检查。门禁不回滚最后检查后已开始clock/RTC/NVS；不是双栈/RF/真实SDK调度或全局DNS/整个网络/整机资源退出验收。产品/OTA/迁移门禁保持，不计P3/P5完成。下一安全开发为时区/其他setter、JSON/远端结果传播、多字段配置、其他文件/空间策略和全栈生命周期；真实同步/恢复/PMU睡醒/触摸/完整产品需另定人工范围。

同步TODO/README/架构/索引/CHANGELOG/周期说明及DEVELOPMENT，新增SNTP说明。`.cache/p3-sntp-*`保留首次失败、获准构建、最终六配置、对象/链接检查、普通/ASan/重复/唯一清单与Python记录，生成物不入库、保留用户修改与私人材料。

## 2026-10-08 · P3时区保存、部分应用结果与独立时间响应（仅软件）

上一目标回合为progress，SNTP真实适配/固定SDK报文测试/编译和文档完成。本轮按TODO继续核对时区保存、环境安装、HTTP配置/时间/手动同步消费者；需求/架构/索引作为基线，fast-context仍缺Key、不提取凭据，改限定rg/实际源码。没有继承测试卡许可为产品/真实配置或网络操作。

新板config_manager_set_timezone检查NVS open/set/commit，候选先复制，关闭后才发布保存确认的RAM并应用。空串/超长/控制/非ASCII拒绝，不截断；保留上游仅形状检查和可打印空格，不引入新POSIX解析器。保存错误不改RAM/环境，NVS可能已部分生效、不假称持久值原样。commit已确认后setenv失败/读回不一致返回NOT_FINISHED，保留已保存配置和false应用标记，调用方明确部分完成；后续显式请求可再保存/应用，无自动写入或清理。

tzset是void，无语法结果；应用true只证明setenv/读回一致且tzset调用返回，不证明完整规则/DST被正确解释，非法语法仍可能库回退UTC。启动也检查文本/环境应用，错误拒绝；失败保留先前确认时区/NTP RAM，若应用已尝试失败保持false，未尝试则保留旧应用状态，不回滚全局环境或全部缓存。init仍启动专用，不是全配置并发reload/原子事务。

此前ntp_control扩成time_config_control，保护新板时区、NTP检查式读写、加载与副本。重入/忙无额外IO/输出；不覆盖其他setter、libc全局所有消费者或借用getter。配置GET新增timezone_applied，时区与应用标记复制后才加入JSON；NTP/时区各自副本不等于整配置原子快照。新板时间GET/手动同步成功响应在同一控制内检查有效time/localtime_r/strftime/容量，再一次发布时间戳、格式化文本和时区独立字符串。不把带库指针的tm留到解锁后格式化；错误保持全部输出。

HTTP忙/未应用/无效clock不发送成功时间，JSON对象/各时间字段/Print/类型及发送返回均检查；手动同步触发前检查HTTP就绪。已执行同步不会因响应失败回滚。utils仅新板依赖setter检查式应用，不二次setenv/tzset，NOT_FINISHED明确已保存但未应用，停止本次后续字段；此前字段可能已改，不承诺多字段事务。旧板原保存/RAM先写及调用方应用环境行为保持。

新增 **25项CTest**（24新板、1旧板）：真实config_manager C接合成NVS、setenv/时钟/格式化错误；正常环境/格式化使用主机libc并由fixture恢复本进程环境。覆盖默认副本/未应用、commit关闭先于应用、open/set/commit失败、部分NVS、应用失败/读回不一致及显式重做、输入在IO中变化、null/空/形状/最长/超长、容量/输出不变、NTP/时区/加载重入、环境应用持锁、实际双线程busy、无效time/localtime/strftime、失败加载保留缓存/应用状态、缺失默认不写、独立非UTC时间文本与旧板原契约。未改变本机系统clock；宏只作用真实config_manager测试源，不作用设备或主机其他进程。

开发中先实现tm输出，随后发现库指针不适合跨保护范围作为独立响应副本，改为保护内格式化字符串并补strftime失败测试；962项/24针对性是中间证据，不作为最终身份。本轮巨大HTTP/utils/main未在主机执行，新增2项Python为源码接入/失败路径断言，原手动时间响应检查随帮助函数重构移动、未删除原JSON/发送错误覆盖；非真实浏览器或ESP newlib/DST验收，ASan不是TSan。

最终主机：普通-j8 **963/963、11.57秒**；ASan/UBSan-j4 **963/963、40.52秒**；唯一库存963、无重复。25针对性各重复20次，**500次执行/25唯一、5.40秒**。Python构建 **56/56、3.510秒**、控制台 **17/17、0.010秒**，共 **73**。主机各目录单独构建后运行，无同目录重叠构建；diff检查通过。

固件：固定SDK六配置失败即停循环最终exit0，无新增warning/error。本轮增量重配/编译受限环境成功，无新的主机网络授权请求。最后C/header 11:48:02，集成config_manager/HTTP对象11:48:58，最终源后编译；map实际.text保留set_timezone/copy_timezone/copy_local_time/send_current_time_checked，非仅discarded。固定IDF `fff9895c82d744c7237be8847347bdd1b07c6643`、厂家`28cde682a4468a581c278761922724f57d976418`不变，厂家干净、dependencies.lock与HEAD一致，未手动覆盖锁或改版本。集成入口仍INTEGRATION_ONLY、不调用产品。

| 配置 | 字节 | 最终SHA256 |
| --- | ---: | --- |
| 集成 | 1,914,928 | `f797b0bc3287fb3f57994bbeaa743d0c533e721a790b46629f07008c00d40223` |
| 原板 | 2,059,392 | `70a99491dd61865388d6f688ed3994a6e03a3e0999cd087c609fe43fbccf543b` |
| 照片 | 406,256 | `a0de8972328d18bedbe6b023094df4208258682a450360fb626f2f8497f3a8c1` |
| SD | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| 工厂 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/真实TZ/NVS/RF/AP/SD/测试卡/原片/显示/相机/提交推送；不改频率、VCOM、波形或固定厂家源码。最后已知c876599f…/STOPPED是此前设备证据，不是本轮实时检查。

未闭环：全TZ/DST语义、其他RTC/cron/电源调度和libc消费者、全部配置/远端结果/多字段保存、文件/空间策略/全栈生命周期、真实网络/恢复/睡醒/触摸与产品验收。部分失败不回滚已变NVS/环境/clock；门禁保持，不计P3/P5完成。下一安全软件先处理其余setter/JSON与远端错误传播，再评估真实调度消费者；不把安全适配通过替代完整目标。

同步TODO/README/架构/索引/CHANGELOG及周期/SNTP交叉说明，新增时区说明；`.cache/p3-timezone-*`保留中间/最终主机、完整/重复/唯一清单、Python和六配置编译记录。生成物不入库，用户修改/私人资料保留。

## 2026-10-08 · P3远端步骤错误、真实保存与版本确认（仅软件）

上一目标回合为progress，时区实际代码/主机/六配置与文档完成。本轮按TODO及需求/架构/索引核对图片服务器响应头、远端三组配置、转换/保存/重载、版本及五个HTTP消费者。fast-context仍缺Key、不提取凭据，改限定rg/实际源码；研究/旧卡授权不作为新产品/联网权限。

新板remote_config便携协调器沿用三个组及general→processing→palette顺序。配置头容量2048含NUL，完整复制、不截断、不重复覆盖；seen每响应重置，异常锁存download owner，不能仅靠SDK事件返回值。解析前16层/NUL转义保护，真实cJSON严格单文档、object组、重复/大小写等价键及非有限数检查，全部结构通过后才回调。未知根metadata沿用忽略，空/未知-only不触发版本；没有把SDK C字符串接口当原始HTTP NUL后字节完整验证，也不是所有组内字段语义/枚举/range验证。

general应用/processing保存/palette保存及reload的已报告错误停止后续回调，记录失败阶段与已完成mask，不touch版本/继续显示/确认本次下载ETag。304保留payload并检查后才发布not_modified成功。完成衍生文件保留、不删原片；general此前可能已改变URL/清旧ETag或其他字段，不宣称整个缓存/配置没变。配置/版本在图像显示前确认，后续图像失败不回滚配置。

processing真实C新板逐项检查13个set，第一个失败不继续、不commit，有限float/字符串终止保护；defaults清零固定槽以保证旧转换器截断边界有NUL。palette save错误/重载传播，load仅NOT_FOUND允许默认，IO/类型/精确长度/非有限record拒绝、不改输出。没有更改默认数值、灰阶gamma、VCOM、频率或厂家源码；这些是保存/错误边界，不是实屏调参。

检查式revision在既有time_config_control内检查clock/int64溢出、NVS open/set/commit/close，只确认后发布RAM。逻辑秒token取max(now,previous+1)，同秒/clock回退不倒退；不是严格实时戳、跨客户端CAS或配置事务。直接void setter新板也检查、拒回退；下载版本头改受保护副本，失败不开始下载且有明确UI错误；加载失败保留最后确认revision。NVS set/commit失败可能已改存储，不假称回滚。五个HTTP配置/处理/调色板更新检查版本，未确认503部分成功而非成功JSON；palette先reload确认再版本。旧板原顺序/void/RAM先写保持。

重要未闭环：general内部仍有void setter/未检查字段，callback返回ESP_OK并不证明所有内部持久化/语义都成功。协调器成功仅证明已接入回调返回与版本确认；整体应用互斥、多字段原子性、重复请求副作用、其余JSON/发送/认证/文件与真实调度消费者仍待逐项处理。本轮不把这一增量称为完整可靠远端配置或P3/P5完成。

新增 **44项CTest**：20便携协调器+真实cJSON（回调/finish错误、先验结构、完整/尾随/重复/NUL/深度/有限值、全部解析分配点故障、header边界），11真实processing/palette C+合成NVS（13个set逐一失败、open/commit、float溢出/字符串、缺失/读取错误/精确blob/输出不变），13真实revision C（12新板/1旧板，clock/同秒/回退、open/set/commit部分结果、忙/跨时区/实际双线程、溢出/直接setter及加载失败）。协调器业务callbacks为替身，utils/HTTP/main未host执行；真实保存代码与parser分别执行，但不声称整链/真实NVS/掉电/网络/浏览器验收，ASan非TSan。

最终主机：完整普通-j8 **1007/1007、9.80秒**；ASan/UBSan-j4 **1007/1007、39.35秒**。唯一库存1007，适配四位CTest编号空白解析、无重复，新增20+11+13=44；针对性各重复20次，**880次执行/44唯一、6.43秒**。Python新增3项，最终构建 **59/59、2.445秒**、控制台 **17/17、0.008秒**，共 **76**。之后只改主机依赖获取/SHA断言及utils早期错误说明；native C来源/现有1007测试保持，重新配置构建成功、最终Python与固件复跑。

干净host依赖验证：原来managed_components忽略，host CI先于SDK解析，不能只依赖本机缓存。首次按普通cJSON v1.7.19标签拉取被双SHA校验拒绝；实际tag `c859b25da02955fef659d658b8f324b5cde87be3`缺SDK补丁（SetNumberHelper null及两处print深度保护）。当次随后shell仍执行构建/CTest得到No tests found，**不计通过**。根据SDK SBOM锁定上游`b2890c8d76bbb64e710585ebc0a917196b9c67e7`，同目录生成依赖改checkout后，以失败即停重跑；SOURCE_SUBDIR阻止执行供应方tests/install工程。

独立`/private/tmp/rc3-remote-clean.nF65pk`最终配置/构建成功，**31/31、0.17秒**（20协调+11真实settings），实际Git HEAD等于b289，C/H双SHA与SDK完全一致：`607e756460fa0de37d20a7a9181f2de29c97bfb7ce5a0e6c2f548243836cd852` / `25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee`。复用已在本机的GoogleTest源码，不计全部依赖离线/空机或远端CI运行；CMake<3.18获取兼容分支未跑。已读取MIT LICENSE/SBOM，源码目录无AGENTS、不改供应方文件，版本1.7.19~2不变。首次ASan编译真实cJSON的macOS sprintf弃用12条告警保留，不抹去或改第三方源码；无自有代码错误，最终增量构建没有新增告警。

固件：首次受限CMake重配组件索引失败exit1未计成功；同一主机授权handle等待到实际SDK session，再完整六配置失败即停exit0，没有因观察timeout另开同目录构建。后续utils早期revision错误说明发生在其中一次之后，已等循环完成再六配置增量确认；最终utils 12:27:06→对象12:28:36，config_manager 12:16:48→对象12:18:23，其他相关源在首次通过前完成。最终六配置无新增warning/error，固定IDF/厂家commit不变且厂家干净，dependencies.lock最终与HEAD一致，无手动覆盖/升级。

map实际.text保留remote_config_apply/capture、apply_remote_group/fetch_apply_remote_config、store_config_revision/touch_config_checked和touch_config_for_http，非仅discarded条目；集成入口仍INTEGRATION_ONLY，不调用产品。照片隔离产物也有新身份、未转移旧S1验收；其defaults诊断不读真实配置/自动联网，既有启动门禁断言保持。

| 配置 | 字节 | 最终SHA256 |
| --- | ---: | --- |
| 集成 | 1,916,720 | `e248c2c7f958ae4886469c7e0f576a407b424c75148f4dc56e5c5905f5c911f2` |
| 原板 | 2,059,456 | `76676b458e2351f12309f3c38b7c3613b89de5f3552c510b8934f5ef90e4322b` |
| 照片 | 406,176 | `2e0380f52529d756f4c8dfd7b71991c75b15974ae60e77a4b6fbac02ecaf661b` |
| SD | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| 工厂 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/设备业务网络/AP/真实NVS/SD/测试卡/原片/显示/相机/提交推送；仅公开构建依赖网络获取，不更改设备或私人数据。最后已知c876599f…/STOPPED仍是历史、非实时核查。

下一安全软件仍为general其余setter/全部JSON、多字段/整体应用并发及重试副作用、RTC/cron/电源/文件/空间策略与全栈退出；真实服务器/RF/恢复/睡醒/触摸/完整产品另定人工范围。产品/OTA/迁移门禁保持，完整开发目标继续。

同步TODO/README/架构/索引/CHANGELOG、第三方/开发说明，新增远端说明；`.cache/p3-remote-*`及独立生成目录保留首次失败、依赖/SHA、最终主机/唯一/重复/Python/六配置证据，生成物不入库，用户修改和私人资料保留。

## 2026-10-08 · P3名称/网络组检查保存与副本（仅软件）

继续当前开发目标，先读TODO/需求/架构/索引并核对真实配置、utils、Wi-Fi/AP、HTTP、mDNS、HA及main。fast-context缺Windsurf Key，不提取/设置凭据，限定rg补查；只读旧报告/历史许可不扩大产品或卡/联网权限。本轮沿用PhotoFrame业务和已有time_config_control，不改厂家/SDK源码。

新板名称保存拒空/超长/ASCII控制与DEL，输入先复制、检查open/set/commit、关闭后发布；void兼容入口失败不更新RAM。名称副本容量/忙输出不变。名称/网络组/配网保存/启动加载及既有time/revision共用非阻塞控制；失败加载保留最后确认名称/网络。不是其他缓存、Wi-Fi凭据void接口或整个配置应用的互斥。

网络JSON便携解析真实cJSON对象，known字段严格字符串/16字节容量、模式dhcp/static、大小写等价重复键检查；无字段不保存。便携patch在保护内当前副本上合并，校验完整静态组/可空DNS，再逐一检查四个字符串及模式set、单commit后一次发布整个RAM组。字段错误不先保存部分网络；NVS仍可能已发生部分效果，不声称回滚或多字段/掉电原子性，nvs_close是void。

固定SDK的esp_netif_str_to_ip4→inet_pton→lwip_inet_pton→ip4addr_aton实际代码会解释前导零为八进制；外层拒绝多位前导零及非四段十进制，避免验证/应用值歧义。DHCP保留未选中静态字符串，新选择非空字段检查IPv4；完整候选可修复旧缓存。仅地址形状，不保证掩码连续、网段/路由/单播或可达。

utils网络更新统一候选/检查返回，名称保存失败不触发mDNS/Wi-Fi更新；已保存而hostname更新返回错误则报告部分效果。Wi-Fi忙副本返回不改已有连接、hostname忙不调用SDK且释放无线控制；AP保留值先复制。HTTP config/system-info、HA和在线main取名称副本，GET网络组同次副本，新增字段分配检查。各copy独立取得控制，非整GET一致快照；utils/HTTP/HA/main没有host整链执行。

mDNS先复制再init/free，同一次注册名称来自同一副本，读取忙不释放服务。已读固定mDNS1.14.0真实mdns_responder.c：hostname/instance/TXT复制字符串，hostname等待动作，instance排队返回；这些返回不证明实际发现/广播或整栈退出。保持上游photoframe可选服务失败非致命，生命周期所有权/互斥未完成。

明确剩余：名称已保存而SDK失败后，同值请求仍可能被原比较逻辑跳过hostname更新，优先后续处理重试/部分状态；整个应用并发、其他general setter/JSON/凭据、RTC/cron/文件/空间及全栈退出继续。不宣称本轮完成完整可靠配置、P3/P5、真实HTTP/NVS/射频或整机运行。

新增**38项唯一CTest**：5便携patch、7真实cJSON解析、16真实config_manager C+合成NVS（逐5次set故障、open/commit部分存储、容量/输入改变、重入/实际双线程、配网共用控制及加载保留）、2真实Wi-Fi C+SDK替身、8真实mDNS C+SDK替身（copy忙无SDK、同一副本、失败顺序/可选契约）。不把替身SDK当RF或内部mDNS调度验收，ASan非TSan。

主机最终普通-j8 **1045/1045、9.42秒**；ASan/UBSan-j4 **1045/1045、41.03秒**。CTest库存1045唯一、无重复，新增38；针对性38项各20次，**760执行/38唯一、0.75秒**。Python新增2项：构建接入**61/61、2.440秒**，控制台**17/17、0.007秒**，合计**78**。首次Python受旧错误文本断言影响59项中1项失败，不计通过；更新文本并保留保护断言后最终复跑成功。

独立`/private/tmp/rc3-remote-clean.nF65pk`固定依赖目录重新配置、构建新增解析/mDNS目标成功，**35/35、0.20秒**（20原remote+7网络JSON+8mDNS）。复用前轮固定cJSON/GoogleTest获取源码，不计新空机下载或远端CI。主机最终与六固件本轮日志无新增自有编译告警/错误；此前第三方弃用告警历史不抹去。

固件首次受限CMake组件索引连接失败exit1，不计通过。主机授权仅访问构建依赖，六配置最终连续构建exit0；网络C源15:20:22、集成对象15:20:37，确认产物包括最后前导零修正。ELF/map实际保留wifi_network_json_patch、网络组/名称checked/copy与mDNS注册调用，非只编译未链接的替代验证。

| 配置 | 字节 | SHA256 |
| --- | ---: | --- |
| Read Pico集成（禁止运行） | 1,918,192 | `ebf3a86ad3871c6bddb079ad2d20ca0245ecdc0bd152778445944b3393e30fd5` |
| 原板 | 2,059,472 | `2032bccdb35d5337f7b4437a258ef2e9053289427fe160403bd3cfff259c1bb9` |
| 照片隔离 | 406,176 | `d2367bdc12b01e8e802020b5fcad95436d21c08c2b89a9891a7fd3b2ce9f0390` |
| SD隔离 | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示隔离 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| factory只读 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

固定SDK HEAD `fff9895c82d744c7237be8847347bdd1b07c6643`及厂家`28cde682a4468a581c278761922724f57d976418`均干净，依赖锁最终相对HEAD无差异、无升级。照片/原板SHA变化按实际构建记录，不由相同尺寸推断相同产物。

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/设备业务联网/AP/NVS/SD/测试卡/原片/刷新/相机/提交推送；仅公开构建依赖网络获取。最后已知c876599f…/STOPPED为历史，不是实时核查。产品/OTA/迁移门禁保持，完整目标继续。

同步TODO/README/架构/索引/CHANGELOG及开发/第三方说明，新增[名称与网络说明](READ-PICO-NETWORK-CONFIG.md)。`.cache/p3-network-*`最终命名日志与独立生成目录保留失败、重复/唯一库存、软件与六构建证据；生成物不入库，用户修改、原片和旧测试产物均保留。

## 2026-10-08 · P3名称同值重试与general串行（仅软件）

上一目标轮为progress（名称/网络组保存与38项测试/六配置完成）。本轮从当前TODO/需求/架构/索引核对实际名称比较、保存、两服务及HTTP/远端入口；fast-context缺Key，限定rg补查，不提取凭据。优先关闭上一轮已记录的“名称已存而hostname失败，同值重试被跳过”。

Read Pico真实config_manager新增检查式名称应用，缓存接受与本启动服务返回确认分离，提供SAVE/MDNS/WIFI/COMPLETE阶段。名称先复制，变化才NVS检查保存；保存失败不调用服务，保存成功但服务失败保留缓存和未确认状态。同值重试mDNS→Wi-Fi，不重复NVS写入；两个回调返回成功才确认，后续同值请求不重启两服务。name_accepted包括未变化的已加载/default缓存，不宣称有物理NVS记录。状态不持久化；成功加载/显式setter/配网使其失效，失败加载保留此前确认。

名称控制持有至回调返回，直接保存/配网/加载及检查式NTP/TZ等零等待返回忙，名称不被后续请求替换。回调只能用新named服务适配，不递归checked getter；两服务收到同一独立名称。Wi-Fi仍用既有无线控制，错误释放；mDNS新增原子注册/更新保护，重入不free活动调用、普通失败不锁死。并非mDNS对象所有权/退出ACK或全部SDK并发，SDK内部等待可能无界，持锁期间相关配置副本不可用，不跨任务强删或硬超时。

实际供应方契约核对：固定mDNS hostname/instance/TXT复制输入，instance入队不等实际广播；固定IDF esp_netif_set_hostname_api通过strdup保存hostname，分配失败还可能已经释放旧字符串并回退默认。外层返回错误/保留待重试，不声称SDK失败无效果或回滚；成功返回也不证明下一DHCP协商/路由器展示、可选发现服务或实机网络正确。

便携config_apply_run零等待串行general公共适配，重入/重叠不执行回调，所有普通成功/错误返回释放。utils本地/远端general都走真实wrapper，root非object拒绝；旧板仍原应用策略。HTTP INVALID_STATE返回503、提示可能已有部分值改变，不消费活动请求的共享diagnostic缓冲。不是完整请求级错误隔离：其他失败仍用旧共享缓冲，guard释放后的诊断/版本也可能与新请求交错。

剩余：远端三个组、独立processing/palette端点、revision确认及直接setter未共享完整应用控制；其余general void/NVS/JSON/凭据、RTC/调度/文件/空间及全栈退出继续。不宣称跨组事务、失败回滚、掉电原子性、SDK所有权/超时、真实HTTP或P3/P5完成。名称重试通过仅对应已接入callback返回与缓存保护。

新增**22项CTest**：12真实config_manager C+合成NVS/回调替身（保存失败、同值/加载值、mDNS/Wi-Fi失败重试、输入变化、重入/真实双线程、保存/配网/加载失效与失败加载保留）；4真实便携general控制（成功/错误释放、重入、invalid、真实双线程）；3真实mDNS C及3真实Wi-Fi C+SDK替身（显式副本不读busy配置、invalid无SDK、重入/错误释放重试）。utils/HTTP/main真实host整链未执行，只做接入检查和固件实际链接，ASan非TSan。

最终完整普通-j8 **1067/1067、10.67秒**，ASan/UBSan-j4 **1067/1067、42.50秒**；库存1067唯一、无重复。新增22项每项重复20次，**440执行/22唯一、1.32秒**。Python新增1项，最终构建检查 **62/62、2.443秒**，控制台 **17/17、0.007秒**，合计**79**。此前全量1067/11.65秒和ASan44.30秒是中间结果，不替代本段最终复跑。

独立固定依赖目录`/private/tmp/rc3-remote-clean.nF65pk`重新配置/构建config_apply/mDNS目标并运行 **15/15、0.09秒**（4general+11mDNS），沿用已核验cJSON/GoogleTest源码，不计新空机或真实SDK联网。最终主机、六配置日志无新增自有编译告警或错误，不删除第三方历史告警。

六配置两次连续构建exit0，主机授权仅固定依赖访问；最终源码http_server/utils15:41:17，集成对象分别15:43:49/15:43:48，config_manager15:36:02→15:43:48。ELF/map实际保留config_apply_run、config_manager_apply_device_name_checked及两个named适配；确认最终错误提示已编入。固定SDK/厂家目录干净，依赖锁最终相对HEAD无差异，无波形/VCOM/gamma/频率或第三方变更。

| 配置 | 字节 | SHA256 |
| --- | ---: | --- |
| Read Pico集成（禁止运行） | 1,919,024 | `aa76894306d47ffc23b9a5c730b4e456e5120b83be605f2afa7c9a03aa3f2cd3` |
| 原板 | 2,059,520 | `f55c0c0f4ce59678685909843e6e7ec84afcf088f38ba5651cf95eb1bcf06ac5` |
| 照片隔离 | 406,176 | `7b64f4f73c7c302512967934fecad18f4c4e1420b7a24c612f85aacafdf3062e` |
| SD隔离 | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示隔离 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| factory只读 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。未操作串口/复位/设备业务网络/AP/NVS/SD/测试卡/原片/显示/相机或提交推送，仅公开软件构建依赖获取。最后已知c876599f…/STOPPED仍为历史、不是实时核查。完整产品/OTA/迁移门禁保持；下一安全软件为全组应用/错误上下文及其余保存与消费者，完整开发目标继续。

同步TODO/README/架构/索引/CHANGELOG/开发/名称网络说明；`.cache/p3-name-retry-*`与独立生成目录保留库存、重复/最终主机/Python/六配置证据。无新人工权限请求，用户修改、旧产物与原片保留，生成物不入库。

后续只读审查曾怀疑固定netif IPC忽略发送返回、消息ret默认零可能导致成功误报，但继续追查条件编译后**不作为当前缺陷**：用真实集成compile_commands将esp_netif_lwip.c仅改为-E/-dM执行，实际`LWIP_MPU_COMPATIBLE=0`、`LWIP_TCPIP_CORE_LOCKING=0`、`API_VAR_ALLOC`为空。固定tcpip_priv.h该分支为栈消息，tcpip_send_msg_wait_sem等待后返回ERR_OK，没有所怀疑的消息池分配失败分支；ERR_MEM仅在未启用MPU分支。`.cache/p3-name-retry-netif-macros.log`保留预处理证据，无SDK/配置变更或新增测试计数。仍不把回调返回当实机广播/下一DHCP协商验收；下一软件轮保持全组应用/请求级诊断优先，未为非当前分支扩张适配。

## 2026-10-08 · P3全组应用控制与请求诊断（仅软件）

按TODO/规格/架构/索引继续上轮名称重试后的全组并发缺口；fast-context缺Key后限定rg补查，不提取凭据。沿用PhotoFrame现有三组、处理/调色板业务及46条HTTP URI方法，不新建产品状态系统。

便携config_apply_transaction在同一应用控制中执行apply及finish，apply错误跳过finish，结果区分未进入繁忙/已进入失败/应用回调成功但版本失败。普通返回释放，不进行回滚、排队或自动重试。utils本地general走该协调，远端一次解析/general/processing/palette/revision整体持有config_apply_run，general走私有值回调避免递归busy。HTTP处理/调色板POST/DELETE从接收到保存/重载/版本/响应都持有同一控制；busy只响应503，已接纳处理器错误不追加第二响应，GET/不支持的方法仍原处理。

本地general仅获得控制后清理旧config/cert诊断，业务返回后在释放前复制到调用方结果；HTTP不再在guard释放后consume共享缓冲。远端不留下其旧诊断供下一本地请求使用。新增general错误JSON分配/打印及响应返回检查；成功用固定JSON，版本失败仍503部分效果说明。旧板保留原值应用、版本和诊断策略。其他general内部void setter错误仍可能未传播，不能据此宣称已完整应用。

范围限制：不覆盖所有直接setter/启动加载/配网的应用级控制、所有SDK或palette图像处理读者，不是整个GET一致快照、多组掉电原子性、CAS或失败撤回。checked配置层仍另行保护其缓存。处理/调色板完整接收、GET读取错误默认化、其他JSON/响应、凭据/字段语义、RTC/调度/文件/空间/整机退出继续。持有控制穿过接收/保存/SDK调用不提供新时限，SDK可能等待无界。当前utils诊断写入调用位于值适配内，但公共set/consume函数不是任意外部调用的自动同步API。

新增**7项唯一CTest**：5真实便携apply/finish控制（两阶段拒重入、失败跳过版本、版本失败释放/重试、invalid输出不改、实际双线程busy无回调且状态可区分）；2固定真实cJSON+远端协调+同一真实应用控制（三组及revision均拒交错、各组/版本错误后释放重试）。HTTP/utils真实host整链未执行，1新增Python仅检查接入/私有回调/结果副本及错误返回，不计行为验收；ASan非TSan。

最终主机普通-j8 **1074/1074、9.84秒**，ASan/UBSan-j4 **1074/1074、42.87秒**。库存1074唯一无重复，新增7；9个配置控制+2个远端协调各重复20次，**220执行/11唯一、0.40秒**（其中4旧控制测试，不算新增）。Python构建接入 **63/63、2.889秒**，控制台 **17/17、0.007秒**，合计**80**。较早11.93/45.00秒为中间结果，最终上述复跑已包含最后HTTP响应检查接入。

独立固定依赖目录`/private/tmp/rc3-remote-clean.nF65pk`重新配置/构建两个目标，**38/38、0.09秒**（9应用控制+22remote+7网络JSON）。复用已校验cJSON/GoogleTest获取源码，不算全新空机/远端CI。主机和六种最终固件日志无新增自有告警或错误。

六配置最终连续构建exit0，仅公开构建依赖/软件编译授权。最终HTTP源码16:09:49，集成对象16:11:47、集成产物16:11:54，确认最后诊断JSON/响应变更已编入。ELF/map实际保留config_apply_transaction、apply_local_config_checked和apply_remote_request，HTTP相应wrapper可能内联，不把缺独立符号误判为未接入。SDK固定fff9895c…/厂家28cde682…均干净，依赖锁相对HEAD无差异，无驱动/波形/VCOM/gamma/频率或第三方变更。

| 配置 | 字节 | SHA256 |
| --- | ---: | --- |
| Read Pico集成（禁止运行） | 1,919,440 | `085b6fce6b960ecd26be6c08c54c7dd728e6816c847c8f4ab06f2da13f7499f4` |
| 原板 | 2,059,552 | `065079380a2902792b6875e3e4eed44e4b510e49abfb9d28fcd357cf9956dce4` |
| 照片隔离 | 406,176 | `7b64f4f73c7c302512967934fecad18f4c4e1420b7a24c612f85aacafdf3062e` |
| SD隔离 | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示隔离 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| factory只读 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/业务网络/AP/NVS/SD/测试卡/原片/屏幕/相机/提交推送；旧c876599f…/STOPPED只为历史，不是实时核查。产品/OTA/迁移运行门禁保持，P3/P5不计完成。同步TODO/README/架构/开发/索引/CHANGELOG/配置说明；`.cache/p3-config-control-*`保留软件库存、重复、独立及最终六构建证据，生成物不入库。下一安全软件继续general其余保存/直接入口与读者、处理/调色板HTTP输入/错误及剩余消费者，完整目标继续，无新人工权限请求。

## 2026-10-08 · P3完整配置文档与两组读取（仅软件）

上一轮完成参与的应用级控制及请求错误副本，属于progress。本轮按当前TODO/规格/架构/索引检查，fast-context缺Key后限定rg定位真实HTTP/处理/调色板/NVS/调用者，不提取凭据。未把旧研究报告或硬件更新许可当本轮操作指令。

新增便携config_document：三个本地更新入口先检查正文1–4095字节再分配，循环完整收声明长度、累计第8次超时结束；EOF/错误/零或超过请求容量返回拒绝，不解析有效前缀。使用固定cJSON解析单一object、深度≤16、拒原始/转义NUL、大小写等价重复/非有限数字/尾随文档。成功才交出tree；OOM沿用无效文档结果，不宣称完整Unicode/RFC校验。远端原安全检查迁至共享模块，保留2048容量/空输入错误/三组阶段和未知metadata/空组契约，现有真实解析回归保留。

processing/palette从默认候选checked转换，选中字段全部通过才输出；原HTTP/远端省略字段默认策略不改成旧记录PATCH。七个processing数值及三个校准数值检查number/有限/float可表示；五个processing文本不截断/实际支持枚举、compress只bool；六颜色组object、RGB各0–255整数不绕回。不是曝光全部范围、校准端点/顺序/gamma物理范围验收或自动调优。原板旧接收/转换策略保留。

真实新板processing load只对namespace/key NOT_FOUND保留默认候选；open/13个get的错误停止、关闭、输出不变，拒非有限float、字符串长度/结尾/嵌入NUL及bool>1。全部成功才输出完整候选。两组HTTP GET也参与共享应用控制，不与参与的更新交错；load失败端点不再fallback。serializer每次cJSON添加/打印及终止/有限性检查，不返回部分JSON；响应type/send、保存/重载及500发送错误传播，不重复响应。新板palette默认全结构清零，只确定存储填充字节，black_y=.009、white_y=.65、gamma=1.42及所有原默认值不变。

仍未完成：utils图片请求头及processing便捷dither/scale/background getter仍可能把读取错误当默认；其余general void保存/凭据/字段语义、直接setter/图像读者共用控制、RTC/调度/文件/空间/全栈退出。不是所有NVS/全GET快照或多键掉电原子性/回滚，已有保存不能撤回。没有新SDK硬超时/绝对请求时间上限，接收调用等待沿用SDK契约；ASan非TSan。详见[配置文档说明](READ-PICO-CONFIG-DOCUMENT.md)。

新增**31项唯一CTest**：14便携接收/真实固定cJSON（全分段尺寸、4095边界/溢出、有效前缀EOF、IO/非法返回、累计超时、原始/转义NUL、16/17深度、重复/尾随/各解析分配点/无输出）；17真实processing/palette C+合成NVS/固定cJSON（缺失默认、完整独立输出、每个get IO/类型/长度错误、七float/五字符串/bool、字段错误与不发布部分候选、非法RGB/float溢出、两serializer全部分配点、默认blob字节稳定）。HTTP/utils整链未在host执行；2新Python仅源码接入检查，不算SDK HTTP/NVS/浏览器行为验收。

首轮主机构建失败于测试变量gets与macOS libc函数同名，Python失败于新增方法间移动时的host作用域引用；保留首轮日志，不计通过。修正为read_calls并恢复原依赖SHA断言后两主机构建、Python及针对性普通/ASan **63/63、0.50/1.06秒**通过；当时尚未加入默认blob第31项，属中间结果。

最终完整普通-j8 **1105/1105、14.08秒**，ASan/UBSan-j4 **1105/1105、48.47秒**；库存1105唯一无重复、新增31。14文档+28设置共42唯一各20次，**840执行/42唯一、3.54秒**，含11旧设置测试不算新增。Python构建接入 **65/65、4.141秒**，控制台 **17/17、0.006秒**，合计**82**。独立固定依赖目录`/private/tmp/rc3-remote-clean.nF65pk`set-e重新配置/构建两个目标，**71/71、0.11秒**（22remote+7网络JSON+14文档+28设置），复用核验过的cJSON/GoogleTest源码，不算新空机/远端CI。

六配置最终连续构建exit0，授权仅固定依赖/软件编译；最终processing/palette源码16:35:57，对象16:38:56、集成产物16:39:03，确认填充清零/容量防护已纳入；HTTP16:23:24/共享文档16:19:04更早已编入。ELF/map实际保留config_document_receive/parse及两checked converter。最终主机/独立/六配置日志无新增自有告警或错误。SDK固定fff9895c…/厂家28cde682…目录干净，依赖锁相对HEAD无差异，无第三方/波形/VCOM/gamma/频率变更。

| 配置 | 字节 | SHA256 |
| --- | ---: | --- |
| Read Pico集成（禁止运行） | 1,920,896 | `e296869c34ac701b02221a6770f903dd237283bad6f2eec834f70a8858af7b7f` |
| 原板 | 2,059,552 | `970c57b57a2df706ffcf874651f001fe747450b74f9857969678e1b4ebfb5da2` |
| 照片隔离 | 406,336 | `def2ebcd0bbb013e8d7896672994594e1bfa1f5469d3ee25e1cd5874ddd4ae14` |
| SD隔离 | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示隔离 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| factory只读 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/业务网络/AP/真实NVS/SD/测试卡/原片/屏幕/相机/提交推送；旧c876599f…/STOPPED仅历史、不是实时核查。产品/OTA/迁移门禁保持，不计P3/P5完成。同步TODO/README/架构/开发/索引/CHANGELOG/配置说明，`.cache/p3-config-document-*`保留首轮失败/库存/重复/独立/最终测试与六构建证据，生成物不入库。下一安全软件继续便捷消费者的读取错误传播及其余配置/全栈工作；完整目标继续，无新人工权限请求。

最终只读契约复核：本机webapp的固定`@aitjcize/epaper-image-convert/src/presets.js`的DITHER_ALGORITHMS只列floyd-steinberg/stucki/burkes/sierra，ProcessingControls.vue的tone/color/scale/background选项与新增checked枚举一致；不因未读第三方接口猜测而扩大枚举。未编辑包或新增测试计数。

## 2026-10-08 · P3图像选项读取与失败前置（仅软件）

上一轮完整配置文档属于progress；本轮按TODO/规格/架构/索引继续。fast-context缺Key，限定rg核对实际settings、HTTP/utils显示消费者、image_processor三geometry调用及主机/隔离诊断桥；不提取凭据，不沿用历史操作授权。未完成的目标继续，不计P3/P5完成。

新板`processing_settings_copy_options`在config_apply零等待共享控制内一次完整load，校验实际支持dither/scale/background，全部成功后发布独立副本；NOT_FOUND保留原默认、其他IO/类型/长度/有限性/未知或空历史枚举/忙失败不改变输出。普通返回释放控制，可重试；仅参与入口互斥、非所有直接NVS writer或全系统事务。旧便捷API仍保留原板/兼容契约。

上传和下载后显示在资产发布/相册派生之前检查副本；上传503逐项检查status/type/send，保留主要或发送错误；下载保留原失败/文件保持/不追认ETag策略。已下载文件、远端配置已有改变不回滚；这不是在发送HTTP下载前的检查，图片请求头load默认化/serializer/SDK header返回及revision一致快照仍未解决。

PNG内存显示/处理到文件在打开显示流或创建输出之前复制布局，流式/缓冲geometry沿用传入副本，不分别重读scale/background。JPEG EXIF/隔离照片固定Cover不读产品布局；RAM-only诊断桥提供明确固定副本，不链接产品NVS/应用控制。原板布局行为保持。调用方显式dither不覆盖；HTTP/utils dither选择与PNG布局读取是两次独立snapshot，palette/方向/其他配置尚未成为整次图像快照，不宣称全图并发安全。无波形、VCOM、gamma或频率调整。

新增**13项唯一CTest**：8真实settings C+合成NVS（缺失默认、4算法×2布局×2背景、逐13get/open失败不发布、非法历史枚举、忙/重入保护/释放及自有副本）；5真实Read Pico image_processor+合成settings/display（PNG错误/忙/非法拒绝与重试、输出前拒绝及原文件保持、单次fit背景副本、此前捕获画面保持、JPEG固定Cover无读取）。共享控制本轮用同步重入探针，不另称新的双线程/TSan证据。实际HTTP/utils整链未在host执行；新增Python仅源码接入检查，不算真实NVS/SDK HTTP/屏幕验收。

首轮11项针对性测试有1项失败：检查当时在writer创建输出后才执行，失败文件按原保留策略仍存在。保留首轮普通/ASan日志，非通过；未通过删除文件绕过，改为入口前置读取后复测，另加fit/旧画面两项。实际固定SDK无HTTPD_503_SERVICE_UNAVAILABLE错误枚举，构建前核对后使用项目既有503状态字符串，检查全部响应返回。

最终针对性普通/ASan **13/13，1.99/4.52秒**；完整普通-j8 **1118/1118、13.25秒**，ASan/UBSan-j4 **1118/1118、48.59秒**，库存1118唯一无重复，较前一轮新增13。13新增各20次，共**260执行/13唯一、38.12秒**。Python构建接入**66/66、2.511秒**，控制台**17/17、0.007秒**，合计**83**。独立目录`/private/tmp/rc3-remote-clean.nF65pk`重新配置/构建真实settings与remote目标（包含新config_apply链接），选定已构建四组**79/79、0.70秒**；复用固定cJSON/GoogleTest源码，不算新空机或远端CI。

六配置连续构建exit0，自有源码最终image_processor16:58:44、processing/utils/桥16:55:06、HTTP16:57:39，对象17:03:24、集成产物17:03:31，最后前置修复已纳入。集成map保留processing_settings_copy_options/config_apply_run；SDK/厂家固定且干净、dependencies.lock相对HEAD无差异。最终主机/六配置日志无新增自有warning/error。日志前缀`.cache/p3-processing-options-*`保留首轮失败、最终测试/库存/重复/独立/六构建，不提交生成物。

| 配置 | 字节 | SHA256 |
| --- | ---: | --- |
| Read Pico集成（禁止运行） | 1,921,424 | `13b43df24f0b500b98bdc3baf556276cdf998d55c1001fb9e3d9eddf90a0c058` |
| 原板 | 2,059,568 | `bd887392b223e1b067e82cb70bd518be58c1afb06eb12b894a4b28ed9eebc8a5` |
| 照片隔离 | 406,352 | `84ae3b2f46c666575e38f99bf34f99e61bccfa01c34544a367494389b273e978` |
| SD隔离 | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示隔离 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| factory只读 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/业务网络/AP/真实NVS/SD/测试卡/原片/屏幕/相机/提交推送；产品/OTA/迁移运行门禁保持。历史已烧写SD诊断c876599f…/STOPPED不是本轮实时核查。同步TODO/README/架构/开发/索引/CHANGELOG/配置说明；下一安全增量为图片请求头读取/序列化/设置返回和一致快照，再继续其余配置/整次图像/全栈工作，无新人工权限请求。

## 2026-10-08 · P3下载的三个配置头副本与确认（仅软件）

上一增量图像选项属于progress，本轮按TODO/规格/架构/索引继续。fast-context缺Key后限定rg核对utils下载整个重试/清理、真实settings/palette serializer、应用控制、固定SDK header/helper/client cleanup，未提取凭据。不把历史设备授权当本轮新操作指令。

新板request_config在共享应用控制内复制revision、load processing/palette、生成两个完整自有JSON；全部成功才发布，失败输出不变并释放已生成字符串，仅缺失记录沿用原默认。下载在响应缓冲/临时文件/HTTP client创建前一次capture，重试复用相同三头；分配失败、304及循环结束释放所有snapshot。参与的三组/版本应用不与捕获交错；不是所有直接setter/NVS writer、完整GET/URL/凭据/方向或整次图像快照。

三头逐remove、get确认不存在、set、get精确确认。SDK原已有item更新分支strdup失败仍ESP_OK并存NULL；外层删除/确认缺失避免该分支，新的item分配返回NO_MEM。未知失败/读回缺失或不一致不perform；清理client/终止自有owner，不按传输失败重试，不追认本次ETag。失败可能已有部分header，仅丢弃client、不假装回滚；新板下载最终保留主要错误返回，原板旧读取默认化/重试保持。

固定SDKcleanup对非NULL client内部忽略close/transport返回、释放后ESP_OK；外层检查返回不等于完整退出ACK。本新header失败分支在perform前，没有新网络连接，但不把它的测试当整栈退出证明。Authorization/custom、分辨率/方向、firmware/ETag/battery、URL/证书/借用凭据及redirect仍待处理；historical字段JSON上报不因本轮成为完整枚举/物理语义验收。原厂数值、波形、VCOM、gamma/频率不改，详见[配置说明](READ-PICO-CONFIG-DOCUMENT.md)。

新增**17项唯一CTest**：8真实processing/palette C+合成NVS/固定cJSON快照（完整字段/版本、版本读取失败先停止、逐13get失败不输出、palette错误不分配、两模式每个JSON分配点释放、忙/重入保护、独立副本）；5便携header协调（已有/缺失替换、逐12操作错误停止、假成功/未删除、非法参数、NULL/缺失读回）；4未改固定SDK http_header.c/http_utils.c及其真实头文件组件（真实复现已有更新分配失败假成功/NULL、已有或缺失记录的9新item分配点、SDK自有副本生命周期、exact compact JSON wire文本）。SDK allocator和esp_check/log适配为合成，没有执行socket、整个esp_http_client/服务器、真实revision配置管理或NVS。共享控制新增同步重入探针，不称新增TSan/全并发证明。1新Python仅接入顺序/释放/SDK SHA门禁源码检查。

四固定SDK源/头SHA256锁在host CMake，READ_PICO_TEST_IDF_PATH启用时直接编译原checkout，不复制或修改第三方：header.c `23056c41505aa38885cf210e22117e1fa57d964080ff537a36a5c9d99d76701e`、utils.c `50bfee8e6ef9d0b74e590f58975c9d35ce32012579994012a6d0717743c6a84b`、header.h `77f7e4faf9f2fe2d3afee3d100a3d642cdd3354d7556147d430ed0fe5e2c8f69`、utils.h `7e4110110767661e60a01d639565f2926ffe30f9d449558a3a5008de5dfeecc2`。SDK宏确实在新item错误设ret，trim对NULL返回；本轮不把新item路径也误报为假成功。

测试准备阶段一次patch因上下文不匹配未应用；随后fake header NULL探针误放remove导致编译失败，未set-e的初轮命令继续误跑旧二进制，旧JSON字段断言发生SEGFAULT；保留初轮日志，不算当前源码证据。修正探针位置、实际camelCase字段/float精度断言后使用set-e明确重建再测试。加入固定SDK后针对性普通/ASan **17/17、0.14/0.83秒**（按原日志校正）；最初只有13非SDK项的0.12/0.59秒属中间结果。

最终普通-j8 **1135/1135、9.05秒**；ASan/UBSan-j4 **1135/1135、42.53秒**，库存1135唯一无重复，较前一轮新增17。17新增各20次共**340执行/17唯一、2.25秒**。Python接入 **67/67、2.325秒**，控制台 **17/17、0.007秒**，合计**84**。独立目录`/private/tmp/rc3-remote-clean.nF65pk`set-e重新配置固定SDK路径并构建remote/settings，四组已构建目标 **96/96、0.63秒**（22remote+7网络JSON+14文档+53settings含17新）；复用固定cJSON/GoogleTest，不算空机或远端CI。重编固定cJSON有macOS sprintf废弃告警，非自有新增错误；未修改第三方消告警。

六配置连续构建exit0，utils/request_config源码17:13:25、集成对象17:24:13/17:24:12、产物17:24:20，确认当前来源已编入。集成map实际保留capture/set_headers，SDK固定fff9895c…/厂家28cde682…目录干净，dependencies.lock相对HEAD无差异。最终六配置无新增自有warning/error。`.cache/p3-request-config-*`保存初轮失败/最终主机/SDK/独立/库存/重复/Python/六编译证据，生成物不入库。

| 配置 | 字节 | SHA256 |
| --- | ---: | --- |
| Read Pico集成（禁止运行） | 1,921,952 | `5a624b949615f420aa781b31119c4a869c15decfd4ea4da89466e2cf7019d7a6` |
| 原板 | 2,059,568 | `9dfc43350181f2b608ef5e98ed9da29c245b370c2e4ac9e80053eb5d107354d4` |
| 照片隔离 | 406,352 | `84ae3b2f46c666575e38f99bf34f99e61bccfa01c34544a367494389b273e978` |
| SD隔离 | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示隔离 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| factory只读 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/业务网络/AP/真实NVS/SD/测试卡/原片/屏幕/相机/参数调整/提交推送；产品/OTA/迁移运行门禁保持。历史SD诊断c876599f…/STOPPED不是实时核查。同步TODO/README/架构/开发/索引/CHANGELOG/配置说明；下一安全开发其余headers/凭据/URL/证书生命周期与redirect，再继续general字段/全图/RTC/调度/文件/空间/全栈退出，完整目标继续，无新人工权限请求，不计P3/P5完成。

## 2026-10-08 · P3主图/缩略图初始请求头检查（仅软件）

上一轮三个配置头属于progress；本轮按TODO/规格/架构/索引和真实header/utils源码继续，fast-context缺Key后限定rg，不提取凭据。修正前一轮专项ASan耗时为原日志0.83秒，不改变17/17或完整1135结果。研究报告/历史设备授权不作本轮操作指令。

将三配置header协调复用成request_header_put_checked：key非空ASCII token，value拒CR/LF/DEL及除HTAB外控制字符，先删除/确认缺失、set、精确readback；已有SDK分配假成功仍由外层保护。SDK首尾trim导致不一致即失败，不自行“修复”凭据/ETag/custom值；内部HTAB、empty value及合法opaque ETag保持。函数调用者仍须提供有效NUL终止文本，不代表所有getter长度/借用并发有保证。

Read Pico主图Authorization/custom、display width/height/orientation、firmware、If-None-Match、battery全用检查，首个header错误保留，后续设置及三配置头跳过；在perform前清理client/终止owner并退出retry，不发不完整请求。缩略图同源认证/custom同保护，失败不perform，终止本次owner返回false，沿用可选预览/已成功主图的旧契约，不把它当全请求失败。原板旧set策略/同源/认证优先级保持。新板不再记录未经校验的custom key，错误日志只有错误名，无新增token/值输出。默认数值/厂家SDK/波形/VCOM/gamma/频率不改。

新增**7项唯一CTest**：5便携真实协调+合成header（全部非法key字节/控制值、token/opaque/empty/interior HTAB、无效参数/callback、每个操作失败停止），2真实未改且SHA锁定SDK header/helper（首尾空白被裁剪后拒绝、内部HTAB/empty exact；认证值自有复制及分配失败）。保留上一轮逐新header分配/假成功回归。新增1项Python只是主图/缩略图接入与停止顺序源码检查；utils整个下载/SDK client/network/真实NVS未在host执行，不以编译证明实际服务器或凭据生命周期。尚未处理borrowed凭据/URL/CA快照、snprintf截断、直接setter/保存/redirect/SDK内部Host/User-Agent/后续认证，全请求/图像/全栈退出仍待审查。

首次set-e主机构建与专项普通/ASan **12/12、0.12/0.60秒**通过，包含5旧RequestHeaders不算新增。最终普通-j8 **1142/1142、9.12秒**，ASan/UBSan-j4 **1142/1142、43.16秒**；库存1142唯一无重复、较前一轮新增7。7新增各20次共**140执行/7唯一、0.91秒**。Python接入 **68/68、2.381秒**，控制台 **17/17、0.007秒**，合计**85**。独立固定SDK目录`/private/tmp/rc3-remote-clean.nF65pk`set-e重建remote/settings四组选定目标 **103/103、0.72秒**（22remote+7网络JSON+14文档+60settings）；不是新空机或远端CI。最终本轮主机/独立/六配置日志无新增自有告警或错误。

六配置连续构建exit0，utils/request_config源码17:46:19、集成对象17:49:33、产物17:49:41；当前修改已纳入，map保留request_header_put_checked。SDK/厂家固定且目录干净，dependencies.lock相对HEAD无差异。证据`.cache/p3-initial-headers-*`含主机/专项/库存/重复/独立/Python/六配置；生成物不入库。

| 配置 | 字节 | SHA256 |
| --- | ---: | --- |
| Read Pico集成（禁止运行） | 1,923,152 | `8e67153054ef4dd400a71e423949e629803c40119c8f304d8c26391dd75697a8` |
| 原板 | 2,059,568 | `0d317c6742aa8a0ce3ff17eb91ccf032665dc6118d025745bd9a51bb9600889d` |
| 照片隔离 | 406,352 | `84ae3b2f46c666575e38f99bf34f99e61bccfa01c34544a367494389b273e978` |
| SD隔离 | 332,304 | `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` |
| 显示隔离 | 305,120 | `44f691bd484d34d7c603c2ab95562053da670574cdec9c00cfe67c132d91eab2` |
| factory只读 | 275,344 | `1541861d7b9b407a2623a28801f39c91390d50d51992b7428ddfb5ef87dcbccb` |

烧写/回读：**未执行**。真机验收：**未执行**。无串口/复位/业务网络/AP/真实NVS/SD/测试卡/原片/屏幕/相机/参数/提交推送，产品/OTA/迁移门禁保持；历史SD诊断c876599f…/STOPPED不是本轮实时核查。同步TODO/README/架构/开发/索引/CHANGELOG/配置说明；下一安全开发凭据/URL/证书生命周期、检查式保存及redirect，再继续general/完整图像/RTC/调度/文件/空间/全栈退出。完整目标继续，无新人工权限请求，不计P3/P5完成。
