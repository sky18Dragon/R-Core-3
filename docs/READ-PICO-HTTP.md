# Read Pico普通HTTP与配网服务资源边界

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-HTTP.md)。

## 共享资源与SDK契约

`read_pico_http_service`是Read Pico的HTTP SDK生命周期适配，不拥有相册/配置/NVS/产品业务状态。普通页面和配网页面使用同一端口80预约，显式FRAME/PROVISION调用方角色；只覆盖已接入的这两个入口，不给未审查的直接SDK消费者作互斥保证，也不是处理器的权限认证。

claim在任一SDK调用之前保留端口；start要求角色相符、已有预约、未持有句柄，检查SDK创建和路由回调，完整注册才发布ACTIVE。stop先关闭请求就绪，使用确切自有句柄；SDK退出成功才清句柄，但仍保留角色，release仅在没有句柄且无不确定故障时清占用。原子控制拒绝同时操作或SDK/注册回调重入，不持有一个可跨任务误交接的FreeRTOS mutex。

固定IDF `httpd_start`仅成功才发布输出句柄，失败输出非空/成功输出为空均按契约不确定锁存，不盲目关闭未知对象；路由失败已取得确切句柄，留给调用方检查式stop。固定SDK stop失败位于释放之前，可显式再用同一确切句柄检查；不会自动循环或重新start覆盖。SDK内部close/线程回收的全部失败未被公开返回值证明，stop含无界信号量/THREAD_STOPPED等待，本适配不发明硬取消或总时限。

## 普通页面

原46个URI/方法、handler指向、auth_gate和可选密码逻辑保留，不修改页面布局。注册帮助函数接收本次确切句柄，Read Pico逐项传播失败；原板仍忽略注册错误，保留原行为，不宣称已回归真实旧板网络。

普通入口自身原子start/stop控制，已ACTIVE重复init成功而不再创建/重注册或覆盖句柄。新claim失败不清理别人的预约；自己创建或注册失败保留首错，并检查自己stop/release，未确认退出不新建。就绪必须在完整注册后显式标记，关闭/失败后不能再mark-ready；新start不继承旧就绪。

Read Pico所有注册路由在认证trampoline先检查服务ACTIVE、显式就绪及请求所属SDK句柄，未就绪返回503，再进入原认证/handler；既有业务处理器的ready检查也接到本次请求。关闭失败的仍存对象不能继续新配置/上传请求。检查之后已经开始的handler仍可能执行，不是SDK/NVS/文件写入回滚或全程硬取消；Basic认证仍非明文网络上的机密保护。

main检查HTTP init、各必需后台创建及ready发布，全部成功才放行业务并报告启动成功；失败合作取消受管回调并检查本角色HTTP停止。wake窗口检查服务helper返回，相关mDNS错误传播但不保证完整mDNS清理。在线通知先检查HTTP就绪和当前IP，不能据此宣称完整并发启动/整机退出验收。

## 配网入口

AP控制门禁先claim HTTP，再取得manager当前调用的无线控制；FRAME占用时不停止无线、不改模式、不取消外国/原保存ticket。manager begin未成功则释放自己尚未用SDK的HTTP预约；异常未释放保留占用供显式处理，不能凭零owner猜测无线控制。

AP仍按固定流程设置AP/DHCP、调用共享HTTP start注册原15个路由/404、启动DNS，然后开放session。关闭仍是取消session、manager quiesce、DNS→HTTP→radio检查，HTTP已关也保留预约至radio退出确认，避免普通服务抢先进入未完成的AP退出阶段。射频/DNS/HTTP失败保留预约及未确认对象；idle AP stop不释放FRAME预约。

## 验证与未验收范围

普通巨大HTTP模块/实际路由与main未在主机运行；两项Python源码断言核对46个完整URI/方法、认证指向/就绪顺序及接入，六配置固件编译确认API类型和链接，不冒充真实HTTP或main业务验收。SDK等待/句柄为替身，role查询使用原子字段，不宣称多字段硬原子快照或已开始请求自动撤销。

共享资源/AP接入主机测试覆盖预约、角色、句柄/路由失败、异常SDK输出、重入、就绪撤销和确切句柄停止。后台、周期与SNTP已有独立适配，真实浏览器/AP、SDK端口退出、断线/重启和完整清理仍待验收，优先级见TODO。
