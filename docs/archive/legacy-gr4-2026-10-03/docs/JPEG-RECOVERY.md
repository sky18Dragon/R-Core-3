# 已有 JPEG / 生成缓存 / 索引严格只读恢复

日期：2026-10-02。仅指定三张私有诊断样片，不是相机照片库或正式产品导入。

## 最新真机结果：首次人工启动恢复 PASS

120秒被动采集结束，2次连接/206次传输错误（含人工断开/USB重枚举重连尝试，不据此判芯片故障），工具退出、串口释放。完整结果取得，不追加JTAG查询。

采集先就绪，再提示拔 USB、完整关机后开机重连，用户确认“已开机”。18292ms `JPEG_RECOVERY_PASS sequence=6 source=existing-only`，18293ms END；18293–27793ms全部20次清理后结果均 `r=1 done=1 mask=7 seq=6 code=0 read_only=1`，28293ms应用正常返回。已有三 JPEG、三生成缓存完整一致，库绑定/双有效相同快照/空日志/固定三KEPT及序号、源再比较和最终元数据复核通过；未创建、补写、修复、解码、提交或换图。

私有日志 `.cache/pico-jpeg-recovery-2fT36M/manual-power-jpeg-recovery-01.log` 2025字节，SHA256 `db050ec5a1b09fe8989a079b1d775d1c6303a884f111978e9db93758df04e6dd`。启动头未捕获，不证明独立电气掉电；20次是单轮结果重播，不是20次恢复。首次源创建、长期可靠性、生成缓存 SD 显示及相机链路仍需另验。本轮只采证/文档，无主机主动复位/烧写/JTAG/清卡/校准或相机操作，29组主机测试和编译沿用上轮；下文待启动为烧写阶段历史。

## 为什么单独验证

后续已有生成缓存只读SD显示已接入独立模式，行为/测试/设备进度见 [JPEG-CACHE-DISPLAY.md](JPEG-CACHE-DISPLAY.md)。本页恢复模式保持不换图，旧回滚固件保留；显示模式仍须独立真机与视觉验收。

上一版三张 SD JPEG 解码、Cover生成及缓存/索引同进程重开已真机PASS。`library=existing` 不等于严格只读跨关机恢复：准备流程允许创建缺失文件，状态发生变化时可能checkpoint。恢复版不调用准备/解码/提交，不重新创建成功状态来掩盖原有问题。

`FRAME_JPEG_RECOVERY=ON` 独立配置，禁止与其他诊断混用；固定检查 `/sdcard/frame/jpeg-source-test` 和 `jpeg-cache-test`。缺失、损坏、错库、未知条目均拒绝，不 mkdir、不补文件、不覆盖、不修复、不清日志、不格式化或擦NVS。旧 cache-test 损坏证据、cache-ram-test 和 storage-test 保留。不启动BLE/Wi-Fi/HTTP，不操作相机、不刷新照片。BSP挂载及硬件初始化不等同整机物理写保护。

## 检查顺序

1. 校验六份可信私有素材：三 JPEG 长度/完整CRC与三 PFG1 绑定、照片身份、1216×684、profile=1、Cover layout=2、完整包CRC匹配；1024字节缓冲必须容量足够且不与输入重叠。
2. 已有三个源文件：固定目录/白名单、常规文件/精确长度、1024字节分块完整逐字节比较、EOF及读后类型/长度检查。主机拒绝符号/硬链接；FatFS无POSIX链接语义。
3. 已有缓存库：绑定正确、两个快照相同且都有效、三已知KEPT/照片序号1/无删除意图/global sequence6、journal存在且为空。不容错回退坏单槽，不调用允许恢复补写的media_open；只读IO移除empty/replace/append/clear能力。
4. 三缓存类型/长度、完整逐字节比较可信包和EOF检查；不是仅比较头部或CRC。
5. 缓存读后再次完整比较三个JPEG源，再复核绑定、两个快照、空journal与目录内容；源比较期间改变元数据也不能通过。
6. 释放内部RAM缓冲后才发布总结果；不换图、无嵌入图片兜底。失败保留文件，输出阶段/图片序号/偏移/errno。

这是诊断单读取流程的有限时间窗核验，不提供对任意并发写入者的多文件原子快照。当前专用应用无并发写入任务。偏移沿用阶段报告，不一定是精确坏字节位置。完整源一致证明持久化源与可信JPEG相同，本模式不重新解码，不独立验证首次源创建或算法；算法由上一轮生成诊断验证。

## 结果与内部RAM

成功必须出现 `JPEG_RECOVERY_PASS sequence=6 source=existing-only`；清理后总结果 `JPEG_RECOVERY_RESULT r=1 done=1 mask=7 seq=6 code=0 read_only=1` 每500ms输出一次、共20次，不重复读取或写入。失败 r=2，mask/seq保持0并给出失败码。USB日志仍可能遗漏，不以缺日志判失败。

独立状态 `frame_jpeg_recovery_status`，56字节/14个uint32，magic=0x4a525231，version=1，启动最先清零；字段依次为 magic/version/app_entered/phase/checked_mask/sequence/result/failure_code/index/stage_crc/offset/system_error/buffers_released/done。phase=1应用/2只读检查/3结束，result=0待定/1通过/2失败，done最后发布。checked_mask只有整轮验证通过才为7，不是实时逐张生成进度。

失败码沿用枚举：1NVS、2BSP、3VCOM读取、4PMU ready、5屏幕尺寸、6SD、7素材尺寸、8缓冲分配/能力、10恢复校验失败（本模式不生成）。stage_crc为阶段字符串CRC32，仅用于辅助定位；errno负数编码UINT32_MAX。状态仅本次启动有效、非持久化。首次结果发布后不允许再次执行入口或改写终态。

不得用旧 `decode_jpeg_status.py` / `read_jpeg_status.cfg` 读取/解释此状态（生成模式16字，不是此恢复模式14字）。本轮不连接JTAG，不为已有结果追加暂停/断点；若以后需要原始RAM读取，另补匹配布局的离线解码/受控读取与验证。

## 主机与固件证据

最终CTest29/29（ASan/UBSan，34.29秒），含实际恢复入口：分配/内部RAM能力/卡/读取/启动失败、清理后发布、20次重播、每次启动重置与终态不变；文件测试：幂等内容不变、缺目录/源/缓存/快照不补写、短/坏源与缓存、错绑定/快照/全局及照片序号/状态/ID/删除意图、缓冲与源或参考重叠拒绝、非空日志、未知源与硬/符号链接、介质失效、缓存读取时源变化、第二次源比较时快照变化。共用只读缓存实现修改后，旧RAM恢复/加载回归同轮通过。

初次主机编译补齐stdio头；故障fixture改为每次重置空存储并使用可编码的TRASHED删除意图，避免把非法fixture当恢复验证。初次固件编译补FreeRTOS.h在task.h前；未改SDK/BSP。最终固件门禁Python23项、采集器15项通过；默认及专用v6.1编译/门禁通过。恢复专用门禁要求只读入口与内部DRAM符号、六素材范围，并拒绝JPEG解码、准备、media_open/create/import/action/checkpoint和照片刷新符号。

私有三图在已有主机库执行 `frame_jpeg_recovery_private` PASS sequence=6 source=existing-only，不创建测试库，不计作真机证据。旧 `build-jpeg-status` f34b…保留不改；新独立 `build-jpeg-recovery` app3180064字节 SHA256 `91830c393633d51a65e596fbdc99591f4bd198a9925294d78e3df41849d21326`，匹配ELF状态 `3fc94cfc 00000038 D`。默认回归app356672字节 SHA256 `eef4e64edfde8f20e962645cf01b98fbee0c22ba74d9080efbdf63c72808361e` 不烧。

构建参数：`idf.py -B build-jpeg-recovery -D SDKCONFIG="$PWD/build-jpeg-recovery/sdkconfig" -D FRAME_JPEG_RECOVERY=ON -D FRAME_DISPLAY_SAMPLE_DIR="$PWD/.cache/<private-assets>" build`，使用ESP-IDF v6.1。烧写/回读摘要和真机结果另见TODO.md及HARDWARE-RESULTS.md；编译/回读不算严格跨关机恢复PASS，生成缓存SD显示仍另验。

## 烧写阶段记录（当时真机恢复待验证）

完整16MiB私有备份 `.cache/pico-jpeg-recovery-2fT36M/pretest-flash.bin` SHA256 `da72bff70fe64e2be4844b0601f6b6325553e52ff81e405e889c4cf766c21368`；六素材/状态/旧JPEG镜像/混合尾部/分区/原厂回滚检查通过，旧boot/app回滚副本保留。三镜像Hash verified，0x410000扩展回读4259840字节全部一致，SHA256 `be9ff453f6ca86e7cc69c5a1da86f7d6520aaf8b7e623cb37bcc4761372976ad`；0x6000–0x7fff/0x8000–0xffff/0x319000–0x40ffff保护字节未变。新app较小，历史尾部保留不额外擦除。

备份进入下载器使用复位，烧写/回读均no-reset，串口释放、设备留下载器，新应用尚未运行。本轮未清SD/擦NVS/改校准/操作相机或连接JTAG。用户就绪后先采集再提示人工开机；首次严格恢复结果仍未知。
