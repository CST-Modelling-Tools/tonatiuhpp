#include "GuiScriptSceneAdapter.h"

#include <QDebug>
#include <QPointer>

#include <memory>
#include <functional>

#include "headless/HeadlessScriptHost.h"
#include "headless/HeadlessScriptFluxBackend.h"
#include "main/MainWindow.h"
#include "main/PluginManager.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneDocument.h"
#include "script/ScriptSceneServices.h"
#include "script/ScriptRunExecutionService.h"
#include "script/GuiScriptFacadeBridge.h"

#include "kernel/shape/ShapeRT.h"
#include "kernel/profiles/ProfileRT.h"
#include "kernel/material/MaterialRT.h"
#include "kernel/trackers/TrackerArmature.h"
#include "kernel/sun/SunShape.h"
#include "kernel/air/AirTransmission.h"

namespace
{
struct GuiScriptCallbacks
{
    QPointer<MainWindow> gui;
    ScriptHost* host;
    std::shared_ptr<ScriptRunExecutionService> scriptedRun;

    GuiScriptCallbacks(QPointer<MainWindow> window, ScriptHost* scriptHost)
        : gui(window), host(scriptHost),
          scriptedRun(std::make_shared<ScriptRunExecutionService>())
    {
    }

    TSceneKit* sceneProvider() const
    {
        return gui ? gui->getSceneKit() : nullptr;
    }

    SoNode* nodeCreator(ScriptSceneServices::FactoryKind kind,
        const QString& name) const
    {
        PluginManager* plugins = gui ? gui->getPlugins() : nullptr;
        if (!plugins)
            return nullptr;
        switch (kind) {
        case ScriptSceneServices::FactoryKind::Shape: {
            auto* factory = plugins->getShapeMap().value(name, nullptr);
            return factory ? factory->create() : nullptr;
        }
        case ScriptSceneServices::FactoryKind::Profile: {
            auto* factory = plugins->getProfileMap().value(name, nullptr);
            return factory ? factory->create() : nullptr;
        }
        case ScriptSceneServices::FactoryKind::Material: {
            auto* factory = plugins->getMaterialMap().value(name, nullptr);
            return factory ? factory->create() : nullptr;
        }
        case ScriptSceneServices::FactoryKind::Tracker: {
            auto* factory = plugins->getTrackerMap().value(name, nullptr);
            return factory ? factory->create() : nullptr;
        }
        case ScriptSceneServices::FactoryKind::Sun: {
            auto* factory = plugins->getSunMap().value(name, nullptr);
            return factory ? factory->create() : nullptr;
        }
        case ScriptSceneServices::FactoryKind::Air: {
            auto* factory = plugins->getAirMap().value(name, nullptr);
            return factory ? factory->create() : nullptr;
        }
        }
        return nullptr;
    }

    SimulationConfig interceptionConfiguration(ulong rays) const
    {
        return gui ? gui->scriptInterceptionConfiguration(rays) : SimulationConfig{};
    }

    bool interceptionDisplay(const QString& surface, uint rays,
        std::uint64_t seed, QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.FindInterception: GUI has been closed.";
            return false;
        }
        return gui->displayScriptInterceptionRays(surface, rays, seed, error);
    }

    bool fieldWriter(SoNode* node, const QString& field,
        const QString& value, QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.SetValue: GUI scene has been closed.";
            return false;
        }
        return gui->setScriptFieldValue(node, field, value, error);
    }

    bool runExecutor(const ScriptRunRequest& request,
        QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.Run: GUI has been closed.";
            return false;
        }
        if (!request.scene || request.scene != gui->getSceneKit()) {
            if (error) *error = "tn.Run: GUI scene changed before execution.";
            return false;
        }
        ScriptRunRequest effective = request;
        // Preserve GUI Run defaults and any SetRays*/SetExport* calls that
        // already updated the GUI state. The headless adapter keeps its own
        // explicit-rays requirement and seed-zero default.
        effective.rays = gui->scriptRunEffectiveRays();
        effective.explicitRays = effective.rays;
        effective.sunWidthDivisions = gui->scriptRunEffectiveGridWidth();
        effective.sunHeightDivisions = gui->scriptRunEffectiveGridHeight();
        effective.exportOptions = gui->scriptPhotonExportSnapshot();
        return scriptedRun->run(effective, gui->scriptRunMasterSeed(), error);
    }

    bool fluxAnalysisExecutor(const ScriptFluxAnalysisRequest& request,
        QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.RunFluxAnalysis: GUI has been closed.";
            return false;
        }
        // Both seven- and eight-argument scientific JS methods now use the
        // identical nonvisual session, grid calculation and TSV writer.
        ScriptFluxAnalysisRequest effective = request;
        effective.fileName = host->resolvePath(request.fileName);
        if (!effective.masterSeed.has_value())
            effective.masterSeed = gui->scriptFluxLegacySeed();
        // Retain the original automatic-millisecond GUI seed for seven args;
        // explicit eight-argument calls continue using their supplied seed.
        return HeadlessScriptFluxBackend::run(gui->getSceneKit(), effective, error);
    }

    bool photonExportWriter(const ScriptPhotonExportChange& change,
        QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.SetExport*: GUI has been closed.";
            return false;
        }
        return gui->applyScriptPhotonExportChange(change, error);
    }

    bool runRayCountWriter(uint rays, QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.SetRaysNumber: GUI has been closed.";
            return false;
        }
        // Keep the existing GUI Run()/export photon-count policy intact.
        gui->SetRaysNumber(rays);
        if (error) error->clear();
        return true;
    }

    bool sunGridWriter(int width, int height, QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.SetRaysGrid: GUI has been closed.";
            return false;
        }
        gui->SetRaysGrid(width, height); // Retains existing UI/session settings.
        return true;
    }

    bool clearDocument(QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.Clear: GUI scene document has been closed.";
            return false;
        }
        gui->Clear(); // Preserves GUI document, scene tree and view refresh.
        const bool success = gui->getSceneKit() != nullptr;
        if (!success && error)
            *error = "tn.Clear: GUI could not create a new scene.";
        return success;
    }

    bool insertNode(SoNode* node, QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.InsertScene: GUI scene document has been closed.";
            return false;
        }
        if (gui->insertScriptNode(node))
            return true;
        if (error)
            *error = "tn.InsertScene: no valid group is selected for insertion.";
        return false;
    }

    bool saveDocument(const QString& fileName, QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.saveScene: GUI document is closed.";
            return false;
        }
        return ScriptSceneDocument::writeScene(gui->getSceneKit(), fileName, error);
    }

    bool loadDocument(const QString& fileName, QString* error) const
    {
        if (!gui) {
            if (error) *error = "tn.loadScene: GUI document is closed.";
            return false;
        }
        return gui->loadSceneForScript(fileName, error);
    }

};
} // namespace

void installGuiScriptSceneAdapter(ScriptHost* host, MainWindow* window)
{
    if (!host || !window)
        return;

    auto* services = new ScriptSceneServices(host);
    const QPointer<MainWindow> gui(window);
    const auto callbacks = std::make_shared<GuiScriptCallbacks>(gui, host);
    services->setSceneProvider(std::bind_front(&GuiScriptCallbacks::sceneProvider, callbacks));
    services->setNodeCreator(std::bind_front(&GuiScriptCallbacks::nodeCreator, callbacks));
    // Capture the GUI's current effective input policy for EACH script call;
    // intercepted power is computed by the shared, widget-free service.
    services->setInterceptionConfiguration(std::bind_front(&GuiScriptCallbacks::interceptionConfiguration, callbacks));
    services->setInterceptionDisplay(std::bind_front(&GuiScriptCallbacks::interceptionDisplay, callbacks));
    services->setFieldWriter(std::bind_front(&GuiScriptCallbacks::fieldWriter, callbacks));
    // One widget-free, per-ScriptHost Run/export service for GUI and CLI.
    // The GUI maps only its current dialog state into the immutable request.
    // Interactive MainWindow::Run() remains a separate presentation adapter.
    services->setRunExecutor(std::bind_front(&GuiScriptCallbacks::runExecutor, callbacks));
    // M3 Phase 3B3A: the domain script call uses the same validated request
    // type in both hosts. Only the GUI adapter may invoke the historical
    // widget-bound FluxAnalysis path; checked trace/file failures now
    // propagate to the script call site. Nonvisual parity remains open.
    services->setFluxAnalysisExecutor(std::bind_front(&GuiScriptCallbacks::fluxAnalysisExecutor, callbacks));
    services->setPhotonExportSettingsPresent(gui->scriptHasPhotonExportSettings());
    services->setPhotonExportWriter(std::bind_front(&GuiScriptCallbacks::photonExportWriter, callbacks));
    services->setRunRayCountWriter(std::bind_front(&GuiScriptCallbacks::runRayCountWriter, callbacks));
    services->setSunGridWriter(std::bind_front(&GuiScriptCallbacks::sunGridWriter, callbacks));
    services->setClearDocument(std::bind_front(&GuiScriptCallbacks::clearDocument, callbacks));
    services->setInsertNode(std::bind_front(&GuiScriptCallbacks::insertNode, callbacks));
    services->setSaveDocument(std::bind_front(&GuiScriptCallbacks::saveDocument, callbacks));
    services->setLoadDocument(std::bind_front(&GuiScriptCallbacks::loadDocument, callbacks));
    registerScriptSceneServices(host->engine(), services);

    // The same file-based, no-export session API as true-headless scripts.
    // Parent it to the host so an unfinished trace is cancelled/joined when
    // the editor's host is destroyed, never retaining a MainWindow pointer.
    auto* traceApi = new HeadlessScriptApi(host->engine(), host);
    QJSEngine::setObjectOwnership(traceApi, QJSEngine::CppOwnership);
    host->engine()->globalObject().setProperty(
        "__tonatiuhppTraceApi", host->engine()->newQObject(traceApi));

    // A plain-JS proxy with an EXPLICIT compatibility allowlist.
    // No arbitrary MainWindow QObject property can escape through `tn`.
    const QJSValue bridge = host->evaluate(guiScriptFacadeBootstrap(),
                                          QStringLiteral("<gui-scene-api>"));
    if (bridge.isError()) {
        qWarning() << "Cannot install shared GUI script scene API:" << bridge.toString();
        // Fail closed: never leave raw MainWindow exposed if the proxy fails.
        host->engine()->globalObject().setProperty("tn", QJSValue());
        host->engine()->globalObject().setProperty("tonatiuh", QJSValue());
    }
}
