# 文件与资料索引

## 先读这四份

| 文件 | 用途 |
| --- | --- |
| [TODO](../TODO.md) | 唯一的当前进度与下一步，只含R1 |
| [测试策略](TESTING.md) | T0–T3分层、按改动触发、R1真机验收单；`scripts/test_quick.sh` |
| [路线图](ROADMAP.md) | R2、P0–P5、G1–G3，不阻塞R1 |
| [验证记录](VALIDATION.md) | 里程碑汇总与候选台账；逐轮全文在[历史](history/2026-10-11/INDEX.md) |

## 项目与规则

| 文件 | 用途 |
| --- | --- |
| [README](../README.md) | 项目方向与入口 |
| [AGENTS](../AGENTS.md) | 项目协作规则 |
| [DESIGN](../DESIGN.md) | 显示和交互约束 |
| [CHANGELOG](../CHANGELOG.md) | 阶段性变更摘要 |
| [PROJECT-SPEC](PROJECT-SPEC.md) | 需求、阶段范围与验收 |
| [ARCHITECTURE](ARCHITECTURE.md) | PhotoFrame 业务与 Read Pico 适配边界 |
| [DEVELOPMENT](DEVELOPMENT.md) | 固定版本、构建命令与环境 |
| [DEPLOYMENT](DEPLOYMENT.md) | 烧写/恢复前的核对要求与本机分区布局 |
| [COMPONENT-GUIDELINES](COMPONENT-GUIDELINES.md) | 组件与职责 |

## R1 链路的现行契约（本地上传/显示/轮播会经过）

| 文件 | 用途 |
| --- | --- |
| [READ-PICO-BRINGUP](READ-PICO-BRINGUP.md) | 板级诊断入口、只读边界、上板条件 |
| [READ-PICO-DISPLAY](READ-PICO-DISPLAY.md) | 像素格式、显示后端、结果传播与硬件边界 |
| [READ-PICO-STORAGE](READ-PICO-STORAGE.md) | SD单一挂载、卡检测、退出租约 |
| [READ-PICO-UPLOAD](READ-PICO-UPLOAD.md) | HTTP上传接收、FAT非覆盖发布 |
| [READ-PICO-PHOTO](READ-PICO-PHOTO.md) | 原片边界、JPEG方向/Cover/灰阶链路 |
| [R1-RELEASE](R1-RELEASE.md) | R1 交付说明：固件身份、构建与只写应用部署、网页使用、已验证范围与已知限制 |
| [R1-RUNTIME](R1-RUNTIME.md) | R1本地相框启动顺序、范围外入口处置、已知限制 |
| [READ-PICO-INTEGRATION](READ-PICO-INTEGRATION.md) | 完整PhotoFrame编译链接、NVS/存储/路径保护、运行门禁 |
| [READ-PICO-WIFI](READ-PICO-WIFI.md) | Wi-Fi初始化、AP、扫描、检查式停机 |
| [READ-PICO-HTTP](READ-PICO-HTTP.md) | HTTP资源、逐路由注册、请求就绪 |
| [READ-PICO-BACKGROUND](READ-PICO-BACKGROUND.md) | 后台任务创建/就绪门禁与合作取消 |
| [READ-PICO-PERIODIC](READ-PICO-PERIODIC.md) | 周期任务、NVS时间戳 |
| [READ-PICO-CONFIG-DOCUMENT](READ-PICO-CONFIG-DOCUMENT.md) | 配置读写、图像选项 |
| [READ-PICO-NETWORK-CONFIG](READ-PICO-NETWORK-CONFIG.md) | 名称/网络配置保存 |
| [READ-PICO-TIME-CONFIG](READ-PICO-TIME-CONFIG.md) | 时区保存与应用 |

## 非R1必需，按需查阅

| 文件 | 何时看 |
| --- | --- |
| [READ-PICO-MIGRATION](READ-PICO-MIGRATION.md) | 部署前确认分区冲突；OTA/旧配置迁移 |
| [READ-PICO-REMOTE-CONFIG](READ-PICO-REMOTE-CONFIG.md) | 重新启用远程配置时（R1拒绝该入口） |
| [READ-PICO-SNTP](READ-PICO-SNTP.md) | 修改时间同步时 |
| [READ-PICO-PHOTO-VALIDATION](READ-PICO-PHOTO-VALIDATION.md) | 修改单张照片诊断profile时 |
| [GRAY16-VALIDATION-PLAN](GRAY16-VALIDATION-PLAN.md) | 修改显示后端/波形/供电时序时 |

分类依据是各文档摘要与TODO的R1范围，未逐份重写。

## 硬件与上游参考

| 文件 | 用途 |
| --- | --- |
| [HARDWARE-REFERENCE](HARDWARE-REFERENCE.md) | 官方规格、GPIO/IOE、PMU、频率与本机档案 |
| [UPSTREAM-BASELINE](UPSTREAM-BASELINE.md) | 固定源码核对、接口与分区差异 |
| [THIRD-PARTY](THIRD-PARTY.md) | 引入来源、版本与许可 |
| [OPEN-ITEMS](OPEN-ITEMS.md) | 信息缺口、决策编号 |
| [README.upstream](../README.upstream.md)、[API](API.md)、[DEV](DEV.md)、[MEASURED_PALETTE](MEASURED_PALETTE.md) | PhotoFrame 上游说明，不等于本板已支持 |
| [研究资料](reference/README.md) | 附件出处与使用边界 |

## 历史（不作为新指令）

| 文件 | 内容 |
| --- | --- |
| [精简前全文](history/2026-10-11/INDEX.md) | VALIDATION全文、精简前TODO与CHANGELOG |
| [进度历史快照](history/README.md) | 2026-10-08整理前专题文档原文 |
| [旧独立方案](archive/README.md) | 旧架构与协议研究 |

## 状态与历史的维护规则

TODO集中维护进度、下一步、运行门禁和最近候选；专题文档维护现行接口/限制，不叠加“最新”段落。VALIDATION/CHANGELOG按轮次追加事实，历史记录不回写成后来结果。历史快照属于当前移植方案的过去记录；archive仅是旧独立方案，二者均不作为新操作指令。

## 当前源码布局

根目录是 PhotoFrame 主工程，保留 `main/`、`components/`、`boards/`、`webapp/`、`process-cli/`、`host_tests/`、`scripts/` 及上游文档。`docs/` 同时含本项目有效文档、上游使用说明、`reference/` 和 `archive/`；上游原板行为不等于 Read Pico 已支持。

`third_party/read_pico_firmware/` 为固定未改子模块；P1 复用 PMU 协议头，P2b 原位选入厂家显示依赖，由独立后端/HAL 持有资源，不引入 Demo/总 BSP。原板不链接新厂家显示驱动。依赖锁为根 `dependencies.lock` 及两份 npm package-lock；测试入口为 `scripts/test_baseline.sh`，移植编译检查为 `.github/workflows/port-baseline.yml`。`build/`、`build-read-pico/`、主机测试构建、生成的 Web/splash、SDK 缓存及日志不入库。

历史快照保留原相对目录以便交叉阅读。历史命令中引用的源码、构建产物和缓存可能已不存在；这些链接不会被伪造为现有文件。
