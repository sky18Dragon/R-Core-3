# 开发与构建

P0 已完成源码引入及原版基线验证。当前根目录是 PhotoFrame 主工程；Read Pico只读/显示/SD诊断隔离，完整业务已有编译链接profile但禁止运行，见 [集成说明](READ-PICO-INTEGRATION.md) 和 [P1说明](READ-PICO-BRINGUP.md)。当前 `waveshare_photopainter_73` 产物不可烧入小纸 Pico。

## 固定版本与目录

| 项目 | 固定值 |
| --- | --- |
| PhotoFrame | `f5e3ec974403b31174a3c8da1cf09385453f4abb`，根目录，保留上游历史 |
| Read Pico | `28cde682a4468a581c278761922724f57d976418`，`third_party/read_pico_firmware/` gitlink |
| ESP-IDF | 正式 tag `v6.1`，commit `fff9895c82d744c7237be8847347bdd1b07c6643`，header 6.1.0 |
| Xtensa 编译器 | `esp-15.2.0_20251204`，GCC 15.2.0 |
| Node / npm | 22.23.3 / 10.9.9；Node 见根 `.node-version` |
| Python / splash | Python 3.14.6，`requirements-build.txt` 固定 qrcode 8.2；rsvg-convert 2.63.2 |
| 主机构建 | CMake 4.3.4、Ninja 1.13.2、libpng 1.6.58；GoogleTest v1.14.0 |

组件解析结果保留根 `dependencies.lock`，包含版本与 component_hash；不要删除锁后自动升级。Node 依赖保留 `webapp/package-lock.json` 与 `process-cli/package-lock.json`，用 `npm ci`。`build.py` 新增 Read Pico 隔离与防误操作，其 webapp 步骤仍使用上游 `npm install`；可先独立执行下面的锁文件构建命令。

Read Pico 已选入固定厂家 EPDiy、波形、SY 故障解码器与扫描时序；PMU 只引用协议头，外层完成解码/只读 GET，避免厂家 init 的 HOST_READY/drain。全部第三方源码保持原样，不引入 Demo main 或总 BSP 初始化。显示代码已编译/链接，但诊断传 false 禁止运行；电源配置权限与实屏条件见 READ-PICO-DISPLAY。

## 本机 SDK 激活

当前隔离目录：`/Users/zhanghuilong/Documents/ChatGPT/R-Core-3-2-baseline-FFl8Vm`，包含未改上游 `photoframe/`、固定 SDK `esp-idf-v6.1/` 和独立 Python 环境。此目录不在项目 Git 中，保留用于基线对照。

本机另一个名为 `/Users/zhanghuilong/esp/esp-idf-v6.1` 的目录实际 header 为 6.2.0 且有修改，未用于本轮、未改动。不能凭目录名选 SDK。

在 bash 中激活（环境变量仅当前终端有效，未修改 shell 配置）：

```sh
export PATH="/opt/homebrew/opt/node@22/bin:$PATH"
export IDF_PYTHON_ENV_PATH=/Users/zhanghuilong/Documents/ChatGPT/R-Core-3-2-baseline-FFl8Vm/python_env/idf6.1_py3.14_env
. /Users/zhanghuilong/Documents/ChatGPT/R-Core-3-2-baseline-FFl8Vm/esp-idf-v6.1/export.sh
idf.py --version
git -C "$IDF_PATH" rev-parse HEAD
cd '/Users/zhanghuilong/Documents/ChatGPT/R-Core-3 2'
```

应分别显示 `ESP-IDF v6.1` 和上述 SDK commit。初始准备使用固定 tag clone、递归子模块、`./install.sh esp32s3`，复用已有匹配工具；Python 环境新建在隔离目录。安装过程更新了 `.espressif` SDK 环境登记/约束缓存，未删除旧 SDK/工具或覆盖旧 Python 环境。

## 已验证命令

主机检查不要求激活 IDF，但需 Node 22、CMake、系统 libpng/zlib 及 process-cli 所需 canvas 库：

```sh
bash scripts/test_baseline.sh
```

脚本执行 `make test`、补跑当前318项CTest（含SDMMC、板级、显示核心/原生后端、业务、会话、标准图案、扫描、真实厂家CPU绘图、局部图边缘断言及供数故障注入）、6项方向、12项构建防误操作、8项无硬件控制台测试、60项Web测试和Web生产构建。最新SDMMC软件准备单独运行了318/318普通CTest、318/318 ASan/UBSan和20/20 Python检查；未复跑方向/Web，硬件历史不转移为本轮真机结果。原始入口可单独执行：

```sh
make test
ctest --test-dir host_tests/build --output-on-failure
npm --prefix webapp ci
npm --prefix webapp test
```

激活固定 SDK 后构建原板，不带 flash/monitor 参数：

```sh
python -m pip install -r requirements-build.txt
npm --prefix webapp ci
npm --prefix webapp run build
python build.py --board waveshare_photopainter_73 --step splash --step firmware
```

独立上游目录还验证了原始全步骤 `python build.py --board waveshare_photopainter_73`。当前根目录含空格，主机测试、Web/splash 生成与 IDF 完整编译均实际通过，不必移动项目。

产物：`build/esp32-photoframe.bin`、`build/bootloader/bootloader.bin`、`build/partition_table/partition-table.bin`、`build/flash_args`。均为原板编译基线，不是 Read Pico 发布固件。实际结果、摘要和告警见 [验证记录](VALIDATION.md)。

Read Pico 只读诊断编译：`python build.py --board read_pico --step firmware`，产物在 `build-read-pico/`、配置在 `.cache/sdkconfig.read_pico`；不要求 Web/splash。配置使用 80 MHz Flash/PSRAM，关闭 Flash coredump，入口约束 SDK 6.1.0；拒绝额外参数及 fullclean，不能据此照用 IDF 打印的烧写命令。完整说明见 [P1 诊断](READ-PICO-BRINGUP.md)。

本设备 factory 专用诊断：`python build.py --board read_pico --step firmware --read-pico-factory-diagnostic`。使用固定的 `boards/partitions.read_pico_factory.csv`，隔离到 `build-read-pico-factory/` / `.cache/sdkconfig.read_pico-factory`；不生成共享 OTA 表，仍只编译。本机已核验表与实板一致，获准仅写 app 并实机启动；不代表该表适用于未知设备，不覆盖完整迁移/OTA。CI 增加该配置编译但远端未运行。烧写范围与用户放弃备份的限制见 [维护说明](DEPLOYMENT.md)。

受控显示编译再加 `--read-pico-display-validation`，隔离 `build-read-pico-display/` / `.cache/sdkconfig.read_pico-display`，显式 y；普通配置 n。显示程序不在 boot 启屏，只有 `clear-white` 触发一次初始化/清白，`white-confirm` 必须有本次启动的人工目视确认。示例串口控制台（不自动发命令，日志私有，不重连/重发）：

```sh
python -u scripts/read_pico_display_console.py --port /dev/cu.usbmodem2101 \
  > .cache/read-pico-display-console-private.log 2>&1
```

仅需显式复位时才加 `--reset`；它会丢失白参考。最新源码及已部署版白名单为 clear-white/white-confirm/stop/status、九种 gc16-* 及 pattern-confirm/reject，含`gc16-gray16-page`。控制图必须是人工白参考后的首次绘制；每张需人工确认，错误/拒绝锁存，无批量轮播。历史GC16欠载0x400、新黑带、旧gray16/编号图白偏和18MHz边缘异常均保留。PM锁候选的官方局部页已确认，设备最后正常`STOPPED`；重新操作前先核实会话状态。官方示例对照及下一实验边界见 [显示说明](READ-PICO-DISPLAY.md)。

显示 overlay 要求 `CONFIG_COMPILER_OPTIMIZATION_PERF=y`、64 KB 数据缓存/64 B 行；只读 profile 不变。已有 `.cache/sdkconfig.read_pico-display` 不会因 defaults 自动切换：在固定 v6.1 环境以相同 `-B`/`-DSDKCONFIG`/`-DSDKCONFIG_DEFAULTS` 参数进入 menuconfig，明确选择三项后重编译，核对 SIZE=0x10000/LINE_SIZE=64。未迁移被入口 `#error` 拒绝，不删除配置/fullclean/擦设备或改频绕过。本机已显式迁移、编译并获准上板核验；不将实机配置日志计为 GC16 成功。

`read_pico_display` 是便携转换/刷新事务核心，`epaper_driver_read_pico` 是 IDF 外层后端。诊断链接后端及厂家依赖，仅调用 false guard，不启动屏幕。Read Pico 分支同时编译 display_manager/utils 的检查式调用，但不启动完整业务。新增后端测试使用模拟 HAL/LCD/RTOS，实际 GUI_Paint 打包与 display_manager 状态提交另有真实源码测试；API/硬件条件见 [显示适配说明](READ-PICO-DISPLAY.md)。

显示profile使用GNU link-wrap扫描观察器：显式clear-white记录末行/填充；`gc16-gray16-page`记录36相计时与12条关键renderer脉冲行指纹。普通入口不链接观察器；旧66425c9c…验证18MHz下软件观察完整但实屏几何仍失败，81ac6974…验证精确684行后几何恢复，当前6f2a4ea…进一步以PM锁恢复11.090–11.112ms/相与16阶。观察有开销，不等于长期稳定或全部物理测量完成。契约与观测限制见 [显示适配](READ-PICO-DISPLAY.md)。

历史gray16白偏、精确684行几何和C阶段连续HV结果均保留。当前部署为305,104字节/`6f2a4ea…` PM锁候选：显示事务持有CPU/APB max锁，固定子模块、18MHz、waveform、factory VCOM及电源流程不改；app-only全长回读一致、分区不变。人工白屏后唯一官方局部页36相399.327ms、照片16阶严格单调，确认后STOPPED。普通CTest 310/310、Python门禁20/20及固定IDF v6.1显示编译通过。该结果关闭持续白偏根因，不替代长期稳定、其他图案和照片链路验收；指纹仍只到软件供数回调。详见VALIDATION。

SDMMC backend 的 `sdcard_config_t.bus_width` 必须显式填写 `SDCARD_SDMMC_BUS_WIDTH_1` 或 `_4`。1-bit 只连接 CLK/CMD/D0，backend 强制 D1–D3 为 `GPIO_NUM_NC`；Read Pico 静态定义为38/42/44。原WaveShare显式填写4-bit。无效配置、缺少必要引脚、空挂载点或路径超过31字节会在调用IDF挂载前拒绝；成功后才记录路径。挂载失败不自动格式化，`sdcard_format()`仍是单独的显式破坏性接口。Read Pico只读/显示诊断不选择SD driver、不调用挂载；完整产品入口与实体卡验收待P3完成。

SD新增独立手动profile，构建命令与所有权/退出见 [SD说明](READ-PICO-STORAGE.md)。storage开关必须与factory同用、与display互斥，构建不烧写。SDMMC另增显式internal_pullups，Read Pico开启、原板false；卸载错误因SDK可能已释放句柄而锁存，不再次调用SDK。

完整业务编译：`python3 build.py --board read_pico --step firmware --read-pico-factory-diagnostic --read-pico-photoframe-integration`，独立目录与配置，禁止烧写/自动运行；需已有嵌入资源作为编译输入，不构成新板splash验收，详见集成说明。

最新全量ASan/UBSan为356/356通过；可按以下命令复跑（SD API由主机替身实现，不接触实体卡）：

```sh
cmake -S host_tests -B host_tests/build-asan \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build host_tests/build-asan -j4
ctest --test-dir host_tests/build-asan --output-on-failure
```

## CI 与尚未覆盖项

新增 `.github/workflows/port-baseline.yml`：Node 22 完整主机/Web 测试，正式 v6.1 SDK commit 检查、原板及五个Read Pico隔离入口编译；仅 `contents: read`，没有发布/烧写/实屏步骤。远端未运行，本机成功不等于 GitHub Actions 成功。

原有上游 `.github/workflows/ci.yml` 仅补主机测试 checkout 的递归子模块（新增测试需协议头）；其 Node 18/格式流程与 `build.yml` 的 release-v6.0/Node 20、上游发布逻辑保留。未执行这些远端流程，也未复跑 v6.0 或 8 板矩阵。后续变更公共 HAL/显示契约时保留原板回归；Read Pico 诊断已加入新 CI；远端未运行。

## Git 状态与保护

源码引入前仓库 `main` 尚无提交，index 两项为工作树缺失的 `.gitmodules`/Read Pico gitlink。完整 `.git` 与当前文档先备份至忽略目录 `.cache/git-before-photoframe-2026-10-03.tar.gz`、`.cache/docs-before-photoframe-2026-10-03.tar.gz`；摘要见 VALIDATION。旧子模块对象库保留，未覆盖。

当前分支 `codex/read-pico-port` 基于 PhotoFrame 固定 commit；原根文档保留，上游 README 另存 `README.upstream.md`。Read Pico 子模块逻辑名为 `read_pico_reference`，避开旧对象库路径。新增只读源码远端 `photoframe-upstream`；本轮未创建项目提交或推送。

新增图像映射、格式、路径或状态逻辑运行对应主机测试；硬件适配需编译并另做真机验证。构建、SDK、生成资源、日志和私人素材不入库；依赖锁属于需要保留的版本元数据，不是固件产物。
