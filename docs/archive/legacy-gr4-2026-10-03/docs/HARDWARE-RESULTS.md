# 上板验证记录 — 2026-10-01

## 项目停止边界 — 2026-10-03

资料最终状态：原始日志和图片已迁移至 `/Users/zhanghuilong/Documents/ChatGPT/R-Core-3-2-测试资料`，保留分类/原批次路径，清单含SHA256。112份固件备份/回读镜像按用户要求已从项目删除至 `/Users/zhanghuilong/.Trash/R-Core-3-2-firmware-backups-3CRIkV`（可恢复）；下文旧.cache路径和“备份保留”是删除前历史记录，不代表当前项目仍有备份。未操作设备。

用户取消项目并要求删除代码，已停止所有开发/调试/设备操作。取消前USB接收修正版d14d1093…完成boot/table/app写入及esptool三镜像哈希验证，尚未做整片回读或人工启动，不计本版真机接收/身份结果。完整烧写前16MiB备份和旧原厂/显示回滚随后按用户要求从项目删除，见上述最终资料状态；项目代码/构建产物也已移入可恢复的废纸篓，不恢复/擦除设备固件。后文全部为历史，不继续重测或执行“下一步”。

## 2026-10-03：USB接收修正已编译，备份已完成

独立RX适配修正无驱动O_NONBLOCK分支错误，每50ms最多32次单字节预读取、拒绝驱动混用。49/49主机46.67秒、37项门禁及真实SDK专用编译通过；app433232字节SHA256 `d14d10936aac5bb4f312f13ac247437216645c5c4f63ad3c567e790e740ecd5f`。新增测试直接编译锁定/摘要核对的原SDK函数和实际适配，仍是模拟FIFO/锁/连接，不计硬件USB或相机身份。SDK/BSP未改，用户新动作和5秒时限不变。

端口独占核对后按既有直接烧写授权完成完整16MiB备份，私有 `.cache/pico-gr4-usbfix-dIGslm`，仅备份default-reset进入下载器；此刻尚未烧写或获得本版启动/接收结果。此前confirm-04仅发送成功但无设备入队，旧负对照清理证据保留，不能套用到新接收版。备份/回滚核验、写入/整片回读后才安排现场TTY重测，不自动运行应用/代发y，不操作相机/SD或擦NVS/写校准。

## 2026-10-03：confirm-04已发送但固件未接收，定位无驱动VFS适配缺陷

用户终端顺序为新候选→y→“已发送一次人工回复”→20次结果。本轮 `.cache/pico-gr4-identity-6idB4M/manual-identity-confirm-04.log` 3867字节，SHA256 `bb6de60bc4c4cedd578c825409e1a7ffe2d402d30c1f4c43c0ec0983b5cec9dd`；30674ms完整提示，35177ms END/第一条RESULT，35177–44677ms共20条一致code11/flow6(EXPIRED)/owner无结果/lengths0,0,0，cleanup/sdk/input/board全1、start/wait/close全0，45177ms返回。没有GR4_CONFIRM_INPUT，没有APP_ENTRY或panic标记。日志首行前一轮45729ms返回残留，不合并解释为本轮两次启动或完整时序。核对开始时用户工具仍占端口，未同时开采集；诊断结束时工具自然退出、串口无占用。

此次不能沿用confirm-01“用户太晚”的解释。主机write成功只证明OS接受字节，不证明设备接收。固定SDK公开源码核对：当前worker打开/dev/usbserjtag采用O_RDONLY|O_NONBLOCK，VFS的read在non_blocking=true时跳过rx_func预读取，随后get_read_bytes_available仅查询安装后驱动ringbuffer；未安装驱动返回0，因此fetch_size=0、EWOULDBLOCK且没有调用硬件FIFO。实际专用ELF保留no-driver RX/read/availability函数，不含driver_install或vfs_use_driver。SDK commit及tracked-clean已核对，驱动/BSP源码未改。

私有离线 `probe_sdk_usb_read.py` 直接取固定SDK实际read与availability函数，用模拟已连接USB/无驱动/49字节就绪FIFO和锁替身编译运行，C目标ASan/UBSan：O_NONBLOCK连续100次EWOULDBLOCK、消费0字节；不设该flag时实际预读取分支每次读1字节、完整49字节后EWOULDBLOCK。SDK VFS源SHA256 `6331c98d9efc2f26de60ed450657ac3961bb5ef6fd84a4176f1550e7eb98685b`，driver源SHA256 `5b9e304dae1dc3f6ebf8f654fa9f14366b24319d5167a478153cfb1124ef0c5c`。这证明当前固定SDK条件分支的适配错误，不是实际USB物理发送/驱动线程并发测试；原48组mock read没有覆盖该分支，不能把其通过升级为接收功能PASS。

无有效确认到期清理的旧负对照证据仍保留，身份连接/Gate A未通过。此轮只诊断/离线复现/文档，未修改公共固件、重烧/主动复位、发送命令或操作相机/SD/NVS/校准。先修复独立接收适配与SDK级回归/门禁，再备份烧写后安排一次重测；不修改锁定SDK/上游驱动，不通过延长时限或自动代发确认掩盖问题。无需用户继续重复输入。

## 2026-10-03：交互工具完整提示可解析，用户回复过期，未连接

用户运行现场TTY工具并提供终端输出：两次USB打开、发现本轮候选、20次“收到诊断结果”，随后输入y，工具回复“未发送：输入无效、提示过期或本轮已尝试”。原始私有 `.cache/pico-gr4-identity-6idB4M/manual-identity-confirm-01.log` 3820字节、SHA256 `c282f92291e0fcb2831fcbb3570ae95a12c018bce0b299ac635455f7d7f2948a`：30674ms完整token提示；35677ms END code11/cleanup1与第一条RESULT，35677–45177ms全部20条一致code11、flow6(EXPIRED)、owner无结果、lengths0,0,0，cleanup/sdk/input/board全1、start/wait/close全0，verified/pair/write全0，随后应用返回。没有GR4_CONFIRM_INPUT。APP_ENTRY未捕获，不补记完整启动时序；终端打开次数不替代独立传输错误统计。

固件在提示约5秒后已结束确认阶段，工具的4秒保守发送期限更短；从用户终端顺序看y在结果之后，已经过期。因此未发送/未激活身份Owner符合安全门，不是相机连接失败。证明实际工具能解析完整当次提示、无有效确认会退出并清理；尚未验证人工有效回复/只读身份/连接后的退出。旧日志保留，不覆盖。本轮只读核对日志与记录，串口无占用，无重烧/主机主动复位/相机或SD操作，不延长确认期限或自动代发y。

重测使用尚不存在的 `manual-identity-confirm-02.log`，先工具显示USB打开再完整关机/开机Pico并重连；提前核对附近唯一相机是自己的GR IV，等新提示出现才在4秒内新输入y+Enter。不要提前输入（会被丢弃），不要等“收到诊断结果”（此时已太晚），不要复用本轮已过期token。

## 2026-10-03：首次无确认负对照及输入/SDK/板卡退出路径PASS

用户“准备好了”后先开120秒只读采集，已确认真实USB打开，再提示人工完整关机/开机/重连。用户随后“已开机”。原 `.cache/pico-gr4-identity-6idB4M/manual-identity-no-confirm-01.log` 为0字节、1连接、0传输错误，无标记；不能据此判启动失败、相机未广播或确认USB真实断电时刻。保留原空日志。

当前端口存在且无占用，保持现状进行一次45秒只读补采，无复位/发送/清输入。补采 `manual-identity-no-confirm-01-supplement.log` 3207字节、1连接、0传输错误，SHA256 `59d119876ccf66246486d80e09ca6f7b4f91e9a9a526dc7248e060f2f724a6f2`。30676ms只收到确认提示前半行；36579–45079ms共18条完全相同的结果：

```text
code=11 cleanup=1 sdk=1 input=1 board=1 start=0 wait=0 close=0
flow=6 owner=4294967295 lengths=0,0,0 verified=0 pair=0 write=0
```

45579ms应用返回。flow6在本版对应EXPIRED，owner为无结果标记，不是Owner成功枚举值；code11是本轮有意不确认导致未完成身份流程，不能解释为身份证据或单独相机故障。结合实际入口只有host到期检查/当次ACCEPT才能激活、只读采集器禁止任何USB发送，支持本轮无回复→到期→不激活身份Owner，且结果发布前输入IO退出/同SDK分阶段关闭和PMU/I2C清理成功。只证明本次无连接负对照及软件退出路径，不证明真实连接后的退出、任务TCB完全回收、独立射频/功耗或电气掉电。

APP_ENTRY/END、完整token提示和前2条结果未捕获，不称20条完整、完整启动时序或确认工具已端到端验证。原0字节采集不是另一次独立启动，两份日志不计两轮成功。没有GR4_CONFIRM_INPUT；工具根本未发确认。采集已结束，串口释放。设备仍已烧写/整片核验d4f2ccaf…，本轮仅采证/文档，未重烧、主机主动复位/JTAG、相机控制/配对或SD/NVS/校准操作；48组主机及固件编译沿用实现轮。

下一步：另次启动由用户运行交互TTY工具，完整新提示后现场确认附近唯一目标就是自己的GR IV，再新输入y+Enter才允许一次只读身份读取。不能拿本轮过期提示/此前聊天室允许自动代发；完整提示可解析性、实际身份/断连与连接后的SDK清理、型号版本及Gate A仍未通过。

## 2026-10-03：身份诊断已烧写、整片回读通过，待人工启动

用户回复“已重连”；本机Pico USB重新枚举，端口无占用。实际专用门禁复核通过，app432992字节SHA256 `d4f2ccaf519e55d913cd36c095eb88d0d0ef457618bf9df2796fc58c06c29380`。本轮完整16MiB私有备份 `.cache/pico-gr4-identity-6idB4M/pretest-flash.bin`，SHA256 `760cf58c1bf421aa8e83f66b4d85954caf0af540c59dca2ac375e55cbf93e68a`。与此前精确回读核对当前792c5409…boot/table/app及全部app后历史尾部；核对已验收显示13117988…和原厂完整备份3ff43d39…/回滚，生成本轮旧镜像回滚和摘要清单，私有文件不提交。

依此前“直接烧写不用确认”授权，按专用生成flash_args写boot21168/table3072/app432992，三目标全部Hash verified。擦写仅0x0–0x5fff、0x8000–0x8fff、0x10000–0x79fff；不额外清历史尾部/NVS/storage。随后完整16MiB回读 `post-write-flash.bin`，SHA256 `a2bed5e6fd11573d435372d826deda9fdb2c944d25628626cb1fa50e277d4cdf`：三镜像逐字节一致，擦页尾FF核对通过；0x6000–0x7fff、0x8000–0xffff（包含分区表/NVS/PHY）及0x7a000–0xffffff（包含历史app尾部/storage/其余Flash）与本轮备份相同。这里只证明烧写和保护字节，不证明新应用启动、USB退出或SDK清理/相机身份。

备份仅default-reset进入下载器；写入/回读before/after均no-reset，不自动运行新应用。所有工具已退出，lsof确认串口无占用；Pico留下载器。相机未操作/连接，无SD/校准写入、NVS擦除或JTAG；软件48组/37项测试沿用实现轮，本轮不新计主机测试。下方未连接/未烧写段落为历史。

下一步用户就绪后先开启只读120秒日志采集，再人工完整关机/开机Pico并重连。这轮不发送任何确认，先验证“无新确认不连接”、输入任务与SDK/PMU/I2C退出。只有该轮真实结果可核对后，才另次启动交互TTY工具作现场新确认并读取公开身份；未经本轮确认不自动连接，不配对/写入，不把候选/长度当型号认证。屏幕不刷新是预期，日志/用户启动动作与独立电气掉电证据区分。

## 2026-10-03：只读身份入口软件就绪，Pico未连接，未新增上板验证

48组主机CTest/37项门禁及真实SDK专用编译通过，app432992字节SHA256 `d4f2ccaf519e55d913cd36c095eb88d0d0ef457618bf9df2796fc58c06c29380`，USB生产者/现场确认工具/有界退出与独立入口已实现。默认359440字节dd5ebff0…、无连接417296字节e167e55f…重编/门禁通过但摘要与之前不同，均未烧写；不套用旧真机PASS。普通compile-only仍拒绝、无.bin，混用模式真实CMake拒绝。SDK/NimBLE/BSP tracked-clean。

仅本机只读端口枚举：开始一度列出usbmodem2101，随后lsof已不存在，两次pyserial枚举只有其他三端口，无Pico USB。未打开串口、复位、备份/烧写、连接相机或操作SD/NVS/校准。最后设备核验仍792c5409…无连接诊断，旧私有回读和回滚保留；需人工重新插好Pico后才能继续备份/烧写。本版身份/USB退出/SDK清理/功耗均无真机结果，详见 [只读身份诊断](GR4-IDENTITY-DIAGNOSTIC.md)。

## 2026-10-03：确认队列开发验证，非新增上板测试

44组ASan/UBSan、门禁33项与实际固定SDK隔离ELF通过；新增当次确认消息队列/host消费及关闭后静态保留，未接实际输入驱动/新动作或相机连接入口，仍compile-only无.bin。正式基线 `build-default` 359568字节b5fe671e…与 `build-gr4-radio-final` 417296字节dac733a0…重编/门禁通过、均未变/未烧；旧辅助配置不同摘要2875ac7d…/3c23578e…另记、不套用真机PASS。设备仍此前792c5409…，没有本轮串口/复位/烧写/相机/SD/NVS/校准操作；旧回读/回滚保留，SDK/BSP未改。详见 [确认消息通道](GR4-CONFIRMATION.md)，目前无需操作设备。

## 2026-10-02：同SDK扫描/延迟激活开发验证，非新增上板测试

43组ASan/UBSan、门禁33项与真实固定SDK隔离组件/ELF、默认/无连接重编通过；新增host扫描/确认API/一次激活和85秒期限/退出屏障，现场确认输入与设备入口未启用。没有串口/复位/烧写、相机连接/操作或SD/NVS/校准操作，SDK/BSP未改。重编无连接417296字节SHA256 `dac733a08bd932923eb5373fb14cb7c13b961ae1f2cf1d55aa012d80fb61f07e` 未烧，不能沿用旧版真机PASS；设备仍792c5409…及此前私有回读/回滚证据，详见 [同SDK会话](GR4-SCAN-SESSION.md)。当前无需重复开关机。

## 2026-10-02：无连接无线栈首次人工启动生命周期PASS，17条一致结果

先采集就绪后提示拔USB、完整关机/开机Pico并重连，用户确认“已开机”。沿用已烧写/完整回读的792c5409…诊断版，相机无需操作。本轮120秒私有 `.cache/pico-gr4-radio-EvRlhj/manual-power-radio-01.log` 2145字节、2连接/226传输错误（含人工断开/USB重枚举重连尝试，不等同芯片故障），SHA256 `66a8fc8dfa93004294f5d7838e78633c833308742bf0b78b28db0491b9d5c7ad`。

5164–13164ms每500ms一条，共17条完全一致 `GR4_RADIO_RESULT code=0 cleanup=1 sdk=1 board=1 start=0 wait=0 close=0 scan=0 connect=0 pair=0 gatt=0`；13664ms `Returned from app_main()`。根据已核对的专用入口，start/wait/close0与sdk1/board1只会在本次成功启动/同步、有序stop和host退出、controller先disable、SDK timer静默/排空/删除核对、host/controllerdeinit状态核对、PMU/I2C清理后发布；确认本次无目标无线栈生命周期成功。不是独立电流/射频关闭测量，也不证明失败分支或任意并发安全。

未捕获ROM启动头、APP_ENTRY/STARTED/END和前3条结果，不能补记为完整20条或完整启动阶段日志；17条是同一结果重播，不是17轮运行。人工电源操作依赖用户确认，不作独立电气掉电证明。无扫描/目标/相机连接/GATT/配对、SD或图片刷新；保持旧屏幕属预期，不用画面证明应用执行。

采集已完成，进程退出、串口释放。本轮只采证与更新文档，没有重烧/主机主动复位/JTAG/SD/NVS/校准/相机操作；主机40组/编译/门禁沿用实现轮，不称本轮重新执行。当前设备仍无连接诊断792c5409…，显示/扫描回滚保留。下一阶段仅准备受控只读身份目标选择和独立设备入口，不因本次PASS自动启用连接、配对或其他相机控制，见 [无线栈方案](GR4-RADIO-LIFETIME.md)。

## 2026-10-02：无连接无线栈诊断烧写/扩展回读通过，等待人工启动

新增独立生命周期与固定SDK公开NPL timer guard，controller先停→timer静默/队列预排空→hostdeinit核对→controllerdeinit；初始化/会话/停止/清理分别有外部期限，超时不强杀或复用对象。普通身份Owner仍compile-only；本专用入口没有目标/扫描/连接/GATT/SD/EPD/VCOM访问，关闭SMP/绑定持久化/PHY校准持久化及SDK默认自动连接重试/隐私/DTM。初版默认重试保留connect符号，未烧；最终fresh profile不含连接启动符号。主机最终40/40 ASan/UBSan（37.75秒）、门禁31项/采集18项、真实SDK默认/专用编译通过；不是实际并发、射频或相机证明。

按此前“直接烧写不用我确认”授权，检查端口无占用后，default-reset进入下载器读取完整16777216字节，103.9秒。私有 `.cache/pico-gr4-radio-EvRlhj/pretest-flash.bin` SHA256 `2c4f6224523cb5e6183eb165ee126369515535471a9866d85e4a1918f17443fa`。当前bd74b761…扫描boot/table/app、历史混合尾部、相同分区/已验收13117988…显示回滚与原厂备份核验通过；保留本次raw boot/4MiB app与精确有效扫描/显示回滚，不放宽原始含历史尾部的镜像检查。

专用app417296字节 SHA256 `792c54093a6a115dda12db21ce7b4e144f89315639ab7710bfee5a94ec1519db`，boot21168/table3072；三镜像写入Hash verified。4259840字节/0x410000范围回读24.4秒，全部三镜像逐字节一致，SHA256 `3904b6161115bfc6740c1336b67ae1774c96fc8dc65cae07aa6989c24bae113c`。核对0x6000–0x7fff、0x8000–0xffff及0x76000–0x40ffff与完整备份相同；不额外擦旧尾部。回读未覆盖0x410000之后，因此不称整片Flash回读；烧写目标仅boot/table/app，没有storage/NVS erase或SD操作。

烧写/回读before/after均no-reset；工具已退出，lsof无串口占用，设备留下载器。没有新应用启动/SDK stop/deinit结果；屏幕未刷新，不能凭旧画面判运行。未连接或操作相机、JTAG或PMU校准；SDK/BSP tracked-clean。默认359568字节 SHA256 `b5fe671e71fa11dc63b3c25bf570fd39e82f79baa757045cbb88339462344219` BLE/IO关闭、门禁通过但未烧；旧扫描与显示回滚完整保留。

下一步用户就绪后先开启只读采集，再提示完整关机/开机Pico并重连；本轮相机无需操作，不进入配对。观察 `GR4_RADIO_END` 和同一启动20次一致 `GR4_RADIO_RESULT code=0 cleanup=1 sdk=1 board=1 start=0 wait=0 close=0 scan=0 connect=0 pair=0 gatt=0` 及应用返回；失败保留、人工完整关机后处理，不继续相机连接。无连接诊断即使通过也不计身份、各型号版本、功耗或Gate A。实现与限制见 [无线栈退出方案](GR4-RADIO-LIFETIME.md)。

## 2026-10-02：第二次扫描流程PASS，用户澄清仅关机；不是无线关闭负对照

采集先就绪，再人工拔USB/完整关机/开机重连，用户“已开机”。原提示要求关闭相机蓝牙，但收到候选后追问，用户明确“只关闭了相机电源”。因此本轮重分类为相机关机条件，不声称菜单无线关闭或负对照通过；保留原采集文件名及摘要。

120秒私有 `.cache/pico-gr4-deadline-xsURqd/manual-power-gr4-deadline-radio-off-01.log` 4054字节、2连接/273传输错误（含断开/重枚举尝试，不等同芯片故障），SHA256 `56c494bd92b9cc5fb1d305c3712c19e5e4e94f2ad196464bd01923afd35f91c6`。30659ms完整候选id1/services2；30660ms COUNTS reports1581/invalid0/matches50/overflow0/candidates1及END code0/cleanup1。30660–40161ms全部20次一致结果 end=deadline/elapsed_ms30000/inactive1/stats1/code0/cleanup1，同一统计；40661ms应用返回。确认第二次同版扫描截止/取消后的inactive及清理流程通过。服务位2是Bluetooth Control，前轮1是Camera Information，不能以服务位变化判开关机编码或同一设备身份；临时id1不跨会话绑定。

APP_ENTRY/STARTED/启动头未捕获，时间/停止/清理来自实现结果，不是独立电气掉电或射频测量。采集结束释放串口，本轮无主机复位/重烧/JTAG/SD/NVS写入或相机连接/控制，固件仍bd74b761…及旧照片；主机32组/编译沿用实现轮。

官方GR IV FAQ确认无线开启时关机仍发送蓝牙；用户提供Live-View仓库只读核对发现其在连接/协议识别后读取Power与Operation Mode，不按广播服务位判电源。参考项目有用户修改，未修改或运行其项目，未移植固定句柄/配对/WLAN/拍摄功能。来源、条件修正和后续只读诊断约束见 [参考核对](GR4-BLE-REFERENCE-NOTES.md)。真正无线关闭对照及身份/配对/功耗仍未验，下方首轮/待启动记录为历史。

## 2026-10-02：独立截止修正版首次人工启动扫描流程PASS

用户“准备好了”后不复位采集先capture ready，再提示拔USB/完整关机/开机重连，用户确认“已开机”。120秒私有 `.cache/pico-gr4-deadline-xsURqd/manual-power-gr4-deadline-01.log` 4054字节、2连接/223传输错误（含人工断开/重枚举重试，不等同芯片故障），SHA256 `cf8942d84a6416b84fc0819a9da60a507e5612292d8cf3e2d593cd9e9102e964`。

30662ms完整候选id1/services1/model=unverified；30663ms完整COUNTS reports1618/invalid0/matches55/overflow0/candidates1、END code0/cleanup1。30663–40164ms全部20次一致结果 code0/cleanup1/passive1/connect0/pair0/end=deadline/elapsed_ms30000/inactive1/stats1，同一完整统计；40664ms应用返回。确认应用独立期限及显式取消后inactive、host/SDK及PMU/I2C清理路径通过；匹配55份报告只合并成1个临时候选，不能称55台相机或已建立连接。

未捕获启动头/APP_ENTRY/STARTED，不补造完整启动采集或独立电气掉电证据。elapsed_ms是应用tick时间，inactive与cleanup是实现/API确认，不是独立射频时间/功耗测量。服务位1只是Camera Information协议候选，不证明GR IV型号、版本、稳定身份或属于该待配对相机；尚无相机关闭蓝牙对照、重复会话/功耗和Gate A配对结果。旧99c…code9失败保留，不改判。

采集已结束、串口释放；本轮只采证/文档，无主机复位/重烧/JTAG/清SD/擦NVS/校准或相机操作，主机32组与编译沿用实现轮。设备保持bd74b761…扫描版及旧照片，下一步用户关闭相机蓝牙后做同版对照，不自动连接/配对。下方待人工启动为历史。

## 2026-10-02：独立截止修正版烧写/扩展回读通过，待人工启动

修复仅在相框扫描适配层：30秒tick截止、显式取消与inactive核实；不依赖Observer-only SDK结束回调，仍关闭连接/配对。SDK提前结束/host提前退出/复位、取消失败或仍活动拒绝成功。聚合统计只在安全退出后复制、地址清空；20次结果增加结束方式/耗时/停止确认/完整计数，方便晚采。最终32/32 ASan/UBSan（35.62秒），入口28场景含无SDK回调/空扫描/回绕/非事件唤醒/晚报告禁收/20次完全相同，门禁26项、采集17项、默认及专用v6.1编译通过。不是实际无线或并发压力证明；SDK与BSP工作树干净。

完整16MiB备份 `.cache/pico-gr4-deadline-xsURqd/pretest-flash.bin`（103.9秒），SHA256 `b7de1f2cd7675cf9a3c430e3b26384896fa02012c82f97d5c794b82989bd7431`。当前99c1109a…boot/table/app与上次扩展回读、完整历史尾部/相同分区、已验收显示13117988…及原厂回滚核验通过。保留raw boot/4MiB应用分区，另精确提取有效 `rollback-scan-boot.bin` / `rollback-scan-app.bin`，并复制已验收显示回滚；含混合尾部原始分区不放宽单镜像检查。

三镜像Hash verified；新app391568字节SHA256 `bd74b761f79c99a84f7b62f17d26d4aa0dcc265086ab65f1407f956ebdd704d3`，boot21168/table3072。0x410000回读4259840字节 `post-write-prefix.bin`，SHA256 `a8195c2b85845fc29887470841ade107b0d74304bebd081941acda94f7a682c1`，三镜像精确相符。0x6000–0x7fff/0x8000–0xffff/0x70000–0x40ffff与本轮备份一致；历史尾部不额外擦除。默认359440字节2fc5a127…未变/未烧。

备份default-reset进入下载器，写入/回读before/after no-reset；工具退出，串口释放，设备留下载器。未清SD/擦NVS/写PMU校准/连接JTAG或操作相机，新版未运行，无本版候选/30秒截止/清理实机结果。下一步用户就绪→不复位采集就绪→人工启动；相机保持蓝牙启用/待配对，不确认配对，屏幕保留旧照片属诊断预期。旧99c…code9整轮仍失败，不改判通过。

## 2026-10-02：被动扫描收到候选，但35秒结束等待失败；已定位SDK条件

本版99c1109a…用户确认“已开机”，另说明相机待配对。首窗 `manual-power-gr4-scan-01.log` 120秒、0字节、1连接/0错误，无标记，SHA256 `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`。文件创建12:03:20，当前USB节点12:09:39，支持采集窗口先结束/晚重连的解释，不能据空日志判启动失败。

不复位补采 `manual-power-gr4-scan-late-01.log` 120秒、1569字节、1连接/0错误，SHA256 `f8aaf4b35fab25f0a3b5457c92ab33b4194aa28be2d7e1bb619a970b5d5a7473`。35656ms `GR4_SCAN_CANDIDATE id=1 services=1 model=unv`半行拼到36707ms结果；之后至45207ms共18次相同 `code=9 cleanup=1 passive=1 connect=0 pair=0`，45707ms应用返回。仅证明至少一个Camera Information协议候选，以及停止/SDK/PMU/I2C清理分支成功；候选不是型号身份。没有APP_ENTRY/STARTED/完整统计/END、20次完整捕获或独立电气掉电证据，30秒正常结束未通过。

本地SDK源码定位：NIMBLE_BLE_CONNECT为Central或Peripheral，而 `ble_hs_timer_exp` GOOD分支的 `ble_gap_timer()` 被其条件编译包住；本版仅Observer，传统扫描结束定时处理未执行，35秒后应用超时与日志吻合。未启用连接角色或改SDK；适配层独立截止/取消确认尚未实施，不把code9改判PASS。两次采集结束/串口释放，本轮仅采证、源码核对和文档，无主动复位/重烧/JTAG/SD/NVS写入或相机控制；主机32组/编译沿用此前，不新增真机兼容结论。见 [扫描问题与后续修复](GR4-PASSIVE-SCAN.md)。

## 2026-10-02：有界被动扫描版烧写/扩展回读通过，无线运行待人工启动

按此前“直接烧写不用我确认”授权，确认串口无占用后读取完整16MiB备份，default-reset进入下载器；私有 `.cache/pico-gr4-scan-bQRWDb/pretest-flash.bin` SHA256 `4308468a3a7306026bae611c3799b85fca0c8451a5710b6319d4acfc805053f6`。逐字节核验当前已验收显示boot/table/app（13117988…）、相同分区/历史混合尾部/原厂备份与回滚。保留原始boot/4MiB app分区，另精确保存有效 `rollback-display-boot.bin` / `rollback-display-app.bin`；原始含已知尾部分区不作为单个镜像放宽检查。

扫描模式只Observer，关闭连接/广播/SMP、PHY结果NVS持久化；不初始化SD/EPD/完整BSP，不读写VCOM，仅PMU握手。主机32/32 ASan/UBSan（36.51秒）、入口22种模拟场景、门禁26项/采集17项、默认及专用v6.1编译通过。早期BT依赖配置及SDK路径空格构建被拒绝，未烧写；最终使用SDK同commit `fff9895c82d744c7237be8847347bdd1b07c6643` 无空格临时副本，未修改其源码或板卡子模块。默认359440字节SHA256 `2fc5a12739de6e48ba0ce801bbba2da5d9e94928a76c878acbd4c321c2e3e7b9`，未烧。

首写app390512字节ae0358…三镜像校验/扩展回读通过，私有 `initial-write-prefix.bin` SHA256 `e38a792fcbc9104970ea635c40b544c3a406010ed2ba3295d6f2a8e16a41ba64`，应用未运行。收紧SDK初始化失败不假定其内部回滚成功的cleanup输出，入口测试/编译/门禁后替换。最终app390480字节SHA256 `99c1109ad60730d921e868a21cd43daeff803bfe7baf945b1aaf27b367d68d33`，boot21168/table3072；三镜像均Hash verified，0x410000回读4259840字节逐字节相符，私有 `post-write-prefix.bin` SHA256 `abe30d07bf0df5807c8f48b8d4e577ac09328eb209cc62710dd1359bb96f69aa`。

0x6000–0x7fff、0x8000–0xffff、0x70000–0x40ffff与本轮备份一致；新app明显小于JPEG版，全部旧应用尾部原样保留，不额外擦除。两次烧写/回读均before/after no-reset，无主机应用复位/JTAG/清SD/NVS擦除/PMU校准或相机操作。设备留下载器、串口释放，尚未执行无线模式；不存在本轮扫描发现/30秒结束/清理结果或型号识别证据。下一步先采集就绪再提示人工开机，不把主机mock或Flash相符当Gate A通过，见 [被动扫描诊断](GR4-PASSIVE-SCAN.md)。

## 2026-10-02：用户确认JPEG生成缓存SD三图实屏无问题，验收通过

在本版首轮人工启动mask7/code0、20次一致总结果及应用返回取得后，询问三张图比例/铺满/竖条/残影，用户回复“没有问题”。本次视觉确认对应 `jpeg-cache-test` 的设备生成缓存SD显示，不是旧RAM库验收；结合本版严格恢复预检查/逐张加载校验/GC16总状态，指定三样片闭环完成。本版app SHA256 `13117988f798cca26aff730a7028d26d4cea4fd77bdc0669804a5b9e5618a105` 保持，第三张保留。

首图单张串口日志缺失不改写为完整；三图整轮由本版总状态核实，视觉由用户本轮确认。本轮仅同步验收文档，无新实现/主机测试/固件编译/设备连接/复位/烧写/清卡/校准或相机操作。旧竖条/坏缓存根因、首次源独占创建、压力/拔卡/满卡/长期可靠性、正式产品导入和GR IV链路仍待验证。下文视觉待确认/待启动为阶段历史。

## 2026-10-02：JPEG生成缓存只读SD三图显示人工启动整轮PASS，视觉待确认

采集先就绪，提示拔USB、完整关机后开机重连，用户确认“已开机”。本版app SHA256 `13117988f798cca26aff730a7028d26d4cea4fd77bdc0669804a5b9e5618a105`，沿用上轮完整回读核验。首个120秒私有 `.cache/pico-jpeg-cache-display-gkvrVA/manual-power-jpeg-cache-display-01.log` 138字节、2次连接/499次传输错误（含人工断开/重枚举尝试，不等同芯片故障），SHA256 `207e9050d19145d5a1cf1538611bd30ff98824080b8ba22f347b06e9eb3c1a49`。仅18284ms READY sequence=6 source=existing-only，确认已有三JPEG/缓存/索引严格预检查成功，但单独不足以判显示PASS。

随后无复位补采120秒，私有 `.cache/pico-jpeg-cache-display-gkvrVA/manual-power-jpeg-cache-display-late-01.log` 2924字节、1次连接/0错误，SHA256 `6d8a56c6a438baa187d79f42f5d2c68debc56caeee2b50f8f61ee6c4bc00ffc3`。图1仅24436ms READ_OK半行，与图2行拼接，尺寸耗时及单张刷新结果缺失，不能宣称其完整日志取得。图2 61927ms完整READ_OK/read6158ms/source=SD；63269ms SAMPLE_RESULT result=0/31°C/copy299ms/refresh459ms。图3 99435ms READ_OK/read6166ms/source=SD；100777ms SAMPLE_RESULT result=0/31°C/copy299ms/refresh459ms。

100777ms END r=1 mask=7 code=0；100777–110278ms全部20次结果均 `r=1 done=1 mask=7 code=0 read_only=1`，110778ms应用返回。mask只在相应缓存完整加载/精确像素及元数据检查、复制、卡及电源温度门禁、GC16返回成功后置位，因此本次三图整轮运行成功，首图缺失由本版总状态核实，不由旧版或单条READ_OK推断。20次是同一清理后结果重播，不是20轮显示。无生成/修复/提交或嵌入图片兜底，第三张保留。

两次采集结束、串口释放，本轮仅采证/记录，无主机主动复位/烧写/JTAG、SD清理/修复、校准或相机操作。主机30组/门禁/默认及专用编译沿用上轮。未捕获启动头/APP_ENTRY，不证明独立电气掉电。用户尚未确认本轮比例/铺满/竖条/残影，不能把串口成功或旧RAM库视觉验收当新JPEG生成缓存实屏验收；首次源独占创建、长期可靠性、压力/拔卡/满卡、正式产品导入及GR IV链路另验。下节待启动为阶段历史。

## 2026-10-02：JPEG生成缓存只读SD显示版烧写/扩展回读通过，待人工启动

独立jpeg-cache-display模式：严格已有三JPEG/缓存/固定索引恢复预检查，再逐张仅从固定jpeg-cache-test暂存加载，头/长度/CRC/完整像素/绑定及最终元数据全部通过才原有像素映射→卡与温度电源检查→GC16。前两图30秒停留，最后图3保留；失败停止、不解码/创建/修复/提交或嵌入兜底，释放缓冲后20次短结果重播。最终CTest30/30 ASan/UBSan（44.21秒）、固件门禁24项、采集器16项、指定三图主机已有库恢复+三暂存比较通过，默认/专用v6.1编译通过；未改SDK/BSP/驱动/波形/校准/像素算法。详见 [JPEG-CACHE-DISPLAY.md](JPEG-CACHE-DISPLAY.md)。

专用app3189776字节，SHA256 `13117988f798cca26aff730a7028d26d4cea4fd77bdc0669804a5b9e5618a105`；默认356560字节SHA256 `508870f7e407d917307304115c4f55239fdc937b6f8b82e09b9d39b01a6112ae`未烧，旧恢复91830…和生成状态f34b…产物保留未改。完整16MiB `.cache/pico-jpeg-cache-display-gkvrVA/pretest-flash.bin` 103.8秒，SHA256 `465b12d6e91265749c1b1dda36049537e15a883650e9acc7e88f77c65ea7ecd3`；六素材/当前恢复镜像/分区/已知混合尾部/原厂回滚通过。首次提取检查拒绝将含旧尾部的原始4MiB分区当单镜像，烧写尚未发生；保留raw分区，另按已完整匹配的有效旧app范围提取 `rollback-recovery-app.bin` 并验证镜像校验和/摘要。不放宽通用镜像检查器、不擦尾部，旧boot/raw app/有效旧镜像与原厂回滚均保留。

按既有直接烧写授权，三镜像Hash verified；0x410000扩展回读4259840字节（24.5秒），boot21168/table3072/app3189776逐字节一致，SHA256 `ab26083c31672fe817e783663f2cdd2f9ab3e6538fa850d08df85cf9da5529ca`。0x6000–0x7fff/0x8000–0xffff/0x31b000–0x40ffff与本轮备份相同；0x31b000–0x31bfff已知历史尾部保留，不额外擦Flash/NVS。备份进入下载器使用default-reset，烧写/回读before/after均no-reset。

工具退出、串口无占用、设备留下载器，新应用未启动，无本版SD读取/GC16/视觉证据。不清卡/修复文件/擦NVS/改校准/操作相机或连接JTAG。下一步先采集再人工开机，期待三个READ_OK/GC16 result=0、释放后mask7/code0及应用返回；视觉仍需用户确认，不能沿用旧RAM库显示验收。上轮严格恢复PASS保留，下文为阶段历史。

## 2026-10-02：已有 JPEG / 生成缓存 / 索引严格只读人工启动恢复 PASS

120秒被动采集已结束，2次连接、206次传输错误（含人工断开和重枚举重连尝试，不等同芯片故障）。工具退出、串口无占用，最终日志摘要不变；完整结果已取得，不追加JTAG查询。

被动采集先确认端口就绪，再提示拔 USB、完整关机后开机重连，用户确认“已开机”。本轮固件为已回读核验的恢复版 app SHA256 `91830c393633d51a65e596fbdc99591f4bd198a9925294d78e3df41849d21326`。私有 `.cache/pico-jpeg-recovery-2fT36M/manual-power-jpeg-recovery-01.log` 2025字节，SHA256 `db050ec5a1b09fe8989a079b1d775d1c6303a884f111978e9db93758df04e6dd`。

18292ms `JPEG_RECOVERY_PASS sequence=6 source=existing-only; no create/repair/decode/commit/photo refresh`；18293ms `JPEG_RECOVERY_END; read-only`。18293–27793ms全部20次总结果一致：`r=1 done=1 mask=7 seq=6 code=0 read_only=1`；28293ms `Returned from app_main()`。20次为同一清理后结果重播，不是20轮恢复测试。

结合已核验的只读固件路径，确认本次已有三 JPEG 完整字节比较、正确库绑定/相同有效双快照/空 journal/三 KEPT/照片序号1及global6、三生成缓存完整比较、缓存读后源再比较及最终元数据复核通过。应用未创建/补写/修复/解码/提交或刷新照片，屏幕保持原图为预期；不借准备流程重建状态。此证据对应本次人工操作确认后的已有文件恢复，不证明独立电气掉电、首次源独占创建、长期耐久性、生成缓存实屏验收、正式产品导入或 GR IV 链路。

启动头/APP_ENTRY未捕获，但恢复专用结果及应用正常返回足以证明本版应用执行。仅采证与文档，无主机主动复位/烧写/JTAG暂停、SD清理/修复、校准或相机操作；主机29组/门禁/编译沿用上轮。下一步接已有 jpeg-cache-test 的只读 SD 三图显示，不能沿用旧 cache-ram-test 的视觉验收。下节烧写待启动为阶段历史。

## 2026-10-02：JPEG/生成缓存严格只读恢复版烧写与扩展回读通过

新增独立jpeg-recovery模式：已有三源完整比较、相同有效双快照/正确绑定/空日志/三KEPT照片序号1与global6、三缓存完整比较、缓存读后源再比较及最终元数据复核；无缺失补写/重建/修复/解码/提交或照片刷新。独立56字节恢复状态，启动清零、释放缓冲后结果发布/20次500ms重播，不沿用旧生成状态布局/身份。主机29组ASan/UBSan、固件门禁23项、采集器15项通过；实际入口涵盖失败停止/释放/终态不变，真实文件测试涵盖缺目录/文件/快照、损坏/错绑定/序号/ID/状态/删除意图、非空日志、链接、介质失效与源/元数据读中变化，共用RAM恢复/加载回归通过。私有三图主机已有库严格恢复PASS，不计设备证据。默认/专用ESP-IDF v6.1编译/门禁通过，专用产物未链接解码、准备、media_open/create/import/action/checkpoint和照片刷新入口；未改SDK/BSP/驱动/波形/校准或时序。

恢复app3180064字节 SHA256 `91830c393633d51a65e596fbdc99591f4bd198a9925294d78e3df41849d21326`，ELF状态 `3fc94cfc 00000038 D frame_jpeg_recovery_status`；旧build-jpeg-status f34b…保留未改。默认回归app356672字节 SHA256 `eef4e64edfde8f20e962645cf01b98fbee0c22ba74d9080efbdf63c72808361e`未烧。

按既有直接烧写授权，完整16MiB `.cache/pico-jpeg-recovery-2fT36M/pretest-flash.bin`（104.1秒）SHA256 `da72bff70fe64e2be4844b0601f6b6325553e52ff81e405e889c4cf766c21368`；六份素材/状态布局/旧JPEG镜像/分区/混合尾部/原厂回滚核对通过，当前旧boot/app回滚副本提取并完整核验。三镜像Hash verified；扩大0x410000回读4259840字节，boot21168/table3072/app3180064逐字节一致，回读SHA256 `be9ff453f6ca86e7cc69c5a1da86f7d6520aaf8b7e623cb37bcc4761372976ad`。0x6000–0x7fff/0x8000–0xffff/0x319000–0x40ffff保护字节与本轮备份相同；新app较小，旧0x319000–0x31bfff尾部保留，不额外擦Flash/NVS。

备份进入下载器使用default-reset，烧写/回读before/after均no-reset。工具退出、串口释放，设备留下载器，新恢复应用尚未执行，无本轮应用SD写入/读取/恢复/显示结果，不清卡/修复文件/改校准/操作相机，也未连接JTAG。下一步先采集再人工关机/开机，期待JPEG_RECOVERY_PASS sequence=6 source=existing-only及清理后重播；屏幕保持预期。严格跨关机恢复及生成缓存SD显示未通过，不把上轮生成模式library=existing PASS当只读证据。见 [JPEG-RECOVERY.md](JPEG-RECOVERY.md)。

## 2026-10-02：JPEG状态版人工启动三图／索引总流程PASS

先打开采集确认就绪，再提示拔USB、完整关机／开机重连；用户确认“已开机”。120秒私有 `.cache/pico-jpeg-status-BFZwdX/manual-power-jpeg-status-01.log` 2503字节、2次连接／209次传输错误（含断开／重枚举尝试，不等同芯片故障），SHA256 `333c1f33c64a2443e10eea0c312b29485d88d145bc220a48812088964f8bafba`。本版 app SHA f34b1213…，启动头／APP_ENTRY仍缺，但本版专用结果重播及完整应用结果已捕获。

三张完整JPEG_SAMPLE_GENERATED均source=SD、scale=0：图1 12336ms、1562×1600、elapsed=11023ms；图2 23582ms、1571×1600、11245ms；图3 34331ms、1571×1600、10748ms。43663ms JPEG_SAMPLES_PASS sequence=6 library=existing，43664ms JPEG_SAMPLES_END。43664–53164ms取得全部20次JPEG_RESULT，字段均 r=1 done=1 mask=7 commit=1 seq=6 origin=2 code=0；53664ms Returned from app_main。

结合已核验固件路径，确认本次三源完整比较→SD文件JPEG解码→Cover生成→可信参考包完整比较→源再比较，设备生成缓存的完整持久化比较／绑定／KEEP／checkpoint／同进程重开及实际序号6核验通过，媒体关闭与缓冲释放后总结果通过。20次是同一结果重播，不是20轮JPEG/SD测试。本版不刷新照片，屏幕保持为预期。

library=existing只证明缓存库本轮打开时已存在，不反推旧版总流程PASS，也不证明本轮三个源均新建。已有文件可以被复用，准备流程仍可能独占创建缺失文件或checkpoint；本轮不是严格只读跨关机恢复或独立电气掉电证据，不计源首次创建耐久性、生成缓存显示、长期可靠性、正式PFU/PFI产品导入或相机链路通过。

采集已结束、串口释放。本轮仅采证／更新记录，无主机主动复位、烧写、JTAG暂停、SD清理／修复、校准或相机操作；设备应用可能写专用诊断目录。主机27组/编译沿用上轮。完整结果已取得，不再追加JTAG查询；新查询脚本仍未真机验证。下一步严格只读已有JPEG／生成缓存／索引恢复，验证后再接生成缓存SD显示。下节待启动为烧写历史。

## 2026-10-02：JPEG可查询结果版烧写／回读通过，真机执行待验证

为补旧版串口截断造成的总结果缺口，新增64字节内部DRAM状态、启动清零、逐图进度/首错、实际commit序号与库来源、媒体关闭及缓冲释放后发布done，20次500ms短结果重播不重跑JPEG或SD流程。不新增NVS/SD状态文件，不刷新照片。实际入口测试发现报告切换可能沿用旧图片序号，已在调用前初始化及调用后发布最终报告；最终主机27/27 ASan/UBSan（31.14秒）、固件门禁22项、解码7项含真实Tcl fake target故障/非法地址测试、采集器14项全部通过。指定三图主机已有库重开完整流程PASS，不是设备证据。默认与专用ESP-IDF v6.1编译/门禁通过，未改SDK/BSP/驱动/时序/波形。

专用独立build-jpeg-status app3194784字节，SHA256 `f34b1213b37a5cde769ade9e7ee64cfb8f49354aaa96215d4509204f2aa8b57e`；ELF状态 `3fc94cfc 00000040 D frame_jpeg_status`，地址只适用于本版。默认回归app356672字节 SHA256 `980e868465cc58ddbc18e275188a235e4629e5d07b1fdb5bd953cb2e9e2def2b`未烧；旧build-jpeg-samples fe48…校验未变。

按用户“直接烧写不用我确认”授权，先读取完整16MiB `.cache/pico-jpeg-status-BFZwdX/pretest-flash.bin`（103.7秒），SHA256 `86683a75e8cfb7b185e0ad3c84b46b1883e53884652bcc40ccc2983a5812a0a4`。六份ELF素材完整相符、状态符号64字节/地址、当前旧JPEG app/boot/分区、历史混合尾部与原厂备份/回滚核验通过；本轮旧boot/app回滚副本已提取完整核验，保留原厂回滚文件。

三个镜像Hash verified；扩展回读0x410000（4259840字节），boot21168/table3072/app3194784逐字节相符，回读SHA256 `acfbd9ad48a2f71c4f1d899e4114b2a3484175b8c2b1e8e212315177d22369d3`。0x6000–0x7fff、0x8000–0xffff、0x31c000–0x40ffff保护字节与本轮备份相同；应用擦写限于0x10000–0x31bfff，不额外清尾部/NVS。备份进入下载器使用default-reset，烧写/回读before/after均no-reset。工具退出、串口释放，设备留下载器，新应用未启动，无本轮应用SD写入/生成/显示结果，不清卡/修复旧文件/校准或操作相机。

下一步用户就绪后先采集再人工开机，核实三图与实际索引总结果；无换图为预期。新RAM查询脚本本轮仅主机fake target验证，尚未连接实机；按需短暂停双核、两次同值、逐核恢复确认，无reset/断点/Flash或内存写入，暂停影响时序。详见 [JPEG-STATUS.md](JPEG-STATUS.md)。新版总结果、跨关机源/缓存恢复及生成缓存SD显示均待验证；不沿用旧RAM缓存视觉验收或旧版两图局部证据。

## 2026-10-02：JPEG第二次人工启动进入应用，整轮结果未完整取得

端口采集先就绪，提示拔USB、完整关机／开机并重连，用户确认“已开机”。120秒私有 `manual-power-jpeg-samples-02.log` 191字节、2次连接／212次传输错误（含人工断开和重枚举重连尝试，不等同芯片故障），SHA256 `11dd87da80319bffede9273524037d0dcbd13568a29bddc1ad929b8bf2d9c5ad`。仅两行：12324ms `JPEG_SAMPLE_GENERATED index=1 source=SD decoded=1562x1600 original=1562x1600 scale=0 elapsed_ms=11011`；同刻 `JPEG_SAMPLE_BEGIN index=2 bytes=564112`。

30秒无复位补采 `manual-power-jpeg-samples-late-02.log` 64字节、1次连接、0错误，SHA256 `0c517ded120bc29707729d1b295279b6af739cd16c608f0018f8f2d46f1b147b`，仅半行 `I (23566) jpeg_samples: JPEG_SAMPLE_GENERATED index=2 source=SD `，后续尺寸／耗时／换行缺失。补采可能取得滞留数据，不据23566ms推断当前执行位置。

结合本轮运行的fe48d59c…代码分支，第一张已完成完整源比较、SD JPEG解码、Cover生成、与可信PFG1全包比较和源再比较；第二张至少到达生成成功日志分支，但完整元数据未捕获。源写入路径可以沿用已有相同文件，日志不区分来源是否本轮创建。没有第三张／库来源／总PASS／FAIL／END，不能确认三缓存写入或global sequence=6提交，不能把局部成功当整轮通过，也不以日志缺失判算法或SD失败。APP_ENTRY／启动头缺失，但生成日志足以证明应用运行。

两个采集结束、串口释放。本轮无主机主动复位／烧写／实现更改／相机或校准操作；设备应用可能操作诊断SD范围，不能宣称本轮无SD写入。主机25/25及编译沿用上轮。代码搜索服务仍缺Windsurf配置，使用本地文件搜索审阅锁定SDK：USB Serial/JTAG非阻塞VFS的满FIFO发送超过50ms会丢字符，写函数仍可返回请求长度，未连接时返回EIO；这是日志丢失的可能机制，不证明本次传输故障或CPU位置。未修改采集器／SDK／BSP或时序。

停止重复同样人工电源采集，下一步以可查询诊断状态或严格只读已有JPEG／缓存／索引检查补齐第三张及全局提交证据；当前不继续新显示验收。此前首次采集仅早期日志不再作为本版本“未进入应用”的结论。

## 2026-10-02：JPEG诊断首次人工启动无应用结果

用户就绪后先启动120秒只读采集，确认端口已打开，再提示关机／开机；用户回复“已开机”。私有 `.cache/pico-jpeg-samples-qJO6sw/manual-power-jpeg-samples-01.log` 1337字节、2次连接／33次传输错误（含USB重枚举重连，不等同芯片故障）；SHA256 `291f6b2e27f165f920f2b2d37655fd733263a4a404d1aab88e29f4bdaa4fbecd`。日志从esp_image segment1（557ms）开始，621ms加载应用，最后626ms进入PSRAM调校。没有ROM复位原因／完整启动头、APP_ENTRY、SD挂载／PMU就绪或任何JPEG诊断标记；不能将字段good-die Pass当PSRAM完整测试通过。

随后10秒只读补采 `manual-power-jpeg-samples-late-01.log` 43字节、1次连接、0传输错误，唯一633ms `Found 8MB PSRAM device`，SHA256 `3d9fbd28bcdc4f156117856e19d93e8813adfb3b34424a039c41bb2616f63d8b`。不能据首次最后一行断言停在调校入口；补采可能读取滞留字节，不证明当前CPU位置或完整PSRAM测试通过。仍无应用／JPEG结果。两个窗口结束、端口释放；本轮未主动复位、烧写、访问SD／相机或改校准，没有实现变更，沿用上轮主机25/25与固件编译。现存专用app校验仍fe48d59c…；实际CPU位置及应用是否曾执行未知，不能记JPEG解码／缓存失败或PASS。

下一步先采集，再由用户执行断开USB的完整关机启动对照；不能用USB保持连接的按钮操作独立证明掉电。暂不重烧、不改Flash／PSRAM时序或清理SD文件。下节待启动／下载器描述为烧写阶段历史。

## 2026-10-02：JPEG 源保存／设备解码／生成缓存诊断烧写，待启动

模式 `jpeg-samples`，使用用户指定三图的私有缩小JPEG与新参考包，固定新目录。板端从SD文件解码、Cover生成并与参考包完整比较，源再次完整比较后才将生成缓存写入并提交本地诊断索引；失败保留，不刷新照片、不操作相机。主机25/25（ASan/UBSan）、Python门禁22项、默认及专用v6.1编译通过。私有三图主机完整流程首次建库及已有库重开均PASS，三生成包完整一致，不是设备执行证据。

专用app3193520字节／SHA256 `fe48d59ce71e74565ee2b841c13bbf0b471c584d12871d8cb84460116a52f50c`；默认回归app356672字节／SHA256 `29004f6d172f780b5af5b5fc1a6a008600e0985d46f1c3d5c4afbbfef29f378f`未烧。新完整16MiB `.cache/pico-jpeg-samples-qJO6sw/pretest-flash.bin` 103.1秒读取，SHA256 `cb8a44325a125f5920d478d8a65a312f32d94ca526110f04a0bbaf7fc03ba3ba`。核验当前3688…显示app／boot／分区、历史混合尾部、六份ELF素材及原厂回滚通过，旧boot／app回滚副本保留。

三个镜像Hash verified；扩大至0x410000的4259840字节回读24.6秒完成，boot21168／table3072／app3193520逐字节一致，回读SHA256 `aa3af1f805d3dffe5bd90361351e60eca7140260a52f0f1fe5a6f062d01ff9eb`。0x6000–0x7fff／0x8000–0xffff／0x31c000–0x40ffff保护字节与新备份相符；应用只擦指定0x10000–0x31bfff范围，没有额外清尾部／全擦／NVS擦除。备份进入下载器使用复位，烧写／回读before/after no-reset；工具退出、串口释放，设备留下载器，新应用尚未启动、没有本轮应用SD写入／JPEG生成／屏幕刷新结果。

主机采集器新增JPEG标记，故障测试发现旧子串匹配会误记显示SAMPLES_END／SAMPLE_BEGIN，改标识符边界匹配后14项循环测试通过。下一步先启动采集再人工关机开机，期待三JPEG_SAMPLE_GENERATED与JPEG_SAMPLES_PASS sequence=6；本版不换图。之后仍需严格只读跨关机源／缓存恢复与生成缓存SD显示。旧缓存损坏根因、正式PFU1／PFI1产品事务及相机链路没有关闭。

## 2026-10-02：已有缓存只读SD三图视觉验收通过

用户本轮明确反馈：“三张图片比例正常，铺满屏幕，没有竖条或者残影”。与下节同轮三个SD_CACHE_READ_OK source=SD、三个GC16 result=0及SAMPLES_END结合，确认指定三张样片从已有SD缓存读取、完整校验、全屏Cover显示的实机验收通过。此前RAM缓冲写入完整比较、人工关机后严格已有缓存恢复也已通过，形成这组三张诊断缓存的写入→恢复→读取→显示证据。

本次为用户目视确认，未新增附件或主机采集；仅更新文档，设备仍运行3688b351…SD显示版。本轮显示正常不关闭原cache-test损坏根因；设备JPEG解码／源文件耐久性／相机链路及异常介质、长期压力待验证。下节“视觉待确认”属于反馈前历史。

## 2026-10-02：已有缓存只读SD三图显示运行通过，视觉待确认

端口采集就绪后提示人工关机开机，用户确认“已开机”。120秒私有 `.cache/pico-sd-cache-ram-display-CBZz6s/manual-power-sd-cache-ram-display-01.log` 5739字节、2次连接／30次传输错误（含USB重枚举重连尝试）；APP_ENTRY sd-cache-ram-display（566ms）、SD mounted（707ms）、PMU ready（776ms）。11136ms `SD_CACHE_RAM_DISPLAY_READY sequence=6 source=existing-only`，证明固定已有三缓存预检查成功。

| 图片 | SD完整读取／校验 | 像素复制 | GC16刷新 | result／温度 |
| --- | --- | --- | --- | --- |
| 1 | 6546ms | 299ms | 401ms | 0／31°C |
| 2 | 6542ms | 300ms | 459ms | 0／31°C |
| 3 | 6545ms | 300ms | 459ms | 0／31°C |

每张READ_OK明确source=SD，检查头、完整CRC、绑定、暂存像素与可信包一致及读后元数据；前两张刷新后各保持30秒。94739ms SAMPLES_END、94740ms应用返回，最终保持图3。三次刷新前后内部空闲245223、PSRAM空闲6260632字节，最低内部208640／PSRAM6258896字节；没有READ_FAIL或刷新错误标记。

本次SD读取与有序刷新执行通过；三图比例、全屏铺满、灰阶／残影／竖条需本轮用户目视确认，不能沿用此前嵌入Cover显示照片。采集结束、串口释放，本轮无新烧写／主机主动复位／实现改动；沿用上轮主机23/23、Python门禁21项及固件编译。应用路径不准备／补写／修复SD文件，旧异常cache-test保留；既有DMA根因、长期可靠性、设备JPEG解码和相机链路尚未验收。

## 2026-10-02：已有 RAM 缓存只读显示版烧写／回读，待启动

模式 `sd-cache-ram-display`：严格三缓存恢复预检查后，逐张只读加载SD像素到415872字节PSRAM；头／完整CRC／绑定／与可信像素一致／读后元数据检查成功才进入映射与GC16。主机23/23（ASan/UBSan，32.07秒）、Python门禁21项、默认及专用v6.1编译通过；接续中断前成果，本轮确认构建源码无待编任务、专用门禁及三份ELF嵌入包一致。新增真实显示入口的主机模拟验证SD暂存来源、逐张失败停止、暂存释放和电源开关配对。

专用app1608400字节／SHA256 `3688b35196d4bf63985856b7a9a625601fa761ab45d67f9b58cb081fd55cd845`。完整16MiB私有 `.cache/pico-sd-cache-ram-display-CBZz6s/pretest-flash.bin` SHA256 `76b2bdf19512e21be578d8a271b36a935e22c4101f58088b1d8c8a50b58b3860`，103.5秒读取完成；旧只读恢复固件、分区、混合尾部及原厂回滚核验通过，旧boot/app副本保留。三个镜像Hash verified，2MiB回读boot21168／table3072／app1608400逐字节一致；0x6000–0x7fff／0x8000–0xffff／0x199000–0x1fffff与新备份一致。应用擦除0x10000–0x198fff，0x199000–0x199fff仍为历史app尾部，未额外清除。

备份进入下载器使用复位，烧写和回读before/after均no-reset。设备仍下载器、串口释放，尚无新版SD读图／刷新日志或实屏结果；本轮未启动SD准备、删除旧异常文件、格式化、擦NVS、改校准或操作相机。下一次人工开机先采集，观察READY、三个READ_OK／SAMPLE_RESULT及SAMPLES_END，再确认面板比例、铺满、灰阶和竖条。三图各次成功刷新是新验收，不能沿用嵌入Cover显示或恢复PASS代替。

## 2026-10-02：严格只读 RAM 缓存首次人工关机后恢复 PASS

先采集端口就绪再提示人工关机开机，用户确认“已开机”。120秒私有 `.cache/pico-sd-cache-ram-recovery-0OeYZS/manual-power-sd-cache-ram-recovery-01.log` 3976字节、2次连接／33次传输错误（包括USB重枚举重连尝试，不等同芯片故障）。APP_ENTRY sd-cache-ram-recovery（571ms）、SD mounted（713ms）、PMU ready（782ms），11159ms `SD_CACHE_RAM_RECOVERY_PASS sequence=6 source=existing-only; three complete byte comparisons; no create/checkpoint/write/photo refresh`；11160ms应用返回。

本次已有 cache-ram-test 正确绑定、相同有效双快照、空日志、三已知KEPT／photo revision=1／global sequence=6、三份完整缓存逐字节比较及读后元数据复核通过。没有重建、补写、修复或照片刷新；旧异常cache-test保留，不能拿面板留图当SD显示通过。本结果支持一次用户确认人工关机后的严格已有缓存恢复，不证明独立电气掉电、长期耐久性、设备JPEG源落盘、相机链路或旧损坏DMA根因。

采集结束、串口释放，本轮无主机主动复位／烧写／相机操作或代码实现更改；设备仍只读恢复版，沿用上轮22/22主机、20项Python门禁及固件编译结果。下一步独立只读SD缓存→暂存完整校验→GC16三图显示闭环，不调用准备／修复或嵌入兜底；多轮恢复与压力另验。

## 2026-10-02：严格只读 RAM 缓存恢复版烧写／回读，待启动

只读模式 `sd-cache-ram-recovery` 要求已有固定目录／绑定、相同有效三条目双快照、空日志、global sequence=6、三个已知 KEPT／photo revision=1；全三文件逐字节比较后再次检查元数据。无创建、补写、索引提交、checkpoint 或换图；移除恢复路径 IO 写回调、不使用坏槽回退。主机 CTest 22/22（ASan/UBSan，最终31.79秒）、Python 门禁20项、默认及专用 v6.1 编译／门禁通过。门禁拒绝准备／索引提交入口，三份 ELF 包与可信素材一致；app1596560字节／SHA256 `70974a15023878fe1458bd93230d7c907cfc5e51fd94b5fb0f156e2cb00e7040`。

首次16MiB读取约78%发生串口中断，无完整备份，未开始烧写；端口恢复后 no-reset 重试成功，失败日志保留。完整私有 `.cache/pico-sd-cache-ram-recovery-0OeYZS/pretest-flash.bin` SHA256 `ebe3c0dfe6dda83166e0f1e6df24b1a0aad3e805d76d1e7a7f1a04c492e8d9d2`；旧RAM app、历史尾部、分区和原厂回滚核对通过，保留旧boot/app回滚副本。三个镜像Hash verified；2MiB回读boot21168／table3072／app1596560逐字节一致。0x6000–0x7fff、0x8000–0xffff、0x196000–0x1fffff与新备份相同；应用擦除仅0x10000–0x195fff，0x196000–0x199fff保留历史app尾部，不额外清除。

首次备份进入下载器使用复位，重试／烧写／回读前后no-reset。工具退出、串口释放，设备仍下载器、新版未运行；无本轮SD补写／修复／照片刷新、格式化、NVS擦除、VCOM校准或相机IO。下一步先采集再人工关机开机，验证 RECOVERY_PASS／FAIL；仅烧写及回读通过，不计作已有缓存跨关机恢复，也不证明旧DMA根因。

## 2026-10-02：RAM 缓冲实验首次启动 PASS

先启动只读串口采集并确认端口就绪，再提示人工电源操作，用户回复“已开机”。120 秒私有 `.cache/pico-sd-cache-ram-Ug4Qet/manual-power-sd-cache-ram-01.log` 4024 字节、2 次连接／29 次传输错误（包括 USB 重枚举重连尝试，不等同芯片故障）；APP_ENTRY sd-cache-ram-probe（566 ms）、SD mounted（707 ms）、PMU ready（798 ms）。1006 ms 报 RAM_BUFFER internal=1 dma=1 alignment4=1 bytes=1024；20415 ms 报 `SD_CACHE_RAM_PASS sequence=6 source=created; three complete byte comparisons; no photo refresh`，同时间应用返回。

本次新建独立 cache-ram-test 库，三份完整 PFG1 字节比较、KEEP／checkpoint 和同进程关闭重开通过；不访问旧异常 cache-test 准备路径、不修复旧文件或刷新照片。此 PASS 不包含跨关机恢复、独立电气掉电测量、设备 JPEG 源文件落盘、相机链路或 SD 读图显示。时间／目录变量与旧实验不同，不能仅据本次成功确认 DMA 是旧文件损坏根因。

采集结束、串口释放，本轮无主机主动复位／烧写／文件清理／相机操作；只更新验证文档，沿用已烧写的 f0253490… 实验版及上轮主机／编译结果。下一步验证已有三缓存及固定 sequence=6 在人工关机后仍存在且比较一致，必须标识 existing、不把新建当恢复；若要求严格零补写证明，应另建只读恢复模式。

## 2026-10-02：独立 RAM 缓冲实验版烧写／回读，待启动

模式 `sd-cache-ram`：1024 字节内部 DMA RAM 作为 fwrite 源，只操作独立 `/frame/cache-ram-test`；原异常 cache-test 和旧 storage-test 不清理，完整比较通过才提交／重开，不刷新照片。主机 CTest 22/22（ASan/UBSan）、Python 门禁 19 项、默认及专用 v6.1 编译／门禁通过，ELF 三份嵌入包与可信素材逐字节一致。专用 app 1601216 字节，SHA256 `f0253490d0c43bbfae5c40e7c5064842c90f101087eefc9aa1ef6fa48252fdae`。

新完整 16 MiB 私有备份 `.cache/pico-sd-cache-ram-Ug4Qet/pretest-flash.bin`，SHA256 `a30fbd05b0f8e11cf12b8c11dd191cb767553fba320a88e9e47d4020ce8ed380`；旧只读 app、分区、原厂回滚与历史混合尾部检查通过，提取回滚副本保留。沿用户直接烧写授权写入三个镜像，均 Hash verified；随后 2 MiB 回读三个镜像逐字节一致，0x6000–0x7fff、0x8000–0xffff、0x197000–0x1fffff 与新备份一致。应用擦除范围 0x10000–0x196fff；历史 app 尾部 0x197000–0x199fff 保留，不额外擦除。

备份进入下载器使用复位，烧写／回读 before/after 均 no-reset。设备仍下载模式，实验版尚未启动、串口释放；本轮无应用 SD 准备／修复／换图、格式化、NVS 擦除、校准更改或相机 IO。下一步先采集再人工开机，检查 RAM_BUFFER／RAM_PASS／RAM_FAIL；不能把烧写成功当运行通过，即使 RAM_PASS 也不证明旧损坏的 DMA 根因或物理掉电持久化。

## 2026-10-02：只读版启动，首图缓存内容异常、后两图缺失

先采集端口就绪再提示人工电源操作，用户确认“已开机”。120 秒私有 `.cache/pico-sd-cache-inspect-xcE4mA/manual-power-sd-cache-inspect-01.log` 4529 字节、2 次连接／29 次传输错误（含重枚举重连尝试）。捕获 APP_ENTRY inspect-only（564 ms）、SD mounted（705 ms）、PMU ready，三项诊断和 INSPECT_END／应用返回（7017 ms）。人工电源操作依赖用户确认，非独立电气掉电测量。

| 项目 | 只读结果 |
| --- | --- |
| 图1 | 检查完成但数据错误：长度415952、首差异文件字节0、头无效、mask=0x2fff；完整像素CRC=9cb5ea2f，不等于预期53f23b87 |
| 图2 | cache-stat errno=2，固定文件未找到；未创建 |
| 图3 | cache-stat errno=2，固定文件未找到；未创建 |

mask 表示头 magic/schema/身份/源/尺寸/布局/缓存及头 CRC 等分组不符，保留字段分组相同；不包含实际字节／ID 内容。首图 payload_complete=1 表示读取了完整415872字节，不代表有效，ok=1 仅指检查过程完成。其 CRC 不等于全零（51f30ab4）或全白（f6dc332f）参考，不能简单判作空白文件。未捕获准备／索引提交／照片刷新调用，不将面板留图计为本轮新显示 PASS。

本地 SDK v6.1 `sdmmc_write_sectors` 与 `sd_host_check_buffer_alignment` 显示：检查函数只验证指针／长度对齐；对齐且不被判为外部 RAM 的源可能直接送 DMA。准备代码 fwrite 参数来自 Flash 嵌入数据，因此 Flash 源直接写卡是待验证的代码级候选，**尚未证实该指针实际穿过 stdio/FatFS 进入 DMA或是本次损坏根因**。后续应保留异常文件，在独立范围验证 RAM 缓冲写入与回读，不修改 SDK／上游驱动或用覆盖重写掩盖证据。

采集正常结束、串口释放，无本轮主机复位／烧写／删除／修复 SD 文件、相机操作或照片刷新。只读诊断运行成功，缓存持久化／SD 显示闭环未通过。

## 2026-10-02：只读缓存差异检查版烧写／回读，待人工启动

新增 inspect-only 专用配置：只读现有固定缓存及绑定、完整 payload CRC 和首差异／头字段分组报告，无准备／导入／KEEP／checkpoint／照片刷新调用。主机 22/22（ASan/UBSan），门禁 Python 18 项，通过只读模式／准备入口隔离及文件不修复、缺失不创建检查。MCU 首次编译的路径截断警告已用显式长度检查和扩大缓冲修正，默认与专用 v6.1 编译／门禁通过；上游无改动。只读 app 1593312 字节，SHA256 `6041aa2e020cc54ca2777f82247b7f9855370ee92aa264abd6881494b58108b2`。

按此前无需再确认烧写的授权，先 default-reset 进入下载器、完整备份 16 MiB，随后所有写／回读均 before/after no-reset。私有 `.cache/pico-sd-cache-inspect-xcE4mA/pretest-flash.bin` SHA256 `09424c4d3c9e8243f0a2ce6cf72f2a98c3ee5751981dc016cea0e5b28fa75dd1`；旧分阶段应用／boot／分区／尾部和原厂回滚核验通过，提取旧 boot/app 副本且历史备份保留。

三个镜像均 Hash verified，擦除 0x0–0x5fff、0x8000–0x8fff、0x10000–0x194fff。2 MiB 回读 boot 21168／table 3072／app 1593312 字节相符；0x6000–0x7fff、0x8000–0xffff、0x195000–0x1fffff 与备份一致，包含核对的 NVS/phy。只读应用较小，所以 0x195000–0x199fff 仍为旧应用尾部，不额外擦除；后续核验应按本轮备份／回读检查这段，不假设它全 FF 或与原厂相同。

工具退出、串口释放、设备留下载器，**没有 SD_CACHE_INSPECT 设备结果或根因修复证据**。本轮未清理／修复 SD 文件，未改相机／VCOM／波形／时序。下一步用户就绪后先开启采集、真实端口打开再提示人工开机；本模式不会切换照片。BSP 仍初始化和挂载，不能把应用只读诊断夸大为整机写保护。

## 2026-10-02：分阶段版启动，首张缓存首块不一致

用户就绪后先启动采集、真实打开端口再提示电源操作，用户确认“已开机”。120 秒私有 `.cache/pico-sd-cache-diag-qSUebQ/manual-power-sd-cache-diag-01.log` 4018 字节、2 次连接／36 次传输错误（含 USB 重枚举重连尝试），捕获 APP_ENTRY sd-cache-samples（568 ms）、SD mounted（710 ms）、PMU ready（779 ms）。1302 ms 报 `SD_CACHE_STAGE_FAIL stage=cache-compare index=1 offset=0 errno=0 created_library=0`，1303 ms PREPARE_FAIL 并返回；无 SD_CACHE_PREPARED／READ_OK／SAMPLE_RESULT／SAMPLES_END。

依据已烧写代码分支，可确定已有库成功打开，首张缓存类型／415952 字节长度、首个 1024 字节 fread 及此前库绑定检查通过，但该块 memcmp 失败。offset 是块起始，不是首个坏字节；尚不能区分头或像素差异。created_library=0 不代表缓存文件未创建，现有日志没有该标记，不能断言本轮无 SD 写入；保护路径不覆盖已有文件、不嵌入兜底或刷新新图。

主机只读比较 `build-sd-cache-samples` 和 `build-sd-cache-diagnostics` ELF 符号／段数据，三份嵌入包均与 `.cache/sd-cache-assets-F2jpBB/sample_N.pfg` 逐字节相符，排除新增诊断时更换期望包这一种情况。不是设备缓存内容的 CRC 证明，最初写入失败原因仍未知。

采集正常结束、串口释放，无本轮主机复位／烧写／删除／修复 SD 文件或相机操作；电源操作依赖用户确认，非独立电气掉电测量。下一步只读首差异定位、头字段有效性和完整 payload CRC 检查，保留文件，不自动覆盖重写，不要求无新证据的重复启动。

## 2026-10-02：第十轮分阶段诊断版烧写／回读，待人工启动

保留 SD 现有文件和停止策略，只补充准备阶段报告：固定 stage／index／offset／errno／created_library，失败信息在关闭前保存；成功标记新建或已有库来源。无私人路径／照片内容，不修改驱动、波形、校准或自动修复。主机 CTest 22/22（ASan/UBSan），包含报告字段和缺挂载 ENOENT／无目录创建检查；默认 JPEG 与诊断 v6.1 编译／门禁通过，旧 bool 接口及旧门禁入口兼容。

完整 16 MiB 私有备份 `.cache/pico-sd-cache-diag-qSUebQ/pretest-flash.bin`，SHA256 `20c7bf064e690d6b301856e623aa171e0baa65ee5137dfd55e58ce389f44afe2`；旧应用 8aae…／boot／分区／尾部／原厂回滚核验通过，提取旧 boot/app 回滚文件且全部历史备份保留。诊断 app 1612304 字节，SHA256 `974afe16ce4af24b267132e9ba131d8a6516feeadb238c1fcab5c0c90b5ffa46`。

按此前无需再确认烧写的授权操作。备份进入下载器使用 default-reset，之后烧写和回读 before/after no-reset；三个镜像 Hash verified。擦除 0x0–0x5fff、0x8000–0x8fff、0x10000–0x199fff。2 MiB 回读 boot 21168／table 3072／app 1612304 字节相符，0x6000–0x7fff、0x8000–0xffff、0x19a000–0x1fffff 与写前备份相同，包括核对的 NVS/phy；不宣称全部写后 Flash 已回读。

工具退出、串口释放、设备留下载器，**新版尚未启动、根因尚未确定**。本轮没有清理／修复 SD 文件或相机操作。下一步先启动采集、端口就绪后再人工开机，以新增失败阶段定位；若旧残缺文件存在，新版仍保留并安全停止，不为了出图绕过校验。

## 2026-10-02：第十轮首次人工启动，缓存准备失败

用户就绪后启动 120 秒采集，真实端口打开才提示人工电源操作，用户确认“已开机”。私有 `.cache/pico-sd-cache-test-IBqYQq/manual-power-sd-cache-01.log` 3907 字节、2 次连接／43 次传输错误（含重枚举重连尝试）；捕获 APP_ENTRY mode=sd-cache-samples（567 ms）、SD mounted（708 ms）、PMU ready（777 ms）。4645 ms 出现 `SD_CACHE_PREPARE_FAIL; preserve files, no fallback to embedded display`，同时间戳应用返回。

本次新应用启动与 SD 挂载取得证据，但缓存准备／SD 读图／屏幕刷新**未通过**：没有 SD_CACHE_PREPARED、SD_CACHE_READ_OK、SAMPLE_RESULT 或 SAMPLES_END。现有失败分支不调用显示刷新，不能把此前面板留图计作本轮成功。准备函数可能已创建部分测试目录／库／缓存，汇总日志不足以确定失败阶段、文件状态或根因；不得断言未写卡、SD 损坏或自动清理。

采集正常结束、串口释放，本轮未主动复位／烧写／删文件／修复／相机操作。启动日志起始片段不完整，电源操作依赖用户确认，不宣称独立掉电测量。上游 GPIO39/40 和 SD delay fallback 警告仍存在，不据此更改硬件时序。下一步需要分阶段错误证据，保留独立测试目录中的现有文件，不要求无新证据的重复开机。

## 第十轮：SD 三图诊断烧写／回读通过，待人工启动

独立 `/frame/cache-test` Cover 夹具准备／提交／SD 只读加载后 GC16 已实现，全套主机 CTest 22/22（ASan/UBSan）；修正新增测试中的手写 ID 文件名错误后全量重跑通过，部分写入失效时无 NEW／KEEP 提交且重试不覆盖。v6.1 sd-cache-samples 门禁与默认 JPEG 编译回归通过；上游仍锁定 28cde682a4468a581c278761922724f57d976418、工作树无修改。

专用 app 1610320 字节，SHA256 `8aae15f1283dd707614f5c71bed57abb7b87ccf678304b2561f3d929b0714423`。私有三图 packet 各 415952 字节，源为主机去白框 JPEG 副本，payload 与独立 Cover 转换一致、头／源／像素 CRC 正确，原图哈希不变；卡上没有保存源 JPEG，不能算作正式源图导入耐久性／设备解码或 ACK 通过。

按此前“直接烧写不用我确认”的授权执行。首先 default-reset 进入下载器、after no-reset 完整备份 16 MiB：`.cache/pico-sd-cache-test-IBqYQq/pretest-flash.bin`，SHA256 `5c0494efa201bad4aa44b535d3d981d46d193221aa3da438d11edb57a3aa98b9`。当前旧 Cover 9b959…、分区、原厂完整备份及回滚镜像核验通过，提取当前旧 boot/app 回滚副本，所有历史备份保留。

用生成的 flash_args、before/after no-reset 写三个镜像，均 Hash verified。擦除范围为 0x0–0x5fff、0x8000–0x8fff、0x10000–0x199fff；无额外擦除／格式化操作。2 MiB 私有回读 `post-write-prefix.bin` 与 boot 21168／table 3072／app 1610320 字节逐字节相符；0x6000–0x7fff、0x8000–0xffff、0x19a000–0x1fffff 与写前备份一致，包含核对的 NVS/phy。没有回读整块写后 Flash，不扩大保护核验范围。

工具已退出、串口无占用，设备留下载模式，**没有启动、SD 写入／读回或实屏 PASS**。下一步用户准备好后先启动不主动复位的有界日志采集、确认端口真实打开，再提示人工关机／开机；核验准备来源、三个 SD_CACHE_READ_OK 与刷新结果、视觉和读耗时。不得复用旧 Cover 实屏 PASS。详见 [SD 三图诊断](SD-CACHE-SAMPLES.md)。

## 第九轮：用户确认全屏／比例正常／无竖条，本轮实屏通过

用户提供三张面板照片并明确反馈“比例正常，铺满屏幕，没有竖条”。附件对应测试图 3（草地人物和狗）、2（大树）、1（健身设施）；三图画面铺满可视屏，构图与本次 Cover 私有预览相符，未见旧 Fit 图 1 的明显竖条。结合本轮 120 秒采集的三次 result=0／有序结束，本次嵌入 g4 → 驱动像素映射 → GC16 全屏等比裁切展示验收通过，第三张保持行为已由日志记录。

旧竖条更新为“本轮未复现、根因未确定”，保留此前照片／观察记录；本次只改素材布局／裁切，未增加首次物理清屏或修改波形／扫描时序／VCOM，不据单次无竖条推断根因或永久修复。首刷软件／物理状态不一致仍是未证实的候选，需实际复现／控制变量后判断。拍屏照与用户确认支持本轮几何／明显竖条验收，不量化面板灰阶、微弱残影或长期／温度／功耗稳定性。

此反馈轮只更新文档，不连接设备／复位／烧写／修改固件，照片留在用户附件，不复制进公开项目。设备端 JPEG 解码、SD 读图、Cover 产品缓存／导入、相机 BLE／Wi-Fi／HTTP 仍未通过；下一阶段优先建立独立 Cover 布局缓存和 SD 显示路径，不将旧 layout=1 Fit 数据误用。下文采集／烧写与旧 Fit 异常均为历史。

## 第九轮：Cover 三图首次启动／刷新日志通过，视觉待确认

用户就绪后开启 120 秒采集，实际端口打开后才提示人工关机／开机，用户回复“已开机”。私有 `.cache/pico-cover-test-VPaA0c/manual-power-cover-01.log` 5233 字节、2 次连接／37 次传输错误（含 USB 重枚举重连尝试，不等同硬件故障）。APP_ENTRY embedded-display-samples、三组有序 SAMPLE_BEGIN／SAMPLE_RESULT 均成功，以及 SAMPLES_END／应用返回已捕获。

| 图片 | result | 温度 °C | copy_ms | refresh_ms |
| --- | --- | --- | --- | --- |
| 1 健身设施 | 0 | 30 | 299 | 401 |
| 2 大树 | 0 | 30 | 299 | 459 |
| 3 草地人物和狗 | 0 | 30 | 299 | 459 |

第三张结果／结束标记时间戳 64947 ms，64948 ms 应用返回。三次 free internal=245311、PSRAM=6678428，最低 free internal=208640、PSRAM=6678428 字节；不是完整堆峰值分析。温度／刷新函数耗时只代表此轮采样，不能作为长期稳定／功耗验收。无捕获的 panic／Guru Meditation／watchdog。人工电源操作依据用户确认，采集并非独立掉电电气测量／无 USB 干扰证明。

采集正常结束、串口释放；本轮无主机主动复位／烧写／SD 测试文件写入／相机操作。日志证明 Cover 素材应用进入并完成三次刷新调用，不证明面板全屏正确、不拉伸或第一张竖条消失。新版目视反馈尚未收到，旧 Fit 异常未关闭；第三张按程序保持，等待用户确认／面板照片。设备 JPEG／SD 读图和相机仍未验证。

## 第九轮：全屏 Cover 版烧写／回读通过，待启动／视觉回归

用户要求“图片全屏展示，根据屏幕尺寸裁切，不要拉伸”。按这一新要求增加便携 Cover／居中裁切／显式 ROI、CLI --cover／--crop、Demo 0 Cover 入口；旧 Fit／持久化 layout=1 保留、不混用旧缓存。私人三张样片在 1500×1500 解码坐标显式去原白框，再等比覆盖 1216×684；旧源／素材保留，原 JPEG SHA256 未变。已目视检查三张桌面 Cover 预览，未加纸边／非拉伸；居中裁切会切掉上下内容，图 2 底部人物不在画面中，裁切位置调节待后续选择，不宣称完整构图。

主机 CTest 20/20（C 目标 ASan/UBSan），新增中心横／纵裁切、EXIF 1–8、ROI／边界采样／奇数 stride、非法参数不写输出、CLI 配置／越界／覆盖保护测试。ESP-IDF v6.1 图片诊断和普通 JPEG Demo 0 均编译／门禁通过。烧写的是 display-samples 模式 Cover 素材版，app 1594416 字节、SHA256 `9b95994cd9fc499ca131789dc4ff6d18531b2b60b5ed195533b380252482371a`；普通 JPEG app 356672 字节、SHA256 `d2506a67d2b7b2db893ead3049da53426104f3942669830ee80361b3527dd7d3` 仅编译，不能视为设备 JPEG 已测试。

按此前无需再确认烧写的授权，default-reset 进入下载器读取全 16 MiB 私有备份 `.cache/pico-cover-test-VPaA0c/pretest-flash.bin`，SHA256 `6a2a1421b6780bc1cb5c244f8db7ec89ec8b980139f49abd153bc76cdd02c998`。旧 Fit 三图 boot/table/app 逐字节检查、分区一致、有效原厂全备份／回滚文件／旧应用尾部核验通过，另提取本次旧应用回滚文件。按生成 flash_args、before/after no-reset 写入，三个 Hash verified；擦除 0–0x5fff、0x8000–0x8fff、0x10000–0x195fff，无整片擦除。

no-reset 回读 0x1b0000（1728 KiB），boot 21168、table 3072、app 1594416 与构建完全一致；0x6000–0x7fff、0x8000–0xffff（含 NVS/phy）、0x196000–0x1affff 与新备份一致。非整片写后复核。工具退出、串口释放，设备留下载模式，尚无本版 APP_ENTRY／刷新／视觉证据。此次仅布局／资产变更，无清屏／换序／驱动／波形／时序／VCOM 校准修改、无 SD 测试文件写入／相机操作；原图 1 竖条未解决，不以裁切掩盖或声称修复。等待用户就绪后先采集再人工开机，分别验收全屏与竖条复现。

## 第八轮：面板照片确认图片 1 竖条，视觉验收未通过

用户提供三张实机照片并报告“第一张图片有异常竖条，2 和 3 没有”。按画面对应，附件顺序是测试图 2（大树）、3（草地人物和狗）、1（健身设施），不是程序显示顺序。健身设施照片天空／地面可见多条竖向痕迹；另两张未见同类明显竖条，与用户报告相符。构图／方向从照片未见明显颠倒，但不能从拍屏照量化灰阶、残影或色阶质量。整轮视觉验收未通过，保留“有序刷新返回码成功”这一独立结论，不称三图显示 PASS。

只读检查：私有样片 1 PGM 与当前转换结果 cmp 相同，桌面预览未见对应竖条。`components/epdiy/src/highlevel.c` 初始化 front/back_fb 全白；`epd_hl_update_screen_full` 即使置全行／全列刷新，仍按 front/back 构造差分，完成后才复制 front 到 back。当前本项目 `main/display_samples.c` 首次刷新前没有物理清屏；锁定上游 `main/app_main.c` 则在首次开机图之前上电／epd_clear，再从白色基线更新。软件认为上一帧全白，但电子纸保留画面并未同步清白，构成可验证的首刷状态不一致风险。这是优先假设，不是竖条根因定论；内容相关扫描／供电／驱动问题不能因后两图正常就排除，Flash 回读相符也不证明运行期 framebuffer／扫描输出正确。

建议下一轮先区分“测试图 1”与“第一次刷新”：换序让图 2 首刷、图 1 后刷，或原序后段重画图 1；之后仅调用原厂清屏 API 建立一次白色基线作对照，其他资产／驱动／波形／时序／校准保持不变。每个实验单独标记，并目视记录首刷／重画条纹是否随顺序或内容移动；未复现实验前不宣称修复。此轮没有设备连接／复位／烧写／固件代码修改，仅源码与私有预览检查／文档记录；照片留在用户附件，未复制到公开目录。

## 第八轮：首次人工开机后三图刷新结果通过，视觉待确认

用户回复“准备好了”，开启 120 秒采集，实际端口打开／capture ready 后才提示保持 USB／microSD 连接、人工关机／开机；用户回复“已开机”。私有 `.cache/pico-display-test-pqfvd2/manual-power-display-01.log` 5232 字节、2 次连接／36 次传输错误（含重枚举重连尝试，不是 36 次硬件故障）。捕获 APP_ENTRY embedded-display-samples、SD mounted、PMU ready，以及三张有序刷新和结束／应用返回。

| 图片 | 刷新返回码 | 温度 °C | copy_ms | refresh_ms |
| --- | --- | --- | --- | --- |
| 1 健身设施 | 0 | 30 | 300 | 394 |
| 2 大树下的人 | 0 | 30 | 299 | 451 |
| 3 草地人物和狗 | 0 | 31 | 299 | 451 |

第三张结果与 SAMPLES_END 时间戳 64971 ms，下一毫秒应用返回；前两次至下一次 SAMPLE_BEGIN 均相隔 30000 ms。三次 free internal=245311、PSRAM=6678428 字节，最低 free internal=208640、PSRAM=6678428；只报告运行时空闲／低水位，不是完整堆峰值分析。copy_ms 不含上电，refresh_ms 是更新函数耗时，不包含整个电源／切图周期。无捕获的 panic／Guru Meditation／watchdog；保留上游 GPIO39/40 提示和 SD input-delay fallback 警告，不作无警告或长期稳定结论，不因此修改驱动／时序。

采集已正常结束、串口释放；本轮无主机主动复位／烧写／SD 测试文件写入或相机操作。ROM 启动头／复位原因未捕获，人工电源操作依据用户确认，不宣称独立掉电测量／无 USB 干扰冷启动。匹配固件／有序返回码／结束标记证明显示入口和刷新调用成功，不证明面板视觉正确；用户尚未反馈逐张画面／方向／层次／异常横条／残影，视觉验收仍待确认。设备端 JPEG／SD 读图／相机链路／压力与功耗不在本轮通过范围，第三张按程序保持。

## 第八轮：三张私人照片显示版烧写／回读通过，待启动

用户要求继续，且本阶段明确授权直接烧写无需再确认。为绕过电脑无法复制到卡的限制，新增独立 display-samples 模式，将三张指定原图的离线内部 g4 嵌入应用 Flash；每张 1216×684／415872 字节，与既有样片 PGM 逐字节一致，原图 SHA256 未变。驱动像素 API 映射旋转／位序，GC16 全屏刷新、前两张各停留约 30 秒、第三张保持；全部长度先核验，电源／温度读取或刷新错误停止。未改锁定上游 BSP／波形／时序，PMU VCOM 只读；不写 SD 测试文件，不执行设备 JPEG 解码或相机操作。固件包含私人照片，构建／备份／日志仅存忽略目录，不提交或上传。

主机 CTest 20/20（C 目标 ASan/UBSan）；实际 g4 blit 测试覆盖全部灰阶、奇数 stride、完整面板尺寸、错长度拒绝／回调失败停止／让步次数。采集上限扩至 120 秒，新增观察标记不自动判 PASS。v6.1 最终编译／display-samples 门禁通过：app 1594416 字节，SHA256 `bb884bc754943805dcdd6f6cf3bbb37e00c611da88f9b121d546bf3c59e653f2`，三个嵌入 Flash 符号范围／长度／不重叠核验通过。

default-reset 进入下载器读取完整 16 MiB 私有备份 `.cache/pico-display-test-pqfvd2/pretest-flash.bin`，SHA256 `24f07bdbeaeebcb77674401a9ea888583c07304d323e465a3fed65273092a7e9`。旧严格恢复 app／分区／原厂完整备份与回滚文件核验，并提取本次旧应用回滚文件。按生成 flash_args 写入 boot/table/app，before/after 均 no-reset；三个 Hash verified。擦除范围分别 0x000000–0x005fff、0x008000–0x008fff、0x010000–0x195fff，无整片擦除。

no-reset 回读 0x1b0000（1769472 字节／1728 KiB），boot 21168、table 3072、app 1594416 与构建逐字节相符；0x6000–0x7fff、0x8000–0xffff（含 NVS/phy）、0x196000–0x1affff 与新备份一致。没有整片写后复核。设备留下载模式、工具退出、串口无占用；尚未取得 SAMPLE_RESULT/SAMPLES_END 或屏幕目视证据。下一步就绪后开始 120 秒采集，实际端口打开才提示人工关机／开机。编译／烧写／回读不等于启动／实屏 PASS；本轮不证明设备 JPEG／SD 读图／相机／功耗／压力测试。旧 SD 固定测试库保留，详见 DISPLAY-SAMPLES.md。

## 第七轮：第三次严格已有库恢复 PASS，累计 3/3

第三轮另起 45 秒采集，真实打开端口确认 capture ready 后提示人工关机/开机，用户回复“已开机”。私有 `.cache/pico-storage-recovery-YdnR8C/manual-power-recovery-03.log` 1592 字节、2 次连接、31 次传输错误（含重枚举尝试）；SD mounted、`SD recovery PASS; source=existing-only; expected sequence=2; no create/checkpoint/write`（852 ms）、Returned from app_main 已捕获。匹配严格恢复模式说明原库/照片身份、有效一致双快照、空 journal、唯一 KEPT、全局序号 2/照片序号 1 检查通过，无测试文件创建/补写/回收。

三轮结果汇总：首轮 921 ms、第二轮 919 ms、第三轮 852 ms 输出 PASS，均为既有库读取检查，不能把这些开机时间戳当作单独的存储操作耗时。三轮人工关机/开机后保留 fixture 验证完成（3/3），电源操作依据用户确认；未捕获 ROM 启动头/复位原因，不是独立掉电电气测量、无 USB 干扰冷启动或写中断验收。采集结束、串口释放，本轮无主动主机复位/烧写/相机操作。设备保留严格恢复固件，停止同类重复电源操作，下一阶段准备原图保留前提下的三张照片实屏测试；压力、满卡、拔卡和提交阶段掉电仍待验证。

## 第七轮：第二次严格已有库恢复 PASS（2/3）

另启 45 秒独立采集，端口实际打开后才提示人工关机/开机，用户回复“已开机”。私有 `.cache/pico-storage-recovery-YdnR8C/manual-power-recovery-02.log` 1592 字节、2 次连接、40 次传输错误（含重枚举尝试）；捕获 SD mounted、`SD recovery PASS; source=existing-only; expected sequence=2; no create/checkpoint/write`（919 ms）、Returned from app_main。严格源码路径验证已有匹配库/照片、有效一致双快照、空 journal、唯一 KEPT、全局序号 2/照片序号 1；不创建或补写，累计两轮明确 PASS。

采集已正常结束并释放串口，无本轮主机主动复位或烧写；未捕获 ROM 原因/启动头，人工电源操作依据用户确认，非独立掉电电气测量。SD 控制器缓存压力、写中断故障注入、实屏和相机不在本轮验证范围。第三轮待新采集及人工操作，不清理诊断文件、不重烧初始化版。

## 第七轮：严格已有库恢复首轮 PASS（1/3）

用户确认就绪后，核对 storage-recovery 门禁/SHA256、串口无占用，启动 45 秒独立采集，确认实际端口打开才提示人工关机/开机；用户回复“已开机”。私有 `.cache/pico-storage-recovery-YdnR8C/manual-power-recovery-01.log` 1592 字节、2 次连接、41 次传输错误（包括重枚举重连，非 41 次硬件故障）。捕获 SD mounted、PMU ready、`SD recovery PASS; source=existing-only; expected sequence=2; no create/checkpoint/write`（921 ms）、Returned from app_main。

匹配已烧写严格恢复源码证明本次已有四文件、同一固定库/照片身份、有效一致双快照、空 journal、唯一 KEPT、全局序号 2/照片序号 1 的读取检查通过；无 mkdir/create/import/action/checkpoint，不是丢失后重建。诊断应用没有 SD 测试文件写入，不能扩大为控制器硬件写保护。没有捕获启动头/ROM 复位原因，电源操作依赖用户确认，不宣称独立掉电测量或无 USB 干扰冷启动。采集已结束、串口无占用，本轮无主机主动复位/烧写或相机操作；剩余两轮待人工操作，不重烧初始化版、不清理目录。压力、写中断、实屏仍未验收。

## 第七轮：严格已有库恢复版烧写／回读通过，尚未启动验证

用户要求继续，并已有本阶段无需再确认烧写的授权。新增独立 recovery-only 构建：应用不创建目录/库、不补写状态、不 checkpoint，只验证原测试库；双快照须相同且有效、journal 空、库/照片身份匹配、唯一 KEPT、全局序号 2/照片序号 1。主机 CTest 19/19（ASan/UBSan）与 v6.1 build/专用 storage-recovery 门禁通过。新 app 345968 字节、SHA256 `b748505163b84a07c4fc2e7529d2499f39a53e1758bee77b8f52787286b660d9`。配置 SELFTEST/RECOVERY_ONLY=1、BOOT_PROBE 未启用；ELF 严格恢复/目录检查入口存在，create/checkpoint/import/action/prepare-directory 入口不在链接符号中（配置/代码路径仍是行为判定依据）。上游 BSP/驱动/波形/时序未改。

default-reset 进入下载器读取完整 16 MiB 到私有 `.cache/pico-storage-recovery-YdnR8C/pretest-flash.bin`，SHA256 `002616e7cdcd2fbd868bbe22f3986f79cddbd647f07c3354cd6fef24c7d9fb4d`。旧自动目录 storage 有效镜像/尾部、分区、原厂完整备份和回滚文件核验通过，并提取本次旧 storage 回滚文件。按生成 flash_args 写 boot/table/app，before/after no-reset，三个 Hash verified，应用擦除范围 0x10000–0x64fff。

no-reset 回读 458752 字节，boot 21168、table 3072、app 345968 与构建逐字节相符；0x6000–0x7fff、0x8000–0xffff（含 NVS/phy）、0x65000–0x6ffff 与本次备份一致，未整片写后复核。设备留下载器、工具退出、串口无占用，本轮未运行设备 SD 诊断或访问相机。尚未取得 recovery PASS，等待先准备采集、再人工电源操作，目标三次逐轮严格恢复；不能沿用旧启动探针 RAM 地址。

## 第六轮：首次 SD 挂载／事务／同进程重开 PASS

用户回复“准备好了”后，启动 `wait_pico_boot.py --seconds 45`，确认实际端口打开、capture ready 后才提示保持 USB/卡连接、人工关机/开机；用户回复“已开机”。私有 `.cache/pico-storage-auto-m1QIyN/manual-power-storage-01.log` 1537 字节，2 次连接、39 次传输错误（含重枚举期间重连，不等于 39 次硬件故障），工具观察标记列表为空是启动探针专用识别，不否定以下 storage 日志。

实际捕获 `SPI SRAM memory test OK`、SD mounted、PMU ready（协议 1.1/固件 1.0.9）、触摸 ready、`SD diagnostic PASS; global sequence=2; requires reboot/power-cut validation`（1026 ms）、`Returned from app_main()`。结合匹配已烧写源码，表明专用目录准备/存储入口通过，测试状态提交、双快照/checkpoint 与同进程重开校验成功。有效 PMU VCOM 是该代码分支前置条件，未捕获单独 VCOM 数值，不补造读数；没有屏幕照片刷新。

缺少 ROM 启动头、APP_ENTRY 和复位原因，不据此证明无干扰冷启动或精确电源时序。采集正常结束、串口无占用；本轮没有主机主动复位/烧写或相机操作，设备端按诊断流程写入/同步/回收专用 SD 文件。首次 PASS 不证明跨断电可靠性：当前摘要不能区分打开已有库与初始化新库，sequence=2 不排除丢失后重建；下一步增加来源标记/严格恢复验证，再执行三次人工电源操作，不把相同 PASS 自动记为数据持久化通过。

## 第六轮：自动目录存储诊断烧写／回读通过，待人工开机

用户确认备用 microSD 无需保留数据，无法通过电脑建目录，要求替代方式并授权直接烧写。独立诊断准备固定目录，不改上游 BSP/驱动/波形/时序；主机安全测试及全量 CTest 18/18（ASan/UBSan）、v6.1 storage 编译/门禁通过。app 347952 字节，SHA256 `ef7f08cce70f5f99962e7de1c83c6ae775c0665ee56106170c80bf71477dcbbb`，SELFTEST=1、BOOT_PROBE 未启用。

进入下载器使用 default-reset，完整 16 MiB 备份 `.cache/pico-storage-auto-m1QIyN/pretest-flash.bin`，SHA256 `2485dbf8975d61525738d071e755f43d9781fb07721a5f19c0c937e6e5430d4e`。旧 RAM 探针有效镜像/占用扇区 padding/保留原厂 app 尾部、分区和原厂完整备份/回滚提取核验通过；另提取本次旧探针回滚镜像。按 `build-storage-test/flash_args` 写 boot 0x0、table 0x8000、app 0x10000，before/after no-reset；三者 Hash verified，应用擦除范围仅 0x10000–0x64fff。

no-reset 回读 458752 字节（448 KiB），boot 21168、table 3072、app 347952 与构建逐字节相符；0x6000–0x7fff、0x8000–0xffff（含 NVS/phy）、0x65000–0x6ffff 与本次备份一致。未整片写后回读，不宣称整片复核；无 SD/PMU/VCOM/相机操作。设备留下载器、工具退出、串口无占用，等待人工开机采集；实际目录创建和 SD 事务未运行验证，屏幕不刷照片。不能沿用旧 RAM 探针状态地址查询新版存储固件。

## 第五轮：第三次人工电源操作确认后 PASS，累计 3/3

用户按第三轮关机/开机重连提示回复“已重连”。重新核对已烧写 bin 门禁/SHA256 与 ELF 状态符号，OpenOCD 20260703/1000 kHz、Flash/服务禁用，运行同一无 reset/断点/目标内存写入查询。私有 `.cache/pico-ram-probe-t4Gol7/jtag-status-power-03.log` 两份一致快照 `0x42505231 0x1 0x1 0x1 0x1 0x3 0xffffffff 0x0 0x0 0x1`，严格解码 PASS，test_done/probe_done=1、完整通过三模式、无错误元数据。双核 Reset cause=1 Power on reset，PRE_STATE/POST_STATE 均 running；工具退出、串口无占用。

三轮人工电源操作确认后均明确通过 64 KiB scratch 测试（3/3）。断电操作依赖用户确认，RAM 没有启动唯一编号，未独立测量电源变化；JTAG 暂停影响时序。此阶段记录不等于 8 MiB 全域/压力/温度验收，也不是 SD 三次断电恢复、PMU ready、实屏或相机验证。本轮无新烧写、SD/PMU/VCOM/相机操作；设备继续保留探针，不再要求同类重复操作。下一阶段需备用卡及测试素材确认，更换固件单独授权。

## 第五轮：第二次人工电源操作后 RAM 仍 PASS

用户再次按关机/开机并重连提示回复“已重连”。匹配 bin/ELF 门禁及状态地址 0x3fc92a5c 复核，OpenOCD 20260703/1000 kHz 使用同一查询脚本，不请求 reset/断点/Flash 写入。私有 `.cache/pico-ram-probe-t4Gol7/jtag-status-power-02.log` 两次一致读取为 `0x42505231 0x1 0x1 0x1 0x1 0x3 0xffffffff 0x0 0x0 0x1`，严格解码 PASS，三模式完成、探针结束、无错误元数据；双核 Reset cause=1 Power on reset，PRE_STATE/POST_STATE 均 running。工具退出，串口无占用，无 SD/PMU/VCOM/相机操作。

累计两轮人工电源操作确认后的明确 PASS。当前 RAM 状态没有启动唯一编号，复位原因也不能单独证明两次查询之间发生过断电；重复启动记录依赖用户执行电源操作的确认，不宣称独立测量电源或自动验证启动次数。下一步第三轮人工操作后查询，尚不换到 SD/显示固件；64 KiB 结果不扩大为 8 MiB 全域、压力或产品功能验收。

## 第五轮：人工开机后首次 RAM 结果 PASS

用户执行电源操作后回复“已重连”。核对 USB VID/PID 303a:1001、串口无占用、产物门禁/SHA256 与匹配 ELF 状态符号；使用 OpenOCD 20260703/1000 kHz、禁 Flash 支持及调试服务，执行 `tools/read_probe_status.cfg`，不请求 reset/load/断点/目标内存写入。双核初始 running，暂停后 PC 均 0x40378f66，Reset cause 均 1（Power on reset）；此复位原因是观察到的状态，不是本轮发出复位。

私有日志 `.cache/pico-ram-probe-t4Gol7/jtag-status-power-01.log` 的两份一致快照：`0x42505231 0x1 0x1 0x1 0x1 0x3 0xffffffff 0x0 0x0 0x1`。严格离线解码成功，magic/version 正确，app_entered/test_done/probe_done=1、result=PASS、patterns_checked=3、无失败元数据。第一次取得专用 64 KiB 三模式通过的直接证据，不再依赖串口末行或空闲路径推断。

查询后双核 POST_STATE running，OpenOCD 已退出、串口无占用；无烧写/SD/PMU/VCOM/相机操作。JTAG 暂停会影响时序，不宣称无干扰；本次不证明 8 MiB 全域、压力/温度/长期运行、PMU ready、SD 或实屏。重复启动尚未验证，下一步人工再次完整关机/开机并重连后读取，不能把同一次 RAM 重读计作多次开机。

## 第五轮：RAM 查询探针烧写／回读通过，待人工开机

用户明确回复“允许”后，核对 Pico USB、串口无占用、新固件门禁和 ELF 状态符号 `3fc92a5c 00000028 D`。先 default-reset 进入下载器，读取完整 16 MiB 到私有 `.cache/pico-ram-probe-t4Gol7/pretest-flash.bin`，SHA256 `d736b90a0a4225208e44c96c2c6c215ffeabdf279da1be3e899483335b98f9b7`。检查旧 app 166512 字节的完整性/身份/SHA256、分区、占用扇区 padding 和保留原厂尾部；原厂完整备份与 boot/app 回滚提取逐字节核验，另外提取本次旧探针回滚文件。

使用生成的 flash_args，仅写 boot 0x0、table 0x8000、app 0x10000；烧写 before/after 均 no-reset，三者 Hash verified。新 app 166656 字节、SHA256 `3bb56158734eb4f7e0dbcef1ba7621ab42e09563269de81827ad41df8ad6b58e`。随后 no-reset 回读 262144 字节，boot 21168、table 3072、app 166656 均与构建逐字节相符；0x6000–0x7fff、0x8000–0xffff（含 NVS/phy）、0x39000–0x3ffff 与本次备份一致。未读取整片写后 Flash，不宣称整片逐字节复核；未擦除 NVS/storage、访问 SD/PMU/VCOM 或相机。

设备留在下载器，esptool 已退出、串口无占用。烧写校验不是启动或 PSRAM PASS；等待人工断开 USB、完整关机再开机并重连，然后按 PROBE-STATUS.md 无 reset/断点读取 RAM。以下为此前阶段历史。

后续开发（非上板）：RAM 查询新版 app 166656 字节已编译/门禁通过，CTest 17/17；未连接设备、复位或烧写，设备仍第四轮 166512 字节探针。新状态地址仅适用于匹配新版 ELF，不能用于当前设备。详见 PROBE-STATUS.md。

当前设备状态：人工关机/开机、USB 重连后恢复检查成功，双核 Power on reset、硬件断点使能为 0，检查后恢复 running。新任务检查亦已进入空闲路径；第四轮结果重播探针（166512 字节）仍在设备，专用 PASS/FAIL 未取得。OpenOCD 已退出、串口无占用，不自动重烧/回滚或烧 SD/显示固件。

## 人工断电／USB 重连后：恢复已确认

用户回复“已重连”后，使用原 IDF 对应 OpenOCD 20260703/1000 kHz、禁 Flash bank 与调试服务，运行既有恢复脚本，无新 reset/烧写。私有 `jtag-recovery-03.log`：JTAG 双核识别成功，初始均 running；短暂停核后双核 PC=0x40378f66，复位原因均 1 Power on reset（不同于此前软件复位原因 3/21）。ibreakenable 读取都为 0，再将易失调试使能位置 0并回读均为 0；恢复双核后 POST_STATE 均 running，工具正常退出。

后续 `jtag-tasks-after-power-01.log` 使用已核对当前 ELF 的有界 RAM/任务脚本，再次检查当前调度器/任务状态，无新的复位、断点或烧写；检查后恢复双核 running。此证据说明此前不确定的恢复状态已解除，实际 Power on reset 后能够进入调度器空闲路径，而不是当前停于 PSRAM 时序调校。具体专用测试结果仍未保存到可查询的 RAM，不能从空闲路径和任务退出反推 PASS，亦未验证 PMU ready/屏幕/SD 或相机。

原始日志和脚本保留私有缓存，不提交设备标识。OpenOCD 均已退出、串口释放；不继续相同复位断点实验。下一步建议探针保留可查询 RAM 结果，在人工开机后读取，避免 USB 重枚举窗口；须另行实现测试/编译和获得烧写确认。

## 硬件断点／受控复位：USB-JTAG 异常，待人工恢复

用户明确回复“允许”。当前固件门禁与 SHA256 不变。离线 GDB/反汇编核对优化后代码：0x420088a8 位于校验结果已选择、缓冲已释放后的输出循环入口；a5 为结果字符串指针、a4 为完整通过模式数；0x4200891a 为结束输出入口。计划以寄存器和实时字符串双重核对 PASS/FAIL/ALLOC_FAIL，并验证结果重播前后一致，不仅依赖优化变量或最后一行日志。

私有 `jtag_result.cfg`，OpenOCD 20260703、ESP_FLASH_SIZE=0、ESP_RTOS=none、GDB/TCL/telnet disabled；先 halt，再一次 `reset halt`，配置两个 hw 断点后 resume。日志 `jtag-result-01.log` 显示复位原因 3 Software core reset，随后 `LIBUSB_ERROR_IO` / `LIBUSB_ERROR_OVERFLOW` / missing bitq，未获得 TEST_RESULT/PROBE_END_REACHED。异常清理尝试后输出 cpu0 unknown、cpu1 running、RESULT_CAPTURE_ERROR。OpenOCD 虽返回进程状态 0，诊断未成功，不能据退出码宣称测试通过或双核恢复。

随后仅尝试重连清理易失硬件断点 enable、恢复执行，不再请求 reset：20260703/5000 kHz 和本机已有 20260831/1000 kHz 分别写 `jtag-recovery-01.log`、`jtag-recovery-02.log`。均在 init/JTAG 扫描阶段出现 IN buffer overflow、LIBUSB_ERROR_OVERFLOW，退出码 1；恢复脚本尚未执行，因此不能声称硬件断点已清除或 CPU 已恢复。USB 串口仍枚举，枚举不证明 JTAG 可用。

没有 program/load/软件断点/Flash/SD/NVS 写入、PMU/VCOM/相机操作或修改固件/BSP/时序；受控复位与硬件断点改变易失执行/调试状态，OpenOCD reset 也会使用其复位辅助路径，不能称为零 RAM/寄存器干扰。所有 OpenOCD 已退出、串口无占用，停止自动尝试。需人工拔 USB、长按电源键关机、重新开机并重连，再先确认执行状态／清理断点；不在未知状态下继续 SD/照片测试或重烧。

## 实时 USB-JTAG：系统已进入空闲路径

用户要求继续启动阶段定位后，使用 IDF v6.1 tools.json 对应 OpenOCD `v0.12.0-esp32-20260703`。先核对本地 board/target 脚本，再以 `ESP_FLASH_SIZE=0` 禁用 Flash bank、`ESP_RTOS=none` 关闭自动任务解析，并禁用 GDB/TCL/telnet 服务；内置 USB-JTAG、5000 kHz。初次 init/examine 成功识别双核，没有执行 reset、program、load 或 Flash probe。官方方法依据：[ESP-IDF v6.1 JTAG 指南](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-guides/jtag-debugging/index.html)。

私有诊断脚本 `jtag_snapshot.cfg` 先确认双核 running，随后三次短暂停核读取寄存器，采样之间恢复执行并间隔 500 ms：

| 核心 | 三次 PC | a0 | a1 |
| --- | --- | --- | --- |
| cpu0 | 0x40378f66 | 0x8200d6b5 | 0x3fc9b490 |
| cpu1 | 0x40378f66 | 0x8200d6b5 | 0x3fc9bb00 |

当前 ELF addr2line：PC 对应 `esp_cpu_wait_for_intr`（cpu.c:64）；按 Xtensa windowed 返回地址映射的 0x4200d6b5/0x4200d6b2 对应 `esp_vApplicationIdleHook`（freertos_hooks.c:60/58）。第三次 halt 曾打印超时提示，但随后确认双核 halted、取得相同寄存器并恢复双核 running；不将超时提示单独判为失败。OpenOCD 首次打印 Debug controller/Core was reset 是观察到的复位状态，本次脚本没有发 reset，不能将其当成新复位命令。

进一步由离线 GDB 核对当前 ELF 的 TCB/List 偏移、长度、25 个优先级列表及符号地址，使用 `jtag_tasks.cfg` 短暂停核读取有界内部 RAM（不是调用设备函数）：scheduler=1、ticks=0x4cc43、task_count=4。当前任务 IDLE0/IDLE1；ready_0 两项 IDLE0/IDLE1、suspended 两项 ipc0/ipc1；其他 ready、delayed、terminating、pending 列表为空，各链表结束哨兵核验通过，没有 main_task。

这直接证明当前双核在 FreeRTOS 空闲路径、已越过早期初始化，不支持“最后日志是 PSRAM tuning，所以现在仍停在该阶段”。任务列表与本探针结束返回一致，但不能据此反推 64 KiB PASS：PASS、FAIL、ALLOC_FAIL 三条分支均返回。串口日志存在缺失，具体丢失机制仍未知；此前软件采集仅说明未捕获后续日志，不等于固件未执行。

每次采样后确认双核 running，OpenOCD 正常退出、串口无占用；无 Flash/SD/NVS/PMU/VCOM/相机写入，不改固件、BSP 或时序。JTAG attach/halt/resume 会影响实时运行与调试状态，不能称为零干扰读取或冷启动验证。下一步建议硬件断点捕获新一次探针结果，需单独确认受控复位；暂不升级到 SD/显示测试。

## 第四轮：授权软件复位对照

用户明确回复“允许”后，核对 Pico USB VID/PID、无串口占用，执行一次对照：

```sh
python tools/capture_pico_boot.py --port /dev/cu.usbmodem2101 \
  --connect-reset default-reset --seconds 30 \
  --output .cache/pico-replay-test-pyV5XS/software-reset-01.log
```

该工具先连接下载器，清除 S3 易失 force-download latch，再请求 watchdog 复位并有界采集；不是被动读取或完整断电实验。日志 2495 字节，ROM `rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)`；`Loaded app from partition at offset 0x10000`，Flash HPM 启用后进入 PSRAM 调校。最后为 `I (74) MSPI Timing: Enter psram timing tuning`，30 秒内没有 APP_ENTRY / PSRAM_64K_PASS / FAIL / ALLOC_FAIL / PROBE_SUMMARY / PROBE_END，也没有捕获 panic 文本。

因此这次软件复位没有取得应用启动和专用测试证据，不能从最后日志定位到某条指令，也不能认定 PSRAM 器件损坏或推断温度根因。Saved PC=0x40378f66 经当前 ELF addr2line 对应 `esp_cpu_wait_for_intr`（cpu.c:64），是复位前上下文，不能视作此次实时阻塞 PC，亦不证明复位前探针 PASS。

采集正常结束、串口无占用；没有 Flash/SD/NVS 擦写、相机操作或 PMU/VCOM 写入，不改 IDF/BSP/时序。设备保留新版探针，下一步实际断电/实时阶段定位须另行规划，停止 SD/照片上板测试，不自动重复相同复位或回滚。

## 第四轮：结果重播探针烧写与回读

第二次人工采集 `manual-power-boot-02.log`：端口打开后提示关机/开机，用户回复“已开机”；45 秒结束，0 字节、1 次连接、0 次传输错误，无任何标记。空文件已核实，当前 USB VID/PID 仍为 303a:1001，串口已释放，本地候选 SHA256 不变。这既不是 PSRAM 专用测试 PASS，也不是启动失败证据；人工操作与窗口的实际同步/供电变化仍未知。本轮不再重烧或复位；下一步建议经用户明确允许后做一次主动软件复位采集对照，需区分软件启动与冷启动，不以对照结果替代断电验证。

烧写后采集结果：提示端口打开后人工操作，用户回复“已开机”。私有 `manual-power-boot-01.log`：45 秒、0 字节、1 次连接、35 次传输错误，无标记。不能确认人工操作发生在完整观察窗口内，不能把重连尝试当 35 次芯片故障。随后核对当前端口 VID/PID 并补采 `powered-followup-01.log`：10 秒、44 字节、1 次连接、0 次错误，内容仅 `I (283) esp_psram: SPI SRAM memory test OK`。系统测试日志不能替代专用 64 KiB 结果；没有 APP_ENTRY/PSRAM_64K_PASS/FAIL/ALLOC_FAIL/PROBE_SUMMARY/PROBE_END。晚采可能读取缓冲内容，不能证明实际开机时间或定位阻塞点。本轮未再次烧写或主动复位，两个采集均已结束释放串口，下一次需同步人工电源操作。

用户在“是否允许烧写新版探针”后回复“准备好了”。核对 Pico 串口无占用、新 app 门禁和 SHA256 后，用 esptool 5.4 `--before default-reset --after no-reset read-flash 0 0x1000000` 备份当前整片 Flash；本轮私有目录 `.cache/pico-replay-test-pyV5XS/`。备份 16777216 字节，SHA256 `c0235e86e3291f48f0eb7bae21090f06614f2953aaa015fd2483caadf22f81dd`。

当前旧探针有效镜像校验/摘要匹配已知 166496 字节版本，分区表相同。初次整分区镜像检查拒绝非擦除尾部；进一步确认有效镜像、末尾扇区 FF 填充及后续 app 区域逐字节匹配原厂备份：短探针仅覆盖自身扇区，未擦除整个 app 分区。已提取并回读核验旧探针回滚 boot/app。另重新核验 `.cache/pico-coldtest-CvkJeU/` 原厂完整备份/回滚提取/Read_Pico 身份，旧探针备份不能当原厂恢复镜像。

在 `build-boot-probe` 中重新执行产物门禁，随后运行：

```sh
python -m esptool --chip esp32s3 --port /dev/cu.usbmodem2101 \
  --baud 460800 --before no-reset --after no-reset write-flash '@flash_args'
```

生成 flash_args 仅三个目标：boot 0x0、分区表 0x8000、app 0x10000，全部 Hash verified；擦除范围分别 0x0–0x5fff、0x8000–0x8fff、0x10000–0x38fff，无 erase-all。新 app 166512 字节，SHA256 `55cee33ab0d9b05bab1c31213186c1b136277e7eb4bbbb3bb769905768d980d7`。随后 no-reset 回读 256 KiB 为 `post-write-prefix.bin`：boot 21168/table 3072/app 166512 字节均逐字节相符；0x8000–0xffff（含 NVS/phy）与备份一致。

相关主机测试重跑 3/3，通过；此前全量 CTest 16/16。没有改源码/BSP/时序或访问 SD/相机/PMU。烧写字节验证不是启动验证，设备留在下载模式，下一步先采集再人工长按关机/开机，不以电子纸旧画面判断启动。

## 已开机后的补充只读检查

后续开发（未上板）：新增结果重播探针，实际源码主机 5 种场景验证、CTest 16/16、v6.1 构建/门禁通过；新 app 166512 字节，SHA256 `55cee33ab0d9b05bab1c31213186c1b136277e7eb4bbbb3bb769905768d980d7`。没有设备访问、烧写、复位或时序/BSP 修改；设备旧探针不变，新日志格式尚无真机证据。等待烧写确认，不能把新版主机 PASS 当设备 PASS。

用户再次回复“已开机”后，使用修正版采集器读取 10 秒：1 次连接、0 字节、0 次传输错误，无观察标记。私有日志 `.cache/pico-powered-check-vevM3L/powered.log`。探针结束后不再输出，此结果不能证明启动失败或专用 64 KiB 校验成功。没有主动复位、烧写或 SD/相机/PMU 操作，采集器已关闭串口；下一轮须先开启采集，再人工关机/开机。

## 修正版采集：应用已执行，开头日志缺失

用户再次回复“准备好了”，使用修正版 `wait_pico_boot.py`，提示 Transport open/capture ready 后人工长按关机、松开再开机，用户回复“已开机”。45 秒窗口、397 字节、2 次连接、32 次传输错误（包含等待重新枚举的失败尝试，不等于 32 次芯片故障）。私有日志 `.cache/pico-coldtest-CvkJeU/manual-power-boot-02.log`。

```text
I (82) esp_psram: Found 8MB PSRAM device
I (282) esp_psram: SPI SRAM memory test OK
I (861) boot_probe: heartbeat=1 uptime_ms=563
I (1361) boot_probe: heartbeat=2 uptime_ms=1063
I (1861) boot_probe: heartbeat=3 uptime_ms=1563
I (2361) boot_probe: heartbeat=4 uptime_ms=2063
I (2861) boot_probe: PROBE_END; no button UI or PMU readiness changes
I (2861) main_task: Returned from app_main()
```

这证明本次越过 PSRAM 调校、系统 PSRAM 测试成功且探针应用已执行完，不再是“所有测试启动均停在早期初始化”。未捕获 ROM 原因、APP_ENTRY、heartbeat=0 或 PSRAM_64K_PASS/FAIL；PROBE_END 在专用测试 pass/失败两条分支都会输出，因此不能由结束标记反推 64 KiB PASS。首次日志缺口/USB 重枚举与修正后的成功有相关性，但仍不是唯一根因或无干扰冷启动证明。

本轮重跑真实库 OS mock 测试 6/6，通过后仅采集，未发送串口数据/复位/烧写或访问 SD、相机、PMU。采集结束释放端口。设备是无 UI 的探针，不是完整产品固件；下一步补齐遗漏标记后再评估 SD/照片测试，不自动切换固件。

## 第三轮授权：烧录后等待人工开机

用户明确要求“继续开发，直到烧录完成需要人工确认为止”。本轮新增独立、不主动复位的采集工具及 9 项模拟测试，CTest 14/14；探针 v6.1 构建/门禁通过，应用仍 166496 字节，时序/BSP/波形/校准未改。

烧录前完整读取当前 16 MiB Flash。私有备份 `.cache/pico-coldtest-CvkJeU/pretest-flash.bin`，SHA256 `3ff43d39c776af19192d0ca3109711e3ff90a5f009e0a56bd6c489e7f6293e6f`；与第二轮备份整体一致。原厂 app 身份 Read_Pico / 8836142 / v6.1、镜像校验和分区布局确认通过，回滚 boot/app 已独占创建并回读核验。备份包含可能敏感配置，不提交或上传。

在 `build-boot-probe` 内，手动重新执行产物门禁后使用生成的 flash_args：

```sh
python ../tools/verify_firmware.py --build-dir . --mode boot-probe
python -m esptool --chip esp32s3 --port /dev/cu.usbmodem2101 \
  --baud 460800 --before no-reset --after no-reset write-flash '@flash_args'
```

地址为 boot 0x0、相同分区表 0x8000、app 0x10000；三个写入均 Hash verified，不是 erase-all。备份操作使用 default-reset 进入下载器；烧录/回读前后均 no-reset，没有请求烧录后的 watchdog/hard reset。Flash 写入不可避免涉及目标扇区擦除，但没有 NVS/phy/storage/SD 擦写。

烧录后读取 0x0/0x40000 至私有 `post-write-prefix.bin`（262144 字节）：boot 21168 字节、app 166496 字节与生成产物逐字节一致；0x8000–0xffff 分区表/NVS/phy 与本次整片备份逐字节一致。没有读写 SD 或相机；仅写 Flash 三个明确目标。串口句柄已关闭，esptool 报 Staying in bootloader。

按用户指定停止条件，暂不自动回滚或复位，留待人工电源操作后的证据。探针无屏幕/按钮界面，不报告 PMU ready，旧电子纸画面不变属预期，不能用“按键不响应”或“画面未变”替代探针日志。下一步先等用户就绪，再用 `wait_pico_boot.py` 采集人工完整关机/开机的日志；尚未看到 APP_ENTRY / PSRAM_64K_PASS / PROBE_END，不标记冷启动、SD、三张样片或相机 Gate 通过。

## 第三轮人工开机采集结果

用户回复“准备好了”后运行 `wait_pico_boot.py`，看到 Capture armed 后提示保持 USB 连接、按 PMU 说明长按至少 10 秒关机、松开再按至少 1 秒开机。用户随后回复“已开机”；这句回复不能单独证明 EN/实际供电状态。

私有日志 `.cache/pico-coldtest-CvkJeU/manual-power-boot-01.log`：45 秒、2495 字节、1 次连接、0 个传输错误。脚本未连接 esptool、未发软件复位/串口数据；后续依赖核对确认 pyserial open 会自动清输入并更新 DTR/RTS，原先全链路“未清输入”的描述不成立。USB 打开的电气影响仍不能排除。采集结束后释放串口。

```text
rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)
boot: Loaded app from partition at offset 0x10000
flash HPM: Enabling flash high speed mode by write sr3-bit5 (ZB25VQ128)
MSPI Timing: Enter psram timing tuning
```

未出现 APP_ENTRY / PSRAM_64K_PASS / PSRAM_64K_FAIL / PROBE_END。最后日志与此前一致，但不能断定阻塞指令、温度或芯片损坏；USB 复位原因也不能作为无干扰冷启动证据。本轮没有新的 Flash/NVS/SD 写入或相机操作；保留探针与证据，等待用户选择回滚原厂或继续受控排查，不自动替换固件。

## 第二次授权测试：启动探针与回滚

2026-10-01，用户明确要求烧写测试。重新确认同一 Pico 串口/ESP32-S3，并在任何写入前完整读取 16 MiB 当前 Flash；原厂 app descriptor 为 Read_Pico / 8836142 / v6.1，分区表与项目生成表一致。

- 新私有备份：`.cache/pico-pretest-G7bldI/pretest-flash.bin`，16777216 字节，SHA256 `3ff43d39c776af19192d0ca3109711e3ff90a5f009e0a56bd6c489e7f6293e6f`。不提交/上传；原始旧备份也保留。
- 本次备份的 boot/app 区域哈希与上一轮原厂备份相同；整体备份不相同，因此仍保留最新完整副本，不假设其他区域没有变化。
- 只读探针独立重编译/产物门禁通过：166496 字节，app SHA256 `efdd971f5af739bbe9493773ea6463be5ebbfcdfefbc2cabb04fe69e0287b578`。按经过核验的 flash_args 写 boot/table/app，全部 Hash verified。
- 首先尝试自建串口连接未同步、一次 watchdog 后 USB 重枚举导致旧句柄失效；这些是采集问题，不能作为应用失败的证据。随后使用 esptool 官方 CLI 等价连接并处理 USB 重枚举，取得有效启动日志（私有 `probe-reconnect-boot.log`）。

有效日志：

```text
rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)
boot: Loaded app from partition at offset 0x10000
flash HPM: Enabling flash high speed mode by write sr3-bit5 (ZB25VQ128)
MSPI Timing: Enter flash timing tuning
octal_psram: density : 0x03 (64 Mbit)
MSPI Timing: Enter psram timing tuning
```

20 秒捕获窗口中没有 APP_ENTRY / PSRAM_64K_PASS / PROBE_END。**烧写字节校验通过，启动门禁未通过**。此次日志明确是正常 SPI 启动，不是 ROM DOWNLOAD；未证明精确故障根因，也未进行物理冷启动。watchdog 请求后 USB 重连可触发另一 USB 复位，不能将该流程视为断电冷启动。

未继续烧写显示或存储诊断，未读取/写入 SD、未启用相机通信。没有改上游 120 MHz/HPM、驱动、波形或校准。

按预先说明的失败回滚，使用本次备份恢复 0x0/0x8000 的 boot 区域和 0x10000/0x400000 的 factory 区域，保留镜像 Flash 参数；两个区域均 Hash verified。然后回读 0x8000–0xffff 共 32768 字节，与本次备份该区域逐字节一致（含分区表、NVS、phy）。没有写 storage 分区或 SD。

原厂镜像回滚后的串口复位观察窗口 15 秒也只有 PSRAM 调校片段，没有可确认的应用就绪日志；不能据此声称原厂功能已恢复，也不能断定原厂镜像损坏。已关闭并释放串口。需要用户通过电源操作完整关机/开机，确认原厂界面和按键；有电池时仅拔 USB 不等于完整断电，不要求拆电池。

本轮未执行三张照片的实屏测试，未验证 SD、断电恢复或相机 Gate A–F。

收到上述关机/开机及功能核对请求后，用户回复“正常”。记录为原厂功能恢复的人工作证，不补写未采集的冷启动日志，不推定测试探针问题已经解决。设备保持原厂固件，不再次连接串口或烧写。

## 第一轮记录（历史）

用户确认 `/dev/cu.usbmodem2101` 为 Pico，并明确允许烧写测试固件。尚未收到备用 microSD/空诊断目录准备完成的确认。

## 已证明

- ESP32-S3，8 MiB PSRAM；原始 Flash 完整读取 16777216 字节。
- 私有备份：`.cache/pico-flash-backup-zujfGX/original-flash.bin`。
- 备份 SHA256：`6227f003b84312dc65e3a3e2cc1db7bcde521114cfe95a9b0bf406b6f8c4bfe3`。可能含设备配置，不上传、不提交。
- 原分区布局与项目一致：NVS 0x9000/0x5000，phy 0xe000/0x1000，factory 0x10000/0x400000，storage 0x410000/0x500000。
- `build-storage-test` 烧写成功；bootloader/app 写入 Hash verified，分区表相同而跳过；未整片擦除、未擦除 NVS 或 storage 分区。
- 串口出现 ESP-IDF v6.1 启动日志并加载诊断应用，Flash 高速模式与 PSRAM 参数初始化开始。

## 尚未证明

最后可见日志：

```text
boot: ESP-IDF v6.1 2nd stage bootloader
boot: Loaded app from partition at offset 0x10000
flash HPM: Enabling flash high speed mode by write sr3-bit5 (ZB25VQ128)
MSPI Timing: Enter flash timing tuning
octal_psram: density : 0x03 (64 Mbit)
MSPI Timing: Enter psram timing tuning
```

随后多次读取串口无新输出；monitor reset 操作也未观察到新启动日志。已退出串口监视，释放端口。不能仅凭日志中断断定精确根因，也不能将烧写成功视为 main、PMU、SD 诊断或显示通过。

上游配置为实验性 Flash/PSRAM 120 MHz（依赖 Zbit HPM）。本轮未更改该时序、上游驱动、波形或 PMU 校准；没有执行 SD 格式化或自动重挂载。未观察到 `SD diagnostic PASS`，SD 创建/恢复测试未计为通过。

## 需要人工操作

后续用户报告按键无反应。检查时串口仍可连接，ROM 输出 `boot:0x0 (DOWNLOAD(USB/UART0))` / `waiting for download`。该次串口打开可能影响 USB 控制线，因此不能用它追溯最初启动中断的精确原因。执行 esptool run 未写 Flash，下载器正常响应并请求硬复位；无复位模式监视仍未收到应用输出，已退出监视并释放串口。诊断程序本身没有按键界面，不能通过按键判断其测试结果。恢复原固件是可选的回滚路径，原始备份仍完整。

1. 完整断电并重新开机，确保 ESP32 确实失去供电；有电池时仅拔 USB 不一定断电，不要求自行拆电池。
2. 确认使用备用 microSD，且已手工创建空 `/frame/storage-test` 目录。
3. 告知完成后重新监视串口；若仍在同一位置停止，再基于新日志制定时序诊断方案，不擅自更改产品默认时序。

真实重启、拔卡、磁盘满、断电恢复、显示目视与相机 Gate A–F 均仍待验证。

## 用户授权回滚

用户明确要求恢复后，再次核验原始备份长度与 SHA256，提取原 boot 区域 0x0/0x8000 和 factory 区域 0x10000/0x400000。按备份原 Flash 参数写入，两个区域均 `Hash of data verified`。没有写 0x8000 分区表、0x9000 NVS、phy、0x410000 storage 或 SD；备份仍保留。

普通 USB 串口监视复位再次观察到 ROM 下载模式，不能据此证明原应用损坏。已退出 monitor，使用 esptool 支持的 watchdog reset 请求正常重启，未再次写入 Flash。当前串口已释放，待用户确认屏幕及按键是否恢复；没有把回滚的字节校验等同于界面/硬件功能验收。
