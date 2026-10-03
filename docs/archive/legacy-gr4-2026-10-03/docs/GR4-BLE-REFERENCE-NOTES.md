# GR IV 关机广播与参考客户端核对

更新：2026-10-02（北京时间）。本轮仅只读参考源码、公开资料核对和设备日志采证；未移植代码、编译、重烧或建立相机连接。

## 结论与证据边界

GR IV 的相机电源关闭不等于无线通信关闭：官方明确说明，无线通信开启时，关机后仍发出蓝牙信号。[GR IV 官方 FAQ](https://www.ricoh-imaging.co.jp/english/support/qa/gr-4/)

本轮用户在追问后确认“只关闭了相机电源”，因此原计划的蓝牙关闭对照实际成为关机对照。私有文件名 `manual-power-gr4-deadline-radio-off-01.log` 是采集前实验意图，不能用文件名证明无线关闭；保留原文件及摘要，不重命名覆盖历史。

| 同版 Pico 扫描 | 用户确认的相机条件 | 协议候选/服务位 | 聚合结果 |
| --- | --- | --- | --- |
| 首轮 | 开机、待配对 | 1 / 1（Camera Information） | reports1618、matches55、invalid0、overflow0 |
| 第二轮 | 只关闭相机电源，未菜单关闭无线 | 1 / 2（Bluetooth Control） | reports1581、matches50、invalid0、overflow0 |

两轮均30秒独立截止、inactive1/stats1/code0/cleanup1、全部20次一致结果与应用返回，说明同版停止/清理流程在两次人工启动中通过。不证明每次候选属于此相机，两个会话的id1只是各自RAM临时编号；服务位1→2不能建立稳定身份或开关机编码。广播存在不能直接授权Wi-Fi、拍摄或唤醒；广播缺失也不能可靠推出关机。启动头/APP_ENTRY/STARTED未捕获，不计独立电气掉电或射频功耗测量。

## 参考仓库与实际代码

用户提供本地项目：`/Users/zhanghuilong/Documents/RICOH-GR-Live-View-Shooting/RICOH-GR-Live-View-Shooting`。基准HEAD `4d1aa81b92409d91399331d2b4a776c009ab7a8b`；工作树有用户修改，本文以读取时实际文件为准，未执行其构建/测试、未修改或回退该项目。其文档中的历史验证不转移为本Pico项目的实机验证。

核对位置：

- `src/ricoh_ble_client.cpp:195`：扫描识别Info/Camera/Shooting/Control服务，作为候选，而不是从UUID判电源状态。
- `src/ricoh_ble_client.cpp:1361`：主动扫描、名称/已保存地址等候选评分、找到目标后停止；与本项目Observer-only、被动/不读名称/地址不落盘的诊断不同，不直接移植扫描策略或自动重连。
- `src/ricoh_ble_client.cpp:1099` 与 `src/camera_protocol_profile.cpp:122`：服务归属、Power UUID/预期句柄、WLAN多特征完整签名联合识别协议代际；共享UUID或单个广播不证明GR IV。其代际结果也不等于本产品GR IV/HDF/Monochrome精确型号/固件白名单。
- `src/ricoh_ble_client.cpp:1919`：连接且协议已识别后读取Power State；GR IV使用参考句柄 `0x00EB`。
- `src/ricoh/gr4/Gr4LegacyBleProtocol.cpp:39`：参考解码 `0x01=On`、`0x00=Off`；其他值不判为已知状态。
- `src/ricoh_ble_client.cpp:1994`：读取Operation Mode UUID `1452335A-EC7F-4877-B8AB-0F72E18BB295`，参考映射0拍摄/1回放/2 BLE启动/3其他/4关机传输。不是广播载荷中的服务位。
- `src/main.cpp:742` 与 `src/services/CameraPowerPolicy.cpp`：在WLAN前检查电源/工作模式。该项目目标是取景/拍摄，其手动唤醒或Unknown放行策略不能直接用于照片相框。

读取时源码SHA256：

| 文件 | SHA256 |
| --- | --- |
| ricoh_ble_client.cpp | `a9d66ebc2be8d164daf73703a2b58db4f0388cee7c7dcb1f1b36460dc0d7c1a8` |
| camera_protocol_profile.cpp | `1c06670ee73abb4841c93751ebdae3e7dbc532500b09b9197a7fa3a568c5ec7c` |
| ricoh/gr4/Gr4LegacyBleProtocol.cpp | `f8f2bd9f5695f347350970553bd21599355e041c2379604a90117f1090cfea02` |

## 本项目采用的设计约束（未接入运行）

后续实现更新（2026-10-02）：已完成便携五字段动态句柄计划及严格值解析，主机33/33与默认组件编译通过，见 [GATT核心](GR4-GATT-CORE.md)。仅实现无IO基础，以下发现/连接/身份/受控读取约束尚未接入运行，参考映射不变为本机已验证语义。

1. 分离 `candidate_present`、`model_verified`、`camera_power`、`operation_mode` 与 `link_authenticated`，初始状态均未知；候选服务位不能推导其他状态。
2. 下一阶段先设计独立有界GATT发现/只读诊断，与现有Observer扫描模式互斥。连接本身可能影响相机待机状态，不能把“只读”描述为完全无状态影响；受控真机阶段须单独记录相机是否被唤醒。
3. 按实际发现的服务/特征UUID、属性和当次句柄选择允许的读取，不将参考 `0x00EB` 或WLAN句柄硬编码为跨固件真理；当前固件静态分析确认Power/Operation Mode特征存在，但未验证本机句柄和值语义。
4. 先核实型号/固件与服务映射，再受控读取Power/Operation Mode。值长度异常、未知枚举、断连或认证错误都保留Unknown并停止，不猜值、不自动改配对参数，不以读取失败触发写入/唤醒重试。
5. 初步只读诊断不读密码/SSID/序列号，不输出原始地址或原始特征值；只允许脱敏型号/版本和分类状态。不得启用Power写入、WLAN写入、ACK、拍摄、升级或删除。
6. 参考Power Notify订阅需要CCCD写入，不属于纯只读发现，暂不启用。参考客户端的固定配对参数、弱化认证条件、bond持久化/清除或自动重连亦不直接移植；先单独完成本项目配对与身份设计、用户交互和验证。
7. 相框需求仍包含“关机时能否唤醒并进入传输”的待验证问题。官方App能力不等于第三方已实现；不照搬取景器的关机即禁Wi-Fi或手动唤醒放行。正式同步按型号/版本、传输队列、认证和已验证工作模式决定，Unknown不授权相机状态写入。

## 后续验证

先做真正的无线关闭负对照：用户在相机设置中关闭无线通信的工作/动作模式，保持其他附近设备条件不变；确认后再采集同版Pico。官方GR WORLD FAQ给出设置入口 `Setup → Wireless Communication Settings → Operating Mode`；中文版名称需按实际菜单核实，不把电源键关机或“电源关闭时传输”开关替代无线总开关。[官方设置说明](https://www.ricoh-imaging.co.jp/english/support/qa/gr-world/)

若仍有候选，先排查其他兼容设备/广播来源，不能仅凭公共UUID归因本台相机；不通过自动连接来补造身份。随后推进只读GATT/型号版本与状态诊断、显式配对、正常/失败退出与功耗，GR IV/HDF/Monochrome分别验证。当前Pico仍为bd74b761…扫描固件，未建立连接。
