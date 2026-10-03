# 组件开发约定

| 目录 | 当前职责 |
| --- | --- |
| `components/frame_core` | 纯 C11 状态、日志 wire format、ACK guard、DCF XS 路径、灰阶 Fit |
| `components/image_pipeline` | FILE 流式 JPEG 解码、EXIF 方向；allocator/yield 由宿主注入 |
| `main` | ESP-IDF/BSP 装配和一次显示 Demo |
| `host` | 桌面构建、转换工具 |
| `tests` | 自有合成 JPEG 与核心回归 |
| `third_party/read_pico_firmware` | 未修改的上游 BSP/驱动/解码器 |

`frame_propose` 只生成新状态，不代替文件落盘。调用方须先完成持久化，再发布候选状态。`frame_replay` 当前是单照片、顺序单调的日志原型，不是多照片数据库；不跳过损坏记录继续解释后续数据。

`frame_can_ack` 是调用端防护条件，不是相机传输 API 的实现；实际 ACK 仍禁用。路径构造只返回 XS GET 的相对路径，调用端还需验证真实 storage/相机身份/地址。

内部 g4 每行 `(width+1)/2` 字节，左像素高 nibble，黑 0/白 15；奇数行宽末尾保留白 padding。渲染 API 的 source 必须拥有至少 `sw*sh` 灰度字节，输出容量显式验证；不允许输入/输出重叠。送 EPD 通过驱动坐标/像素转换。

所有资源有上限；输入异常返回错误并释放内存。网络与设备 IO 不放入便携核心；以后只通过独立适配器接入。添加依赖时记录来源、commit/版本、许可和内存影响。
