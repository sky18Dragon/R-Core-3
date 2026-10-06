# 固件烧写与维护

2026-10-05最新部署：用户允许更新并明确“刷白后直接执行S1显示”。404,336字节/`9f6d6f6fc329344647f601b976d1869d70bd758b141f4db4f580f389fa2540a5`仅factory写`[0x10000,0x72b70)`、擦除`[0x10000,0x73000)`；旧62cac21b…完整应用已读回保留，新应用独立全长回读一致，4096字节分区表前后保持`f078383d…`。新启动一次挂载、清白78/78软件成功，按本次许可推进white-confirm而不记人工白屏验收；同一S1一次处理56,161ms、GC16 36/36相398.130ms、刷新/电源退出成功。用户实拍确认“画面正常没有问题”，photo-confirm接受后一次stop、显示/SD/HAL清理ESP_OK、STOPPED/operation=cleanup=0，控制台exit0/串口无占用、SD卸载、原片保留。不重刷、不写数据分区/校准，不启动产品；旧状态以下按历史保留。私有证据`.cache/p3-buffered-photo-hardware-PAms8H/`。

2026-10-05最新部署与验收：403,184字节/`62cac21b8cc7502fd6c79611f6b57d3f1c174d2549b2be41ee539f7827c0a554`已仅factory写入`[0x10000,0x726f0)`，擦除`[0x10000,0x73000)`。写前旧e7d5bd8d…完整回读保留，写后新应用全长一致，4096字节分区表保持`f078383d…`。清白及S1正向实屏获用户确认；photo-confirm接受后正常stop，显示/SD/HAL退出均ESP_OK、最终STOPPED，SD已卸载、串口关闭。私有证据目录`.cache/p3-photo-orientation-hardware-GxPcPw/`。下文e7d5bd8d…为历史部署，当前以本段及VALIDATION最新条目为准。

固定部署镜像位于`.cache/p3-photo-orientation-candidate-0VzAUF/photo-app.bin`，与本轮独立回读一致。仅应用更新许可已按上段执行；后续任何新流程仍遵循具体范围及每启动一次的诊断门禁。见 [单张照片诊断](READ-PICO-PHOTO-VALIDATION.md)。

当前部署（2026-10-05首张照片轮次）：403,168字节/`e7d5bd8d1c338e448829fab7c241002f93b5c5fef0c304dd9b1a0d1c20b39ee2`仅factory写`[0x10000,0x726e0)`、擦除`[0x10000,0x73000)`。独立全长回读逐字节一致，写前后4096字节分区表均`f078383d…`；写前旧d760371b…完整应用匹配并保留。新启动一次mount/clear-white，人工白屏通过；一次photo-show完整读取S1/EOF并完成EXIF6、Cover、灰阶和36相GC16，软件返回成功，但实拍上下颠倒，已photo-reject并stop。显示释放、SD卸载及HAL退出均成功，当前STOPPED、串口关闭、卡仍插入且卸载、文件保留；屏幕静态保留倒置照片不是程序仍在运行。未写bootloader/分区/NVS/PHY/内部storage/PMU校准，完整PhotoFrame集成固件未烧写。

本轮写前整长读取322,848字节旧文件诊断，与6b85769b…固定输入一致，保留该诊断app副本；此前3168a7cc…副本亦保留。这不是完整Flash/产品迁移备份。此前有卡/无卡及写/sync/关闭/重开属于各自历史会话，本轮只追加ESP重启/重挂读取证据，不重跑file-test或覆盖/删除同名文件。不照用构建器的全量flash建议。

后续单次取卡断电轮次没有烧写：用户按要求完整取卡/重插并确认后，同一d62fd8d2…新启动status/mount/file-check/status/stop再次核对4096字节/EOF成功，created=0/written=0、关闭错误与errno均0，正常卸载/HAL退出、STOPPED/串口关闭。本卡单次取卡断电后数据保持通过，但不等于整机/写入中异常断电；不自动追加验证或推导为长期任意更新许可。挂载不是硬件写保护，不宣称FAT元数据绝无变化。

photo-check的主机测试/编译与本轮部署/ROM真机结果分别记录，本轮没有源码/配置变更或重跑软件测试/编译。仅发送status/mount/photo-check/status/stop各指定次数，不连带file-test/file-check或显示/产品启动。后续更新须核对新产物和新许可，不照用构建器全量flash建议。详见 [照片检查](READ-PICO-PHOTO.md)。

以下PM锁候选是此前显示验收记录，已被SD应用替换。首次SD部署前整长读取305,104字节与6f2a4ea…一致，可作为这一显示诊断应用的本地恢复副本；这不提供移植前旧应用或完整Flash恢复保证，恢复动作需另行确认。

此前显示部署记录：PM锁候选`6f2a4ea…`/305,104字节已仅写factory范围`[0x10000,0x5a7d0)`，扇区擦除止于`0x5b000`；完整应用回读逐字节一致，烧写前后4096字节分区表SHA均为`f078383d…`。未写bootloader、NVS、PHY、storage或PMU校准。镜像ELF为`44ec27ed6eb7d2783415a5629f296dfe6be2da9a30892731b415c9bcf4983545`，sdkconfig仍`e16d333e…`。

该候选保持两个6行bounce buffer/精确684行、18MHz、leading-skip关闭、factory VCOM、DEFAULT 36相、80MHz存储、图案及电源流程不变，仅在显示事务持有CPU/APB max锁。静态清白78/78帧并人工通过；同启动唯一局部页36/36相合计399.327ms，实拍16阶严格单调并已确认。显式stop后显示/HAL正常释放，当前STOPPED。不得因已部署而照用构建器输出的完整flash命令；后续硬件轮次仍需单独人工关口。详见VALIDATION与 [后续方案](GRAY16-VALIDATION-PLAN.md)。

历史C阶段连续HV候选曾完成2帧/72相但仍白偏，说明供电连续性不是共同根因。当前PM锁A/B已同时恢复时序和16阶；固定上游内部`portMAX_DELAY`未在本轮触发，但仍不具备安全硬中止语义。单张灰阶通过不等于长期稳定通过。

原板 `build/`、双 OTA 诊断 `build-read-pico/`、factory 只读 `build-read-pico-factory/` 和受控显示 `build-read-pico-display/` 是四个隔离配置，不是同一个发布镜像。**不能将原板产物烧入 Pico，也不能照用编译器打印的全套烧写命令。** 用户放弃完整备份；当前候选仅app更新、回读一致且分区未变。历史白偏与边缘异常保留为失败记录；当前16灰阶根因已关闭，但P2其余验证和完整迁移仍未完成。见 [验证记录](VALIDATION.md)。

## 首次上板前的人工关口

### 历史候选：此前未部署的官方绘图控制

显示 app 284,688 字节，SHA256 `eea2743296c97656da0d63ee3196c4e96cfeb63980dcba41a0cfec1eb8419248`；ELF `21cfb088d507ecd34eaa139d62c0caf8217d79a6bce52abbe421538ec8c022ca`。配置 SHA 仍 `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`，12 MHz LCD/80 MHz Flash及PSRAM/PERF/64 KB/64 B/默认36相/1K LUT/标准电源不变。主机 289/152 sanitizer 子集/10/8、四配置编译及本地 image-info 通过；S3/DIO/16 MB/80 MHz/MMU64 KB/checksum/hash 合法，不代表实板已回读。

新增 gc16-gray16-vendor 保持原无编号全屏布局，用真实厂家 epd_fill_rect(i*17)/from-white，只可用于本启动人工全白后的首次绘制；不是官方整包。此轮没有打开串口、复位、烧写或刷新，设备仍是 03be87b6…、最后 STOPPED/白偏拒绝，当前 USB 串口未枚举。旧私有应用回读只证明旧设备应用，不对应本地新产物。

拟写范围 `[0x10000,0x55810)`、扇区擦除 `[0x10000,0x56000)`，**未执行、未获本轮具体许可**。下一轮先由用户重新连接/允许，核验身份、安全、分区与兼容后，仅 factory app 写入及全长回读/分区不变；新启动一次 clear-white、人工全白后 white-confirm，再仅一张 gc16-gray16-vendor，独立人工观察/确认或拒绝并 stop。失败不补刷，不沿用旧白参考或跨图 from-white。保留 bootloader/分区/NVS/PHY/FAT/PMU 校准；用户不完整备份的决定不代表旧 app 完整可恢复。GC16 清白、HV 保持、前导跳过、产品升频等不在该候选内。

### 通用人工检查

1. 确认实际板型、Flash/PSRAM、PMU 出厂 VCOM 读取及源码/SDK/配置版本。
2. 通常保存完整 Flash 备份、分区表及恢复说明。用户本次明确放弃完整备份：仅保留读取的分区表/应用头元数据，不声称具有旧应用的完整回滚能力。历史废纸篓两条备份路径已失效，不作为恢复副本。
3. 核对本版分区表、生成的烧写参数、产物摘要和保护区域，避免套用旧 Demo 的偏移。
4. 按本版经验证的流程烧写和回读，单独记录启动、显示与人工实屏验收。
5. OTA 前验证板型匹配、分区空间、升级后配置/SD 保留及失败恢复；不直接向设备推送上游通用板固件。

6. 已获用户允许：把 PMU VCOM 配置进 SY 易失性运行寄存器，应用厂家标准 VLDO/延时/operation；另获 factory 只读诊断、受控显示测试/首次清白授权。普通诊断仍 false，显示测试须命令触发。任意调参/波形修改、PMU/NVS 校准写和完整布局迁移不在授权范围。

不自动擦除 NVS、格式化 SD、写回 PMU 校准或启用未核验 OTA。当前 build.py 仍只允许编译，不执行烧写。ESP Flash 副本也不包含 CW32 PMU 固件/校准，不能声称一次备份已覆盖所有芯片。

## 本设备的 factory 诊断方案

用户确认是小纸 Pico，USB 已连接 Mac。ROM 报告 ESP32-S3 v0.2、8 MB PSRAM、16 MB Flash（厂商 0x5e / 器件 0x4018）；安全启动/Flash 加密关闭。PCB 修订与是否改装尚未核实。

首次读取的布局与厂家 Demo 一致，但当时应用头是 `gr4_paper_frame 0.1.0` / IDF v6.1，不是厂家 Demo 应用。当前已替换为本方案受控显示测试 app；表保持如下：

| 区域 | 偏移 | 大小 | 本次允许写入 |
| --- | --- | --- | --- |
| nvs | 0x9000 | 0x5000 | 否 |
| phy_init | 0xe000 | 0x1000 | 否 |
| factory | 0x10000 | 0x400000 | 仅诊断应用镜像及其扇区范围 |
| storage（FAT） | 0x410000 | 0x500000 | 否 |

bootloader、0x8000 分区表及其他数据区域同样不在授权写入范围。独立 `boards/partitions.read_pico_factory.csv` 固定上述布局；构建命令为：

```sh
python build.py --board read_pico --step firmware --read-pico-factory-diagnostic
```

产物 `build-read-pico-factory/`、配置 `.cache/sdkconfig.read_pico-factory` 与原板/双 OTA 诊断完全隔离。只编译，不自动烧写；完整烧写示例仍禁止照用。须核验镜像 chip/revision、MMU、大小、校验和及生成分区与实板一致后，仅向 `0x10000` 写入 app；不写新 bootloader/分区/otadata。写后对应用实际长度回读比对，并再次读取分区表检查未改变。保留写入范围不能替代对所有 NVS/FAT 内容的完整校验，也不保证旧应用可恢复。

诊断保持 80 MHz，禁止显示/SD/Wi-Fi/NVS 业务。旧应用头的 SDK 版本只作为兼容性输入，不证明完整镜像或现有 bootloader 已验证；最终以本版启动/PMU 读数为证据。后续完整相框/OTA 分区迁移仍需单独审查。

## 已批准的首次清白测试

构建时增加 `--read-pico-display-validation`，隔离到 `build-read-pico-display/` / `.cache/sdkconfig.read_pico-display`，保留相同 factory 表。历史清白 app 为 275,600 字节；前轮调试图案 app 为 288,224 字节，写入 `[0x10000,0x565e0)`、擦除 `[0x10000,0x57000)`，全长回读一致、分区表未变。该调试图案 app 已被本轮性能候选覆盖，当前镜像及状态见下节；未写 bootloader/数据区域。

入口先只读核验布局/PMU/VCOM，再等待指令；启动/重启不自动开屏。clear-white 退出检查通过后必须人工确认才发送 white-confirm。本轮重新清白（13.145 秒）人工确认后仅一次 gc16-corners，欠载 0x400 失败锁存；退出观察成功，再显式 stop，status=STOPPED/operation=0、串口关闭。USB 日志未观测到各条 DISPLAY_DEINIT/HAL_DEINIT ESP_OK，成功以随后状态及 stop 返回契约为依据，不补造日志。**参考已清除，不跨启动复用。** 见 [显示说明](READ-PICO-DISPLAY.md)。

上述是前轮调试镜像测试；当前设备已换为性能候选，见下节。清白、人工观察、GC16 失败和退出分别记录，不宣称灰阶/方向/照片通过。两轮均有 STATUS 首读 CRC 拒绝后有界重读有效的记录，不跳过校验/重发 GET/HOST_READY/清除 faults。

## 历史性能候选：曾部署，白参考未通过

此前用户另行“确认”后，app 换为 `7ced0efd9feb7d84699d5a5c4e4a623165634707bf4f8cb1f9a943dfdc4cee80`（275,680 字节），仅写 `[0x10000,0x534e0)`、擦除 `[0x10000,0x54000)`；全长回读一致，前后 4096 字节分区表相同。身份/布局/安全配置/image-info 核验通过；实机 PERF/65536/64，仍 80 MHz/原扫描/波形/校准，不写 bootloader/分区/NVS/PHY/FAT。该镜像现已被下节扫描诊断 app 覆盖，不再是当前应用。

本次启动只读核验 PMU 1.0.9/VCOM valid/1580 mV，显式 status=WAIT_CLEAR_COMMAND 后仅一次 clear-white，14.332 秒，退出 input0/out0=0x81。用户报告黑线并进一步确认刷固件前不存在，新近照可见下沿连续黑带，实屏白参考不通过；未 white-confirm/GC16。显式 stop 后 DISPLAY_DEINIT/HAL_DEINIT ESP_OK、status=STOPPED/operation=0，控制台关闭 exit 0。下一步补末行/帧结束与退出证据，拟新受控恢复，不自动刷白/升频/复位，不把软件返回当实屏通过。用户仍放弃完整备份，不承诺原移植前 app 完整回滚。

## 历史扫描诊断：四角单次通过，灰阶实屏未通过

历史扫描诊断 app 新增末行首尾脉冲/填充行与帧完成观测，应用 282,896 字节，SHA256 `89c5f1739ed58bbb6c9130793b1e9a0e3d0333a4da5dbb6407f6f3a8272bd861`；ELF `7717b54c8fb5b682af709ae428b4a3b5a0f94fbc162d2df68a9c7e27ae67902a`。配置 SHA 沿用 `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`。全长回读一致，曾替换历史 275,680 字节性能候选，现被下节新候选覆盖，不混用回读或实屏记录。

针对新黑带，用户回复 `yunxu`（允许上述 app-only 更新和一次诊断清白）。重新核验同设备身份、ESP32-S3 v0.2/8 MB PSRAM、16 MB Flash 5e/4018/3.3 V、安全配置及实际分区与候选一致后，仅写 `[0x10000,0x55110)`、擦除 `[0x10000,0x56000)`，仍在 factory 内。全长字节/SHA 一致，前后 4096 字节分区表相同，SHA `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20`。不写 bootloader/分区/NVS/PHY/FAT，不备份原 app、不承诺完整回滚；未使用 IDF 全套 flash_args。

部署轮显式启动新 app，确认 PMU/VCOM 及 WAIT_CLEAR_COMMAND 后仅一次 clear-white。14.018 秒、退出检查通过，78 帧软件记录完整，用户确认“黑带消失，整屏白色无明显异常”；同启动 white-confirm 接受后 stop，DISPLAY_DEINIT/HAL_DEINIT ESP_OK、STOPPED/operation=0、串口关闭，该部署轮未 GC16。

用户随后另行允许新启动清白、人工确认后仅一张四角 GC16。复测轮未刷写/改代码；新启动 ELF/PMU/VCOM/布局门禁通过，一次清白 14.018 秒/78 帧完整、人工确认全白后 white-confirm。唯一 gc16-corners completed=1、错误均 0、2.140 秒、退出检查通过；用户确认“四角位置正确，无明显异常”，pattern-confirm 后 stop，显示/HAL ESP_OK、STOPPED/operation=0、drawn/visually_confirmed=0x20，完整 status 捕获，串口关闭。

上述四角验收位仅属历史，退出已清除参考。用户随后允许新启动清白及人工确认后仅一张灰阶：本轮仍未刷写，清白 14.022 秒/78 帧完整且人工全白，gray16 completed=1/错误均 0/2.138 秒，退出/内存释放通过；用户反馈“白色偏多”并提供照片，实屏拒绝。pattern-reject 后 stop/显示及 HAL ESP_OK、完整 STOPPED/operation=0、drawn=0x8/visually_confirmed=0，控制台关闭 exit 0，端口无占用。未 pattern-confirm/补刷/重试，参考清除，物理屏保留测试图。

下一步先核验灰阶能力和温度有效性并制定诊断方案，不跨会话复用白参考、不自动连刷/更新固件。已部署观察器未记录 GC16 feeder；新候选只对12条选定feeder行做指纹且尚未部署。两者都有开销，也都不证明DMA/物理扫描；四角通过或黑带消失不证明长期修复，P2 未验收。若需 CKV/STV/高压轨测量，另安排人工介入。

## 当前应用：主动温度与编号灰阶，编号图实屏未通过

当前 `build-read-pico-display/esp32-photoframe.bin` 为 283,792 字节，SHA256 `03be87b6a65ffd555940a9beef36c33dd209f895940f9bc27ba539b8a8dc47a6`，ELF `0f69c9874b4c11792fbce4caa9459ef1c7e55e22dc17b49f9ccfdeba71c83d25`；sdkconfig SHA 仍 `e16d333ea9818c1030ae2cbe084de301820dca4013f1f836b0a95228671191e5`。新应用回读 `.cache/numbered-gray-app-readback.bin` 全长一致；旧 `.cache/scan-probe-app-readback.bin` 仅作 282,896 字节/89c5f173… 历史证据。

新候选只修正 ON=1 后采样（标准启动配置不变）和新增单张 `gc16-gray16-labeled`。原 80 MHz/12 MHz FULL/1K LUT/厂家波形/PMU 校准保持；276 CTest/139 sanitizer/9 构建保护/8 控制台及四配置编译沿用同产物离线结果。用户允许后重新核验身份/安全/布局/兼容，仅写 `[0x10000,0x55490)`、擦除 `[0x10000,0x56000)`；全长回读一致，写前后分区表不变，SHA f078383d…；不写 bootloader/分区/NVS/PHY/FAT，未使用完整 flash_args，不保证旧 app 可回滚。

新启动门禁/PSRAM/PMU 1.0.9/VCOM 1580 mV 通过；唯一清白 14.022 秒/78 帧/退出通过，用户确认全白后同启动 white-confirm。唯一编号 GC16 2.137 秒/completed=1/错误均 0，主动温度 28°C/ON=1/PGOOD=1，退出及内存释放通过；用户仍报告明显白偏，不计实屏通过。pattern-reject 后 stop/完整 status=STOPPED/operation=0、drawn=0x40/visually_confirmed=0，显示/HAL ESP_OK，参考清除。控制台随后报 Device not configured、exit 1，已结束且端口消失；不猜断连原因、不复位/重连。下一步按 [官方灰阶对照](READ-PICO-DISPLAY.md#官方-16-灰阶示例对照) 制定隔离实验，追加设备动作需新的具体许可。

## 分区迁移审查方案（尚未批准或执行）

实板已核实为 Demo 布局。默认双 OTA 诊断仍生成 PhotoFrame NVS 0x9000/0x6000、PHY 0x11000、OTA metadata 0xF000/0x2000、ota_0 0x20000；这些不是设备当前布局，也不是本次 factory 诊断布局。

新 NVS 范围包含原 PHY 区域，OTA metadata 与原 factory 开头重叠，新布局不保留旧 Demo 内部 FAT。即使不调用 erase_flash，只写烧写参数中的区域也可能破坏旧固件或配置；不承诺“只改 app 就能安全迁移”。

拟采用的审查顺序：

1. 已完成器件/容量/分区/应用头读取，用户放弃完整备份；如后续需要迁移全部布局，再说明额外风险并重新取得对应授权，不将本次 app-only 授权扩展为完整迁移。
2. 根据真实分区列出 NVS/PHY/内部 FAT/用户配置的保护范围。确定是否要保留阅读器设置及内部文件；能离线导出则先导出，不能导出时保留原分区原始副本，不猜格式。
3. 评估保留原 NVS/PHY 的专用布局与受控迁移布局；只有实际记录与用户选择齐备后才修改分区生成器/配置。当前双 OTA 编译布局不是预选迁移方案。
4. 核对目标镜像和每项写入范围，以及恢复能力或用户明确接受的不可回滚风险；不自动擦 NVS 或格式化内部/外部存储。本次放弃备份只覆盖已批准的 factory 诊断操作，不预先批准迁移。
5. 先只读诊断，核对启动/PMU/VCOM后，才在屏电源运行权限获准的专用流程验证 EPD。首次清白须人工确认本次启动参考，再显示灰阶/奇偶像素/四角/照片。当前诊断不会自动进入显示测试。

完整PhotoFrame已有编译链接隔离入口，新板NVS自动擦除和Web整分区reset已禁止，运行仍未开放；初始化成功不代表旧数据/分区完成迁移。独立SD已有有限挂载/专用4KiB读写/取卡断电读取及S1 ROM解码验收，Web/照片实屏/睡眠/OTA尚未集成验收。

2026-10-04文件JPEG业务软件轮次未操作设备：当前仍为STOPPED的d760371b…/325,904字节。新集成cf0e40bc…/1,978,080字节只验证编译/链接，入口不执行产品；新SD构建e05fc2ed…/326,128字节亦未烧写。主机方向/Cover/灰阶通过不构成照片显示授权，须另准备受控单张诊断并确认具体候选/写入/显示范围，不直接烧写集成应用。

编译成功、烧写校验成功、屏幕仍有旧画面均不能单独证明本版应用正常工作。

官方资料（2026-10-03）：[Demo 与硬件诊断](https://dot.mindreset.tech/docs/read_0/start)、[官方源码构建恢复入口](https://dot.mindreset.tech/docs/read_0/firmware) 已查阅，尚未取得并核验用于本设备恢复的固定二进制。分区依据见 [上游核对](UPSTREAM-BASELINE.md)；显示权限/参考状态见 [显示适配](READ-PICO-DISPLAY.md)。
