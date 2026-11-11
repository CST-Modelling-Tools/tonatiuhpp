#pragma once

#include "headless/HeadlessScriptFileExportResources.h"
#include "core/PhotonExportFileService.h"

// M3 unified script-run executor for both GUI and true-headless ScriptHost.
// Single widget-free lifecycle for the production File/No-export plugins,
// SimulationSession, append, failure containment and photon ownership.
// Per-host state retains the owned instance tree behind serialized surface pointers.

#include <cstdint>
#include <limits>
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

class ScriptRunExecutionService final
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
        QString detail;
        if (!PhotonExportFileService::validateFreshOutputDirectory(folder, &detail))
            return fail(error, "tn.Run: " + detail);
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
                return fail(error, "tn.Run: parameters for No export are unsupported in shared script runs.");
            owned->exporter = std::make_unique<PhotonsAbstract>();
        } else if (name == QStringLiteral("File")) {
            const QString directory = state.parameters.value(QStringLiteral("ExportDirectory"));
            const QString basename = state.parameters.value(QStringLiteral("ExportFile"),
                                                             QStringLiteral("PhotonMap"));
            const QString fileSize = state.parameters.value(QStringLiteral("FileSize"),
                                                             QStringLiteral("-1"));
            QString fileError;
            if (!PhotonExportFileService::validateFileOptions(basename, fileSize, &fileError))
                return fail(error, "tn.Run: " + fileError);
            for (auto it = state.parameters.cbegin(); it != state.parameters.cend(); ++it) {
                if (it.key() != "ExportDirectory" && it.key() != "ExportFile" && it.key() != "FileSize")
                    return fail(error, "tn.Run: unsupported File exporter parameter: " + it.key());
            }
            if (!freshOutputDirectory(directory, error))
                return false;

            const QStringList roots = TonatiuhCore::pluginSearchPaths(
                QCoreApplication::applicationDirPath());
            QString discoveryError;
            if (!PhotonExportFileService::createFileExporter(
                    roots, &owned->plugin, &owned->exporter, &discoveryError))
                return fail(error, "tn.Run: production File photon exporter plugin is unavailable.");
        } else {
            return fail(error, "tn.Run: scripted execution supports only File and No export photon modes.");
        }
        owned->exporter->setPhotonSettings(&owned->settings);
        return true;
    }

    // Capture a full directory snapshot rather than just PhotonMap.dat.
    // FileSize>0 creates PhotonMap_N.dat segments; unknown files or links must
    // also block append so a different process cannot add an overwrite target.
    static bool outputFiles(Resources* r, QMap<QString, QByteArray>* digests,
                            bool* hasData, QString* error)
    {
        QString checkpointError;
        if (!PhotonExportFileService::captureOutputCheckpoint(
                r->settings.parameters.value("ExportDirectory"),
                r->settings.parameters.value("ExportFile", "PhotonMap"),
                digests, hasData, &checkpointError))
            return fail(error, "tn.Run: " + checkpointError);
        return true;
    }

    static bool captureOutput(Resources* r, QString* error)
    {
        if (r->settings.name != "File") return true;
        bool data = false;
        if (!outputFiles(r, &r->outputDigests, &data, error)) return false;
        r->appendable = data;
        return true;
    }

    static bool verifyOutputUnchanged(Resources* r, QString* error)
    {
        if (r->settings.name != "File") return true;
        QMap<QString, QByteArray> observed;
        if (!outputFiles(r, &observed, nullptr, error)) return false;
        if (observed != r->outputDigests)
            return fail(error, "tn.Run: existing File export changed externally; refusing append.");
        return true;
    }

public:
    // A script host may supply a UI-specific default seed. Explicit
    // tn.Run(seed) always wins; the legacy headless default remains zero.
    bool run(const ScriptRunRequest& request, QString* error)
    {
        return run(request, 0u, error);
    }

    // One instance per ScriptHost. The session always joins workers before
    // any retained scene, tree or photon buffer can be replaced.
    bool run(const ScriptRunRequest& request, std::uint64_t defaultSeed, QString* error)
    {
        if (error) error->clear();
        if (!request.scene)
            return fail(error, "tn.Run: active scene is unavailable.");
        if (!request.explicitRays || request.rays < 1)
            return fail(error, "tn.Run: effective ray count must be explicitly set to a positive integer.");

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
            // Guard the actual accounting type, never an arbitrary export budget.
            if (active->completedRays > std::numeric_limits<std::uint64_t>::max() -
                    static_cast<std::uint64_t>(request.rays))
                return fail(error, "tn.Run: cumulative ray count exceeds 64-bit accounting capacity.");
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
        input.configuration.masterSeed = request.masterSeed.value_or(defaultSeed);
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
