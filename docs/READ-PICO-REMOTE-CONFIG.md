# Read Pico 远端配置协调与版本确认

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-REMOTE-CONFIG.md)。

现行解析契约：解析前安全检查/真实cJSON文档检查迁到共享`config_document`，远端2048容量、三组顺序/阶段/空组语义保留；processing/palette回调改checked转换，拒错误类型/截断/非float有限值及非法RGB，不自动改数值。两组HTTP GET与更新共享应用控制，读取/序列化及输入详细边界见[配置文档说明](READ-PICO-CONFIG-DOCUMENT.md)。general内部其余未检查字段仍待补齐。

## 输入与执行

响应头容量2048（含NUL），不截断超长值、不重复覆盖；空首值也算一次。每个响应重置seen，失败锁存到本次下载owner，SDK callback返回值不是唯一保护。输入接口接收SDK的C字符串；不据此宣称识别原始HTTP中被SDK截断的NUL后字节。

解析前限制嵌套16层、拒绝转义NUL；真实cJSON严格接受单一完整文档，拒绝尾随内容、重复/大小写等价键、非有限数字。根及已知组需object，三个组的结构全部通过后才开始业务。未知根metadata沿用上游忽略，空/未知-only不写版本；不代表组内所有字段/枚举/range都已语义验证。

按原顺序general→processing→palette执行；保存/重载/版本回调错误停止后续步骤并返回阶段/已完成mask。保存已成功不回滚，多组并非原子事务。processing逐个检查13个NVS set，第一个失败不继续/commit；有限浮点/字符串终止检查，新板默认清零保证固定槽终止。不改变现有数值、处理默认、gamma或厂家参数。palette保存错误/重载错误传播，load仅NOT_FOUND允许默认，IO/类型/长度/非有限record拒绝且不改输出。

协调器的成功只表示这些回调返回成功和版本保存确认。**general内部仍有void setter及未完整检查的字段，可能掩盖自己的保存错误**；其余字段/JSON语义、并发和多字段事务仍需逐项完善，不能把本轮当“完整远端配置已可靠应用”。旧转换器的默认填补/未知字段/枚举策略未全部重写。

## 版本和下载结果

远端解析将一次远端解析、三组回调及finish一起放在共享应用控制内，与本地general/处理/调色板修改互斥。繁忙不开始解析/业务回调，不自动重试；private general回调避免递归获取控制。本地general的值更新和版本在同一保护内，诊断在释放前复制到请求结果。仅参与的应用入口互斥，不包含所有直接setter/读者/SDK消费者，不增加SDK等待时限、跨组回滚或掉电原子性；详见[配置说明](READ-PICO-NETWORK-CONFIG.md)。

检查式版本touch在共用time_config_control中检查有效clock、int64溢出、NVS open/set/commit/close，仅确认后发布RAM。逻辑秒版本取max(now,previous+1)，同秒/时钟回退也前进；不是严格实时时间或跨客户端CAS。直接void setter在新板也检查保存并拒绝回退，读取请求头使用受保护副本；加载失败保留此前确认版本。IO失败可能已改NVS，不假称回滚；未保护旧getter仍不适合外部并发消费者。

五个本地配置/处理/调色板更新入口检查版本结果，未确认返回503部分成功说明，而不是成功JSON；调色板先检查重载再版本。所有已有字段保存/JSON分配/收发并未因此完整验证。

远端配置失败不开始本次图像显示，不确认本次下载ETag；独占已完成衍生文件保留，不删除原片。此前general步骤可能已改变URL/清掉旧ETag或其他设置，不能说失败后所有缓存/配置一定没变。304也检查携带的配置，失败不发布not_modified成功；不刷新图片。配置callback/版本在图像显示前确认，后续图像失败不自动撤回配置成功。

## 依赖与证据

真实cJSON测试优先复用SDK managed component；干净host环境没有缓存时只获取SBOM固定上游提交`b2890c8d76bbb64e710585ebc0a917196b9c67e7`，不加载其tests/install子工程。SDK组件1.7.19~2含安全补丁，与普通v1.7.19标签不同，不能只比较版本号。源码SHA `607e756460fa0de37d20a7a9181f2de29c97bfb7ce5a0e6c2f548243836cd852`、头SHA `25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee`均检查。原MIT版权/许可证保留，不修改SDK/供应方源码。

巨大utils/HTTP/main未在host运行，源接入检查不是浏览器/SDK网络/真实NVS/掉电或整栈验收；ASan非TSan。详情与最终身份见[VALIDATION](VALIDATION.md)末节。未部署/回读/联网或操作设备、测试卡、原片、相机；产品/OTA/迁移门禁保持。

本地/远端应用与配置请求捕获已共享控制，图像选项读取及自有初始header检查已接入；其余general字段、凭据/完整快照及RTC/调度/文件/退出按TODO继续。CMake<3.18获取分支、真实服务器/RF/NVS/恢复尚未验收。
