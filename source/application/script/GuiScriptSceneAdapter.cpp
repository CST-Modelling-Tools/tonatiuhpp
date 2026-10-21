#include "GuiScriptSceneAdapter.h"

#include <QDebug>
#include <QPointer>

#include "main/MainWindow.h"
#include "main/PluginManager.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneServices.h"

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
    services->setInterception([gui](const QString& surface,
                                     uint rays) -> std::optional<double> {
        if (!gui)
            return std::nullopt;
        return findInterception(surface, rays, gui);
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
    registerScriptSceneServices(host->engine(), services);

    // Only three operations move to the shared domain facade in Step 3.
    // All other GUI script methods still forward to the legacy MainWindow
    // object, preserving their original call signatures and selection policy.
    const QJSValue bridge = host->evaluate(QStringLiteral(R"JS(
(function(global) {
  var legacy = global.tn;
  var scene = global.__tonatiuhppSceneApi;
  if (typeof Proxy !== 'function')
    throw new Error('M3.2 GUI scene API requires JavaScript Proxy support.');
  var facade = new Proxy(legacy, {
    get: function(target, property) {
      // Return bound QObject methods directly. Rewrapping them in JS
      // attributes native exceptions to <gui-scene-api>, not the script.
      if (property === 'getScene')
        return scene.getScene;
      if (property === 'Clear')
        return scene.Clear;
      if (property === 'InsertScene')
        return scene.InsertScene;
      var value = target[property];
      return typeof value === 'function' ? value.bind(target) : value;
    },
    set: function(target, property, value) {
      target[property] = value;
      return true;
    }
  });
  global.tn = facade;
  global.tonatiuh = facade;
})(this);
)JS"), QStringLiteral("<gui-scene-api>"));
    if (bridge.isError())
        qWarning() << "Cannot install shared GUI script scene API:" << bridge.toString();
}
