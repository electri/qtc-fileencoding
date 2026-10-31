// Copyright (C) 2026 The File Encoding Plugin Authors
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileencodingconstants.h"
#include "fileencodingtr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>

#include <texteditor/extraencodingsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/textcodec.h>

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>

using namespace Utils;

namespace FileEncoding::Internal {

namespace {

// What happened while re-reading the open documents.
struct ReloadSummary
{
    int reloaded = 0;
    int decodingErrors = 0;
    QStringList failed;
};

/*!
    Returns the GBK encoding of the current Qt build.

    GBK is not a built-in Qt encoding, it comes in through the ICU based
    QStringDecoder. GB18030 is a strict superset of GBK and is used as a
    fallback in case only that one is available.
*/
TextEncoding gbkEncoding()
{
    static const TextEncoding gbk = [] {
        for (const char *name : {"GBK", "GB18030", "CP936"}) {
            const TextEncoding candidate{QByteArray(name)};
            if (candidate.isValid())
                return candidate;
        }
        return TextEncoding();
    }();
    return gbk;
}

/*!
    Refreshes the codec label in the status bar of all editors showing \a textDocument.

    The label only listens to modificationChanged of the text document, which a
    plain encoding change or a reload does not necessarily emit.
*/
void updateCodecLabels(TextEditor::TextDocument *textDocument)
{
    const QList<TextEditor::BaseTextEditor *> editors =
        TextEditor::BaseTextEditor::textEditorsForDocument(textDocument);
    for (TextEditor::BaseTextEditor *editor : editors)
        editor->editorWidget()->updateTextCodecLabel();
}

} // namespace

class FileEncodingPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "FileEncoding.json")

public:
    FileEncodingPlugin() = default;
    ~FileEncodingPlugin() final = default;

    void initialize() final;
    void extensionsInitialized() final;

private:
    void switchToEncoding(const TextEncoding &encoding);
    void updateCheckedState();
    void reloadOpenDocuments(const TextEncoding &encoding, ReloadSummary &summary);

    QAction *m_utf8Action = nullptr;
    QAction *m_gbkAction = nullptr;
};

void FileEncodingPlugin::initialize()
{
    Core::MenuBuilder(Constants::MENU_ID)
        .setTitle(Tr::tr("文件编码(&E)"))
        .setOnAllDisabledBehavior(Core::ActionContainer::Show)
        .addToContainer(Core::Constants::M_TOOLS);

    const TextEncoding utf8{TextEncoding::Utf8};

    Core::ActionBuilder(this, Constants::ACTION_SWITCH_TO_UTF8)
        .setText(Tr::tr("UTF-8"))
        .setContext(Core::Constants::C_GLOBAL)
        .setCheckable(true)
        .bindContextAction(&m_utf8Action)
        .addToContainer(Constants::MENU_ID)
        .addOnTriggered(this, [this, utf8] { switchToEncoding(utf8); });

    Core::ActionBuilder(this, Constants::ACTION_SWITCH_TO_GBK)
        .setText(Tr::tr("GBK"))
        .setContext(Core::Constants::C_GLOBAL)
        .setCheckable(true)
        .bindContextAction(&m_gbkAction)
        .addToContainer(Constants::MENU_ID)
        .addOnTriggered(this, [this] { switchToEncoding(gbkEncoding()); });
}

void FileEncodingPlugin::extensionsInitialized()
{
    // The default encoding can also be changed in the settings dialog, so the
    // check marks are refreshed whenever the menu is about to be shown.
    if (Core::ActionContainer *menu = Core::ActionManager::actionContainer(Constants::MENU_ID)) {
        if (QMenu *menuWidget = menu->menu())
            connect(menuWidget, &QMenu::aboutToShow, this, &FileEncodingPlugin::updateCheckedState);
    }
    updateCheckedState();
}

void FileEncodingPlugin::updateCheckedState()
{
    if (!m_utf8Action || !m_gbkAction)
        return;

    const TextEncoding current = Core::EditorManager::defaultTextEncoding();
    const TextEncoding gbk = gbkEncoding();
    m_utf8Action->setChecked(current.isUtf8());
    m_gbkAction->setChecked(gbk.isValid() && current == gbk);
}

void FileEncodingPlugin::switchToEncoding(const TextEncoding &encoding)
{
    if (!encoding.isValid()) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             Tr::tr("文件编码"),
                             Tr::tr("当前 Qt 构建不提供 GBK 编码，无法切换。"));
        updateCheckedState();
        return;
    }

    TextEditor::ExtraEncodingSettings &settings = TextEditor::globalExtraEncodingSettings();
    settings.defaultEncoding.setValue(encoding);
    settings.apply();

    // Make sure the value really ended up where EditorManager reads it from,
    // otherwise the switch would only look successful.
    if (Core::EditorManager::defaultTextEncoding() != encoding) {
        Core::MessageManager::writeFlashing(
            Tr::tr("无法保存默认文件编码（%1）。").arg(encoding.displayName()));
        updateCheckedState();
        return;
    }

    ReloadSummary summary;
    reloadOpenDocuments(encoding, summary);
    updateCheckedState();

    QStringList notes;
    if (summary.reloaded > 0)
        notes << Tr::tr("重新打开了 %1 个文件").arg(summary.reloaded);
    if (summary.decodingErrors > 0)
        notes << Tr::tr("%1 个文件在该编码下无法完整解码").arg(summary.decodingErrors);
    if (!summary.failed.isEmpty())
        notes << Tr::tr("%1 个文件重新打开失败").arg(summary.failed.size());

    QString message = Tr::tr("默认文件编码已切换为 %1").arg(encoding.displayName());
    if (!notes.isEmpty())
        message += QStringLiteral("：") + notes.join(QStringLiteral("，"));
    message += QStringLiteral("。");

    if (QStatusBar *statusBar = Core::ICore::statusBar())
        statusBar->showMessage(message, 5000);

    if (!summary.failed.isEmpty()) {
        Core::MessageManager::writeFlashing(
            message + QLatin1Char('\n') + summary.failed.join(QLatin1Char('\n')));
    }
}

/*!
    Re-reads all open text documents whose encoding differs from \a encoding.

    Documents with unsaved modifications are only reloaded after an explicit
    confirmation, since reloading drops the current content. The reload itself
    is an undoable edit, so the content is not lost for good.
*/
void FileEncodingPlugin::reloadOpenDocuments(const TextEncoding &encoding, ReloadSummary &summary)
{
    QList<TextEditor::TextDocument *> pending;
    int modifiedCount = 0;

    for (Core::IDocument *document : Core::DocumentModel::openedDocuments()) {
        auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
        if (!textDocument || textDocument->encoding() == encoding)
            continue;

        // A never saved document has no bytes to re-read, but it should be
        // written in the new encoding once it gets saved.
        if (textDocument->filePath().isEmpty()) {
            textDocument->setEncoding(encoding);
            updateCodecLabels(textDocument);
            continue;
        }

        pending.append(textDocument);
        if (textDocument->isModified())
            ++modifiedCount;
    }

    if (pending.isEmpty())
        return;

    bool skipModified = false;
    if (modifiedCount > 0) {
        QMessageBox box(Core::ICore::mainWindow());
        box.setWindowTitle(Tr::tr("文件编码"));
        box.setIcon(QMessageBox::Question);
        box.setText(Tr::tr("有 %1 个已打开的文件包含未保存的修改。").arg(modifiedCount));
        box.setInformativeText(Tr::tr("重新打开这些文件会放弃修改。重新打开后仍可在各自的编辑器中撤销。"));

        auto reloadAllButton = new QPushButton(Tr::tr("全部重新打开"));
        auto skipButton = new QPushButton(Tr::tr("跳过已修改的文件"));
        box.addButton(reloadAllButton, QMessageBox::AcceptRole);
        box.addButton(skipButton, QMessageBox::RejectRole);
        QPushButton *cancelButton = box.addButton(QMessageBox::Cancel);
        box.setDefaultButton(skipButton);
        box.setEscapeButton(cancelButton);
        box.exec();

        if (box.clickedButton() == skipButton)
            skipModified = true;
        else if (box.clickedButton() != reloadAllButton)
            return; // cancelled by the user, keep everything as it is
    }

    for (TextEditor::TextDocument *textDocument : pending) {
        if (skipModified && textDocument->isModified())
            continue;

        const Result<> result = textDocument->reload(encoding);
        if (!result) {
            summary.failed.append(textDocument->filePath().toUserOutput());
            continue;
        }

        if (textDocument->hasDecodingError())
            ++summary.decodingErrors;

        // A byte order mark wins over the requested encoding, so such a file
        // was read again, but not interpreted as requested.
        if (textDocument->encoding() == encoding)
            ++summary.reloaded;

        updateCodecLabels(textDocument);
    }
}

} // namespace FileEncoding::Internal

#include "fileencodingplugin.moc"
