#include "HeadlessScriptHost.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QTextStream>

#include "benchmark/BenchmarkRunner.h"
#include "core/CorePluginRegistry.h"
#include "core/SceneLoader.h"
#include "core/TonatiuhCore.h"

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
}

HeadlessScriptApi::HeadlessScriptApi(QJSEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

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

    QFileInfo info(fileName);
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

    const QString source = QString::fromUtf8(scriptFile.readAll());

    QJSEngine engine;
    HeadlessScriptApi api(&engine);
    QJSEngine::setObjectOwnership(&api, QJSEngine::CppOwnership);
    engine.globalObject().setProperty("__tonatiuhppHeadlessApi", engine.newQObject(&api));

    const QString bootstrap = QStringLiteral(R"JS(
(function(global) {
  const api = global.__tonatiuhppHeadlessApi;
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
    writeJson: function(path, value) {
      return api.writeJson(requirePath("tn.writeJson", path), value);
    },
    validateScene: function(path) {
      return api.validateScene(requirePath("tn.validateScene", path));
    },
    runBenchmark: function(path) {
      return api.runBenchmark(requirePath("tn.runBenchmark", path));
    }
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
          throw new Error("Headless script API does not support '" + String(property) + "'. Available APIs: print(value), tn.writeJson(path, value), tn.validateScene(path), tn.runBenchmark(path).");
        };
      }
    });
  } else {
    global.tn = tnApi;
  }

  global.tonatiuh = global.tn;
})(this);
)JS");

    QJSValue bootstrapResult = engine.evaluate(bootstrap, QStringLiteral("<headless-bootstrap>"));
    if (bootstrapResult.isError()) {
        err << "Cannot initialize headless script host: " << bootstrapResult.toString() << Qt::endl;
        return 1;
    }

    QJSValue result = engine.evaluate(source, scriptInfo.absoluteFilePath(), 1);
    if (result.isError()) {
        printScriptError(result, scriptInfo.absoluteFilePath(), source);
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
    const QString message = error.toString();
    err << "Script execution failed: " << fileName;
    if (lineNumber > 0)
        err << ":" << lineNumber;
    err << ": " << message << Qt::endl;

    const QString context = scriptLineAt(source, lineNumber);
    if (!context.isEmpty())
        err << "  " << lineNumber << " | " << context << Qt::endl;

    const QString stack = error.property("stack").toString();
    if (!stack.isEmpty())
        err << "Stack:" << Qt::endl << stack << Qt::endl;
}
