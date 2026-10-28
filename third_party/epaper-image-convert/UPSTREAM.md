# epaper-image-convert（本地引入副本）

- 来源：npm `@aitjcize/epaper-image-convert` 0.1.20（<https://github.com/aitjcize/epaper-image-convert>），MIT，原 LICENSE 随附。
- 引入方式：从 `webapp/node_modules` 中按 `webapp/package-lock.json` 完整性校验安装的 0.1.20 副本原样复制 `package.json`、`LICENSE`、`README.md`、`src/`（2026-10-25），原样文件 SHA256 见下。
- 使用方式：`webapp` 通过 Vite `resolve.alias` 使用本目录；`process-cli` 仍使用 npm 0.1.20，未切换。
- 本项目修改（单独列出，未向上游提交）：
  1. `makeGrayscale16()` 新增可选 `levelsY`（16 个实测相对亮度），按码直接生成 perceived 灰阶。
  2. `findClosestColorRGB/LAB` 仅对非灰阶调色板跳过下标 4（六色屏保留位）；灰阶模式 16 码全部可用。

原样文件 SHA256：

```
9f63bc59c7ae35ba4a7c65d609e7bec70c03af559daf871b450ee35bcd02a331  LICENSE
a59ad49047477650de3d0fc86203aa84e2eeb1e4adf819603461bbbb916fe7ee  README.md
9b786ea1a2b1af3a57975f66242aeede2a0f4369ff2213bb34c7dfc82dc37ae5  package.json
8ad534eced96d043970710d079f5be5af5d16740ac621a18c27cd1f06df35c75  src/cli.js
e6376a6ce6b8ebe060f44f96c16d45a97e1eef5056b222504983d701ab8f71fb  src/index.js
ce277634d487514502d07f7f88b5c65c72ae347f5f775a055c8e58e8aebfae5e  src/palettes.js
491940ac32d5a79785fda1a7d86a73828c0e00d9be3efdda3eeb666227a6018c  src/presets.js
852fae11d36fafa7e8d017306138033149f23c04fa757151f1e8d7a25067b83c  src/processor.js
```
