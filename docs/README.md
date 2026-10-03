# 文件与资料索引

## 当前有效文档

| 文件 | 用途 |
| --- | --- |
| [README](../README.md) | 项目方向与入口 |
| [README.upstream](../README.upstream.md) | PhotoFrame 原始功能说明，非本板烧写指引 |
| [TODO](../TODO.md) | 唯一的当前任务状态与实施顺序 |
| [AGENTS](../AGENTS.md) | 项目协作规则 |
| [DESIGN](../DESIGN.md) | 显示和交互约束 |
| [CHANGELOG](../CHANGELOG.md) | 简要变更记录 |
| [PROJECT-SPEC](PROJECT-SPEC.md) | 当前需求、阶段范围与验收 |
| [ARCHITECTURE](ARCHITECTURE.md) | PhotoFrame 业务与 Read Pico 适配边界 |
| [DEVELOPMENT](DEVELOPMENT.md) | 固定版本、已验证命令与环境 |
| [READ-PICO-BRINGUP](READ-PICO-BRINGUP.md) | P1 诊断入口、只读边界与上板条件 |
| [READ-PICO-DISPLAY](READ-PICO-DISPLAY.md) | P2a/P2b 像素格式、真实后端、结果传播与未验收的硬件边界 |
| [GRAY16-VALIDATION-PLAN](GRAY16-VALIDATION-PLAN.md) | 官方16灰示例复核、分变量验证顺序与光学验收方法 |
| [COMPONENT-GUIDELINES](COMPONENT-GUIDELINES.md) | 拟修改组件与职责 |
| [DEPLOYMENT](DEPLOYMENT.md) | 新方案的烧写/恢复准备要求 |
| [THIRD-PARTY](THIRD-PARTY.md) | 已引入来源、版本与许可边界 |
| [VALIDATION](VALIDATION.md) | 新方案测试矩阵与实际验证记录 |
| [HARDWARE-REFERENCE](HARDWARE-REFERENCE.md) | 官方规格、GPIO/IOE、PMU、频率与本机待填档案 |
| [UPSTREAM-BASELINE](UPSTREAM-BASELINE.md) | 固定源码核对、测试/构建入口、接口与分区差异 |
| [OPEN-ITEMS](OPEN-ITEMS.md) | 剩余信息、负责人、首版范围分层及样本准备 |
| [研究资料说明](reference/README.md) | 附件出处、使用边界及未核对项 |
| [历史归档](archive/README.md) | 旧需求、旧 TODO、实验与协议资料 |

## 当前源码布局

根目录是 PhotoFrame 主工程，保留 `main/`、`components/`、`boards/`、`webapp/`、`process-cli/`、`host_tests/`、`scripts/` 及上游文档。`docs/` 同时含本项目有效文档、上游使用说明、`reference/` 和 `archive/`；上游原板行为不等于 Read Pico 已支持。

`third_party/read_pico_firmware/` 为固定未改子模块；P1 复用 PMU 协议头，P2b 原位选入厂家显示依赖，由独立后端/HAL 持有资源，不引入 Demo/总 BSP。原板不链接新厂家显示驱动。依赖锁为根 `dependencies.lock` 及两份 npm package-lock；测试入口为 `scripts/test_baseline.sh`，移植编译检查为 `.github/workflows/port-baseline.yml`。`build/`、`build-read-pico/`、主机测试构建、生成的 Web/splash、SDK 缓存及日志不入库。

历史快照保留原相对目录以便交叉阅读。历史命令中引用的源码、构建产物和缓存可能已不存在；这些链接不会被伪造为现有文件。
