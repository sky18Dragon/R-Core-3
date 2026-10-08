# Read Pico 上传接收、非覆盖发布与隔离探针

日期：2026-10-07。沿用 PhotoFrame HTTP/相册架构，未启用完整产品。用户确认仍用测试卡并允许更新后，332,304字节/c876599f…SD合成诊断已仅factory部署/独立全长回读/分区保持，旧7861965c…完整应用保留。唯一mount/upload-test的created2/written4112/published1/verified8192/conflict_rejected1/close_failed0通过，打开占用1→关闭0、最终status正常及卸载/HAL/STOPPED通过；控制台退出见VALIDATION。完整HTTP业务仍未部署运行，不将此实卡子路径当全产品通过。

## 已实现的软件保护

仅 Read Pico 的 HTTP 上传改用便携 `upload_stream` 和 `upload_file`，板级IO在 `read_pico_upload`。其他板保留原接收/覆盖策略，不修改固定厂家代码。

- multipart解析使用有界字节流，不对未终止接收缓冲做strstr；支持浏览器image/thumbnail带文件名的子集，边界≤70字节、每段头≤1024、文件名≤119字节。跨块分隔符和二进制零字节保持，分隔符CRLF不写入文件；重复/未知字段、危险路径/控制符、缺失末边界或尾部垃圾失败。
- 每文件≤5MiB，multipart请求≤10MiB+4096字节；raw保留原5MiB上限。不宣称原生HTTP上传支持9–16MiB相机原片。连续8次接收超时或120秒接收阶段预算失败；同步网络/SDK调用本身不是硬取消/严格实时上限。
- 暂存用独占open(O_CREAT|O_EXCL)，随机数+原子序号生成隐藏名称`.rc3-upload-*.part`，碰撞最多16次，仅EEXIST换新名。短写、零写、同步、关闭或介质错误不当成功；FD直写不经过stdio缓冲，完成时fsync→一次close→介质检查。关闭不确定锁存SD并不重试、卸载或删除路径。
- 事务绑定本次独占创建的源路径，不能拿另一原片路径冒充待发布源。失败保留已创建衍生文件，未新增自动清理策略；长期空间回收仍待设计/明确权限。
- 相册PNG/EPDGZ及JPEG缩略图都必须完整接收/同步/关闭；检查魔数只用于格式识别，不代替完整解码验收。路径组件校验通过后，同名返回409，旧文件不预删除。最终由FAT的非覆盖rename保护，不能只靠先stat再rename。

锁定IDF v6.1源码已核对：`components/fatfs/vfs/vfs_fat.c:979`的rename持FAT锁，`components/fatfs/src/ff.c:5305`在不同既存对象冲突时返回FR_EXIST，映射EEXIST；不能将普通主机POSIX rename的覆盖语义拿来替代。Read Pico唯一/storage禁止fallback至其他文件系统，适配只接受该根下有效路径。

发布顺序为缩略图先、主图后。不是两文件原子事务，也不保证FAT掉电安全；第二步失败时保留已发布缩略图/未发布主图并返回错误，不伪称回滚。rename错误或rename后介质失效也可能留下可见结果，不重试或按名称删除。

## 2026-10-07后续：直接显示、URL和快照（仅软件）

raw/multipart直接显示继续持有接收事务对象。便携`upload_asset`只接受完整同步/关闭、本次独占创建的隐藏PART，检查图像/可选JPEG预览魔数后，预览先发布为`.preview.jpg`、主图后发布为独立`.jpg/.png/.bmp/.epdgz`。Read Pico裸路径stage接口拒绝，retire/drop不删除固定current图片；原板保留原流程。失败保留所有文件，不将魔数识别当完整解码通过。

URL入口保留PhotoFrame HTTP头、远端配置、ETag和Downloads业务，改用独占暂存。主图接收≤32MiB、缩略图≤5MiB，不是总堆或所有格式支持保证；非200正文不写入图像，检查SDK完整接收、已知长度相等、非空、写入/fsync/关闭/最终介质。120秒预算在回调及完成检查处生效，不宣称同步调用可硬取消。失败保留每次暂存；304关闭并保留空暂存、跳过显示。ETag只在完整显示/记录成功后提交。

固定SDK的ON_DATA调度忽略用户回调返回值，故写入错误必须锁存在owner、完成时检查，不能只信perform的ESP_OK。跳转清除旧响应元数据，Location有界保存，在安排下次请求前去除跨站Authorization/配置自定义头，失败不安排跳转；相对跳转/默认端口归一有主机测试。跨站缩略图不携带源站配置凭据，缩略图也使用配置CA；真实HTTP/跳转/TLS仍待验收。

Downloads使用标准可见`download_<随机ID>-<序号>.epdgz/.bmp`及同名JPEG预览，不新增相册系统。JPEG文件流不整份复制压缩输入；预览/就绪文件复制检查长度/EOF/两端关闭/同步，每32KiB让出调度，独立保留输入。渲染快照先独占隐藏暂存，完成/关闭后FAT非覆盖发布，不把半写入最终EPDGZ给相册。快照在成功刷新后写入，失败返回ESP_ERR_NOT_FINISHED并指向保留源图。

当前图片读取通过既有显示锁复制RAM记录，不读取正在写的链接。记录写入/flush/fsync/关闭/介质失败明确报告“已显示但保存未完成”；匿名/清白/校准的记录清除失败也传播。仍保留原`.current.lnk`契约，其持久元数据是原位写入，**不具备掉电原子性或重启事务恢复保证**；旧链接/NVS迁移仍待审。主机JPEG用实际固定解码器+显示替身，发布IO替身模拟FAT非覆盖语义，不取代实机结果。

新增27项CTest和1项Python，普通/ASan各577、Python46、六配置编译通过，见VALIDATION末节。完整集成1,985,056字节/947458be…仅编译未部署，设备仍c876599f…/STOPPED；无新串口、复位、SD IO或屏幕/业务网络操作。

## 仍未完成的边界

日志及其余消费者/并发、保留元数据/NVS/OTA迁移、长期暂存清理与产品启动仍需审查。直接显示/URL/相册软件保护不宣称所有文件所有权、发布原子性或HTTP真机性能已验收。真实HTTP/URL、Wi-Fi、重启恢复、关闭/满卡/热拔/掉电故障、照片与长期运行未由主机回归或以下SD探针覆盖；BMP/EPDGZ沿用上游文件解码器，不将本轮JPEG主机通过推广到所有格式。

## 隔离SD探针（本卡单次通过，产物保留）

SD profile新增精确`upload-test`，开机不执行，仅新启动一次，失败也不再尝试。display/photo控制台不转发该命令。已有file-test/file-check/photo-check不自动附带执行。

| 固定路径 | 计划动作 |
| --- | --- |
| `/storage/.RC3U0001.PART` | 独占新建4096字节合成图样，256字节分块、同步/关闭后rename |
| `/storage/RC3U0001.BIN` | 预先存在即停止；新发布后读取4096字节/EOF，再在冲突尝试后复读确认不变 |
| `/storage/.RC3U0002.PART` | 独占新建16字节0xA5，同步/关闭；尝试发布到同一BIN必须EEXIST，保留该暂存文件 |

成功要求created=2、written=4112、published=1、verified=8192、conflict_rejected=1、close_failed=0；两次读/EOF均正确，每次打开/关闭的active_io=1→0、介质有效，最后status/卸载/HAL/控制台正常退出。成功后仅保留4096字节BIN和16字节PART；失败可能保留部分文件，不删除、修复、换路径或重跑。不会注入真实关闭故障/热拔，不碰RC3T0001.BIN或私人照片、不格式化/刷新/启网络/产品。

固定应用332,304字节，SHA256 `c876599f119258c0a59c93419283a3ad97df18d93714b3c2523fdd6548a062e3`，副本`.cache/p3-upload-storage-candidate-EuBiLz/`。本轮新许可下已核实设备/安全/分区、保存旧完整应用，仅factory写`[0x10000,0x61210)`/擦除`[0x10000,0x62000)`，独立完整回读及4096字节分区保持，再status→mount一次→upload-test一次→status→stop完成。私有证据`.cache/p3-upload-storage-hardware-fAeGCh/`。两个合成文件的创建/单次发布仅本轮获准；产物保留后不重跑、删除或换名，不将许可扩展为HTTP/产品/显示。

软件证据：上一软件轮普通与ASan/UBSan各550项、Python45项、六固件配置编译通过，本部署轮未重新运行。FAT发布/冲突/完整复读的本卡单次实机通过，软件、部署与实卡证据分开，见[验证记录](VALIDATION.md)；仍不宣称异常关闭/热拔/掉电安全、任意卡或HTTP/产品验收完成。
