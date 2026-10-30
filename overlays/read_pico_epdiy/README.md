# Read Pico EPDiy 诊断覆盖 / diagnostic overlay

此覆盖仅由显式 Read Pico 显示验证 profile 选中，只在构建目录中生效。它先校验两个固定
EPDiy 源文件的 SHA-256，再生成补丁副本；其余 EPDiy 源码仍直接取自固定子模块。

This build-only overlay is selected exclusively by the explicit Read Pico display-validation
profile. It verifies the two pinned EPDiy source hashes, generates patched copies in the build
directory, and compiles all other EPDiy sources directly from the pinned submodule.

唯一诊断变量是纵向传输几何 / The single diagnostic variable is vertical transport geometry:

- 两个 6 行 DMA bounce buffer 构成 12 行周期；/ two six-line DMA bounce buffers form one
  12-line cycle;
- 684 行恰为 57 个周期，不再扫描 684–687 填充行；/ 684 panel rows are exactly 57 cycles,
  so no 684–687 padding rows are scanned;
- renderer 与 LCD 行数同步改变；/ renderer and LCD line counts change together;
- 像素时钟、波形、VCOM、面板时序、framebuffer 与电源流程不变。/ pixel clock, waveform,
  VCOM, panel timing, framebuffer and power sequencing are unchanged.

生成文件保留上游 EPDiy 许可头，固定子模块不会被修改。源码哈希或替换锚点不匹配时，
CMake 配置直接失败，不会把补丁静默套到不同版本。这只是诊断候选，不是已验证的生产修复。

The generated files retain the upstream EPDiy license headers. The pinned submodule is never
modified. A source hash or replacement-anchor mismatch fails CMake configuration rather than
silently applying the overlay to different upstream code. This is a diagnostic candidate, not a
validated production fix.
