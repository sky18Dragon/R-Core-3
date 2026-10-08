# Read Pico SD 接入与隔离入口

2026-10-07后续文件消费者软件：Read Pico wifi.txt新增非空普通文件≤1KiB、完整长度/EOF/错误/关闭/最终介质检查，解析后只返回值，不保存设备名或删除来源；IO/坏优先文件不被根文件掩盖。7项真实storage源码/主机文件与关闭故障用例通过；普通/ASan各626、Python50、六配置编译通过，仅软件无卡IO。SD镜像仍332,304字节/c876599f…，未重新部署或重跑探针；整个配网/实际FAT故障不计通过，见[Wi-Fi说明](READ-PICO-WIFI.md)及VALIDATION末节。

2026-10-07最新设备：332,304字节/c876599f…合成发布SD诊断已获新许可仅factory部署，独立全长回读/4096字节分区保持、旧7861965c…完整应用保存。唯一mount/upload-test通过created2/written4112/published1/verified8192/conflict_rejected1/close_failed0，各打开/关闭占用1→0、介质有效；最终status正常，卸载/HAL清理ESP_OK并STOPPED，控制台EOF退出0。保留4096字节BIN和16字节PART，不操作原片/旧测试文件、不刷新/启产品；以下未部署/待许可/786…当前状态为历史轮次，详见[验证记录](VALIDATION.md)。

2026-10-07最新软件：已准备332,304字节/c876599f…独立合成发布候选，新增精确upload-test、每启动仅一次、不自动运行；需要新的factory更新及两份合成文件写入许可。只验证新建/同步/关闭、FAT同名拒绝与复读，不替代真实HTTP/产品。候选未部署，当前仍如下7861965c…/STOPPED，见[上传探针](READ-PICO-UPLOAD.md)。

2026-10-07最新设备：新获准SD候选328,016字节/7861965c…已factory app-only更新，独立全长回读/4096字节分区保持，旧9f6d6f6f…完整应用保存。新启动唯一mount及file-check通过：实际打开active_io=1、关闭0，4096字节/EOF正确，created=written=0、关闭错误0；最终status正常、stop卸载/HAL清理ESP_OK、STOPPED，主机控制台退出见VALIDATION。不清白/显示、不file-test/photo-check、不修复文件或启产品；下述未部署及较早设备状态均属历史轮次。

SD软件接入与主机测试已完成，PhotoFrame业务存储层已接到单一适配；本卡有限实板挂载/文件测试通过，完整相框产品运行尚未验收。编译成功不等于挂载、读写或照片显示成功。完整栈默认禁止运行，见 [集成说明](READ-PICO-INTEGRATION.md)。

2026-10-04当前实板：用户确认样本已复制并允许更新后，325,904字节/d760371b…原片只读诊断仅factory烧写，全长回读一致、4096字节分区表不变。新启动一次1-bit挂载及photo-check，RC3P0001.JPG完整9,464,714字节/EOF和S3 ROM 1/4解码通过、关闭无错；显式卸载/HAL退出ESP_OK、STOPPED/串口关闭。卡仍插入且已卸载，文件保留，不创建/写入/修复/删除文件、不格式化/启屏/NVS/网络。不计EXIF/Cover/dither/照片显示、整机/异常断电、全部卡容量或长期稳定。

此前3168a7cc…完成本卡有卡/无卡门禁，6b85769b…完成专用4096字节写/sync/关闭/重开核对，d62fd8d2…完成ESP重启及单次取卡断电后只读核对；这些历史结论不转移为当前镜像的全部验收。写前旧诊断应用副本保留，RC3T0001.BIN未操作，不再用file-test重建。当前photo-check仅验收S1原片读取/解码，不是完整相框固件。见 [照片检查](READ-PICO-PHOTO.md)。

## 固定参考与硬件边界

参考厂家固定commit `28cde682a4468a581c278761922724f57d976418` 的 `components/read_pico/read_pico_sd.c` 和 `read_pico_board.c`，不修改子模块、不调用总BSP或厂家 `/sdcard` 挂载。

复用PhotoFrame唯一SDMMC backend：`/storage`、1-bit、CLK38/CMD42/D0=44、D1–D3未连接、最高协商频率40MHz。内部上拉显式opt-in，Read Pico开启，原WaveShare保持默认关闭和4-bit；这不构成实物电阻/信号完整性验证。

卡座CD为FCA9555 P0.6、低有效。HAL在共享递归总线锁内读取方向与输入；P0.6不是输入、IO失败或HAL未初始化均返回错误，不误判有卡。不写IOE/PMU、不借用显示租约。固定厂家映射与SD代码没有独立SD电源控制接口；适配不操作SY_EN/VCOM_EN、不猜测电源开关，卡供电与断电退出仍需实板验证。

## 所有权与退出

`read_pico_sd_mount/unmount/get_status` 是Read Pico单一外层适配，调用既有 `sdcard`，不另造相册系统。挂载锁内检查新鲜CD、占用HAL存储租约、只尝试一次，再核对CD；无卡/读取失败不初始化SDMMC，挂载失败不自动格式化或重试。挂载中拔卡在发布可用状态前清理。

挂载后观察到拔卡/CD错误，`media_valid`锁存为false；重新插卡不能自动恢复。状态观察不隐式卸载，因为未来业务可能持有文件。消费者须在任务前检查介质状态、在卸载前关闭全部文件；当前隔离入口的同步file-test/file-check/photo-check返回前关闭句柄，任一close失败则保留租约并拒绝正常stop。没有后台热插拔或跨任务文件引用计数，不作为并发业务保护已完成的证明。

正常退出：关闭消费者 → backend卸载 → 释放存储租约 → HAL退出。HAL在显示/存储租约有效时拒绝拆总线。

后续文件JPEG业务软件增加`read_pico_sd_note_file_close_failure()`：只原子锁存、不执行IO/重试/卸载，直到重启不清除。状态报告介质无效/exit_failed，mount/unmount拒绝，保留SDK挂载和HAL租约；不会在消费者关闭不确定时继续正常拆除。新增主机直接锁存用例通过，未进行真机FAT fclose错误注入；格式探测、PNG及其他文件消费者尚待逐项接入/并发审查，不将这个钩子当全产品生命周期完成。新SD构建e05fc2ed…/326,128字节仅编译，当前设备仍d760371b…。

固定ESP-IDF v6.1的 `esp_vfs_fat_sdcard_unmount()` 可在返回VFS注销错误前释放context/card。backend清除旧句柄，并将卸载错误锁存到重启；后续deinit返回原错、不再调用SDK，init/format拒绝。Read Pico同时保留HAL租约、公开 `exit_failed=true`，阻止正常HAL退出。不得重试失效句柄或当成功睡眠；其他板既有睡眠业务未在本轮改写。

## 编译与命令

后续软件补充：新 [照片诊断profile](READ-PICO-PHOTO-VALIDATION.md)显式组合唯一SD适配与显示后端，独立于下述SD/display入口且互斥构建，开机仍不挂载/不开屏；只有mount→清白→人工白参考→一次照片的命令门禁才进入IO。候选未部署，不把旧photo-check许可扩展为照片刷新。格式探测与PNG输出关闭已接SD原子锁存，其他消费者/PNG输入时序与并发仍待审；输出只独占创建、失败不删路径、PNG原片保持，不宣称全生命周期完成。

激活 [固定SDK环境](DEVELOPMENT.md) 后，仅编译：

```sh
python3 build.py --board read_pico --step firmware \
  --read-pico-factory-diagnostic --read-pico-storage-validation
```

输出为 `build-read-pico-storage/`、`.cache/sdkconfig.read_pico-storage`。与显示profile互斥，禁止额外flash/monitor/erase/fullclean参数。factory布局只匹配已核验设备，不是完整PhotoFrame分区迁移；编译器通用flash建议不构成操作授权。

入口 `main/read_pico_storage_validation.c` 开机只执行板级身份/VCOM GET和CD检测，不挂载、不启屏/NVS/Wi-Fi、不格式化、不创建目录或读写文件。明确选择后，串口接受以LF或CRLF结束的精确命令：

- `status`：新鲜CD和 mounted/media_valid/exit_failed，不列私人文件名。
- `mount`：一次挂载请求，不自动重试，已有挂载拒绝。
- `file-test`：明确一次限定文件写入/回读，见下节；已保留同名文件，后续请求会拒绝创建，不要重试或删除/改名绕过门禁。
- `file-check`：只读核对已保留专用文件，源码与当前镜像支持；此前d62fd8d2…的4096字节/EOF核对通过，本轮没有发送此命令。
- `photo-check`：当前d760371b…支持，固定RC3P0001.JPG分块只读JPEG解码/完整源读取及EOF检查；本轮S1单次ROM解码及正常退出通过，不刷新。见 [照片检查](READ-PICO-PHOTO.md)。
- `upload-test`：当前332,304字节/c876599f…镜像支持，本轮新许可下单次已通过；独占新建两份合成文件、非覆盖发布/复读与同名拒绝，每启动一次，见[上传探针](READ-PICO-UPLOAD.md)。产物已保留，不再次运行/删除/换名，不能套用此前只读许可。
- `stop`：文件测试返回后先卸载，成功后HAL退出；文件关闭失败则拒绝卸载/HAL拆除、保持租约，报告FAILED。成功STOPPED、失败FAILED，两者均结束命令处理，需重新启动开启新会话。

解析器固定16字节缓冲，超长、二进制、控制字符和非白名单整行拒绝，不执行前缀或溢出尾部。使用同步SDK挂载，未新增可取消硬超时，stop不能承诺打断SDK。启动不代发HOST_READY，因此已有PMU启动超时行为仍是实板限制。

主机控制台显式选择 `scripts/read_pico_display_console.py --port <已核验端口> --profile storage`，最新源码白名单为status/mount/file-test/file-check/photo-check/upload-test/stop；默认display白名单不变，不串用显示/格式化命令，必须核验实板镜像支持及当轮许可。只有另加 `--reset` 才ESP复位，不自动发指令、不自动重连/重试。部分写入即退出；最新软件build28/console17共45项通过，不访问真实串口；较早file-check/photo-check轮次的检查数量仅属历史证据。

## 限定文件测试与已验证范围

便携核心`main/sd_file_probe.c/.h`只持有同步IO回调，板级状态和POSIX/VFS封装位于隔离入口，不改业务相册或厂家驱动。输入没有任意路径、删除、覆盖或格式化能力。固定目标为`/storage/RC3T0001.BIN`（8.3文件名），一次创建4096字节，256字节分块，内容由绝对offset确定、覆盖全部字节值。

首次`file-test`即消耗本启动唯一机会，包括无挂载、同名冲突或失败；不会自动挂载或重试，重复请求返回ALREADY_ATTEMPTED。同名（含FAT大小写等价）文件以`O_WRONLY|O_CREAT|O_EXCL`拒绝，不读取原内容或截断；锁定IDF v6.1 FAT VFS明确映射为`FA_CREATE_NEW`。测试卡仍应已备份且无重要数据，挂载/创建涉及FAT元数据，不是硬件只读。

流程：每次操作前检查新鲜CD/mounted/media_valid/exit_failed → 独占新建 → 有界分块写4096字节 → fsync → close → 只读重新打开 → 逐字节核对4096字节及EOF → close → 最终介质检查。支持正常短传输，零/负数/越界返回、缺字节、多尾字节或内容损坏失败；已有句柄在失败路径尝试close一次、不重试。关闭失败另锁存、禁止正常卸载/HAL拆除；此保守门禁不宣称可以取消卡驱动同步调用或解决所有掉卡恢复。

测试文件成功或失败后都保留，日志仅报告固定测试路径、错误阶段、written/verified、close_failed和errno，不列卡内其他文件。不自动清理部分文件、不生成新文件名绕过冲突。重新测试须另行安排测试文件处理；本次许可不包含删除原片或覆盖现有文件。

本入口只验证当次fsync/关闭/重新打开路径；不能替代卸载后重新挂载、断电持久性、满卡/坏卡/热拔、真实照片或并发消费者验收。未接入Web/显示/NVS/网络/PMU写。此前软件轮次新增19项核心/故障/实际POSIX独占创建用例，普通及ASan/UBSan总375项通过；后续本轮已独立完成一次真机4096字节测试，不转移为其余异常/持久性或产品验收。

## 已保留文件只读核对（已部署，重启与单次取卡断电后读取通过）

`sd_file_probe_check`接收独立read-only IO结构，仅有介质检查、open_read/read/close，没有create/write/sync能力；实际隔离入口只以O_RDONLY打开固定RC3T0001.BIN，无任意路径参数，不列目录或读取私人照片。file-test与file-check共享逐字节/EOF核对核心，但独立attempted/check_attempted门禁，每种命令每启动最多一次（含失败），不自动挂载/重试。

2026-10-07候选增加同一file-check的句柄观测：SD隔离入口只有该单一消费者，打开成功后必须active_io=1、关闭成功后0，介质/退出状态有效；不符则该一次检查失败，不新增路径/写能力/重试，不在持有句柄时尝试实卡卸载。software已编译但这两点还没有真机证据。

文件缺失返回READ_OPEN_FAILED及errno，损坏/缺字节/多尾字节/EOF读取错误失败，不创建或修复；完整结果只允许created=0/written=0/verified=4096、close_failed=0及有效介质。每次IO前读取新鲜CD/挂载状态，所有已打开句柄在返回前关闭一次；主读错误与关闭错误分别报告，任一file命令的close失败都锁存并阻止正常卸载/HAL退出。正常短读处理有界，不把字节已读完但EOF读取失败算通过。

当前诊断仍开机无挂载/文件IO，完整产品/显示/NVS/网络/PMU写保持关闭。本轮获准后只执行status→mount一次→file-check一次→status→stop，不发送file-test，已证明此前文件在ESP重启及FAT重新挂载后完整。单次ESP USB reset不是卡断电，不将此称为断电持久性。真实断电/重新插拔须正常卸载后人工安排；挂载不是硬件写保护，不保证FAT元数据完全不变，不能把无应用写入等同于硬件只读。软件、部署及实卡证据分别见 [验证记录](VALIDATION.md)。

后续本卡单次取卡断电验证另行完成：在前轮正常卸载/STOPPED后，用户按完整取卡再插回要求确认“已重新插入”，本轮新启动同一只读序列再次核对4096字节及EOF成功。断电动作来源是人工确认，不是固件测量卡供电；没有记录断电持续时间，不扩大为整机断电、异常断电或长期数据保持。

初始软件接入轮次没有串口、烧写/回读或实体卡IO；后续获准SD轮次完成旧镜像有卡/无卡门禁，并在本次新镜像完成上述有限文件IO及退出。文件写入许可仅用于新建专用测试文件，不碰原片。完整PhotoFrame业务已有NVS防擦除及启动门禁，并发文件消费者退出、照片链路和实际迁移仍待完成，见 [TODO](../TODO.md)、[验证记录](VALIDATION.md)。

## 2026-10-07 · 标准文件句柄与元数据资源保护（仅软件）

`read_pico_sd_io_begin/end`在打开/关闭之间或元数据操作期间持有IO占用：begin和mount/unmount使用同一Board锁，end只做原子计数、不访问I²C；有占用时unmount立即拒绝且不使正常介质失效。移除/检测错误使新IO不可用，已知句柄可关闭后再卸载；关闭失败或不匹配释放锁存，保留占用/Board租约至重启。它是资源生命周期保护，不是SDK调用硬取消/超时、热拔恢复或数据原子发布。

三个SD相关profile链接包装fopen/fdopen/fclose、open/close、opendir/closedir、stat/mkdir/unlink/rename；覆盖本次查到的main、PNG/BMP/gzip、HTTP、相册/文件日志标准入口，不改固定厂家源码。非/storage路径和socket/console句柄透传；/storage前缀有边界检查。固定32槽，无动态分配，登记临界区内没有IO/日志；fdopen转移同一占用，真实关闭完成后才释放。新libc打开可能复用尚在记账的旧指针/FD，已为SD与非SD新对象解除过期键，避免误拒绝/泄漏。不得把这个API扩展为任意并发close同一无效句柄的安全保证；未审查的直接VFS、freopen/dup等别名或未来新消费者仍须检查。

主机普通/ASan各510项、Python42项及六配置编译通过；新增24项含生命周期/关闭锁存、描述符转移、目录、元数据、注册表上限、跨路径句柄复用及快照保护。8线程各50次打开/关闭的用例额外执行100次成功；只证明主机模型，不替代FreeRTOS/实际FAT/DMA/掉卡验收。初次普通回归揭露旧记账窗口的句柄复用竞态，修正后确定性用例及全量通过，失败记录不抹为已通过。

上述SD候选已获新许可，status→唯一mount→唯一file-check→status→stop完成；固定4KiB内容/EOF、占用1→0及正常卸载/HAL退出通过，不创建或换文件，不刷新屏幕/启产品。已有新标准库适配的单次基本实卡证据；继续上传错误反馈、所有临时文件/原片所有权、原子发布及配置迁移审查，不宣称故障/并发/全产品已验收。
