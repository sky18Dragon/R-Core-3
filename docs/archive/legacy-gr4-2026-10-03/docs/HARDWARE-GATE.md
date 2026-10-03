# 第一轮上板验证清单

更新时间：2026-10-01。历史存储诊断和第二轮启动探针未通过启动门禁，回滚原厂获用户确认正常。第五轮 RAM 查询探针烧写/回读通过，三轮人工电源操作确认后专用 64 KiB 三模式 3/3 PASS，双核 Power on reset/查询后 running；不是独立电源测量或全域压力验收。下一阶段准备备用卡及 A/B 测试，更换固件须单独确认。详见 [安全启动排查](BOOT-PROBE.md) 和 [上板记录](HARDWARE-RESULTS.md)。SD 三次断电恢复、显示或 GR IV 自动同步尚未验收。

## 准备

需要 Read Pico（对应本项目锁定的官方 BSP）、可传数据的 USB 线，以及可丢弃数据的测试 microSD。先保留设备原固件和卡中资料。不要使用唯一照片备份卡。PMU/VCOM 必须是有效出厂校准；本项目只读取，不改写、不格式化卡、不自动擦除 NVS。

连接后确认真实开发板串口，不选择 Bluetooth、耳机或 debug-console 占位串口。`/dev/cu.usbmodem2101` 曾由用户确认是 Pico；此前替换测试已结束并回滚，不能把历史授权当成下一轮自动烧写许可。原 Flash 保存在私有缓存。

## A：默认 JPEG Demo 0

把自有 baseline JPEG（≤8 MiB、≤4 Mi 像素）放到卡的 `/frame/test.jpg`。默认构建不执行存储诊断，不改写 SD 照片。

```sh
. .cache/esp-idf-v6.1/export.sh
idf.py --version
idf.py build
```

确认 v6.1、板型、分区布局与 `build/flash_args` 后，才使用实际端口手动烧写：

```sh
idf.py -p <实际开发板串口> flash monitor
```

验收：BSP/PMU/SD 成功，无 VCOM 写入/格式化；照片完整构图、方向/镜像正确；真实画面更新而非仅保留旧画面；记录 `Demo 0 refresh result`、渲染耗时、内部 RAM/PSRAM 空闲量、温度和残影。未目视验证不能将编译成功写为显示通过。

## B：显式存储诊断

最新验收进度：严格已有库三轮人工关机/开机后恢复 3/3 PASS，每轮均 `source=existing-only`，无建库/补写；完成固定 fixture 保留验证，不再追加同类电源操作。电源操作依据人工确认，无独立掉电测量，未完成写中断/拔卡/满卡/压力验收；下一阶段准备三张照片实屏测试。下面“待采集”段落为此前历史。

设备最新已切换严格已有库恢复版（345968 字节），测试 19/19、编译/专用门禁和烧写回读通过，待人工开机采集。不会创建目录/库或执行检查点，下面自动目录初始化模式仅为历史/独立流程。恢复记录见 [严格恢复](STORAGE-RECOVERY.md)。

最新真机进度：首次采集明确 `SD diagnostic PASS; global sequence=2`，SD mounted、PMU ready、应用返回；证明事务/checkpoint 与同进程重开校验，不证明跨断电。当前摘要未区分 open-existing/create，继续三次电源验证前须补来源标记或严格恢复模式，不能把丢失后重建的 sequence=2 误记为恢复。首次日志缺启动头/ROM 原因，不能据此认定无干扰冷启动。设备仍 storage 固件，暂无实屏验证。

最新：自动目录版已按用户授权直接烧写，app 347952 字节、SHA256 `ef7f08cce70f5f99962e7de1c83c6ae775c0665ee56106170c80bf71477dcbbb`，CTest 18/18、v6.1 编译/门禁、写入和 448 KiB 字节回读通过。设备留下载器，下一步准备采集并人工关机/开机；实际目录和事务尚未验证。下面 347104 字节记录是修改前准备阶段历史。

2026-10-01 准备进度：用户确认备用卡无需保留数据；v6.1 storage 构建/门禁与 CTest 17/17 重新通过，app 347104 字节、SHA256 `189e5540b4233870e523118cd8a4f8b5b16cc3a212152042e9b5bf927cb35ff1`。本轮未访问设备或烧写，仍是启动探针。先准备以下目录，再单独确认烧写；此模式不显示照片，PASS 需日志/实际介质证据，不能沿用启动探针 RAM 地址读存储结果。

新版显式存储诊断会在已挂载的备用卡上创建缺失的 `/frame`、`/frame/storage-test`，不需要电脑或读卡器。已有目录必须是目录，测试子目录只能为空或包含四个普通诊断文件；未知条目停止，已有库由核心核验，损坏/错库不清理重建。此模式只写该固定目录的 library.id、两个快照和 journal，不是正式照片库，身份是固定诊断 fixture。

用户已确认备用卡无需保留数据，并授权本次直接烧写，无需再次确认。不要带电拔插，也不要在后续断电恢复测试之间清空诊断文件。本轮不要求格式化；若卡无法挂载则保留失败证据并停止。目录创建失败可能留下空的父/子目录，保留用于下次检查，不递归清理；不保证 FAT 目录更新的掉电原子性。

使用独立构建目录和独立 sdkconfig，避免更改默认 Demo 配置：

```sh
idf.py -B build-storage-test -D SDKCONFIG="$PWD/.cache/sdkconfig-storage-test" -D FRAME_STORAGE_DIAGNOSTIC=ON build
```

核对 `build-storage-test/config/sdkconfig.h` 中 `CONFIG_FRAME_STORAGE_SELFTEST` 为 1，再按该构建的 flash_args 烧写：

```sh
idf.py -B build-storage-test -D SDKCONFIG="$PWD/.cache/sdkconfig-storage-test" -D FRAME_STORAGE_DIAGNOSTIC=ON -p <实际开发板串口> flash monitor
```

注意：烧写替换设备应用。不要手写 app 偏移或直接把 app bin 写到 0。若切换回 JPEG Demo，使用默认 build 的配置与产物。

首次预期日志：`SD diagnostic PASS; global sequence=2`，表示身份登记 + KEEP、双快照回读/日志回收、同一进程重新打开恢复通过。它不等于断电可靠性通过。

随后断电重启至少 3 次，预期仍 PASS、sequence=2、测试状态保持 KEPT，journal 保持空，两个快照保持一致。记录任何 VFS fsync/ftruncate、挂载、PSRAM、看门狗或重启异常。失败时保留整份测试目录和串口日志，不删文件再把“重新建立成功”记为恢复通过。

跨断电验证改用独立 [严格已有库恢复模式](STORAGE-RECOVERY.md)，不再用可初始化/补写的普通诊断摘要充当恢复证据。恢复版日志必须 `SD recovery PASS; source=existing-only`，不创建缺失库、不做检查点；普通模式 PASS 只作为初始化/同进程验证。两者产物门禁模式分别为 storage 与 storage-recovery。

## C：人工介质故障验证（仅备用卡）

先完成 A/B，再执行拔卡、满卡和不同写入阶段断电测试。当前自动程序没有故障阶段停点，不承诺每次断电恰好落在提交边界；精确阶段注入需结合实际设备日志/控制器进一步开发。

验证拔卡后出现 IO 失败且停止写入，不自动格式化或 remount；重新插卡后按 BSP 要求显式重新挂载/重启，不能继续使用失效会话。损坏尾部应锁写，两个坏快照/错库应拒绝打开。任何故障导致库打不开都保留证据，不自动修复。

Read Pico BSP 会在检测到拔卡后锁存挂载失效，重新插卡须显式 remount。本项目不调用 format/remount。介质接口仍要求调用方独占目录与单任务串行访问；目前没有产品存储服务任务，也没有源图/缓存事务、Flash 备份和跨介质补偿。

## 记录模板

| 项目 | 实际结果（待填） |
| --- | --- |
| 板型/固件构建/真实串口 | 待连接 |
| 默认 JPEG 刷新、方向、残影 | 待目视 |
| 存储首次创建/同步/回读 | 待上板 |
| 三次断电重启 sequence 与 KEPT | 待上板 |
| 峰值内存/看门狗/温度 | 待测 |
| 拔卡/满卡/断电阶段及保留证据 | 待测 |

记录后更新 TODO.md，区分编译、同进程重开、真实重启和故障注入结果。A/B 未通过前，不启用相机 BLE 写值或 ACK；相机 Gate A–F 仍未通过。
