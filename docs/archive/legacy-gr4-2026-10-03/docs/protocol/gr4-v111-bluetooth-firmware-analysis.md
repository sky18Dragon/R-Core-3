# GR4 固件 v1.11 蓝牙功能与参数说明

## 1. 文档范围

本文档只分析当前工作区中的 `gr4_v111.zip` 及其已解包文件，不引用官网、蓝牙规范、第三方应用、其他机型或其他固件版本的信息。

分析方法为静态检查：文件清单、启动脚本、ELF 动态符号、字符串、反汇编及 GATT 特征注册参数。未在真实相机上执行连接、配对或读写测试。因此：

- “已确认”表示可由当前固件文件直接证明。
- “程序命名”表示名称来自二进制中的类名、函数名或日志文本。
- “推断”表示依据当前固件内的调用关系得出，但尚未进行实机验证。
- 未在固件中找到的枚举含义、字节序或业务数据结构，不使用外部资料补全。

固件包 SHA-256：

```text
ded47aa6385886e4b1f8f41f8551866ab0f1ecd0aef89934a43ef0e456539e8f  gr4_v111.zip
```

## 2. 结论摘要

当前固件中的相机蓝牙业务由 `bled` 提供，形态为 BLE 外设/GATT 服务端。它通过 `libBtStack.so` 调用底层蓝牙栈，通过 `libcamera-controller.so` 访问相机状态和控制接口。

固件实际注册 6 个自定义 128-bit GATT 服务、64 个特征：

| 服务 | 特征数 | 主要用途 |
|---|---:|---|
| Bluetooth Control | 2 | 蓝牙启用条件、配对设备名称 |
| Camera Information | 6 | 厂商、型号、序列号、固件版本、蓝牙名称和地址 |
| Camera Service | 14 | 电源、时间、工作模式、存储、文件传输、相机名、固件升级 |
| GPS Control | 1 | GPS 信息读写 |
| Shooting | 35 | 拍摄参数、拍摄操作、状态、参数列表、自定义控制配置 |
| WLAN Control | 6 | WLAN 类型、SSID、口令、信道、5 GHz 可用性、频率 |

关键运行参数如下：

| 项目 | 固件内参数 |
|---|---|
| 控制器串口 | `/dev/ttyUSI1` |
| 初始串口速率 | `115200` |
| 硬件流控 | `crtscts` / `hciattach ... flow` |
| 初始化后串口速率 | `3000000`（`ble_enable.sh`） |
| HCI 设备名 | `hci0` |
| 蓝牙服务进程 | `/usr/bin/bled` |
| 服务日志级别 | `BLESRVD_LOG=info` |
| 常规启动条件 | 非 `restrict` 启动模式 |
| 广播参数组 1 | `0x0020`–`0x0030`，附加值 `30000` |
| 广播参数组 2 | `0x0152`–`0x01FE`，附加值 `-1` |
| 启动广播重试 | 最多 5 次；重试间等待常量 `100000000` |
| 配对确认 | 支持 Numeric Comparison 确认回调 |
| 隐私地址 | 程序调用 `LeSetRpaEnable`，即存在 RPA 开关流程 |

> 广播数值的字段名称可由固件中的 `fastInterval`、`slowInterval` 日志格式和结构体使用方式确认；时间单位未在固件中自描述，本文不使用外部规范换算。

## 3. 固件内蓝牙组件

### 3.1 核心文件

| 路径 | 大小 | SHA-256 | 作用 |
|---|---:|---|---|
| `/usr/bin/bled` | 676244 B | `1df9a91eb331750cf534edbb13373c0353e4742d17a02f37c9e8dd66afd5541e` | 相机 BLE 服务进程 |
| `/usr/lib/libBtStack.so` | 959852 B | `310a759dd8ca42946cda05752c3f6ecae2bb6f8c73c27feed174707485dcd02a` | 蓝牙协议栈接口库 |
| `/usr/bin/testbt` | 162524 B | `f4d73f8e008ab2a012c766e9cf6db8050ffeb626b1cb4c7d6f7dc5fdc32e2d01` | 蓝牙测试程序，不是常规启动服务 |
| `/lib/firmware/nxp/sduart_nw61x_v1.bin.se` | 1038668 B | `fc10344d2514e5874f5db3943282089d6d192788d14a95be3d3abb80558cb04a` | 文件名表明为 NW61x 的组合固件镜像 |
| `/lib/firmware/nxp/uartuart_n61x_v1.bin.se` | 363452 B | `af0b7d68b0d316126706a776d86c9caff69bc3d859096739b9a958f141390e50` | BT-only 加载脚本使用的固件镜像 |

`bled`、`testbt` 和 `libBtStack.so` 均为 32-bit little-endian ARM ELF。`bled` 为剥离后的 PIE，但仍保留了大量动态 C++ 符号，因此能够恢复服务和特征名称。

固件中的模块版本记录为：

```text
2732063 variant/2025-07-HDF-OW-model-development bleapi-server
```

### 3.2 软件调用关系

```text
相机属性/控制
      ↑
libcamera-controller.so
      ↑
    bled  ←→  libBtStack.so  ←→  HCI / hci0
                                      ↑
                              /dev/ttyUSI1
                                      ↑
                         NXP 固件镜像与控制器
```

`libBtStack.so` 暴露的接口覆盖 GATT 服务端、广播、扫描、配对、绑定数据库、RPA、P-256/AES 等能力；当前 `bled` 的业务实现主要使用 BLE 广播、连接、配对和 GATT 服务端接口。库中存在更宽泛的蓝牙能力，不等于本固件的相机业务已启用这些能力。

## 4. 启动与控制器初始化

### 4.1 系统启动

`/etc/rc.local` 中可直接确认：

1. 将 `/dev/ttyUSI1` 设为 `115200`。
2. 为该串口启用 `crtscts` 硬件流控。
3. 系统处于 `restrict` 模式时提前退出，不启动 `bled`。
4. 常规模式下，在 `netmgrd` 启动并完成前置等待后执行：

```sh
BLESRVD_LOG=info /usr/bin/run_app /usr/bin/bled &
```

### 4.2 `ble_enable.sh`

脚本只接受一个参数 `0` 或 `1`：

- `1`：先对 `1b000000.mmc` 执行 unbind/bind，并等待 `2000000`。
- `0`：跳过上述重新绑定。

随后执行：

```text
hciattach /dev/ttyUSI1 any 115200 flow
hciconfig hci0 up
/lib/firmware/nxp/bt_power_config_ALL.sh
hcitool -i hci0 cmd 0x3f 0x0009 c0 c6 2d 00
等待 50000
stty -F /dev/ttyUSI1 3000000
hciconfig hci0 down
killall hciattach
```

这表明初始化阶段以 `115200` 接入控制器，完成厂商命令后将串口切换到 `3000000`。脚本最终关闭临时 HCI 接口并结束 `hciattach`。

### 4.3 BT-only 固件加载

`ble_loadBTonlyFW.sh` 执行以下流程：

```text
unbind 1b000000.mmc
关闭 ttyUSI1 的 crtscts
fw_loader_linux /dev/ttyUSI1 115200 0 \
  /lib/firmware/nxp/uartuart_n61x_v1.bin.se 4000000
重新启用 crtscts
hciattach /dev/ttyUSI1 any 115200 flow
拉起 hci0、执行电源配置和厂商命令
关闭 hci0、结束 hciattach
```

`/usr/bin/ble_disable.sh` 仅含 shebang，没有实际禁用命令。

### 4.4 固件内原始 HCI 命令

`bt_power_config_ALL.sh` 顺序执行：

```text
3F 61 00 00 01 1C 37 43 1C 00 1C 00 00 00 01 7F 00 07 08 00 00 00 00 C2 01 00 00 00 00 00 00 00 00 00
03 003
3F FF 64 87 10 00 FF FF FF FF 01 00 04 00 01 00 04 00
03 003
3F 00EE 01 00
03 003
```

此外，两个初始化脚本都执行：

```text
3F 0009 C0 C6 2D 00
```

这些命令的厂商私有字段含义未由固件自描述，本文仅保留原始值，不做外部释义。

## 5. GATT 注册参数说明

下表中的权限来自 `bled` 调用 `Service::AddCharacteristic` 时传入的标志值，并与对应类是否实现 Read、Write、Notify 路径交叉确认：

| 固件标志 | 本文表示 | 程序行为 |
|---:|---|---|
| `0x02` | R | 读 |
| `0x08` | W | 写 |
| `0x0A` | R/W | 读、写 |
| `0x12` | R/N | 读、通知 |
| `0x1A` | R/W/N | 读、写、通知 |

“最大长度”是特征注册时传给 `AddCharacteristic` 的最大值长度，单位为字节。它不是业务负载格式说明；具体枚举值、位域和字节序需要继续分析各特征的 Read/Write 实现或进行实机验证。

### 5.1 Bluetooth Control

服务 UUID：`0F291746-0C80-4726-87A7-3C501FD3B4B6`

| 特征（程序命名） | UUID | 权限 | 最大长度 | 固件内行为说明 |
|---|---|---:|---:|---|
| BLEEnableCondition | `D8676C92-DC4E-4D9E-ACCE-B9E251DDCC0C` | R/W/N | 1 | 蓝牙启用条件 |
| PairedDeviceName | `FE3A32F8-A189-42DE-A391-BC81AE4DAA76` | W | 33 | 配对设备名称；写入路径包含长度和可打印字符检查 |

### 5.2 Camera Information

服务 UUID：`9A5ED1C5-74CC-4C50-B5B6-66A48E7CCFF1`

| 特征 | UUID | 权限 | 最大长度 |
|---|---|---:|---:|
| ManufacturerName | `F5666A48-6A74-40AE-A817-3C9B3EFB59A6` | R | 28 |
| ModelName | `35FE6272-6AA5-44D9-88E1-F09427F51A71` | R | 11 |
| SerialNumber | `0D2FC4D5-5CB3-4CDE-B519-445E599957D8` | R | 8 |
| FirmwareRevision | `B4EB8905-7411-40A6-A367-2834C2157EA7` | R | 10 |
| BluetoothDeviceName | `6FE9D605-3122-4FCE-A0AE-FD9BC08FF879` | R | 33 |
| BluetoothMacAddress | `97E34DA2-2E1A-405B-B80D-F8F0AA9CC51C` | R | 18 |

这些值由 `bled` 通过相机控制接口取得，而不是在 GATT 服务中硬编码完整字符串。

### 5.3 Camera Service

服务 UUID：`4B445988-CAA0-4DD3-941D-37B4F52ACA86`

| 特征 | UUID | 权限 | 最大长度 | 说明 |
|---|---|---:|---:|---|
| CamSrvNotification | `FAA0AEAF-1654-4842-A139-F4E1C1E722AC` | R/N | 182 | 相机服务通知数据 |
| CameraPower | `B58CE84C-0666-4DE9-BEC8-2D27B27B3211` | R/W/N | 1 | 相机电源状态/控制 |
| BatteryLevel | `875FC41D-4980-434C-A653-FD4A4D4410C4` | R/N | 1 | 电池级别 |
| DateTimeZone | `FA46BBDD-8A8F-4796-8CF3-AA58949B130A` | R/W | 7 | 日期、时间与时区数据 |
| OperationModeList | `430B80A3-CC2E-4EC2-AACD-08610281FF38` | R/N | 6 | 可用工作模式列表 |
| OperationMode | `1452335A-EC7F-4877-B8AB-0F72E18BB295` | R/W/N | 1 | 当前工作模式 |
| GEOTag | `A36AFDCF-6B67-4046-9BE7-28FB67DBC071` | R/W/N | 1 | 地理标记开关/状态 |
| StorageInformation | `A0C10148-8865-4470-9631-8F36D79A41A5` | R/N | 29 | 存储信息 |
| FileTransferList | `D9AE1C06-447D-4DEA-8B7D-FC8B19C2CDAE` | R/N | 2 | 文件传输列表状态/选择值 |
| PowerOffDuringFileTransfer | `BD6725FC-5D16-496A-A48A-F784594C8ECB` | R/W/N | 2 | 文件传输期间关机相关控制 |
| CameraName | `EF437870-180A-44AE-AFFD-99DBD85ADE43` | R/W | 32 | 相机名称 |
| FirmwareUpdateAvailable | `E450ED9B-DD61-43F2-BDFB-6AF500C910C3` | W | 1 | 写入固件升级可用状态/请求 |
| FirmwareUpdateStart | `5F0A7BA9-AE46-4645-ABAC-58AB2A1F4FE4` | W | 1 | 启动固件升级流程 |
| FirmwareUpdateResult | `39879AAC-0AF6-44B5-AFBB-CA50053FA575` | R/N | 1 | 固件升级结果 |

### 5.4 GPS Control

服务 UUID：`84A0DD62-E8AA-4D0F-91DB-819B6724C69E`

| 特征 | UUID | 权限 | 最大长度 | 说明 |
|---|---|---:|---:|---|
| GPSInformation | `28F59D60-8B8E-4FCD-A81F-61BDB46595A9` | R/W | 32 | GPS 信息；程序读写路径使用字符串/字节数据 |

### 5.5 Shooting

服务 UUID：`9F00F387-8345-4BBC-8B92-B87B52E3091A`

| # | 特征 | UUID | 权限 | 最大长度 |
|---:|---|---|---:|---:|
| 1 | ShootingNotification | `671466A5-5535-412E-AC4F-8B2F06AF2237` | R/N | 182 |
| 2 | HFShootingNotification | `2AC97991-A78B-4CD4-9AE8-6E030E1D9EDB` | R/N | 182 |
| 3 | ShutterSpeed | `D3CE2AED-10FA-4648-833D-CD74C6F35905` | R/W/N | 1 |
| 4 | Aperture | `3911F22D-9771-479D-B2B9-F729D9BAF9DC` | R/W/N | 1 |
| 5 | ISO | `206BD02C-78B2-42C4-820A-CF30E0963909` | R/W/N | 4 |
| 6 | ExposureCompensation | `30BCC8EB-725D-4048-A832-E76AE26A57E9` | R/W/N | 1 |
| 7 | WhiteBalance | `2361F4FF-2C7E-4FC5-876B-F9B0EFBC06FD` | R/W/N | 1 |
| 8 | DriveMode | `B29E6DE3-1AEC-48C1-9D05-02CEA57CE664` | R/W/N | 1 |
| 9 | ShootingMode | `A3C51525-DE3E-4777-A1C2-699E28736FCF` | R/W/N | 2 |
| 10 | MeteringMode | `ED58217E-1839-43B2-BCD7-DC48C36AC0DE` | R/W/N | 1 |
| 11 | CaptureMode | `78009238-AC3D-4370-9B6F-C9CE2F4E3CA8` | R/W/N | 1 |
| 12 | FileType | `95BFA8CA-4680-424D-B27C-AAC20D86E48B` | R/W/N | 4 |
| 13 | JPEGResolution | `9838BB04-4ABB-4C12-AE22-626D02E3704B` | R/W/N | 1 |
| 14 | MovieConfiguration | `404F6626-1294-407F-AB3D-DDC6B805B6BC` | R/W/N | 1 |
| 15 | Shutter | `559644B8-E0BC-4011-929B-5CF9199851E7` | W | 2 |
| 16 | SelfTimer | `009A8E70-B306-4451-B943-7F54392EB971` | W | 1 |
| 17 | ShutterSpeedList | `B355330D-4ADC-4434-A222-7B91404B4788` | R/N | 63 |
| 18 | ApertureList | `4866F4A9-2C83-457B-B393-B9535E1447E5` | R/N | 41 |
| 19 | ISOList | `9C83DF56-FD93-4639-8CA7-857BB7B3CA3D` | R/N | 161 |
| 20 | ExposureCompensationList | `01879798-28EE-4D97-92C9-FD249C88BBCC` | R/N | 50 |
| 21 | WhiteBalanceList | `FB673486-2A76-41B8-88F7-F88552FE5745` | R/N | 18 |
| 22 | DriveModeList | `F4B6C78C-7873-43F0-9748-F4406185224D` | R/N | 17 |
| 23 | FileTypeList | `F3BFB222-C62B-4AAA-BB61-EF6486626CC8` | R | 6 |
| 24 | FocusMode | `89458F80-50A1-42C1-B031-1BC6082179C0` | R/W/N | 2 |
| 25 | AutoFocusStatus | `CDFC734E-EA21-427D-A69F-C1A0F7F1E9A3` | R/N | 2 |
| 26 | CaptureStatus | `B5589C08-B5FD-46F5-BE7D-AB1B8C074CAA` | R/N | 4 |
| 27 | ShotCount | `12D262BA-D8BF-44B0-8E85-C414A40230A9` | R/N | 2 |
| 28 | ShootingModeList | `F662DCD8-AC6E-4E02-A4B2-CE92CD44C7C3` | R/N | 8 |
| 29 | FocusSettingList | `31B28DAB-BD3C-4C27-AA08-F379BF737C1E` | R/N | 10 |
| 30 | StillImageControlCustom1Name | `30ADB439-1BC0-4B8E-9C8B-2BD1892AD6B0` | R/W | 32 |
| 31 | StillImageControlCustom2Name | `E799198F-CF3F-4650-9373-B15DDA1B618C` | R/W | 32 |
| 32 | StillImageControlCustom3Name | `DF77DD09-0A48-44AA-9664-2116D03B6FD7` | R/W | 32 |
| 33 | StillImageControlCustom1Data | `3E0673E0-1C7B-4F97-8CA6-5C2C8BC56680` | R/W | 56 |
| 34 | StillImageControlCustom2Data | `0936B04C-7269-4CEF-9F34-07217D40CC55` | R/W | 56 |
| 35 | StillImageControlCustom3Data | `CD879E7A-AB9F-4C58-90ED-689BAE67EF8E` | R/W | 56 |

列表型特征和当前值特征是分开的。例如 `ShutterSpeed` 最大 1 字节，而 `ShutterSpeedList` 最大 63 字节；应用端应先读列表，再按固件接受的编码写入当前值。具体编码表尚不能仅凭注册参数确定。

### 5.6 WLAN Control

服务 UUID：`F37F568F-9071-445D-A938-5441F2E82399`

| 特征 | UUID | 权限 | 最大长度 | 说明 |
|---|---|---:|---:|---|
| NetworkType | `9111CDD0-9F01-45C4-A2D4-E09E8FB0424D` | R/W/N | 1 | 网络类型 |
| SSID | `90638E5A-E77D-409D-B550-78F7E1CA5AB4` | R/W | 33 | WLAN SSID |
| Passphrase | `0F38279C-FE9E-461B-8596-81287E8C9A81` | R/W | 64 | WLAN 口令 |
| Channel | `63BC8463-228F-4698-B30D-FAF8E3D7BD88` | R/W | 1 | WLAN 信道 |
| Available5GHz | `460828AC-94EB-4EDF-9BB0-D31E75F2B165` | R | 1 | 5 GHz 可用状态 |
| WlanFrequency | `C4B7DFC0-80FD-4223-B132-B7D25A59CF85` | R/W | 1 | WLAN 频率选择/状态 |

二进制中还保留 `SecurityType` 类名和实现痕迹，但该对象未出现在当前 `WLAN Control::InitService` 的 6 个实际注册项中。因此它属于未注册实现残留，不能按当前固件的可用 GATT 特征对外公布。

## 6. 广播参数与广播数据

### 6.1 广播参数

`BleServer` 构造并传给 `SetAdvertParams` 的两组参数为：

| 程序内用途 | 最小值 | 最大值 | 附加值 |
|---|---:|---:|---:|
| fastInterval 参数组 | `0x0020` | `0x0030` | `30000` |
| slowInterval 参数组 | `0x0152` | `0x01FE` | `-1` |

`SetAdvertParams(bool)` 会按状态切换当前参数，且保留慢速参数组作为另一组配置。二进制没有为 `30000` 和 `-1` 的字段提供足够的类型信息；从使用方式看它们与对应区间共同组成配置，但本文不进一步命名其单位。

服务初始化后会调用启动广播；失败路径最多重试 5 次，重试等待使用常量 `100000000`。

### 6.2 广播内容

可直接从 `SetAdvertData` 的组包逻辑确认：

- 固定起始字节：`02 01 06`。
- 紧接一个 128-bit 服务 UUID 数据段。
- 根据状态在两个服务 UUID 间切换：
  - Camera Information：`9A5ED1C5-74CC-4C50-B5B6-66A48E7CCFF1`
  - Bluetooth Control：`0F291746-0C80-4726-87A7-3C501FD3B4B6`
- 后续固定字节包含 `02 0A 00`。
- 扫描响应包含以 `FF 5F 06 DA` 开头的数据段，以及由设备名称动态生成的数据段。
- UUID 在写入广播缓冲区前调用 `ReverseCopyUUID16`，即广播内存顺序与本文展示的 UUID 文本顺序相反。

扫描响应中的设备名称、序列号相关内容为动态数据。当前静态分析不足以给出所有动态字节的稳定业务字段定义。

本地 Class-of-Device 设置常量为 `0x00080620`。该常量由程序直接传给底层栈；本文不引用外部分类表解释其类别。

## 7. 配对、安全与设备数据库

固件中存在两种明确命名的安全状态：`SetDisplayPairingMode` 与 `SetNonSecureMode`。

已确认行为：

- 安全请求回调名为 `OnSecurityRequest`。
- 只有处于配对模式时才接受新的安全请求。
- 非配对状态收到请求时，程序存在否定应答和强制断开路径。
- 已配对设备再次请求时有独立处理路径。
- 支持 `OnNCConfirmNotify` 与 `SetNumericComparisonConfirmResult`，即数字比较确认流程。
- 配对完成回调为 `OnPairingComplete`。
- 底层栈提供 DDB 的枚举、读取和删除接口；程序会把配对设备表保存为持久数据。
- 固件字符串中存在逻辑数据名 `BLE_DEVICE_DB` 和 `BLE_PAIRED_DEVICES`。
- 程序调用 `LeSetRpaEnable`，存在 RPA 启用流程。
- 删除、更新以及“最旧设备”相关日志表明配对表存在维护/淘汰逻辑；当前分析未确认容量常量，不给出最大配对台数。

反汇编可见安全设置调用使用的原始枚举组合包括：

| 模式 | 原始调用参数 |
|---|---|
| Display pairing | 首组参数 `(1, 1)`；后续安全级别参数 `3`；RPA 开关参数 `1` |
| Non-secure | 首组参数 `(3, 0)`；安全级别参数 `0`；RPA 开关参数 `1` |

枚举值 `1`、`3` 的厂商 API 语义未在固件中自描述，不能仅依名称把它们进一步翻译成某种认证或加密等级。

`testbt` 中存在测试用安全、配对和 passkey 文本，但 `testbt` 没有出现在常规启动流程中，因此测试值不能当作相机产品运行时凭据或固定 PIN。

## 8. 日志与诊断入口

### 8.1 运行日志

- 系统启动时设置 `BLESRVD_LOG=info`。
- `bled` 还识别 `BLESRVD_LOG_OUTPUT`。
- 二进制日志覆盖初始化、广播、连接/断开、安全请求、配对完成、DDB 更新、GATT 读写和通知发送。

### 8.2 可用于后续实机验证的观察点

在不修改固件行为的前提下，可优先验证：

1. 启动日志中蓝牙栈初始化、服务注册和广播启动结果。
2. 配对模式与非配对模式下 `OnSecurityRequest` 的差异。
3. 连接后实际发现的 6 个服务和 64 个特征是否与静态注册表一致。
4. 各特征的实际长度、字节序、枚举范围和错误响应。
5. fast/slow 广播参数切换的触发条件及 `30000` 字段的实际单位。
6. 配对数据库容量、淘汰策略和持久化位置。

## 9. 功能边界与风险提示

### 9.1 当前固件明确具备的业务面

- 相机信息读取。
- 相机电源和工作模式控制。
- 拍摄参数读取、设置、状态通知和快门操作。
- GPS 信息写入/读取。
- 通过 BLE 配置 WLAN 参数。
- 文件传输状态与相关电源策略控制。
- 相机名称和三组静态图像自定义控制配置。
- 固件升级可用、开始和结果状态通道。

### 9.2 二次开发注意事项

- 不要只凭 UUID 写入：多数可写特征的业务枚举尚未解码，错误值可能被拒绝，也可能触发真实相机操作。
- `Shutter`、`CameraPower`、`FirmwareUpdateStart` 等具有明显副作用，应先完成离线格式分析，再进行受控实机验证。
- `Passphrase`、配对数据库和设备身份数据属于敏感信息；调试日志和转储应避免泄露。
- 广播包与扫描响应包含动态设备信息，修改名称/序列号相关逻辑时要同时检查长度上限。
- `libBtStack.so` 的能力集合大于 `bled` 当前使用集合，不能把库导出符号直接视为相机已开放功能。
- `SecurityType` 是未注册代码痕迹；修改后若要启用，仍需验证对象生命周期、相机控制接口和客户端兼容性。

## 10. 证据位置

本报告的主要证据均来自解包目录 `output/http-analysis/initfs`：

| 证据 | 固件内路径 |
|---|---|
| 系统启动和 `bled` 启动条件 | `/etc/rc.local` |
| 控制器启用流程 | `/usr/bin/ble_enable.sh` |
| BT-only 固件加载 | `/usr/bin/ble_loadBTonlyFW.sh` |
| 空禁用脚本 | `/usr/bin/ble_disable.sh` |
| 厂商 HCI 命令 | `/lib/firmware/nxp/bt_power_config_ALL.sh` |
| BLE 服务、特征、广播、安全和配对逻辑 | `/usr/bin/bled` |
| 底层蓝牙栈能力 | `/usr/lib/libBtStack.so` |
| 测试程序与参数名称交叉验证 | `/usr/bin/testbt` |
| 模块版本 | `/etc/revisions/bleapi-server` |
| 控制器固件镜像 | `/lib/firmware/nxp/*.bin.se` |

## 11. 尚未由当前静态分析确定的项目

以下信息不能只依据当前已提取证据可靠确定，故没有用外部资料填充：

- 每个拍摄参数枚举值对应的用户界面含义。
- 多字节数值的完整字节序和缩放规则。
- 182 字节通知载荷的内部消息格式。
- GPSInformation 的精确文本/二进制格式。
- 广播参数附加值的时间单位。
- 扫描响应中 `FF 5F 06 DA` 后全部动态字段的定义。
- 厂商 HCI 命令各字段的硬件含义。
- 最大绑定设备数量。
- RPA 轮换周期。
- 手机端协议状态机和兼容性要求。

这些项目需要继续对各 Read/Write/Notify 函数做数据流反汇编，或在隔离条件下进行实机抓包与读写验证。

