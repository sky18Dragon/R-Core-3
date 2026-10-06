# 新方案验证矩阵与记录

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
