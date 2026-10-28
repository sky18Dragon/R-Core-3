# Read Pico SD 接入与隔离入口

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-STORAGE.md)。

## 固定参考与硬件边界

参考厂家固定commit `28cde682a4468a581c278761922724f57d976418` 的 `components/read_pico/read_pico_sd.c` 和 `read_pico_board.c`，不修改子模块、不调用总BSP或厂家 `/sdcard` 挂载。

复用PhotoFrame唯一SDMMC backend：`/storage`、1-bit、CLK38/CMD42/D0=44、D1–D3未连接、最高协商频率40MHz。内部上拉显式opt-in，Read Pico开启，原WaveShare保持默认关闭和4-bit；这不构成实物电阻/信号完整性验证。

卡座CD为FCA9555 P0.6、低有效。HAL在共享递归总线锁内读取方向与输入；P0.6不是输入、IO失败或HAL未初始化均返回错误，不误判有卡。不写IOE/PMU、不借用显示租约。固定厂家映射与SD代码没有独立SD电源控制接口；适配不操作SY_EN/VCOM_EN、不猜测电源开关，卡供电与断电退出仍需实板验证。

## 所有权与退出

`read_pico_sd_mount/unmount/get_status` 是Read Pico单一外层适配，调用既有 `sdcard`，不另造相册系统。挂载锁内检查新鲜CD、占用HAL存储租约、只尝试一次，再核对CD；无卡/读取失败不初始化SDMMC，挂载失败不自动格式化或重试。挂载中拔卡在发布可用状态前清理。

挂载后观察到拔卡/CD错误，`media_valid`锁存为false；重新插卡不能自动恢复。状态观察不隐式卸载，因为未来业务可能持有文件。消费者须在任务前检查介质状态、在卸载前关闭全部文件；当前隔离入口的同步file-test/file-check/photo-check返回前关闭句柄，任一close失败则保留租约并拒绝正常stop。没有后台热插拔或跨任务文件引用计数，不作为并发业务保护已完成的证明。

正常退出：关闭消费者 → backend卸载 → 释放存储租约 → HAL退出。HAL在显示/存储租约有效时拒绝拆总线。

`read_pico_sd_note_file_close_failure()`只原子锁存、不执行IO/重试/卸载，直到重启不清除。状态报告介质无效/exit_failed，mount/unmount拒绝，保留SDK挂载和HAL租约；消费者关闭不确定时不继续正常拆除。文件JPEG、格式探测、PNG输入/输出及标准句柄包装已接入关闭保护；真实FAT关闭故障、其他直接消费者与并发仍待验收，不将钩子视为全产品生命周期完成。

固定ESP-IDF v6.1的 `esp_vfs_fat_sdcard_unmount()` 可在返回VFS注销错误前释放context/card。backend清除旧句柄，并将卸载错误锁存到重启；后续deinit返回原错、不再调用SDK，init/format拒绝。Read Pico同时保留HAL租约、公开 `exit_failed=true`，阻止正常HAL退出。不得重试失效句柄或当成功睡眠；其他板既有睡眠业务未在本轮改写。

## 编译与命令

[照片诊断profile](READ-PICO-PHOTO-VALIDATION.md)显式组合唯一SD适配与显示后端，独立于SD/display入口且互斥构建，开机不挂载/不开屏；mount→清白→人工白参考→一次照片门禁才进入IO。该profile已有S1单张证据，不扩展旧photo-check许可为新照片刷新。输出独占新建、失败不删路径、PNG原片保持；其他消费者与完整并发继续验收。

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

主机控制台显式选择 `scripts/read_pico_display_console.py --port <已核验端口> --profile storage`，白名单为status/mount/file-test/file-check/photo-check/upload-test/stop；不串用显示/格式化命令，须核验实板镜像支持及当轮范围。另加`--reset`才ESP复位，不自动发指令/重连/重试，部分写入即退出。主机检查数量与各轮实卡结果只在VALIDATION维护。

## 限定文件测试与已验证范围

便携核心`main/sd_file_probe.c/.h`只持有同步IO回调，板级状态和POSIX/VFS封装位于隔离入口，不改业务相册或厂家驱动。输入没有任意路径、删除、覆盖或格式化能力。固定目标为`/storage/RC3T0001.BIN`（8.3文件名），一次创建4096字节，256字节分块，内容由绝对offset确定、覆盖全部字节值。

首次`file-test`即消耗本启动唯一机会，包括无挂载、同名冲突或失败；不会自动挂载或重试，重复请求返回ALREADY_ATTEMPTED。同名（含FAT大小写等价）文件以`O_WRONLY|O_CREAT|O_EXCL`拒绝，不读取原内容或截断；锁定IDF v6.1 FAT VFS明确映射为`FA_CREATE_NEW`。测试卡仍应已备份且无重要数据，挂载/创建涉及FAT元数据，不是硬件只读。

流程：每次操作前检查新鲜CD/mounted/media_valid/exit_failed → 独占新建 → 有界分块写4096字节 → fsync → close → 只读重新打开 → 逐字节核对4096字节及EOF → close → 最终介质检查。支持正常短传输，零/负数/越界返回、缺字节、多尾字节或内容损坏失败；已有句柄在失败路径尝试close一次、不重试。关闭失败另锁存、禁止正常卸载/HAL拆除；此保守门禁不宣称可以取消卡驱动同步调用或解决所有掉卡恢复。

测试文件成功或失败后都保留，日志仅报告固定测试路径、错误阶段、written/verified、close_failed和errno，不列卡内其他文件。不自动清理部分文件、不生成新文件名绕过冲突。重新测试须另行安排测试文件处理；本次许可不包含删除原片或覆盖现有文件。

## 已保留文件只读核对

`sd_file_probe_check`接收独立read-only IO结构，仅有介质检查、open_read/read/close，没有create/write/sync能力；实际隔离入口只以O_RDONLY打开固定RC3T0001.BIN，无任意路径参数，不列目录或读取私人照片。file-test与file-check共享逐字节/EOF核对核心，但独立attempted/check_attempted门禁，每种命令每启动最多一次（含失败），不自动挂载/重试。

2026-10-07候选增加同一file-check的句柄观测：SD隔离入口只有该单一消费者，打开成功后必须active_io=1、关闭成功后0，介质/退出状态有效；不符则该一次检查失败，不新增路径/写能力/重试，不在持有句柄时尝试实卡卸载。该占用1→0契约有单次实卡file-check证据，未覆盖真实关闭故障/热拔/并发。

文件缺失返回READ_OPEN_FAILED及errno，损坏/缺字节/多尾字节/EOF读取错误失败，不创建或修复；完整结果只允许created=0/written=0/verified=4096、close_failed=0及有效介质。每次IO前读取新鲜CD/挂载状态，所有已打开句柄在返回前关闭一次；主读错误与关闭错误分别报告，任一file命令的close失败都锁存并阻止正常卸载/HAL退出。正常短读处理有界，不把字节已读完但EOF读取失败算通过。

## 标准文件句柄与元数据资源保护

`read_pico_sd_io_begin/end`在打开/关闭之间或元数据操作期间持有IO占用：begin和mount/unmount使用同一Board锁，end只做原子计数、不访问I²C；有占用时unmount立即拒绝且不使正常介质失效。移除/检测错误使新IO不可用，已知句柄可关闭后再卸载；关闭失败或不匹配释放锁存，保留占用/Board租约至重启。它是资源生命周期保护，不是SDK调用硬取消/超时、热拔恢复或数据原子发布。

三个SD相关profile链接包装fopen/fdopen/fclose、open/close、opendir/closedir、stat/mkdir/unlink/rename；覆盖本次查到的main、PNG/BMP/gzip、HTTP、相册/文件日志标准入口，不改固定厂家源码。非/storage路径和socket/console句柄透传；/storage前缀有边界检查。固定32槽，无动态分配，登记临界区内没有IO/日志；fdopen转移同一占用，真实关闭完成后才释放。新libc打开可能复用尚在记账的旧指针/FD，已为SD与非SD新对象解除过期键，避免误拒绝/泄漏。不得把这个API扩展为任意并发close同一无效句柄的安全保证；未审查的直接VFS、freopen/dup等别名或未来新消费者仍须检查。

## 验证与剩余边界

本测试卡有卡/无卡、限定4KiB写入回读、ESP重启/单次取卡断电后读取、file-check占用1→0、合成非覆盖发布有各自有限证据。最后已知部署见TODO，历史次数/镜像/许可和失败日志见VALIDATION。

开机不挂载或运行文件测试。挂载不是硬件写保护；完整供电/断电、热拔/满卡/关闭故障、所有消费者与数据原子性仍待验收，不由主机线程模型推导为FreeRTOS/FAT通过。
