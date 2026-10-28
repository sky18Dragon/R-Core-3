# Read Pico 时区保存与时间响应

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-TIME-CONFIG.md)。

## 保存和应用是两件事

新板时区setter在零等待控制内校验并复制请求，再检查NVS open/set/commit，关闭后才发布保存确认的RAM。空串/超长/控制/非ASCII拒绝，不截断。与上游一致仅校验文本形状，不新建POSIX语法解析器；可打印空格仍可进入库解析。

保存未确认时不调用setenv/tzset，不发布候选RAM；NVS失败仍可能部分生效，不能说旧持久值一定未变。commit确认后才尝试安装TZ、调用tzset并读回TZ一致性。setenv失败或读回不一致返回NOT_FINISHED，RAM与NVS已保存的候选保留，应用标记false；明确“已保存但系统应用未确认”，不假称回滚。后续用户显式再次保存可以重做，不自动写/清NVS。

tzset是void、没有语法错误结果；应用标记true只证明环境安装/读回一致且tzset调用返回，不证明库理解了全部规则、DST语义正确或所有消费者都在本锁内。非法语法仍可能被libc退回UTC，现有形状校验政策没有把这一点改成完整语义验收。

启动加载也校验时区文本并检查应用，错误拒绝启动；失败保留先前确认的时区/NTP RAM，应用尝试失败则标记false，不声称已恢复全局TZ。init仍启动专用，不是全配置在线reload/原子回滚，其他缓存沿用此前fail-closed加载。配置加载不写迁移、默认或删除键。

## 副本与接口

时区setter、NTP检查式读写、时区/时间副本及启动加载共用time_config_control；忙/重入拒绝，不额外IO。借用getter保留旧板契约，新板这三个HTTP消费者不再用它。不宣称所有配置/libc/localtime或其他后台时钟消费者已串行。

配置GET复制时区和应用标记，返回原timezone及新增timezone_applied。各字段副本并非整份配置原子快照，不能当多字段事务。

时间GET与手动同步成功响应使用同一检查式帮助函数：保护内检查应用标记/有效time/localtime_r/strftime与两输出容量，再一次发布时间戳、格式化文本及独立时区字符串。无效/忙/未应用/容量错误均不改输出；不把带库指针的struct tm交给响应层，避免解锁后配置变化再格式化。HTTP失败不发送成功时间，JSON对象/字段/Print和响应发送均检查；手动同步在触发前检查HTTP就绪。已经完成的同步不会因响应生成失败回滚。

utils只在新板依赖检查式setter，不再次setenv/tzset；应用未确认明确部分成功，停止本次后续配置字段。此前已保存字段不回滚，参与的本地/远端三组及版本已有应用控制；所有直接字段/读者、掉电原子性和整份快照仍待完善。

## 验证范围

新增25项真实config_manager C测试（24新板、1旧板），接合成NVS/环境错误/time/localtime/strftime；部分成功状态、启动拒绝/恢复缓存、容量/输出不变、申请输入变化、跨NTP/时区/加载重入及真实双线程忙拒绝覆盖。正常环境安装/时区格式化使用主机libc，不是ESP newlib/DST实机证明。ASan不是TSan，也未执行真实HTTP/main/RTC/网络/NVS。

真实TZ/DST、HTTP/RTC/网络/NVS及完整产品仍待验收；参与应用控制见 [配置说明](READ-PICO-NETWORK-CONFIG.md)，下一步见TODO。
