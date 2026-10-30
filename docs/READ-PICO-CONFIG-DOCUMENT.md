# 配置文档接收与处理/调色板读写

进度、候选身份与下一步统一见 [TODO](../TODO.md)；逐轮测试、部署与失败记录见 [验证记录](VALIDATION.md)。整理前原文保留在 [历史快照](history/2026-10-08/docs/READ-PICO-CONFIG-DOCUMENT.md)。

## 文档和字段

Read Pico的general POST/PATCH、processing POST、palette POST使用同一便携接收器。正文1–4095字节，分配前检查长度；循环接收完整声明长度后才解析。提前EOF、SDK错误、返回零/超请求容量拒绝，八次**累计**超时返回结束接收，期间成功收字节不清空超时计数。没有新的SDK调用硬超时或整个请求绝对时间上限；部分接收缓冲必须丢弃，不解析其已有有效JSON前缀。新板不记录正文内容。

`config_document_parse`统一本地与远端文档检查：真实固定cJSON解析单一object、拒绝尾随文档、原始/转义NUL、16层以上嵌套、大小写等价重复键及非有限值；失败不改变输出tree指针，成功由调用方释放。cJSON无独立OOM结果，解析分配失败仍归无效文档；不宣称完整Unicode/RFC语法验证。远端配置头仍2048字节容量（含NUL），未扩大服务器契约；空远端输入仍无效响应，未知metadata/组内未知字段仍忽略。

processing/palette新增checked转换，先复制调用方候选，所有选中字段通过后才输出；缺失字段保留候选，HTTP/远端仍从原上游默认候选开始，因此不是改变成PATCH整个旧记录。processing七个数值须number、有限且可用float表示；五个文本不截断、只接受实际支持的tone/color/dither/scale/background枚举，compress须JSON bool。palette颜色组须object，RGB须0–255整数而非uint8绕回/截断，三个校准值须number/有限/float可表示。**不增加校准调优，不证明曝光范围、亮度端点顺序或gamma物理范围已全部校验**。原板沿用旧解析/接收策略。

## 读取、查询和响应

processing真实新板NVS load先创建原默认候选。仅namespace/key NOT_FOUND使用该字段默认；open/各get的IO、类型、长度错误不发布默认或部分值，关闭句柄后返回。检查七个浮点记录有限、五个字符串实际长度/结尾/嵌入NUL及bool记录0/1；不把所有历史文本枚举/数值范围视为已语义验收。成功才复制完整候选到调用方，失败输出不变，不自动擦除/迁移或修复NVS。NVS close是void，跨多键读取不具备掉电或全系统事务保证。

processing/palette两个HTTP GET也参与上一轮共享应用控制，避免与参与控制的远端/本地修改交错；不代表general整个GET快照或所有直接setter/SDK读者互斥。load错误返回失败，不在端点上再fallback。查询/修改成功响应检查type/send；保存、palette重载、版本及500发送错误传播，已进入处理器错误不重复响应。版本失败仍明确部分效果，保存或重载不能撤回。

两组serializer检查已知每次cJSON字段添加，任一失败不返回部分JSON，打印结果及候选终止/有限性也检查。新板palette默认初始化全结构为零以确定blob填充字节；所有原数值保持（black_y=.009、white_y=.65、gamma=1.42等），不是光学重新校准。旧板转换、加载和serializer原契约保留。

## 图像消费者与下载请求契约

### 全部自有初始请求头

将三配置头的remove/确认缺失/set/exact读回复用为`request_header_put_checked`。名称须非空ASCII token；值拒绝CR/LF、DEL与除HTAB外的控制字符。首尾空白不自动改值，固定SDK裁剪后精确读回不符即失败；内部制表符、空值及合法不透明缓存标记保持。调用者必须提供有效NUL终止文本，尚未证明所有borrowed getter的并发/生命周期或snprintf不截断。

Read Pico主图初始Authorization/custom、width/height/orientation、firmware/ETag/battery全用此检查，保留首个错误，后续header设置不再执行；三配置头也仅此前全部成功时设置。准备失败在perform之前清理client/终止owner，不按传输错误重试。缩略图同源认证/custom也检查，失败停止其请求、沿用可选缩略图false及文件保留契约，不因此否认已经成功的主图。原板保持旧set策略，不改变同源/认证优先级或厂家代码。

Read Pico不再记录未校验custom key，错误只记录错误名；未新增凭据输出。便携测试覆盖token/key每个非法字节、控制/换行value、空值/opaque值、缺失callback、逐操作失败；真实固定SDK测试覆盖首尾trim不符、内部HTAB/空值与认证副本/分配失败。实际utils主图/缩略图整链仍仅源码接入检查与固件编译，非真实服务器或redirect全栈验收。下载配置副本见下节，数量及身份见VALIDATION。

### 重定向的认证头删除与错误锁存

新板跨源跳转在安排下一请求之前调用`request_headers_strip_credentials`：先验证自定义名称，再删除Authorization并读回确认确实不存在，随后同样删除可选custom头；Authorization的大小写别名只处理一次。只有remove为OK/NOT_FOUND且get为NOT_FOUND并给出NULL才确认缺失，未知错误、仍存在或异常空值均停止后续操作。不调用set，不回滚部分删除，不在后续跳转恢复秘密。同源判断和原板行为保持。

主图与可选缩略图共享事件处理器，`redirect_error`保留首个错误。Location为空/缺失/超限、其副本分配失败、认证头删除未确认或SDK set_redirection失败均锁存owner失败；只有无错才调用set_redirection安排下一跳。固定SDK忽略该事件回调返回，所以perform返回后再次检查锁存结果，在文件finish与清理之前恢复主要错误，不让其被泛化为成功或普通下载失败。失败owner阻止主图传输重试，文件/ETag不追认；缩略图仍是可选失败。HEADERS_SENT不清除错误锁存。

便携协调器测试覆盖每步错误/不一致状态和部分删除；两项未改固定SDK header组件测试确认大小写删除、无分配、wire文本不含两个合成秘密，以及已有header更新失败留下NULL后仍能删除。它们没有执行整个SDK client或真实网络；事件接入/错误锁存目前只有源码检查和固件编译证据。URL origin解析、SDK内部认证/Host、响应字段截断、完整client退出及真实重定向链仍未完成审查，本节不是全部重定向安全验收。

### 下载配置所有权与检查式保存

绝对HTTP(S)重定向在调度前先保留SDK的HTTPS不降级门禁，再set_url、检查式生成/写入/读回Host，最后set_redirection。准备步骤自身不得发送请求，任一步失败不进入后续步骤、不承诺URL回滚。固定SDK最终set_url看到相同hostname，不再覆盖已确认Host；SDK升级需重新核对此假设。相对/网络路径保持原SDK解释和降级限制。此改动仍非完整HTTP client/多跳网络验收，不保证供应方所有内部OOM行为。

初始主图/缩略图的Host从目标URL生成，通过检查式remove/set/readback覆盖SDK初值：只省略HTTP的80或HTTPS的443，IPv6补方括号。自定义头名Host（大小写无关）拒绝，避免把传输路由字段当作凭据覆盖；失败不perform。绝对重定向另按上文准备流程处理，不能由初始检查推定真实多跳正确。

Read Pico在处理redirect前调用http_redirect_reference_valid：绝对HTTP(S)目标复用SDK端点校验，网络路径`//authority/path`临时加`http:`仅用于相同校验，拒绝userinfo/异常端口/主机及其他scheme；分配失败同样拒绝。无scheme/authority的相对引用保留，不做解析重写。原始引用与临时完整地址均须小于2048字节（网络路径因此含5字节校验前缀预算）。拒绝后锁存失败，不删除/重建头或安排下一跳。SDK本身对相对地址、HTTPS降级和Host处理仍独立约束，校验通过不意味着跳转一定被SDK接受，更不代表完整redirect链验收。

Read Pico将Location、X-Thumbnail-URL、ETag视为单值字段，同一请求响应中重复出现即owner失败；Location同时锁存redirect_error，在SDK安排下一跳前拒绝。固定SDK会将Location值追加到内部字符串，不能只校验回调最后一次值；因此即使重复值相同也不接受。新请求HEADERS_SENT重置三个seen标志，但不清除既有owner失败或redirect_error。此规则不等于完整HTTP响应状态机验收。

同源授权对完整绝对URL拒绝原始空白、控制字符和反斜杠，包含path/query/fragment；相对跳转保持同样限制。百分号编码保持原文，不在授权检查中自行解码。这不是完整URL语法或SDK解析一致性证明。证书错误文本通过有界输出函数写入，NULL/零容量不写，正容量包括终止NUL；固定文本不再使用容量减一的strncpy。

证书获取通过http_endpoint_parse复用固定SDK HTTP URL解析器，拒绝非HTTPS、userinfo、零端口、超长主机，不截断或atoi猜测端口；主机副本剥除IPv6外括号，与SDK解析字段一致。Read Pico general图片源入口用相同适配接受HTTP/HTTPS或空值，并按大小写无关的scheme决定是否获取证书。固定SDK不接受紧随authority的`#fragment`（需先有path/query），不自行修正。此契约不改变旧NVS载入，不覆盖完整重定向，也不代表TLS认证握手已验收。

Read Pico主图执行入口（含旧配置选出的URL和显式URL）及缩略图入口在日志、创建暂存文件和HTTP初始化前调用同一端点校验；主图无效返回失败，可选缩略图无效返回false。不会为此修改或擦除旧NVS。响应X-Thumbnail-URL与ETag分别必须完整容纳于512/HTTP_ETAG_MAX_LEN字节缓冲（含NUL）；超长或NULL拒绝，不发布截断前缀，并设置owner.failed，防止SDK忽略事件返回后仍finish成功。缺失头与空值沿用原契约；重复头策略、内容语义和完整redirect仍待核对。主机测试执行端点/helper/文件owner，utils整链仅接入检查与编译，非真实网络证明。

Read Pico 的 `config_manager_copy_download` 在共享配置锁内捕获 URL、token、自定义头名称/值、ETag 和证书；文本有界复制，证书独立分配，失败不修改输出。调用者持有副本直至主图重试、可选缩略图和显示流程结束，最后 release。HTTP general 读取也先捕获此副本。大结构使用受检查的堆分配，避免叠加到按钮任务栈；分配失败不开始下载或保存。

检查式 patch 拒绝超长文本、不一致证书长度及未知字段；复制调用者输入后才执行 NVS 写入，全部选中字段、ETag 失效和一次 commit 成功后才发布 RAM。失败不发布 RAM，但不声称 NVS 多键事务回滚。URL/凭据/证书身份改变清空 ETag；下载完成仅在身份仍匹配时接收 ETag，旧请求不能污染新配置。URL 与证书配对保存，兼容 void writer 失败仅记录错误且不截断后发布。

该副本不是整个 general/图像配置事务，也不替代 URL/TLS/头字段语义审查。主机测试直接执行配置管理器及合成 NVS/分配器；完整 utils/HTTP/TLS 链仍须实机验证。

### 三个配置请求头的一致副本

新板`request_config_capture`在共享应用控制内依次复制revision、load processing/palette，再调用真实checked serializer；失败不发布部分snapshot、释放已生成字符串，不把IO错误当默认。仅namespace/key缺失沿用上游默认。成功拥有24字节revision文本及两份JSON，释放使用cJSON allocator。参与的本地/远端应用不会在三个读取之间交错；所有直接setter/NVS写者及其余配置不在本保证内。

下载在响应缓冲分配/文件创建/HTTP client初始化之前捕获一次，三个传输重试复用同一副本，不重新读取。准备失败不开始下载；设置失败立即清理本次client/终止自有临时文件，不perform、不按传输错误重试，不更新本次ETag。304、响应缓冲分配失败及循环结束都释放自有JSON。新板最终错误返回保留主要错误，原板原默认化与重试契约保持。

`request_config_set_headers`对X-Config-Last-Updated/X-Processing-Settings/X-Color-Palette逐个删除、确认不存在、设置、读回exact value。固定SDK已存在header的update分支中，strdup失败仍返回ESP_OK并留下NULL；通过先删除并确认缺失避免该分支，新item分配错误返回失败。四项主机测试编译SHA锁定、未修改的SDK http_header/http_utils源码，真实复现NULL假成功、逐新item分配失败、SDK副本生命周期和compact JSON wire文本。这里只执行请求头组件/合成分配器，不是整个esp_http_client、socket/redirect或服务器行为。

### 图像选项消费者

Read Pico的`processing_settings_copy_options`在共享零等待应用控制内一次load，交出自有dither/scale/background副本。namespace/key缺失仍用原默认；IO/类型/长度/有限性错误或控制忙返回失败、不改变输出。三个选项只接受原支持枚举；历史未知或空字符串不再由这条路径默认为Floyd/Cover/白，不自动修复存储。保护仅覆盖参与的配置应用，不约束所有直接NVS writer。

直接上传显示与下载后的显示先检查该副本，失败不进入资产发布、相册派生复制或显示。上传503响应逐项检查status/type/send，保留主要错误或发送错误；下载走原错误返回/文件保留/不追认ETag分支。实际HTTP/utils端到端仍只编译与源码接入检查，不算主机行为或真实联网验收。

PNG内存显示和处理到文件在打开显示流/创建输出之前读取布局副本；流式和缓冲式geometry只使用传入副本，不再次分别读取scale/background。失败不刷新、无新增输出文件、既有捕获画面和输入保留。JPEG EXIF/隔离照片的固定Cover策略不改，不读取产品布局；诊断桥新增明确RAM-only默认副本，不链接NVS或产品应用控制。原板仍用旧便捷getter。

这是单次布局读取，不是整个图像/请求的配置事务：HTTP/utils选择dither与处理器布局读取是两个时间点，图像处理期间palette/方向及其他配置仍需完整快照设计；调用方显式dither不被布局副本覆盖。旧便捷API仍保留供原板/兼容使用，不能宣称所有读取调用都已检查。三个配置请求头及下载配置分别按上节捕获，整次图像快照仍待处理。

utils/HTTP整链未在host执行；接入检查/固件实际链接不能替代SDK网络、浏览器、真实NVS、掉电或全栈退出验收。ASan非TSan。最终数量、构建SHA和日志见[验证记录](VALIDATION.md)末节。

## 证据与剩余工作

逐轮接收/解析/真实settings/serializer、下载配置、固定SDK header组件和合成IO测试均见VALIDATION。utils/HTTP完整链路仍只做源码接入及固件编译，未验收真实服务器/NVS、所有字段语义/直接writer或整机退出。下一步以TODO为准：URL/TLS与响应字段边界/redirect、general字段与完整图像快照。
