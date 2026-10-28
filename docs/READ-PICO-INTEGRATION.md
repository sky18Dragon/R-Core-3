# Read Pico 完整 PhotoFrame 集成编译与运行门禁

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-INTEGRATION.md)。

## 构建入口

激活 [固定SDK](DEVELOPMENT.md) 后：

```sh
python3 build.py --board read_pico --step firmware \
  --read-pico-factory-diagnostic --read-pico-photoframe-integration
```

独立输出 `build-read-pico-integration/` / `.cache/sdkconfig.read_pico-integration`。必须指定Read Pico和factory配置，与display/storage互斥；禁止flash/monitor/erase/fullclean及额外参数。普通诊断默认关闭集成选项，仍不挂载SD、不进入业务。

此配置保留PhotoFrame的真实 `main.c`、Wi-Fi/Web、上传/相册、图片处理、轮播及OTA源码。构建将原 `app_main` 仅重命名为 `photoframe_app_main`，并通过有副作用的volatile函数指针保留链接；专用 `read_pico_integration_entry.c` 只报告INTEGRATION_ONLY，不调用产品入口、不初始化板卡、NVS、存储、显示或网络。主机测试实际调用该入口，并确认产品调用次数为零。

使用与已验收显示诊断相同的精确684行外层覆盖和PERF/cache配置；厂家固定子模块不改。不调VCOM、gamma、PCLK或Flash/PSRAM频率。此时共享嵌入Web/splash资源只作为编译输入，不把已有资源尺寸或链接成功计为新板开机画面验收。factory布局不提供完整产品OTA条件；不得照用编译器打印的通用全量烧写命令。

## 现行业务适配

R1候选另用`python3 build.py --board read_pico --step firmware --read-pico-local-frame`，输出`build-read-pico-local-frame/`和`.cache/sdkconfig.read_pico-local-frame`。依次加载Read Pico、既有factory布局、全栈integration及local-frame defaults，禁止混用诊断选项及flash/monitor/erase/fullclean。R1分支调用真实产品入口，并在后台/Web启动前初始化显示、自动清白建立软件参考；不冒充人工白屏确认。普通integration分支仍只链接。R1须单独批准部署并完成实机验收，编译不等于可交付。

- NVS启动只读严格加载，错误在板级/SD前停止；排除自动擦除恢复和Web整分区reset，不自动改旧cron/方向覆盖。
- 存储只走唯一`read_pico_sd`，无自动格式化/fallback；IO占用、确切句柄和关闭失败锁存保护卸载。真实全部消费者/供电退出继续验收。
- 显示元数据容量512字节含NUL，超过511字符在刷新前拒绝；历史截断记录不自动修复。
- JPEG/PNG、HTTP上传/URL、Wi-Fi/AP、HTTP资源、后台/周期/SNTP与配置错误保护已进入真实编译链接；测试模型和源码接入范围见VALIDATION。
- PMU睡醒未验收时睡眠入口直接返回；OTA暂不执行检查/升级/确认回滚。原板保留原策略。

## 产品开放条件

完整集成编译成功不调用`photoframe_app_main`，不证明产品启动或真实Web/NVS/SD/网络/显示闭环。R1采用独立的本地相框运行配置：开放前核对实际启用的Web上传、SD相册、图像显示与轮播路径，以及旧配置/方向、镜像分区适配和数据保留。未验收的远端URL/远程配置、OTA、触摸及睡眠路径须隔离；不能直接删除集成入口门禁后启用全部服务。全局优化与完整OTA迁移开发不是R1前置条件，具体执行清单见TODO。

显示标准页、测试卡IO和S1照片的有限真机通过各自保留，不能替代手机上传、相册选图/轮播与重启恢复。具体硬件轮次以本次核验的镜像/输入/范围实施；不复用历史诊断许可为完整产品启动。

接口和限制见 [存储](READ-PICO-STORAGE.md)、[照片](READ-PICO-PHOTO.md)、[配置](READ-PICO-CONFIG-DOCUMENT.md)、[迁移](READ-PICO-MIGRATION.md)、[部署](DEPLOYMENT.md)。
