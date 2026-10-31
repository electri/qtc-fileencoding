# File Encoding plugin for Qt Creator 20.0.2

**English** | [简体中文](README-zh.md)

Switches the default text file encoding of Qt Creator between **UTF-8** and **GBK**
straight from the **Tools** menu, without opening *Preferences > Text Editor > Behavior*,
and re-reads the files that are already open with the new encoding.

```
Tools
└── 文件编码            (File Encoding)
    ├── ✓ UTF-8
    └──   GBK
```

The two entries are checkable and always show the encoding that is currently configured
as default, no matter whether it was changed here or in the preferences dialog.

> The repository root is `src/plugins/fileencoding/` of a Qt Creator source tree.
> The plugin has to be built together with the IDE (the `add_qtc_plugin` macro, the same Qt,
> the same compiler), so the way to use it is to drop it into a source tree.

---

## 1. What it does

A click on **UTF-8** or **GBK**:

1. writes the encoding to the very same setting the preferences page uses
   (`General/DefaultFileEncoding`), including the in-memory copy of the settings page,
   so both stay in sync;
2. re-reads every open text document whose current encoding differs from the new default,
   so files that were displayed as mojibake become readable immediately;
3. reports the result in the status bar (`默认文件编码已切换为 GBK：重新打开了 3 个文件。`).

Details that are easy to get wrong:

| Situation | Behaviour |
|---|---|
| File with a byte order mark | The BOM wins, the file keeps its UTF-8/UTF-16/UTF-32 encoding |
| Document with unsaved changes | A dialog asks: reopen all / skip the modified ones (default) / cancel. Reopening itself is an undoable edit, so the old content is not lost for good |
| Document that was never saved | Has no bytes to re-read, only its *save* encoding is updated |
| Document that is not a text document (image, binary, form editor, ...) | Untouched |
| File cannot be decoded in the new encoding | Counted in the status message and listed in the output pane |
| Encoding is already the default | Still re-reads documents that are stuck in the other encoding, so the action doubles as a repair button |

Because the labels are Chinese string literals in the source, the menu reads
`文件编码 / UTF-8 / GBK` in any UI language, including an English one.

## 2. Installation

```bash
cd <qtcreator-source>
git clone https://github.com/electri/qtc-fileencoding.git src/plugins/fileencoding
```

Then add the subdirectory to `src/plugins/CMakeLists.txt` (the list is grouped by
dependency level; `fileencoding` depends only on `Core` and `TextEditor`, so any level
from 2 upwards works — this repository is normally placed next to `astyle`):

```cmake
add_subdirectory(fileencoding)
```

Reconfigure CMake afterwards.

## 3. Building

```bash
cmake --build <build-dir> --target FileEncoding
```

The result is `lib/qtcreator/plugins/FileEncoding.dll` (`libFileEncoding.so` on Linux,
`libFileEncoding.dylib` on macOS) inside the build directory, which is where the IDE
expects it.

**Close Qt Creator before relinking** — on Windows a running IDE keeps the previous
`FileEncoding.dll` mapped and the linker fails with `LNK1104`.

## 4. Requirements

- Qt Creator **20.0.2** sources (`CompatVersion` is `20.0.0`).
- A Qt build whose `QStringDecoder` knows **GBK**. GBK is not one of Qt's built-in
  encodings, it comes in through ICU; the plugin checks the name at runtime and falls
  back to `GB18030` and `CP936` (a superset of GBK) if it is not offered under the
  plain name. If none of them is available, the `GBK` action reports that instead of
  corrupting files.

## 5. Layout

```
(repository root = src/plugins/fileencoding/ of the source tree)
├── CMakeLists.txt                 # add_qtc_plugin(FileEncoding ...)
├── FileEncoding.json.in           # plugin metadata, version and dependencies filled in by CMake
├── fileencodingconstants.h        # menu and action ids
├── fileencodingplugin.cpp         # the whole plugin
├── fileencodingtr.h               # Tr::tr context
├── LICENSE
└── LICENSES/
    └── LICENSE.GPL3-EXCEPT
```

## 6. License

GPL-3.0-only WITH Qt-GPL-exception-1.0, the same terms as Qt Creator itself.
See [LICENSE](LICENSE) and [LICENSES/LICENSE.GPL3-EXCEPT](LICENSES/LICENSE.GPL3-EXCEPT).

Qt and Qt Creator are trademarks of The Qt Company Ltd. This plugin is not affiliated
with or endorsed by The Qt Company.
