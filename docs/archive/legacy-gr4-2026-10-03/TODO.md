# 开发计划与当前进度

## 项目已终止 — 2026-10-03

最终资料整理：按用户新要求，将原.cache的64项测试图片/显示缓存、244份日志及4份校验元数据、32项辅助测试数据统一迁移到 `/Users/zhanghuilong/Documents/ChatGPT/R-Core-3-2-测试资料`，按类别和原批次保留路径，全部456个原文件移动后SHA256核对一致。112份固件备份/回读镜像共590,558,256字节已从项目删除，移入 `/Users/zhanghuilong/.Trash/R-Core-3-2-firmware-backups-3CRIkV`（可恢复，未清空废纸篓）。原.cache已为空并移除；项目仅保留文档及Git元数据，原始Downloads图片和设备不动。后文“备份保留”均为删除前历史，不作为当前恢复文件位置；归档清单记录去向。

用户明确放弃当前项目开发和调试，并要求删除现有代码。开发、烧写和采集已停止，核对无相关运行进程；源码、测试/工具、第三方及托管依赖源码、构建配置、全部build目录、项目内缓存SDK和临时校验脚本已从项目移到 `/Users/zhanghuilong/.Trash/R-Core-3-2-code-U2TFQd`，可从废纸篓恢复。没有清空Git元数据或修改其暂存状态；文档保留、测试资料外部归档、固件备份已删除，项目不再可构建。下方所有计划、下一步、待重测和里程碑仅为历史记录，不继续执行。

设备停止边界：取消前d14d1093…三镜像已完成写入及esptool镜像哈希验证，整片回读尚未执行，应用未自动启动；完整16MiB烧写前备份历史SHA256 `a2bed5e6fd11573d435372d826deda9fdb2c944d25628626cb1fa50e277d4cdf`，该文件及原厂备份现已按用户要求移入上述固件备份废纸篓。未操作硬件或恢复原厂固件，不计新版启动/接收/身份验收。

更新时间：2026-10-03（北京时间）

当前软件回归：49/49 CTest、46.67秒（C目标ASan/UBSan），包含新增实际SDK USB分支回归；门禁37项/只读采集19项/交互工具8项通过。身份专用app433232字节d14d1093…已编译/过门禁，整片备份已完成，尚未烧写；真机接收/身份/Gate A未通过。下方旧数量和设备状态为历史。

## 当前里程碑

USB无驱动接收修正已实现（2026-10-03，烧写前）：独立io/gr4_usb_rx.c使用固定SDK的O_RDONLY无驱动预读取分支，每50ms最多32次单字节读取，打开前及每字节前检查未安装驱动，stop/断开/混用丢弃局部批次；不修改SDK/BSP或延长确认期限、不自动确认。实际SDK open/read/availability函数+实际RX适配离线ASan/UBSan回归通过，复现旧O_NONBLOCK读0并验证完整49字节解析、空/驱动/停止/断开/参数/跨轮CRLF；USB worker新增2种混用拒绝场景和每tick读取上限，原10种仍通过。49/49全量46.67秒，测试输出文字更新后对应3项复核2.18秒。专用真实SDK编译/门禁通过，SHA256 `d14d10936aac5bb4f312f13ac247437216645c5c4f63ad3c567e790e740ecd5f`；旧RX缺新符号被新门禁拒绝，driver_install/use_driver T/W符号禁止。默认359440字节dd5ebff0…重编未变、无连接417296字节5c63fedc…重编变化，均门禁通过/未烧/不套旧真机；普通compile-only ELF通过、无.bin、jpeg与identity门禁均拒绝。Pico端口独占核对后按既有授权完成完整16MiB备份，私有 `.cache/pico-gr4-usbfix-dIGslm`；仅备份default-reset进入下载器，不自动运行新应用。实际USB收命令/相机只读身份仍待新人工TTY确认；禁止自动代发y，旧负对照只证明旧版到期清理。

当前发现接收适配缺陷，停止继续人工重测（2026-10-03）：用户confirm-04在新提示后输入y，主机工具报告已发送，但原始3867字节日志/20条一致code11-flow6显示固件未入队/未读取身份，清理全1；SHA256 `bb6de60bc4c4cedd578c825409e1a7ffe2d402d30c1f4c43c0ec0983b5cec9dd`，END35177ms、当轮返回45177ms。开头还有前一轮Returned45729ms，不把两条返回计两次本轮启动。固定SDK无驱动VFS用O_NONBLOCK时先查询只存在于驱动ringbuffer的可读数，始终0，完全不调用硬件FIFO取字节。离线直接编译实际SDK read/availability函数，ASan/UBSan复现49字节就绪→100次EWOULDBLOCK/0消费；ELF无driver_install/use_driver，SDK/BSP未改。原mock替代read没有覆盖此路径，48组通过不能证明接收可用。此前无确认负对照/输入退出证据仍有效，但不代表接收功能通过。下一步修正独立无驱动接收适配、保证有界非阻塞与禁止混用驱动，补实际SDK分支回归/主机/编译/门禁，再按既有授权备份烧写/回读及重测；不延长确认时限/自动代发y、不修改SDK驱动。此轮只诊断/记录，未修改固件或设备/相机状态；工具已退出、串口释放。

交互工具首次真机提示解析通过，但人工回复已过期（2026-10-03）：用户终端记录显示完整“发现本轮候选”后先输出20次诊断结果，再输入y，工具明确未发送。私有 `manual-identity-confirm-01.log` 3820字节、SHA256 `c282f92291e0fcb2831fcbb3570ae95a12c018bce0b299ac635455f7d7f2948a`；30674ms完整token提示、35677ms END/第一条结果，35677–45177ms共20条一致code11/flow6/身份无结果，cleanup/sdk/input/board全1、start/wait/close全0及应用返回，无GR4_CONFIRM_INPUT。确认本轮完整提示能被实际工具解析、未获有效确认时到期退出；不是相机连接/身份PASS或相机故障。APP_ENTRY仍缺失，不补造完整启动日志。工具已退出、串口无占用，未重烧/主动复位/发送确认/操作相机或SD。重测需新日志confirm-02，先运行工具再人工启动，提示出现后4秒内现场新输入y+Enter，不预输入、不等待结果、不复用已过期token。

身份诊断首次无确认负对照/退出路径真机PASS（2026-10-03）：用户确认“已开机”。原120秒只读采集0字节/1连接/0传输错误，未捕获结果，保留为空文件、不判启动失败。随后保持现状45秒只读补采3207字节/1连接/0传输错误，私有 `manual-identity-no-confirm-01-supplement.log` SHA256 `59d119876ccf66246486d80e09ca6f7b4f91e9a9a526dc7248e060f2f724a6f2`；30676ms提示仅半行，36579–45079ms共18条完全一致code11/flow6(EXPIRED)/owner4294967295/lengths0,0,0，cleanup/sdk/input/board全1、start/wait/close全0、verified/pair/write全0，45579ms应用返回。结合只读工具禁发及已核对入口，确认本轮未消费人工回复/未激活身份Owner，到期后输入IO与SDK/PMU/I2C退出路径通过；不是相机身份证据或独立射频/功耗/掉电证明。APP_ENTRY/END/完整提示/前2条结果缺失，不补记20条完整或完整启动时序。采集已结束、串口无占用，本轮仅采证/文档，无主机主动复位/重烧/JTAG/相机/SD/NVS/校准操作；48组主机/编译沿用实现轮。设备仍d4f2ccaf…，下一轮需用户在Mac交互式终端现场新输入y+Enter才允许一次只读身份连接，不自动代替用户确认。下方“采集中/待人工启动”为历史。

身份诊断无确认负对照采集中（2026-10-03）：用户“准备好了”后开启120秒只读采集，已确认USB端口真实打开/capture ready，私有 `.cache/pico-gr4-identity-6idB4M/manual-identity-no-confirm-01.log`，采集进程session98623。待人工拔USB、完整关机/开机Pico并重连；这轮不发送确认、不操作相机，无新运行/退出/身份结果，不能预判PASS。设备为已烧写/整片核验的d4f2ccaf…，本轮无再烧/主机主动复位/JTAG/SD/NVS/校准操作。下方待采集为此前状态。

最新烧写/回读完成（2026-10-03）：用户“已重连”后Pico USB与端口独占核对通过。完整16MiB备份、当前792c5409…/历史尾部/显示及原厂回滚核验后，按既有直接烧写授权烧写身份诊断d4f2ccaf…；boot21168/table3072/app432992三镜像Hash verified，整片16MiB回读逐字节验证三镜像、页尾FF及全部保护区域：0x6000–0x7fff、0x8000–0xffff和0x7a000–0xffffff与本轮备份相同，未额外擦尾部/NVS/storage。私有目录 `.cache/pico-gr4-identity-6idB4M`，备份SHA256 `760cf58c1bf421aa8e83f66b4d85954caf0af540c59dca2ac375e55cbf93e68a`，整片回读SHA256 `a2bed5e6fd11573d435372d826deda9fdb2c944d25628626cb1fa50e277d4cdf`；旧回滚及本轮完整备份保留。仅备份default-reset进入下载器，写入/回读before/after no-reset，工具退出/串口无占用，设备留下载器、新应用未运行。相机/SD/校准未操作。下一步人工就绪→只读采集就绪→完整关机/开机Pico并重连，先验证“无确认不连接”及输入/SDK/板卡退出；之后另一次启动现场TTY确认才测试身份读取。未计真实身份或cleanup/Gate A通过。下文回读中/未烧/未连接均为历史。

当前设备更新（2026-10-03）：用户“已重连”后，Pico USB重新枚举/端口无占用。完整16MiB备份、当前792c5409…/历史尾部/显示及原厂回滚核验通过；本轮私有目录 `.cache/pico-gr4-identity-6idB4M`，备份SHA256 `760cf58c1bf421aa8e83f66b4d85954caf0af540c59dca2ac375e55cbf93e68a`。依既有直接烧写授权写入身份诊断d4f2ccaf…的boot/table/app，三镜像Hash verified，正在整片16MiB回读，尚不计回读/启动/身份PASS。备份仅default-reset进入下载器，写入/回读no-reset，新应用未运行；相机/SD/NVS擦除/校准未操作。下方“Pico未连接/尚未烧写”为此前阶段。

最新：独立 USB 人工确认只读身份诊断已实现（尚未烧写）。包含便携严格命令解析、USB 输入任务与≤1000ms退出屏障、单次设备入口/随机源nonce、现场交互工具以及专用门禁；没有自动确认/配对/写入。48/48主机CTest（C目标ASan/UBSan，58.92秒）、37项门禁及真实固定SDK专用编译通过，app432992字节d4f2ccaf…；普通compile-only仍明确拒绝烧写、无.bin，混用诊断/compile-only的真实CMake拒绝。默认/无连接重编通过但产物已变为359440字节dd5ebff0…/417296字节e167e55f…，均未烧、不套用旧真机证据。SDK/NimBLE/BSP tracked-clean。只做本机端口枚举，Pico USB现已不在端口列表，未打开串口/复位/备份/烧写或操作相机/SD/NVS/校准；最后设备证据仍792c5409…。下一步需要人工重新插好Pico，随后完整备份/产物与回滚核对/直接烧写/回读；新应用不自动启动，现场新确认后才允许一次只读连接。详见 `docs/GR4-IDENTITY-DIAGNOSTIC.md`；以下里程碑为历史，不代表目前仍缺这些软件入口。

人工确认固定队列适配与同SDK host消费接入（仍compile-only）：提示/回复深度1静态队列，校验启动nonce/当次epoch/id/services，host每tick最多一条，拒绝/过期/reset/关闭后不激活；关闭后保留静态存储，迟到输入不接触NPL/已释放SDK资源。未实现实际输入驱动/界面/新动作验证、生产者退出或连接设备入口，不能把测试confirmed当人工同意。全量44/44 ASan/UBSan（43.68秒）、门禁33项及真实SDK隔离ELF编译通过，无.bin且实际门禁拒绝；初次测试声明顺序错误已修正。正式默认b5fe671e…及无连接dac733a0…重编/门禁通过、均未变/未烧，旧辅助构建不同摘要另记，不套用真机证据。设备仍792c5409…，本轮无串口/复位/烧写/相机/SD/NVS/校准操作，SDK/BSP未改。下一步真实输入、新动作/提示/nonce/有界退出与独立设备入口及门禁，当前无需操作设备。见 `docs/GR4-CONFIRMATION.md`；以下为历史阶段记录。

同SDK扫描与延迟只读身份会话适配完成（仍compile-only）：host被动观察30秒、禁收/取消/inactive后重新取时钟seal、当次确认后立即一次Owner激活；主任务85秒独立全流程期限，关闭栅栏/重复sync/reset阻止迟到激活并清除旧身份，退出追加扫描timer退休。最终43/43 ASan/UBSan（40.88秒）、门禁33项、真实固定SDK隔离组件/ELF链接及默认/无连接编译通过；初次mock同步reset期望错误已纠正并全量回归。未实现现场确认输入/连接设备入口，无.bin且门禁明确拒绝，不把waitOK或唯一候选当身份/同意。重编无连接417296字节dac733a0…未烧，默认b5fe671e…未变；设备仍此前已验证792c5409…，旧回读/回滚保留。本轮无串口/复位/烧写/相机/SD/NVS/校准操作，SDK/BSP未改。下一步确认事件通道、独立上板入口及退出/故障门禁，不要求重复开关机。见 `docs/GR4-SCAN-SESSION.md`；以下为历史阶段记录。

受控候选选择与 Owner 交接完成（尚无连接设备入口）：完整30秒窗口唯一候选、截止后禁收/停止证据、新鲜度、世代/id人工确认、单次消费与失败清零；便携核心无SDK依赖，prepare交接不做IO。最终41/41 ASan/UBSan（41.50秒）、门禁32项、全新真实SDK隔离组件/ELF编译链接通过，无.bin且compile-only门禁明确拒绝；默认359568字节b5fe671e…未变，重编无连接417296字节09101d54…摘要变化、未烧/无本版真机证据。设备仍此前792c5409…无连接版，旧私有回读证据保留。本轮无串口/复位/烧写/相机/SD/NVS/校准操作，SDK/BSP未改。下一步同一SDK生命周期延迟激活（扫描禁收/取消/inactive→当次确认→Owner），不得拼接销毁后的旧扫描或自动选择最强/首个设备。见 `docs/GR4-TARGET-SELECTION.md`；无需重复开关机，身份/Gate A仍未通过。以下为历史阶段记录。

无连接无线栈诊断首次人工启动生命周期PASS：用户确认“已开机”，120秒私有 `.cache/pico-gr4-radio-EvRlhj/manual-power-radio-01.log` 2145字节、2连接/226传输错误（含人工断开/重枚举尝试，不等同芯片故障），SHA256 `66a8fc8dfa93004294f5d7838e78633c833308742bf0b78b28db0491b9d5c7ad`。5164–13164ms共17条完全相同结果 code0/cleanup1/sdk1/board1/start0/wait0/close0/scan0/connect0/pair0/gatt0；13664ms应用返回。结合已核对入口：结果只在分阶段停止/host退出/controller先停/timer静默及删除核对/controllerdeinit与PMU/I2C清理后发布，确认本次无目标SDK生命周期成功。APP_ENTRY/STARTED/END/前3条重播及ROM头未捕获，不能称20条完整、独立电气掉电/射频功耗或相机身份/配对验证。采集已结束、串口释放；本轮仅采证/文档，未重烧/主机主动复位/JTAG/SD/NVS/PMU校准/相机操作；40组主机及编译沿用实现轮，不新计测试。设备仍792c5409…诊断版、旧画面保持；下一步开发受控只读身份诊断目标选择/设备入口，普通Owner仍compile-only，不自动连接相机。见 `docs/GR4-RADIO-LIFETIME.md` / `docs/HARDWARE-RESULTS.md`。以下待启动为此前状态。

用户确认“准备好了”，无连接无线栈诊断首次120秒只读采集已开启，并确认端口打开/capture ready；私有 `.cache/pico-gr4-radio-EvRlhj/manual-power-radio-01.log`。待人工拔USB、完整关机/开机Pico并重连；相机无需操作，屏幕不刷新。此刻无本版启动/SDK清理结果，不计真机PASS。本轮没有重烧、主机主动复位/JTAG或相机/SD/NVS/校准操作；烧写回读证据沿用上一轮。下文待采集/开发为历史。

无连接无线栈诊断已按既有直接烧写授权烧写并扩展回读通过，现需人工启动：最终40/40 ASan/UBSan（37.75秒）、门禁31项/采集18项、默认/专用真实SDK编译及门禁通过。专用app417296字节SHA256 `792c54093a6a115dda12db21ce7b4e144f89315639ab7710bfee5a94ec1519db`；完整16MiB私有备份 `.cache/pico-gr4-radio-EvRlhj/pretest-flash.bin` SHA256 `2c4f6224523cb5e6183eb165ee126369515535471a9866d85e4a1918f17443fa`，当前扫描/历史尾部/分区及已验收显示/原厂回滚核验并保留。boot21168/table3072/app三镜像Hash verified，0x410000回读4259840字节逐字节相符、SHA256 `3904b6161115bfc6740c1336b67ae1774c96fc8dc65cae07aa6989c24bae113c`；0x6000–0x7fff/0x8000–0xffff/0x76000–0x40ffff与本轮备份相同。仅备份default-reset进入下载器，写入/回读before/after no-reset；串口无占用，设备留下载器，新应用未运行，不计真实SDK清理/身份/功耗。未操作相机、清SD/擦NVS/写PMU校准/JTAG或改SDK/BSP；默认b5fe671e…未烧，显示回滚13117988…保留。下一步用户就绪→只读采集就绪→人工完整关机/开机Pico并重连，相机无需操作；普通Owner连接仍compile-only。见 `docs/GR4-RADIO-LIFETIME.md` / `docs/HARDWARE-RESULTS.md`。以下为开发和此前阶段记录。

无线栈独立期限/退出适配与无连接板上诊断已实现：SDK公开NPL ABI guard、运行时函数表/检查式timer删除包装，controller先disable→应用/SDK timer静默与事件排空→hostdeinit核对→controllerdeinit；初始化/同步/会话/停止/清理独立期限，失败不强杀/复用。普通Owner仍compile-only；新专用入口无扫描/目标/连接/GATT/SD/EPD，显式关闭SDK默认连接重试/隐私/DTM。主机40/40 ASan/UBSan（38.58秒）、门禁31项及采集18项、实际SDK默认/专用编译通过；专用417296字节792c5409…，默认359568字节b5fe671e…未烧。首次入口mock计数期望错误已修正并全量回归；默认重试保留connect符号的初版未烧，最终fresh配置/门禁拒绝旧配置。SDK/BSP未改。Pico已完整16MiB备份，当前应用/历史尾部/回滚核对中；新版尚未烧写/运行，不能计SDK真机清理或Gate A。见 `docs/GR4-RADIO-LIFETIME.md`。以下为此前阶段记录。

应用timer静默与检查式退休完成（仍compile-only）：Owner改用独立ESP_TIMER_TASK一次性timer，仅投递host event；确认host exit后外部正常任务以≤1000ms stop_blocking等待回调，再检查delete、移除排队event/释放event。失败封存/保留，部分释放重试不复用已删除handle；终态advisory stop失败清除身份。最终主机36/36 ASan/UBSan（34.61秒）和门禁27项、真实SDK组件/默认固件编译通过；默认359440字节02b2254c…未烧，隔离构建无应用镜像，扫描bd74b761…门禁复核通过。初次主机编译缺stddef头已修复再全量回归；扫描复核误选旧build-gr4-scan被门禁拒绝，正确build-gr4-passive-final通过，未使用拒绝产物。本轮无设备/相机操作，SDK/BSP未改。SDK内部host/stop callout仍经void释放，port_deinit先毁默认队列；应用timer退休不足以放行整个SDK退出。独立watchdog/stop工作任务/内部timer与transport静默/板卡清理/设备入口未完成，不计cleanup=1，见 `docs/GR4-HOST-TIMER.md`。以下为此前阶段记录。

受控GATT连接Owner完成（仍compile-only）：单目标/单次连接、host默认队列100ms串行pump、目标/角色/句柄核对、5秒连接/关闭期限、取消成功竞态与不延期、GATT先禁收再关闭、严格断连/不存在证据及host reset/安全事件拒绝。主机35/35 ASan/UBSan、门禁27项，真实v6.1组件/默认固件编译与门禁通过；默认359440字节02b2254c…未烧，扫描bd74b761…未变。本轮无设备/相机连接或SDK/BSP修改。发现固定SDK void callout-deinit隐藏timer停止/删除失败，host退出不证明timer task静默；已禁用自动释放，只请求停止并保留对象。整机wrapper/独立watchdog/timer静默屏障/SDK stop-deinit及板卡清理未完成，link_closed不等于cleanup=1，仍不具备烧写条件，见 `docs/GR4-GATT-OWNER.md`。以下为此前阶段记录。

GATT操作层完成（不是可烧写的连接诊断）：便携单次会话、整轮30秒/单步5秒、16服务/128特征/20不可复用ticket，实际NimBLE发现/分段读取接口；旧世代/句柄/请求和终态回调隔离、失败清零。仅读取三项公开身份，停在IDENTITY_UNVERIFIED，不提供Power/Operation或配对入口。主机34/34 ASan/UBSan及门禁27项通过；隔离Central/GATT Client、无SMP/cache的真实v6.1组件编译通过，仅库、没有应用镜像；门禁显式拒绝compile-only配置。默认编译/门禁通过359440字节02b2254c…（摘要已变，未烧），既有扫描bd74b761…门禁通过且未变。本轮无串口/复位/烧写/相机操作，SDK/BSP未改。Owner连接入口、同host串行调度、GAP世代/断连/host异常关闭与清理未实现，不能称无线已停或具备上板条件；见 `docs/GR4-GATT-IO.md`。以下为此前阶段记录。

便携GATT只读诊断核心完成：五字段动态句柄计划、服务/特征归属与读取属性校验、完整发现/连接世代约束、严格文本和电源/工作模式参考映射，失败清零/未知状态不放行；无IO/身份认证或运行时调用。新增合成元数据/所有256状态值/文本边界测试，全量33/33 ASan/UBSan（37.35秒）；默认v6.1编译/产物门禁通过，359440字节2fc5a127…未变。首次SDK路径缓存冲突经原SDK reconfigure修复，SDK/BSP未改。既有bd74b761…扫描产物门禁复核通过，本轮无串口连接/复位/烧写/相机操作。独立GATT适配、真实读取/型号版本/配对/功耗仍未完成，见 `docs/GR4-GATT-CORE.md`。以下均为此前阶段记录。

第二次人工启动扫描流程PASS，但实际是相机关机而非蓝牙关闭：用户明确“只关闭了相机电源”。120秒采集4054字节、2连接/273传输错误（含人工断开/重枚举重试），私有 `manual-power-gr4-deadline-radio-off-01.log` SHA256 `56c494bd92b9cc5fb1d305c3712c19e5e4e94f2ad196464bd01923afd35f91c6`。30659ms完整候选id1/services2；30660ms COUNTS reports1581/invalid0/matches50/overflow0/candidates1及END code0/cleanup1；30660–40161ms全部20次一致结果 end=deadline/elapsed_ms30000/inactive1/stats1，同一统计；40661ms应用返回。两次同版人工启动均有界停止/清理通过；服务位变化不证明开关机编码或同一设备，启动头/APP_ENTRY/STARTED仍缺失，不计独立掉电或功耗。真正无线关闭负对照尚未执行，不能标已通过；文件名为原实验意图，证据保留。

用户提供Live-View项目已只读核对：官方FAQ确认GR IV无线开启时关机仍发蓝牙；参考代码在协议发现后通过GATT读取Power/Operation Mode，不通过服务UUID判电源。参考HEAD4d1aa81…工作树含用户修改，不修改/运行其项目，不把历史验证或固定句柄/配对参数搬为Pico已验证协议。详细依据、分层状态/Unknown拒绝放行和后续设计见 `docs/GR4-BLE-REFERENCE-NOTES.md`。本轮采集结束/串口释放，仅日志/公开资料/参考源码/文档，无主机复位/重烧/JTAG/SD/NVS写入或相机连接/控制；主机32组/编译沿用实现轮。以下“蓝牙关闭对照采集”是条件澄清前历史。

相机蓝牙关闭对照采集中：用户在“关闭相机蓝牙后回复准备好了”的提示后回复“准备好了”，以该用户操作确认为实验条件，未独立读取相机设置。120秒不主动复位采集已开启/capture ready，私有 `.cache/pico-gr4-deadline-xsURqd/manual-power-gr4-deadline-radio-off-01.log`；待人工重启同版Pico，无本轮候选/统计/截止/清理结果，不能预先判空扫描或对照通过。不重烧/主机复位/JTAG/SD/NVS写入或相机控制，旧开启蓝牙轮PASS独立保留。

截止修正版首次人工启动扫描流程真机PASS：用户确认“已开机”，120秒采集4054字节、2连接/223传输错误（含人工断开/重枚举尝试，不等同芯片故障），私有 `manual-power-gr4-deadline-01.log` SHA256 `cf8942d84a6416b84fc0819a9da60a507e5612292d8cf3e2d593cd9e9102e964`。30662ms完整候选id1/services1；30663ms COUNTS reports1618/invalid0/matches55/overflow0/candidates1及END code0/cleanup1；30663–40164ms全部20次一致结果 end=deadline/elapsed_ms30000/inactive1/stats1，同一统计；40664ms应用返回。确认独立截止/取消后inactive/host及SDK、PMU/I2C清理路径通过，候选只表示Camera Information协议兼容，不证明当前相机型号/身份或配对。APP_ENTRY/STARTED/启动头未捕获，30秒为应用tick证据，不称独立电气掉电或射频功耗测量。采集结束/串口释放，本轮仅采证与文档，无主机复位/重烧/JTAG/SD/NVS写入或相机控制；主机32组/编译沿用实现轮。下一步相机蓝牙关闭对照、重复会话/退出功耗，Gate A身份与配对仍未通过。以下待启动状态为历史。

用户确认“准备好了”，截止修正版首次120秒不主动复位采集已开启并收到capture ready；私有 `.cache/pico-gr4-deadline-xsURqd/manual-power-gr4-deadline-01.log`。待提示人工拔USB/完整关机/开机并重连；此刻尚无本版扫描/截止/清理结果，不计真机通过。本轮未重烧/主机复位/JTAG或操作相机。

截止修正版已按既有授权直接烧写并扩展回读通过，待人工启动。最终主机32/32 ASan/UBSan（35.62秒），28场景含晚回调禁收/全部20次重播逐行一致；门禁26项、采集17项及默认/专用编译通过。app391568字节SHA256 `bd74b761f79c99a84f7b62f17d26d4aa0dcc265086ab65f1407f956ebdd704d3`，boot21168/table3072，三镜像Hash verified；私有 `.cache/pico-gr4-deadline-xsURqd/post-write-prefix.bin` 4259840字节SHA256 `a8195c2b85845fc29887470841ade107b0d74304bebd081941acda94f7a682c1`，全部精确相符。0x6000–0x7fff/0x8000–0xffff/0x70000–0x40ffff与本轮完整备份相同，不额外擦旧尾部。旧扫描及已验收显示/原厂回滚保留；备份default-reset进入下载器，烧写/回读before/after no-reset，串口释放，新版未运行，无本版候选/30秒截止/退出实机结果。未清SD/擦NVS/写PMU校准/连接JTAG或操作相机；固定SDK及BSP工作树干净。下一步用户就绪→采集就绪→人工启动，不把回读/模拟当Gate A。以下备份中/未写入为本轮历史。

被动扫描独立截止修复已实现：FreeRTOS tick 30秒到期主动取消，核实发现inactive；SDK提前结束/复位/host提前退出、取消失败/EALREADY仍活动保持失败。不修改SDK/连接角色/BSP，无配对/GATT/SD/EPD。结果20次重播增加end/elapsed_ms/inactive/stats/完整聚合统计，安全退出后才取统计。主机32/32 ASan/UBSan（37.15秒），实际入口28场景含无SDK回调/空扫描/tick回绕/非事件唤醒；门禁26项与采集17项、默认/专用编译通过；追加晚回调/20次完全相同断言后全量回归进行中。专用391568字节SHA256 `bd74b761f79c99a84f7b62f17d26d4aa0dcc265086ab65f1407f956ebdd704d3`，默认359440字节2fc5a127…未变/未烧。完整备份 `.cache/pico-gr4-deadline-xsURqd/pretest-flash.bin` SHA256 `b7de1f2cd7675cf9a3c430e3b26384896fa02012c82f97d5c794b82989bd7431`，当前99c…/历史尾部/相同分区/显示及原厂回滚已验证并保留精确旧镜像；设备已进入下载器，待写入/回读，新版未运行。以下“修复未实施”为历史。

被动扫描首次人工开机已确认运行，但整轮未通过：用户“已开机”，另确认相机处于待配对状态。本版99c1109a…首次120秒采集0字节/1连接/0错误（文件12:03:20创建，USB节点12:09:39），窗口与操作可能错开，不能据空日志判启动失败。无复位120秒补采1569字节/1连接/0错误，私有 `manual-power-gr4-scan-late-01.log` SHA256 `f8aaf4b35fab25f0a3b5457c92ab33b4194aa28be2d7e1bb619a970b5d5a7473`：35656ms候选id=1/services=1半行，36707–45207ms18次一致 `code=9 cleanup=1 passive=1 connect=0 pair=0`，45707ms应用返回。仅证明至少一个Camera Information协议候选及已实现清理分支通过，不证明型号身份、30秒正常结束、完整统计或全部20次重播；APP_ENTRY/STARTED/COUNTS/END未捕获。两次采集结束、串口释放，本轮未复位/重烧/JTAG/SD/NVS写入或相机操作，主机32组/编译沿用实现轮。

源码核对定位固定SDK的Observer-only结束问题：`nimble_opt_auto.h`将NIMBLE_BLE_CONNECT定义为Central或Peripheral，而 `ble_hs.c` 的正常同步态 `ble_gap_timer()` 调用放在 `#if NIMBLE_BLE_CONNECT` 内；本版只Observer、传统扫描，SDK时长到期处理因此不执行。收到候选后约35秒触发应用等待失败，与日志吻合，不是相机拒绝配对。未修改SDK或启用连接角色。下一步在相框适配层新增独立30秒截止/显式取消及停止确认，覆盖此配置的测试和细分结束证据，然后编译/备份/烧写复验；目前未实现，不能把code9改判成功。相机待配对即可，仍不发起配对。下文待人工开机为阶段历史。

用户确认“准备好了”，被动扫描首轮120秒无软件复位采集已开启并收到capture ready，私有 `.cache/pico-gr4-scan-bQRWDb/manual-power-gr4-scan-01.log`。待提示人工拔USB/完整关机后开机重连；当前无本版应用/扫描/候选/退出结果，不计真机通过。本轮未烧写/主机复位/JTAG或相机控制。

Pico独立有界被动扫描已实现/测试/编译/授权直接烧写并扩展回读，待人工启动。每会话30秒，仅Observer；无主动scan request/连接/配对/GATT/SD/EPD，最小PMU握手、不改VCOM。主机CTest32/32（ASan/UBSan，36.51秒），实际入口22种模拟生命周期、门禁26项/采集器17项、默认/专用v6.1编译通过。SDK stop内部无期限等待放独立任务，主任务等待10秒；失败保留被引用对象、cleanup=0，不强杀/复位或宣称无线已停，初始化失败也不假定嵌套回滚成功。只输出临时编号/服务/聚合统计，候选不证明型号，地址会话内RAM去重。

当前专用app390480字节SHA256 `99c1109ad60730d921e868a21cd43daeff803bfe7baf945b1aaf27b367d68d33`；完整16MiB备份 `.cache/pico-gr4-scan-bQRWDb/pretest-flash.bin` SHA256 `4308468a3a7306026bae611c3799b85fca0c8451a5710b6319d4acfc805053f6`，已验收13117988…显示版/已知尾部/分区/原厂回滚核验并保留精确镜像及原始分区。首写ae0358…未运行，回读核验后收紧SDK初始化失败cleanup状态，修订测试/编译通过再替换；最终三镜像Hash verified，0x410000回读4259840字节完整相符，SHA256 `abe30d07bf0df5807c8f48b8d4e577ac09328eb209cc62710dd1359bb96f69aa`。0x6000–0x7fff/0x8000–0xffff/0x70000–0x40ffff保护字节未变，新app之外旧尾部保留，不额外擦除。备份default-reset进入下载器，烧写/回读均no-reset；工具退出、串口释放，设备留下载器，新应用未运行，无真机BLE扫描/发现/退出结果。不清SD/擦NVS/写PMU校准/相机原片；无线PHY初始化仍由SDK执行，但本模式关闭校准结果NVS持久化。默认359440字节2fc5a127…未烧。详见 `docs/GR4-PASSIVE-SCAN.md`；下文“设备保持显示版/无适配”为历史。

GR IV广播候选解析基础完成：严格31字节TLV与失败清零，两个公开UUID、会话内8候选去重/合并/容量限制/饱和计数，不输出或持久化地址，不证明型号。主机CTest31/31（ASan/UBSan，35.54秒）、默认ESP-IDF v6.1新组件编译通过；默认app356560字节SHA256 `508870f7e407d917307304115c4f55239fdc937b6f8b82e09b9d39b01a6112ae`未变。尚无无线适配/运行时调用，本轮未连接设备/扫描/配对/烧写/SD写入，设备保留已验收13117988…显示固件。下一步独立有界被动扫描，见 `docs/GR4-ADVERTISING.md`。下文为此前显示阶段记录。

JPEG生成缓存只读SD三图实屏验收通过：针对本轮比例/铺满/竖条/残影确认问题，用户回复“没有问题”。结合本版mask7/code0、20次一致清理后结果及应用返回，指定三张样片完成设备SD JPEG解码/Cover生成与缓存索引提交→严格已有源/缓存/索引人工启动恢复→只读SD加载/GC16→本轮视觉确认闭环。此验收独立于旧cache-ram-test视觉结果，首图单张串口日志不完整的事实保留。设备保持13117988…显示版及第三张画面；本轮仅记录用户验收，无新代码/编译/连接/复位/烧写/清卡或相机操作。首次源独占创建的独立真机证据、正式产品导入、压力/拔卡/满卡/长期可靠性和GR IV链路仍未完成，不能以指定样片闭环宣称产品已完成。下文“视觉待确认”为收到本轮反馈前的历史。

JPEG生成缓存只读SD显示首次人工启动整轮运行PASS，视觉待用户确认。首窗120秒138字节/2次连接/499次传输错误，仅18284ms READY；无复位120秒补采2924字节/1次连接/0错误，SHA256 `6d8a56c6a438baa187d79f42f5d2c68debc56caeee2b50f8f61ee6c4bc00ffc3`，私有 `manual-power-jpeg-cache-display-late-01.log`。图1仅24436ms READ_OK半行，单张刷新日志未捕获；图2 61927ms完整READ_OK/read6158ms、63269ms GC16 result=0；图3 99435ms READ_OK/read6166ms、100777ms GC16 result=0，均后两图31°C/copy299ms/refresh459ms。100777ms END r=1 mask=7 code=0，100777–110278ms全部20次一致总结果，110778ms应用返回。结合已核验代码mask只在每图完整加载/门禁/刷新成功后置位，确认三图流程全通过；不宣称首图完整单张日志取得，不把20次重播当20轮显示。不创建/解码/修复/提交/嵌入兜底，第三张保留。两次采集均结束、串口释放，本轮只采证/文档，无主机复位/烧写/JTAG/清卡/校准或相机操作，主机30组与编译沿用上轮；启动头缺失，不证明独立电气掉电。新库比例/铺满/竖条/残影视觉待本轮反馈，不沿用旧RAM库验收；下文补采中/待启动为阶段历史。

JPEG生成缓存SD显示首轮人工启动已收到应用预检查通过，整轮显示待核实：用户确认“已开机”，首个120秒私有 `manual-power-jpeg-cache-display-01.log` 138字节、2次连接/499次传输错误（含人工断开/重枚举尝试），SHA256 `207e9050d19145d5a1cf1538611bd30ff98824080b8ba22f347b06e9eb3c1a49`；仅18284ms JPEG_CACHE_DISPLAY_READY sequence=6 source=existing-only。此分支确认三源/生成缓存/索引严格预检查通过，不确认后续暂存读图/刷新总结果。已开启无复位120秒 `manual-power-jpeg-cache-display-late-01.log` 补采，暂取得24436ms图1 READ_OK半行；不能将部分读取日志当实屏或三图PASS。不重烧/主动复位/JTAG/清卡，等待补采完整结果及用户视觉反馈；下文待人工启动为阶段历史。

用户确认“准备好了”，JPEG生成缓存SD显示首轮120秒被动采集已开启并确认 capture ready。私有日志 `.cache/pico-jpeg-cache-display-gkvrVA/manual-power-jpeg-cache-display-01.log`；待人工拔USB、完整关机后开机重连，无本轮主机主动复位/烧写/JTAG，显示结果尚未取得。先严格恢复预检查，再三图SD读取/GC16，前两张30秒停留，第三张保留，实屏效果待用户确认。

JPEG生成缓存只读SD显示版已直接烧写并扩展回读通过，待人工启动。专用app3189776字节SHA256 `13117988f798cca26aff730a7028d26d4cea4fd77bdc0669804a5b9e5618a105`；完整16MiB私有备份 `.cache/pico-jpeg-cache-display-gkvrVA/pretest-flash.bin`（103.8秒）SHA256 `465b12d6e91265749c1b1dda36049537e15a883650e9acc7e88f77c65ea7ecd3`。六素材/当前恢复镜像/已知混合尾部/分区/原厂回滚核验通过。首次提取检查因4MiB原始应用分区含已知旧尾部而拒绝（当时未烧写）；保留原始分区，另按已完整匹配的91830…有效镜像精确边界提取 `rollback-recovery-app.bin` 并验证校验和/摘要，不放宽通用镜像检查。旧boot/raw app、有效恢复镜像和原厂回滚保留。

三镜像Hash verified；0x410000扩展回读4259840字节（24.5秒），boot21168/table3072/app3189776逐字节一致，SHA256 `ab26083c31672fe817e783663f2cdd2f9ab3e6538fa850d08df85cf9da5529ca`。0x6000–0x7fff/0x8000–0xffff/0x31b000–0x40ffff与本轮备份一致；新app较上一代生成状态版小，0x31b000–0x31bfff已知历史尾部继续保留不额外擦。备份进入下载器使用default-reset，烧写/回读before/after均no-reset；工具退出、串口释放、设备留下载器，新显示应用未运行，无本版SD读取/GC16/实屏结果。不清SD/擦NVS/改校准/操作相机或连接JTAG。下文备份中与旧恢复记录为阶段历史；先用户就绪→开启采集→提示人工开机，不沿用旧RAM缓存视觉验收。

已有JPEG生成缓存只读SD显示已实现：严格恢复预检查→固定jpeg-cache-test每张暂存/头CRC/完整像素/绑定/元数据复核→原有像素映射/卡与电源温度门禁/GC16，前两张30秒、最后图3保留；不解码/准备/修复/提交或嵌入兜底。释放缓冲后20次结果重播，失败停止后续图。最终主机30/30 ASan/UBSan（44.21秒）、固件门禁24项、采集器16项、指定三图主机已有库恢复+三暂存加载PASS，默认/专用v6.1编译通过。专用3189776字节SHA256 `13117988f798cca26aff730a7028d26d4cea4fd77bdc0669804a5b9e5618a105`；默认356560字节SHA256 `508870f7e407d917307304115c4f55239fdc937b6f8b82e09b9d39b01a6112ae`未烧。旧恢复/生成产物保留。按既有授权备份当前固件中，尚未烧写/执行新版显示；真机与视觉待验，详见 docs/JPEG-CACHE-DISPLAY.md。

- [x] 生成缓存只读SD显示实现、真实入口与文件加载故障测试、默认/专用编译与门禁。
- [x] 本轮完整备份/回滚核验、直接烧写及扩展回读。
- [x] 新显示版人工启动三图SD加载/GC16总结果：mask7/code0、20次一致重播、应用返回；首图单张日志不完整，整轮由总状态核实。
- [x] 本版用户比例/铺满/竖条/残影视觉验收：用户针对本轮回复“没有问题”。
- [ ] 正式产品导入、首次源创建独立证据、压力/拔卡/满卡/长期可靠性与GR IV链路。

本轮120秒采集已结束：2025字节、2次连接、206次传输错误（含人工断开/重枚举重连尝试，不等同芯片故障）；最终日志摘要与下述SHA256一致，工具退出、串口无占用。完整PASS已取得，不追加JTAG查询。

严格只读 JPEG 恢复首次人工启动真机 PASS：采集先就绪，提示拔 USB、完整关机后开机重连，用户确认“已开机”。18292ms `JPEG_RECOVERY_PASS sequence=6 source=existing-only`，18293ms END；18293–27793ms 全部20次结果均 `r=1 done=1 mask=7 seq=6 code=0 read_only=1`，28293ms应用返回。已有三 JPEG、生成缓存完整比较与固定索引/绑定/相同有效双快照/空日志、源再比较和最终元数据复核通过；未创建/补写/修复/解码/提交或换图。私有日志 `.cache/pico-jpeg-recovery-2fT36M/manual-power-jpeg-recovery-01.log` 2025字节，SHA256 `db050ec5a1b09fe8989a079b1d775d1c6303a884f111978e9db93758df04e6dd`。本轮仅采证/文档，无主机主动复位/烧写/JTAG/清卡/校准或相机操作，主机29组与编译沿用上轮；启动头未捕获，不证明独立电气掉电、首次源创建、长期可靠性或新生成缓存显示。下一步接已有生成缓存只读 SD 三图显示。下文待启动记录为烧写阶段历史。

JPEG/生成缓存/索引严格只读恢复版已实现、测试、编译、直接烧写/扩展回读，待人工启动。固定已有jpeg-source-test与jpeg-cache-test：三源完整比较→正确绑定/相同有效双快照/空日志/三KEPT/照片序号1/global6→三缓存完整比较→源再比较→最终元数据复核；缺失或损坏拒绝，不创建/补写/修复/解码/提交/换图。独立56字节恢复状态，启动清零，释放内部RAM缓冲后发布结果并重播20次；旧生成模式16字查询工具不适用本版14字状态，不连接JTAG。主机29组ASan/UBSan、固件门禁23项、采集器15项及默认/专用ESP-IDF v6.1编译通过，指定三图主机已有库严格恢复PASS（非设备证据）。详见 docs/JPEG-RECOVERY.md。

专用app3180064字节 SHA256 `91830c393633d51a65e596fbdc99591f4bd198a9925294d78e3df41849d21326`，匹配ELF `3fc94cfc 00000038 D frame_jpeg_recovery_status`；默认回归app356672字节 SHA256 `eef4e64edfde8f20e962645cf01b98fbee0c22ba74d9080efbdf63c72808361e`未烧，旧build-jpeg-status f34b…未改。完整16MiB `.cache/pico-jpeg-recovery-2fT36M/pretest-flash.bin` SHA256 `da72bff70fe64e2be4844b0601f6b6325553e52ff81e405e889c4cf766c21368`，六素材/56字节状态/当前旧JPEG镜像/混合尾部/分区/原厂回滚核验通过，旧boot/app回滚副本保留。三镜像Hash verified，0x410000回读4259840字节逐字节相符，SHA256 `be9ff453f6ca86e7cc69c5a1da86f7d6520aaf8b7e623cb37bcc4761372976ad`；0x6000–0x7fff/0x8000–0xffff/0x319000–0x40ffff保护字节未变。新app较小，0x319000–0x31bfff历史尾部原样保留，不额外擦除。备份进入下载器使用复位，烧写/回读before/after no-reset；工具退出、串口释放、设备留下载器，新只读应用尚未运行，无本轮SD/JPEG/显示结果，不清卡/擦NVS/改校准/操作相机。下文旧生成PASS及待启动记录为历史，不计本版跨关机恢复。

- [x] JPEG源/生成缓存/固定索引只读恢复、读中变更门禁、清理后结果重播。
- [x] 主机故障/回归测试、默认/恢复专用编译与禁止写入入口门禁、完整备份/烧写/扩展回读。
- [x] 真机人工关机后严格恢复PASS source=existing-only；20次一致结果与应用返回，不换图为预期。
- [x] 已有生成缓存只读加载接入SD显示，后续本版运行与用户视觉验收通过；首次源独占创建、压力/拔卡/满卡、产品导入/相机链路仍另验。

JPEG状态版首次人工启动整轮真机PASS：采集先就绪，提示拔USB、完整关机／开机重连，用户确认“已开机”。120秒私有 `manual-power-jpeg-status-01.log` 2503字节、2次连接／209次传输错误（含人工断开／重枚举重连尝试），SHA256 `333c1f33c64a2443e10eea0c312b29485d88d145bc220a48812088964f8bafba`。三GENERATED source=SD：12336ms图1 1562×1600/11023ms，23582ms图2 1571×1600/11245ms，34331ms图3 1571×1600/10748ms，均scale=0。43663ms JPEG_SAMPLES_PASS sequence=6 library=existing；43664ms END后20次JPEG_RESULT全部 r=1 done=1 mask=7 commit=1 seq=6 origin=2 code=0，53664ms应用返回。确认本次三源完整比较／SD解码／Cover生成／参考包完整比较／源再比较、生成缓存准备及完整比较／索引提交／checkpoint／同进程重开／清理后总结果通过。library=existing不证明旧版整轮PASS或本轮新建源；准备流程可能写文件/checkpoint，不计严格只读跨关机恢复，也无本版显示验收。启动头仍缺，不证明独立电气掉电。采集结束、串口释放，本轮无主机复位／烧写／JTAG／清卡／校准或相机操作；主机27组及编译沿用上轮。下一步实现严格只读已有JPEG源／生成缓存／固定索引恢复，验证后再接生成缓存SD显示；下文待启动为烧写阶段历史。

JPEG可查询结果版已完成实现、测试、编译、直接烧写与扩展回读，待人工启动。内部DRAM64字节每次应用进入清零，记录三图进度／实际提交序号和库来源／首错阶段偏移，媒体关闭与缓冲释放后最后发布done；20次500ms结果重播只输出，不重跑IO、不新增SD/NVS结果文件。修正报告切换时沿用旧图片序号，入口测试覆盖发布/清理/重播。最终主机27/27（ASan/UBSan，31.14秒）、Python固件门禁22项、离线解码7项含Tcl故障/非法地址测试、采集器14项通过；默认/专用v6.1编译通过，指定三图主机已有库完整流程PASS（非设备证据）。专用app3194784字节 SHA256 `f34b1213b37a5cde769ade9e7ee64cfb8f49354aaa96215d4509204f2aa8b57e`，状态符号 `3fc94cfc 00000040 D`；默认回归356672字节 SHA256 `980e868465cc58ddbc18e275188a235e4629e5d07b1fdb5bd953cb2e9e2def2b`未烧，旧build-jpeg-samples fe48…未改。

完整16MiB备份 `.cache/pico-jpeg-status-BFZwdX/pretest-flash.bin` SHA256 `86683a75e8cfb7b185e0ad3c84b46b1883e53884652bcc40ccc2983a5812a0a4`，六素材／状态布局／旧JPEG固件／混合尾部／分区／原厂回滚核验，旧boot/app副本保留。三镜像Hash verified；回读0x410000（4259840字节）boot21168/table3072/app3194784逐字节相符，SHA256 `acfbd9ad48a2f71c4f1d899e4114b2a3484175b8c2b1e8e212315177d22369d3`；0x6000–0x7fff/0x8000–0xffff/0x31c000–0x40ffff保护字节未变。备份进入下载器使用复位，烧写/回读before/after no-reset；串口释放、设备留下载器，新应用尚未运行，无本轮应用SD/JPEG/显示结果、无清卡/NVS擦除/校准/相机操作。新JTAG状态脚本仅主机fake target验证，尚未连接真机；它会暂停双核，不能当零干扰观察。详见 docs/JPEG-STATUS.md；下文旧版记录为阶段历史。

- [x] JPEG内部DRAM状态／首错／清理后发布和有界结果重播。
- [x] 主机27组、固件门禁与默认/专用编译；完整备份、直接烧写、扩展回读。
- [x] 新版人工启动三图生成／索引提交总结果：library=existing，20次清理后PASS结果及应用返回；不换图为预期。
- [ ] 可选受控RAM状态查询：本轮完整串口结果已取得，无需查询；脚本尚未真机验证，不复位或设置断点。
- [ ] 严格只读跨关机JPEG/生成缓存恢复与生成缓存SD显示，正式产品导入和GR IV链路仍未接。

JPEG第二次人工启动对照已取得应用运行／局部生成证据，整轮仍待核实：采集就绪后提示拔USB→完整关机／开机→重连，用户确认“已开机”。120秒 `manual-power-jpeg-samples-02.log` 191字节、2次连接／212次传输错误（含断开／重枚举重连尝试）：12324ms第一张GENERATED source=SD decoded=1562x1600 scale=0 elapsed=11011ms，随后第二张BEGIN bytes=564112。30秒无复位补采 `manual-power-jpeg-samples-late-02.log` 64字节、1次连接、0错误，仅23566ms第二张GENERATED source=SD前缀，半行结束。结合实际分支，第一张完整源校验／SD解码／Cover生成／完整参考比较及源再比较通过，第二张到达生成成功日志；源是否本轮新建未记录。无第三张、库来源、总PASS／FAIL／END，不能将两张生成当三缓存及索引提交完成，也不据缺日志认定失败或卡住。两个窗口结束、串口释放，本轮仅采证／文档，无主动复位／烧写／实现修改；主机25组和编译沿用上轮。已证实本轮进入应用，不再重复相同开关机；下一步补可查询结果／严格只读检查已有产物，核实第三张及提交。SDK非阻塞USB输出有50ms满FIFO丢字符机制，列为日志遗漏候选，不证明本次根因。下文首次无应用日志／烧写待启动为历史。

JPEG诊断首次人工启动未取得应用结果：先采集端口就绪再提示电源操作，用户确认“已开机”；120秒私有 `manual-power-jpeg-samples-01.log` 1337字节、2次连接／33次传输错误（含重枚举重连尝试），镜像加载后最后626ms进入PSRAM调校。随后10秒无复位补采43字节、1次连接、0错误，仅633ms `Found 8MB PSRAM device`，说明不能认定停在调校入口；补采可能读到滞留日志，不证明当前CPU位置或完整PSRAM测试。两次都无ROM复位原因、APP_ENTRY、SD mounted或JPEG结果，不认定JPEG／缓存失败或PASS。两个采集结束、串口释放，固件仍fe48d59c…，实时执行位置与SD状态未知；本轮仅采证／记录，无新实现／烧写／主机主动复位／相机或校准操作，不改时序／不清卡。主机25组及编译沿用上轮。下一步先采集，再做断开USB的完整关机启动对照，应用启动证据补齐前不继续JPEG／显示验收。下文“待人工开机／下载器”是烧写阶段历史。

设备 JPEG 落盘／解码与缓存生成诊断已实现、验证、按已有授权烧写及扩展回读，待人工开机：独立 jpeg-source-test／jpeg-cache-test，私有三图缩小副本与新参考包；内部DMA RAM缓冲独占写入，源完整比较→SD解码→Cover生成→完整参考比较→源再比较→设备生成缓存持久化／KEEP／checkpoint／同进程重开。失败保留，不刷新照片／不操作相机；不是正式PFU1／PFI1产品导入。主机25组（ASan/UBSan）、Python门禁22项、默认及JPEG专用v6.1编译通过；指定三图主机首次建库及已有库重开PASS。专用app3193520字节／SHA256 `fe48d59ce71e74565ee2b841c13bbf0b471c584d12871d8cb84460116a52f50c`；默认回归app356672字节／SHA256 `29004f6d172f780b5af5b5fc1a6a008600e0985d46f1c3d5c4afbbfef29f378f` 未烧。完整16MiB `.cache/pico-jpeg-samples-qJO6sw/pretest-flash.bin` SHA256 `cb8a44325a125f5920d478d8a65a312f32d94ca526110f04a0bbaf7fc03ba3ba`，六素材／旧显示固件／混合尾部／分区／原厂回滚核验通过，旧boot/app副本保留。三镜像Hash verified，扩展回读0x410000（4259840字节）三镜像逐字节相符，0x6000–0x7fff／0x8000–0xffff／0x31c000–0x40ffff保护字节未变。备份进入下载器使用复位，烧写／回读before/after no-reset；设备留下载器、串口释放，尚未执行新JPEG诊断或SD写入。主机采集器补JPEG标记并修正子串误记显示完成，14项循环测试通过。下一步先采集、再人工开机，检查三GENERATED和PASS；屏幕不换图为预期。此前SD显示验收保持通过，不沿用为JPEG证据，旧异常根因及相机链路待验。详见 docs/JPEG-CACHE-SAMPLES.md。

- [x] JPEG 诊断源落盘与生成缓存模块、独立目录／无覆盖／失败保留、实际入口测试。
- [x] 默认与专用固件编译及互斥／入口／素材范围门禁。
- [x] 本轮完整备份／回滚核验、烧写与扩展回读。
- [x] 首次真机三 JPEG 源完整验证／SD 解码／完整参考比较／缓存提交 PASS；源本轮是否新建未记录，不能宣称首次源落盘已经单独验证。
- [x] JPEG版应用运行证据：第二次人工启动取得第一张完整生成与第二张成功分支前缀；启动头仍缺，不证明独立电气掉电。
- [x] 新JPEG状态版三图／索引总结果：三完整GENERATED、library=existing、20次相同总PASS及应用返回；旧版截断日志的整轮结果仍不反推。
- [ ] 三JPEG源首次独占创建／写入的独立真机证据；本版可复用已有源，不把本轮完整验证当全部新建。
- [ ] 严格只读跨关机 JPEG 与缓存恢复，再接生成缓存 SD 显示。

已有RAM缓存只读SD三图显示实机验收通过：用户本轮明确确认“三张图片比例正常，铺满屏幕，没有竖条或者残影”。结合同轮三个READ_OK source=SD、三个GC16 result=0及SAMPLES_END，完成指定三张样片从已有SD缓存读取／校验／全屏等比裁切显示的验收；首次RAM写入完整比较与人工关机后严格恢复此前已通过。此次仅更新进度与验收文档，设备保留3688b351…SD显示版。后续开发先接设备JPEG解码／源文件落盘与图像缓存生成，再推进GR IV相机链路；读失败保持画面、拔卡／满卡／长期压力仍需真机测试，旧cache-test损坏根因继续保留待查。以下“视觉待确认”记录为本轮反馈前历史。

已有RAM缓存只读SD三图显示首次人工启动运行通过，视觉待确认：先采集端口就绪，再提示人工电源操作，用户确认已开机；120秒私有 `manual-power-sd-cache-ram-display-01.log` 5739字节、2次连接／30次传输错误（含USB重枚举重连尝试），APP_ENTRY sd-cache-ram-display（566ms）、SD mounted（707ms）、PMU ready（776ms）。11136ms DISPLAY_READY sequence=6 source=existing-only，三张SD_CACHE_READ_OK读取6546／6542／6545ms，SAMPLE_RESULT均result=0、temperature=31°C，像素复制299／300／300ms、刷新401／459／459ms；94739ms SAMPLES_END、94740ms应用返回，最后保持图3。三缓存严格预检查、逐张SD暂存读取与校验及有序GC16执行通过，不能沿用此前嵌入Cover视觉验收；本轮比例／铺满／灰阶／竖条待用户目视确认。采集结束、串口释放，本轮仅采证／文档，无新实现／主机主动复位／烧写，运行路径无SD准备／补写／修复，旧异常cache-test保留。沿用上轮主机23/23、Python门禁21项及默认／专用编译结果。设备现运行SD显示版；旧损坏根因、长期可靠性、设备JPEG解码及相机链路仍待验证。以下烧写待启动为历史。

已有 RAM 缓存只读SD显示版已接续完成、验证、授权烧写及回读，待人工启动：先严格恢复三缓存，再逐张从SD加载到PSRAM，头／完整CRC／库绑定／像素全比较／读后元数据通过才映射到屏幕并GC16，1216×684 Cover、前两图各保持30秒、最后保留图3；失败停止且释放暂存区。主机最终CTest23/23（ASan/UBSan，32.07秒），新增实际display_samples入口测试，模拟SD像素区别于嵌入包，验证预检查／分配／逐张读取／复制／拔卡／电源／刷新失败时停止、释放及电源配对；文件测试覆盖暂存容量／重叠／介质失效／读中元数据变化。Python门禁21项，默认及专用v6.1编译通过；本轮确认构建无源码待编、专用门禁及ELF三素材一致，沿用中断前已完成的测试和编译。专用app1608400字节／SHA256 `3688b35196d4bf63985856b7a9a625601fa761ab45d67f9b58cb081fd55cd845`；默认回归app356672字节／SHA256 `e5b2f30f71dd6cd030c3719b0bd7387bca5ccde147161c923bdd11e4f8b8f2ec`未烧写。完整16MiB备份 `.cache/pico-sd-cache-ram-display-CBZz6s/pretest-flash.bin` SHA256 `76b2bdf19512e21be578d8a271b36a935e22c4101f58088b1d8c8a50b58b3860`，旧只读恢复app／混合尾部／分区／原厂回滚核对通过，回滚副本保留。三个镜像Hash verified，2MiB回读逐字节一致；0x6000–0x7fff／0x8000–0xffff／0x199000–0x1fffff保护字节未变，0x199000–0x199fff历史尾部保留。首次备份进入下载器使用复位，烧写／回读before/after no-reset；设备留下载器、串口释放，新版尚未运行。下一步用户就绪后先采集，再提示人工开机，确认三个SD_CACHE_READ_OK／SAMPLE_RESULT及SAMPLES_END，再做视觉验收。本轮未运行SD准备、未清理旧异常文件，设备JPEG解码／相机链路及旧损坏根因仍待验证。下文为历史。

严格只读 RAM 缓存首次人工关机后恢复 PASS：端口采集就绪后用户确认已开机；120秒私有 `manual-power-sd-cache-ram-recovery-01.log` 3976字节、2次连接／33次传输错误（含重枚举重连尝试）。APP_ENTRY sd-cache-ram-recovery（571ms）、SD mounted（713ms）、PMU ready（782ms），11159ms RECOVERY_PASS sequence=6 source=existing-only，11160ms应用返回。正确绑定、相同有效双快照、空日志、三已知KEPT／固定序号、三包完整比较及读后元数据复核均通过；恢复路径无创建／补写／checkpoint／换图，旧异常cache-test保留。采集结束、串口释放，本轮仅采证／更新文档，无主机主动复位／烧写、相机操作或实现改动，主机22/22与固件编译结果沿用上轮。支持本次用户确认的人工关机后已有缓存恢复，不证明独立电气掉电、长期可靠性、旧损坏DMA根因或设备JPEG／相机链路；SD读后显示闭环仍未验。下一步将已验证的cache-ram-test只读读取接入暂存门禁→GC16三图展示，不补写／修复或使用嵌入图兜底；多轮恢复／压力测试另验。设备现运行只读恢复版，下文待启动记录为历史。

严格只读 RAM 缓存恢复版已实现、测试、授权烧写／回读，等待人工开机：仅检查已有 cache-ram-test，要求正确绑定、相同有效双快照、空日志、三已知 KEPT／photo sequence=1／global sequence=6 和完整逐字节缓存比较；读完再次检查元数据，禁止 mkdir／建库／补文件／提交／checkpoint／换图，旧异常 cache-test 保留。主机 CTest 22/22（ASan/UBSan，最终31.79秒），覆盖幂等读取、缺目录／文件／快照、坏单槽／相同坏双槽、错误序号／状态／ID、非空日志、未知条目／硬链接、介质失效和读中元数据改变，失败内容摘要不变；Python 固件门禁20项通过，默认及只读 v6.1 编译／门禁通过。只读产物拒绝准备／索引提交入口，三份 ELF 嵌入素材与可信私有包完全一致。只读 app1596560字节／SHA256 `70974a15023878fe1458bd93230d7c907cfc5e51fd94b5fb0f156e2cb00e7040`；默认回归 app356672字节／SHA256 `42c5ab8c6e91d886f445c0ab68d993942f5eefc3107de6d2ab4df2e051310b83` 未烧写。首次整片读取约78%串口中断，无完整备份、未烧写；保留失败日志，不主动复位重试完整16MiB备份成功，`.cache/pico-sd-cache-ram-recovery-0OeYZS/pretest-flash.bin` SHA256 `ebe3c0dfe6dda83166e0f1e6df24b1a0aad3e805d76d1e7a7f1a04c492e8d9d2`，旧RAM应用／混合尾部／分区／原厂回滚核验通过，回滚副本保留。三个镜像Hash verified、2MiB回读逐字节一致，0x6000–0x7fff／0x8000–0xffff／0x196000–0x1fffff保护字节未变，历史app尾部仍保留、不额外清Flash。首次备份进入下载器使用复位，重试／烧写／回读before/after no-reset；设备留下载器、串口释放，新版尚未启动，未执行SD补写／修复／照片刷新／相机操作／校准或NVS擦除。下一步先采集再人工关机开机，期待 RECOVERY_PASS source=existing-only；跨关机缓存恢复与旧DMA根因仍未关闭，不沿用上轮created PASS。

独立 RAM 缓冲实验首次人工开机真机 PASS：先采集端口就绪，用户确认已开机；120 秒私有 `manual-power-sd-cache-ram-01.log` 4024 字节、2 次连接／29 次传输错误（含重枚举重连尝试），捕获 APP_ENTRY sd-cache-ram-probe（566 ms）、SD mounted（707 ms）、PMU ready（798 ms）。1006 ms RAM_BUFFER internal=1 dma=1 alignment4=1 bytes=1024，20415 ms RAM_PASS sequence=6 source=created／应用返回，三份缓存完整字节比较及同进程索引提交／重开通过，无照片刷新。旧异常 cache-test 不覆盖／修复，本轮无主机复位／烧写／相机操作，采集结束、串口释放。主机 22/22、Python 门禁 19 项及默认／专用编译为上轮结果；本轮仅采证及更新文档，不新增实现。不能据此证明旧损坏 DMA 根因、物理掉电持久化、源 JPEG 保存或 SD 显示闭环。下一步先做关机后已有三缓存／固定序号恢复验证，必须区分 source=existing 与重建，不将 created 当跨重启通过；当前实验版仍在设备。以下待开机记录为历史。

独立 RAM 缓冲写卡实验版已实现、测试、授权烧写及回读，等待人工开机：只在 `/frame/cache-ram-test` 使用经内部 RAM／DMA 能力／4 字节对齐检查的 1024 字节缓冲写入三份 PFG1，同步关闭、完整逐字节比较后才 KEEP／checkpoint／重开；旧异常 `/frame/cache-test` 与旧 storage-test 保留，失败不覆盖，本轮不刷新照片。主机 CTest 22/22（ASan/UBSan），含容量／重叠拒绝、三包读回、幂等重开和坏文件保留；固件门禁 Python 19 项通过，默认及 RAM 专用 v6.1 编译／门禁通过，三份 ELF 嵌入包与可信私有素材完全一致。专用 app 1601216 字节／SHA256 `f0253490d0c43bbfae5c40e7c5064842c90f101087eefc9aa1ef6fa48252fdae`；默认回归 app 356672 字节／SHA256 `d80025295fd70fbddf6323d87204d4f9e196aa876f4a410b70e442eb95c821d5` 未烧写。新完整 16 MiB 备份 `.cache/pico-sd-cache-ram-Ug4Qet/pretest-flash.bin` SHA256 `a30fbd05b0f8e11cf12b8c11dd191cb767553fba320a88e9e47d4020ce8ed380`，旧只读应用、混合尾部、分区和原厂回滚核验通过，回滚副本保留。三镜像 Hash verified，2 MiB 回读逐字节一致，0x6000–0x7fff／0x8000–0xffff／0x197000–0x1fffff 保护字节未变；0x197000–0x199fff 仍有历史 app 尾部，不额外清除。设备留下载器、新版未启动，串口释放；备份进入下载器使用复位，烧写／回读前后 no-reset。无本轮应用 SD 写入／修复／换图、NVS 擦除、VCOM 校准或相机操作。下一步就绪后先采集，再提示人工开机；RAM_PASS 仅表示同进程完整比较和事务重开，不证明掉电持久化或旧损坏 DMA 根因。

第十轮只读版首次人工启动完成：三项检查有序结束，确认首张缓存整体内容异常、后两张文件缺失，未刷新／修复。120 秒私有采集 `manual-power-sd-cache-inspect-01.log` 4529 字节、2 次连接／29 次传输错误（含重枚举重连尝试）；APP_ENTRY inspect-only（564 ms）、SD mounted（705 ms）、PMU ready，首图检查 6815 ms 完成：size=415952、first_diff=0、header_valid=0、header_mask=0x2fff、payload_complete=1，完整 payload 实际 CRC=9cb5ea2f／预期=53f23b87。图2／3分别 6915／7016 ms cache-stat errno=2（未找到），7017 ms INSPECT_END／应用返回。ok=1 仅证明检查完成，不是缓存正确；实际 CRC 不等于全零51f30ab4或全白f6dc332f。SDK v6.1 本地源码显示 SDMMC 缓冲检查仅做对齐，aligned 且非 PSRAM 的源会走直接 DMA；准备代码 fwrite 源来自 Flash 嵌入包，列为待验证的 DMA 可达性候选，不宣称根因已证实。下一步在不覆盖原异常文件的独立测试范围验证 RAM 缓冲写入／回读并调查源指针路径，不能靠删除／重写出图关闭问题。本轮无主机主动复位／烧写／SD 修复或照片刷新，采集结束、串口释放；应用只读边界不等于 BSP 整机硬件写保护。以下待启动记录为历史。

第十轮只读差异检查版已实现／验证／授权烧写／回读，等待人工介入：专用 inspect-only 配置仅检查现有固定目录／库绑定／三个缓存，输出首个差异的文件字节偏移、头有效性／字段分组 mask、完整 payload 标记和实际／预期 CRC；缺失或不符不创建／补写／提交／修复／刷新照片。主机 CTest 22/22（ASan/UBSan），覆盖完整相同、像素／magic／合法错布局差异、读中介质失效、缺目录和文件不创建；固件门禁 18 项 Python 测试，要求专用模式并拒绝准备／索引提交入口。首次 MCU 编译发现路径格式截断警告，补显式长度检查／足够容量后默认及只读 v6.1 编译／门禁通过。只读 app 1593312 字节／SHA256 `6041aa2e020cc54ca2777f82247b7f9855370ee92aa264abd6881494b58108b2`；默认回归 app 356672 字节／SHA256 `325ee1a2c4c032b3637eb597ecc114cc8fe26e1d195302b9b4bd01389c021e5f`，未烧默认版。新完整 16 MiB 私有备份 `.cache/pico-sd-cache-inspect-xcE4mA/pretest-flash.bin` SHA256 `09424c4d3c9e8243f0a2ce6cf72f2a98c3ee5751981dc016cea0e5b28fa75dd1`，旧分阶段 app／分区／原厂回滚检查通过，回滚副本保留。三个镜像 Hash verified／2 MiB 回读逐字节相符，0x6000–0x7fff／0x8000–0xffff／0x195000–0x1fffff 保护字节未变。新版较小，0x195000–0x199fff 保留旧 app 尾部，不清额外 Flash；后续回滚检查不得假设该段为 FF 或原厂数据。工具退出、串口释放、设备留下载器，新版尚未启动；本轮无 SD 文件清理、相机操作或校准更改。下一步用户就绪后先采集，再提示人工开机；只读版不会切换照片，检查结果／根因仍未知。详见 `docs/SD-CACHE-SAMPLES.md`。以下为阶段历史。

第十轮分阶段版首次人工启动：已有库成功打开，但首张缓存的首块比较失败，未刷新新图。120 秒采集 `manual-power-sd-cache-diag-01.log` 4018 字节、2 次连接／36 次传输错误（含重枚举重连尝试），捕获 APP_ENTRY（568 ms）、SD mounted（710 ms）、PMU ready（779 ms）；1302 ms 报 `stage=cache-compare index=1 offset=0 errno=0 created_library=0`，1303 ms PREPARE_FAIL／应用返回，无 READ_OK／SAMPLE_RESULT。代码路径证明类型／415952 字节长度／首个 1024 字节读取和此前绑定检查通过，首块内容与预期不符；offset=0 不是准确坏字节位置，不能区分头／像素不符。两版 ELF 三份嵌入包均与同一私有素材逐字节一致，排除本次诊断编译更换素材。没有记录缓存文件是否本次创建，不能断言此次完全无 SD 写入；未覆盖已有文件或显示嵌入兜底。本轮无主机复位／烧写／删卡文件，采集结束、串口释放。下一步补只读首差异／头字段与完整 payload CRC 检查，保留证据、不重写，暂不要求重复开机；根因尚未确定。以下待启动状态为历史。

第十轮分阶段诊断版已实现／授权烧写／回读，待人工开机采证：新增 prepare_report，固定阶段／图片编号／处理偏移／errno／是否成功新建库，首错在 fclose 清理前保存；不输出私人路径，不改失败停止／现有文件保留策略，未宣称根因修复。主机 CTest 22/22（ASan/UBSan），覆盖输入错误、缺挂载 errno、未知条目、损坏块、写中断报告；默认及诊断 v6.1 编译／门禁通过。旧 bool 准备接口保留，门禁兼容旧／新准备入口。诊断 app 1612304 字节／SHA256 `974afe16ce4af24b267132e9ba131d8a6516feeadb238c1fcab5c0c90b5ffa46`。完整 16 MiB 私有备份 `.cache/pico-sd-cache-diag-qSUebQ/pretest-flash.bin` SHA256 `20c7bf064e690d6b301856e623aa171e0baa65ee5137dfd55e58ce389f44afe2`，旧 8aae…应用／分区／原厂回滚检查通过、旧 boot/app 回滚副本保留。三个镜像 Hash verified，2 MiB 回读逐字节相符，0x6000–0x7fff／0x8000–0xffff／0x19a000–0x1fffff 保护字节未变。工具退出、串口释放、设备留下载器；本轮未运行 SD 准备，未清理卡、不格式化／擦 NVS／改校准／操作相机。下一步就绪后先采集再人工开机，查看 SD_CACHE_STAGE_FAIL，仍可能因旧残缺文件安全停止。以下为此前失败／烧写历史。

第十轮首次人工开机：应用启动／SD 挂载通过，缓存准备失败，未显示新图。120 秒私有采集 `manual-power-sd-cache-01.log` 3907 字节、2 次连接／43 次传输错误（含 USB 重枚举重连尝试），捕获 APP_ENTRY sd-cache-samples、SD mounted、PMU ready；应用时间戳 4645 ms 报 SD_CACHE_PREPARE_FAIL 并返回，无 SD_CACHE_PREPARED／READ_OK／SAMPLE_RESULT。保护分支未使用嵌入图兜底；不能将面板保留旧画面当新显示通过。现有汇总日志不足以定位目录／建库／缓存写入或回读／索引提交具体失败，不断言 SD 卡损坏或 fsync 根因。采集结束、串口释放；本轮无主机主动复位／烧写／删除或修复 SD 文件，设备准备路径可能留下部分测试文件，不能断言卡上未写入。下一步增加分阶段错误证据并保留现有文件后定位，暂不要求重复开机。下文烧写等待状态为历史。

第十轮 SD Cover 三图诊断已实现／验证／授权烧写／回读，等待人工开机：只准备独立 `/frame/cache-test`，合法 PFG1 文件同步并全比较后提交三个 KEPT／global sequence=6，关闭重开再从 SD 经完整 CRC／头／库绑定门禁读入暂存区，成功才 GC16；失败无嵌入显示兜底、无覆盖／修复，旧 `/frame/storage-test` 保留。三张原图 SHA256 不变，主机去白框 JPEG 副本记录真实源大小／CRC，三份缓存与独立 Cover 渲染一致；SD 不保存源 JPEG，不证明设备解码或产品源文件耐久性／ACK。主机 CTest 22/22（ASan/UBSan），包含部分写入失效不提交／重试不修复，默认 JPEG 和专用 v6.1 编译／门禁通过。专用 app 1610320 字节／SHA256 `8aae15f1283dd707614f5c71bed57abb7b87ccf678304b2561f3d929b0714423`；默认回归 app 356672 字节／SHA256 `9229e3d98d95ccedaf3e1096d74d0f9d6cdb9eb0dac4966589352935307248e9`，未烧默认版。新完整 16 MiB 私有备份 `.cache/pico-sd-cache-test-IBqYQq/pretest-flash.bin` SHA256 `5c0494efa201bad4aa44b535d3d981d46d193221aa3da438d11edb57a3aa98b9`，旧 Cover／分区／原厂回滚核验通过。三个镜像写入 Hash verified，2 MiB 回读逐字节相符，核对范围内 NVS/phy／保护字节未变。工具退出、串口释放、设备留下载器；没有 SD_CACHE_READ_OK 或启动／实屏证据。下一步用户就绪后先采集再提示人工电源操作；读耗时、三图视觉、跨重启、读失败保持画面／压力另验。见 `docs/SD-CACHE-SAMPLES.md`；以下为阶段历史。

Cover 缓存／SD 只读加载基础已实现（未上板）：PFI1／PFU1／PFG1 增加 layout=2 Cover，旧 layout=1 Fit 保留；主机显式 --cover 导入，按源凭据记录的布局本地恢复，旧已提交 ID 不迁移／覆盖，不同布局待恢复凭据冲突停止锁写。新增便携完整缓存读回门禁（每次读取≤1024 字节／CRC／前后头与长度／奇数填充），FatFS 只读加载要求已提交可展示状态与每次 IO 库绑定／介质检查，失败不允许发布暂存区。主机 CTest 21/21（ASan/UBSan），覆盖全部小缓存字节损坏／截断／IO 失败、完整缓存每个 present 失效点、错库／布局／链接／未提交／删除／PUT_AWAY 拒绝，Fit／Cover 分配失败本地恢复及布局冲突保留源。v6.1 默认 JPEG 构建／门禁通过，两个读取函数在组件库内；app_main 未调用新缓存接口，默认 app 356672 字节／SHA256 `d2506a67d2b7b2db893ead3049da53426104f3942669830ee80361b3527dd7d3` 未变，不视为设备新路径已启用。当前设备仍三图 Cover 实屏验收版，本轮无设备连接／复位／烧写／SD 写入。自动缓存迁移、设备 SD 缓存写入与读取后显示闭环尚未完成。下一阶段独立诊断固件准备素材并接只读加载→GC16，不要求电脑复制到卡，不碰旧测试库／原片。见 `docs/COVER-CACHE.md`。

第九轮 Cover 三图本轮实屏验收通过：用户明确确认“比例正常，铺满屏幕，没有竖条”，并提供三张面板照片；附件顺序为测试图 3／2／1。画面与 Cover 预览构图相符，未见旧 Fit 图 1 的明显竖条。结合本轮三个 result=0／有序结束，确认这次嵌入 g4 → GC16 的全屏等比裁切展示通过。旧竖条记录改为“本轮未复现、根因未确定”，不能声称 Cover 根治或首次物理清屏假设已证实；本版没有清屏／波形／时序／VCOM 改动。仍非设备 JPEG／SD 读图／产品缓存／相机链路／长期灰阶残影或功耗验收。此反馈轮仅更新文档，无连接设备／复位／烧写／代码修改，原图／私有素材／SD 库保留。下一阶段建议接通有独立 Cover 布局标识的本地缓存与 SD 读图显示，再做相机只读连接；不把旧 layout=1 Fit 缓存当 Cover。以下为本轮采集／烧写及旧 Fit 历史。

第九轮 Cover 三图首次人工开机后启动／刷新日志通过，视觉回归待确认：端口真实打开后提示关机／开机，用户回复“已开机”。120 秒私有采集 `.cache/pico-cover-test-VPaA0c/manual-power-cover-01.log` 5233 字节、2 次连接／37 次传输错误（含重枚举尝试），捕获 APP_ENTRY、三组有序 SAMPLE_BEGIN／SAMPLE_RESULT 全部 result=0、SAMPLES_END 与应用返回。三张温度均 30°C，copy_ms 均 299，refresh_ms 401／459／459；第三张结束时间戳 64947 ms。三次 free internal=245311、PSRAM=6678428，最低 free internal=208640、PSRAM=6678428 字节，非完整堆峰值数据。采集正常结束，串口释放，无本轮主动复位／烧写／SD 测试文件写入／相机操作。刷新调用成功不证明全屏／不拉伸或竖条消失；等待用户报告三图画面及第一张竖条，旧异常仍未关闭。以下“待开机”属本轮烧写阶段历史。

第九轮全屏等比裁切版已实现／授权烧写／回读通过，待人工开机：按用户最新“图片全屏展示，根据屏幕尺寸裁切，不要拉伸”更新需求／技术设计／DESIGN。便携核心增加 Cover 居中裁切和 EXIF 纠正后显式 ROI，横／纵同一步长、双线性采样、不加纸边；原 Fit API／layout=1 缓存保留兼容，Cover 尚未接旧持久化导入契约。CLI --cover／--crop，Demo 0 SD JPEG 入口改 Cover；三图嵌入诊断先显式去样片白框后 Cover，私有 `.cache/display-cover-FuVMxc`，原图 SHA256 未变，预览已目视检查。居中裁切会丢弃上下内容，图 2 底部人物被裁出，后续可提供裁切位置调整，不能承诺完整构图。主机 CTest 20/20（ASan/UBSan），display-samples／JPEG 展示 v6.1 编译／门禁通过；本次写入的是三图诊断 app 1594416 字节、SHA256 `9b95994cd9fc499ca131789dc4ff6d18531b2b60b5ed195533b380252482371a`，普通 JPEG app 356672 字节仅编译未烧写。新完整 16 MiB 私有备份 `.cache/pico-cover-test-VPaA0c/pretest-flash.bin`，SHA256 `6a2a1421b6780bc1cb5c244f8db7ec89ec8b980139f49abd153bc76cdd02c998`，旧 Fit 三图／分区／原厂回滚核验通过。boot/table/app Hash verified、1728 KiB 回读相符，检查范围内 NVS/phy 与边界外保护字节未变。设备留下载模式、串口释放，尚无新版启动／视觉结果。仅改布局与样片，未加清屏／换序或改波形／时序／VCOM；第一张竖条仍独立未解决。下一步用户就绪后开 120 秒采集，再人工关机／开机，确认铺满、不拉伸及竖条是否复现；下文均为此前 Fit 阶段记录。

第八轮视觉反馈：用户提供三张面板照片，报告测试图片 1（健身设施）有异常竖条，图片 2／3 没有。附件顺序实际为测试图 2／3／1；第三个附件可见天空至地面多条竖向痕迹，另两图未见同类明显条纹。整轮视觉验收未通过，三个 result=0 不覆盖此异常。只读离线检查：样片 1 PGM 与已转换数据一致，桌面预览未见同类竖条；源码显示 highlevel 初始化 front/back_fb 全白，当前显示入口首次刷新前不执行物理清屏，而锁定原厂 app_main 先 epd_clear 再绘制开机图。full GC16 路径仍构造 front/back 差分，刷新后才更新 back_fb，因此首次软件上一帧与实际保留画面的不一致是优先假设，不是已确认根因；图片内容与首刷顺序尚混杂，不能排除内容相关扫描／电源／驱动问题。下一步建议换序／后段重画图片 1 对照，再比较一次原厂 API 清屏基线，不改底层波形／时序／VCOM。此轮仅检查源码／私有预览并更新记录，未连设备、复位、改固件或烧写。

第八轮三图首次人工开机后启动／刷新日志通过，视觉验收待用户确认：端口真实打开后提示关机／开机，用户回复“已开机”。120 秒私有采集 `.cache/pico-display-test-pqfvd2/manual-power-display-01.log` 5232 字节、2 次连接／36 次传输错误（含重枚举重连尝试）；捕获 APP_ENTRY embedded-display-samples、三组有序 SAMPLE_BEGIN／SAMPLE_RESULT 全部 result=0、SAMPLES_END 与应用返回。三张温度依次 30／30／31°C、copy_ms 300／299／299、refresh_ms 394／451／451；末张完成于应用时间戳 64971 ms。三次 free internal=245311、PSRAM=6678428 字节，最低 free internal=208640、PSRAM=6678428；不是完整堆峰值或长期压力数据。SD 挂载／PMU ready 已捕获；无 panic/watchdog 日志，存在上游 GPIO39/40 提示与 SD input-delay fallback 警告，不据此判断硬件故障或修改时序。ROM 启动头／复位原因缺失，人工电源操作依赖用户确认。采集正常结束、串口释放，无本轮主动复位／烧写／SD 测试文件写入／相机操作。刷新函数成功不等于画面正确；下一步逐张确认图像对应、方向／灰阶／异常横条／残影，第三张保持。设备端 JPEG／SD 读图／Demo 0 与相机仍未验收；以下烧写等待说明是历史记录。

第八轮三张私人照片实屏测试版已烧写／回读通过，等待人工启动与视觉验收：新增显式 display-samples 模式，离线 g4 嵌入 Flash，经原驱动像素映射执行 GC16；顺序 1→2→3，前两张各停留约 30 秒，第三张保持。长度／电源／刷新错误停止，不写 SD 测试文件，不改上游波形／时序／VCOM 校准；不证明设备端 JPEG 解码、SD 读图或相机连接。三张原图 SHA256 未变，转换预览与此前私有样片逐字节一致。主机 CTest 20/20（ASan/UBSan），v6.1 编译／display-samples 门禁通过，app 1594416 字节、SHA256 `bb884bc754943805dcdd6f6cf3bbb37e00c611da88f9b121d546bf3c59e653f2`。新完整 16 MiB 私有备份 `.cache/pico-display-test-pqfvd2/pretest-flash.bin`，SHA256 `24f07bdbeaeebcb77674401a9ea888583c07304d323e465a3fed65273092a7e9`；旧严格恢复／分区／原厂回滚检查通过。按用户无需再确认烧写的授权写入三个镜像，Hash verified；1728 KiB 回读三个镜像相符，检查范围内 NVS/phy 和写入边界外保护字节未变。工具退出、串口释放，设备留下载模式；尚无 SAMPLE_RESULT 或实屏证据。下一步用户就绪后启动 120 秒采集，端口真实打开后才提示人工关机／开机，逐张核验画面／返回码／温度／耗时／内存。见 `docs/DISPLAY-SAMPLES.md`；以下为阶段历史。

第七轮严格已有库恢复三轮 PASS（3/3）：第三轮私有 `manual-power-recovery-03.log` 1592 字节、2 次连接/31 次传输错误（含重枚举尝试），捕获 SD mounted、`SD recovery PASS; source=existing-only; expected sequence=2; no create/checkpoint/write`（852 ms）和应用返回。三次均先确认采集真实就绪、再提示人工关机/开机，用户确认“已开机”；无测试文件重建/补写，原库/双快照/空日志/唯一 KEPT/全局序号 2 与照片序号 1 读取检查保持。采集结束、串口释放，本轮无主机主动复位或烧写。完成三轮人工重启后已有 fixture 保留验证，不等于独立掉电测量、写中断/满卡/拔卡/压力验收；日志未捕获启动头/ROM 原因。停止同类重复电源操作，下一阶段准备用户三张照片的设备限额内副本/实屏验证，原图和测试库保留。

第七轮第二次严格已有库恢复 PASS，累计 2/3：用户就绪后另启采集，真实端口打开才提示电源操作，用户回复“已开机”；私有 `manual-power-recovery-02.log` 1592 字节、2 次连接/40 次传输错误（含重枚举尝试），捕获 SD mounted、`SD recovery PASS; source=existing-only; expected sequence=2; no create/checkpoint/write`（919 ms）与应用返回。原库/双快照/空日志/唯一 KEPT/固定序号读取检查通过，没有重新建库或补写。采集正常结束、串口释放，无本轮主机主动复位/烧写；ROM 原因/启动头未捕获，电源操作依赖人工确认，非独立电气测量或压力/写中断验收。下一步第三轮新采集后再操作电源，不清理文件、不更换卡或初始化固件。

第七轮严格已有库恢复首次真机 PASS（1/3）：采集端口真实打开后提示人工电源操作，用户回复“已开机”；私有 `manual-power-recovery-01.log` 1592 字节、2 次连接/41 次传输错误（含重枚举尝试），捕获 SD mounted、PMU ready、`SD recovery PASS; source=existing-only; expected sequence=2; no create/checkpoint/write`（921 ms）和应用返回。匹配已烧写严格模式说明本次读取原有库、双快照/空日志/唯一 KEPT 与固定序号检查通过，不是缺库重建。缺启动头/ROM 复位原因，电源操作依赖人工确认，不宣称独立电气掉电验证。采集结束、串口无占用，无主机主动复位/烧写；诊断应用不创建或写回 SD 文件，仍未做压力/写中断或实屏/相机验收。下一步另起新采集、第二轮人工关机/开机，不清理目录、不重烧初始化版。

严格已有库恢复版已实现、验证并按用户授权直接烧写，待人工开机：新 recovery-only 配置跳过 mkdir/create/import/action/checkpoint，先检查已有目录/普通文件，再要求双快照完全相同且有效、空 journal、原 fixture 唯一 KEPT、全局序号 2/照片序号 1；缺失/损坏失败，不重新建库。主机 CTest 19/19（ASan/UBSan），新增实际临时文件恢复/写回调禁止/逐个 size/read IO 失败测试、恢复与初始化配置门禁；开发中测试夹具会话关闭/序号错误已修正后全量重跑通过。v6.1 recovery 编译/门禁通过，app 345968 字节、SHA256 `b748505163b84a07c4fc2e7529d2499f39a53e1758bee77b8f52787286b660d9`。完整 16 MiB 私有备份 `.cache/pico-storage-recovery-YdnR8C/pretest-flash.bin`，SHA256 `002616e7cdcd2fbd868bbe22f3986f79cddbd647f07c3354cd6fef24c7d9fb4d`；旧自动目录 storage/分区/原厂回滚核验通过，三镜像写入 Hash verified、448 KiB 回读逐字节相符，保护区域/NVS/phy 与备份一致。设备留下载器、工具退出、串口无占用，无设备端 SD 执行/相机操作；等待先采集再人工关机/开机，恢复 PASS 尚未取得。见 docs/STORAGE-RECOVERY.md。

第六轮首次 SD 事务真机 PASS：采集端口真实打开后提示人工关机/开机，用户回复“已开机”。私有 `manual-power-storage-01.log` 1537 字节、2 次连接/39 次传输错误（含 USB 重枚举尝试，不等同硬件故障）；捕获 SD mounted、PMU ready、触摸 ready、`SD diagnostic PASS; global sequence=2`、Returned from app_main。证明存储入口运行、专用目录可用、事务/checkpoint/同进程重开校验通过；代码路径要求有效 PMU VCOM，但日志没有单独 VCOM 值，不补造读数。启动头/APP_ENTRY/ROM 原因缺失，不认定无干扰冷启动。采集结束、串口无占用，本轮无主机复位/烧写/相机操作，设备端按诊断流程操作 SD 文件。尚未证明跨断电恢复：当前 PASS 摘要不区分 open-existing 与 create，sequence=2 本身不能排除重新建库；下一步补充已有库/初始化来源标记或严格恢复模式，再验证三次真实重启，不直接把相同 PASS 当持久化验收。

自动目录存储诊断已实现、验证并烧写（待人工开机）：用户无法用电脑建目录，要求其他方式并明确“直接烧写不用我确认”。新增独立诊断目录准备，在已挂载根下仅创建固定 `/frame` 与 `/frame/storage-test`，未知条目/文件冒充目录/损坏库停止，不删除/格式化/清 NVS/改 VCOM。主机 CTest 18/18（ASan/UBSan），v6.1 storage 编译/门禁通过；app 347952 字节、SHA256 `ef7f08cce70f5f99962e7de1c83c6ae775c0665ee56106170c80bf71477dcbbb`。本次完整 16 MiB 私有备份 `.cache/pico-storage-auto-m1QIyN/pretest-flash.bin`，SHA256 `2485dbf8975d61525738d071e755f43d9781fb07721a5f19c0c937e6e5430d4e`；旧 RAM 探针/分区/原厂回滚核验后按生成 flash_args 烧写，三镜像 Hash verified。448 KiB 回读三镜像逐字节相符，0x6000–0x7fff、0x8000–0xffff、0x65000–0x6ffff 与备份一致。设备留下载器，工具退出、串口无占用；没有 SD 执行结果，不等于目录创建/存储 PASS。下一步先准备日志采集再人工关机/开机；存储模式无照片刷新，不用旧探针 RAM 地址读取。

SD 阶段准备：用户确认已插入备用 microSD、没有需保留数据。重新审查现有 app_main/storage_selftest/FatFS/BSP 挂载路径，不改上游驱动/波形/时序；正常探测 mount_card(false)，应用不调用格式化/remount/NVS erase/VCOM 校准写入，读取有效 PMU VCOM 后才继续。诊断仅操作专用 `/frame/storage-test` 的 library.id、snapshot.0、snapshot.1、journal，需人工预建空目录。ESP-IDF v6.1 storage 构建/门禁通过，app 347104 字节、SHA256 `189e5540b4233870e523118cd8a4f8b5b16cc3a212152042e9b5bf927cb35ff1`，SELFTEST=1、BOOT_PROBE 未启用；flash_args 已核对。全量 CTest 17/17 再跑通过。本轮未连设备/复位/烧写，仍运行 166656 字节启动探针；等待目录准备和单独烧写确认，未验证实际 SD 挂载/事务或屏幕。

第五轮第三次人工电源操作确认后 RAM 查询 PASS，累计 3/3：私有 `jtag-status-power-03.log` 两份一致快照 `42505231 1 1 1 1 3 ffffffff 0 0 1`，三模式全部完成、探针结束、无错误元数据；双核 Reset cause=1 Power on reset、PRE/POST 均 running。无 reset/断点/烧写/SD/PMU/相机操作，工具退出、串口无占用。三轮均基于用户执行关机/开机并重连的确认，不是独立电源测量或 8 MiB 压力测试。启动探针阶段已有三轮明确专用结果，下一阶段准备备用 microSD 与 SD/显示验证方案；更换固件需单独确认，存储三次断电恢复仍未测试。

第五轮第二次人工电源操作后 RAM 查询仍 PASS：用户再次按关机/开机重连提示回复“已重连”，私有 `jtag-status-power-02.log` 两份一致状态为 `42505231 1 1 1 1 3 ffffffff 0 0 1`；三模式完成、探针结束、无失败元数据，双核 Reset cause=1、查询前后 running。无新复位/断点/烧写/SD/PMU/相机操作，工具已退出、串口无占用。累计两轮人工操作确认后的 PASS；RAM 没有启动唯一编号，不能仅凭相同快照和复位原因独立证明断电发生，重复启动记录依赖人工操作确认。下一步第三轮人工关机/开机重连后查询，再评估 SD/显示阶段；本轮仍不换固件。

第五轮人工开机后 RAM 查询取得明确 PASS（首次）：用户回复“已重连”，匹配已烧写 166656 字节 bin/ELF；OpenOCD 20260703/1000 kHz 无 reset/断点/Flash 操作，双核 Reset cause=1 Power on reset。两份相同快照为 `42505231 1 1 1 1 3 ffffffff 0 0 1`，严格离线解码 PASS：app_entered/test_done/probe_done 均 1，三模式完整通过，无错误元数据。双核查询前/后均 running，工具已退出、串口无占用。私有证据 `.cache/pico-ram-probe-t4Gol7/jtag-status-power-01.log`。仅证明本次 64 KiB scratch 测试，不证明 8 MiB 全域、长期稳定、SD/PMU/显示/相机；下一步再人工关机/开机并重连，补重复启动证据，暂不换固件。

RAM 查询探针已授权烧写并回读核验：app 166656 字节、SHA256 `3bb56158734eb4f7e0dbcef1ba7621ab42e09563269de81827ad41df8ad6b58e`，匹配 ELF 状态地址 0x3fc92a5c。新完整 16 MiB 私有备份 `.cache/pico-ram-probe-t4Gol7/pretest-flash.bin`，SHA256 `d736b90a0a4225208e44c96c2c6c215ffeabdf279da1be3e899483335b98f9b7`；旧探针身份/尾部、分区与原厂回滚均核验通过。boot/table/app 写入 Hash verified，256 KiB 回读三个镜像逐字节相符，0x6000–0x7fff、0x8000–0xffff、0x39000–0x3ffff 与备份一致。进入下载器使用 default-reset，烧写/回读均 no-reset，工具已退出、串口无占用；设备留在下载器，尚未验证新应用启动或专用测试结果。下一步人工拔 USB、关机/开机并重连，再无 reset/断点查询 RAM；不烧 SD/照片固件。后续段落为阶段历史。

RAM 查询探针已实现并验证（未烧写）：40 字节内部 DRAM 状态、明确结果/完成标志、通过模式数、第一处错误元数据，每次进入重置，不跨复位持久化。实际 C 测试补状态重置/发布顺序；新增离线解码与真实 Tcl fake target 测试，CTest 17/17、ASan/UBSan；v6.1 build/门禁通过，新 app 166656 字节、SHA256 `3bb56158734eb4f7e0dbcef1ba7621ab42e09563269de81827ad41df8ad6b58e`，状态符号 `3fc92a5c 00000028 D`。设备仍第四轮 166512 字节探针，本轮未连接/复位/烧写。下一步确认烧写新版，人工开机后无 reset/断点读取状态；操作见 `docs/PROBE-STATUS.md`。

用户按提示断开 USB、关机/开机并回复“已重连”，恢复检查成功：OpenOCD 20260703/1000 kHz、禁 Flash/网络服务，无新复位/烧写。双核复位原因 1 Power on reset，PC=0x40378f66（空闲等待），ibreakenable 读取均为 0；仅将易失调试使能再次置 0，随后确认双核 running、工具退出和串口释放。断电后再次核对任务列表，已进入调度器空闲路径，专用结果仍未知，不反推 PASS。此前 JTAG 异常/恢复未知已解除；下一步建议探针保留可查询的 RAM 结果，再开机后读取，避免 USB 重枚举期间断点/串口丢失；该改动需要实现、测试、编译和新的烧写确认，不自动重烧。

用户授权硬件断点/受控复位，已按当前 ELF 设置结果入口 0x420088a8 与结束入口 0x4200891a（仅硬件断点，Flash 功能/网络服务关闭）。OpenOCD 20260703 执行 reset halt 后日志显示 Software core reset，resume 时 USB-JTAG 报 IO/OVERFLOW/missing bitq，未捕获结果；退出时 cpu0 unknown、cpu1 running，不能确认恢复或断点清理。随后无复位重连清理，20260703/5000 kHz 与已安装 20260831/1000 kHz 都在 JTAG init 阶段报 IN buffer overflow，清理脚本未执行。工具均已退出、串口无占用，无重烧/Flash/SD/NVS/PMU/相机写入；设备仍新版探针，但实时执行状态未知。已停止自动尝试，需要人工拔 USB、完整关机再开机并重连后，先检查并清理易失调试状态。专用测试结果仍未知，不能据 OpenOCD 进程退出码 0 判成功。

实时 JTAG 排查纠正“仍卡 PSRAM”的判断：IDF 对应 OpenOCD 20260703，Flash 支持和网络调试服务关闭，不请求 reset/load/烧写。三次短暂停核采样，双核 PC 均 0x40378f66（esp_cpu_wait_for_intr），返回地址对应 esp_vApplicationIdleHook；任务 RAM 核对 scheduler=1、tick=0x4cc43、task_count=4，当前为 IDLE0/IDLE1，完整任务列表只有 IDLE0/IDLE1/ipc0/ipc1，没有 main_task。说明当前已进入 FreeRTOS 空闲路径，早期初始化不是当前阻塞位置，探针主任务已不在任务列表；支持探针执行完，不能证明 PASS，PASS/FAIL/ALLOC_FAIL 均会结束返回。本轮短暂停核影响时序，但每次已恢复双核 running，工具退出、串口无占用，无固件/Flash/SD/PMU/相机写入。下一步建议用硬件断点在结果输出处读取测试结果，需再授权一次受控复位，不必立即重烧。

用户明确“允许”软件复位对照，已完成一次主动采集：`capture_pico_boot.py --connect-reset default-reset --seconds 30`，私有 `software-reset-01.log` 2495 字节。进入下载器、清易失 force-download latch 后 watchdog 复位；ROM 为 `USB_UART_CHIP_RESET` / `SPI_FAST_FLASH_BOOT`，应用加载到 0x10000，最后日志 `MSPI Timing: Enter psram timing tuning`（74 ms），30 秒内无 APP_ENTRY/专用结果/摘要/结束。说明这次软件复位未取得应用启动证据，不是完整断电测试，也不能定位具体阻塞指令。Saved PC=0x40378f66 离线解析为 esp_cpu_wait_for_intr，属于复位前上下文，不是当前 PC。本轮无 Flash/SD/NVS/PMU/相机写入、未改时序，串口已释放。新版探针保留，停止 SD/照片上板；下一步须另行规划实际断电或实时启动阶段定位，不自动再次复位/烧写。

第四轮人工开机第二次采集已结束：45 秒、0 字节、1 次连接、0 次传输错误，无探针标记；日志 `manual-power-boot-02.log` 为空，当前 Pico VID/PID 和本地新固件摘要再次核对相符，串口无占用。不能证明开机失败、实际断电/复位时刻或原因，专用 64 KiB 结果仍未确认。不再重复相同人工采集，建议经用户允许做一次主动软件复位对照（只改易失复位状态，不烧 Flash/SD，不作为冷启动证据），或由用户补充电源操作观察。本轮未烧写/主动复位。

第四轮人工开机第一次采集：用户回复“已开机”后检查 45 秒窗口，0 字节、1 次连接、35 次传输错误、无标记；错误包括端口不可用尝试，不等同芯片故障。随后当前端口 VID/PID 相符，10 秒补采 44 字节、1 次连接、0 次错误，仅 `SPI SRAM memory test OK`，无 APP_ENTRY/专用 PASS/FAIL/PROBE_SUMMARY/PROBE_END。不能将系统测试 OK 当专用测试通过，也不能从晚采缓冲日志证明完整开机时序。采集结束释放串口，本轮无再次烧写或主动复位；下一次先开启采集再人工关机/开机。

用户在烧写确认问题后回复“准备好了”，第四轮结果重播探针已烧写。新完整 16 MiB 备份 `.cache/pico-replay-test-pyV5XS/pretest-flash.bin`，SHA256 `c0235e86e3291f48f0eb7bae21090f06614f2953aaa015fd2483caadf22f81dd`；当前旧探针/原厂回滚文件/分区核验通过。boot/table/app 三目标 Hash verified，256 KiB 回读中三个镜像逐字节相符，0x8000–0xffff 与备份一致；新 app 166512 字节。备份进入下载器用了 default-reset，烧写和回读 before/after 均 no-reset。没有 SD/PMU/相机操作，不擦除 NVS/storage；尚未验证新版启动和专用 64 KiB 结果，等待人工电源操作与采集。后文“未烧写”是开发阶段历史，不代表当前设备。

继续开发完成探针结果重播：64 KiB 三模式只执行一次，释放临时缓冲后每 500 ms 重播结果、已完成模式数与应用摘要，共 20 次；分配失败明确 ALLOC_FAIL，PROBE_END 携带结果。新增实际探针源码的主机测试覆盖正常、分配失败、三种模式损坏与晚接收；CTest 16/16（ASan/UBSan），采集循环 11 项。ESP-IDF v6.1 探针编译/门禁通过，166512 字节，SHA256 `55cee33ab0d9b05bab1c31213186c1b136277e7eb4bbbb3bb769905768d980d7`。新固件未烧写；设备仍是旧 166496 字节探针。下一步替换探针需用户确认烧写，随后人工关机/开机；本轮不连接设备、不改时序/BSP/PMU/SD/相机。

用户再次确认“已开机”后，补做 10 秒只读串口检查：1 次连接、0 字节、0 次传输错误，无新标记。启动探针已结束后不会持续输出，不能据此判定启动失败，也未补齐专用 64 KiB 校验结果。未主动复位或烧写；等待先开启采集、再人工关机/开机的下一轮操作。

修正版采集取得突破：用户再次就绪并确认“已开机”，45 秒采集 397 字节，2 次连接/32 次传输错误（重枚举窗口）。日志为 Found 8MB PSRAM device、SPI SRAM memory test OK、heartbeat=1..4、PROBE_END、Returned from app_main，证明本次越过早期初始化并进入/完成探针应用。没有捕获启动头/APP_ENTRY/PSRAM_64K_PASS，不能认定专用 64 KiB 校验或完整冷启动门禁全部通过；USB 重枚举导致的日志缺口仍待补齐。串口已释放，探针仍在设备，本轮未烧写/主动复位/访问 SD 或相机。

用户授权“开始”修正采集工具：新增固定 POSIX pyserial 3.5 适配，阻止库层 DTR/RTS 更新和自动输入清理、禁写、清 HUPCL、未知环境拒绝、真实打开后才提示就绪。循环模拟 10 项、真实依赖 OS mock 6 项，CTest 15/15；IDF Python 依赖测试 6/6。本轮只改主机采集/测试/文档，未连设备、未复位或烧写，探针固件不变，真正冷启动待人工新实验。

新增只读排查结果：真实 pyserial 3.5 open 在无设备 OS mock 下仍更新 DTR/RTS 并清输入；现有模拟测试未覆盖依赖层，采集器不能证明无干扰冷启动。ELF 确认 PLL 无超时等待实际已链接，但 Saved PC 属复位前 Flash dummy 配置，不能视作当前阻塞点。已纠正证据说明，本轮未改实现/时序、未连接设备或烧写。见 `docs/BOOT-PROBE.md`。

第三轮探针烧录/回读通过后，用户回复“准备好了”，已启动 45 秒不主动复位的采集；用户随后确认“已开机”。获得 2495 字节日志，仍停 PSRAM 调校，无 APP_ENTRY / PSRAM_64K_PASS / PROBE_END。ROM 原因为 USB_UART_CHIP_RESET，不能认定无 USB 干扰的冷启动。采集已结束、串口已释放，设备仍装有探针，本轮未回滚或继续烧写。未做 SD/照片实屏测试；CTest 14/14，启动门禁及 S1/S3、相机 Gate A–F 未通过。

此前第二轮测试 SPI 启动停于 PSRAM 调校，已回滚原厂 boot/app，用户回复“正常”确认原厂功能恢复；该历史结论不代表第三轮探针已通过。

本轮继续完成只读离线镜像对比：原厂/探针 bootloader 的加载段仅编译时间字段不同，两段 IRAM 完全一致；ROM 头一致，原厂应用编译时序仍未知。新增可复跑工具及 9 项测试，CTest 13/13。未访问设备、未复位、未烧写，未修改 IDF/BSP/时序。

用户新目标“继续开发，直到烧录完成需要人工确认为止”授权第三轮烧录。新增独立、不主动复位的人工开机采集工具及 9 项模拟测试；探针重新构建/门禁通过。本轮整片 Flash 私有备份 `.cache/pico-coldtest-CvkJeU/pretest-flash.bin`（16 MiB），检查/回滚提取通过；随后烧录并核验启动探针，不改时序。没有相机操作或 Flash/NVS/SD 清理，未通过 APP_ENTRY/PSRAM 门禁前不烧 SD/照片测试。

## S0：工程与资料

- [x] 将 PRD v2.0 保存到 `docs/PROJECT-SPEC.md`，原文件保留。
- [x] 将技术方案移到 `docs/ARCHITECTURE.md`。
- [x] 创建项目协作规范和开发进度文档。
- [x] 引入官方 Read Pico 固件子模块，锁定 `28cde682a4468a581c278761922724f57d976418`。
- [x] 建立独立 ESP-IDF v6.1 checkout（tag v6.1，未修改已有 6.2 环境）；固件构建通过。
- [x] README、开发/组件/维护文档与构建入口。
- [x] 只读启动/64 KiB PSRAM 探针与独立构建；旧探针专用结果缺失，新结果重播版已烧写/回读相符，等待人工启动验证，完整门禁未通过。
- [x] build/flash 前自动检查产物模式、分区 MD5/布局、HPM 强符号与入口，拒绝异常产物。
- [x] 原厂/探针 boot/app 离线完整性/头部/段对比工具；boot 仅编译时间差异，启动根因仍待受控冷启动证据。
- [x] 独立、不主动复位的人工开机日志工具与模拟测试；控制线电气影响仍须真机验证。
- [x] 修正 pyserial open 的控制线更新/输入清理，补真实 POSIX 后端 OS mock 测试；仍不保证 OS/USB 零干扰。

## S1：文件到屏幕

- [x] C11 可移植核心 + CMake/CTest。
- [x] 照片状态、放下/撤销、删除意图、防重同步决策（核心原型，未接持久化/UI）。
- [x] CRC/版本/序列校验的日志编码与单照片恢复回放原型。
- [x] 完整构图 Fit、双线性缩放、16 灰阶量化和 g4 像素编码。
- [x] 有界 JPEG 分块解码、EXIF 方向 1–8 与主机离线转换工具。
- [x] Read Pico 启动、只读 VCOM、SD 测试 JPEG、GC16 显示入口（未上板）。
- [x] 主机测试 17/17 通过；C 主机目标 ASan/UBSan，实际探针 5 种场景；产物门禁/镜像对比/采集循环/真实 pyserial 后端分别含 12/9/11/6 个 Python 测试，状态解码 9 项含 Tcl 脚本 6 种 fake target 场景。
- [x] 用户三张 3000×3000 黑白原图离线测试 3/3：显式 1/2 解码、完整构图缓存、分配失败保留源图、无原输入本地恢复；原图 SHA256 未变。见 `docs/IMAGE-TESTS.md`，非实屏验证。
- [x] ESP-IDF 6.1 固件编译通过（0.1.0，ESP32-S3，minimal build）。
- [ ] 真机 Demo 0、峰值内存、方向/色彩/残影样片验证。
- [x] 私有三图嵌入 g4 独立实屏诊断实现，20/20 主机测试、v6.1 编译／门禁及授权烧写／回读通过；不替代设备 JPEG／SD 路径验收。
- [x] 三图诊断首次人工启动，120 秒日志中三个刷新返回码均为 0、有序结束；第三张保持，非视觉验收。
- [ ] 长期灰阶／残影与重复启动验收：旧 Fit 图 1 曾有竖条，Cover 本轮未复现；单轮照片不替代完整稳定性测试。
- [ ] 旧竖条根因对照（待复现）：换序／后段重画图片 1、比较原厂 API 清屏基线；假设未证实，不为无复现状态修改底层。
- [x] 全屏 Cover／等比居中裁切＋显式源 ROI，主机测试和两种固件编译通过；三图 Cover 已烧写／回读。
- [x] Cover 三图首次真机启动日志，三个刷新返回码 0、有序结束，120 秒采集完成。
- [x] Cover 三图本轮全屏／不拉伸视觉回归：用户确认比例正常、铺满、无竖条，面板照片已提供；旧异常本轮未复现，非根因修复证明。
- [ ] 可选裁切位置调整以保留底部主体；默认居中已按本轮要求验收。
- [ ] Cover 独立缓存布局标识／失效重渲染／SD 读图显示闭环；设备 JPEG 与旧 Fit 缓存兼容分别验证。
- [x] Cover layout=2／Fit layout=1 隔离，主机显式导入／按凭据恢复，未知布局拒绝；不是已提交旧缓存自动迁移。
- [x] 可移植完整缓存校验与 FatFS 只读暂存加载，基础 21/21；已接独立 SD 三图诊断，产品显示任务待接。
- [ ] 已提交旧 Fit 缓存显式版本迁移；保持用户决策、源图与失败证据。
- [ ] 独立 SD Cover 缓存诊断准备／写入／回读／显示（无需电脑复制到卡），跨重启及读失败保留旧画面真机验证。
- [x] SD 三图诊断实现／主机 22/22／默认与专用固件编译门禁／完整备份／授权烧写及 2 MiB 回读；不计作设备 SD 执行 PASS。
- [x] 独立RAM写入实验／完整比较、人工关机后严格已有缓存恢复、只读SD三图完整读取／刷新和视觉验收：比例正常、全屏铺满、无竖条／残影；读取约6.5秒／张。原cache-test损坏根因仍待查。

## S2：GR IV 协议探针

- [x] 便携广播候选解析/有界会话表，合成数据主机测试及默认组件编译通过，无无线IO、不证明型号。
- [x] Pico独立有界被动扫描适配、22种模拟入口场景、专用门禁/编译、备份/直接烧写/扩展回读；不计真机无线运行通过。
- [x] 被动扫描首次人工启动候选服务/计数与正常截止清理采证，20次一致总结果；启动头/STARTED未捕获，不计独立掉电或型号身份。
- [ ] 相机不广播对照/重复会话/退出功耗，型号兼容另验。
- [x] 第二次同版人工启动（相机仅关机）仍收到候选，30秒截止/清理与20次一致结果PASS；不计无线关闭负对照。
- [x] 官方关机广播说明与用户Live-View客户端只读核对，独立电源/工作模式/身份诊断计划入库；未移植或连接相机。
- [x] 便携GATT五字段动态读取计划与严格值解析；33组主机ASan/UBSan及默认组件编译通过，未接入运行，不计身份或真实读取。
- [x] 有界GATT会话与实际NimBLE操作层：分段身份读取/不可复用ticket/超时及晚回调隔离；主机34组/门禁27项/真实SDK组件编译，compile-only不可烧写，无连接入口。
- [x] 受控单目标Owner、host队列串行调度、连接/关闭期限与竞态/失败清理证据；主机35组和真实SDK组件编译通过，无设备入口/真机连接。
- [x] 应用Owner timer有界静默屏障与检查式退休/失败保留；实际新适配主机mock及SDK组件编译通过，不计真实并发/SDK全局清理。
- [x] 独立初始化/会话/停止/清理期限、SDK公开NPL timer guard、controller先停/队列预排空/deinit核对、无连接板上入口与固定ABI/反汇编门禁；40组主机及真实SDK编译通过，不计真实并发/射频或清理验收。
- [x] 无连接专用无线栈诊断首次人工开机生命周期PASS：17条一致code0/cleanup1/sdk1/board1及应用返回；启动头/阶段/前3条缺失，不补造20次完整捕获或功耗证据。
- [x] 无连接诊断完整16MiB备份、当前扫描/显示及原厂回滚检查、直接烧写/0x410000扩展回读与保护字节核对；新版待人工启动，非执行PASS。
- [x] 便携受控候选选择及Owner交接；41组ASan/UBSan、门禁32项、实际SDK组件/ELF通过，compile-only、无设备/相机操作。
- [x] 同SDK host扫描→禁收/取消/inactive→当次确认API→一次延迟Owner激活；85秒主任务期限、关闭/复位栅栏、扫描timer退休，43组主机/33项门禁/真实SDK隔离ELF通过，非真机验证。
- [x] 固定静态确认消息通道及同SDK host消费：nonce/epoch/id/services核对、无覆盖/自动同意、拒绝/过期/重放/关闭竞态及迟到生产者保留；44组主机和真实SDK隔离ELF通过，未接实际输入/设备。
- [x] USB输入生产者/严格命令解析/提示/启动nonce、独立只读身份入口、有界输入退出、入口故障测试、人工终端工具及专用固件门禁；主机/编译通过，不固定confirmed=true，不自动选择/配对。
- [x] 本版完整16MiB备份、当前/历史尾部/显示与原厂回滚核验、直接烧写及整片回读/全部保护区域比较；设备留下载器，非运行或身份PASS。
- [x] 本版首次人工启动无确认负对照：18条一致EXPIRED/身份无结果/cleanup1及应用返回；输入IO/SDK/板卡退出路径通过。缺完整提示/启动头，不算完整端到端日志或独立电气/射频证明。
- [x] 实际TTY工具可解析完整提示、用户现场新y回复能在主机完成一次发送；不是固件接收/确认消费证据。
- [ ] 修复无驱动O_NONBLOCK接收适配缺陷并补固定SDK实际分支回归；当前49字节回复无法从硬件FIFO读入，不要求继续重复人工输入。
- [ ] 修复验证/编译/备份烧写回读后另次启动现场确认，测试固件真实消费、只读身份及实际断连/SDK退出；型号/版本及Gate A仍待验。不自动回复y。
- [ ] 受控连接Owner、同host调度/GAP世代、断连/host reset/停止清理、设备入口/新固件门禁；完成后再受控身份真机读取。
- [ ] 独立有界GATT IO适配、连接世代/晚回调隔离、分段限额读取、身份先行门禁及受控真机验证；不启用配对/通知/状态写入。
- [x] 本版人工启动捕获至少一个Camera Information候选与code9/cleanup1及应用返回；固定SDK Observer-only的GAP定时编译条件定位，不计30秒扫描通过。
- [x] 适配层独立30秒截止/显式取消及inactive确认、28场景主机测试、细分/聚合结果重播和编译门禁；不启用Central或修改SDK。
- [x] 截止修正版完整备份/回滚核验/直接烧写及0x410000扩展回读，保护字节未变。
- [x] 截止修正版人工启动：候选1、elapsed_ms30000/end=deadline/inactive1/stats1/code0/cleanup1、20次一致结果与应用返回。
- [x] 安全 DCF 路径校验、只读 XS 请求构造（无网络 IO，尚未证明真机命名全覆盖）。
- [ ] BLE 独立配对、bond/RPA 恢复、身份校验（Gate A）。
- [ ] 2.4 GHz/WPA2 建链与相机 IP 探测。
- [ ] HTTP 列表、storage 映射、XS 下载与中断恢复（Gate B）。
- [ ] 人工预约/未预约/自动预约对照（Gate C）。
- [ ] ACK 幂等、跨存储歧义、GR WORLD 共享状态（Gate D）。
- [ ] 未知固件只读降级和脱敏 fixtures。

## S3：可靠本地照片库

- [x] 多照片日志核心：持久化回调先于发布、交错恢复、序列冲突、失败提交锁写、5000 条上限（主机原型，未接介质）。
- [x] 模拟日志每字节截断/损坏与不确定提交测试；详见 `docs/STORAGE-CORE.md`。
- [x] library-id/全局提交序号封装、双快照编码与恢复选择；5000 条快照实测 200048 字节（仅状态记录）。
- [x] 快照损坏回退 + 日志补齐、错库拒绝、同序号冲突、主机临时文件同步往返测试；见 `docs/STORAGE-FORMAT.md`。
- [x] 主机 POSIX 文件参考适配：绑定文件、独占锁、同步追加、双槽回读后回收日志、重新打开与损坏只读恢复；见 `docs/FILE-STORE.md`。未接 FatFS。
- [x] 介质接口、1216 字节有界日志扫描、FatFS 文件映射和默认关闭的存储诊断入口（待上板）；见 `docs/DEVICE-STORAGE.md`。
- [x] 枚举检查点/打开流程每个 IO 回调失败、部分/完整不确定写入、实际临时文件和卡不可用测试。
- [x] PFI1 导入意图、PFG1 缓存头、源图/缓存回读后提交索引；主机文件与 CLI 链路，24 个 IO 边界 × 3 种故障及重开、防重入测试；见 `docs/PHOTO-IMPORT.md`。未接设备，不代表 Flash/ACK 完成。
- [x] 1024 字节源图复制/CRC/产物回读、PFU1 凭据与源图先落盘后渲染、按 ID 纯本地 recover、损坏/错库锁写；主机参考实现，保留 `.part`，尚无空间回收、HTTP 接收或设备适配。
- [x] Read Pico 首次存储事务/checkpoint/同进程重开 PASS；严格已有库三次人工关机/开机后恢复 3/3 PASS（操作依赖人工确认，无独立掉电测量）。
- [ ] FatFS 写中断/同步回收掉电边界、拔卡/满卡/压力验证；执行 `docs/HARDWARE-GATE.md`，不由三轮只读恢复替代。
- [ ] microSD 事务文件、双快照、单写入任务。
- [ ] Flash 持久化用户决策/tombstone，5000 条容量停入。
- [ ] 拔卡、换卡、满卡、序列冲突与断电注入。
- [ ] 触摸/按键上一张下一张、操作菜单与撤销。

## S4：自动闭环

- [ ] 经验证的 BLE WLAN 控制与预约通知（Gate E）。
- [ ] 有界轮询、重试、批次合并刷新。
- [ ] 双端功耗、即时/省电模式、相机忙退避。
- [ ] 连续 50 次闭环、P95、断电恢复等 Gate F 验收。
- [ ] GR IV HDF 与 Monochrome 独立兼容测试。

## S5：P1 产品化

- [ ] 留下/只看留下、EXIF 信息相纸、恢复已放下。
- [ ] 本地管理 AP/STA、JPEG 上传、裁剪与清理。
- [ ] 横竖屏与拿起唤醒；长期运行测试。
- [ ] P2 再实现本地 OTA、签名校验与回滚。

## 验证记录

| 项目 | 当前结果 |
| --- | --- |
| 文档入库整理 | 完成 |
| 最新无连接真机 | 2026-10-02：792c5409…首次人工启动17条一致code0/cleanup1/sdk1/board1/start0/wait0/close0及应用返回，120秒2145字节；无扫描/连接/GATT。启动头/阶段/前3条缺失，不计电气掉电/射频功耗/身份配对。本轮仅采证/文档、串口释放，以下待启动为历史 |
| 最新无线栈与烧写 | 2026-10-02：40/40 ASan/UBSan（37.75秒）/31项门禁/18项采集、真实SDK默认/专用编译；无连接诊断792c5409…完整备份后烧写/扩展回读通过，保护检查范围不变，留下载器、串口释放。真实运行待人工开机；普通Owner仍compile-only，相机/SD未操作。以下为历史 |
| 最新受控连接Owner | 2026-10-02：主机35/35 ASan/UBSan、门禁27项、真实SDK组件/默认编译通过；单目标连接/串行host队列/严格关闭证据。SDK timer释放无法从host exit自证安全，保留资源/禁用自动deinit；整机wrapper与timer静默/无线栈退出未完成，compile-only禁止烧写。本轮无设备/相机操作，以下为历史 |
| 最新GATT操作层 | 2026-10-02：主机34/34 ASan/UBSan、门禁27项；真实v6.1 Central/GATT组件编译通过，仅库/无应用入口，门禁拒绝compile-only。默认359440字节02b2254c…编译通过未烧；扫描bd74b761…未变，本轮无设备/相机连接。Owner/退出清理与真机未完成，以下为历史 |
| 最新便携GATT核心 | 2026-10-02：33/33主机ASan/UBSan（37.35秒），默认v6.1编译/门禁通过，2fc5a127…未变；动态五字段元数据计划及严格值解析，无IO或运行时调用。扫描产物bd74b761…门禁复核，本轮未烧写/连接设备；真实GATT/身份/功耗未验。下表32/31组为历史 |
| 最新真机与参考核对 | 2026-10-02：第二次同版人工启动扫描流程PASS，关机条件仍候选1/services2；用户仅关闭相机电源，不计无线关闭负对照。官方关机广播说明及用户Live-View源码只读核对完成，Power/Operation Mode后续独立GATT诊断未实现，无相机连接/配对 |
| 当前最新主机/编译/烧写 | 2026-10-02：截止修正版32组ASan/UBSan、入口28场景、门禁26项/采集17项、默认/专用v6.1通过；bd74b761…烧写/回读一致，首次人工启动扫描流程PASS：候选1、30秒独立截止/inactive1/code0/cleanup1、20次一致结果，启动头缺失，不计型号身份或配对。旧99c…code9失败保留，以下为历史 |
| 最新主机测试 | 2026-10-02：CTest31/31，ASan/UBSan，35.54秒；新增广播候选合成数据测试，无相机IO |
| 最新编译与设备状态 | 默认v6.1组件编译通过，356560字节、508870f7…未烧；设备仍13117988…已验收显示版。BLE扫描/配对及HTTP未验。下表旧构建/屏幕待验行为历史 |
| 主机单元/集成测试 | AppleClang 17，Debug，C 目标 ASan+UBSan，IDF Python 3.14.6，CTest 16/16 通过；实际探针 5 种场景；此前真实样片 3/3；门禁/镜像对比/采集循环/真实库分别含 12/9/11/6 个 Python 测试。首次旧缓存 Python 缺依赖失败，指定 IDF Python 后全部重跑通过 |
| 本轮离线启动核对 | 四镜像完整性通过；boot 两 IRAM 段一致、数据仅时间差异；头部一致；没有设备操作，原厂应用编译时序/根因未知 |
| ESP-IDF 编译 | 2026-10-01：v6.1，0.1.0，显示/存储/只读探针三构建均通过；应用分别 355648 / 347104 / 166496 字节，产物门禁均通过 |
| 最近固件编译 | 本轮 v6.1 结果重播探针 build/门禁通过，166512 字节；仅探针日志/失败结果处理改变，时序/BSP 未改，新版未烧。此前默认显示构建 355920 字节，未烧 |
| 第二轮上板探针（历史） | 166496 字节，boot/table/app 写入 Hash verified；SPI_FAST_FLASH_BOOT 后停于 PSRAM tuning，20 秒无 APP_ENTRY，未通过，已回滚 |
| 第三轮烧录/回读 | boot/table/app 写入校验通过；回读 boot/app 与构建产物逐字节一致、0x8000–0xffff 与本轮备份一致；no-reset，等待人工开机，未验证启动 |
| 构建模式核验 | 默认配置 SELFTEST 未启用；独立诊断配置 SELFTEST=1，已链接介质/FatFS/诊断入口 |
| USB 设备 | /dev/cu.usbmodem2101；用户已确认 Pico 并授权烧写 |
| 原始固件备份 | `.cache/pico-flash-backup-zujfGX/original-flash.bin`，16777216 字节，私有缓存，勿上传 |
| 烧写/恢复 | 测试固件曾烧写成功但未获诊断结果；用户要求恢复后，原 boot 0x0–0x7fff 与 app 0x10000–0x40ffff 均恢复且 Hash verified，未动其他分区 |
| 本次备份/回滚 | `.cache/pico-pretest-G7bldI/` 最新完整 16 MiB 备份；恢复原 boot/app 均 Hash verified；0x8000–0xffff 回读与备份一致，未写 storage/SD |
| 回滚功能验证 | 用户回复“正常”，确认本次原厂功能恢复；串口复位捕获仍与人工功能确认分开记录，测试探针冷启动未通过验证 |
| 相机连接与预约 | 未进行 |
| 屏幕样片/功耗 | 未进行 |

## 下一步

当前下一步：在已接入的固定确认队列上实现实际输入驱动和新的人工操作绑定，保证提示/nonce不复用、按键保持不确认、生产者有界退出与板卡静默，再集成独立只读身份设备入口/日志/故障测试及专用门禁。消息通道不引用NPL/SDK、关闭后永久保留静态队列，不以此代替硬件输入任务退出证据。44组主机/33项门禁/真实SDK隔离ELF通过，仍compile-only无相机连接证据；设备792c5409…未变，无需开关机。以下下一步计划为历史。

当前下一步：实现绑定当次epoch/id的新人工确认事件通道及独立设备入口，保证输入生产者有界停止、host事件串行消费/退出后不再触及队列，补入口异常/日志/产物门禁，再受控上板仅读取公开身份。同SDK扫描/延迟激活已完成并经43组主机和真实SDK隔离ELF验证，仍compile-only；没有新相机连接/型号证明，不启用配对/CCCD/状态写入。设备仍792c5409…、重编dac733a0…未烧；无需重复开关机。以下下一步计划为历史。

当前下一步：在既有SDK lifecycle内实现host串行的被动扫描与延迟Owner激活、当次确认事件及独立全流程期限；先覆盖取消/歧义/过期/host reset/退出失败并建立专用构建门禁，再受控上板只读身份。选择器及无IO交接已经测试/编译，但当前start前准备Owner不能直接复用旧扫描地址；禁止扫描销毁SDK后重新初始化或固定confirmed=true。本轮不要求开关机，设备仍792c5409…；新无连接09101d54…只重编未烧，完整回读/回滚证据保留。以下下一步段落为历史。

当前开发下一步：解决SDK callout生命周期的timer静默确认，补主任务独立watchdog、有界SDK stop/host exit/deinit与板卡清理，故障情况下保留资源；再集成明确候选选择的独立设备入口与新门禁。当前Owner仅完成链路级关闭，不能称无线已停或cleanup=1；不绕过compile-only拒绝烧写、不要求用户重复开关机。完成后先真机读取公开身份再建立兼容档案，电源/模式/配对仍不放行。以下下一步计划保留为历史。

当前开发下一步：实现独立受控连接Owner与host串行调度，管理候选选择、连接世代、断连/host reset、操作结束后有界关闭与资源保留；覆盖连接/超时/失败退出模拟，建立新专用产物门禁。现有操作层只完成三项身份读取，不允许手动布尔值放行Power/Operation；先取得实际身份/版本，再建立兼容档案与状态读取规则。当前无需重复开关机，设备保持扫描版，不绕过compile-only拒绝烧写。真实无线关闭负对照仍待做。以下开发下一步保留为历史。

当前开发下一步：在已测试的便携GATT核心上实现独立有界NimBLE只读诊断适配与故障测试，处理连接世代、晚回调、完整发现、分段读取和停止清理；先核实型号/实际固件再读取状态，不依赖固定句柄，不启用自动配对/通知/写入。编译及安全门禁后再受控真机采证。真正无线关闭负对照仍未执行，与相机仅关机明确区分；本轮不要求重复开关机，设备保持bd74b761…扫描版。以下下一步计划保留为历史。

当前下一步：两次同版人工启动扫描流程PASS，第二次用户只关机，尚无无线关闭负对照。请用户在菜单关闭无线通信工作/动作模式（不只是按电源键），确认条件后先采集再人工启动同版Pico，不重烧。非0候选仍不自动归因本台相机。参考Live-View源码核对已完成；后续优先独立GATT只读发现/型号版本/Power与Operation Mode诊断设计与测试，再受控上板，连接/配对/功耗另验。广播服务位不推导电源，不照搬固定句柄/配对参数/取景器唤醒策略，详见 `docs/GR4-BLE-REFERENCE-NOTES.md`。

当前下一步：相机蓝牙启用并靠近Pico；用户就绪后先开启无软件复位采集，再提示完整关机/开机Pico，核实本版扫描开始、候选/统计、END与20次一致结果、cleanup=1。0候选仅为观察结果，不认定机型不支持或触发自动配对。缺失关键证据不补造PASS，不重复重烧；Gate A型号/版本/配对另行开发。以下优先计划保留为历史。

当前优先：指定三图设备JPEG生成、严格已有库恢复、只读SD显示与用户验收已完成，结束同类重复开关机。广播候选模块已测试/编译，下一步Pico独立有界被动扫描与失败清理/专用门禁，然后真机采证。服务匹配不作为型号身份；Gate A配对/型号版本/bond恢复另行推进。设备固件不变，SD旧库及异常证据保留；首次源创建耐久性、产品导入、压力、相机同步未完成。以下优先计划与待启动段落仅为历史。

当前优先：严格已有库三轮人工关机/开机后恢复 3/3 PASS，结束同类重复操作。下一步准备用户三张照片的设备限额内测试副本与实屏验证路径（用户无法通过电脑建目录/复制，不依赖读卡器），原图及 SD 测试库保留；实际新显示固件需编译/门禁/备份/回读。现设备仍严格恢复固件，不是 RAM 探针；原厂回滚仍用 `.cache/pico-coldtest-CvkJeU/`。压力/写中断/拔卡/实屏/相机未验收；下述阶段计划保留历史。

第四轮新版已烧写，先开启采集，再人工关机/开机；最新完整备份是旧探针，原厂恢复使用 `.cache/pico-coldtest-CvkJeU/` 已核验的原厂 boot/app，不混淆两者。下面开发阶段计划已推进到烧写完成，仍不自动回滚或烧 SD/显示测试。

结果重播探针已完成主机验证与固件编译，等待用户确认替换设备旧探针；烧写前重新核对设备、备份与产物，烧写后先开启采集，再人工关机/开机。晚收到的摘要只证明应用进入与专用测试结果，不补造 ROM 复位原因或无干扰冷启动证据。未补齐门禁前不烧 SD/显示测试，不自动回滚。回滚可用本轮完整备份。见 `docs/BOOT-PROBE.md`、`docs/HARDWARE-RESULTS.md`；三张照片仅离线通过，个人文件不提交，不改 120 MHz/HPM、校准或格式化卡。
