#include "HeadlessScriptHost.h"

#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QJSEngine>
#include <QTextStream>

#include "benchmark/BenchmarkRunner.h"
#include "core/CorePluginRegistry.h"
#include "core/RayTraceExecutor.h"
#include "core/SimulationSession.h"
#include "core/SceneLoader.h"
#include "core/TracePreparation.h"
#include "core/TonatiuhCore.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneDocument.h"
#include "script/ScriptSceneServices.h"

namespace
{
QString absoluteFilePath(const QString& fileName)
{
    return QFileInfo(fileName).absoluteFilePath();
}

QString scriptLineAt(const QString& source, int lineNumber)
{
    if (lineNumber <= 0)
        return QString();

    const QStringList lines = source.split('\n');
    if (lineNumber > lines.size())
        return QString();

    return lines.at(lineNumber - 1).trimmed();
}

bool readIntegerOption(const QJSValue& value, const QString& name, bool allowZero, ulong* parsed, QString* errorMessage)
{
    auto fail = [errorMessage](const QString& message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };

    if (!parsed)
        return fail(QString("%1 parser target is not available.").arg(name));
    if (!value.isNumber())
        return fail(QString("%1 must be an integer.").arg(name));

    const double number = value.toNumber();
    if (!std::isfinite(number) || std::floor(number) != number)
        return fail(QString("%1 must be an integer.").arg(name));
    if (number < 0.)
        return fail(QString("%1 must not be negative.").arg(name));
    if (!allowZero && number == 0.)
        return fail(QString("%1 must be greater than zero.").arg(name));

    constexpr double kMaxSafeInteger = 9007199254740991.;
    const double platformMax = static_cast<double>(std::numeric_limits<ulong>::max());
    const double maxValue = platformMax < kMaxSafeInteger ? platformMax : kMaxSafeInteger;
    if (number > maxValue)
        return fail(QString("%1 is outside the supported range.").arg(name));

    *parsed = static_cast<ulong>(number);
    return true;
}

QJSValue makeTraceSummary(QJSEngine* engine, const QString& sceneFilePath, ulong rays, ulong seed, const RayTraceExecutorResult& result)
{
    QJSValue summary = engine->newObject();
    summary.setProperty("scene_file", QJSValue(sceneFilePath));
    summary.setProperty("rays", QJSValue(static_cast<double>(rays)));
    summary.setProperty("seed", QJSValue(static_cast<double>(seed)));
    summary.setProperty("no_export", QJSValue(true));
    summary.setProperty("photon_export", QJSValue(false));
    summary.setProperty("export_path", QJSValue(QStringLiteral("none")));
    summary.setProperty("rays_traced", QJSValue(static_cast<double>(result.raysTraced)));
    summary.setProperty("elapsed_seconds", QJSValue(result.elapsedSeconds));
    summary.setProperty("rays_per_second", QJSValue(result.raysPerSecond));
    summary.setProperty("worker_count", QJSValue(result.workerCount));
    summary.setProperty("chunk_count", QJSValue(static_cast<double>(result.chunkCount)));
    summary.setProperty("chunk_size", QJSValue(static_cast<double>(result.chunkSize)));
    summary.setProperty("sun_aperture_area", QJSValue(result.sunApertureArea));
    summary.setProperty("irradiance", QJSValue(result.irradiance));
    summary.setProperty("power_per_ray", QJSValue(result.powerPerRay));
    return summary;
}
}

HeadlessScriptApi::HeadlessScriptApi(QJSEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

HeadlessScriptApi::~HeadlessScriptApi() = default;

void HeadlessScriptApi::print(const QJSValue& value)
{
    QTextStream out(stdout);
    out << toDisplayString(value) << Qt::endl;
}

bool HeadlessScriptApi::writeJson(const QString& fileName, const QJSValue& value)
{
    if (fileName.trimmed().isEmpty()) {
        recordError("tn.writeJson failed: output path must not be empty.");
        return false;
    }

    QString errorMessage;
    const QString json = toJsonString(value, &errorMessage);
    if (!errorMessage.isEmpty()) {
        recordError(QString("tn.writeJson failed for %1: %2").arg(fileName, errorMessage));
        return false;
    }

    // Match DataObject's per-script path policy. A headless script can be
    // launched from any working directory without redirecting JSON output.
    QString resolvedPath = fileName;
    if (m_engine) {
        const QJSValue context = m_engine->globalObject().property("__tonatiuhppScriptHost");
        if (auto* host = qobject_cast<ScriptHost*>(context.toQObject()))
            resolvedPath = host->resolvePath(fileName);
    }
    QFileInfo info(resolvedPath);
    QDir parentDir = info.absoluteDir();
    if (!parentDir.exists() && !parentDir.mkpath(".")) {
        recordError(QString("tn.writeJson failed: cannot create output directory %1.").arg(parentDir.absolutePath()));
        return false;
    }

    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        recordError(QString("tn.writeJson failed: cannot open %1: %2").arg(info.absoluteFilePath(), file.errorString()));
        return false;
    }

    const QByteArray bytes = json.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        recordError(QString("tn.writeJson failed: cannot write %1: %2").arg(info.absoluteFilePath(), file.errorString()));
        return false;
    }

    return true;
}

bool HeadlessScriptApi::validateScene(const QString& fileName)
{
    if (fileName.trimmed().isEmpty()) {
        recordError("tn.validateScene failed: scene path must not be empty.");
        return false;
    }

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    initializeSceneServices(fileName, &plugins);

    LoadedScene scene;
    QString errorMessage;
    if (!SceneLoader::readFile(fileName, &scene, &errorMessage)) {
        recordError(QString("tn.validateScene failed for %1: %2").arg(absoluteFilePath(fileName), errorMessage));
        return false;
    }

    return true;
}

int HeadlessScriptApi::runBenchmark(const QString& configFileName)
{
    if (configFileName.trimmed().isEmpty()) {
        recordError("tn.runBenchmark failed: benchmark config path must not be empty.");
        return 1;
    }

    BenchmarkRunner benchmarkRunner;
    QString errorMessage;
    const QString sceneFileName = benchmarkRunner.sceneFileName(configFileName, &errorMessage);
    if (sceneFileName.isEmpty()) {
        recordError(QString("tn.runBenchmark failed for %1: %2").arg(absoluteFilePath(configFileName), errorMessage));
        return 1;
    }

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    initializeSceneServices(sceneFileName, &plugins);

    LoadedScene scene;
    if (!SceneLoader::readFile(sceneFileName, &scene, &errorMessage)) {
        recordError(QString("tn.runBenchmark failed while loading %1: %2").arg(absoluteFilePath(sceneFileName), errorMessage));
        return 1;
    }

    const int result = benchmarkRunner.run(configFileName, scene.get(), &errorMessage);
    if (result != 0)
        recordError(QString("tn.runBenchmark failed for %1: %2").arg(absoluteFilePath(configFileName), errorMessage));

    return result;
}

QJSValue HeadlessScriptApi::traceScene(const QJSValue& optionsValue)
{
    if (m_pendingTrace) {
        recordError("tn.traceScene failed: finish the active script trace first.");
        return QJSValue();
    }
    if (!m_engine) {
        recordError("tn.traceScene failed: script engine is not available.");
        return QJSValue();
    }
    if (!optionsValue.isObject()) {
        recordError("tn.traceScene failed: options must be an object.");
        return QJSValue();
    }

    const QJSValue sceneValue = optionsValue.property("scene");
    if (!sceneValue.isString() || sceneValue.toString().trimmed().isEmpty()) {
        recordError("tn.traceScene failed: options.scene must be a non-empty scene path.");
        return QJSValue();
    }

    ulong rays = 0;
    QString errorMessage;
    const QJSValue raysValue = optionsValue.property("rays");
    if (raysValue.isUndefined() || !readIntegerOption(raysValue, "options.rays", false, &rays, &errorMessage)) {
        recordError(QString("tn.traceScene failed: %1").arg(errorMessage.isEmpty() ? QStringLiteral("options.rays is required.") : errorMessage));
        return QJSValue();
    }

    ulong seed = 0;
    const QJSValue seedValue = optionsValue.property("seed");
    if (!seedValue.isUndefined() && !readIntegerOption(seedValue, "options.seed", true, &seed, &errorMessage)) {
        recordError(QString("tn.traceScene failed: %1").arg(errorMessage));
        return QJSValue();
    }

    const QJSValue noExportValue = optionsValue.property("noExport");
    if (!noExportValue.isBool() || !noExportValue.toBool()) {
        recordError("tn.traceScene failed: options.noExport must be true; photon export is not supported in headless scripts.");
        return QJSValue();
    }

    const QString sceneFileName = sceneValue.toString();
    TonatiuhCore::initializeCoin();
    auto plugins = std::make_shared<CorePluginRegistry>();
    initializeSceneServices(sceneFileName, plugins.get());

    auto scene = std::make_shared<LoadedScene>();
    if (!SceneLoader::readFile(sceneFileName, scene.get(), &errorMessage)) {
        recordError(QString("tn.traceScene failed while loading %1: %2").arg(absoluteFilePath(sceneFileName), errorMessage));
        return QJSValue();
    }

    SimulationSession session;
    session.retainResource(plugins);
    session.retainResource(scene);
    HeadlessTracePreparationInput preparationInput;
    preparationInput.scene = scene->get();
    preparationInput.configuration.rays = rays;
    preparationInput.configuration.masterSeed = seed;
    if (!session.prepareHeadless(preparationInput, &errorMessage)
        || !session.start(&errorMessage)
        || !session.waitForFinished(&errorMessage)) {
        recordError(QString("tn.traceScene failed for %1: %2").arg(absoluteFilePath(sceneFileName), errorMessage));
        return QJSValue();
    }

    return makeTraceSummary(m_engine, absoluteFilePath(sceneFileName), rays, seed, session.result());
}

QJSValue HeadlessScriptApi::startTrace(const QJSValue& options)
{
    auto fail = [this](const QString& reason) -> QJSValue {
        if (m_engine)
            m_engine->throwError(QStringLiteral("tn.startTrace failed: ") + reason);
        return QJSValue();
    };
    if (!m_engine)
        return QJSValue();
    if (m_pendingTrace)
        return fail("a script trace is already active; call tn.finishTrace first.");
    if (!options.isObject())
        return fail("options must be an object.");

    const QJSValue sceneValue = options.property("scene");
    if (!sceneValue.isString() || sceneValue.toString().trimmed().isEmpty())
        return fail("options.scene must be a non-empty scene path.");

    ulong rays = 0;
    QString error;
    const QJSValue raysValue = options.property("rays");
    if (raysValue.isUndefined()
        || !readIntegerOption(raysValue, "options.rays", false, &rays, &error))
        return fail(error.isEmpty() ? QStringLiteral("options.rays is required.") : error);

    ulong seed = 0;
    const QJSValue seedValue = options.property("seed");
    if (!seedValue.isUndefined()
        && !readIntegerOption(seedValue, "options.seed", true, &seed, &error))
        return fail(error);
    const QJSValue noExport = options.property("noExport");
    if (!noExport.isBool() || !noExport.toBool())
        return fail("options.noExport must be true; photon export is not supported.");

    // Resolve the same script-local path as DataObject and tn.loadScene.
    // Do not assume the process working directory is the script directory.
    auto* host = qobject_cast<ScriptHost*>(m_engine->globalObject()
        .property("__tonatiuhppScriptHost").toQObject());
    const QString path = host ? host->resolvePath(sceneValue.toString())
                              : absoluteFilePath(sceneValue.toString());
    TonatiuhCore::initializeCoin();
    auto plugins = std::make_shared<CorePluginRegistry>();
    initializeSceneServices(path, plugins.get());
    auto scene = std::make_shared<LoadedScene>();
    if (!SceneLoader::readFile(path, scene.get(), &error))
        return fail(QString("cannot load %1: %2").arg(path, error));

    auto session = std::make_unique<SimulationSession>();
    session->retainResource(plugins);
    session->retainResource(scene);
    HeadlessTracePreparationInput input;
    input.scene = scene->get();
    input.configuration.rays = rays;
    input.configuration.masterSeed = seed;
    if (!session->prepareHeadless(input, &error) || !session->start(&error))
        return fail(QString("cannot start %1: %2").arg(path, error));

    m_pendingTrace = std::move(session);
    m_pendingSceneFile = absoluteFilePath(path);
    m_pendingRays = rays;
    m_pendingSeed = seed;
    QJSValue started = m_engine->newObject();
    started.setProperty("status", QJSValue(QStringLiteral("running")));
    started.setProperty("rays_requested", QJSValue(static_cast<double>(rays)));
    started.setProperty("seed", QJSValue(static_cast<double>(seed)));
    started.setProperty("scene_file", QJSValue(m_pendingSceneFile));
    return started;
}

QJSValue HeadlessScriptApi::cancelTrace()
{
    if (!m_pendingTrace || !m_pendingTrace->isRunning()) {
        if (m_engine)
            m_engine->throwError(QStringLiteral("tn.cancelTrace failed: no active script trace; call tn.startTrace first."));
        return QJSValue();
    }
    // This only requests cancellation. finishTrace() always joins workers.
    m_pendingTrace->cancel();
    return QJSValue(true);
}

QJSValue HeadlessScriptApi::finishTrace()
{
    if (!m_pendingTrace) {
        if (m_engine)
            m_engine->throwError(QStringLiteral("tn.finishTrace failed: no active script trace."));
        return QJSValue();
    }

    // Releasing the owning pointer here also makes the host reusable after
    // a cancelled or failed run, with all worker/resource joins complete.
    std::unique_ptr<SimulationSession> finished = std::move(m_pendingTrace);
    QString error;
    const bool success = finished->waitForFinished(&error);
    if (!success && finished->state() != SimulationSession::State::Cancelled) {
        if (m_engine)
            m_engine->throwError(QStringLiteral("tn.finishTrace failed: ") + error);
        return QJSValue();
    }

    QJSValue result;
    if (success && finished->state() == SimulationSession::State::Completed) {
        result = makeTraceSummary(m_engine, m_pendingSceneFile,
                                  m_pendingRays, m_pendingSeed, finished->result());
        result.setProperty("status", QJSValue(QStringLiteral("completed")));
        result.setProperty("accepted", QJSValue(true));
        result.setProperty("rays_requested", QJSValue(static_cast<double>(m_pendingRays)));
    } else {
        // Cancellation is NOT a successful trace. Return counters only; do
        // not supply final area, irradiance, or power that could be mistaken
        // for scientifically accepted results.
        result = m_engine->newObject();
        result.setProperty("status", QJSValue(QStringLiteral("cancelled")));
        result.setProperty("accepted", QJSValue(false));
        result.setProperty("no_export", QJSValue(true));
        result.setProperty("photon_export", QJSValue(false));
        result.setProperty("export_failed", QJSValue(finished->exportFailed()));
        result.setProperty("rays_requested", QJSValue(static_cast<double>(m_pendingRays)));
        result.setProperty("rays_traced", QJSValue(static_cast<double>(finished->fullyCompletedRays())));
        result.setProperty("chunks_completed", QJSValue(static_cast<double>(finished->fullyCompletedChunks())));
        result.setProperty("elapsed_seconds", QJSValue(finished->result().elapsedSeconds));
    }
    return result;
}

bool HeadlessScriptApi::abandonTrace()
{
    if (!m_pendingTrace)
        return false;
    std::unique_ptr<SimulationSession> abandoned = std::move(m_pendingTrace);
    if (abandoned->isRunning()) {
        abandoned->cancel();
        QString ignoredError;
        abandoned->waitForFinished(&ignoredError);
    }
    return true;
}

void HeadlessScriptApi::initializeSceneServices(const QString& fileName, CorePluginRegistry* plugins) const
{
    if (plugins)
        plugins->loadScenePlugins(TonatiuhCore::pluginSearchPaths(QCoreApplication::applicationDirPath()));
    TonatiuhCore::setProjectSearchPaths(fileName);
}

void HeadlessScriptApi::recordError(const QString& message)
{
    m_errors << message;
    QTextStream err(stderr);
    err << message << Qt::endl;
}

QString HeadlessScriptApi::toDisplayString(const QJSValue& value) const
{
    if (value.isUndefined())
        return "undefined";
    if (value.isNull())
        return "null";
    if (value.isString() || value.isNumber() || value.isBool())
        return value.toString();

    QString errorMessage;
    const QString json = toJsonString(value, &errorMessage);
    if (errorMessage.isEmpty())
        return json.trimmed();

    return value.toString();
}

QString HeadlessScriptApi::toJsonString(const QJSValue& value, QString* errorMessage) const
{
    if (errorMessage)
        errorMessage->clear();

    if (!m_engine) {
        if (errorMessage)
            *errorMessage = "script engine is not available.";
        return QString();
    }

    QJSValue stringify = m_engine->evaluate("(function(value) { return JSON.stringify(value, null, 2); })",
        QStringLiteral("<headless-json>"));
    if (stringify.isError()) {
        if (errorMessage)
            *errorMessage = stringify.toString();
        return QString();
    }

    QJSValueList arguments;
    arguments << value;
    QJSValue serialized = stringify.call(arguments);
    if (serialized.isError()) {
        if (errorMessage)
            *errorMessage = serialized.toString();
        return QString();
    }
    if (serialized.isUndefined()) {
        if (errorMessage)
            *errorMessage = "value is not JSON serializable.";
        return QString();
    }

    return serialized.toString() + "\n";
}

int HeadlessScriptHost::runScript(const QString& fileName) const
{
    QTextStream err(stderr);

    if (fileName.trimmed().isEmpty()) {
        err << "run-script requires a script file path." << Qt::endl;
        return 2;
    }

    QFileInfo scriptInfo(fileName);
    if (!scriptInfo.exists() || !scriptInfo.isFile()) {
        err << "Script file not found: " << scriptInfo.absoluteFilePath() << Qt::endl;
        return 1;
    }

    QFile scriptFile(scriptInfo.absoluteFilePath());
    if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        err << "Cannot open script file " << scriptInfo.absoluteFilePath() << ": " << scriptFile.errorString() << Qt::endl;
        return 1;
    }

    const QByteArray scriptBytes = scriptFile.readAll();
    if (scriptFile.error() != QFileDevice::NoError) {
        err << "Cannot read script file " << scriptInfo.absoluteFilePath() << ": " << scriptFile.errorString() << Qt::endl;
        return 1;
    }

    const QString source = QString::fromUtf8(scriptBytes);

    // This script owns its in-memory document and factory registry; neither
    // depends on widgets, a MainWindow, or a process-global scene pointer.
    TonatiuhCore::initializeCoin();
    ScriptHost host;
    QJSEngine& engine = *host.engine();
    auto* nodeServices = new ScriptSceneServices(&host);
    auto factories = std::make_shared<CorePluginRegistry>();
    auto document = std::make_shared<ScriptSceneDocument>();
    auto factoriesLoaded = std::make_shared<bool>(false);
    nodeServices->setNodeCreator([factories, factoriesLoaded](
        ScriptSceneServices::FactoryKind kind, const QString& name) -> SoNode* {
        if (!*factoriesLoaded) {
            factories->loadScenePlugins(
                TonatiuhCore::pluginSearchPaths(QCoreApplication::applicationDirPath()));
            *factoriesLoaded = true;
        }
        switch (kind) {
        case ScriptSceneServices::FactoryKind::Shape:
            return factories->createSceneNode("shape", name);
        case ScriptSceneServices::FactoryKind::Profile:
            return factories->createSceneNode("profile", name);
        case ScriptSceneServices::FactoryKind::Material:
            return factories->createSceneNode("material", name);
        case ScriptSceneServices::FactoryKind::Tracker:
            return factories->createSceneNode("tracker", name);
        case ScriptSceneServices::FactoryKind::Sun:
            return factories->createSceneNode("sun", name);
        case ScriptSceneServices::FactoryKind::Air:
            return factories->createSceneNode("air", name);
        }
        return nullptr;
    });
    nodeServices->setSceneProvider([document] { return document->scene(); });
    nodeServices->setClearDocument([document](QString* error) {
        return document->clear(error);
    });
    nodeServices->setInsertNode([document](SoNode* node, QString* error) {
        return document->insert(node, error);
    });
    nodeServices->setSaveDocument([document](const QString& path, QString* error) {
        return document->save(path, error);
    });
    nodeServices->setLoadDocument([document, factories, factoriesLoaded](
                                   const QString& path, QString* error) {
        if (!*factoriesLoaded) {
            factories->loadScenePlugins(
                TonatiuhCore::pluginSearchPaths(QCoreApplication::applicationDirPath()));
            *factoriesLoaded = true;
        }
        return document->load(path, error);
    });
    registerScriptSceneServices(&engine, nodeServices);
    HeadlessScriptApi api(&engine);
    QJSEngine::setObjectOwnership(&api, QJSEngine::CppOwnership);
    engine.globalObject().setProperty("__tonatiuhppHeadlessApi", engine.newQObject(&api));

    const QString bootstrap = QStringLiteral(R"JS(
(function(global) {
  const api = global.__tonatiuhppHeadlessApi;
  const sceneApi = global.__tonatiuhppSceneApi;
  global.print = function(value) {
    return api.print(value);
  };

  function requirePath(apiName, path) {
    if (path === undefined || path === null || String(path).trim() === "") {
      throw new Error(apiName + " requires a non-empty path.");
    }
    return String(path);
  }

  const tnApi = {
    // Forward bound QObject methods rather than adding a JavaScript call frame.
    // Native errors should identify the caller in the user script.
    getScene: sceneApi.getScene,
    Clear: sceneApi.Clear,
    InsertScene: sceneApi.InsertScene,
    saveScene: sceneApi.saveScene,
    loadScene: sceneApi.loadScene,
    FindInterception: sceneApi.FindInterception,
    writeJson: function(path, value) {
      return api.writeJson(requirePath("tn.writeJson", path), value);
    },
    validateScene: function(path) {
      return api.validateScene(requirePath("tn.validateScene", path));
    },
    runBenchmark: function(path) {
      return api.runBenchmark(requirePath("tn.runBenchmark", path));
    },
    traceScene: function(options) {
      return api.traceScene(options);
    },
    // Forward the bound native functions for accurate user-source lines.
    startTrace: api.startTrace,
    cancelTrace: api.cancelTrace,
    finishTrace: api.finishTrace
  };

  if (typeof Proxy === "function") {
    global.tn = new Proxy(tnApi, {
      get: function(target, property) {
        if (property in target) {
          return target[property];
        }
        if (String(property) === "then") {
          return undefined;
        }
        return function() {
          throw new Error("Headless script API does not support '" + String(property) + "'. Available APIs: print(value), tn.getScene(), tn.Clear(), tn.InsertScene(node), tn.saveScene(path), tn.loadScene(path), tn.FindInterception(surface, rays), tn.writeJson(path, value), tn.validateScene(path), tn.runBenchmark(path), tn.traceScene(options), tn.startTrace(options), tn.cancelTrace(), tn.finishTrace().");
        };
      }
    });
  } else {
    global.tn = tnApi;
  }

  global.tonatiuh = global.tn;
})(this);
)JS");

    QJSValue bootstrapResult = host.evaluate(bootstrap, QStringLiteral("<headless-bootstrap>"));
    if (bootstrapResult.isError()) {
        err << "Cannot initialize headless script host: " << bootstrapResult.toString() << Qt::endl;
        return 1;
    }

    QJSValue result = host.evaluate(source, scriptInfo.absoluteFilePath(), 1);
    const bool unfinishedTrace = api.abandonTrace(); // Joins even on JS errors.
    if (result.isError()) {
        printScriptError(result, scriptInfo.absoluteFilePath(), source);
        return 1;
    }

    if (unfinishedTrace) {
        err << "Script ended without tn.finishTrace: unfinished trace was cancelled and joined." << Qt::endl;
        return 1;
    }
    if (api.hasErrors()) {
        err << "Headless script completed with " << api.errors().size() << " API error(s)." << Qt::endl;
        return 1;
    }

    return 0;
}

void HeadlessScriptHost::printScriptError(const QJSValue& error, const QString& fileName, const QString& source) const
{
    QTextStream err(stderr);

    const int lineNumber = error.property("lineNumber").toInt();
    err << "Script execution failed: "
        << ScriptHost::formatError(error, fileName) << Qt::endl;

    const QString context = scriptLineAt(source, lineNumber);
    if (!context.isEmpty())
        err << "  " << lineNumber << " | " << context << Qt::endl;

    const QString stack = error.property("stack").toString();
    if (!stack.isEmpty())
        err << "Stack:" << Qt::endl << stack << Qt::endl;
}
