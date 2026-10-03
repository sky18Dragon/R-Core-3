# Ricoh GR IV 固件 1.11 HTTP / WebSocket 接口分析

## 1. 结论摘要

本报告针对用户提供的 `gr4_v111.zip` 做静态逆向分析。压缩包内固件标识为 **GR IV 1.11.10.7**，固件中的 `webapid` 服务实现了：

- **24 个 HTTP 路由**；
- **1 个 WebSocket 路由**；
- 相机属性读取与设置、拍照和对焦、实时取景、照片浏览与传输、拍摄统计、会话结束、固件升级等能力。

服务默认监听 **`0.0.0.0:80`**。端口可由进程环境变量 `WEBAPID_PORT` 覆盖。程序中没有发现 TLS 配置，也没有发现 `Authorization`、Cookie、Bearer Token 或挑战应答式认证逻辑；因此从静态证据看，它是供相机本地 WLAN 连接使用的明文 HTTP API。这个判断不排除网络层、配网状态或另一个进程施加的访问限制。

最容易误判的一点是：绝大多数写接口的请求体 **不是 JSON**，而是未经 URL 解码的键值串：

```text
key=value&key2=value2
```

解析器先按 `&` 分项，再按第一个 `=` 分隔键和值。空 body、无 `=`、空 key 或空 value 都会被判为错误。

## 2. 分析对象与证据

| 项目 | 结果 |
|---|---|
| 输入文件 | `gr4_v111.zip` |
| ZIP SHA-256 | `ded47aa6385886e4b1f8f41f8551866ab0f1ecd0aef89934a43ef0e456539e8f` |
| 固件文件 | `fwdc248b.bin` |
| 机型 / 版本 | `GR IV` / `1.11.10.7` |
| 解码帧数 | 2,765 |
| 解码后大小 | 67,948,280 字节 |
| 根文件系统 | SquashFS v4、LZ4；标记 `INITFS` |
| HTTP 服务程序 | `/usr/bin/webapid`，32 位 ARM Thumb PIE ELF |
| HTTP 框架 / JSON 库 | Crow 0.1 / RapidJSON |
| 内部相机事件通道 | `unix:/tmp/webapid_camev.sock` |
| 默认监听 | `0.0.0.0:80` |

`/etc/rc.local` 会在存储挂载后启动 `webapid`。固件中其他可执行文件没有发现相机业务 HTTP 路由；`dbus-daemon` 和 `iperf` 中出现的 URL 只是标准文档或项目说明。因此下表基本可视为本固件的完整业务 HTTP 面。

## 3. 协议共性

### 3.1 基础地址

```text
http://<camera-ip>/v1/...
ws://<camera-ip>/v1/changes
```

### 3.2 请求参数位置

- URL 查询参数：普通 `?key=value&key2=value2`。
- 写接口 body：`key=value&key2=value2`，不是 JSON。
- 固件数据上传：唯一例外，`PUT /v1/configs/firmware` 的 body 是原始固件字节。
- 路径参数 `{dir}`、`{file}`：DCF 目录名和文件名，例如 `100RICOH`、`R0000001.JPG`。

### 3.3 通用返回与错误

普通响应使用 `application/json`，并设置禁止缓存的响应头。错误对象至少含：

```json
{
  "errCode": 400,
  "errMsg": "Bad Request"
}
```

固件升级等接口还可能增加 `detail` / `reason`。程序中定义了以下错误类别：

| HTTP/错误类别 | 含义 |
|---|---|
| 200 / `OK` | 成功 |
| 400 / `Bad Request` | 参数缺失、格式或枚举值错误 |
| 403 / `Forbidden` | 当前相机状态不允许执行 |
| 404 / `Not Found` | 文件、存储或资源不存在 |
| 412 / `Precondition Failed` | 前置条件不满足 |
| 500 / `Internal Server Error` | 内部处理失败 |
| 503 / `Service Unavailable` | 相机控制服务不可用或忙 |

固件升级细分原因包括 `battery_too_low`、`not_available`、`err_fw_same`、`err_fw_old`、`err_fw_broken`。

## 4. 完整路由清单

| # | 方法 | 路径 | 主要作用 |
|---:|---|---|---|
| 1 | GET | `/v1/debug/revisions` | 返回系统各组件 revision 信息 |
| 2 | GET | `/v1/ping` | 连通性检测，并返回相机时间等状态 |
| 3 | GET | `/v1/props` | 返回设备、相机、镜头、存储和实时取景属性 |
| 4 | PUT | `/v1/params/device` | 修改设备参数 |
| 5 | PUT | `/v1/params/camera` | 修改拍摄参数 |
| 6 | PUT | `/v1/params/lens` | 修改镜头参数 |
| 7 | POST | `/v1/camera/shoot` | 触发拍照 |
| 8 | POST | `/v1/lens/focus` | 在指定位置对焦 |
| 9 | POST | `/v1/camera/shoot/compose` | 开启/关闭构图微调 |
| 10 | POST | `/v1/camera/shoot/cancel` | 取消当前拍摄/定时拍摄 |
| 11 | GET | `/v1/liveview` | MJPEG 实时取景流 |
| 12 | GET | `/v1/photos` | 返回存储及 DCF 目录概览 |
| 13 | GET | `/v1/photos/infos` | 分页返回照片/视频元数据 |
| 14 | GET | `/v1/photos/{dir}/{file}` | 下载原图、缩略图或小图 |
| 15 | GET | `/v1/photos/{dir}/{file}/info` | 返回单个媒体的元数据 |
| 16 | PUT | `/v1/photos/{dir}/{file}/transfer` | 设置文件传输状态及传输尺寸 |
| 17 | GET | `/v1/photos/{dir}/{file}/imgctrl` | 读取照片 image-control 二进制数据 |
| 18 | GET | `/v1/transfers` | 查询传输队列 |
| 19 | GET | `/v1/logs/camera` | 查询月度或累计拍摄统计 |
| 20 | POST | `/v1/device/finish` | 结束远程设备会话 |
| 21 | POST | `/v1/device/wlan/finish` | 结束 WLAN 会话 |
| 22 | POST | `/v1/configs/firmware/prepare` | 校验条件并分配固件接收区 |
| 23 | PUT | `/v1/configs/firmware` | 上传固件原始字节 |
| 24 | POST | `/v1/configs/firmware/cancel` | 取消固件传输/升级准备 |
| 25 | WebSocket | `/v1/changes` | 推送属性、拍摄、存储、传输等变化 |

## 5. 各接口参数与返回说明

### 5.1 调试与状态

#### `GET /v1/debug/revisions`

- 参数：无。
- 作用：执行等价于读取 `/etc/revisions/*` 的操作，返回固件内各组件版本/revision。
- 返回：JSON；具体键由镜像内 revision 文件名决定。

#### `GET /v1/ping`

- 参数：无。
- 作用：连接保活/能力探测。
- 返回：通用成功字段以及 `datetime`；代码中 `Pong` 还会序列化当前设备状态信息。

#### `GET /v1/props`

- 参数：无。
- 作用：一次性读取可用能力、当前值和状态，是调用设置接口前最重要的能力发现接口。
- 主要返回字段：

| 分组 | 字段 |
|---|---|
| Device constants | `manufacturer`, `model`, `serialNo`, `firmwareVersion`, `macAddress`, `channelList` |
| Device params | `datetime`, `operationMode`, `stillFormat`, `geoTagging`, `gpsInfo`, `bdName`, `bleEnableCondition`, `channel`, `ssid`, `key`, `powerOffTransfer`, `autoResize` |
| Device status | `hot`, `battery`, `operationModeList`, `stillFormatList`, `storages` |
| Camera params | `stillSize`, `movieSize`, `captureMode`, `shootMode`, `onePushBracket`, `WBMode`, `exposureMode`, `meteringMode`, `av`, `tv`, `sv`, `xv`, `effect`, `cameraOrientation` |
| Camera status | `capturing`, `stateStill`, `stateMovie`, `countDown`, `shotsTotal`, `shotsCurrent`，以及各参数的 `...List` |
| Lens | `focusMode`, `focusSetting`, `focused`, `focusSettingList`, `focusEffectiveArea` |
| Live view | `liveState`, `resoList` |
| Storage | `equipped`, `writable`, `remain`, `recordableTime`, `numOfPhotos`, `numOfMovies` |

客户端应优先使用返回的 `...List` 作为当前机型、当前模式下的合法值集合，因为光圈、快门、ISO、拍摄模式等集合会随曝光模式和相机状态变化。

### 5.2 参数设置

以下三个接口的 body 都使用 `key=value&...`。多个字段可以在一次请求中设置；未知字段会被忽略或导致“未找到有效 key”，有效字段全部设置失败时返回错误。

#### `PUT /v1/params/device`

可写字段：

| 字段 | 类型/值 | 作用 |
|---|---|---|
| `datetime` | 字符串 | 设置机身日期时间；建议使用 `/v1/props` 当前值的同格式字符串 |
| `operationMode` | 枚举 | `playback`, `bleStartup`, `other` |
| `stillFormat` | 枚举 | `jpeg`, `dng`, `rawdng`, `pef`, `rawpef`；实际支持值以 `stillFormatList` 为准 |
| `geoTagging` | 布尔字符串 | `true` / `false` |
| `gpsInfo` | 字符串 | GPS 信息；格式应沿用 `/v1/props` 或媒体信息中的表示形式 |
| `bdName` | 字符串 | Bluetooth device name |
| `bleEnableCondition` | 枚举 | 发现值包括 `idle`, `busy2s`, `busy10s`, `busy12s` |
| `channel` | 整数/字符串 | WLAN 信道；合法范围取 `channelList` |
| `ssid` | 字符串 | WLAN SSID |
| `key` | 字符串 | WLAN 密钥 |
| `powerOffTransfer` | 布尔/枚举字符串 | 关机传输设置；应使用 `/v1/props` 返回值域 |
| `autoResize` | 枚举 | `disable`, `enableAnytime`, `enablePowerOn` |

示例 body：

```text
geoTagging=false&autoResize=disable
```

#### `PUT /v1/params/camera`

可写字段：

| 字段 | 类型/值 | 作用 |
|---|---|---|
| `stillSize` | 枚举 | 静态照片尺寸，如 `L3`, `M3`, `S3`, `XS3`, `L2`, `L1`, `M2`, `M1`, `S2`, `S1`, `XS2`, `XS1` |
| `movieSize` | 枚举 | `4K30`, `4K24`, `FHD60p`, `FHD30p`, `FHD25p`, `FHD24p`, `HD60p`, `HD30p`, `HD25p`, `HD24p` |
| `captureMode` | 枚举 | 例如 `still` / `movie`；以返回的列表为准 |
| `shootMode` | 枚举 | 单拍、自拍、遥控、连拍、间隔、多重曝光、包围、反光镜预升、星流等，见后文枚举 |
| `WBMode` | 枚举 | 白平衡模式 |
| `exposureMode` | 枚举 | 发现 `TV`, `AV`, `BT`, `SFP`, `U0`～`U5` 等值 |
| `meteringMode` | 枚举 | `multi`, `center`, `highlight` |
| `av` | 字符串/数值 | 光圈；使用 `avList` 中的值 |
| `tv` | 字符串/数值 | 快门；使用 `tvList` 中的值 |
| `sv` | 字符串/数值 | ISO；含 `auto`, `auto_hi` 或数值，使用 `svList` |
| `xv` | 字符串/数值 | 曝光补偿；使用 `xvList` |
| `effect` | 枚举 | 图像风格/滤镜，见后文枚举 |

`onePushBracket` 和 `cameraOrientation` 会出现在属性返回中，但未进入普通 GR IV 的可写 setter map，应按只读字段处理。

#### `PUT /v1/params/lens`

- 可写字段：`focusSetting`。
- 已发现的合法值：`manual`, `snap`, `inf`, `multiauto`, `spot`, `pinpoint`, `tracking`, `continuous`, `multiauto_center`, `zone_select`；实际值域取 `focusSettingList`。
- `focusMode` 会返回，但没有进入该版本 setter map，应按只读字段处理。

示例 body：

```text
focusSetting=spot
```

### 5.3 拍摄、对焦与实时取景

#### `POST /v1/camera/shoot`

- body 字段：
  - `pos=x,y`：可选的对焦点，两个十进制整数用逗号分隔；
  - `af=setting|reset`：快门/AF 动作模式。
- 返回：通用结果；成功拍摄时包含 `captureId`。
- 示例：`pos=500,500&af=setting`。

坐标的实际范围与坐标原点没有在字符串/符号中直接给出，应结合 `focusEffectiveArea` 做归一化或真机验证，不能假定为像素坐标。

#### `POST /v1/lens/focus`

- body：`pos=x,y`。
- 作用：仅执行指定位置对焦，不触发完整拍照。
- 返回：通用结果；部分成功路径同样使用含 `captureId` 的响应对象。

#### `POST /v1/camera/shoot/compose`

- body：`setting=true` 或 `setting=false`。
- 作用：开启或关闭构图微调/compose adjustment；实现内部会在关闭或异常时执行 reset。

#### `POST /v1/camera/shoot/cancel`

- 参数：无。
- 作用：取消正在进行的拍摄、倒计时或相关 timer。

#### `GET /v1/liveview`

查询参数：

| 参数 | 必需 | 格式 | 说明 |
|---|---|---|---|
| `reso` | 否 | `宽x高` | 两个无符号十进制数；服务会校验是否是当前相机支持的分辨率 |
| `maxfps` | 否 | `1`～`90` | 最大帧率；非数字或大于 90 会报错 |

返回不是 JSON，而是：

```text
Content-Type: multipart/x-mixed-replace; boundary=--boundary
```

每一帧使用 `Content-Type: image/jpeg`。连接关闭时服务会退出远程 Live View 模式。

### 5.4 照片、视频与传输

#### `GET /v1/photos`

- 参数：无。
- 作用：返回各存储的 DCF 目录概览。
- 返回字段包括 `dirs`，以及存储状态 `equipped`, `writable`, `remain`, `recordableTime`, `numOfPhotos`, `numOfMovies`。

#### `GET /v1/photos/infos`

| 查询参数 | 必需 | 格式/默认 | 说明 |
|---|---|---|---|
| `storage` | 否 | `sd1`, `sd2`, `head` | 不给时使用服务选择的可用存储 |
| `after` | 否 | `<8字符目录>/<12字符文件>` | DCF 游标，例如 `100RICOH/R0000001.JPG`；固定总长度 21，不含 `DCIM/` |
| `limit` | 否 | 非负整数；缺省内部值为 0 | 返回数量限制；0 在下游表示默认/不额外限制 |

每个媒体对象可包含：`cameraModel`, `dir`, `file`, `datetime`, `orientation`, `aspectRatio`, `av`, `tv`, `sv`, `xv`, `gpsInfo`, `duration`, `resolution`。静态照片主要使用曝光/GPS 字段，视频主要使用 `duration`、`resolution`。

#### `GET /v1/photos/{dir}/{file}`

| 查询参数 | 必需 | 值 | 说明 |
|---|---|---|---|
| `storage` | 否 | `sd1`, `sd2`, `head` | 源存储 |
| `size` | 否 | `full`, `xs`, `thumb` | 原文件、小图或缩略图 |

响应 Content-Type 按媒体类型选择：`image/jpeg`、`image/x-adobe-dng` 或 `application/octet-stream`。

#### `GET /v1/photos/{dir}/{file}/info`

- 查询参数：可选 `storage=sd1|sd2|head`。
- 作用：返回单个媒体的上述元数据对象。

#### `PUT /v1/photos/{dir}/{file}/transfer`

- body：`status=<状态>&size=<尺寸>`。
- `status` 值域：`all`, `untransferred`, `transferring`, `transferred`；设置单文件时有实际意义的通常是 `untransferred` 或 `transferred`。
- `size` 值域：`full`, `xs`, `thumb`。
- 作用：设置该文件的传输状态/类型。服务会根据 DCF 路径解析并定位存储。

#### `GET /v1/photos/{dir}/{file}/imgctrl`

- 查询参数：可选 `storage=sd1|sd2|head`。
- 返回：相机内部 image-control 数据，二进制响应；日志会记录数据地址与大小。

#### `GET /v1/transfers`

| 查询参数 | 必需 | 格式/默认 | 说明 |
|---|---|---|---|
| `status` | 否 | `all`, `untransferred`, `transferring`, `transferred` | 过滤传输状态；不是 `transferState` |
| `after` | 否 | 非负整数，默认 0 | 按传输条目的 `index` 翻页 |
| `limit` | 否 | 非负整数，默认/上限路径为 5000 | 最大返回条数 |
| `storage` | 否 | `sd1`, `sd2`, `head` | 过滤存储 |

返回列表条目至少含 `index` 和 `filepath`。

### 5.5 拍摄统计

#### `GET /v1/logs/camera`

| 查询参数 | 必需 | 值 | 说明 |
|---|---|---|---|
| `type` | 是 | `monthly_capture`, `total_capture` | 统计类型 |
| `date` | 月度时是 | `YYYY-MM` | 月份；会校验格式和年月范围 |

- `monthly_capture` 返回 `date` 和 `dailyData`。
- `total_capture` 返回 `startDate` 和 `totalData`。
- 明细结构包含 `count`，以及按 `exposure`、`effect`、`crop_size`、`aspect_ratio`、`still`、`video` 等维度汇总的数据。

### 5.6 会话结束

#### `POST /v1/device/finish`

- 参数：无。
- 作用：通知相机结束当前远程设备控制会话。

#### `POST /v1/device/wlan/finish`

- 参数：无。
- 作用：结束 WLAN 远程会话，可能导致客户端立即断线。

### 5.7 固件升级

> 高风险：这些接口能够改变相机固件状态。静态分析不能保证所有安全前置条件都已完整识别；不要用非官方固件或在低电量、供电不稳定时测试。

调用顺序：

1. `POST /v1/configs/firmware/prepare`
2. `PUT /v1/configs/firmware`
3. 等待结果；需要中止时调用 cancel

#### `POST /v1/configs/firmware/prepare`

- body：`filesize=<无符号十进制字节数>`。
- 作用：检查升级条件、让底层相机控制服务创建固件对象，并按给定大小分配/准备接收缓冲区。
- 如果已有升级传输进行中，会返回错误。

#### `PUT /v1/configs/firmware`

- body：原始固件二进制，不是键值串或 JSON。
- `Content-Length` 应与 prepare 的 `filesize` 一致。
- 作用：把收到的数据保存/转交给底层固件更新流程。

#### `POST /v1/configs/firmware/cancel`

- 参数：无。
- 作用：取消正在进行的固件传输并释放相关状态。WebSocket 断开也会触发固件更新取消逻辑。

### 5.8 WebSocket 变化推送

#### `ws://<camera-ip>/v1/changes`

- 参数：无。
- 作用：持续推送相机状态变化。
- 已识别事件类别/字段：
  - 属性变化：`changed`；
  - 拍摄直传：`cameraDirect`, `result`, `numOfFiles`；
  - 镜头变化：`lens`；
  - 存储变化：`storage`, `action`, `filepath`, `captureId`；
  - 传输变化：`transfer`, `action`。
- 固件模块注册了 WebSocket 连接状态回调；升级期间断开连接会取消升级传输。

## 6. 主要枚举值

这些字符串来自固件转换表。最终应以 `/v1/props` 的当前 `...List` 为准。

### 白平衡 `WBMode`

`multiAuto`, `daylight`, `shade`, `cloud`, `daylightFluorescent`, `dayWhiteFluorescent`, `coolWhiteFluorescent`, `warmWhiteFluorescent`, `tungsten`, `incandescent`, `cte`, `colorTemp1`, `colorTemp2`, `colorTemp3`, `manual1`, `manual2`, `manual3`, `autoWarm`, `autoWhite`。

### 效果 `effect`

`col_vivid`, `efc_monochrome`, `efc_softMonochrome`, `efc_hardMonochrome`, `efc_highContrast`, `efc_posiFilm`, `efc_bleachBypass`, `efc_retro`, `efc_HDRTone`, `col_custom1`, `col_custom2`, `col_custom3`, `efc_negaFilm`, `efc_cineYellow`, `efc_cineGreen`, `efc_crossProcess`, `efc_monoStandard`, `efc_monoSoft`, `efc_monoHighContrast`, `efc_monoGrainy`, `efc_monoSolid`, `efc_monoHDRTone`。

### 拍摄方式 `shootMode`

- 基本：`single`, `self2s`, `self10s`, `self12s`, `remocon`, `remocon3s`。
- 连拍：`continuousH/M/L` 及相应 `Self2s`, `Self12s`, `Remocon`, `Remocon3s` 组合。
- 间隔：`interval`、`intervalComp`、`intervalMovie` 及相应自拍/遥控组合。
- 多重曝光：`multiExp`, `multiExpContinuous` 及相应自拍/遥控组合。
- 包围：`bracket`, `motionBracket`, `dofBracket` 及相应自拍/遥控组合。
- 其他：`mirrorUp`, `starstream` 及相应自拍/遥控组合。

### 方向与比例

- `cameraOrientation`：`none`, `positive`, `reverse`, `vertical_right`, `vertical_left`。
- 画幅比例：`1:1`, `3:2`, `4:3`, `16:9`。
- Crop：`Off`, `35mm`, `50mm`。

## 7. 安全与实现注意事项

1. **没有应用层认证迹象**：同一相机 WLAN 内能访问 80 端口的客户端，静态上看可能直接调用控制、关 WLAN 和固件接口。客户端应把相机网络视为可信本地链路，不应把端口转发到互联网。
2. **明文传输**：照片、控制参数和固件上传均未见 TLS。
3. **固件接口影响最大**：`prepare` + `PUT` 是真实写入路径，错误文件、断连或供电异常可能造成设备不可用。
4. **写接口不是 JSON**：发送 JSON 会落入 key/value 解析错误。
5. **能力是动态的**：光圈、快门、ISO、效果、分辨率等必须先读 `/v1/props`，不要只使用本报告枚举硬编码。
6. **静态分析边界**：接口、字段名、解析方式和主要枚举有直接二进制证据；坐标系、部分字段业务语义、所有状态机前置条件仍需真机抓包或隔离环境动态验证。

## 8. 置信度说明

- **高置信度**：路由、HTTP 方法、查询键/body 键、`&`/`=` 解析格式、主要枚举、默认端口、MJPEG 格式、固件上传顺序。
- **中置信度**：部分返回对象的条件字段、`head` 存储的物理含义、`powerOffTransfer` 的具体值域、对焦坐标单位。
- **待动态验证**：每个接口在不同相机模式下的完整状态机、HTTP 状态码与所有底层错误的一一对应、固件升级成功后的重启行为。

