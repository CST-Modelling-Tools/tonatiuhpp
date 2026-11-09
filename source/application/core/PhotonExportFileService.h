#pragma once

// M4.1: nonvisual File-export discovery and fail-closed output preflight.
// The same implementation is used by direct CLI tracing and GUI/headless
// scripts. It does not instantiate widgets or change any plugin ABI.

#include <memory>
#include <utility>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonObject>
#include <QLibrary>
#include <QPluginLoader>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>

#include "kernel/photons/PhotonsAbstract.h"

namespace PhotonExportFileService
{
inline bool fail(QString* error, const QString& message)
{
    if (error) *error = message;
    return false;
}

// A fresh export may never silently remove an existing photon file, unrelated
// document, or directory entry. A unique temporary probe avoids clobbering
// any fixed-name test file and leaves no artifact when the function returns.
inline bool validateFreshOutputDirectory(const QString& folder, QString* error)
{
    const QFileInfo info(folder);
    if (folder.isEmpty() || !info.isAbsolute() || !info.isDir())
        return fail(error, "File export requires an existing absolute output directory.");
    if (info.isSymLink())
        return fail(error, "File export directory must not be a symbolic link.");
    const QDir directory(info.absoluteFilePath());
    if (!directory.entryList(QDir::AllEntries | QDir::Hidden |
                             QDir::System | QDir::NoDotAndDotDot).isEmpty())
        return fail(error, "File export directory must be empty (never overwrite existing photon data).");

    QTemporaryFile probe(directory.filePath(".tonatiuhpp-file-export-XXXXXX.tmp"));
    if (!probe.open())
        return fail(error, "File export directory is not writable.");
    const bool written = probe.write("1", 1) == 1 && probe.flush();
    probe.close();
    if (!written || probe.error() != QFile::NoError || !probe.remove())
        return fail(error, "File export directory probe failed.");
    if (error) error->clear();
    return true;
}

// Exporters must die BEFORE their Qt plugin loader is released. This exact
// lifetime order is a resource-ownership contract, not a matter of field order.
struct Plugin final
{
    std::unique_ptr<QPluginLoader> loader;
    std::unique_ptr<PhotonsAbstract> exporter;
    ~Plugin() { exporter.reset(); loader.reset(); }
};

inline bool createFileExporter(const QStringList& pluginRoots,
                               std::unique_ptr<QPluginLoader>* loader,
                               std::unique_ptr<PhotonsAbstract>* exporter,
                               QString* error)
{
    if (!loader || !exporter || *loader || *exporter)
        return fail(error, "File photon exporter destination is not empty.");

    for (const QString& root : pluginRoots) {
        QDirIterator files(root, QDir::Files | QDir::NoSymLinks,
                           QDirIterator::Subdirectories);
        while (files.hasNext()) {
            const QString path = files.next();
            if (!QLibrary::isLibrary(path)) continue;
            auto candidateLoader = std::make_unique<QPluginLoader>(path);
            if (candidateLoader->metaData().value("IID").toString() !=
                QStringLiteral("tonatiuh.PhotonsFactory")) continue;
            auto* factory = dynamic_cast<PhotonsFactory*>(candidateLoader->instance());
            if (!factory || factory->name() != QStringLiteral("File")) continue;
            std::unique_ptr<PhotonsAbstract> candidate(factory->create(0));
            if (!candidate) continue;
            *loader = std::move(candidateLoader);
            *exporter = std::move(candidate);
            if (error) error->clear();
            return true;
        }
    }
    return fail(error, "The File photon exporter plugin is unavailable.");
}
} // namespace PhotonExportFileService
