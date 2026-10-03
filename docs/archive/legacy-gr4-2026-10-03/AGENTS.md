# 项目协作规范

本项目是 GR IV 系列相机直连的小纸 Pico 相框固件，不是网站。

- 需求基线：`docs/PROJECT-SPEC.md`；技术设计：`docs/ARCHITECTURE.md`。
- 开工先查看 `TODO.md`；完成实现或验证后同步更新状态，区分主机测试、固件编译和真机验证。
- 代码上下文探索优先使用 `rg` 并说明限制。
- 当前支持 GR IV / GR IV HDF / GR IV Monochrome；保留扩展，型号/版本兼容需分别验证。
- 相机原片不可删除。未经验证的 BLE 写值、远端 ACK、相机升级/拍摄操作不得启用。
- 便携核心不得依赖 ESP-IDF；板卡与协议 IO 置于独立适配层。
- `third_party/read_pico_firmware` 锁定上游 commit；遵守其目录内 AGENTS.md，不直接修改驱动、波形、校准代码。
- VCOM 只从 PMU 读取；不得写回或自动格式化 SD，不自动擦除 NVS。
- 修改核心状态、路径校验、日志或图像算法时运行对应主机测试；硬件相关修改还需固件编译。
- 保留用户修改；本地编辑使用 apply_patch；不要提交凭据、抓包中的序列号或构建生成物。
- macOS 上 `gh auth status`、认证 git push/gh 写操作在可访问 Keychain 的主机环境执行，不用 GH_TOKEN/GITHUB_TOKEN 绕过 Keychain。
