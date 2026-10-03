# 历史资料归档

归档日期：2026-10-03。目录 [legacy-gr4-2026-10-03](legacy-gr4-2026-10-03/README.md) 保存旧独立 GR 相框方案的 43 份 Markdown，按原根目录与 `docs/` 层级原文迁移；[SHA256 清单](legacy-gr4-2026-10-03/MANIFEST.sha256) 用于完整性核对。

归档核对：43/43 文件 SHA256 与迁移前一致，原文合计 662,617 字节。可在快照目录执行 `shasum -a 256 -c MANIFEST.sha256` 复核。

归档内“当前”“下一步”“已完成”、构建命令、设备操作授权和 AGENTS 规则属于旧项目背景，不是新方案操作入口。历史源码/缓存引用可能失效，原文保留以免损失证据。

## 查阅入口

| 内容 | 历史文件 |
| --- | --- |
| 停止边界与旧任务流水 | [TODO](legacy-gr4-2026-10-03/TODO.md) |
| 原需求与架构 | [PRD](legacy-gr4-2026-10-03/docs/PROJECT-SPEC.md)、[架构](legacy-gr4-2026-10-03/docs/ARCHITECTURE.md) |
| 历史显示、SD、蓝牙实验 | [硬件记录](legacy-gr4-2026-10-03/docs/HARDWARE-RESULTS.md) |
| BLE 协议研究 | [GR4 v1.11 BLE](legacy-gr4-2026-10-03/docs/protocol/gr4-v111-bluetooth-firmware-analysis.md) |
| HTTP 协议研究 | [GR4 v1.11 HTTP](legacy-gr4-2026-10-03/docs/protocol/gr4-v111-http-api-analysis.md) |
| 旧版变化 | [CHANGELOG](legacy-gr4-2026-10-03/CHANGELOG.md) |

## 外部资料位置

文档整理时记录过以下路径；P0 源码引入后重新检查，只有测试资料目录仍存在，没有迁移或恢复这些文件：

- 测试图片/日志/辅助资料：`/Users/zhanghuilong/Documents/ChatGPT/R-Core-3-2-测试资料`。
- 历史记录中的旧源码路径：`/Users/zhanghuilong/.Trash/R-Core-3-2-code-U2TFQd`，本轮已不存在。
- 历史记录中的旧备份路径：`/Users/zhanghuilong/.Trash/R-Core-3-2-firmware-backups-3CRIkV`，本轮已不存在。

旧 TODO 记载测试资料归档共 456 个文件、112 份固件备份移至废纸篓；数量和内容本次未重验。两条旧废纸篓路径已失效，不能承诺仍可恢复；原因未知。本轮未删除它们，上板前必须重新取得可核验的备份/恢复资料。

## 最后设备记录的使用边界

按旧 TODO 停止说明，最后 d14d1093… 三镜像已写入并通过镜像哈希验证，整片回读未做，应用未自动启动。这只是停止时的历史记录；本次没有连接设备，不能确定此刻设备状态或宣告已恢复原厂。

旧显示与蓝牙实验可帮助定位风险；新 PhotoFrame 主工程必须重新构建并验收。当前计划统一见 [新 TODO](../../TODO.md)。
