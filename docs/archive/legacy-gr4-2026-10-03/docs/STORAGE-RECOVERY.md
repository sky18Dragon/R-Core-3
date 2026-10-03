# 严格已有测试库恢复

设备已切换私人三图显示诊断，原 SD 测试库保留；以下严格恢复固件／3/3 结果为已完成的上一阶段记录。当前上板状态见 DISPLAY-SAMPLES.md，不将显示模式当作再次执行严格恢复。

当前状态（2026-10-01）：v6.1 build/专用门禁、CTest 19/19 通过，app 345968 字节、SHA256 `b748505163b84a07c4fc2e7529d2499f39a53e1758bee77b8f52787286b660d9`；已按用户授权烧写/回读相符，三轮人工电源操作后均明确 `SD recovery PASS; source=existing-only`（3/3）。原测试库读取检查持续通过，无重建/补写；结束同类重复操作，非独立电气测量、压力或写中断验收。

本模式只验证之前自动目录诊断已提交的固定 fixture，不是正式照片库或相机同步功能。用户的备用卡已完成一次 `SD diagnostic PASS; global sequence=2`，同进程重开通过；本模式防止把丢失后重建误记为恢复成功。

## 判断与边界

启用 `CONFIG_FRAME_STORAGE_SELFTEST` 和 `CONFIG_FRAME_STORAGE_RECOVERY_ONLY`。既有 `/frame/storage-test` 必须存在，仅接受四个普通测试文件。不会 mkdir、create/import/action/checkpoint、补写 KEEP、清空日志或修复。任一检查失败停止，保留卡上证据，不切回初始化模式。

要求 library.id 与原固定 fixture 匹配，snapshot.0/1 均存在且长度为单条状态快照、内容完全一致并通过核心版本/CRC/身份校验，journal 存在且为零字节；恢复后照片数为 1，原测试照片状态 KEPT、delete_pending=false、全局提交序号 2、照片状态序号 1。注意全局序号与照片状态序号不同：IMPORT 对应照片序号 0，KEEP 对应 1，两个全局提交使总序号为 2。

此要求比生产恢复策略严格，坏单槽也拒绝，不利用好槽回退掩盖测试异常。只用于完成初始检查点的固定 fixture，不适用于一般有未回收日志的照片库。目录/文件 IO 由独占会话访问，读取过程中卡失效拒绝。应用层不发测试文件写入；SD 挂载仍用既有 BSP，不是硬件写保护，不宣称整个控制器零写入。VCOM 只读校准、无格式化/NVS erase/相机操作。

## 构建与验证

```sh
. .cache/esp-idf-v6.1/export.sh
idf.py -B build-storage-recovery \
  -D SDKCONFIG="$PWD/.cache/sdkconfig-storage-recovery" \
  -D FRAME_STORAGE_DIAGNOSTIC=ON -D FRAME_STORAGE_RECOVERY=ON build
python tools/verify_firmware.py --build-dir build-storage-recovery --mode storage-recovery
```

产物门禁拒绝将普通初始化模式冒充 recovery，要求配置一致且链接严格恢复函数。烧写前核对生成 flash_args、完整备份/原厂回滚，写后回读；不改上游驱动、波形或时序。不要在恢复测试之间清空、换卡或重烧初始化模式。

期望开机日志：`SD recovery PASS; source=existing-only; expected sequence=2; no create/checkpoint/write`。FAIL 或日志缺失不作为 PASS；不从屏幕旧画面推断运行。先准备采集，再人工关机/开机，至少三轮逐次记录。未捕获复位原因时注明，USB 重枚举错误不等同芯片故障；重复启动操作依赖用户确认，日志不独立测量掉电电气过程。

主机 CTest 19/19（ASan/UBSan）：缺库不创建、未完成 fixture、错库/错照片、卡不可用、容量不足、逐个 size/read 回调失败、坏单槽/双槽、非空或缺失日志、多照片/序号变化拒绝；所有恢复测试写回调为硬失败哨兵，成功/拒绝后已有文件 CRC 保持。目录检查覆盖不创建及未知条目拒绝。上述不是 microSD 断电、写中断注入或产品功能验收。
