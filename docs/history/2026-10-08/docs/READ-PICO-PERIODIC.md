> **历史快照：2026-10-08文档整理前。** 来源 `docs/READ-PICO-PERIODIC.md`，原文字节SHA256 `2db29bfecce85e018028bdb56f09a2870f0361c909159822bc5239ca82a681a8`。以下“当前/最新/下一步/许可”只描述当时；不作实时状态或新操作指令。原文完整保留，仅相对Markdown链接为快照目录重定位。当前任务见 [TODO](../../../../TODO.md)。

# Read Pico 周期任务与时间戳边界

后续[时区软件](READ-PICO-TIME-CONFIG.md)已补保存/应用状态和HTTP时间副本；与NTP共用time_config_control，非全配置/libc互斥。普通/ASan各963、Python73、六配置通过，未部署；下方938/884/867是此前轮次。

最新SNTP已接入[检查式服务](READ-PICO-SNTP.md)：main不再借用可变NTP，带ticket外层DNS→数字地址SDK、唯一请求时间戳/来源回声检查、PCB预约/时钟返回/自有退出检查；未确认不继续无线/HTTP清理。普通/ASan各938、Python71及六配置通过，未部署。下面884/867及“服务未修复”是此前轮次状态，SDK本身/全栈退出和真实联网不据此宣称通过。

2026-10-08，仅软件。沿用 PhotoFrame 的 periodic_tasks 接口与 daily SNTP；Read Pico 编译 `read_pico_periodic.c` 适配，旧板仍编译原 `periodic_tasks.c`。完整产品入口仍不运行。

最新增量：检查式NTP保存/副本与启动加载已接入，普通/ASan各884唯一、Python69和六配置编译通过。下面36项周期/867为上一轮证据，本轮新增17项配置测试；最终产物见VALIDATION末节。

## 检查式NTP配置（本轮）

新板`config_manager_set_ntp_server_checked`先在控制内校验并复制输入，再检查NVS open/set/commit，关闭句柄后仅在全部确认成功时发布RAM。超长值不截断；拒绝空白/控制/非ASCII字节，空串仍表示上游默认回退。此为文本边界而非DNS/IP语法、可达性或服务器信任验证。void兼容setter在新板调用检查式实现，错误仅记录未确认，不报保存成功；旧板保持原实现。

`config_manager_copy_ntp_server`复制有效值到调用方独立缓冲；无IO，忙/容量不足等错误不改输出。检查式读写及启动加载共用零等待原子保护，重入或并发不阻塞、不追加NVS操作。启动加载失败保留先前已确认NTP副本；其他配置仍是原有startup-only/fail-closed加载，不能据此把init当作全配置并发reload服务。

配置应用先检查保存，失败立即返回且不触发本字段同步；commit确认后同步失败则明确“配置已变化但同步未确认”，不假称多字段配置回滚。配置GET在JSON前复制NTP，忙返回503而非混合值；检查对象和NTP字段分配，但未声称所有JSON字段/配置已检查。NVS set/commit失败可能已影响持久值；失败后可显式再次保存，不自动回滚/清键。

保护不包含仍返回可变静态RAM的借用getter；当前main/SNTP仍使用它，底层又借用服务器指针，**所有NTP消费者并发安全尚未完成**。没有将短期局部变量交给SDK或在异步stop后释放服务器内存。下一步必须将服务所有权、服务器存活期和回复关联一起处理，产品门禁保持。

## SDK服务审查（只读发现，未实现修复）

固定IDF v6.1：`esp_sntp_init/stop`为void异步入队；lwIP `sntp_setservername`保留传入指针。`sntp_stop`移除PCB/部分timer但不取消已经发出的DNS请求；`sntp_dns_found`不使用hostname/arg判断本次请求，转而使用全局当前服务器并发送/重试。不能把停止后下一次同步的通知当作可靠请求关联。

`esp_netif_sntp_init/start/sync_wait`虽有部分检查式接口，但deinit仍返回void，且忽略TCPIP停止执行返回后释放自己的存储；仅改用该包装不能证明完整退出。没有修改SDK或固定厂家代码，也没有实际联网验证上述竞争。下一步继续审查应用拥有的解析请求/服务生命周期与失败保留边界，不宣称系统时钟/RTC变化可回滚。

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

现有 `esp_sntp_init/stop` 是 void 异步 lwIP入队接口：调用 stop 并不确认 SDK SNTP 已退出，不能取消已更新的系统时钟；通知也没有应用请求编号。上述串行化/门禁不是所有延迟回复关联或SDK关闭确认。NTP检查式保存和GET副本见本轮段落，但借用getter/SDK消费者、其他配置并发仍未完成；不宣称端到端SNTP服务、所有配置或整机退出通过。

## 验证

36项真实周期适配C + 真实后台包装C + 合成timer/FreeRTOS/NVS/clock测试；原18项后台测试兼容第五角色。覆盖重复init、创建/启动/worker失败、SDK异常句柄、取消/重入、stop/delete失败与确切句柄重试、timer仅排队、注册满表/键长、缺失与读取错误、输出不变、时钟/回退/极值、易失force/单任务、回调与open/set/commit失败锁存、回调/读取中取消及批次失败不继续。

新增3项Python核对main/power/HTTP/utils/CMake接入；完整普通/ASan各867个唯一用例、Python50+17=67通过。巨大main/HTTP/SNTP回调未在主机运行，模拟调度不是真实RTOS/RF/浏览器/SDK/NVS验收。固件最终编译及产物身份见 [VALIDATION](../../../VALIDATION.md) 末节。未烧录、联网或操作设备/测试卡/原片；产品/OTA/迁移门禁保持。

下一安全开发：SDK SNTP服务所有权/请求关联/关闭确认、NTP与其余配置setter的保存/并发、其余Web/文件消费者和元数据空间策略。真机恢复、睡醒、异常NVS及完整产品仍需另定范围与人工验收。

本轮新增17项真实config_manager C + 合成NVS测试：16项新板含真实双线程、1项旧板行为回归；针对性每项重复20次通过，完整普通/ASan各884唯一通过。新增2项Python核对utils/HTTP保存失败顺序与副本接入，共52+17=69。未主机执行真实main/HTTP/SNTP，ASan不是线程竞争检查，也不证明真实NVS/掉电/网络行为；无设备操作。
