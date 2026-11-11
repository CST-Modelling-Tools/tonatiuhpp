#pragma once

// Resolve packaged example/resource directories or the source-tree equivalents
// when running a CMake build. No working-directory or installed prefix assumptions.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace TonatiuhppAppDataPaths {

inline QString findDataDirectory(const QString& relativePath,
                                 const QString& executableDirectory = QCoreApplication::applicationDirPath())
{
    if (relativePath.isEmpty() || QDir::isAbsolutePath(relativePath)
        || relativePath.split('/').contains(QStringLiteral("..")))
        return QString();

    const QDir executableDir(executableDirectory);

    // An installed application takes precedence over any nearby build tree.
#ifdef Q_OS_MACOS
    const QString packaged = executableDir.absoluteFilePath(QStringLiteral("../Resources/") + relativePath);
#else
    const QString packaged = executableDir.absoluteFilePath(QStringLiteral("../") + relativePath);
#endif
    const QFileInfo packagedInfo(packaged);
    if (packagedInfo.isDir())
        return packagedInfo.canonicalFilePath();

    // Build-tree executables now live in build/bin/<CONFIG> (MSVC) or build/bin
    // (Ninja). Walk upward to CMakeCache.txt; its CMAKE_HOME_DIRECTORY points to
    // the actual source folder even if the build directory is out of source.
    QDir ancestor(executableDirectory);
    for (int level = 0; level != 8; ++level) {
        QFile cache(ancestor.absoluteFilePath(QStringLiteral("CMakeCache.txt")));
        if (cache.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString key = QStringLiteral("CMAKE_HOME_DIRECTORY:INTERNAL=");
            while (!cache.atEnd()) {
                const QString line = QString::fromUtf8(cache.readLine()).trimmed();
                if (!line.startsWith(key))
                    continue;
                const QString sourceDirectory = line.mid(key.size()).trimmed();
                const QFileInfo sourceDataInfo(
                    QDir(sourceDirectory).absoluteFilePath(QStringLiteral("../") + relativePath));
                if (sourceDataInfo.isDir())
                    return sourceDataInfo.canonicalFilePath();
                break;
            }
            return QString(); // CMake tree found, but the requested data is absent.
        }
        if (!ancestor.cdUp())
            break;
    }
    return QString(); // The caller should show an explicit missing-data error.
}

} // namespace TonatiuhppAppDataPaths

namespace TonatiuhppAppDataPaths {

// Unlike a directory lookup, a Help action must verify the actual HTML file.
// Returning empty makes a missing Sphinx build an explicit, actionable error.
inline QString findDataFile(const QString& relativePath,
                            const QString& executableDirectory = QCoreApplication::applicationDirPath())
{
    if (relativePath.isEmpty() || QDir::isAbsolutePath(relativePath)
        || relativePath.split('/').contains(QStringLiteral("..")))
        return QString();

    const QFileInfo relativeInfo(relativePath);
    if (relativeInfo.path() == QStringLiteral(".") || relativeInfo.fileName().isEmpty())
        return QString();

    const QString directory = findDataDirectory(relativeInfo.path(), executableDirectory);
    if (directory.isEmpty())
        return QString();

    const QFileInfo file(QDir(directory).absoluteFilePath(relativeInfo.fileName()));
    return file.isFile() ? file.canonicalFilePath() : QString();
}

} // namespace TonatiuhppAppDataPaths
