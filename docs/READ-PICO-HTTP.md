# Read Pico普通HTTP与配网服务资源边界

2026-10-08后续：[四类后台门禁](READ-PICO-BACKGROUND.md)现已接入main/power，普通启动在检查式任务创建完成后发布HTTP ready，失败请求合作取消并检查HTTP stop。下文813/61及“此前button已启动”的描述属于上一轮，仅保留历史；周期timer、mDNS和整机清理仍未完成，以VALIDATION末节为准。

日期：2026-10-08。沿用PhotoFrame普通页面、REST、可选Basic认证与配网入口；仅软件开发与验证，未启动设备网络或烧录。完整产品入口、OTA和迁移门禁保持。

## 共享资源与SDK契约

`read_pico_http_service`是Read Pico的HTTP SDK生命周期适配，不拥有相册/配置/NVS/产品业务状态。普通页面和配网页面使用同一端口80预约，显式FRAME/PROVISION调用方角色；只覆盖已接入的这两个入口，不给未审查的直接SDK消费者作互斥保证，也不是处理器的权限认证。

claim在任一SDK调用之前保留端口；start要求角色相符、已有预约、未持有句柄，检查SDK创建和路由回调，完整注册才发布ACTIVE。stop先关闭请求就绪，使用确切自有句柄；SDK退出成功才清句柄，但仍保留角色，release仅在没有句柄且无不确定故障时清占用。原子控制拒绝同时操作或SDK/注册回调重入，不持有一个可跨任务误交接的FreeRTOS mutex。

固定IDF `httpd_start`仅成功才发布输出句柄，失败输出非空/成功输出为空均按契约不确定锁存，不盲目关闭未知对象；路由失败已取得确切句柄，留给调用方检查式stop。固定SDK stop失败位于释放之前，可显式再用同一确切句柄检查；不会自动循环或重新start覆盖。SDK内部close/线程回收的全部失败未被公开返回值证明，stop含无界信号量/THREAD_STOPPED等待，本适配不发明硬取消或总时限。

## 普通页面

原46个URI/方法、handler指向、auth_gate和可选密码逻辑保留，不修改页面布局。注册帮助函数接收本次确切句柄，Read Pico逐项传播失败；原板仍忽略注册错误，保留原行为，不宣称已回归真实旧板网络。

普通入口自身原子start/stop控制，已ACTIVE重复init成功而不再创建/重注册或覆盖句柄。新claim失败不清理别人的预约；自己创建或注册失败保留首错，并检查自己stop/release，未确认退出不新建。就绪必须在完整注册后显式标记，关闭/失败后不能再mark-ready；新start不继承旧就绪。

Read Pico所有注册路由在认证trampoline先检查服务ACTIVE、显式就绪及请求所属SDK句柄，未就绪返回503，再进入原认证/handler；既有业务处理器的ready检查也接到本次请求。关闭失败的仍存对象不能继续新配置/上传请求。检查之后已经开始的handler仍可能执行，不是SDK/NVS/文件写入回滚或全程硬取消；Basic认证仍非明文网络上的机密保护。

main普通启动检查init及ready返回，不报PhotoFrame成功或创建之后的late Wi-Fi任务；相关wake两处检查helper结果，不报服务可用后继续该分支。检查相关mDNS启动错误，但未实现mDNS失败完整清理；此前已启动的button/periodic/板级/SD等对象不由这个return自动退出。在线通知前检查HTTP已就绪和原有当前IP核对，不能将其当完整并发启动/整机退出验收。

## 配网入口

AP控制门禁先claim HTTP，再取得manager当前调用的无线控制；FRAME占用时不停止无线、不改模式、不取消外国/原保存ticket。manager begin未成功则释放自己尚未用SDK的HTTP预约；异常未释放保留占用供显式处理，不能凭零owner猜测无线控制。

AP仍按固定流程设置AP/DHCP、调用共享HTTP start注册原15个路由/404、启动DNS，然后开放session。关闭仍是取消session、manager quiesce、DNS→HTTP→radio检查，HTTP已关也保留预约至radio退出确认，避免普通服务抢先进入未完成的AP退出阶段。射频/DNS/HTTP失败保留预约及未确认对象；idle AP stop不释放FRAME预约。

## 验证与未验收范围

新增13项CTest：真实共享HTTP适配配合合成SDK11；真实AP适配接真实共享HTTP适配、合成manager/radio/DNS/SDK2。原24项AP用例也改为通过真实共享HTTP实现；manager原用例补重复初始化/接口阶段断言，没有新增计数。覆盖预约/角色/重入、唯一创建、就绪/请求句柄、注册错误、SDK异常输出、不确定锁存、同句柄停止重试、AP先查占用、完整射频退出才释放与manager忙后的预约释放。

普通巨大HTTP模块/实际路由与main未在主机运行；两项Python源码断言核对46个完整URI/方法、认证指向/就绪顺序及接入，六配置固件编译确认API类型和链接，不冒充真实HTTP或main业务验收。SDK等待/句柄为替身，role查询使用原子字段，不宣称多字段硬原子快照或已开始请求自动撤销。

普通CTest与ASan/UBSan各813个唯一用例、Python44+17共61、六配置编译通过；集成1,907,456字节/SHA256 `69432b505036b782ce520981ba2871c3774b6c5c640e537aedeef846cb94ba58`仅编译未部署。构建清单重复登记问题、失败日志和最终唯一计数见[验证记录](VALIDATION.md)末节。最后已知设备仍c876599f…SD诊断/STOPPED，非本轮实时观察；无设备/测试卡/原片/NVS/真实网络或提交推送操作。

下一安全软件工作是后台任务创建/生命周期与就绪发布、其他Web配置/文件消费者/元数据空间策略。真实浏览器/AP/SDK端口退出、断线/重启/并发与完整产品需要另定范围及人工验收；不扩大测试卡的factory诊断许可。
