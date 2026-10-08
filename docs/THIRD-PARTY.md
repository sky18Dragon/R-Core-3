# 上游、固定版本与许可核对

2026-10-08主机JSON测试新增源码依赖复用：SDK组件cJSON 1.7.19~2的SBOM锁定[DaveGamble/cJSON提交](https://github.com/DaveGamble/cJSON/tree/b2890c8d76bbb64e710585ebc0a917196b9c67e7)，MIT，已读取原LICENSE和组件SBOM。host缓存缺失时获取同一提交、只取source、不启供应方子工程；C/H双SHA确认与SDK一致，不使用缺少安全补丁的普通v1.7.19标签。源码`607e756460fa0de37d20a7a9181f2de29c97bfb7ce5a0e6c2f548243836cd852`、头`25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee`，独立获取/31测试通过，详见远端说明/VALIDATION末节。没有复制/改写SDK或供应方源码，已有锁版本不变。

2026-10-03 已引入两份固定源码并复现原板编译；P2b Read Pico 诊断已链接厂家显示依赖，但禁止屏幕启动。主机/编译/真机分开记录，详见 [验证记录](VALIDATION.md)。原板未链接这些新厂家显示组件。

| 来源 | 角色 | 固定基线/状态 |
| --- | --- | --- |
| [aitjcize/esp32-photoframe](https://github.com/aitjcize/esp32-photoframe) | 主工程、相册/Web/网络/图像/轮播/OTA | `f5e3ec974403b31174a3c8da1cf09385453f4abb`，根目录/保留历史 |
| [MindReset/read_pico_firmware](https://github.com/MindReset/read_pico_firmware) | Read Pico 板级与显示驱动参考 | `28cde682a4468a581c278761922724f57d976418`，固定 gitlink，子模块未修改 |
| ESP-IDF | SDK | v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`，原板编译通过；v6.0 原 CI 保留但本机未复跑 |

已读取 PhotoFrame LICENSE 的 MIT 声明、Read Pico Apache-2.0 声明，以及内部 EPDiy LICENSE 的 LGPL-3.0-or-later 和波形 LICENSE 的 Apache-2.0 声明，原文件随源码保留，固定链接见上游基线。P2b 直接编译未改 EPDiy/波形/扫描时序/SY 故障解码器；未修改厂商源码。外层 read_pico_sy_power 的标准易失性启动序列源自固定 sy7636a.c，保留 MindReset 版权与 Apache-2.0 标识，仅补错误传播/资源保留；它不是 PMU 校准实现。发布前按实际链接的 EPDiy LGPL 等文件复核源码、构建与链接交付方式，不能只用顶层 MIT/Apache 声明代替全部组件许可。

P0 解析的根 `dependencies.lock` 固定 cJSON 1.7.19~2、esp_jpeg 1.3.1、libpng 1.6.58~2、mDNS 1.14.0、qrcode 0.1.0~2、zlib 1.3.2~1、LittleFS 1.22.3 和 IDF 6.1.0（含组件 hash）。新显示组件通过固定 gitlink 引用。原板构建后的锁摘要为 `4639f56e78af56730ceed80dcb2c64aa3678647d4be4092f5ab77656b9283514`；选入厂家 SY/波形组件的 Read Pico 构建增加直接 `idf >=5.3` 约束，组件管理器重算 manifest hash，当前锁摘要为 `c499f738912ad362e66884b6190edc8c1428174fca47d6685e9dad2262997d65`。逐项核对两个锁的 dependencies 对象完全一致，未升级任何包/SDK或改变组件 hash；不要将配置间锁摘要变化误报为依赖升级。npm 两份锁文件沿用上游；GoogleTest v1.14.0 由 host CMake 引用；SDK Python 使用 v6.1 约束，组件管理器为 3.1.2。

EPDiy、波形、PMU、触摸、图像解码及其传递依赖分别记录版本。上游通用 EPDiy 不能未经实板验证替换厂家时序/波形。新增代码许可随主工程引入方案明确，不在本次资料整理中代替用户选择。

2026-10-08名称/网络软件只在外层修改。读取固定mDNS1.14.0的hostname/instance/TXT复制与排队契约，栈名称副本不是SDK异步借用；读取固定IDF/lwIP地址转换的前导零八进制行为，外层拒绝歧义拼写。未更改这些供应方源码/许可/版本，SDK和厂家目录干净、依赖锁最终无差异。新增host SDK替身仅验证调用顺序，不当作供应方内部或RF验收，见[名称与网络说明](READ-PICO-NETWORK-CONFIG.md)。

研究报告的完整参考文献及伪代码保留在 [原始参考副本](reference/esp32-photoframe-read-pico-port-research.md)。
