#pragma once

#include "HeadlessScriptFileExportResources.h"

// M3 Phase 3B2B2A: bounded, host-owned, repeated scripted File runs.
// All photons still use the production PhotonsFile exporter/PhotonsBuffer
// and the existing SimulationSession. No new serializer or RNG path.
// The append instance tree MUST outlive the File exporter's surface pointers.

#include <cstdint>
#include <memory>
#include <utility>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QLibrary>
#include <QPluginLoader>
#include <QString>

#include "core/SceneInstanceBuilder.h"
#include "core/SimulationSession.h"
#include "core/TonatiuhCore.h"
#include "kernel/photons/PhotonsAbstract.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/photons/PhotonsSettings.h"
#include "kernel/scene/TSceneKit.h"
#include "script/ScriptRunRequest.h"

class HeadlessScriptRunFileBackend final
{
    using Resources = HeadlessScriptFileExportResources;

    std::unique_ptr<Resources> m_current;

    static bool fail(QString* error, const QString& message)
    {
        if (error) *error = message;
        return false;
    }

    static bool sameOptions(const ScriptPhotonExportState& a,
                            const ScriptPhotonExportState& b)
    {
        return a.settingsPresent == b.settingsPresent &&
            a.exporterName == b.exporterName &&
            a.saveCoordinates == b.saveCoordinates &&
            a.saveCoordinatesGlobal == b.saveCoordinatesGlobal &&
            a.saveSurfaceID == b.saveSurfaceID &&
            a.saveSurfaceSide == b.saveSurfaceSide &&
            a.savePhotonsID == b.savePhotonsID &&
            a.surfaces == b.surfaces &&
            a.parameters == b.parameters &&
            a.bufferSize == b.bufferSize &&
            a.bufferAppend == b.bufferAppend;
    }

    static bool freshOutputDirectory(const QString& folder, QString* error)
    {
        const QFileInfo info(folder);
        if (!info.isAbsolute() || !info.isDir())
            return fail(error, "tn.Run: File export requires an existing absolute output directory.");
        const QDir directory(info.absoluteFilePath());
        if (!directory.entryList(QDir::AllEntries | QDir::Hidden |
                                 QDir::System | QDir::NoDotAndDotDot).isEmpty())
            return fail(error, "tn.Run: File export directory must be empty (never overwrite existing photon data).");
        const QString probeName = directory.filePath(".tonatiuhpp_script_run_probe.tmp");
        QFile probe(probeName);
        if (!probe.open(QIODevice::WriteOnly | QIODevice::NewOnly))
            return fail(error, "tn.Run: File export directory is not writable.");
        const bool written = probe.write("1", 1) == 1 && probe.flush();
        probe.close();
        const bool removed = probe.remove();
        if (!written || !removed)
            return fail(error, "tn.Run: File export directory probe failed.");
        return true;
    }

    static bool setupExporter(Resources* owned, const ScriptRunRequest& request,
                              QString* error)
    {
        const auto& state = request.exportOptions;
        const QString name = state.exporterName.value_or(QStringLiteral("No export"));
        owned->settings.name = name;
        owned->settings.saveCoordinates = state.saveCoordinates.value_or(true);
        owned->settings.saveCoordinatesGlobal = state.saveCoordinatesGlobal.value_or(true);
        owned->settings.saveSurfaceID = state.saveSurfaceID.value_or(true);
        owned->settings.saveSurfaceSide = state.saveSurfaceSide.value_or(true);
        owned->settings.savePhotonsID = state.savePhotonsID.value_or(true);
        owned->settings.surfaces = state.surfaces;
        owned->settings.parameters = state.parameters;

        if (name == QStringLiteral("No export")) {
            if (!state.parameters.isEmpty())
                return fail(error, "tn.Run: parameters for No export are unsupported in headless scripts.");
            owned->exporter = std::make_unique<PhotonsAbstract>();
        } else if (name == QStringLiteral("File")) {
            const QString directory = state.parameters.value(QStringLiteral("ExportDirectory"));
            const QString basename = state.parameters.value(QStringLiteral("ExportFile"),
                                                             QStringLiteral("PhotonMap"));
            const QString fileSize = state.parameters.value(QStringLiteral("FileSize"),
                                                             QStringLiteral("-1"));
            if (basename.isEmpty() || basename == "." || basename == ".." ||
                basename.contains('/') || basename.contains('\\') || basename.contains(':'))
                return fail(error, "tn.Run: File exporter basename must be a simple filename.");
            if (fileSize != QStringLiteral("-1"))
                return fail(error, "tn.Run: headless File export currently supports one output file only (FileSize=-1).");
            for (auto it = state.parameters.cbegin(); it != state.parameters.cend(); ++it) {
                if (it.key() != "ExportDirectory" && it.key() != "ExportFile" && it.key() != "FileSize")
                    return fail(error, "tn.Run: unsupported File exporter parameter: " + it.key());
            }
            if (!freshOutputDirectory(directory, error))
                return false;

            const QStringList roots = TonatiuhCore::pluginSearchPaths(
                QCoreApplication::applicationDirPath());
            for (const QString& root : roots) {
                QDirIterator files(root, QDir::Files | QDir::NoSymLinks,
                                   QDirIterator::Subdirectories);
                while (files.hasNext()) {
                    const QString path = files.next();
                    if (!QLibrary::isLibrary(path)) continue;
                    auto plugin = std::make_unique<QPluginLoader>(path);
                    if (plugin->metaData().value("IID").toString() !=
                        QStringLiteral("tonatiuh.PhotonsFactory")) continue;
                    auto* factory = dynamic_cast<PhotonsFactory*>(plugin->instance());
                    if (!factory || factory->name() != QStringLiteral("File")) continue;
                    owned->exporter.reset(factory->create(0));
                    if (owned->exporter) {
                        owned->plugin = std::move(plugin);
                        break;
                    }
                }
                if (owned->exporter) break;
            }
            if (!owned->exporter)
                return fail(error, "tn.Run: production File photon exporter plugin is unavailable.");
        } else {
            return fail(error, "tn.Run: this headless slice supports only File and No export photon modes.");
        }
        owned->exporter->setPhotonSettings(&owned->settings);
        return true;
    }

    static bool fileDigest(const QString& path, QByteArray* output, QString* error)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return fail(error, "tn.Run: cannot inspect existing export: " + path);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!hash.addData(&file))
            return fail(error, "tn.Run: cannot hash existing export: " + path);
        if (output) *output = hash.result();
        return true;
    }

    static QString outputPath(const Resources& r)
    {
        const QString dir = r.settings.parameters.value("ExportDirectory");
        const QString name = r.settings.parameters.value("ExportFile", "PhotonMap");
        return QDir(dir).filePath(name);
    }

    static bool captureOutput(Resources* r, QString* error)
    {
        if (r->settings.name != "File") return true;
        const QString path = outputPath(*r);
        if (!fileDigest(path + "_parameters.txt", &r->parametersDigest, error))
            return false;
        if (!QFileInfo::exists(path + ".dat") ||
            QFileInfo(path + ".dat").size() == 0) {
            // An empty selection can be a valid first run. Keep the result
            // but refuse append until the exporter has real photon records.
            r->appendable = false;
            r->outputDigest.clear();
            return true;
        }
        r->appendable = true;
        return fileDigest(path + ".dat", &r->outputDigest, error);
    }

    static bool verifyOutputUnchanged(Resources* r, QString* error)
    {
        if (r->settings.name != "File") return true;
        QByteArray data, parameters;
        const QString path = outputPath(*r);
        if (!fileDigest(path + ".dat", &data, error) ||
            !fileDigest(path + "_parameters.txt", &parameters, error))
            return false;
        if (data != r->outputDigest || parameters != r->parametersDigest)
            return fail(error, "tn.Run: existing File export changed externally; refusing append.");
        return true;
    }

public:
    // One instance per ScriptHost. This is intentionally synchronous: the
    // SimulationSession joins before it can release any borrowed scene data.
    bool run(const ScriptRunRequest& request, QString* error)
    {
        if (error) error->clear();
        if (!request.scene)
            return fail(error, "tn.Run: active scene is unavailable.");
        if (!request.explicitRays || request.rays < 1 || request.rays > 200000u)
            return fail(error, "tn.Run: headless scripted Run requires tn.SetRaysNumber(1..200000) first.");

        if (m_current && m_current->poisoned)
            return fail(error, "tn.Run: previous export failed; exporter and any unwritten photons remain retained. Further runs are blocked.");

        const bool append = request.exportOptions.bufferAppend.value_or(false);
        const bool reuse = append && m_current &&
            m_current->options.bufferAppend.value_or(false);
        std::unique_ptr<Resources> candidate;
        Resources* active = nullptr;
        if (reuse) {
            active = m_current.get();
            if (request.scene != active->scene)
                return fail(error, "tn.Run: cannot append after replacing the scene; use a fresh output directory.");
            if (!sameOptions(request.exportOptions, active->options))
                return fail(error, "tn.Run: cannot append after changing exporter/buffer settings; use a fresh output directory.");
            if (active->completedRays + request.rays > 200000u)
                return fail(error, "tn.Run: bounded headless append permits at most 200000 cumulative rays.");
            if (!active->appendable)
                return fail(error, "tn.Run: append requires a nonempty previous photon File output.");
            if (!verifyOutputUnchanged(active, error)) return false;
            if (active->buffer->hasExportFailed() || active->exporter->hasExportError()) {
                active->poisoned = true;
                return fail(error, "tn.Run: existing photon exporter is failed; retained photons cannot be reset.");
            }
        } else {
            // Only a fresh output directory is accepted for a reset. The
            // previously accepted resources are not released until this new
            // run has been completed and its File output finalized.
            candidate = std::make_unique<Resources>(request.scene);
            candidate->options = request.exportOptions;
            candidate->tree = SceneInstanceBuilder::build(request.scene);
            if (!candidate->tree.sceneRoot || !candidate->tree.layoutRoot)
                return fail(error, "tn.Run: could not build a headless scene instance tree.");
            const uint size = request.exportOptions.bufferSize.value_or(1000000u);
            candidate->buffer = std::make_unique<PhotonsBuffer>(size);
            active = candidate.get();
        }

        SimulationSession session;
        HeadlessTracePreparationInput input;
        input.scene = request.scene;
        input.photonBuffer = active->buffer.get();
        input.updateTrackers = true;
        input.borrowedInstanceTree = &active->tree;
        input.configuration.rays = request.rays;
        input.configuration.masterSeed = 0; // Bounded native GUI/headless acceptance.
        input.configuration.sunWidthDivisions = request.sunWidthDivisions;
        input.configuration.sunHeightDivisions = request.sunHeightDivisions;
        input.exportSurfaceUrls = request.exportOptions.surfaces;
        if (!session.prepareHeadless(input, error)) return false;

        if (!reuse) {
            if (!setupExporter(active, request, error)) return false;
            if (!active->buffer->setExporter(active->exporter.get()))
                return fail(error, "tn.Run: photon exporter could not start; no ray trace began.");
        } else if (!active->exporter->startExport()) {
            active->poisoned = true;
            return fail(error, "tn.Run: photon exporter could not restart safely for append.");
        }

        // Own the newly started exporter even if a worker fails. Never
        // abandon a buffer that might retain unwritten photon records.
        if (candidate) m_current = std::move(candidate);
        active = m_current.get();
        QString traceError;
        const bool started = session.start(&traceError);
        const bool completed = started && session.waitForFinished(&traceError);
        const std::uint64_t acceptedTotal = active->completedRays +
            (completed ? static_cast<std::uint64_t>(session.result().raysTraced) : 0u);
        double power = 0.;
        if (completed && acceptedTotal > 0) {
            const auto* context = session.preparedContext();
            if (context)
                power = context->sunApertureArea() * context->irradiance() /
                        static_cast<double>(acceptedTotal);
        }
        // The live scene instance tree is pinned by Resources through here.
        const bool exportSucceeded = active->buffer->endExport(completed ? power : 0.);
        if (!completed || !exportSucceeded) {
            active->poisoned = true;
            if (!completed)
                return fail(error, "tn.Run: trace was not completed: " + traceError);
            return fail(error, "tn.Run: photon exporter failed; partial output must not be accepted.");
        }
        active->completedRays = acceptedTotal;
        // Only File exporters are accepted for subsequent append when they
        // have produced a valid complete snapshot of both output files.
        if (append && !captureOutput(active, error)) {
            active->poisoned = true;
            return false;
        }
        return true;
    }
};
