> **历史快照：2026-10-08文档整理前。** 来源 `docs/READ-PICO-CONFIG-DOCUMENT.md`，原文字节SHA256 `eb39089e1577afba4ae3bec2d77d1778fbdf96882a788ff12698f0f55d6dd740`。以下“当前/最新/下一步/许可”只描述当时；不作实时状态或新操作指令。原文完整保留，仅相对Markdown链接为快照目录重定位。当前任务见 [TODO](../../../../TODO.md)。

# 配置文档接收与处理/调色板读写

2026-10-08，仅软件。沿用PhotoFrame现有配置、默认数值、字段名、HTTP路由和三组远端结构；不调整当前设备参数、厂家波形、VCOM、gamma或频率，不操作NVS/SD/原片。产品/OTA/迁移运行门禁保持。

## 文档和字段

Read Pico的general POST/PATCH、processing POST、palette POST使用同一便携接收器。正文1–4095字节，分配前检查长度；循环接收完整声明长度后才解析。提前EOF、SDK错误、返回零/超请求容量拒绝，八次**累计**超时返回结束接收，期间成功收字节不清空超时计数。没有新的SDK调用硬超时或整个请求绝对时间上限；部分接收缓冲必须丢弃，不解析其已有有效JSON前缀。新板不记录正文内容。

`config_document_parse`统一本地与远端文档检查：真实固定cJSON解析单一object、拒绝尾随文档、原始/转义NUL、16层以上嵌套、大小写等价重复键及非有限值；失败不改变输出tree指针，成功由调用方释放。cJSON无独立OOM结果，解析分配失败仍归无效文档；不宣称完整Unicode/RFC语法验证。远端配置头仍2048字节容量（含NUL），未扩大服务器契约；空远端输入仍无效响应，未知metadata/组内未知字段仍忽略。

processing/palette新增checked转换，先复制调用方候选，所有选中字段通过后才输出；缺失字段保留候选，HTTP/远端仍从原上游默认候选开始，因此不是改变成PATCH整个旧记录。processing七个数值须number、有限且可用float表示；五个文本不截断、只接受实际支持的tone/color/dither/scale/background枚举，compress须JSON bool。palette颜色组须object，RGB须0–255整数而非uint8绕回/截断，三个校准值须number/有限/float可表示。**不增加校准调优，不证明曝光范围、亮度端点顺序或gamma物理范围已全部校验**。原板沿用旧解析/接收策略。

## 读取、查询和响应

processing真实新板NVS load先创建原默认候选。仅namespace/key NOT_FOUND使用该字段默认；open/各get的IO、类型、长度错误不发布默认或部分值，关闭句柄后返回。检查七个浮点记录有限、五个字符串实际长度/结尾/嵌入NUL及bool记录0/1；不把所有历史文本枚举/数值范围视为已语义验收。成功才复制完整候选到调用方，失败输出不变，不自动擦除/迁移或修复NVS。NVS close是void，跨多键读取不具备掉电或全系统事务保证。

processing/palette两个HTTP GET也参与上一轮共享应用控制，避免与参与控制的远端/本地修改交错；不代表general整个GET快照或所有直接setter/SDK读者互斥。load错误返回失败，不在端点上再fallback。查询/修改成功响应检查type/send；保存、palette重载、版本及500发送错误传播，已进入处理器错误不重复响应。版本失败仍明确部分效果，保存或重载不能撤回。

两组serializer检查已知每次cJSON字段添加，任一失败不返回部分JSON，打印结果及候选终止/有限性也检查。新板palette默认初始化全结构为零以确定blob填充字节；所有原数值保持（black_y=.009、white_y=.65、gamma=1.42等），不是光学重新校准。旧板转换、加载和serializer原契约保留。

## 证据与下一步

### 2026-10-08追加：其余初始请求头

将三配置头的remove/确认缺失/set/exact读回复用为`request_header_put_checked`。名称须非空ASCII token；值拒绝CR/LF、DEL与除HTAB外的控制字符。首尾空白不自动改值，固定SDK裁剪后精确读回不符即失败；内部制表符、空值及合法不透明缓存标记保持。调用者必须提供有效NUL终止文本，尚未证明所有borrowed getter的并发/生命周期或snprintf不截断。

Read Pico主图初始Authorization/custom、width/height/orientation、firmware/ETag/battery全用此检查，保留首个错误，后续header设置不再执行；三配置头也仅此前全部成功时设置。准备失败在perform之前清理client/终止owner，不按传输错误重试。缩略图同源认证/custom也检查，失败停止其请求、沿用可选缩略图false及文件保留契约，不因此否认已经成功的主图。原板保持旧set策略，不改变同源/认证优先级或厂家代码。

Read Pico不再记录未校验custom key，错误只记录错误名；未新增凭据输出。五新增便携测试覆盖token/key每个非法字节、控制/换行value、空值/opaque值、缺失callback、逐操作失败；两新增真实固定SDK测试覆盖首尾trim不符、内部HTAB/空值与认证副本/分配失败。实际utils主图/缩略图整链仍仅源码接入检查与固件编译，非真实服务器、credential/URL/CA快照或redirect全栈验收。数量及身份见VALIDATION末节。

下一安全开发仍为凭据/URL/证书自有副本与checked保存/并发、redirect、完整图像配置及其余产品边界。仅初始自有header设置已检查，不把SDK内部Host/User-Agent/后续认证和重定向header都算入本次闭环。

### 2026-10-08追加：图片下载的三个配置请求头

新板`request_config_capture`在共享应用控制内依次复制revision、load processing/palette，再调用真实checked serializer；失败不发布部分snapshot、释放已生成字符串，不把IO错误当默认。仅namespace/key缺失沿用上游默认。成功拥有24字节revision文本及两份JSON，释放使用cJSON allocator。参与的本地/远端应用不会在三个读取之间交错；所有直接setter/NVS写者及其余配置不在本保证内。

下载在响应缓冲分配/文件创建/HTTP client初始化之前捕获一次，三个传输重试复用同一副本，不重新读取。准备失败不开始下载；设置失败立即清理本次client/终止自有临时文件，不perform、不按传输错误重试，不更新本次ETag。304、响应缓冲分配失败及循环结束都释放自有JSON。新板最终错误返回保留主要错误，原板原默认化与重试契约保持。

`request_config_set_headers`对X-Config-Last-Updated/X-Processing-Settings/X-Color-Palette逐个删除、确认不存在、设置、读回exact value。固定SDK已存在header的update分支中，strdup失败仍返回ESP_OK并留下NULL；通过先删除并确认缺失避免该分支，新item分配错误返回失败。四项主机测试编译SHA锁定、未修改的SDK http_header/http_utils源码，真实复现NULL假成功、逐新item分配失败、SDK副本生命周期和compact JSON wire文本。这里只执行请求头组件/合成分配器，不是整个esp_http_client、socket/redirect或服务器行为。

新增13项真实settings/NVS/serializer及便携请求头协调测试、4项上述固定SDK组件测试，1项Python接入检查；最终数量/日志/构建身份见VALIDATION末节。SDK cleanup对非NULL client返回ESP_OK但内部忽略close/transport返回；外层检查返回不证明完整退出ACK（本新分支在perform之前），不修改固定SDK。

仍待处理Authorization/custom header、width/height/orientation、firmware/ETag/battery等其余header的读取/设置和凭据/URL/证书生命周期，redirect重建、完整请求/图像/配置快照及全部字段语义。历史未知枚举的JSON上报不因本轮全部成为语义合法；revision copy实际配置管理有独立控制，snapshot测试使用合成revision getter，不替代真实general全栈并发验收。产品/OTA/迁移门禁保持，无参数调整或实机操作。

### 2026-10-08追加：图像选项消费者

Read Pico的`processing_settings_copy_options`在共享零等待应用控制内一次load，交出自有dither/scale/background副本。namespace/key缺失仍用原默认；IO/类型/长度/有限性错误或控制忙返回失败、不改变输出。三个选项只接受原支持枚举；历史未知或空字符串不再由这条路径默认为Floyd/Cover/白，不自动修复存储。保护仅覆盖参与的配置应用，不约束所有直接NVS writer。

直接上传显示与下载后的显示先检查该副本，失败不进入资产发布、相册派生复制或显示。上传503响应逐项检查status/type/send，保留主要错误或发送错误；下载走原错误返回/文件保留/不追认ETag分支。实际HTTP/utils端到端仍只编译与源码接入检查，不算主机行为或真实联网验收。

PNG内存显示和处理到文件在打开显示流/创建输出之前读取布局副本；流式和缓冲式geometry只使用传入副本，不再次分别读取scale/background。失败不刷新、无新增输出文件、既有捕获画面和输入保留。JPEG EXIF/隔离照片的固定Cover策略不改，不读取产品布局；诊断桥新增明确RAM-only默认副本，不链接NVS或产品应用控制。原板仍用旧便捷getter。

这是单次布局读取，不是整个图像/请求的配置事务：HTTP/utils选择dither与处理器布局读取是两个时间点，图像处理期间palette/方向及其他配置仍需完整快照设计；调用方显式dither不被布局副本覆盖。旧便捷API仍保留供原板/兼容使用，不能宣称所有读取调用都已检查。图片下载**请求头**的processing/palette load默认化、serializer/header返回及revision一致快照仍待下一增量。

新增8项真实settings C+合成NVS测试（原默认、全部三选项组合、逐get/open失败不发布、非法历史枚举、忙/重入控制与释放、独立副本），5项真实Read Pico image_processor+合成settings/display测试（PNG失败/忙/重试、输出前拒绝/原文件保持、单次fit背景副本、旧捕获画面保持、JPEG固定Cover不读取）。一项Python仅验证接入顺序和RAM诊断边界。不是真实NVS/屏幕、全部跨线程读写或掉电验收；完整数量与构建身份见VALIDATION末节。

新增14个便携接收/真实cJSON文档测试，17个真实processing/palette C+合成NVS/固定cJSON测试，2个Python源码接入检查。覆盖分段、最大长度/溢出、有效前缀EOF、累计超时、原始/转义NUL、深度、重复、解析/序列化各分配点，逐读取错误、无部分输出、类型/截断/枚举/RGB以及默认blob字节确定性。

utils/HTTP整链未在host执行；接入检查/固件实际链接不能替代SDK网络、浏览器、真实NVS、掉电或全栈退出验收。ASan非TSan。最终数量、构建SHA和日志见[验证记录](../../../VALIDATION.md)末节。

1105/1118增量的“便捷消费者/三个配置请求头待处理”按历史保留，最新追加节与VALIDATION末节为当前范围。仍需处理其余请求头/凭据/URL/证书/redirect、general void setter/字段、直接写者及整次图像快照；再继续RTC/调度/文件/空间和全栈退出。当前没有新硬件验证、烧写或光学结论。
