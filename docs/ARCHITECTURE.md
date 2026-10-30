# 当前技术架构

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/ARCHITECTURE.md)。

## 分层与数据路径

第一阶段数据路径：

`Web 上传 / SD / URL → PhotoFrame 存储与相册 → image_processor → 标准 4bpp → epaper_read_pico → EPD`

触摸输入调用 PhotoFrame 现有轮播入口；电源策略通过 Board HAL 调用 Read Pico PMU。第二阶段增加 `GR 适配器 → 下载本地副本 → 同一存储/图像流程`。

| 层 | 职责 | 改动方向 |
| --- | --- | --- |
| PhotoFrame 产品层 | Web、相册、REST、Wi-Fi、配置、轮播（OTA 已移除） | 复用，仅做必要兼容修正 |
| 图像层 | 解码、方向、缩放/裁切、灰阶和标准 framebuffer | 先验证已有处理，后优化大图与缓存 |
| Board HAL | 初始化、电池/充电、共享资源、睡眠能力 | 新增 Read Pico 实现 |
| 显示后端 | `epaper_*` 契约到裸屏时序/波形 | 独立 Read Pico 后端 |
| 存储适配 | 上游挂载/文件语义到 SDMMC | 唯一/storage、只读CD与租约已接；真实业务存储层接入，产品运行待验收 |
| 输入适配 | CST836U 坐标/事件到轮播请求 | 驱动只报事件，业务层处理手势 |
| 硬件参考组件 | EPDiy、PMU、SY7636A、FCA9555、CST836U、波形 | 固定版本并保留来源，不引入阅读器应用 |
| 后续 GR 扩展 | 相机协议、能力验证、下载和同步 | 独立适配，默认不参与第一阶段运行 |

## 关键移植决策

### 显示

Read Pico使用ESP32-S3 16-bit LCD驱动裸屏。独立后端复用PhotoFrame `epaper_*` 调用，板级IO放在HAL，不引入厂家总BSP/Demo。上游void契约由新板检查式刷新/状态提交包裹，原板保留原行为。

1216×684紧凑4bpp为415,872字节；PhotoFrame偶数像素高nibble、EPDiy偶数像素低nibble，0黑/15白。外层转换采用精确684行；厂家驱动、波形、校准保持锁定原样。HAL拥有总线，后端持有显示租约，按显示→总线顺序锁定。

完整prepare→主动温度→draw→stop事务持有CPU/APB max锁，防止直接LL RMT与CPU feeder受DFS影响。VCOM仅读取PMU出厂值，使用厂家标准易失性启动配置；默认GC16/36相、0–50°C表不改。失败清理并锁存，初始化内存填白不建立物理白参考；隔离诊断由本次启动人工确认白参考后才绘制。

显示租约、软件相位/行指纹与返回值只证明各自观察边界。固定EPDiy内部`portMAX_DELAY`没有安全DMA/renderer取消契约，清理以同步调用返回为前提。实屏重复性、温度准确性、供电与故障恢复仍需验收。详见 [显示契约](READ-PICO-DISPLAY.md)。

### SD 与图像

唯一挂载为PhotoFrame `/storage`：Read Pico SDMMC 1-bit、CLK38/CMD42/D0=44、D1–D3未连接；原板保持4-bit。只读P0.6卡检测，存储租约与IO占用阻止活动文件期间卸载。失败不自动格式化/fallback/重挂，不猜测或借用屏幕电源位控制SD。

标准库包装登记文件/目录及元数据占用，关闭失败保留不确定资源并锁存；有限32槽不是任意VFS/别名/并发close契约。上传使用有界接收、独占新建、完整写入/同步/关闭/介质检查后非覆盖发布；临时文件、原片和相册同名文件保留。FAT两文件和元数据仍不具备已验证的掉电原子性。详见 [存储](READ-PICO-STORAGE.md)、[上传](READ-PICO-UPLOAD.md)。

baseline JPEG采用分块压缩输入、≤5MiB缩放RGB、EXIF 1–8采样坐标映射、固定居中Cover与现有灰阶/抖动/sink；完整源/EOF/关闭/介质确认后才显示，原片不删除。4KiB读缓存减少卡检测开销。内存JPEG沿用相同metadata/预算，PNG在创建输出/显示流前获取布局副本，并在源关闭后提交。保护边界不等于格式/尺寸支持上限，真实峰值仍待测量。详见 [照片](READ-PICO-PHOTO.md)。

Read Pico默认native方向为厂家inverted portrait对应的180°，隔离照片桥同步变换；既有NVS方向覆盖不静默重写。完整图像palette/方向/dither/layout尚未统一为一个事务快照。

### 配置、网络与后台

配置启动只读严格加载，NVS错误在板级/SD前停止，不自动擦除/迁移。名称/网络/NTP/时区/版本的检查式保存确认后发布；参与的本地/远端三组更新与请求配置捕获共用零等待应用控制。完整有界JSON拒绝截断、重复键/NUL/深度错误，serializer和HTTP响应失败传播；直接setter与全部读取者仍待覆盖。

下载前捕获revision/processing/palette自有JSON；URL/凭据/ETag/CA另有受锁保护的自有副本，持有至重试和缩略图结束。下载字段检查式保存全部成功后才发布RAM，身份改变失效ETag，旧请求不写回新身份；NVS多键失败不承诺持久回滚。全部自有初始header先remove/确认缺失/set/精确readback，非法名称/控制值或首错在perform前停止。跨源redirect在下一跳前逐项删除/确认认证头缺失，事件首错锁存并在perform返回后传播，失败不安排下一跳。缩略图保留同源认证和可选失败契约。URL/TLS解析、响应字段截断、SDK内部认证和redirect完整处理仍待补齐。详见 [配置文档](READ-PICO-CONFIG-DOCUMENT.md)。

Wi-Fi manager与AP复用既有控制权；普通HTTP/AP预约共同资源，保留确切句柄和未确认故障。后台角色创建逐项检查，HTTP就绪后才运行；周期timer只合并通知，worker串行执行。SNTP使用外层DNS身份、数字地址、自有服务与回复关联检查。合作取消不是IO回滚/SDK硬中断，完整退出与真实联网尚未验收。详见 [Wi-Fi](READ-PICO-WIFI.md)、[HTTP](READ-PICO-HTTP.md)、[后台](READ-PICO-BACKGROUND.md)、[周期](READ-PICO-PERIODIC.md)、[SNTP](READ-PICO-SNTP.md)。

### 电源与触摸

CW32 PMU可以切断ESP EN。HAL须区分ESP原生睡眠和外部PMU控制；存储提交、显示和网络退出完成后才交给PMU，alarm/按键唤醒、RTC重建和电源unknown状态分别验收。现有Read Pico睡眠入口直接返回。

触摸驱动只生成有界事件，手势复用rotation入口；刷新中重复切图合并，“上一张”先确定历史/顺序语义。CST836U与PMU睡醒产品能力均待实现/验收。

### SDK、构建与迁移

固定ESP-IDF v6.1与厂家gitlink，原板保留回归；诊断80MHz与产品120MHz/HPM分开验证。完整integration profile只保留真实main/Web/相册/处理/轮播链接，不调用产品启动；独立只读/显示/SD/照片profile各有明确IO边界。

OTA 已从项目移除（2026-10-30），固件只经 USB app-only 烧写；本机保持已核验 factory 布局，任何分区变更需单独方案。详见 [集成](READ-PICO-INTEGRATION.md)、[迁移](READ-PICO-MIGRATION.md)、[部署](DEPLOYMENT.md)。

## GR 后续接入

第一阶段验收后增加独立相机协议/能力/下载适配，副本校验提交后进入已有存储、图像与轮播系统，不建立第二套相册/状态系统。GR IV/HDF/Monochrome逐型号/固件验证，不继承旧协议实验的兼容或写操作授权。

来源与许可见 [THIRD-PARTY](THIRD-PARTY.md)，研究资料仅作参考。
