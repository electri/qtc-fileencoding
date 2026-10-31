# Qt Creator 20.0.2 文件编码插件

[English](README.md) | **简体中文**

不打开 *首选项 > 文本编辑器 > 行为*，直接从 **工具** 菜单在 **UTF-8** 与 **GBK** 之间切换
Qt Creator 的默认文件编码，并把已经打开的文件按新编码重新读入。

```
工具
└── 文件编码
    ├── ✓ UTF-8
    └──   GBK
```

两个菜单项可勾选，勾选状态始终反映当前的默认编码——不管它是在这里改的，还是在首选项里改的。

> 仓库根目录对应源码树里的 `src/plugins/fileencoding/`。
> 插件必须和 IDE 一起编译（`add_qtc_plugin` 宏、同一份 Qt、同一个编译器），
> 所以用法是把它放进源码树。

---

## 1. 它做了什么

点击 **UTF-8** 或 **GBK** 之后：

1. 把编码写进首选项用的是同一个设置项（`General/DefaultFileEncoding`），并且同步更新设置页
   内存里的那一份，两边不会打架；
2. 把所有「当前编码与新默认编码不同」的已打开文本文档重新读一遍，之前显示成乱码的文件立刻可读；
3. 在状态栏给出结果（`默认文件编码已切换为 GBK：重新打开了 3 个文件。`）。

几个容易踩的细节：

| 情况 | 行为 |
|---|---|
| 文件带 BOM | BOM 优先，文件保持它自己的 UTF-8/UTF-16/UTF-32 编码 |
| 文档有未保存修改 | 弹窗三选一：全部重新打开 / 跳过已修改的文件（默认）/ 取消。重新打开本身是一次可撤销的编辑，旧内容还能撤销回来 |
| 从未保存过的新文档 | 没有字节可读，只更新它将来保存时用的编码 |
| 非文本文档（图片、二进制、表单编辑器……） | 不处理 |
| 文件在新编码下无法解码 | 计入状态栏提示，并在输出面板列出文件名 |
| 当前默认编码已经是目标编码 | 仍会重读那些卡在另一种编码里的文档，相当于一个「修复」按钮 |

菜单文案是源码里的中文字面量，所以在任何界面语言下（包括英文界面）都显示
`文件编码 / UTF-8 / GBK`。

## 2. 安装

### 现成包（Windows x64，免编译）

[`dist/FileEncoding-20.0.2-win64.zip`](dist/FileEncoding-20.0.2-win64.zip)（约 34 KB）是编译好的精简包，
里面只有 `lib/qtcreator/plugins/FileEncoding.dll`、`README.txt`、`LICENSE.txt` 和 `SHA256.txt`：

1. 关闭 Qt Creator；
2. 把压缩包里的 `lib` 目录合并进 Qt Creator 安装根目录，最终为
   `<安装目录>\lib\qtcreator\plugins\FileEncoding.dll`（例如
   `C:\Qt\Tools\QtCreator-20.0.2\lib\qtcreator\plugins\`）；
3. 重启 IDE，「工具 → 文件编码」即可使用。

这个包只适用于 **Qt Creator 20.0.2 x64、MSVC 2022 构建的版本**——插件与宿主共享 C++ ABI，
换编译器或跨大版本（21.x）必须按下文自行编译。卸载只需删掉那个 DLL。

### 从源码编译安装

```bash
cd <qtcreator源码目录>
git clone https://github.com/electri/qtc-fileencoding.git src/plugins/fileencoding
```

然后在 `src/plugins/CMakeLists.txt` 里加上子目录（该文件按依赖层级分组，
`fileencoding` 只依赖 `Core` 和 `TextEditor`，第 2 层往上都行；本仓库一般和 `astyle` 放一起）：

```cmake
add_subdirectory(fileencoding)
```

之后重新跑一次 CMake 配置。

## 3. 编译

```bash
cmake --build <构建目录> --target FileEncoding
```

产物是构建目录里的 `lib/qtcreator/plugins/FileEncoding.dll`（Linux 为
`libFileEncoding.so`，macOS 为 `libFileEncoding.dylib`），正是 IDE 期望的位置。

**重新链接前先关掉 Qt Creator**——Windows 上运行中的 IDE 映射着旧的
`FileEncoding.dll`，链接会报 `LNK1104`。

## 4. 环境要求

- Qt Creator **20.0.2** 源码（`CompatVersion` 是 `20.0.0`）。
- 所用的 Qt 里 `QStringDecoder` 认识 **GBK**。GBK 不是 Qt 内置编码，靠 ICU 提供；
  插件运行时会检查，若没有以该名字提供则依次回退到 `GB18030`、`CP936`（GBK 的超集）。
  三者都没有时，`GBK` 菜单项会给出提示，而不是把文件读坏。

## 5. 目录结构

```
（仓库根目录 = 源码树的 src/plugins/fileencoding/）
├── CMakeLists.txt                 # add_qtc_plugin(FileEncoding ...)
├── FileEncoding.json.in           # 插件元数据，版本与依赖由 CMake 填充
├── fileencodingconstants.h        # 菜单与命令 Id
├── fileencodingplugin.cpp         # 插件的全部实现
├── fileencodingtr.h               # Tr::tr 上下文
├── LICENSE
└── LICENSES/
    └── LICENSE.GPL3-EXCEPT
```

## 6. 许可

GPL-3.0-only WITH Qt-GPL-exception-1.0，与 Qt Creator 本身一致。
见 [LICENSE](LICENSE) 与 [LICENSES/LICENSE.GPL3-EXCEPT](LICENSES/LICENSE.GPL3-EXCEPT)。

Qt 和 Qt Creator 是 The Qt Company Ltd. 的商标。本插件与 The Qt Company 无关，也未获其背书。
