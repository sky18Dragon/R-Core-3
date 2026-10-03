# 烧写与维护

当前是开发原型，不涉及网站或 Cloudflare 部署。

烧写替换 Read Pico 当前应用。先保留需要的原固件与 SD 数据，确认板型与 PMU 出厂 VCOM，再核对 IDF 生成的 `build/flash_args`。本项目不会自动烧写、擦除 NVS 或格式化卡。

确认目标串口后使用：

```sh
idf.py -p <实际串口> flash monitor
```

不要将 app bin 写到地址 0，也不要凭记忆手写偏移；使用生成的 flash_args。首次原型分区维持上游 nvs/phy/factory/storage 偏移，尚未实现技术方案中的最终双槽/日志分区。更改分区表时必须先制定数据迁移方案。

日志中须核对 ESP-IDF v6.1、BSP/PMU 成功、JPEG/render/copy 时长与 `Demo 0 refresh result`；只有编译或 esptool 字节校验不足以证明真实显示成功。电子纸保留旧画面也不能证明新固件正在运行。

P2 OTA 尚未实现；相机固件升级接口永不从此项目调用。
