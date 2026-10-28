# Read Pico 周期任务与时间戳边界

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-PERIODIC.md)。

沿用PhotoFrame periodic_tasks接口与daily SNTP；Read Pico编译read_pico_periodic.c，原板仍用periodic_tasks.c。

## 检查式NTP配置

新板`config_manager_set_ntp_server_checked`先在控制内校验并复制输入，再检查NVS open/set/commit，关闭句柄后仅在全部确认成功时发布RAM。超长值不截断；拒绝空白/控制/非ASCII字节，空串仍表示上游默认回退。此为文本边界而非DNS/IP语法、可达性或服务器信任验证。void兼容setter在新板调用检查式实现，错误仅记录未确认，不报保存成功；旧板保持原实现。

`config_manager_copy_ntp_server`复制有效值到调用方独立缓冲；无IO，忙/容量不足等错误不改输出。检查式读写及启动加载共用零等待原子保护，重入或并发不阻塞、不追加NVS操作。启动加载失败保留先前已确认NTP副本；其他配置仍是原有startup-only/fail-closed加载，不能据此把init当作全配置并发reload服务。

配置应用先检查保存，失败立即返回且不触发本字段同步；commit确认后同步失败则明确“配置已变化但同步未确认”，不假称多字段配置回滚。配置GET在JSON前复制NTP，忙返回503而非混合值；检查对象和NTP字段分配，但未声称所有JSON字段/配置已检查。NVS set/commit失败可能已影响持久值；失败后可显式再次保存，不自动回滚/清键。

main已经取得独立NTP副本，服务采用 [read_pico_sntp](READ-PICO-SNTP.md) 的外层DNS身份、数字地址及退出确认。旧借用getter保留兼容，但不代表全部直接入口/SDK生命周期受同一控制。

## SDK服务边界

固定SDK原生SNTP init/stop为void异步入队、旧DNS回调无应用身份，netif deinit也不提供完整错误确认；因此新板采用自有适配，不将裸API返回当整栈退出ACK。固定SDK/厂家不修改，IPC等待与真实联网仍待验收，不宣称clock/RTC/NVS可回滚。

## 定时器与受管任务

重复成功 init 不清空注册表、覆盖句柄或创建第二个 timer/worker。先检查确切 timer 创建/每小时启动，再检查后台 worker 创建；错误保留主错误，并检查自己 timer 的退出。SDK create 返回成功空句柄或错误却有句柄时锁存不确定，不猜所有权；关闭失败保留确切句柄，禁止 init 绕过，后续只允许显式 stop 核对。

固定 IDF v6.1 的 ESP_TIMER_TASK 是共享串行派发线程，不宜等待 SNTP。现在 timer 回调只检查原子门禁、合并 pending 标记，无 NVS、网络、业务回调、日志或等待。新增 `periodic_tasks` worker（8192字节、优先级5）接入原后台包装，普通 HTTP 发布必需角色增加 PERIODIC。worker 等就绪再处理；手动批次占用控制时保留 pending，定时通知不丢给下一小时，也不重入回调。

`request_stop` 即时关闭接纳，不是假称退出成功。恢复/启动返回先请求周期停机、取消受管任务并有限等待，再检查 periodic stop，最后才沿用无线/HTTP退出。手动 HTTP 批次仍执行时，periodic stop 的零等待控制保护会拒绝 SDK 销毁，调用方不继续清理。stop 在用户任务上下文使用 SDK `esp_timer_stop_blocking(..., 100ms)` 确认自有 timer 回调结束，再检查 delete；delete 成功只表示 SDK 接受异步删除，非内存已回收。未跨任务删除、关闭共享 timer 服务或自动重建。

## 串行执行与时间戳

注册、批次、强制请求、手动单任务及时间戳读写共用非阻塞控制门禁；回调/NVS 操作期间其他调用报忙，不修改表、再执行或复用资源。注册名按 NVS 15 字节限制校验；满16槽仍可更新既有任务。原 interval=0 禁用语义保留，禁用不能强制；注册更新不清除未确认完成记录。

只将 `ESP_ERR_NVS_NOT_FOUND` 当缺失；IO/类型/无效时间错误不触发网络业务。有效当前时间需不早于2020且 time 未失败；未来/回退时间不做溢出减法或强行运行。检查式 due/get 的输出只在成功且接纳仍有效时发布，bool兼容查询错误时为false。缺失记录也不能让无效时钟自动进入普通任务；RTC未恢复时，main在注册后才请求本次启动的强制 SNTP，用于建立有效时钟。

force 仅设置易失性请求，不删键、commit或绕过不可读/错误类型记录。`run_now` 在同一控制内强制并执行单个指定任务，手动 NTP/配置触发不再有“别人先消费请求，自己批次无操作却报成功”的间隙；它不执行无关注册任务。普通批次遇错误停止，不继续下一个业务回调。

回调错误直接传播、不写完成时间；成功后检查有效时钟和 NVS open/set/commit，再报告完成。记录写入未确认则本任务锁存错误，不由重复 init/register/force 重新运行，以免重复已发生的外部操作；需要后续显式诊断/重新启动，不自动清理旧记录。写入/commit 错误可能已部分生效，测试不证明真实NVS/掉电原子性。取消不回滚已开始的同步、NVS、RTC或网络动作。

## main、Web与SDK边界

main三处批次、电源漂移强制请求及 init/register/force 均检查返回，不伪报在线工作成功。Read Pico SNTP通知标记改原子存储，执行循环检查周期接纳/当前连接，取消或断线不等满10秒；回调有效时间/RTC更新失败不确认完成。手动 Web 同步失败返回503 JSON，不继续发送成功时间；成功响应检查时钟、JSON分配/构建与发送。配置消费方返回同步错误并明确配置可能已变化，不假称回滚。

SNTP适配已覆盖外层请求关联/来源检查和自有退出，不代表固定SDK全部IO有界或所有服务已退出。RTC、其他配置/调度消费者和完整产品清理继续审查。

## 验证边界

主机覆盖真实周期/后台/config_manager适配与合成timer、RTOS、NVS、clock，含重复初始化、错误创建/启动/退出、取消、时钟回退、force/run_now和时间戳失败锁存；部分真实双线程配置测试不等于TSan或真实RTOS验收。完整计数与产物见VALIDATION，不在专题重复维护。

真实NVS/联网/RTC、整机恢复与睡醒按TODO继续验收。
