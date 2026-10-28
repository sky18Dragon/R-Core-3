# 验证记录

当前进度见 [TODO](../TODO.md)；测试怎么跑、各层何时做见 [测试策略](TESTING.md)。本文件只保留**汇总与候选台账**，按层分开：主机测试、固件编译、烧写回读、真机各自判断，不互相代替；旧失败不因后来通过而删除。

2026-10-19前的逐轮详细记录（2686行，含全部命令、日志位置、人工观察与失败细节）已原文保存在 [验证全文](history/2026-10-11/VALIDATION-full-log.md)（来源SHA256 `b3b89ffe1582405dd48bdfa1c697a8746214c281fb0bd56ac8dffffd1bb17718`）。其中“当前/本轮/下一步”只在当时成立，不作为实时状态或新操作许可。

## 记录规则（简化）

每个候选一条，字段如下，不再为每个小改动写长篇：

- 日期、改动范围、提交/工作区状态。
- 主机：T0/T1命令与通过/总数。
- 编译：profile、字节数、SHA256。
- 烧写回读：写入范围、回读是否一致、分区前后是否一致（未做则写“未执行”）。
- 真机：按 [验收单](TESTING.md) 逐项通过/失败与现象，日志位置。
- 限制与对应TODO。

私人样片和可识别设备信息留在私有目录，公共记录脱敏。

## 关键里程碑与结论（来源：全文，按层）

| 日期 | 层 | 结论 | 边界 |
| --- | --- | --- | --- |
| 2026-10-03 | 主机/编译 | P0：上游PhotoFrame基线在固定ESP-IDF v6.1下主机/Web测试与原板编译通过 | 不代表Read Pico |
| 2026-10-03 | 真机 | factory只读诊断app-only部署并回读；PMU 1.0.9、出厂VCOM 1580 mV只读、PSRAM自检通过；受控清白与人工全白确认 | 用户放弃完整备份，无旧app完整回滚 |
| 2026-10-03/04 | 真机（失败，保留） | GC16白偏：连续高压、GC16铺白预处理、精确684行、18MHz等多个变量均未解决 | 作为失败保留，不当作仍待修复 |
| 2026-10-04 | 真机 | 根因：自动DFS下EPDiy相位缺CPU/APB PM锁。加锁候选305,104字节 SHA256 `6f2a4ea106f7aab2c1d8ff2fd2605f70b0aefafaa5c437b0d688106a12055255`，相位11.09–11.11 ms，标准页16阶严格单调 | 单次，重复性/其他图案/长稳未验 |
| 2026-10-04 | 真机 | SD隔离诊断：有卡/无卡检测、4KiB限定写入回读、重启及单次取卡断电后只读核对通过 | 有限测试卡，非完整业务 |
| 2026-10-04/05 | 真机 | S1原片ROM解码通过；首张照片显示倒置被拒绝，方向修复后S1单张通过（S1处理56,161 ms，GC16相位合计398.130 ms） | 单样本 |
| 2026-10-07 | 烧写回读/真机 | SD合成发布诊断 332,304字节 SHA256 `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3` app-only部署，全长回读一致，分区保持，有限文件发布与清理通过 | 最后已知部署，设备当前状态未核查 |
| 2026-10-08 | 主机/编译 | 软件快照提交 `53e441e`：CTest 1142/1142（普通与ASan），六配置编译exit0，集成镜像1,923,152字节 SHA256 `8e67153054ef4dd400a71e423949e629803c40119c8f304d8c26391dd75697a8` | 未烧写、未真机、远端CI未运行 |
| 2026-10-11 | 主机/编译 | R1 local-frame：独立构建选择与本地策略（固定SD来源/不睡眠/无HA；URL与远端配置入口拒绝）；首次编译1,927,488字节 SHA256 `2c213214d383141fb71d18a6b2bd3a5be97dfd477ffa10b55a6ee590713d8716`（早于本地策略代码） | 启动门禁未解除，未部署 |
| 2026-10-11 | 主机 | 下载入口/重定向/重复响应头等P3硬化多项（redirect准备、Host端口、userinfo拒绝等） | 属R2路径，URL图源不在R1；真实多跳未验 |

未执行：完整产品入口真机运行、手机配网/上传/轮播、PMU睡眠、OTA、远端CI。

## 候选台账

| 候选 | 日期 | T0主机 | T1 ASan/编译 | 镜像（字节/SHA256） | 烧写回读 | T3真机 | 备注 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| R1 local-frame（工作区，未提交） | 2026-10-19 | CTest 1301/1301，Python 96/96（`scripts/test_quick.sh`，约15秒） | ASan/UBSan 1301/1301（含重建约56秒）；固件编译未跑 | 未重编，不复用旧摘要 | 未执行 | 未执行 | 文档与测试流程重整，源码未改 |
| R1 local-frame（时间同步非阻断） | 2026-10-19 | CTest 1301/1301，Python 97/97（T0） | 固件编译exit0；ASan/UBSan 1301/1301（`--asan`，T1完成） | 1,940,944 / `b759cc803e8f47e84d58346be012d6f05b04fc261a3e5832d9c868a2ef88de43` | 2026-10-20 app-only 写 0x10000，hash verified，全长回读 cmp一致；分区表4096字节前后SHA均 `f078383d1525d206e6f7eb4aea29af811fc36a89e195d222d9256fffe6892d20` | 首次启动：产品入口启动→HAL→SD挂载失败（FatFS 13=无可识别文件系统），按设计停止；未到显示/Web，T3未开始 | 改 `main/main.c`；见 [R1启动路径](R1-RUNTIME.md) |

## 2026-10-20 · R1 local-frame 首次上板（SD挂载失败，已停止）

设备：ESP32-S3 v0.2，Flash 0x5e/0x4018 16MB，分区表与已核验factory表一致。仅 app-only 写 `0x10000`（1,940,944字节，hash verified，全长回读一致），未写 bootloader/分区/NVS/PHY/storage，未格式化SD、未擦NVS。USB复位后被动抓串口75秒，日志 `.cache/r1-boot1-private.log`（私有，不入库）。

启动：bootloader→PSRAM 8MB自检→`R1_LOCAL_FRAME`入口→config/NVS加载（沿用默认，未擦）→Board HAL→SD 1-bit挂载 `vfs_fat_sdmmc: failed to mount card (13)`（FatFS FR_NO_FILESYSTEM，卡被识别但无可挂载FAT；可能是未格式化/exFAT/换卡，**未确认**）。按设计 `Storage unavailable ... No retry, format or fallback`，释放HAL后返回，未清白、未起Wi-Fi/Web。结论：R1产品入口能启动并安全失败；T3验收项1起均未执行。

同日用户称换为FAT32卡并重新上电后，被动抓串口（`.cache/r1-boot2-private.log`）结果相同：SD总线识别后 `f_mount` 返回 13（FR_NO_FILESYSTEM），安全停止。此前2026-10-04的SD诊断曾在同硬件挂载59638MB SDHC卡成功，故更可能是卡的分区方案（如macOS默认GUID分区表而非MBR）或换卡，**待用户在Mac上只读核对**；本轮未格式化SD。另有一次PMU状态读CRC拒绝（1/3，无采样发布），不影响启动。

**根因确认（同日，只读诊断）：** 在挂载失败路径加入只读第0扇区诊断（`components/sdcard/src/sd_sector_diag.c` 纯解析 + `sdcard_sdio.c` 中仅 `CONFIG_READ_PICO_LOCAL_FRAME` 的只读调用，不写卡、不格式化；解析器主机测试8项，Python源码检查确认诊断段无写/格式化/挂载API）。重编1,943,360字节 SHA256 `f6599ba6aec2ebb3323836c5159eea3d32413870d00156656a8d507e9608f66f`，app-only写0x10000 hash verified，全长回读cmp一致，分区表前后SHA不变。启动日志（`.cache/r1-boot3-private.log`）：扇区0为带0x55AA的**MBR**，分区1类型0x07、起始LBA 32768、122,105,856扇区（约58.2GiB），分区引导扇区OEM为 `EXFAT   `。即**这张卡是exFAT，不是FAT32**；固件的FatFS未启用exFAT，故报FR_NO_FILESYSTEM。分区方案正常（MBR），不是GPT问题。诊断日志里的容量行本次未出现，原因未查，不影响结论。该镜像对应的T1：CTest与ASan/UBSan各1309/1309，Python 73+8+17=98全过。需要FAT32卡（macOS磁盘工具选 MS-DOS(FAT)+MBR，或≤32GB卡）；固件不会格式化，也不通过打补丁启用exFAT。

## 2026-10-25 · R1 local-frame 换 FAT32 卡后启动（T3 第1–2项部分）

设备镜像仍为 `f6599ba6…f66f`（只读回读 cmp 与本地一致，未重烧）。卡：Mac 侧只读查看为 32.2GB、MBR、FAT32、已有用户数据，未格式化未写入。USB复位后被动抓日志（`.cache/r1-boot4-private.log`）：SD 1-bit 挂载成功→无RTC→自动清白约1.5秒（软件白参考，**屏幕目视结果未确认**）→Wi-Fi（NVS中旧凭据）3.2秒获取 192.168.1.21→SNTP超时，按R1修复告警后继续→mDNS→`PhotoFrame started successfully`，约16.2秒Web可用。Mac 只读 GET：`/` 200，`/api/config` 返回配置（rotation_mode=storage，auto_rotate=false，orientation landscape/180°），`/api/albums` 返回 Default。证实：时间同步非阻断修复在实机生效；SD、Wi-Fi、HTTP 启动路径通。未验证：上传、显示、轮播、重启保持、失败路径。

## 2026-10-25 · R1 上传/显示 T3（部分）与灰阶标定

用户通过网页上传4张图（存为 `.epdgz`，浏览器端按设备调色板处理）；`/api/display` 逐张显示均返回成功（1.9–2.1秒，一张12.3秒原因未查）；GC16绘制约424ms，面板温度44°C。人工观察：第1张方向/铺满/无条带正常；第2、3张部分过曝（中间调和高光偏亮），切换到第2张时有上一张残影。对 `/api/calibration/display` 色阶图拍照（手机照片，仅近似）：黑/白亮度比约0.078（默认模型约0.014），L*空间归一化曲线拟合gamma≈1.09（默认1.42），实测上中间调高于模型。默认值来自另一块面板（ED103TC2）。经用户同意，写入灰阶标定 `black_y=0.05, white_y=0.65, gamma=1.1`（原值备份 `.cache/palette-before.json`，可 `DELETE /api/settings/palette` 恢复）；已存图片需重新上传才应用新曲线。残影原因未查。

## 2026-10-25 · 读文件约30次后全部失败：根因与修复（待实机复验）

现象：运行中缩略图 `/api/image` 404、`/api/display` 500（`Failed to open EPDGZ file`），目录列表仍正常，重启恢复。可复现：重启后连续读同一缩略图，第28–32次起全部失败；空闲70秒不触发，与请求数相关。

定位（三轮诊断镜像，均经用户同意 app-only 写 0x10000、全长回读一致、分区表 SHA 不变，未写 SD）：`fopen` 失败 errno=24(EMFILE)；文件登记表32槽中31个为 OPEN、无 CHANGING/UNCERTAIN、无 `fclose` 失败；泄漏槽没有 `__wrap_fopen` 打的路径标签。反汇编确认构建用 picolibc，`fopen` 内部调用被包装的 `open()` 与 `fdopen()`：内层 `fdopen` 已把 FILE 登记在描述符的槽里，外层 `__wrap_fopen` 又登记一次，`fclose` 只释放一个，每次打开 SD 文件泄漏一槽。

修复：`components/board_hal/src/read_pico_fs.c` 中 `__wrap_fopen` 若发现该 FILE 已被内层登记，则释放自己的预留槽，不重复登记；保留非 ENOENT 打开失败的状态日志，去掉临时诊断代码。新增主机回归测试 `ReadPicoFs.NestedOpenFdopenStdioDoesNotLeakRegistrySlots`（模拟 picolibc 嵌套，100 次开关），去掉修复时首次关闭即失败、加上修复通过。T1：CTest 与 ASan/UBSan 各 1310/1310，Python 98 全过。修复镜像 1,943,792 字节 SHA256 `c3c980f3c2ced489cf3b586116d4dee909e747678eaad5cea7bab24bf94bbca5`。经用户同意 app-only 写 0x10000，hash verified，全长回读 cmp 一致，分区表前后 SHA 均 `f078383d…6892d20`，未写 SD。实机复验：重启后连续读缩略图 100/100 成功（修复前第28–32次起失败），`current_image` 与另一缩略图 200，串口无 `read_pico_fs` 告警，随后 `/api/display` 城堡图 200（1.9 秒）。长时间运行与上传/删除混合场景未专门测。

## 2026-10-25 · 照片刷新改为“先刷白、再从白 GC16”（仅软件，未上板）

依据厂家 16 灰流程（`flash_white` → `update_display_from_white`）和波形组件说明（裁剪表擦除余量小，残影先出现）。新增便携函数 `components/epaper_driver_read_pico/src/read_pico_white_first.c`：先把前缓冲置白做一次 GC16，再恢复目标、GC16 从白绘制；面板已白（开机清白后首张）时跳过刷白；刷白失败或两次之间总线回调出错即停止，不启动第二次通电事务。驱动仅在 `CONFIG_READ_PICO_LOCAL_FRAME` 下调用它，并为目标帧多分配一帧 PSRAM（415,872 字节）；诊断 profile 行为不变，波形/VCOM 未改。主机测试：新增 5 项（顺序、跳过、刷白失败恢复缓冲、总线异常不再通电、从白失败传播）及 1 项 Python 接入检查。T1：CTest 与 ASan/UBSan 各 1315/1315，Python 99 全过。固件 1,944,096 字节 SHA256 `3b39cf18807b6c2fd45f5cffc0a21ddf35fb62bd5d5f87db4d91672f6bb576e2`，**未烧写**。预期代价：每次换图多一次整屏刷白（约 0.45 秒，估计）。经用户同意 app-only 写 0x10000，hash verified，全长回读一致，分区表 SHA 不变。实机：开机后首张（卡通图）只有一次 GC16（面板已白，跳过刷白）；第二张（城堡）两次 GC16（刷白 464ms + 从白 464ms），接口 2.4 秒，均成功。残影效果待用户目视确认。

## 2026-10-25 · 16 码实测查表与灰码 4 修正（仅软件，未上板）

- 转换库：`@aitjcize/epaper-image-convert` 0.1.20 原样引入 `third_party/epaper-image-convert/`（来源、许可、原样 SHA256 见其 `UPSTREAM.md`）；webapp 通过 Vite alias 使用，切换后在未改库代码时网页构建输出与原 npm 版本逐字节一致。修改：`makeGrayscale16({levelsY})` 按 16 个实测值生成灰阶；`findClosestColorRGB/LAB` 只对 7 项六色调色板跳过下标 4。process-cli 仍用 npm 原版。
- 网页：上传与预览传 `levels_y`；灰阶标定页新增 16 值输入（为空时用 black/white/gamma）。
- 固件：`color_palette` 新增独立 NVS 键 `gray_levels`（16 个 float，原 palette blob 布局不变）及校验、读写、清除、JSON；`/api/settings/palette` GET 返回、POST 接受数组/`null`（未提及则不变）、DELETE 同时清除；`image_processor` 有查表时据此生成 16 级灰。两条 NVS 记录不是原子事务，失败按失败报告。
- 测试：转换库 vitest 6 项（含灰码 4 可选、六色仍跳过 4；去掉修复后对应用例失败）；固件 settings 7 项、设备端处理 2 项（去掉查表后用例失败）；更新 1 项源码接入计数。`npm --prefix webapp test` 66/66，生产构建通过。T1：CTest 与 ASan/UBSan 各 1324/1324，Python 99 全过。
- 固件 1,946,048 字节 SHA256 `bebb7e5dc3c5c72232d5e27fc7de0269deb75a0dc281ceb52bd108dccb445516`（含新网页资源）。经用户同意 app-only 写 0x10000，hash verified，全长回读一致，分区表 SHA 不变，未写 SD。启动后 `POST /api/settings/palette` 写入 16 码 `levels_y`（原值备份 `.cache/palette-before-3.json`），GET 读回一致，缩略图读取正常。

重传验证：第一次重传结果与旧文件逐字节相同（浏览器用了缓存的旧网页脚本；设备提供的已是新脚本，响应无 Cache-Control）。用户改用无痕窗口、确认标定页出现 Measured ramp 后重传，新文件 `421c1431qd0s.epdgz`：灰码 4 已使用（105,410 像素，此前恒为 0）；按实测梯子换算的归一 L* 与原图差：天空 −0.006、建筑 −0.025、暗部 −0.023，均在 ±0.03 目标内（此前 +0.081/−0.014/−0.107）。被动串口监听期间无复位。屏幕观感待用户确认。遗留：网页资源固定文件名且无缓存控制，固件更新后浏览器可能继续用旧脚本。用户反馈新图准确但雾蒙蒙、对比度低（屏幕黑白亮度比约 1:23，压缩动态范围后整体发平）。经用户同意将设备默认处理参数 contrast 1.0→1.2（保留压缩动态范围，原值备份 `.cache/processing-before.json`），待重传观察。

## 2026-10-25 · 网页中英文切换（仅软件，未上板）

- 新增 `webapp/src/i18n/`：英文原文作键、`zh.js` 中文字典（303 条），不新增依赖；语言存浏览器本地，首次按浏览器语言；顶栏按钮切换，Vuetify 自带文案同步切换。已翻译设备端所有页面组件（顶栏、主页、相册、上传、预览、处理参数、轮播计划、设置、灰阶标定、OTA、崩溃记录、配网页）；未翻译 GitHub Pages 演示页 `LandingPage` 与彩色屏专用 `PaletteCalibration`，后端返回的错误文本保持原文。轮播计划说明新增 `describeCardZh`。处理参数面板为曝光、对比度、抖动、颜色匹配、色调映射、S 曲线各项及压缩动态范围加了中英文说明。
- 测试：网页 69/69（新增 i18n 覆盖检查：所有 `t()` 文案必须有中文，删去一条即失败；切换/插值/回退；`describeCardZh` 1 项）；ESLint、Prettier 通过；本地预览确认中英文切换生效。生产构建通过。
- 固件（仅网页资源变化）1,957,600 字节 SHA256 `add49cba7fcbafe9a45d92a8a4b4181669042c16db4debb4e774971b5dcefe28`。经用户同意 app-only 写 0x10000，hash verified，全长回读一致，分区表 SHA 不变，未写 SD。重启后网页可用，设备提供的脚本含中文字典；灰阶查表与对比度 1.2 等设置保留。浏览器端切换效果待用户确认（需强刷或无痕窗口避开旧缓存）。

## 2026-10-25 · 网页资源缓存控制（仅软件，未上板）

问题：网页资源以固定文件名嵌入固件、响应无缓存控制，app-only 更新后浏览器继续用旧 `index.js`（2026-10-25 实测，需无痕窗口才生效）。修复（由并行会话在主工作区完成 C 改动，本会话补测试与文档）：`main/http_server.c` 与 `main/wifi_provisioning.c` 新增检查式 `send_webapp_asset()`，对 `/`、`/provision`、`/assets/*`、`/icon.svg` 设置 `Content-Encoding: gzip` 与 `Cache-Control: no-cache`，设头失败即返回不发送；图片缩略图的 `max-age=3600` 与测量样例不变。新增 Python 源码检查 1 项。T1：CTest 与 ASan/UBSan 各 1324/1324，Python 74+9+17=100 全过（并行会话运行）；其固件编译因未按开发说明设置 `IDF_PYTHON_ENV_PATH` 被 idf.py 拒绝，本会话按说明重编通过。固件 1,957,872 字节 SHA256 `8551f9b037d9fa6585568cfa7c965966fc1224e70142b96ec836092ea4e7daed`。经用户同意 app-only 写 0x10000，hash verified，全长回读一致，分区表 SHA 不变，未写 SD。实机响应头：`/`、`/assets/index.js`、`/assets/index.css`、`/icon.svg` 均为 `Content-Encoding: gzip` + `Cache-Control: no-cache`；缩略图仍为 `public, max-age=3600`。

## 2026-10-28 · 构建目录被清空后重编并重新上板

本地 `build-read-pico-*` 构建目录不见了（原因不明，源码与 `.cache` 均在）。源码未改，T0 通过（CTest 1324/1324，Python 100 项），重新完整编译 R1 local-frame：1,957,872 字节 SHA256 `a2193bafbbfb33303d5d7d7c9e9c7b8a5b097e1f576dbdec42bbee199a28e207`（与 `8551f9b0…` 同源，摘要不同来自完整重编的构建信息）。设备 USB 曾一度未枚举，用户重新插线后经同意 app-only 写 0x10000，hash verified，全长回读一致，分区表 SHA 不变，未写 SD。启动日志：SD 挂载成功，IP 192.168.1.21，网页 200。相册 Default 现有 2 张图，轮播测试需至少 3 张。

**T3 轮播（2026-10-28/29）：** 用户再上传 1 张，共 3 张。经用户同意临时设为 `auto_rotate=true`、`sd_rotation_mode=sequential`、`rotate_cron=["*/1 * *"]`（原设置备份 `.cache/config-before-rotation.json`）。串口记录前 4 次：设置后 42 秒首次轮播，此后每约 60 秒一次，顺序 `25125d68lcbs → a83231aemxt2 → 12907098exeg → 25125d68lcbs`，每次先刷白再从白 GC16（各约 464ms），全部 `Image displayed successfully`、无错误或复位。之后串口输出中断（被动读取收不到数据，原因未查，网页与轮播不受影响），改用 `/api/current_image` 缩略图摘要每 30 秒核对，顺序继续按 12907 → 25125 → a8323 推进，合计超过 2 轮。关闭 `auto_rotate` 后连续 3 分钟（7 次采样）当前图不变，停止生效。随后恢复原设置（随机、`0 */12 *`、自动轮播关）并读回确认。用户目视确认屏上换图正常：顺序正确、无残影、无异常条带。

**T3 重启后轮播（2026-10-29）：** 再次临时开启顺序、每分钟轮播，确认换图后经 USB 串口复位设备（`USB_UART_CHIP_RESET`，非断电）。重启后 SD 挂载成功、网页可用，`/api/current_image` 显示轮播自动恢复：重启后约 3 分钟内按 a83231 → 12907 → 25125 顺序换图 2 次，无需手动操作，说明轮播开关、顺序与 cron 规则均从 NVS 保留。测完恢复原设置（随机、`0 */12 *`、自动轮播关）并读回确认。断电冷启动尝试：用户拔掉 USB 约 10 秒再插回，轮播照常按顺序换图，但设备运行时间约 428 秒，与此前串口复位时刻吻合，说明拔线期间设备靠自带电池继续运行、并未重启，故不计为断电测试。按用户决定跳过，断电冷启动后的轮播仍未验证；测完恢复原设置（随机、`0 */12 *`、自动轮播关）并读回确认。

## 2026-10-29 · T3 上传失败路径（实机）

镜像 `a2193baf…e207`。经 `POST /api/upload?album=Default` 逐项发送，前后比对相册列表与原片 `25125d68lcbs.epdgz` 摘要：

| 用例 | 结果 |
|---|---|
| 随机字节伪装 `.png` + 正常缩略图 | 400 `Upload file signature mismatch` |
| `.gif` 扩展名 | 500 `Upload incomplete or storage failed; temporary files retained`（拒绝正确，但状态码与提示把格式错误说成存储故障） |
| 缺缩略图 | 400 `Upload incomplete - expected image and thumbnail` |
| 与已有图同名重传 | 409 `Filename already exists; original retained` |
| 11MB 超限 | 400 `Invalid or oversized multipart upload`（上限 5MB/文件、约 10MB/请求） |

相册仍为 3 张，原片逐字节不变，网页可用：不发布半成品、不覆盖原片成立。发现：`storage_used` 增加 229,376 字节，失败上传留下的隐藏临时文件（`.rc3-upload-*.part`）没有清理，多次失败会持续占用 SD 空间；`.gif` 的错误码/提示不准确。

**修复（同日，仅软件，未上板）：** 便携上传核心 `upload_file` 新增 `created` 标记与 `upload_file_discard()`：仅当本对象独占创建了该临时文件、已干净关闭、从未发布时才删除；关闭失败、创建失败（路径可能属于他人）、已发布或适配层不提供删除器时一律保留。设备适配 `read_pico_upload.c` 的删除器只接受 `/storage` 下 `/.rc3-upload-*.part` 名称。`http_server.c` 在多段解析失败、缺缩略图、签名不符、同名 409、存储不可用、发布失败（已发布的缩略图保留）、直接显示的无图/未知格式和原始上传失败处调用它。扩展名不符改为 415 `Unsupported file type: image must be .png or .epdgz, thumbnail .jpg`。新增主机测试 3 项（删除一次、先关后删、五种不可信情形均不删），更新 1 项源码检查。T1：CTest 与 ASan/UBSan 各 1327/1327，Python 100 全过。固件 1,958,368 字节 SHA256 `8464a43d2717887541a1c58e0dfdac889b68e0f1b07ef5a4cde40a98e126be12`，**未烧写**。此前失败上传已留下的约 224KB 临时文件不会被这次修复清掉。

**上板复测（`8464a43d…be12`，经用户同意 app-only 写 0x10000，回读一致、分区表不变）：** 坏图 400、`.gif` 415、缺缩略图 400、同名 409、超限 400，坏图再重复 3 次；`storage_used` 前后均为 56,754,176，增量 0；相册仍 3 张，原片逐字节不变。发现 415 响应只收到状态行、正文丢失：在第一段头部就拒绝时服务端未读完请求体即关闭连接。随即补上“拒绝后在既有截止时间内读完剩余请求体再回复”，T1 各 1327/1327、Python 100 通过，新固件 1,958,448 字节 SHA256 `18a6ddb48d0298bef38e92ece4ec7170d62f624d97f49377f974e9282b654db0`；经用户同意 app-only 写 0x10000，回读一致、分区表不变。复查：`.gif` 返回 415 且正文完整（`Unsupported file type: image must be .png or .epdgz, thumbnail .jpg`），坏图 400，存储增量 0，相册 3 张。

## 2026-10-29 · T3 无卡启动（实机）

镜像 `18a6ddb4…4db0`。先用 esptool 让芯片停在 ROM 下载模式（应用不运行、SD 未挂载）再由用户拔卡，随后串口复位进入应用：日志 `Storage unavailable (ESP_ERR_NOT_FOUND); insert/check the SD card and restart. No retry, format or fallback. HAL release: ESP_OK`，`app_main` 返回，无 panic、无反复重启、未格式化，网页不启动。插回卡后复位：SD 挂载成功，IP 192.168.1.21，网页 200，相册 3 张完整。不足：无卡时屏幕没有任何提示（显示未初始化），用户只能看到“没反应”。

**热点配网：未验证（用户决定跳过）。** 设置接口改 Wi-Fi 时会先试连、失败即拒绝保存并回到原凭据，因此无法用错误密码触发配网热点；可行方式只有设备无凭据（恢复出厂会清掉全部设置）或路由器拒绝密码（临时改路由密码）。

## 2026-10-29 · 最终候选 `18a6ddb4…4db0` 快速冒烟（实机）

同一镜像上完成 R1 闭环：
- 上传：经 `/api/upload` 新增 `r1-smoke-test.epdgz`（内容与卡通图相同）→ 200，相册 4 张；`/api/display` 显示 200（2.0 秒）。
- 轮播：顺序、每分钟一次，01:08:30 开启。按 `/api/current_image` 每 15 秒采样，01:09–01:12 完整一轮（12907 → 测试图 → 25125 → a8323；测试图与卡通图缩略图相同，采样上合为一次变化），01:12:51 串口复位，重启期间错过 01:13 一次，01:14 起从 12907 继续，01:14–01:17 第二轮完整，01:18 再进入第三轮。
- 停止：01:19:07 关闭后 2 分钟（5 次采样）当前图不变。
- 测完恢复原设置（随机、`0 */12 *`、自动轮播关）并读回确认。测试图留在相册，是否删除由用户决定。
连同此前在本镜像上的失败路径、无卡启动结果，R1 冒烟闭环在同一候选上完成；热点配网与断电冷启动仍未验证。屏幕观感本轮未单独请用户确认。

## 2026-10-29 · R2 长稳：Wi-Fi 断线后不再永久离线（已上板，恢复未实测）

- 问题（源码核对）：正常开机连上网后，断线只由事件处理快速重试 5 次（`WIFI_MAX_RETRY`），之后置 `WIFI_FAIL_BIT` 并不再重连；只有开机超时路径会调用 `wifi_manager_keep_reconnecting()`。路由器重启超过几秒即会让设备离线到重启为止，与“运行约 3 天后网络不可达”的现象相符（未确认那次的实际原因）。
- 修复：新增便携策略 `wifi_watch`（30 秒起、加倍、上限 10 分钟），`wifi_manager_supervise()` 在重试已耗尽且无进行中的连接时调用 `keep_reconnecting`；由 local-frame 轮播任务每秒调用。不重启、不开热点、不改凭据；配网/已停止的站点不受影响。断线日志带 reason。
- 主机：`wifi_watch_test` 6 项、Wi-Fi 管理器 2 项新测试；T0+ASan 各 1336/1336。另发现既有问题：`read_pico_wifi_manager_test` 在单进程连续运行全部用例时段错误（HEAD 上同样复现；CTest 逐用例独立进程运行，不受影响），未修。
- 编译与部署：local-frame 镜像 `ee30ef64de5f72a95621d946fa4b0141360061ef7829df6d27d9332a5eb21136`（1,958,864 字节），2026-10-29 app-only 写入，全长回读一致，分区表不变，启动正常并联网。
- 实机断网恢复：**未验证**。第一次关路由只观测到 3 秒中断；改用手机热点时设备未切换成功（仍连家庭 Wi-Fi），用户决定先跳过。

## 2026-10-29 · 上传与轮播重叠（实机，发现问题）

镜像 `18a6ddb4…4db0`。开启顺序轮播，计划为 `*/1 * *`（每分钟一次），01:34:57 起连续上传 20 组（内容同 `r1-smoke-test`，文件名 `ovlp-01..20`，间隔 0–3 秒，单次约 0.5–0.8 秒），同时被动串口监听。
- 19/20 成功；`ovlp-02` 在 01:35:01 返回 **500 “Album storage unavailable”**，此时正好是轮播刷屏（日志里 `scan clock lock` 之后紧跟 500）。失败发生在相册目录 `stat`，尚未建临时文件。
- 根因（源码）：刷屏全程（刷白 + GC16，约 2 秒）持有板级锁；`read_pico_sd_io_begin` 只等这把锁 100ms，超时即 `EBUSY`，上传处理把它当成存储不可用。刷屏期间其他 SD 访问（缩略图、列表）同理会失败。超时不会使介质失效，下一次请求即恢复。
- 轮播本身不受影响：01:35、01:36、01:37 三次换图均成功，并按顺序显示了新上传的图。19 张成功上传的抽查（01/03/04/10/19/20）原图与缩略图摘要与源文件一致，原有 `25125d68lcbs.epdgz` 摘要不变。无重启、无 panic。
- 测后恢复原设置（随机、`0 */12 *`、自动轮播关）并读回确认；19 张测试图已经 `/api/delete` 删除（各 200，缩略图 404），相册回到 4 张。
- 修复（仅软件，未上板）：`read_pico_sd_io_begin` 等板级锁改为 `READ_PICO_SD_IO_LOCK_WAIT_MS`（5000ms），mount/unmount/status 仍为 100ms；新增主机测试 `IoBeginWaitsLongerThanOnePhotoRefresh`。T0+ASan 各 1328/1328 通过；local-frame 编译 exit 0，镜像 `d4b92f61346d994951059f120b66de938d7e6534faa8a10a4109c6eadc75f5fe`（1,958,448 字节）。未覆盖：反方向竞争，即刷屏开始时以 0ms 取锁，恰逢 SD 检测持锁几毫秒会导致本次换图失败（此前也存在，本次未改）。
- 上板复测 `d4b92f61`（2026-10-29 01:47，app-only，全长回读一致，分区表不变）：在 3 次换图前后各 12 秒内连续上传 42 次，40 次成功（含等待刷屏、耗时约 2.1 秒的请求），2 次 500：`ovlq-03` “Upload incomplete or storage failed”，`ovlq-18` “Publication failed; no overwrite or retry”，但图与缩略图实际已发布。根因同源：上传前后的就绪检查走 `read_pico_sd_get_status`，它仍只等锁 100ms。已改为同样等待 `READ_PICO_SD_IO_LOCK_WAIT_MS`，测试补充 get_status 断言；T0+ASan 各 1328/1328，local-frame 编译 exit 0，镜像 `13ebf1991db220806e710ceca5147eb0f971ff601351632e0cbc6e5384272afc`（1,958,448 字节），未上板。设备显示版本仍为 `dev-53e441e`（构建缓存的版本信息）。测试图 41 张已删，设置已恢复。
- 上板复测 `13ebf199`（2026-10-29 01:53，app-only，全长回读一致，分区表不变，启动正常）：在 4 次换图前后各 12 秒内连续上传 40 次，**40/40 成功**；换图窗口内有 9 次请求等待刷屏，耗时 1.5–5.7 秒（单次请求会多次等锁，总耗时可能超过 5 秒）。抽查 5 张与源文件摘要一致，原片不变。串口在首次换图后停止输出，后 3 次换图由上传耗时间接佐证，未逐条日志确认。测后恢复设置，40 张测试图已删，相册回到 4 张。

## 2026-10-19 · 文档与测试流程重整

仅整理文档与测试入口，未改固件源码。

- 新增 [测试策略](TESTING.md)（T0–T3分层、按改动触发、R1真机验收单）与 `scripts/test_quick.sh`、`scripts/check_doc_links.py`。
- TODO收窄为R1；R2/P0–P5/G1–G3迁入 [路线图](ROADMAP.md)；本文件由2686行精简为汇总，全文、旧TODO、旧CHANGELOG存入 `history/2026-10-11/`。
- T0实测：`scripts/test_quick.sh` CTest 1301/1301（10.1秒）、Python 73+6+17=96（约2.5+0.5+0.01秒），整体约15秒。
- 文档链接检查通过；归档副本只改相对链接，内容未变。
