# JPEG 诊断可查询结果

## 最新真机结果

首次人工启动采集整轮PASS：三图完整GENERATED source=SD，43663ms JPEG_SAMPLES_PASS sequence=6 library=existing，43664ms END后全部20次结果均 r=1 done=1 mask=7 commit=1 seq=6 origin=2 code=0，53664ms应用返回。120秒日志2503字节、2次连接／209次传输错误（含断开／重枚举尝试），私有日志SHA256 `333c1f33c64a2443e10eea0c312b29485d88d145bc220a48812088964f8bafba`。清理后结果重播已真机验证；64字节原始RAM读取尚未验证，无需为已完整取得的结果再暂停双核。

确认本轮三源验证／SD解码／Cover生成／完整参考比较及生成缓存/索引提交与同进程重开通过。existing仅说明缓存库来源，不证明源本轮新建或旧版整轮通过；准备模式仍可能写SD，不替代严格只读跨关机恢复。启动头缺失、未测独立电气掉电，不刷新照片、不计新缓存显示验收。采集已结束、串口释放，本轮无主动复位／烧写／JTAG／清卡／相机或校准操作。下文“待启动”为烧写阶段历史；下一步做严格只读已有产物恢复，再接新缓存SD显示。

## 目的与边界

上一版串口只取得第一张完整生成和第二张成功日志前缀，没有三图／索引总结果。缺日志不能证明失败。新版本保留内部 DRAM 状态，并在释放缓冲后每 500ms 输出一次简短 `JPEG_RESULT`，共 20 次；不重跑流程、不新增 SD/NVS 结果文件、不刷新照片、不操作相机。非阻塞 USB 日志仍可能遗漏。

每次 `app_main` 最先清零状态，记录应用进入、分配、逐张生成、提交、结束；NVS/BSP/VCOM/PMU/屏幕尺寸/SD 初始化失败也发布失败结果。状态不跨复位持久化，不证明冷启动、真实电气掉电或跨启动 SD 恢复。

`generated_mask` 只在对应 JPEG 源完整比较、SD 解码、Cover 渲染、完整参考包比较及源再比较成功后推进。三张成功不等于索引提交成功。只有三图 mask=7、缓存准备／完整比较／KEEP／checkpoint／同进程重开成功，核验实际 global sequence=6 后才标记 commit。关闭媒体、释放所有诊断缓冲后最后发布 done=1。库来源仅说明缓存库 created/existing，不说明每个 JPEG 源是否本轮新建。

## 64 字节布局（16 个 uint32）

| 字序 | 字段 | 解释 |
| --- | --- | --- |
| 0–2 | magic, version, app_entered | 0x4a505231、1、1 |
| 3–4 | phase, index | APP=1 / ALLOCATE=2 / GENERATE=3 / COMMIT=4 / DONE=5；图片 1–3，0 为无指定 |
| 5–8 | generated_mask, commit_verified, global_sequence, library_origin | 顺序有效 mask=0/1/3/7；commit=0/1；通过序号6；来源0未知/1新建/2已有 |
| 9–10 | result, failure_code | 0进行中/1通过/2失败；失败码见下 |
| 11–13 | stage_crc, offset, system_error | 阶段字符串 CRC32、阶段相关字节偏移、errno；负 errno 编码为 UINT32_MAX |
| 14–15 | buffers_released, done | 清理完成与最终发布标志；done 最后写入 |

失败码：1 NVS、2 BSP、3 VCOM读取、4 PMU ready、5屏幕尺寸、6 SD、7素材尺寸、8内部RAM能力/分配、9生成包分配、10源/解码/渲染/参考比较、11缓存提交、12内部状态不一致。首个失败保留；offset 沿用阶段报告，并非所有阶段都代表精确坏字节位置。stage_crc 仅辅助对应本版源码字符串，不是身份认证或无碰撞阶段编号。

## 读取方式与风险

工具 `tools/decode_jpeg_status.py` 仅离线解码，不访问设备。必须核对当前已烧镜像和匹配 ELF；magic/version 不能替代镜像核验。未发布 done 时始终输出 PENDING，不因为单个 PASS 字或缺日志判通过/失败。完成 PASS 还要求三图、提交、序号、库来源、清理和零失败字段全部一致；矛盾结果拒绝。

`tools/read_jpeg_status.cfg` 用于已初始化的 OpenOCD：ESP_FLASH_SIZE=0、ESP_RTOS=none、网络服务关闭，由匹配 ELF 指定 `FRAME_JPEG_STATUS_ADDRESS`。仅允许对齐的内部 RAM 64 字节范围。先确认双核 running，再短暂停核、确认双核 halted、两次读取16字且相同，最后逐核恢复并确认 running。异常初始状态不请求 halt/resume。读取或恢复失败不得认定成功，即使进程返回0；传输错误也应单独核对。

该动作会暂停执行、改变时序，不是零干扰观察；不得加入 reset、断点、Flash读写、内存写入或固件函数调用。本轮仅主机 fake target 验证，尚未用新脚本连接实机。后续先人工启动采证，再按需做受控查询；保留已有 SD 内容，不重写来“修复”未知结果。

## 构建与验证

保留 `build-jpeg-samples` 为旧版回滚参考；新版独立 `build-jpeg-status`，仍使用 FRAME_JPEG_SAMPLES 和相同六份私有 JPEG／参考包。主机 27/27 CTest（ASan/UBSan）；实际入口覆盖正常、分配／能力／拔卡／生成／提交失败、20次重播与先清理后发布；状态单测覆盖重置、序号／顺序、首错和终态不可变。离线解码7项含真实 Tcl fake target：正常、读失败、不稳定、单核未停、恢复失败、初始非running及5种非法地址。固件门禁22项，默认及专用ESP-IDF v6.1编译通过。

新版 app 3194784 字节，SHA256 `f34b1213b37a5cde769ade9e7ee64cfb8f49354aaa96215d4509204f2aa8b57e`；匹配 ELF 状态 `3fc94cfc 00000040 D frame_jpeg_status`。地址仅适用于此版。默认回归 app356672字节、SHA256 `980e868465cc58ddbc18e275188a235e4629e5d07b1fdb5bd953cb2e9e2def2b`，不烧默认版。

本轮直接烧写／回读通过。完整16MiB备份位于私有 `.cache/pico-jpeg-status-BFZwdX`，旧JPEG镜像、混合尾部、分区、原厂回滚及六素材/状态符号核验通过，旧boot/app回滚副本保留。三个镜像Hash verified，0x410000扩展回读完整相符，保护字节未变。备份进入下载器使用复位，烧写/回读no-reset；串口释放、设备留下载器，新应用尚未运行。本轮未运行新JTAG查询脚本。详细校验摘要见 TODO.md / HARDWARE-RESULTS.md；编译与回读均不等于设备 JPEG 总流程 PASS。
