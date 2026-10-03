# 新方案验证矩阵与记录

边缘供数探针候选299,952字节/SHA256 `daf0b7bffc8f9f723ebe9e861bb0222b6248c995a20d37e14ef55a91d1692eef`已仅factory部署并全长回读一致，分区表前后不变。唯一静态清白78/78帧完整且用户确认整屏正常；同启动仅一张`gc16-gray16-page`完成，36/36相与12条目标行均完整。六组成对轨迹相等、五个边界全部可区分；已部署分析器因688旧计数及CPU1启动预填误分类而锁存FAULT，不把该软件误报当成扫描故障。新实拍仍复现左侧宽连续区，本轮明确不通过；会话已正常STOPPED，P2未完成。

离线修正候选为300,080字节/SHA256 `5069a6d01093006215c76736fcbc6427c86e0bffa4e232b6560a023ea8bd5349`：要求696次回调，并把CPU1上每帧前8次同步启动预填与后续异常跨核分开。定向普通/ASan各19/19、串行全量各300/300及固定IDF 6.1显示编译通过；未部署。软件指纹不等于DMA/物理gate证据。

下方按日期记录历史验证，不将旧产物测试或实屏结果转移给新产物。

更新时间：2026-10-04。本文件仅记录 PhotoFrame 移植。P0 完成；历史四角 GC16 单次软件/人工通过（2.140 秒），旧未编号灰阶及新编号灰阶均因白偏未通过。边缘探针已证明关键source回调行关系正确，但局部图实拍仍有左侧宽连续区；P2 未完成，白偏、回调后扫描边界、历史欠载/黑带机制及连续稳定性未核验。标准 SY 易失性启动允许，校准写/完整迁移不批准。

## 验证矩阵

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
| P2 显示 | 修正后300 CTest及300 ASan/UBSan通过，含真实厂家CPU绘图、官方局部布局、边缘故障注入及双核启动预填分类 | daf0b7bf…/299,952字节已仅factory部署/回读一致；唯一清白人工通过，唯一局部图36相和12条目标行完整；5069a6d0…修正候选未部署 | 历史四角通过；旧/编号灰阶白偏失败；前次局部图左边缘失败；本次关键行回调关系通过但实屏待用户复核 | 当前FAULT由已知旧分析器误判触发，屏幕保留局部图；先取目视/照片，不做GC16铺白，灰阶/温度精度/长期稳定待验收 |
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
