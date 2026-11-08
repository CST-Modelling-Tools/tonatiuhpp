#pragma once

#include <QString>

// Script-only GUI compatibility adapter. The proxy target is an ordinary JS
// object, never the MainWindow QObject. Explicitly named legacy commands are
// retained until their scientific backends can move to shared services.
inline QString guiScriptFacadeBootstrap()
{
    return QStringLiteral(R"JS(
(function(global) {
  var legacyWindow = global.tn;
  var scene = global.__tonatiuhppSceneApi;
  var trace = global.__tonatiuhppTraceApi;
  if (typeof Proxy !== 'function')
    throw new Error('Tonatiuh++ GUI script compatibility requires JavaScript Proxy support.');
  if (!legacyWindow || !scene || !trace)
    throw new Error('Tonatiuh++ GUI script API adapters are unavailable.');

  // Bound native methods preserve source attribution of C++ exceptions.
  // Changing these names requires equivalent changes in HeadlessScriptHost.
  var shared = Object.create(null);
  shared.getScene = scene.getScene;
  shared.Clear = scene.Clear;
  shared.InsertScene = scene.InsertScene;
  shared.saveScene = scene.saveScene;
  shared.loadScene = scene.loadScene;
  shared.FindInterception = scene.FindInterception;
  shared.ChangeSunPosition = scene.ChangeSunPosition;
  shared.SetValue = scene.SetValue;
  shared.SetRaysGrid = scene.SetRaysGrid;
  shared.SetRaysNumber = scene.SetRaysNumber;
  shared.Run = scene.Run; // Phase 3B2B1: never call legacyWindow.Run from JS.
  shared.RunFluxAnalysis = scene.RunFluxAnalysis; // Phase 3B3A: no raw window domain call.
  // Phase 3B2A: all export-setting domain calls route through the same
  // ScriptSceneServices object in GUI and true-headless script hosts.
  shared.SetExportAllPhotonMap = scene.SetExportAllPhotonMap;
  shared.SetExportCoordinates = scene.SetExportCoordinates;
  shared.SetExportIntersectionSurface = scene.SetExportIntersectionSurface;
  shared.SetExportIntersectionSurfaceSide = scene.SetExportIntersectionSurfaceSide;
  shared.SetExportPhotonMapType = scene.SetExportPhotonMapType;
  shared.SetExportPreviousNextPhotonID = scene.SetExportPreviousNextPhotonID;
  shared.SetExportTypeParameterValue = scene.SetExportTypeParameterValue;
  shared.AddExportSurfaceURL = scene.AddExportSurfaceURL;
  shared.SetPhotonBufferSize = scene.SetPhotonBufferSize;
  shared.SetPhotonBufferAppend = scene.SetPhotonBufferAppend;
  shared.traceScene = trace.traceScene;
  shared.startTrace = trace.startTrace;
  shared.cancelTrace = trace.cancelTrace;
  shared.finishTrace = trace.finishTrace;

  // GUI document editing or visual presentation; intentionally unsupported
  // by a truly headless process. These commands do not form the domain API.
  var presentation = [
    'Screenshot', 'render', 'Select', 'SetName', 'Copy', 'Cut',
    'Delete', 'Paste', 'PasteCopy', 'PasteLink', 'fileNew',
    'fileOpen', 'fileSave', 'fileSaveAs', 'InsertNode', 'InsertShape',
    'InsertTracker', 'InsertArray', 'InsertShapeSurface',
    'CreateComponentNode', 'nodeImport', 'nodeExport',
    'SetRaysScreen', 'print', 'printTimed'
  ];

  // Legacy forwarding is now restricted to presentation/document editing.
  // Scientific RunFluxAnalysis is dispatched via ScriptSceneServices above.
  var legacyMethods = Object.create(null);
  presentation.forEach(function(name) {
    legacyMethods[name] = true;
  });
  var userProperties = Object.create(null);
  var owns = Object.prototype.hasOwnProperty;

  var facade = new Proxy(Object.create(null), {
    get: function(target, property) {
      if (typeof property !== 'string' || property === 'then')
        return undefined;
      if (owns.call(shared, property))
        return shared[property];
      if (owns.call(legacyMethods, property)) {
        // The ONLY path to a legacy MainWindow method is this allowlist.
        var method = legacyWindow[property];
        return typeof method === 'function' ? method.bind(legacyWindow) :
          function() { throw new Error('GUI legacy script command tn.' + property + ' is unavailable.'); };
      }
      if (owns.call(userProperties, property))
        return userProperties[property];
      // Same unsupported-capability model as the true-headless facade.
      return function() {
        throw new Error('GUI script API does not support tn.' + property + '.');
      };
    },
    set: function(target, property, value) {
      if (typeof property !== 'string' || property === 'then' ||
          owns.call(shared, property) || owns.call(legacyMethods, property))
        throw new Error('Cannot replace a Tonatiuh++ script API method.');
      // Preserve JS scratch properties without writing arbitrary QWidget fields.
      userProperties[property] = value;
      return true;
    },
    has: function(target, property) {
      return typeof property === 'string' &&
        (owns.call(shared, property) || owns.call(legacyMethods, property) ||
         owns.call(userProperties, property));
    }
  });
  global.tn = facade;
  global.tonatiuh = facade;
})(this);
)JS");
}
