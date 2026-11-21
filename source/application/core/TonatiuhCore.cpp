#include "TonatiuhCore.h"

#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/nodekits/SoNodeKit.h>

#include <QCoreApplication>
#include <QJsonObject>
#include <atomic>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QSet>

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

// A single deterministic directory walk keeps factory discovery consistent
// between graphical and nonvisual adapters. Avoid symlink-directory cycles.
void collectPluginLibraries(const QDir& directory, QStringList& files,
                            QSet<QString>& seenDirectories, QSet<QString>& seenFiles)
{
    const QString canonical = directory.canonicalPath();
    if (canonical.isEmpty() || seenDirectories.contains(canonical))
        return;
    seenDirectories.insert(canonical);

    const QFileInfoList entries = directory.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& entry : entries) {
        if (entry.isDir()) {
            if (!entry.isSymLink())
                collectPluginLibraries(QDir(entry.absoluteFilePath()), files,
                                       seenDirectories, seenFiles);
            continue;
        }
        if (!QLibrary::isLibrary(entry.absoluteFilePath()))
            continue;
        const QString key = entry.canonicalFilePath().isEmpty()
            ? entry.absoluteFilePath() : entry.canonicalFilePath();
        if (!seenFiles.contains(key)) {
            seenFiles.insert(key);
            files.append(entry.absoluteFilePath());
        }
    }
}
}

namespace {
std::atomic<bool> s_headlessPluginMode{false};
}

namespace TonatiuhCore
{
void setHeadlessPluginMode(bool enabled) { s_headlessPluginMode.store(enabled); }
bool headlessPluginMode() { return s_headlessPluginMode.load(); }
bool allowsPluginMetadata(const QJsonObject& metadata)
{
    if (!headlessPluginMode()) return true;
    // This compatibility declaration is cooperative metadata, not a
    // cryptographic trust or sandbox boundary for third-party code.
    return metadata.value("MetaData").toObject()
        .value("TonatiuhppHeadlessABI").toInt() == 1;
}

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
    // TONATIUHPP_P1_CENTRAL_WIN_RUNTIME: Windows build-tree plugins
    // now reside beside all executables in appDir/plugins.

#ifdef Q_OS_MACOS
    // Installed .app/Contents/MacOS -> installed <prefix>/bin, where the
    // same first-party scientific plugin binaries serve the console runtime.
    // Only enable this side path for an actual .app bundle (Info.plist).
    if (QFileInfo(appDir.absoluteFilePath("../Info.plist")).isFile()) {
        appendExistingPath(paths, appDir.absoluteFilePath("../../../bin"));
        appendExistingPath(paths, appDir.absoluteFilePath("../PlugIns"));
    }
#endif
#ifdef Q_OS_MACOS
    // The uninstalled CMake bundle lives at
    // build/application/TonatiuhPP.app/Contents/MacOS, while its scene
    // plugins are emitted into build/plugins. Do not use this extra path
    // for installed bundles that do not have an adjacent CMake build tree.
    if (QFileInfo(appDir.absoluteFilePath("../../../CMakeFiles")).isDir())
        appendExistingPath(paths, appDir.absoluteFilePath("../../../../plugins"));
#endif
    appendExistingPath(paths, applicationDirPath);

    return paths;
}

QStringList pluginLibraryFiles(const QStringList& searchRoots)
{
    QStringList files;
    QSet<QString> seenDirectories;
    QSet<QString> seenFiles;
    for (const QString& root : searchRoots)
        collectPluginLibraries(QDir(root), files, seenDirectories, seenFiles);
    return files;
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
