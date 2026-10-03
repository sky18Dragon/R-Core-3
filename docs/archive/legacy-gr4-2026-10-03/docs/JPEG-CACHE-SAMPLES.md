# Pico 设备 JPEG 落盘／解码／缓存生成诊断

日期：2026-10-02。仅固定私有三图诊断，不是 GR IV 相机链路或产品照片库。

最新 `build-jpeg-status` 版人工启动整轮真机PASS：三GENERATED source=SD、JPEG_SAMPLES_PASS sequence=6 library=existing、END后20次相同JPEG_RESULT r=1 done=1 mask=7 commit=1 seq=6 origin=2 code=0及应用返回。完整源比较／SD解码／Cover生成／参考比较／源再比较／生成缓存及索引提交／同进程重开通过；不证明源本轮新建、旧版总PASS、严格只读跨关机恢复、独立电气掉电或本版SD显示。采集结束、串口释放，本轮未复位／烧写／JTAG查询，未清卡。新版27组主机测试／默认和专用编译为上轮结果，详见 [JPEG-STATUS.md](JPEG-STATUS.md) 与 HARDWARE-RESULTS.md。旧 `build-jpeg-samples` 保留回滚；下文25组／3193520字节及前两次旧采集为阶段历史。

## 范围与顺序

后续严格只读源/生成缓存/固定索引恢复已首次人工启动真机 PASS：独立恢复模式20次一致结果及应用返回，未创建/修复/解码/提交或换图，见 [JPEG-RECOVERY.md](JPEG-RECOVERY.md)。它不调用本节准备/解码路径，不能将本节existing PASS当严格只读跨关机证据；生成缓存SD显示仍另验。

独立 `FRAME_JPEG_SAMPLES=ON`，不刷新照片、不启动 BLE／Wi-Fi／HTTP。保留原有 `cache-test` 异常证据、已验收的 `cache-ram-test` 和 `storage-test`，仅使用：

- `/sdcard/frame/jpeg-source-test/source_1.jpg` 至 `source_3.jpg`：私有测试副本。
- `/sdcard/frame/jpeg-cache-test`：设备生成的 PFG1、固定诊断库身份、快照和日志。

逐图先校验嵌入 JPEG 与参考 PFG1 的来源长度／CRC／身份／Cover 布局／完整缓存；缺失源用独占创建、1024 字节内部 DMA RAM 搬运、flush／fsync／close 保存。已有文件必须类型／长度／完整字节一致；部分、损坏、未知条目不删除、不覆盖。主机额外拒绝符号链接／硬链接；板端 FatFS 没有 POSIX 链接语义。

完整读回源后，从 SD 文件调用 `frame_jpeg_load_scaled`，按 EXIF 方向解码灰度并等比 Cover 到 1216×684；生成 PFG1 与可信主机参考包完整逐字节比较。解码／渲染后再次比较源文件及目录。失败不发布输出，源文件保留；无自动重试或修复。

三张全部生成成功，才将**设备生成缓冲**交给独立缓存准备接口；每份 PFG1 完整持久化、比较、绑定检查后才本地 import／KEEP，最后 checkpoint／同进程重开，要求三 KEPT、各 revision=1、global sequence=6。这些 KEEP 仅为诊断身份，并非替用户作真实照片决定。后续图失败时此前图的成功事务可以存在，不是三图原子事务。

## 素材与内存

用户原始三张 JPEG 未修改、未加入仓库。沿用此前私有去白框派生图，再使用系统 `sips -Z 1600 -s format jpeg -s formatOptions normal` 制作本次副本：1562×1600、1571×1600、1571×1600，531395／564112／484161 字节。去白框 ROI 仅用于这些指定样片，不自动应用到相机照片。

参考包由主机 `frame_cache_fixture` 从这些**同一 JPEG 字节**重新生成，每份 415952 字节。不能使用此前 JPEG 的旧包 CRC，也不能沿用旧缓存显示验收作为本版证据。源 JPEG 与三参考包合计2827524字节，专用 app3193520字节，低于4MiB分区；未缩小分区或改上游时序。

板端提前分配三份 PSRAM 输出包，共1247856字节，再逐图分配／释放灰度（约2.5MB）、8192字节解码工作区及有界 EXIF 区；写卡缓冲必须内部 RAM／DMA 可达／4字节对齐。任何初始分配失败，在 SD 写入前停止。灰度分配失败则保留已完整落盘的源；不是正式 RENDER_PENDING 持久化状态。

## 主机与编译证据

CTest25组（ASan／UBSan），新增实际 JPEG 诊断入口及文件测试：生成缓冲来源、全部分配失败、DMA能力拒绝、逐图失败停止、缓存提交失败／清理；完整读回、解码分配失败源保留、有效但错误参考包拒绝、写中拔卡留下1024字节残片且不重写、未知条目／链接／损坏／读中变更拒绝、独立目录及幂等重开。新增故障测试首次以调用次数模拟拔卡未实际进入写入，修正为写块后的取消回调，确认残片留存后最终全量测试通过。门禁Python22项，要求源／解码／缓存入口、六份嵌入范围合法且不重叠，禁止旧准备及照片刷新入口。

指定三图在私有主机挂载目录中首次建库和已有库重开均PASS，生成三份完整包均与参考一致。该测试使用真实文件适配器，不是设备证据。可选工具 `frame_jpeg_samples_private` 接受绝对素材路径与已有测试挂载目录，不加入公共CTest。

默认及专用ESP-IDF v6.1编译／门禁通过。构建示例：

```sh
idf.py -B build-jpeg-samples \
  -D SDKCONFIG="$PWD/build-jpeg-samples/sdkconfig" \
  -D FRAME_JPEG_SAMPLES=ON \
  -D FRAME_DISPLAY_SAMPLE_DIR="$PWD/.cache/<private-assets>" build
```

## 实机判定与后续

第二次人工启动更新：先采集、再拔USB完整关机／开机重连，用户确认已开机。120秒191字节、2次连接／212次传输错误，第一张完整GENERATED（source=SD、1562×1600、scale=0、11011ms）及第二张BEGIN；30秒补采64字节、1次连接、0错误，第二张GENERATED source=SD半行前缀。证明本轮应用运行、第一张完整生成验证与第二张成功分支已到达；不证明本轮新建源文件、三缓存写入、总PASS或全局索引提交。无第三张／总结果且日志截断，两个采集已结束、串口释放；本轮无新实现／主动复位／烧写，不按缺日志认定失败，也不再重复相同开关机。下一步补可查询状态或严格只读已有产物检查，核实第三张和提交。SDK满FIFO非阻塞输出可丢字符只是候选机制，未定位本次根因。下文首次无应用日志为历史。

首次人工启动采集更新：120秒1337字节、2次连接／33次传输错误，仅捕获镜像加载至626ms PSRAM调校日志，无APP_ENTRY或任何JPEG测试结果。随后10秒无复位补采43字节、1次连接、0错误，仅633ms发现8MB PSRAM；可能读到滞留日志，不证明当前指令位置或完整PSRAM测试，不能据首次最后一行认定调校卡住。两次采集已结束，串口释放；本轮无烧写／主动复位／实现变更。当前启动结果未确认，不认定JPEG算法失败、SD写入失败或PSRAM已定位故障；先做断开USB完整关机对照，再推进本版验收。下文等待首次启动为历史。

2026-10-02 已按此前无需再确认烧写的授权完成完整16MiB备份、旧应用／原厂回滚及六素材核验、三个镜像写入与扩展回读。备份 `.cache/pico-jpeg-samples-qJO6sw/pretest-flash.bin` SHA256 `cb8a44325a125f5920d478d8a65a312f32d94ca526110f04a0bbaf7fc03ba3ba`；专用app SHA256 `fe48d59ce71e74565ee2b841c13bbf0b471c584d12871d8cb84460116a52f50c`。回读0x410000字节中boot／table／app完全一致，0x6000–0x7fff、0x8000–0xffff、0x31c000–0x40ffff与新备份一致；没有额外清Flash。设备留下载器，等待人工启动。主机采集器使用有标识符边界的匹配，JPEG_SAMPLES_END不误计为SAMPLES_END，循环测试14项通过。

整轮验收期待 APP_ENTRY mode=jpeg-samples、JPEG_SAMPLES_READY、三份 JPEG_SAMPLE_GENERATED source=SD，再 JPEG_SAMPLES_PASS sequence=6 和 JPEG_SAMPLES_END。目前仅局部生成证据，不能勾选整轮PASS。失败日志记录固定stage／index／offset／errno，不输出路径、相机序列号或照片内容。屏幕不换图是预期行为，不应据此判断未启动。

PASS仅表示本轮保存、SD解码、完整参考比较、缓存写入／本地提交及同进程重开，不证明实际电气掉电、长期可靠性、相机XS兼容或HTTP来源。后续先严格只读已有JPEG／缓存恢复，再接设备生成缓存的SD显示；源PFU1／PFI1事务与产品身份适配、满卡／拔卡／压力测试另行实现验证。

未开启远端ACK、相机写入、格式化、NVS擦除或VCOM写回；旧SD损坏根因仍未关闭。解码／渲染在块yield中让出CPU，但本诊断的卡移除判定在解码前后及源再比较处，不能称为每一JPEG块立即取消。
