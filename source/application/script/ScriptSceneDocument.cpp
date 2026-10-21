#include "ScriptSceneDocument.h"

#include <Inventor/nodes/SoGroup.h>
#include <Inventor/actions/SoWriteAction.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

#include "kernel/scene/TSceneKit.h"
#include "core/TonatiuhCore.h"
#include "kernel/scene/TSeparatorKit.h"

ScriptSceneDocument::ScriptSceneDocument()
{
    TonatiuhCore::initializeCoin();
    TonatiuhCore::initializeCoreTypes();
    clear();
}

bool ScriptSceneDocument::clear(QString* errorMessage)
{
    if (errorMessage)
        errorMessage->clear();
    // LoadedScene::reset refs the new root and unreferences the old one.
    // Script NodeObject wrappers pin any previously exposed Coin nodes.
    m_scene.reset(new TSceneKit);
    return m_scene.get() != nullptr;
}

bool ScriptSceneDocument::insert(SoNode* node, QString* errorMessage)
{
    auto fail = [errorMessage](const QString& message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };

    if (!node)
        return fail("Script document insertion requires a node.");
    TSceneKit* root = m_scene.get();
    TSeparatorKit* layout = root ? root->getLayout() : nullptr;
    if (!layout)
        return fail("Script document has no scene layout.");
    if (node == root || node == layout)
        return fail("Script document cannot insert its own scene root or layout.");
    auto* group = dynamic_cast<SoGroup*>(layout->getPart("group", true));
    if (!group)
        return fail("Script document layout has no child group.");

    // Match the GUI's root insertion when no scene-tree selection is active.
    // Coin's group takes a strong reference to the inserted node.
    group->addChild(node);
    root->updateParents(); // Establish tracker parent links, if present.
    if (errorMessage)
        errorMessage->clear();
    return true;
}

bool ScriptSceneDocument::load(const QString& fileName, QString* errorMessage)
{
    // Parse into a separate owner. A failed read must not clear or corrupt the
    // active in-memory scene (including wrappers pinning its old Coin nodes).
    LoadedScene replacement;
    if (!SceneLoader::readFile(fileName, &replacement, errorMessage))
        return false;

    // Mirror MainWindow::ChangeModelScene / openFileProject preparation.
    replacement.get()->updateParents();
    replacement.get()->updateTrackers();
    m_scene.reset(replacement.get()); // Both owners hold a reference until exit.
    if (errorMessage)
        errorMessage->clear();
    return true;
}

bool ScriptSceneDocument::save(const QString& fileName, QString* errorMessage) const
{
    return writeScene(m_scene.get(), fileName, errorMessage);
}

bool ScriptSceneDocument::writeScene(TSceneKit* scene, const QString& fileName,
                                     QString* errorMessage)
{
    auto fail = [errorMessage](const QString& message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };
    if (!scene)
        return fail("No active scene to save.");
    const QFileInfo destination(fileName);
    if (destination.suffix().compare(QStringLiteral("tnhpp"), Qt::CaseInsensitive) != 0)
        return fail("Only .tnhpp scene files are supported by tn.saveScene.");
    if (destination.exists())
        return fail(QString("Scene file already exists; refusing to overwrite %1.")
                        .arg(destination.absoluteFilePath()));
    if (!destination.absoluteDir().exists())
        return fail(QString("Scene save directory does not exist: %1.")
                        .arg(destination.absolutePath()));

    // Coin writes to a temporary path on the same filesystem. Do not expose
    // a truncated/partial output as a .tnhpp scene if serialization fails.
    QTemporaryFile temporary(destination.absoluteDir().filePath(
        QStringLiteral(".tonatiuhpp-scene-XXXXXX.tnhpp")));
    if (!temporary.open())
        return fail(QString("Cannot create temporary scene file in %1: %2")
                        .arg(destination.absolutePath(), temporary.errorString()));
    const QString temporaryName = temporary.fileName();
    temporary.close();

    SoWriteAction action;
    const QByteArray encoded = QFile::encodeName(temporaryName);
    if (!action.getOutput()->openFile(encoded.constData()))
        return fail(QString("Cannot open temporary scene file %1.").arg(temporaryName));
    action.getOutput()->setBinary(false);
    // Preserve the established GUI project's written precision.
    action.getOutput()->setFloatPrecision(6);
    action.apply(scene);
    action.getOutput()->closeFile();

    if (QFileInfo(temporaryName).size() == 0)
        return fail("Scene serialization produced an empty file.");
    LoadedScene verified;
    QString validationError;
    if (!SceneLoader::readFile(temporaryName, &verified, &validationError))
        return fail(QString("Saved scene failed validation: %1").arg(validationError));

    // QTemporaryFile retains control of its file handle after close(). Use
    // its own atomic, no-overwrite rename to publish the verified scene.
    if (QFileInfo::exists(fileName) || !temporary.rename(fileName))
        return fail(QString("Cannot publish scene file without overwriting %1: %2")
                        .arg(destination.absoluteFilePath(), temporary.errorString()));
    // rename() does not disable QTemporaryFile's automatic deletion.
    temporary.setAutoRemove(false);
    if (errorMessage)
        errorMessage->clear();
    return true;
}
