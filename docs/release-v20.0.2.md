编辑器快捷设置插件：不打开 *首选项*，直接在 **工具** 菜单完成两件日常小事——切换默认文件编码、切换编辑器字号。

## 功能

**工具 → 文件编码**：`UTF-8` / `GBK` 两项可勾选，勾选状态始终反映当前默认编码，不管是在这里改的还是在 *首选项 > 文本编辑器 > 行为* 里改的。
点击后用新编码重新读入所有已打开文件；若其中有未保存的修改，会先询问「全部重新打开 / 跳过已修改的文件 / 取消」，
重新打开本身是一个可撤销的编辑块。

**工具 → 文字字号**：`10` / `11` / `12` / `14` / `16` 五档。改完所有已打开的编辑器立即重排，
不重读文件、不丢撤销栈，也不弹未保存确认。

## 安装

1. 关闭 Qt Creator；
2. 解压，把包里的 `lib` 目录合并进 Qt Creator 安装根目录，最终为
   `C:\Qt\Tools\QtCreator-20.0.2\lib\qtcreator\plugins\FileEncoding.dll`；
3. 重启 IDE，「工具」菜单下即出现「文件编码」与「文字字号」。

卸载只需删除那一个 DLL。

## 适用范围

**仅适用于 Qt Creator 20.0.2 x64（MSVC 2022 构建的官方安装包）**。插件与宿主共享 C++ ABI 并导入
`TextEditor.dll`、`Core.dll` 的符号，换编译器或跨大版本（21.x）需要按 README 自行编译。
GBK 解码依赖 Qt 自带的 ICU，官方构建均已包含。

## 校验

```
sha256(FileEncoding-20.0.2-win64.zip) = 5d88a184359ab8ac6380750640d2f2250e86d0c8c76edc0a3cb68f74c5b6cbac
```

包内仅有 `lib/qtcreator/plugins/FileEncoding.dll`、`README.txt`、`LICENSE.txt`、`SHA256.txt`。

---

Switches the default text encoding between UTF-8 and GBK and picks a font size between
10 and 16 pt, both straight from the **Tools** menu, without opening the preferences dialog.
Reopening files is only done for the encoding change, where it is actually required.
Built for Qt Creator 20.0.2 x64 with MSVC 2022.
