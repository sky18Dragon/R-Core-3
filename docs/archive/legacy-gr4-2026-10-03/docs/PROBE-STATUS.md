# 启动探针 RAM 结果查询

设备最新已切换为私人三图显示诊断（1594416 字节，见 DISPLAY-SAMPLES.md），不是下面的启动探针。**不得用 0x3fc92a5c 读取当前设备并解码探针结果**；以下地址和三轮 PASS 仅属此前匹配探针 bin/ELF 的历史验证。

2026-10-01：CTest 17/17、ESP-IDF v6.1 编译/门禁通过；新 app 166656 字节，SHA256 `3bb56158734eb4f7e0dbcef1ba7621ab42e09563269de81827ad41df8ad6b58e`。用户授权后已烧写、Hash verified，256 KiB 回读三个镜像逐字节相符，NVS/phy 区域未变。人工开机后首次 RAM 查询明确 PASS（三模式、test_done/probe_done=1、无失败元数据），双核 Power on reset，查询后 running；重复启动仍待验证。详见 HARDWARE-RESULTS.md。

## 布局与判定

最新进度：第三轮人工电源操作确认后仍 PASS，累计 3/3；`jtag-status-power-03.log` 中双核 Power on reset、查询后 running，专用三模式完成、探针结束、无失败元数据。探针阶段不再追加同类查询，转入备用卡/SD/显示准备；实际更换固件须单独确认。以下第二轮说明保留历史。

进度补充：第二轮人工关机/开机并重连确认后查询仍 PASS，双核 Power on reset、查询后 running；日志 `jtag-status-power-02.log` 保留私有缓存。累计两轮，第三轮待人工操作。状态没有启动唯一编号，重复启动记录依赖人工电源操作确认，不能仅凭同样的 RAM 值推算启动次数。

`frame_boot_probe_status` 是显式 DRAM_ATTR、volatile 的 40 字节全局状态。当前 ELF 符号 `3fc92a5c 00000028 D`，位于内部 RAM，不依赖被测 PSRAM。每次进入探针初始化为 NOT_RUN；测试/缓冲释放后发布结果，test_done 最后置 1；结束日志输出后 probe_done 置 1。保留到下次复位，不写 Flash/NVS/SD，不跨复位沿用。

uint32_t 字段顺序：magic（0x42505231）、version（1）、app_entered、test_done、result、patterns_checked、failure_offset、expected_byte、actual_byte、probe_done。result=0 尚未完成、1 PASS、2 FAIL、3 ALLOC_FAIL；没有失败时 offset=0xffffffff、expected/actual=0。FAIL 记录第一处错误偏移与字节值，不保存照片或身份。

volatile 不是多字段事务或跨任务同步接口，只在双核暂停时读两份相同快照。PASS 必须 test_done=1、result=1、完整通过 3 模式且无错误元数据；probe_done 不替代 test_done。未完成 publication 无论 result 字段是什么都只报 PENDING；错布局/格式/矛盾状态拒绝。此状态不是认证机制。

## 操作顺序

1. 单独确认烧写，备份当前 Flash、验证原厂回滚，按已检查生成 flash_args 写入/回读。不擦除 NVS、格式化 SD 或写 VCOM。
2. 人工断开 USB、完整关机再开机并重连；不在重枚举期间做 reset/断点实验，不要求抢日志窗口。
3. 核对已烧写 bin 和配套 ELF，用 `nm -S` 重新查询状态地址。地址随构建变化，以下仅适用于上面的 SHA256 产物。
4. 核对工具无占用，使用匹配的 IDF/OpenOCD：

```sh
# 日志路径必须是新的私有文件，不覆盖旧证据。
openocd -l <新的私有日志文件> \
  -c 'set ESP_FLASH_SIZE 0; set ESP_RTOS none; gdb port disabled; tcl port disabled; telnet port disabled' \
  -f board/esp32s3-builtin.cfg \
  -c 'adapter speed 1000; init' \
  -c 'set FRAME_PROBE_STATUS_ADDRESS 0x3fc92a5c' \
  -f tools/read_probe_status.cfg
python tools/decode_probe_status.py --log <同一日志文件>
```

脚本不请求 reset、Flash probe、load、断点或目标内存写入。初始双核非 running 时拒绝改变状态；短暂停核后读两份状态，错误仍尝试恢复。解码器要求双核 POST_STATE running，读取/恢复错误、缺标记、多个快照或矛盾状态均拒绝。OpenOCD 退出码 0 本身不证明成功。JTAG/libusb 异常后停止，核对恢复，不自动反复 reset/烧写。

## 测试与边界

实际探针 C 源测试覆盖正常、分配失败、三模式分别损坏、状态重置/发布顺序、失败元数据和重播；ASan/UBSan 通过。解码器 9 项测试含真实 Tcl 脚本的 6 种 fake target 场景：成功、读取异常、不稳定快照、单核未暂停、恢复失败、初始非 running。全量 CTest 17/17；主机还需 Tcl/tclsh，本机 /usr/bin/tclsh，缺依赖会失败，不静默跳过。

新版已完成烧写/字节回读和首次人工开机 RAM PASS 查询。JTAG attach/halt/resume 影响时序和调试状态，不是零干扰冷启动验证。查询 PASS 也只证明本次 64 KiB 三模式 scratch 测试，不证明 8 MiB 全域/压力/温度、PMU ready、SD、显示或相机协议；重复启动须逐次人工操作与记录，不将同一次 RAM 重读计为新启动。
