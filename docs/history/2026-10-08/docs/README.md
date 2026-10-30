> **历史快照：2026-10-08文档整理前。** 来源 `docs/README.md`，原文字节SHA256 `ac630f142cad34c28a6dc68180a68500f82fa5435455598dcada3c27f7bbd17f`。以下“当前/最新/下一步/许可”只描述当时；不作实时状态或新操作指令。原文完整保留，仅相对Markdown链接为快照目录重定位。当前任务见 [TODO](../../../../TODO.md)。

# 文件与资料索引

## 当前有效文档

| 文件 | 用途 |
| --- | --- |
| [README](../README.md) | 项目方向与入口 |
| [README.upstream](../../../../README.upstream.md) | PhotoFrame 原始功能说明，非本板烧写指引 |
| [TODO](../TODO.md) | 唯一的当前任务状态与实施顺序 |
| [AGENTS](../../../../AGENTS.md) | 项目协作规则 |
| [DESIGN](../../../../DESIGN.md) | 显示和交互约束 |
| [CHANGELOG](../../../../CHANGELOG.md) | 简要变更记录 |
| [PROJECT-SPEC](../../../PROJECT-SPEC.md) | 当前需求、阶段范围与验收 |
| [ARCHITECTURE](ARCHITECTURE.md) | PhotoFrame 业务与 Read Pico 适配边界 |
| [DEVELOPMENT](DEVELOPMENT.md) | 固定版本、已验证命令与环境 |
| [READ-PICO-BRINGUP](READ-PICO-BRINGUP.md) | P1 诊断入口、只读边界与上板条件 |
| [READ-PICO-DISPLAY](READ-PICO-DISPLAY.md) | P2a/P2b 像素格式、真实后端、结果传播与未验收的硬件边界 |
| [READ-PICO-STORAGE](READ-PICO-STORAGE.md) | P3 SD单一挂载、卡检测、退出租约、限定文件写入与重启只读核对 |
| [READ-PICO-UPLOAD](READ-PICO-UPLOAD.md) | P3 HTTP接收/关闭保护、FAT非覆盖发布与本卡单次合成探针通过/保留产物 |
| [READ-PICO-PHOTO](READ-PICO-PHOTO.md) | P3 原片边界、S1 ROM真机结果、文件JPEG方向/Cover/灰阶软件链路与显示门禁 |
| [READ-PICO-PHOTO-VALIDATION](READ-PICO-PHOTO-VALIDATION.md) | 单张原片显示候选、固定路径、人工白参考/照片确认、内存记录与更新权限 |
| [READ-PICO-INTEGRATION](READ-PICO-INTEGRATION.md) | 完整PhotoFrame编译链接、NVS/存储/路径保护和运行门禁 |
| [READ-PICO-MIGRATION](READ-PICO-MIGRATION.md) | 只读配置加载、旧方向/配置审查、实际分区冲突和OTA恢复/运行门禁 |
| [READ-PICO-WIFI](READ-PICO-WIFI.md) | Wi-Fi初始化/候选保存/扫描、AP共享无线控制/检查式停机、服务取消/资源退出与DNS边界 |
| [READ-PICO-HTTP](READ-PICO-HTTP.md) | 普通页面/AP共用HTTP资源、逐路由注册、请求就绪与错误退出边界 |
| [READ-PICO-BACKGROUND](READ-PICO-BACKGROUND.md) | 四类后台检查式创建/就绪门禁、合作取消与恢复前有界确认；非整机退出 |
| [READ-PICO-PERIODIC](READ-PICO-PERIODIC.md) | 周期timer/第五任务、NVS时间戳与检查式NTP配置；SNTP后续适配另见下项 |
| [READ-PICO-SNTP](READ-PICO-SNTP.md) | 带身份DNS/数字地址服务、PCB预约、SDK回复关联与检查式退出；非真实联网验收 |
| [READ-PICO-TIME-CONFIG](READ-PICO-TIME-CONFIG.md) | 时区检查式保存/应用状态、保护内生成时间副本与HTTP返回；非完整TZ/DST语义验收 |
| [READ-PICO-REMOTE-CONFIG](READ-PICO-REMOTE-CONFIG.md) | 远端结构/步骤错误、三组及版本共享控制；其余general字段/原子性/实机未闭环 |
| [READ-PICO-NETWORK-CONFIG](READ-PICO-NETWORK-CONFIG.md) | 名称/网络保存与同值重试、全组应用保护和请求诊断副本；直接入口/实机未闭环 |
| [READ-PICO-CONFIG-DOCUMENT](READ-PICO-CONFIG-DOCUMENT.md) | 有界文档/两组读写/图像选项与初始header检查；凭据/URL/证书/整图和实机未闭环 |
| [GRAY16-VALIDATION-PLAN](../../../GRAY16-VALIDATION-PLAN.md) | 官方16灰示例复核、分变量验证顺序与光学验收方法 |
| [COMPONENT-GUIDELINES](../../../COMPONENT-GUIDELINES.md) | 拟修改组件与职责 |
| [DEPLOYMENT](DEPLOYMENT.md) | 新方案的烧写/恢复准备要求 |
| [THIRD-PARTY](../../../THIRD-PARTY.md) | 已引入来源、版本与许可边界 |
| [VALIDATION](../../../VALIDATION.md) | 新方案测试矩阵与实际验证记录 |
| [HARDWARE-REFERENCE](../../../HARDWARE-REFERENCE.md) | 官方规格、GPIO/IOE、PMU、频率与本机待填档案 |
| [UPSTREAM-BASELINE](../../../UPSTREAM-BASELINE.md) | 固定源码核对、测试/构建入口、接口与分区差异 |
| [OPEN-ITEMS](OPEN-ITEMS.md) | 剩余信息、负责人、首版范围分层及样本准备 |
| [研究资料说明](../../../reference/README.md) | 附件出处、使用边界及未核对项 |
| [历史归档](../../../archive/README.md) | 旧需求、旧 TODO、实验与协议资料 |

## 当前源码布局

根目录是 PhotoFrame 主工程，保留 `main/`、`components/`、`boards/`、`webapp/`、`process-cli/`、`host_tests/`、`scripts/` 及上游文档。`docs/` 同时含本项目有效文档、上游使用说明、`reference/` 和 `archive/`；上游原板行为不等于 Read Pico 已支持。

`third_party/read_pico_firmware/` 为固定未改子模块；P1 复用 PMU 协议头，P2b 原位选入厂家显示依赖，由独立后端/HAL 持有资源，不引入 Demo/总 BSP。原板不链接新厂家显示驱动。依赖锁为根 `dependencies.lock` 及两份 npm package-lock；测试入口为 `scripts/test_baseline.sh`，移植编译检查为 `.github/workflows/port-baseline.yml`。`build/`、`build-read-pico/`、主机测试构建、生成的 Web/splash、SDK 缓存及日志不入库。

历史快照保留原相对目录以便交叉阅读。历史命令中引用的源码、构建产物和缓存可能已不存在；这些链接不会被伪造为现有文件。
