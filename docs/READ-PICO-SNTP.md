# Read Pico 检查式时间同步适配

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-SNTP.md)。

## 配置、解析与所有权

main检查式复制NTP配置，适配再次持有完整副本；SDK不借用活动RAM或栈字符串。解析采用SDK默认地址类型（当前IPv4优先、IPv6回退），不缩成IPv4-only；未证明真实双栈兼容。适配在TCPIP上下文用非零、不回绕ticket发起自己的DNS查询，仅匹配ticket且PENDING才发布。SDK SNTP只配置已解析数字地址，不发出内部无身份DNS；未指定/组播地址拒绝。缓存/异步成功与失败均检查。

10秒循环预算覆盖解析/回复等待，但不限制SDK同步IPC、连接检查或系统调用总时长。DNS超时/断线/取消不复用pending静态槽、不释放其存储、不清全局DNS缓存；晚到结果只结束本槽，不能自动同步。未完成时新请求/退出返回NOT_FINISHED；完成后退出还检查core栅栏，确认callback派发返回再允许main清理无线/HTTP。不是全局DNS或整个网络栈退出证明。

零等待原子控制串行同步/退出。request_stop永久关闭本启动接纳与时钟应用，不是假称资源退出；控制忙不进入SDK。进入服务前拒绝已启用或已配置的外来SNTP，不先stop或覆盖它。

固定SDK在udp_new_ip_type失败时断言；外层先通过真实分配器预约一个准确PCB，失败返回NO_MEM且尚未配置/启动。只有当前core同步SDK init消耗预约；链接包装对其他UDP/DNS分配透明透传，异常未消费只移除自己的PCB并锁存。启动检查enabled；结束先关闭时钟接纳，再core停止自有服务、确认disabled后清自有引用。IPC返回与callback执行ACK均须确认。异常/未确认退出保留所有权并锁存，重复同步不能绕过；显式stop可再尝试实际关闭，但不自动抹掉不确定故障。

强同步hook是此板唯一SNTP客户端的外层实现：未拥有/接纳不改clock。外来客户端不被配置/停掉，但不能通过本hook更新时间；不是多SNTP客户端共享支持。当前新板没有其他同步入口，后续组件必须重新审查这一排他契约。

## 回复与时钟

外层CMake只在新板完整集成给固定lwIP启用SNTP_CHECK_RESPONSE=2、关闭RTT补偿；旧板/隔离profile策略不改。SDK检查来源地址、端口和回复Originate Timestamp。链接包装SDK取请求时间，生成本启动严格单调唯一的秒/微秒，重复时刻或wall-clock回退也不复用；SDK按原逻辑编码Transmit并检查回声。耗尽/无效输入锁存，不发布成功。这只是关联标记，不作为设备时钟/RTT校时，因此补偿必须关闭。

通过SDK检查的回复进入同一TCPIP上下文hook，仅接纳第一次；检查非阻塞门禁、2020–2099范围/微秒及settimeofday返回。取消/写入错误不记成功；自有退出确认并再次检查接纳/连接后才输出时间。main在RTC和轮播timer前也复查，周期层另检查NVS完成记录。

关联不是NTS/认证、可信时间或全协议安全验证；固定SDK级2没有实现级3/4注释中所有LI/版本/stratum/Root Dispersion规则，本轮不宣称补齐。不能回滚通过最后门禁后已发生的时钟/RTC/NVS，也不承诺断线瞬间零竞态。main传给clock hook的仅周期原子门禁；连接检查在worker，不在网络callback中等待Wi-Fi锁。

## 接入、证据与限制

main日同步使用副本及检查式结果；恢复/启动返回先关闭SNTP/周期接纳，再等受管回调、检查周期timer/SNTP退出。忙/未完成/故障不继续无线/HTTP清理。旧板原volatile通知路径保持，产品/OTA/迁移门禁不开放。

新增54项CTest：5便携时间戳、40真实适配C接合成DNS/core/RTOS/clock故障、9真实适配与固定SDK原始sntp.c接合成UDP/pbuf/timeout。后者直接运行实际报文处理，验证正确/旧Originate、clock回退、错误地址/端口/长度/模式/KoD、重复/取消回复、资源不足与PCB/timer退出，不是自行重写parser后测试模型。

SDK测试需显式READ_PICO_TEST_IDF_PATH；CMake检查原文件SHA256 `8695155c80c807d7ac949801b023d4e2f27db7975f78bcbba726bcdc012020e7`，不下载/复制SDK分叉。未设置时这9项不会存在，见[开发命令](DEVELOPMENT.md)。IO/timeout为替身、随机启动延迟替身100ms、地址类型模型简化，非真实RTOS/RF/DNS/双栈/分配器/浏览器/掉电验收；ASan不是TSan。

时区保存/应用与时间响应契约见 [时区说明](READ-PICO-TIME-CONFIG.md)。服务真实网络/双栈/恢复、RTC和整机退出尚待验收，逐轮计数/产物只在VALIDATION保留，下一步见TODO。
