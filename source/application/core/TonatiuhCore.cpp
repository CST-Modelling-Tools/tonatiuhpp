#include "TonatiuhCore.h"

#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/nodekits/SoNodeKit.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include "kernel/node/TNode.h"
#include "kernel/scene/TSceneKit.h"
#include "libraries/Coin3D/MFVec2.h"
#include "libraries/Coin3D/UserMField.h"
#include "libraries/Coin3D/UserSField.h"

namespace
{
void appendExistingPath(QStringList& paths, const QString& path)
{
    QFileInfo info(path);
    if (!info.exists() || !info.isDir())
        return;

    const QString absolutePath = QDir(path).absolutePath();
    if (!paths.contains(absolutePath))
        paths << absolutePath;
}
}

namespace TonatiuhCore
{
void initializeCoin()
{
    static bool initialized = false;
    if (initialized)
        return;

    SoDB::init();
    SoNodeKit::init();
    SoInteraction::init();
    initialized = true;
}

void initializeCoreTypes()
{
    static bool initialized = false;
    if (initialized)
        return;

    UserMField::initClass();
    UserSField::initClass();
    MFVec2::initClass();
    TNode::initClass();
    TSceneKit::initClass();

    initialized = true;
}

QStringList pluginSearchPaths(const QString& applicationDirPath)
{
    QStringList paths;
    QDir appDir(applicationDirPath);

    appendExistingPath(paths, appDir.absoluteFilePath("plugins"));
    appendExistingPath(paths, appDir.absoluteFilePath("../plugins"));
    appendExistingPath(paths, applicationDirPath);

    return paths;
}

void setProjectSearchPaths(const QString& fileName)
{
    QStringList searchPaths;

    QFileInfo info(fileName);
    if (info.exists())
        searchPaths << info.absolutePath();

    searchPaths << QDir::currentPath();
    searchPaths << QCoreApplication::applicationDirPath();

    QDir::setSearchPaths("project", searchPaths);
}
}
