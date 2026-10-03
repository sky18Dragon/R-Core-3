# 三张私人照片实屏测试

最新验收：用户提供三张面板照片并确认“比例正常，铺满屏幕，没有竖条”。照片对应测试图 3／2／1，画面与 Cover 预览相符，未见此前明显竖条；结合本轮三个刷新返回码 0／有序结束，本次嵌入 g4 全屏等比裁切实屏验收通过。旧 Fit 竖条记为本轮未复现、根因未确定，不声称已根治；没有新增清屏／波形／时序／VCOM 改动。此反馈轮仅文档更新，不重烧或访问设备。设备 JPEG／SD 读图／产品缓存／相机和长期残影仍待验证。以下“待目视确认”为历史阶段状态。

最新 Cover 真机进度：首次人工开机后 120 秒采集完成，5233 字节／2 次连接／37 次传输错误（含重枚举尝试）；APP_ENTRY、三个有序 SAMPLE_RESULT result=0、SAMPLES_END／应用返回已捕获。温度均 30°C、copy_ms 均 299、refresh_ms 401／459／459。第三张保持，采集结束／串口释放，无新烧写／主动复位；全屏／不拉伸及第一张竖条仍待用户目视确认，旧异常未关闭。以下“待开机”为烧写阶段历史。

当前设备是全屏 Cover 样片版，1594416 字节，SHA256 `9b95994cd9fc499ca131789dc4ff6d18531b2b60b5ed195533b380252482371a`；主机 20/20、v6.1 编译／门禁、全备份／回滚核验、授权烧写／1728 KiB 回读通过。检查范围内 NVS/phy 未变，设备留下载模式、串口释放，等待采集后人工开机；尚无新版刷新／视觉验收。以下旧 Fit 的结果不沿用到新版，第一张竖条仍未解决。居中裁切会将图 2 底部人物裁出，后续可提供裁切位置调节，不承诺主体完整保留。

2026-10-01 用户要求更新：新版样片采用 Cover 全屏／等比居中裁切，不拉伸、不新增白边。旧 Fit 和下面的首次面板反馈保留作历史，不视为新版验收。原图内置白框在展示副本中显式裁掉：1/2 解码后的 1500×1500 坐标，图 1 ROI `(141,127,1214,1243)`，图 2／3 ROI `(138,127,1221,1243)`；再以同一缩放比例覆盖 1216×684。ROI 来自本次样片白框边界内侧选取，不用于自动识别相机照片，不修改 JPEG 原片。正方形主体上／下部分被居中裁掉，不能同时承诺完整构图与全屏。新私有素材 `.cache/display-cover-FuVMxc/`，旧素材保留。

Cover 构建示例（同一诊断模式，独立 build／sdkconfig／私人资产目录）：

```sh
./build-host/host/frame_convert input.jpg output.g4 preview.pgm --cover
# 可选 ROI 是 EXIF 纠正后、实际解码图的坐标，非原 JPEG 分辨率。
./build-host/host/frame_convert input.jpg output.g4 preview.pgm --cover --crop x y width height
idf.py -B build-display-cover \
  -D SDKCONFIG="$PWD/.cache/sdkconfig-display-cover" \
  -D FRAME_DISPLAY_SAMPLES=ON \
  -D FRAME_DISPLAY_SAMPLE_DIR="$PWD/.cache/display-cover-FuVMxc" build
```

便携核心 Cover 使用同一个横／纵步长、浮点居中源视窗和双线性采样；先验证 ROI 边界／尺寸／EXIF／输出容量，非法输入不写输出。保留原 Fit API 和运算路径；CLI 默认 Fit 兼容旧工具，`--cover` 显式选择，Demo 0 的 SD JPEG 显示入口改用 Cover。旧 PFG1 导入／恢复仍 layout=1 Fit，新 Cover 未接持久化契约，不复用旧缓存。主机 20/20 通过，覆盖中心横／纵裁切、EXIF 1–8、ROI 去边／边界钳位、奇数 stride、非法坐标／溢出／容量、CLI 两模式与拒绝覆盖。

此次只改布局／资产，不新增清屏／换序，不更改波形／驱动／VCOM；第一张竖条仍待独立对照，不能将新版裁切说成竖条修复。以下是旧 Fit 实验记录。

视觉反馈更新：用户实机照片确认测试图 1（健身设施）有异常竖条，2／3 未报告同类问题。附件次序为图 2／3／1。三次刷新返回成功，但整轮视觉验收未通过。离线预览未见相同条纹；当前测试未做首次物理清屏，而 highlevel 的上一帧初始化全白，与原厂开机清屏路径不同，作为优先排查假设。需换序／后段重画图 1 区分内容与首刷，再比较清屏基线，不据此直接断言根因或改波形／校准。此反馈轮未改代码／烧写，详细记录见 HARDWARE-RESULTS.md。

最新真机状态：首次人工开机后 120 秒采集已结束，5232 字节／2 次连接／36 次传输错误（含重枚举尝试）；APP_ENTRY、三组有序 SAMPLE_RESULT 均 result=0、SAMPLES_END／应用返回已捕获。温度 30／30／31°C，copy_ms 300／299／299，refresh_ms 394／451／451。无捕获的 panic/watchdog；原有 GPIO／SD fallback 警告保留。第三张保持，串口释放，无新主动复位／烧写。刷新调用结果通过，方向／灰阶／残影视觉验收待用户确认；不证明设备 JPEG／SD 读图或相机。详细数据见 HARDWARE-RESULTS.md，下段是烧写阶段历史。

当前状态（2026-10-01）：主机 CTest 20/20、v6.1 编译／门禁通过；app 1594416 字节，SHA256 `bb884bc754943805dcdd6f6cf3bbb37e00c611da88f9b121d546bf3c59e653f2`。完整备份／回滚检查后按用户授权直接烧写，三个镜像 Hash verified、1728 KiB 回读相符，检查范围内 NVS/phy 未变。设备留下载模式、串口释放，等待先开启采集再人工关机／开机；尚无启动／刷新／视觉 PASS。备份与写入证据见 HARDWARE-RESULTS.md。

显式 `display-samples` 诊断构建，不是产品相册、JPEG 接收或相机同步。用户无法在电脑上向卡复制文件，因此将指定三张图片离线转换后的内部 g4 嵌入应用 Flash；不向 SD 测试库写入，不创建照片库或删除原图。固件和嵌入产物包含私人照片，不提交/上传。

## 数据与显示流程

原图均 3000×3000，默认设备 JPEG 限额不放宽。使用既有主机 `frame_convert` 的显式 1/2 解码，1500×1500 灰度中间图、Fit 完整构图/保留白边、16 灰阶，输出每张 1216×684/415872 字节内部 g4。与此前私人样片 PGM 逐字节相同，转换前后原图 SHA256 不变。私有数据位于被忽略的 `.cache/display-samples-tW4L6h/`，仅供这次构建；其他机器自行准备输入，不发布这些产物。

板级初始化仍使用锁定上游 BSP，不改波形/扫描时序；有效 PMU VCOM 读取及 report_ready 成功后，设置横屏几何 1216×684。嵌入三图长度全部核验后，通过驱动 epd_draw_pixel 做旋转/位序转换，不能直接 memcpy 到驱动 framebuffer。每 32 行让步，GC16 全屏刷新，读取 SY7636A 电源状态/温度；电源不就绪或刷新返回错误时关屏电源、停止序列，不声称画面已正确。

顺序 1（健身设施）→2（大树下的人）→3（草地人物和狗）；每张成功刷新后停留约 30 秒，第三张后结束返回、保持最后画面。期间短暂闪黑属于全刷过程，具体质量须目视判断；触摸/相册按钮没有实现，不要求按键切图。测试应用不复制或写入 SD 文件，BSP 后台仍会探测/挂载卡，不是硬件写保护；不要取卡或清理原诊断库。

## 构建

```sh
. .cache/esp-idf-v6.1/export.sh
idf.py -B build-display-samples \
  -D SDKCONFIG="$PWD/.cache/sdkconfig-display-samples" \
  -D FRAME_DISPLAY_SAMPLES=ON \
  -D FRAME_DISPLAY_SAMPLE_DIR="$PWD/.cache/display-samples-tW4L6h" build
python tools/verify_firmware.py --build-dir build-display-samples --mode display-samples
```

缺素材/错误长度/非显式选项拒绝构建。此模式与 boot/storage 互斥，产物门禁核验三个不重叠的 Flash 映射符号范围及各 415872 字节，拒绝与普通 JPEG 模式混淆。先全 Flash 备份、核验原厂回滚及生成 flash_args 后再烧写/字节回读，不擦 NVS/storage，不动 VCOM 校准。

## 验收

先开启 120 秒采集，确认端口真实打开后再人工关机/开机。新采集上限是 120 秒（默认仍 30），用于覆盖三张图约一分钟的序列；不是长时间无人值守监控。预期 SAMPLE_BEGIN/SAMPLE_RESULT index=1/2/3、result=0、SAMPLES_END。出现标记只代表观察到文本，不自动判定全序列/视觉 PASS，需分别检查每张返回码、温度、copy_ms/refresh_ms、空闲/最低空闲内存和有无 panic/watchdog。copy_ms 不含上电，refresh_ms 是更新函数耗时；最低空闲量不是完整堆分配峰值统计。

人工逐张确认图像对应、方向/镜像、完整构图/白边、暗部/天空层次、异常横条、残影及切图正常；第三张停留后可拍摄面板照片作证。物理看起来一直是旧原厂页面不作为成功，编译/写入/result=0 也不能代替目视验收。不证明设备端 JPEG 解码、SD 读图、照片库交互、相机 BLE/Wi-Fi/HTTP、温度压力或功耗验收。

主机 CTest 20/20（ASan/UBSan），新增实际 g4→驱动回调映射测试（全部灰阶、奇数 stride、完整面板尺寸、错长度不调用、回调失败中止、让步次数），产物模式门禁和 120 秒采集边界/新标记测试。真机结果另见 HARDWARE-RESULTS.md。
