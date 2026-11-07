#include "GuiScriptSceneAdapter.h"

#include <QDebug>
#include <QPointer>

#include "headless/HeadlessScriptHost.h"
#include "main/MainWindow.h"
#include "main/PluginManager.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneDocument.h"
#include "script/ScriptSceneServices.h"
#include "script/GuiScriptFacadeBridge.h"

#include "kernel/shape/ShapeRT.h"
#include "kernel/profiles/ProfileRT.h"
#include "kernel/material/MaterialRT.h"
#include "kernel/trackers/TrackerArmature.h"
#include "kernel/sun/SunShape.h"
#include "kernel/air/AirTransmission.h"

void installGuiScriptSceneAdapter(ScriptHost* host, MainWindow* window)
{
    if (!host || !window)
        return;

    auto* services = new ScriptSceneServices(host);
    const QPointer<MainWindow> gui(window);
    services->setSceneProvider([gui]() -> TSceneKit* {
        return gui ? gui->getSceneKit() : nullptr;
    });
    services->setNodeCreator([gui](ScriptSceneServices::FactoryKind kind,
                                   const QString& name) -> SoNode* {
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
    });
    // Capture the GUI's current effective input policy for EACH script call;
    // intercepted power is computed by the shared, widget-free service.
    services->setInterceptionConfiguration([gui](ulong rays) {
        return gui ? gui->scriptInterceptionConfiguration(rays) : SimulationConfig{};
    });
    services->setInterceptionDisplay([gui](const QString& surface, uint rays,
                                           std::uint64_t seed, QString* error) {
        if (!gui) {
            if (error) *error = "tn.FindInterception: GUI has been closed.";
            return false;
        }
        return gui->displayScriptInterceptionRays(surface, rays, seed, error);
    });
    services->setFieldWriter([gui](SoNode* node, const QString& field,
                                    const QString& value, QString* error) {
        if (!gui) {
            if (error) *error = "tn.SetValue: GUI scene has been closed.";
            return false;
        }
        return gui->setScriptFieldValue(node, field, value, error);
    });
    // Phase 3B2B1: shared JS Run dispatch, historical GUI execution for
    // exporter/progress/retained-buffer parity until backend extraction.
    services->setRunExecutor([gui](const ScriptRunRequest& request, QString* error) {
        if (!gui) {
            if (error) *error = "tn.Run: GUI has been closed.";
            return false;
        }
        return gui->runScriptFromFacade(request, error);
    });
    services->setPhotonExportSettingsPresent(gui->scriptHasPhotonExportSettings());
    services->setPhotonExportWriter([gui](const ScriptPhotonExportChange& change,
                                          QString* error) {
        if (!gui) {
            if (error) *error = "tn.SetExport*: GUI has been closed.";
            return false;
        }
        return gui->applyScriptPhotonExportChange(change, error);
    });
    services->setRunRayCountWriter([gui](uint rays, QString* error) {
        if (!gui) {
            if (error) *error = "tn.SetRaysNumber: GUI has been closed.";
            return false;
        }
        // Keep the existing GUI Run()/export photon-count policy intact.
        gui->SetRaysNumber(rays);
        if (error) error->clear();
        return true;
    });
    services->setSunGridWriter([gui](int width, int height, QString* error) {
        if (!gui) {
            if (error) *error = "tn.SetRaysGrid: GUI has been closed.";
            return false;
        }
        gui->SetRaysGrid(width, height); // Retains existing UI/session settings.
        return true;
    });
    services->setClearDocument([gui](QString* error) {
        if (!gui) {
            if (error) *error = "tn.Clear: GUI scene document has been closed.";
            return false;
        }
        gui->Clear(); // Preserves GUI document, scene tree and view refresh.
        const bool success = gui->getSceneKit() != nullptr;
        if (!success && error)
            *error = "tn.Clear: GUI could not create a new scene.";
        return success;
    });
    services->setInsertNode([gui](SoNode* node, QString* error) {
        if (!gui) {
            if (error) *error = "tn.InsertScene: GUI scene document has been closed.";
            return false;
        }
        if (gui->insertScriptNode(node))
            return true;
        if (error)
            *error = "tn.InsertScene: no valid group is selected for insertion.";
        return false;
    });
    services->setSaveDocument([gui](const QString& fileName, QString* error) {
        if (!gui) {
            if (error) *error = "tn.saveScene: GUI document is closed.";
            return false;
        }
        return ScriptSceneDocument::writeScene(gui->getSceneKit(), fileName, error);
    });
    services->setLoadDocument([gui](const QString& fileName, QString* error) {
        if (!gui) {
            if (error) *error = "tn.loadScene: GUI document is closed.";
            return false;
        }
        return gui->loadSceneForScript(fileName, error);
    });
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
