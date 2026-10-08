# 当前技术架构

2026-10-08最新初始header：三配置头协调拆出便携request_header_put_checked，token key/control-free value（HTAB保留）、remove/absence/set/exact readback。新板主图全部自有初始header保留首错并在perform前停止、缩略图同源认证/custom失败沿用false；旧板原策略保持。普通/ASan各1142、Python85、六配置与独立103项通过，8e671530…未部署。不是borrowed凭据/URL/CA生命周期、全部SDK内部header/redirect/完整请求或真实验收，见[说明](READ-PICO-CONFIG-DOCUMENT.md)/VALIDATION末节，产品门禁保持，下方1135为历史。

2026-10-08最新请求配置：PhotoFrame下载前request_config在共享config_apply内复制revision/两组并生成自有JSON，三个配置头跨重试复用/各出口释放；便携header协调先删除/确认缺失再set/精确get，失败不perform/传输重试。原板契约保持，SHA锁定未改SDK请求头组件主机验证假成功和外层拒绝。普通/ASan各1135、Python84、六配置与独立96项通过，5a624b94…未部署。非整个HTTP/所有配置写者/退出ACK或真实NVS，其余headers/凭据/URL/证书/redirect与全图快照待处理，见[说明](READ-PICO-CONFIG-DOCUMENT.md)/VALIDATION末节，产品门禁保持，下方1118为历史。

2026-10-08最新图像消费者：PhotoFrame路径沿用，新板processing_options自有副本在config_apply保护内一次load/校验后发布；上传/下载显示先检查，PNG布局一次副本前置于显示流/输出创建并传入geometry。原板旧getter、JPEG EXIF固定Cover、RAM-only诊断策略保持。普通/ASan各1118、Python83、六配置及独立79项通过，13b43df2…未部署。不是整次请求/图像快照、所有直接NVS/方向/palette互斥或真实验收；请求头错误传播仍待处理，见[说明](READ-PICO-CONFIG-DOCUMENT.md)/VALIDATION末节，产品门禁保持，下方1105为历史。

2026-10-08最新文档/读取：便携config_document供本地三更新入口有界完整接收/文档检查，远端复用解析、仍保持2048头容量和三组契约；processing/palette候选checked转换、真实processing load错误不发布、两组GET也参与应用控制及serializer/响应检查。palette blob默认填充确定、数字不变。普通/ASan各1105、Python82、六配置/独立71项通过，e296869c…未部署。仍非所有便捷消费者/数值语义/直接写者或SDK硬超时、真实NVS验收，见[说明](READ-PICO-CONFIG-DOCUMENT.md)/VALIDATION末节，产品门禁保持，下方1074为历史。

2026-10-08最新全组应用：保留PhotoFrame三组/业务，本地general由便携config_apply_transaction保护apply+revision，远端一次解析/三组/finish共用config_apply_run；HTTP processing/palette POST/DELETE同控制。请求诊断在释放前复制，busy不执行、已接纳失败不追加响应。普通/ASan各1074、Python80、六配置及独立38项通过，085b6fce…未部署。非所有直接setter/读者/SDK或多组原子性，SDK等待不增新时限，见[配置说明](READ-PICO-NETWORK-CONFIG.md)/VALIDATION末节；产品门禁不变，下方1067为历史。

2026-10-08最新名称重试：保持PhotoFrame业务，检查式名称应用分离缓存接受与本启动服务确认，失败同值重试、成功跳过，显式同一副本回调在已有控制内；mDNS注册/更新重入不free活动操作。一般配置适配零等待串行，忙503不消费活动诊断，非远端三组/独立端点/版本完整事务。普通/ASan各1067、Python79、六配置/独立15项通过，aa768943…未部署；SDK等待/实际广播/DHCP及所有权仍未验收，见[名称与网络说明](READ-PICO-NETWORK-CONFIG.md)/VALIDATION末节，产品门禁不变，下方1045为历史。

2026-10-08最新名称/网络：继续PhotoFrame配置业务，新板网络PATCH便携解析/有效组合后一次检查保存；名称/网络/配网/加载共用已有控制、确认后发布，Wi-Fi/AP/HTTP/mDNS/HA/main相应读取改独立副本。普通/ASan各1045、Python78、六配置及独立35项通过，ebf3a86a…未部署。非整个GET/应用快照、mDNS生命周期、掉电原子性或真实联网；名称已存hostname失败的同值重试仍待处理，见[名称与网络说明](READ-PICO-NETWORK-CONFIG.md)及VALIDATION末节，产品门禁保持，下方1007为历史。

2026-10-08最新远端：沿用PhotoFrame服务器/三组配置，便携remote_config仅协调解析/顺序/报告阶段，不替代业务存储。已报告错误停止后续显示/下载ETag确认，304配置保留；processing set/调色板load/reload/检查式单调revision及五本地入口接入。44新增、普通/ASan各1007、Python76、六配置及独立依赖31项通过，e248c2c7…未部署。general仍有未检查void字段、非多组原子/全配置并发或真实服务器/NVS验收，产品门禁不变；见[远端说明](READ-PICO-REMOTE-CONFIG.md)/VALIDATION末节，下方963为历史。

2026-10-08最新时区：保留PhotoFrame配置业务，新板检查持久化后才发布/应用；已保存但TZ安装/读回失败保留候选、NOT_FINISHED/false。与NTP/加载共用零等待控制，HTTP时间戳/时间文本/时区独立副本在保护内生成，GET显式应用状态。25项真实配置C/2项Python、普通/ASan各963、Python73及六配置通过，f797b0bc…未部署。非全部配置/libc锁、全语法/DST或真实HTTP/产品验收，多字段/其他消费者仍待完成；见[时区说明](READ-PICO-TIME-CONFIG.md)及VALIDATION末节，下方938为历史。

2026-10-08最新SNTP：沿用PhotoFrame同步/RTC业务，外层read_pico_sntp持有副本/单DNS身份，SDK仅数字地址、不借用RAM或参与内部DNS。未改SDK构建开启来源/Originate和单调唯一时间戳；PCB预约避null断言，clock/自有启停/执行ACK检查，退出失败不继续无线/HTTP清理。新增54项含9固定SDK真实报文，普通/ASan各938、Python71与六配置通过，126a5b7a…未部署。SDK IPC仍可能无界等待，非认证/全协议/真实RF/全栈验收，产品门禁保持；见[同步说明](READ-PICO-SNTP.md)与VALIDATION末节，下方未处理服务表述为此前状态。

2026-10-08最新NTP配置：保留PhotoFrame配置/同步业务，仅新板检查式NTP setter与副本读取、启动加载共用非阻塞原子控制；完整commit后发布RAM，保存错误不触发同步，GET先获取独立NTP副本。新增17项真实配置C测试及2项源码接入检查，普通/ASan各884、Python69和六配置编译通过，b68e48b5…未部署。此保护仅覆盖检查式入口及加载，不包含SNTP仍用的借用getter、全部配置/SDK读写；固定SDK晚到DNS无请求号且stop不取消，netif deinit也不提供错误确认。服务所有权/关联/退出继续审查，产品/OTA/迁移门禁不变，见周期说明及VALIDATION末节；下方867是上一轮证据。

2026-10-08最新周期：保留PhotoFrame periodic接口/日同步业务，新板编译read_pico_periodic适配、旧板仍原实现。共享timer只合并原子通知，新增第五受管worker；批次/注册/时间戳/单任务触发串行，force易失不删键，读取错误不触发业务、完成记录未确认不重跑。恢复/启动返回按关闭周期接纳→合作等待→检查timer退出→无线/HTTP清理。普通/ASan各867、Python67通过，固件最终证据见VALIDATION末节。SDK SNTP void入队/无请求号并非退出或回复关联确认，NTP/其他配置并发及整机清理仍待审查，产品门禁保持，见[周期说明](READ-PICO-PERIODIC.md)。下面831为上一轮后台证据。

2026-10-08最新后台：沿用PhotoFrame四类业务任务，只新增固定槽SDK包装与启动门禁，不另建相册/产品状态。power与main逐次检查创建，HTTP ready发布后才放行，失败合作取消；恢复/启动返回最多2秒等其他自有回调返回，忙/超时不继续清理，不跨任务删除。睡眠门禁返回不继续轮播或二次启动。真实适配合成调度新增18项及Python3项，全量普通/ASan各831、Python64与六配置编译通过，8de840e5…未部署；最终固件证据见VALIDATION末节。合作取消不是硬中断/IO回滚/idle内存回收或整栈退出，周期timer/SNTP仍待审查，产品/迁移门禁保持，见[后台说明](READ-PICO-BACKGROUND.md)。下面813为上一轮HTTP证据。

2026-10-08最新HTTP资源控制：沿用PhotoFrame页面/认证/普通46个URI方法和AP15路由，Read Pico两入口共用read_pico_http_service占用与确切SDK句柄。仅板级HTTP生命周期适配，不是另一套产品状态系统；原子控制拒绝SDK调用重入，stop保留占用、release须确认无句柄/不确定状态。AP改射频前预约端口，DNS→HTTP→radio确认后释放；普通HTTP重复init不覆盖、注册逐项检查，失败/停止拒绝新请求，main检查启动/就绪。普通/ASan各813个唯一用例、Python61、六配置编译通过，69432b50…未部署。请求门禁不取消已开始业务，HTTP底层停止仍可能无界等待；后台/mDNS/整机失败退出及产品/迁移门禁未闭环，见HTTP说明/VALIDATION末节，下方800为前一轮共享无线证据。

2026-10-07最新共享无线控制：Read Pico manager/AP沿用原业务与连接mutex，begin/quiesce保留当前调用的SDK控制权，finish在同任务释放；持续服务用不复用owner和阶段保护，不跨返回持有FreeRTOS mutex。AP活动拒绝普通station接管，稳定活动允许候选/扫描/省电/hostname且仍串行；停止先取消目标/重连，busy不关闭SDK对象或给别人的锁，故障保留lease、后续显式退出确认。取消扫描不发布旧结果，初始化不能绕过STARTING/STOPPING/FAILED；main三处检查式station停止错误不继续网络工作。普通/ASan各800、Python59、六配置编译通过，558ce60d…未部署。回调按policy锁检查取消/阶段，不宣称SDK默认回调、全部后台/直接消费者被同一mutex覆盖；HTTP退出仍可能无界等待，产品/迁移门禁保持。详见Wi-Fi说明/VALIDATION末节，下方784为此前生命周期证据。

2026-10-07此前配网生命周期：保留PhotoFrame原入口/15个路由和保存业务，把Read Pico SDK资源操作分到wifi_provisioning_ap；控制入口原子互斥，当时manager只在ready时给出稳定自有AP接口，未形成共享保留锁。同步检查各启动阶段、全部就绪后发布本次session；退出取消旧请求、检查DNS→HTTP→radio，未确认对象不重复替换。main两处完成重启前接上AP退出，等待取消返回。普通/ASan各784、Python58、六配置编译通过，b3a89b26…未部署，保留历史证据。

2026-10-07后续DNS：便携captive_dns只处理有界单问题IN/非压缩名称及受限EDNS；网络适配不复制原报文后无界追加，启动同步检查AP地址绑定和任务创建。后台独占socket，停止等待完成位/关闭确认；超时或关闭错误不跨任务close/强删/重复复用，AP调用方传播错误并保留未确认对象。稳定事件组按应用生命周期保留，完成位不是任务内存已被idle回收的证明。普通/ASan各760、Python57、六配置编译通过，66245d1f…未部署；AP/HTTP自身和后台生命周期仍待审查，产品门禁不解除。以下731为上一轮初始化证据。

2026-10-07后续初始化：新板真实manager不用会abort的默认网口创建器，检查式驱动/接口/回调/模式准备后才发布ready；初始化原子互斥，成功重复调用不改已连接目标。发布前失败只清理确认自有对象，不删除共享netif核心/事件循环，稳定同步句柄按应用生命周期保留；SDK attach/清理不确定阻止重入。main启动/唤醒接收错误，非整栈退出或运行态deinit实现。普通/ASan各731、Python56、六配置编译通过，dfcf1fc0…未部署；产品门禁保持，AP/HTTP/后台生命周期继续审查。以下703为上一轮扫描软件证据。

2026-10-07后续扫描：沿用PhotoFrame配网HTTP与manager，新板空闲AP扫描复用连接锁，SDK列表清理/AP恢复成功后才发布；清理不确定阻止扫描/连接复用。响应使用现有cJSON，不新建网络状态系统；扫描锁不是所有直接SDK调用或后台生命周期的完整互斥。普通/ASan各703、Python55、六配置编译通过，3c2bf6df…集成未部署；部分/重复初始化与后台资源仍待审查，产品门禁保持。以下691为上一轮候选/保存软件证据。

2026-10-07后续AP保存：沿用PhotoFrame同步处理器，便携候选参数与顺序帮助函数不拥有产品/NVS/网络状态。新板先测试快照，再显式检查八键/一次commit，关闭后更新RAM；过期/失败不伪报完成，按本次连接ID清理候选且保留AP。普通/ASan各691、Python54、六配置编译通过，8ab2513d…集成未部署。其他Web setter/初始化与后台并发、多键/掉电原子性仍未完成，产品/迁移门禁保持，见[Wi-Fi说明](READ-PICO-WIFI.md)。以下659为上一轮。

2026-10-07后续连接核对：PhotoFrame真实Wi-Fi manager复用便携目标/关联检查，凭据成对只读提交输出、驱动只用RAM配置；私有队列栅栏加当前SSID/BSSID/地址核对，不拿旧bit通过候选。DNS/重连/省电错误传播、过期地址不输出，在线启动先检查地址。普通/ASan各659、Python53通过；固件见VALIDATION末节，仅软件未部署。网络参数提前保存/多键原子性、初始化退出/后台与真实时序继续审查，产品/迁移门禁不解除，见[Wi-Fi说明](READ-PICO-WIFI.md)。以下626项为上一轮。

2026-10-07后续Wi-Fi软件：沿用PhotoFrame配置/配网，将新板文本导入改为检查式只读返回，完整表单校验后使用字段；失败重配保留旧凭据，用易失性本次保存ticket跨任务通知，不新建产品配置/相册系统。普通/ASan各626、Python50、六配置编译通过，e42ca710…集成未部署；运行/迁移门禁不解除，连接事件/后台并发/全部配置保存继续审查，见[Wi-Fi说明](READ-PICO-WIFI.md)。以下600项属于上一轮软件证据。

2026-10-07后续启动保护：Read Pico真实main在板级/SD前只读严格加载配置；错误拒绝，不自动迁移/删除旧键，方向覆盖不静默重写。OTA在迁移/身份/恢复未验收时只报不可用，不启动网络/NVS/升级/自动确认；原板实现保留。普通/ASan各600、Python48、六配置编译及ELF检查通过，fa2a3f94…集成仅编译；完整产品门禁/设备状态不变。实际迁移及其余消费者仍待审，见[迁移审查](READ-PICO-MIGRATION.md)。以下947458be…/577项为上一软件轮。

2026-10-07后续软件：直接显示与URL保持独占接收对象，独立格式文件/预览不覆盖旧current；Downloads继续上游目录/EPDGZ契约，快照隐藏暂存后非覆盖发布。URL接收完整性与跨站凭据受保护，当前记录复制复用显示锁，保存失败明确已显示未完成。普通/ASan各577、Python46、六配置编译通过；集成947458be…仅编译，设备不变。持久链接仍原位写入，不宣称掉电原子性，消费者/NVS/OTA及长期清理继续审查。以下550项/727ffed2…与待更新表述为较早软件阶段，见[上传保护](READ-PICO-UPLOAD.md)。

2026-10-07最新隔离硬件：332,304字节/c876599f…合成发布SD镜像已获新许可仅factory部署/完整回读/分区保持，旧7861965c…完整保留。唯一mount/upload-test实际创建/写4112字节、发布1份、两次复读8192字节/EOF、同名EEXIST拒绝/内容保持及占用1→0通过；status正常，卸载/HAL/STOPPED及控制台exit0。只验收本卡这一个FAT路径，不把主机/隔离探针替代HTTP、两文件/掉电原子性或产品/NVS/OTA。以下待许可/未部署/当前786…为此前阶段。

2026-10-07最新软件：PhotoFrame HTTP接收在Read Pico用便携有界MIME+独占写入事务，板级FAT适配提供非覆盖发布；保留原业务目录与原板行为，不新增相册系统。原片/旧同名不预删除，关闭/介质不确定不发布成功；缩略图/主图不是两文件原子提交，失败可能保留部分可见结果。550普通/550ASan、45Python及六配置编译通过，新SD合成探针未部署，等待新许可；直接显示后处理/URL/其余消费者和产品/NVS/OTA门禁仍未关闭。见[上传保护](READ-PICO-UPLOAD.md)。

2026-10-07最新硬件：SD隔离328,016字节/7861965c…已获准factory app-only部署、独立全长回读/分区保持。实际libc/FAT只读打开占用1→关闭0，固定4096字节/EOF通过；最终status正常、卸载/HAL清理ESP_OK、STOPPED，控制台退出见VALIDATION。未刷新屏幕或启产品，9f6d6f6f…照片镜像已被替换；以下软件轮“未更新设备”为历史。基本单次路径不代替全消费者错误传播/并发/原子发布或NVS/OTA验收。

2026-10-07后续软件：SD占用保护在Board锁下预约、原子结束，标准文件/目录/元数据入口链接包装仅用于三个SD相关profile；固定32槽、无登记临界区IO，关闭不确定锁存、不拆挂载。快照独占新建并检查sync/close/介质，失败不删除目标。510普通/510ASan、42Python与六配置编译通过；待新的仅SD只读句柄真机验收，未更新设备，未开放完整产品或宣称全部消费者/原子发布完成。

2026-10-07最新软件：Read Pico内存JPEG复用文件路径的便携元数据、固定TJPGD、≤5MiB缩放RGB、EXIF1–8与居中Cover；压缩输入调用方持有，不二次复制，失败不开始显示。文件流仅允许JPEG/PNG检查式入口，拒绝其他格式误入MemFS删除逻辑。未操作设备；其他消费者关闭/并发、临时文件所有权及原子发布仍待审，产品/NVS/OTA门禁不变。以下内存JPEG待审及候选状态按历史日期保留。

2026-10-05最新设备状态：9f6d6f6f…读取优化版已获准仅factory部署、完整回读/分区保持，清白软件成功后按用户明确跳过白屏等待直接一次S1；处理56,161ms、GC16 36/36相398.130ms及退出正常。用户实拍确认画面正常，photo-confirm后一次stop、显示/SD/HAL清理ESP_OK，当前STOPPED、SD卸载、串口关闭。本轮不记人工白参考通过，仅计S1单张；下文62cac21b…/STOPPED及新候选“未部署”为历史阶段，不覆盖本段。产品生命周期/NVS/OTA门禁保持关闭。

2026-10-05最新软件：文件JPEG通过调用方供4KiB缓冲的便携适配减少小块物理读/IOE查询，块读取前后及最终EOF/关闭/介质门禁保留，缓存命中不重复查询；拔卡检测最多延后一个块，任何缓存像素不得提前发布。PNG输入已调整关闭后才允许刷新，并验证完整源和IEND/裁切外行。474普通/474ASan、38Python、6方向及六固件配置通过。9f6d6f6f…照片候选未部署，设备仍如下62cac21b…/STOPPED；不把该软件轮次记为性能/产品运行通过。

2026-10-05最新硬件状态：方向修复版62cac21b…完成factory更新/全长回读/分区保持、白参考和S1单张读取/处理/GC16；用户实拍确认正向正常，photo-confirm接受后正常stop，显示/SD/HAL均退出成功，当前STOPPED、SD卸载、串口关闭。下文“候选未部署”和旧设备状态属于此前阶段，不能覆盖本段最新结果；完整产品生命周期/NVS迁移仍待审查。

当前设备为单张照片诊断e7d5bd8d…/403,168字节，factory app-only完整回读/分区保持；清白通过后一次S1读取/EXIF/Cover/灰阶/GC16完成，但实拍上下颠倒，已拒绝并正常停止、卸载、关闭串口。方向修复候选62cac21b…/403,184字节已完成软件回归和六配置编译但未部署。此前d760371b…只读解码、d62fd8d2…取卡断电、6b85769b…文件写入、3168a7cc…有卡/无卡及下文6f2a4ea…显示结论属于各自历史镜像，不转移为整机或新软件验收，详见VALIDATION。

最新状态补充：`6f2a4ea…` PM锁候选已app-only部署/全长回读一致，分区表不变。显示适配层在静态清白及完整prepare→draw→stop事务持有CPU/APB max锁，使自动DFS下的直接LL RMT与CPU feeder保持官方240/80MHz条件；固定厂家子模块、18MHz PCLK、factory VCOM、默认36相、灰码和电源流程不变。清白78/78帧并人工通过；唯一官方局部页36相399.327ms，实拍16阶严格单调，确认后完整STOPPED。持续白偏根因已闭环，但单张标准页不等于P2重复性、照片链路或长期稳定完成。见 [16灰阶方案](GRAY16-VALIDATION-PLAN.md)，以下较早阶段状态保留为背景。

状态：2026-10-04 P0 完成，P1/P2b代码/主机/编译通过。16灰阶白偏已由PM锁真机A/B解决并确认，设备最后STOPPED；不计全部P2完成。普通诊断不启屏、不自动恢复。温度准确性、历史欠载/黑带机制、其他图案/照片链路、重复性和长期稳定仍待核验。主工程为`aitjcize/esp32-photoframe`，硬件参考为`MindReset/read_pico_firmware`；见 [上游基线](UPSTREAM-BASELINE.md)、[硬件资料](HARDWARE-REFERENCE.md)。

## 分层与数据路径

第一阶段数据路径：

`Web 上传 / SD / URL → PhotoFrame 存储与相册 → image_processor → 标准 4bpp → epaper_read_pico → EPD`

触摸输入调用 PhotoFrame 现有轮播入口；电源策略通过 Board HAL 调用 Read Pico PMU。第二阶段增加 `GR 适配器 → 下载本地副本 → 同一存储/图像流程`。

| 层 | 职责 | 改动方向 |
| --- | --- | --- |
| PhotoFrame 产品层 | Web、相册、REST、Wi-Fi、配置、轮播、OTA | 复用，仅做必要兼容修正 |
| 图像层 | 解码、方向、缩放/裁切、灰阶和标准 framebuffer | 先验证已有处理，后优化大图与缓存 |
| Board HAL | 初始化、电池/充电、共享资源、睡眠能力 | 新增 Read Pico 实现 |
| 显示后端 | `epaper_*` 契约到裸屏时序/波形 | 独立 Read Pico 后端 |
| 存储适配 | 上游挂载/文件语义到 SDMMC | 唯一/storage、只读CD与租约已接；真实业务存储层接入，产品运行待验收 |
| 输入适配 | CST836U 坐标/事件到轮播请求 | 驱动只报事件，业务层处理手势 |
| 硬件参考组件 | EPDiy、PMU、SY7636A、FCA9555、CST836U、波形 | 固定版本并保留来源，不引入阅读器应用 |
| 后续 GR 扩展 | 相机协议、能力验证、下载和同步 | 独立适配，默认不参与第一阶段运行 |

## 关键移植决策

### 显示

官方 BSP 确认 Read Pico 使用 ESP32-S3 16-bit LCD 并口驱动裸屏，与 IT8951 控制器后端有本质差异。采用独立后端复用 `epaper_*` 公共调用。已核对上游 init/display/clear/enter_deepsleep 返回 void，因此需额外设计结果状态与失败传播；具体显示后端在 P2 落实；当前诊断入口见 [P1 说明](READ-PICO-BRINGUP.md)。

1216×684 紧凑 4bpp 为 415,872 字节；PhotoFrame 偶数像素高 nibble，EPDiy 偶数像素低 nibble，均 0 黑/15 白。便携转换/事务及原生后端已实现；新板 display_manager/轮播只在检查式刷新成功后提交状态，其他板保留 void 契约。HAL 拥有总线，后端持有显示租约并按显示→总线锁顺序执行；失败锁存到重启。原生初次白屏需人工确认，内存填白不能替代物理参考。用户已允许厂家标准 SY 易失性启动配置、禁止 PMU/NVS 校准写；只读诊断已实机启动，一次清白的电源/退出与人工白屏确认通过。参考仅属本次启动，deinit 已清除，后续 GC16 需重新建立；产品时序、连续电源运行与完整迁移仍待验收，详见 [显示适配](READ-PICO-DISPLAY.md)。

GC16 全刷为首期照片模式；刷新、电源与资源释放串行进行。驱动、时序、波形、校准保留参考基线，VCOM 只读 PMU。

刷新事务增加可选 `read_temperature`：未配置时仍要求预先验证温度 gate；配置时先验证 PMU/帧，再 prepare 标准上电，随后在同锁下读有效温度，最后 draw/stop。Read Pico 使用主动采样路径，待机预检仅验证 OFF/故障，不将复位的温度寄存器当实时值。采样失败/越界/未写样本均不绘制且清理，原生后端锁存故障；不改变厂家驱动与高压启动配置，不新增单独试探上电。候选已 app-only 部署，主动采样单次 28°C/ON=1/PGOOD=1；准确性与连续性未验收。

标准4bpp保留16个输入码，不增加gamma/阈值补偿。厂家默认波形为36相、单个0–50°C温区；完整48相表声明约11级可分辨是能力资料，不是本机测量。历史待机0°C不作为有效采样；主动27–32°C均落在同一温区。最终同页面在CPU/APB PM锁下恢复11.090–11.112ms/相及16阶严格单调，证明此前白偏来自DFS下扫描事务失速，而非温度、灰码或波形选择。详见显示说明。

隔离显示测试入口有原六种便携图案、编号图和官方局部页，经标准4bpp→同一原生转换/GC16路径；官方页经实际厂家`epd_fill_rect(i*17)`逐字节核对目标后走from-white，只可在人工全白后的首次绘制使用，普通配置不链接。逐张显式绘制/人工确认，拒绝或IO失败锁存到退出；启动不自动开屏/轮播，不把transport completed当作实屏正确。历史DEBUG四角欠载0x400保留；PM锁候选的官方页已真机通过并确认，设备最后STOPPED/串口结束。仍需像素、照片链路、重复性及连续稳定验收；参考随退出清除。不为复现官方示例引入总BSP/NVS/SD/出厂标定副作用。

扫描观察分为`read_pico_display`的便携分析与`epaper_driver_read_pico`诊断profile专用link-wrap IO适配。显式静态清白记录末行首尾脉冲/填充与帧完成；局部页GC16模式记录36相时间及12条选定行的304字节脉冲指纹。两者均保留厂家数据源/返回值、回调内不分配/日志/IO，观测不完整时会话失败锁存。PM锁候选真机36/36相及全部行关系通过；指纹仍只证明renderer回调数据与顺序，不代替所有DMA/CKV/STV物理测量。固定第三方代码、波形、校准不改；详见显示说明。

### SD 与图像

单张照片诊断将唯一SD适配、真实文件JPEG处理及既有显示后端在独立profile内组合，命令只允许一次mount→clear-white→人工白参考→photo-show→人工验收→stop。首轮实拍证明处理/刷新贯通但native方向与厂家机壳正向相差180°；修正把Read Pico板级默认设为180°，隔离诊断封装同样变换，四角测试逐点对照厂家`EPD_ROT_INVERTED_PORTRAIT`。诊断完整4bpp sink不发布产品NVS/快照，使用无NVS默认调色板；普通/ASan各459项、Python38、六配置编译及5原片本机检查通过，62cac21b…未部署。已有产品NVS方向覆盖的迁移仍待审，完整产品不可运行。详见 [照片诊断](READ-PICO-PHOTO-VALIDATION.md)。

已核对 Read Pico SD 源码为 1-bit、CLK=38/CMD=42/D0=44，板级静态定义 D1–D3 为未连接。PhotoFrame SDMMC backend 已通过 `sdcard_config_t.bus_width` 显式选择 1/4-bit，1-bit 强制不使用 D1–D3；原板显式保留 4-bit。配置/路径校验、挂载失败与重复初始化保护、显式格式化及卸载状态已通过真实 backend 的主机替身测试，双板编译通过；实体挂载尚未验收。保留上游 `/storage` 及相册组织，不引入旧 `/frame` 事务库。厂家总初始化自带 `/sdcard` 挂载，移植时由 PhotoFrame 存储适配统一管理，避免双重初始化。

Read Pico只读/显示诊断不选择SD driver、不调用挂载。新增独立手动SD入口，仅显式mount才调用PhotoFrame backend，与显示配置互斥，不进入完整业务。HAL只读P0.6 CD，SD持有存储租约，正常卸载后才允许HAL退出。固定SDK卸载错误可能已释放句柄，backend清指针并锁存、外层保留租约；拔卡/检测错误使介质失效，不自动重挂。厂家代码没有独立SD电源接口，适配不写显示电源位。完整产品入口、实际供电/断电、读写与照片另行验收，见 [SD说明](READ-PICO-STORAGE.md)。

完整PhotoFrame业务的独立integration profile编译/链接真实main及Web/相册/处理/轮播，不调用其启动函数。存储层只走read_pico_sd，失败无fallback，格式化禁用；NVS恢复及Web全分区reset排除擦除，长显示路径完整保存/超长拒绝。新板睡眠禁止落入原板原生ESP分支。运行、白参考、配置/OTA迁移与PMU策略仍须人工/真机门禁；不可把编译配置当成产品固件，见 [集成说明](READ-PICO-INTEGRATION.md)。

先使用上游解码链路，测量内存峰值后再增加流式/缩放解码。baseline 与 progressive JPEG 分开确认。缓存若引入，保存业务层标准 4bpp，携带尺寸/格式/处理版本/源指纹/CRC，并以临时文件提交；不保存 EPDiy 私有状态作为通用缓存。

获准原片全部9–16MiB，大于8MiB PSRAM，不能先整体分配压缩文件。独立photo-check只检查读取/解码/EOF并丢弃像素，S1 ROM真机通过。随后Read Pico文件JPEG软件分支接入有界元数据解析、分块压缩输入、≤5MiB缩放RGB、EXIF 1–8无旋转副本映射，再复用现有居中Cover/灰阶/抖动与显示行列sink；完整源/EOF/关闭确认后才开始显示。它不是完全无RGB缓冲的MCU直出，也不修改原片。主机5张业务流程及433项回归通过，未部署或实屏；内存输入JPEG、PNG压缩源和关闭/并发仍待审查。见 [照片检查](READ-PICO-PHOTO.md)。

### 电源与触摸

报告指出 CW32 PMU 可切断 ESP EN，因此需要 Board HAL 明确区分原生 ESP 睡眠和外部 PMU 控制，不能只在普通 deep-sleep 前插入一个 PMU 调用。存储提交与显示/网络退出必须先完成，再交给 PMU。alarm、按键唤醒、失败处理与功耗单独上板验收。

触摸驱动只生成有界事件；手势复用现有 rotation 接口。刷新中重复切图合并；“上一张”需历史/顺序语义，不假设反向遍历即可正确。

### SDK 与频率

Read Pico 官方要求 ESP-IDF v6.1；PhotoFrame CI 为 release-v6.0。目标 v6.1 已通过诊断入口及原板编译，仍需显示集成/真机验证，并保留原有板型回归。官方 120 MHz 配置与 CI 80 MHz 配置用途不同；Flash/PSRAM 频率与 LCD 时序不能分别照抄，须记录实板依据和测量结果。

两个工程 NVS/PHY/应用偏移不同，新分区不能直接沿用旧 Demo 的恢复假设。实板已核实为 Demo factory 布局；专用诊断保持原表，仅覆盖 factory app，并通过回读/启动。用户放弃完整备份，无旧 app 完整回滚保证；完整业务/OTA 的迁移仍未批准。差异及保护范围见 [分区核对](UPSTREAM-BASELINE.md) 和 [烧写维护](DEPLOYMENT.md)。

## GR 后续接入

相机模块拥有协议与连接状态，不拥有第二套相册或显示系统。下载完成并通过校验后才发布本地副本；路径、超时、断线和重试均有界。相机与管理网络的切换、BLE/Wi-Fi 共存和功耗在第二阶段单独设计。

旧便携核心、日志格式和诊断固件可以提供经验，是否复用代码要重新评估兼容性与测试覆盖。旧工程验收不转移到新代码。

来源与候选版本见 [THIRD-PARTY](THIRD-PARTY.md)，原始建议见 [研究报告](reference/esp32-photoframe-read-pico-port-research.md)。
