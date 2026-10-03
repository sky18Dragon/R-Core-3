# 组件与目录约定

以下为锁定 PhotoFrame 目录及新增适配位置；P1 HAL/便携 PMU、P2a 核心及 P2b 原生后端/业务错误传播已实现、主机测试和编译通过。实际屏电源与完整相框仍未启用，边界见 [诊断说明](READ-PICO-BRINGUP.md) 与 [显示适配](READ-PICO-DISPLAY.md)。

| 路径/组件 | 任务 |
| --- | --- |
| `boards/` | Read Pico 板型登记、sdkconfig 默认配置 |
| `components/board_hal/` | Read Pico 初始化、电池、PMU 与外部睡眠适配 |
| `components/read_pico_display/` | 已实现便携 nibble/stride/方向转换及 GC16 刷新事务，不含设备 IO |
| `components/epaper_driver_read_pico/` | IDF 外层 EPDiy/LCD/SY 电源 transport、检查式 API 与受保护启动；私有 SY 适配在 src |
| `components/read_pico_timing/` | 原位编译固定厂家扫描时序，不引入总 BSP |
| `main/display_manager.c`、`utils.c` | 新板检查式刷新/轮播、成功后提交状态；旧板接口保持 |
| 固定版本硬件组件 | EPDiy/时序/波形、CW32 PMU、CST836U、FCA9555、SY7636A |
| `components/sdcard/` | 原有 SDMMC backend 的总线宽度和引脚参数化 |
| 输入适配与 `main/` | 触摸事件转轮播请求、必要的 power/image 适配 |
| `webapp/` | 沿用上游管理界面，按移植需求做兼容验证 |
| 后续相机组件 | 独立 GR 协议/同步适配，名称在第二阶段确定 |

新增便携算法与设备 IO 分离；驱动不直接操作相册。沿用上游所有权/任务边界，给新的缓冲、队列和资源明确上限与释放路径。显示请求串行化，回调不执行长时间解码。

最小化修改公共层；新板特例封装在 HAL/backend，公共接口增加能力时回归已有板型。引用第三方源码时记录原路径、commit、许可证及任何差异；不复制阅读器应用或旧相框状态系统来替代上游业务。

研究报告里的代码是设计草案，不能当作已经存在或经过编译的 API。
