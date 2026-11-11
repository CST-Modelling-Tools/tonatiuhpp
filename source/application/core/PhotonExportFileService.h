#pragma once

// M4.1: nonvisual File-export discovery and fail-closed output preflight.
// The same implementation is used by direct CLI tracing and GUI/headless
// scripts. It does not instantiate widgets or change any plugin ABI.

#include <limits>
#include <memory>
#include <QCryptographicHash>
#include <QFile>
#include <QMap>
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
#include "core/TonatiuhCore.h"

namespace PhotonExportFileService
{
// Photon File export has no arbitrary global ray-count cap. The source
// ray-count type, physical storage and actual write failures remain enforced.
// FileSize below is a per-file photon segment size, not a total export limit.

// All three adapters accept the same conservative cross-platform File output
// parameters. A negative sentinel other than -1 is deliberately rejected.
inline bool validateFileOptions(const QString& basename, const QString& fileSize,
                                QString* error);

inline bool fail(QString* error, const QString& message)
{
    if (error) *error = message;
    return false;
}

inline bool validateFileOptions(const QString& basename, const QString& fileSize,
                                QString* error)
{
    if (basename.isEmpty() || basename == "." || basename == ".." ||
        basename.endsWith(QLatin1Char('.')) || basename.endsWith(QLatin1Char(' ')))
        return fail(error, "File exporter basename must be a portable simple filename.");
    const QString forbidden = QStringLiteral("/\\:*?\"<>|");
    for (QChar c : basename) {
        if (c.unicode() < 32 || forbidden.contains(c))
            return fail(error, "File exporter basename contains an invalid filename character.");
    }
    const QString stem = basename.section('.', 0, 0).toUpper();
    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
        ((stem.size() == 4) && (stem.startsWith("COM") || stem.startsWith("LPT")) &&
         (stem[3].unicode() >= '1' && stem[3].unicode() <= '9')))
        return fail(error, "File exporter basename is reserved on Windows.");
    if (fileSize != QStringLiteral("-1")) {
        bool ok = false;
        const qulonglong n = fileSize.toULongLong(&ok);
        if (!ok || n == 0 || n > std::numeric_limits<ulong>::max())
            return fail(error, "FileSize must be -1 or an integer number of photons >= 1.");
    }
    if (error) error->clear();
    return true;
}

// The GUI and script adapters use one bounded-memory SHA-256 manifest for
// completed File exports. A changed/new/removed directory entry blocks append.
// The physical files are never modified by checkpoint inspection.
inline bool isGeneratedSegmentFile(const QString& name, const QString& basename)
{
    const QString prefix = basename + QLatin1Char('_');
    if (!name.startsWith(prefix) || !name.endsWith(QStringLiteral(".dat")))
        return false;
    const QString serial = name.mid(prefix.size(), name.size() - prefix.size() - 4);
    if (serial.isEmpty() || serial.front() == QLatin1Char('0'))
        return false;
    for (QChar c : serial) {
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            return false;
    }
    return true;
}

inline bool validateOutputDirectory(const QString& folder, QString* error)
{
    const QFileInfo info(folder);
    if (folder.isEmpty() || !info.isAbsolute() || !info.isDir())
        return fail(error, "File export requires an existing absolute output directory.");
    if (info.isSymLink())
        return fail(error, "File export directory must not be a symbolic link.");
    return true;
}

inline bool captureOutputCheckpoint(const QString& folder, const QString& basename,
                                   QMap<QString, QByteArray>* digests,
                                   bool* hasPhotonData, QString* error)
{
    if (!digests)
        return fail(error, "File export checkpoint destination is missing.");
    if (!validateOutputDirectory(folder, error)) return false;

    const QDir directory(folder);
    const QString parametersName = basename + QStringLiteral("_parameters.txt");
    bool metadataPresent = false;
    bool dataPresent = false;
    QMap<QString, QByteArray> observed;
    const QFileInfoList entries = directory.entryInfoList(
        QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        if (!entry.isFile() || entry.isSymLink())
            return fail(error, "File export directory contains an unsafe entry.");
        QFile file(entry.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly))
            return fail(error, "Cannot open File export checkpoint entry: " + entry.fileName());
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!hash.addData(&file) || file.error() != QFile::NoError)
            return fail(error, "Cannot hash File export checkpoint entry: " + entry.fileName());
        observed.insert(entry.fileName(), hash.result());
        if (entry.fileName() == parametersName) metadataPresent = true;
        if ((entry.fileName() == basename + QStringLiteral(".dat") ||
             isGeneratedSegmentFile(entry.fileName(), basename)) && entry.size() > 0)
            dataPresent = true;
    }
    if (!metadataPresent)
        return fail(error, "File export checkpoint metadata is missing.");
    *digests = std::move(observed);
    if (hasPhotonData) *hasPhotonData = dataPresent;
    if (error) error->clear();
    return true;
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
            if (!TonatiuhCore::allowsPluginMetadata(candidateLoader->metaData()))
                continue;
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
