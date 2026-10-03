# 第三方依赖与许可

上游：[MindReset/read_pico_firmware](https://github.com/MindReset/read_pico_firmware)，commit `28cde682a4468a581c278761922724f57d976418`。

本项目通过子模块引用，不修改上游源码。许可文件保留在子模块中，发布二进制前须检查具体被链接组件的再分发义务。

| 部分 | 许可/来源 | 引用位置 |
| --- | --- | --- |
| Read Pico BSP、PMU、芯片驱动 | 上游 Apache-2.0 声明 | `third_party/read_pico_firmware/components` |
| EPDiy 裁剪分支 | LGPL-3.0-or-later | `components/epdiy/LICENSE`（子模块内） |
| E0470 波形 | 上游 Apache-2.0 声明 | `components/e0470_epaper_waveform/LICENSE` |
| TJpgDec R0.03 | ChaN 原始再分发声明 | `main/book/vendor/tjpgd.c` 和 vendor README |
| ESP-IDF | 组件各自声明，以锁定 v6.1 为准 | IDF LICENSE/组件许可 |

上游字体、书籍应用、Web 上传应用不作为本次产品模块链接。测试图为本项目生成的 RGB 渐变，没有引用用户附件或相机原片。

新增项目源码的发布许可尚待选择；本文件不替用户作许可证决定。
