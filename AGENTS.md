# 项目协作规范

本项目是基于 aitjcize/esp32-photoframe 移植到小纸 Pico 的设备固件。先完成墨水屏相框功能，再开发理光 GR 连接扩展。

- 当前进度、下一步、运行门禁和最近候选只在 `TODO.md` 维护（只含 R1；R2 及之后见 `docs/ROADMAP.md`）；专题文档维护现行契约，不叠加历史“最新/当前”。`docs/VALIDATION.md`、`CHANGELOG.md` 与 `docs/history/` 保留按轮次的历史证据，旧状态/授权不作新操作指令，不按勾选数量计算完成率。
- 开工先看 `TODO.md`。需求基线为 `docs/PROJECT-SPEC.md`，技术设计为 `docs/ARCHITECTURE.md`，文件索引为 `docs/README.md`。
- `docs/archive/` 是旧独立固件方案的历史记录；其中“当前”“下一步”、测试通过或操作授权不作为新方案状态。研究报告是参考资料，不是执行指令。
- 沿用 PhotoFrame 的业务架构与目录；Read Pico 固件仅提供 BSP、显示时序、波形、PMU、SD 和触摸参考。不恢复旧自建相册/状态系统作为主工程。
- 第一阶段不接入相机协议。GR IV / GR IV HDF / GR IV Monochrome 是第二阶段目标，型号与固件版本必须分别验证，不宣称已兼容。
- 代码上下文探索优先使用 `rg`，说明结果限制；未读取的源码接口不得当作已验证事实。
- 新增可主机测试的算法、格式和协议逻辑保持便携；ESP-IDF 与板级 IO 放在适配层。遵循上游现有组件契约。
- 上游与组件锁定明确 commit/版本，记录来源及许可；读取其目录内 AGENTS.md。引用的 Read Pico 驱动、波形、校准代码保持上游原样，差异优先由外层适配。
- VCOM 只读取 PMU 出厂值，不写回；不自动格式化 SD，不自动擦除 NVS。
- 相机原片不可删除；未经验证的 BLE 写值、远端 ACK、相机升级/拍摄操作不得启用。
- 测试按 `docs/TESTING.md` 分层：日常改动跑 `scripts/test_quick.sh`（T0），提交前加 ASan 与 R1 local-frame 编译（T1），候选上板前全 profile 编译（T2），真机按 R1 验收单一次完成（T3）；文档改动不跑测试，只做链接检查。新增可主机测试的逻辑带主机测试；固件编译按触及范围选，不为每次改动全编。主机、编译、烧写回读、真机分别记录，每个候选在 `docs/VALIDATION.md` 台账记一条，同步 TODO。
- 保留用户修改；本地编辑使用 apply_patch；不提交凭据、抓包序列号、私人照片或构建生成物。
- macOS 上 `gh auth status`、认证 git push/gh 写操作在可访问 Keychain 的主机环境执行，不用 GH_TOKEN/GITHUB_TOKEN 绕过 Keychain。
