#include "SceneLoader.h"

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/nodes/SoSeparator.h>

#include <QFile>

#include "kernel/scene/TSceneKit.h"

LoadedScene::~LoadedScene()
{
    reset();
}

TSceneKit* LoadedScene::release()
{
    TSceneKit* scene = m_scene;
    m_scene = nullptr;
    return scene;
}

void LoadedScene::reset(TSceneKit* scene)
{
    if (m_scene)
        m_scene->unref();

    m_scene = scene;
    if (m_scene)
        m_scene->ref();
}

bool SceneLoader::readFile(const QString& fileName, LoadedScene* scene, QString* errorMessage)
{
    auto fail = [errorMessage](const QString& message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };

    if (scene)
        scene->reset();

    if (fileName.isEmpty())
        return fail("Scene file path is empty.");

    SoInput input;
    const QByteArray encodedFileName = QFile::encodeName(fileName);
    if (!input.openFile(encodedFileName.constData()))
        return fail(QString("Cannot open file %1.").arg(fileName));

    if (!input.isValidFile()) {
        input.closeFile();
        return fail(QString("Error reading file %1.").arg(fileName));
    }

    SoSeparator* separator = SoDB::readAll(&input);
    input.closeFile();

    if (!separator)
        return fail(QString("Error reading file %1.").arg(fileName));

    separator->ref();
    auto failWithSeparator = [separator, &fail](const QString& message) {
        separator->unref();
        return fail(message);
    };

    if (separator->getNumChildren() < 1)
        return failWithSeparator(QString("Error reading file %1: missing Tonatiuh++ scene root.").arg(fileName));

    TSceneKit* loadedScene = dynamic_cast<TSceneKit*>(separator->getChild(0));
    if (!loadedScene)
        return failWithSeparator(QString("Error reading file %1: invalid Tonatiuh++ scene root.").arg(fileName));

    SoSFString* versionField = dynamic_cast<SoSFString*>(loadedScene->getField("version"));
    if (!versionField)
        return failWithSeparator(QString("Error reading file %1: missing Tonatiuh++ project version.").arg(fileName));

    const QString version = versionField->getValue().getString();
    if (version != "2020")
        return failWithSeparator(QString("Version %1 is not compatible.").arg(version));

    if (scene)
        scene->reset(loadedScene);

    separator->removeChild(loadedScene);
    separator->unref();
    return true;
}
