#include "HeadlessCommandRunner.h"

#include <limits>
#include <memory>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QTextStream>

#include "benchmark/BenchmarkRunner.h"
#include "core/CorePluginRegistry.h"
#include "core/NativeTraceSignature.h"
#include "core/RayTraceExecutor.h"
#include "core/SceneLoader.h"
#include "core/TracePreparation.h"
#include "core/TonatiuhCore.h"
#include "headless/HeadlessScriptHost.h"

namespace
{
class TextProgressReporter
{
public:
    explicit TextProgressReporter(QTextStream* stream):
        m_stream(stream)
    {
    }

    void operator()(const QString& message) const
    {
        if (m_stream)
            *m_stream << message << Qt::endl;
    }

private:
    QTextStream* m_stream = nullptr;
};

bool failParse(QString* errorMessage, const QString& message)
{
    if (errorMessage)
        *errorMessage = message;
    return false;
}
}

int HeadlessCommandRunner::run(const QStringList& arguments) const
{
    QStringList args = arguments;
    if (!args.isEmpty())
        args.removeFirst();

    args.removeAll("--headless");

    if (args.isEmpty() || args[0] == "--help" || args[0] == "-h") {
        printUsage();
        return 0;
    }

    const QString command = args[0];
    if (command == "validate-scene") {
        if (args.size() != 2)
            return printUsageError("validate-scene requires exactly one scene file path.");

        return validateScene(args[1]);
    }

    if (command == "trace-scene")
        return traceScene(args.mid(1));

    if (command == "benchmark")
        return benchmark(args.mid(1));

    if (command == "run-script")
        return runScript(args.mid(1));

    return printUsageError(QString("Unknown headless command: %1.").arg(command));
}

int HeadlessCommandRunner::validateScene(const QString& fileName) const
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    initializeSceneServices(fileName, &plugins);

    LoadedScene scene;
    QString errorMessage;
    if (!SceneLoader::readFile(fileName, &scene, &errorMessage)) {
        err << "Scene validation failed: " << errorMessage << Qt::endl;
        return 1;
    }

    out << "Scene validation succeeded: " << QFileInfo(fileName).absoluteFilePath() << Qt::endl;
    return 0;
}

int HeadlessCommandRunner::traceScene(const QStringList& args) const
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    TraceSceneArguments parsed;
    QString errorMessage;
    if (!parseTraceSceneArguments(args, &parsed, &errorMessage))
        return printUsageError(errorMessage);

    const QString a0SignatureFile = NativeTraceSignature::outputPathFromEnvironment();
    std::unique_ptr<NativeTraceSignature> a0Signature;
    if (!a0SignatureFile.isEmpty()) {
        if (!NativeTraceSignature::validateRayCount(parsed.rays, &errorMessage)) {
            err << "A0 diagnostic failed: " << errorMessage << Qt::endl;
            return 1;
        }
        a0Signature = std::make_unique<NativeTraceSignature>();
    }

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    initializeSceneServices(parsed.sceneFileName, &plugins);

    LoadedScene scene;
    if (!SceneLoader::readFile(parsed.sceneFileName, &scene, &errorMessage)) {
        err << "Scene load failed: " << errorMessage << Qt::endl;
        return 1;
    }

    const QString sceneFilePath = QFileInfo(parsed.sceneFileName).absoluteFilePath();
    out << "Tracing scene: " << sceneFilePath << Qt::endl;
    out << "scene_file: " << sceneFilePath << Qt::endl;
    out << "rays: " << parsed.rays << Qt::endl;
    out << "seed: " << parsed.seed << Qt::endl;
    out << "photon_export: false" << Qt::endl;
    out << "export_path: none" << Qt::endl;
    if (parsed.updateTrackers)
        out << "tracker_update: enabled" << Qt::endl;

    QElapsedTimer timer;
    timer.start();
    TextProgressReporter progress(&out);
    HeadlessTracePreparationInput preparationInput;
    preparationInput.scene = scene.get();
    preparationInput.configuration.rays = parsed.rays;
    preparationInput.configuration.masterSeed = parsed.seed;
    preparationInput.updateTrackers = parsed.updateTrackers;
    preparationInput.progress = progress;
    if (a0Signature) {
        preparationInput.hitCallback = [signature = a0Signature.get()](const RayTracerHit& hit) {
            signature->add(hit);
        };
    }
    PreparedTraceContext context;
    RayTraceExecutorResult result;
    if (!TracePreparation::prepareHeadlessTrace(preparationInput, &context, &errorMessage)) {
        err << "Trace failed: " << errorMessage << Qt::endl;
        return 1;
    }
    TracePreparation::initializeResult(context, &result);
    NativeTraceSignature::Inputs a0Inputs;
    if (a0Signature) {
        a0Inputs.sceneFile = parsed.sceneFileName;
        a0Inputs.rays = parsed.rays;
        a0Inputs.seed = parsed.seed;
        a0Inputs.sunGridWidth = preparationInput.configuration.sunWidthDivisions;
        a0Inputs.sunGridHeight = preparationInput.configuration.sunHeightDivisions;
        a0Inputs.apertureArea = context.sunApertureArea();
        a0Inputs.irradiance = context.irradiance();
        a0Inputs.powerPerRay = context.powerPerRay();
    }
    progress("Starting ray loop.");
    RayTraceExecutor executor;
    RayTraceExecution execution = executor.start(std::move(context));
    if (!execution.started) {
        err << "Trace failed: " << execution.errorMessage << Qt::endl;
        return 1;
    }
    if (!executor.waitForFinished(&execution, &errorMessage) ||
        !TracePreparation::finalizeResult(*execution.context(), executor.exportFailed(), timer.elapsed() / 1000., &result, &errorMessage)) {
        err << "Trace failed: " << errorMessage << Qt::endl;
        return 1;
    }

    if (a0Signature) {
        if (execution.future.isCanceled()
            || !a0Signature->write(a0SignatureFile, "headless-cli",
                                   a0Inputs, &errorMessage)) {
            err << "A0 diagnostic failed: "
                << (errorMessage.isEmpty() ? "Trace was cancelled." : errorMessage)
                << Qt::endl;
            return 1;
        }
        out << "a0_headless_signature: " << a0SignatureFile << Qt::endl;
    }

    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);
    out << "Trace completed." << Qt::endl;
    out << "rays_traced: " << result.raysTraced << Qt::endl;
    out << "elapsed_seconds: " << result.elapsedSeconds << Qt::endl;
    out << "rays_per_second: " << result.raysPerSecond << Qt::endl;
    out << "worker_count: " << result.workerCount << Qt::endl;
    out << "chunk_count: " << result.chunkCount << Qt::endl;
    out << "chunk_size: " << result.chunkSize << Qt::endl;
    return 0;
}

int HeadlessCommandRunner::benchmark(const QStringList& args) const
{
    QTextStream err(stderr);

    if (args.size() != 1)
        return printUsageError("benchmark requires exactly one benchmark config JSON file path.");

    BenchmarkRunner benchmarkRunner;
    QString errorMessage;
    const QString sceneFileName = benchmarkRunner.sceneFileName(args[0], &errorMessage);
    if (sceneFileName.isEmpty()) {
        err << "Benchmark configuration failed: " << errorMessage << Qt::endl;
        return 1;
    }

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    initializeSceneServices(sceneFileName, &plugins);

    LoadedScene scene;
    if (!SceneLoader::readFile(sceneFileName, &scene, &errorMessage)) {
        err << "Scene load failed: " << errorMessage << Qt::endl;
        return 1;
    }

    const int result = benchmarkRunner.run(args[0], scene.get(), &errorMessage);
    if (result != 0)
        err << "Benchmark failed: " << errorMessage << Qt::endl;
    return result;
}

int HeadlessCommandRunner::runScript(const QStringList& args) const
{
    if (args.size() != 1)
        return printUsageError("run-script requires exactly one script file path.");

    HeadlessScriptHost host;
    return host.runScript(args[0]);
}

void HeadlessCommandRunner::initializeSceneServices(const QString& fileName, CorePluginRegistry* plugins) const
{
    if (plugins)
        plugins->loadScenePlugins(TonatiuhCore::pluginSearchPaths(QCoreApplication::applicationDirPath()));
    TonatiuhCore::setProjectSearchPaths(fileName);
}

bool HeadlessCommandRunner::parseTraceSceneArguments(const QStringList& args, TraceSceneArguments* parsed, QString* errorMessage) const
{
    if (parsed)
        *parsed = TraceSceneArguments();

    if (!parsed)
        return failParse(errorMessage, "Internal argument parser error.");
    if (args.isEmpty())
        return failParse(errorMessage, "trace-scene requires a scene file path.");

    parsed->sceneFileName = args[0];
    if (parsed->sceneFileName.startsWith("--"))
        return failParse(errorMessage, "trace-scene requires a scene file path before options.");

    for (int i = 1; i < args.size(); ++i) {
        const QString option = args[i];

        if (option == "--rays") {
            if (parsed->hasRays)
                return failParse(errorMessage, "--rays was specified more than once.");
            if (++i >= args.size())
                return failParse(errorMessage, "--rays requires a positive integer value.");
            if (!parseUnsignedLongOption("--rays", args[i], false, &parsed->rays, errorMessage))
                return false;
            parsed->hasRays = true;
        } else if (option == "--seed") {
            if (parsed->hasSeed)
                return failParse(errorMessage, "--seed was specified more than once.");
            if (++i >= args.size())
                return failParse(errorMessage, "--seed requires an integer value.");
            if (!parseUnsignedLongOption("--seed", args[i], true, &parsed->seed, errorMessage))
                return false;
            parsed->hasSeed = true;
        } else if (option == "--no-export") {
            if (parsed->noExport)
                return failParse(errorMessage, "--no-export was specified more than once.");
            parsed->noExport = true;
        } else if (option == "--update-trackers") {
            if (parsed->updateTrackers)
                return failParse(errorMessage, "--update-trackers was specified more than once.");
            parsed->updateTrackers = true;
        } else {
            return failParse(errorMessage, QString("Unknown trace-scene option: %1.").arg(option));
        }
    }

    if (!parsed->hasRays)
        return failParse(errorMessage, "trace-scene requires --rays N.");
    if (!parsed->hasSeed)
        return failParse(errorMessage, "trace-scene requires --seed S.");
    if (!parsed->noExport)
        return failParse(errorMessage, "trace-scene currently requires --no-export.");

    return true;
}

bool HeadlessCommandRunner::parseUnsignedLongOption(const QString& optionName, const QString& value, bool allowZero, ulong* parsed, QString* errorMessage) const
{
    bool ok = false;
    const qulonglong parsedValue = value.toULongLong(&ok);
    if (!ok) {
        if (errorMessage)
            *errorMessage = QString("%1 requires an integer value.").arg(optionName);
        return false;
    }

    if (!allowZero && parsedValue == 0) {
        if (errorMessage)
            *errorMessage = QString("%1 requires a positive integer value.").arg(optionName);
        return false;
    }

    if (parsedValue > std::numeric_limits<ulong>::max()) {
        if (errorMessage)
            *errorMessage = QString("%1 value is too large for this build.").arg(optionName);
        return false;
    }

    if (parsed)
        *parsed = static_cast<ulong>(parsedValue);
    return true;
}

void HeadlessCommandRunner::printUsage() const
{
    QTextStream out(stdout);
    out << "Tonatiuh++ headless mode" << Qt::endl;
    out << Qt::endl;
    out << "Usage:" << Qt::endl;
    out << "  tonatiuhpp --headless --help" << Qt::endl;
    out << "  tonatiuhpp --headless validate-scene <scene.tnhpp>" << Qt::endl;
    out << "  tonatiuhpp --headless trace-scene <scene.tnhpp> --rays N --seed S --no-export [--update-trackers]" << Qt::endl;
    out << "  tonatiuhpp --headless benchmark <benchmark_config.json>" << Qt::endl;
    out << "  tonatiuhpp --headless run-script <script.tnhpps>" << Qt::endl;
    out << Qt::endl;
    out << "Commands:" << Qt::endl;
    out << "  validate-scene <scene.tnhpp>                         Validate that a Tonatiuh++ scene can be loaded." << Qt::endl;
    out << "  trace-scene <scene.tnhpp> --rays N --seed S --no-export [--update-trackers]" << Qt::endl;
    out << "                                                     Run ray tracing without photon export." << Qt::endl;
    out << "                                                     --update-trackers applies GUI-like tracking (opt-in)." << Qt::endl;
    out << "  benchmark <benchmark_config.json>                  Run a headless benchmark and write JSON results." << Qt::endl;
    out << "  run-script <script.tnhpps>                         Run a script through the limited true-headless API." << Qt::endl;
    out << Qt::endl;
    out << "Headless script API:" << Qt::endl;
    out << "  print(value)" << Qt::endl;
    out << "  tn.writeJson(path, value)" << Qt::endl;
    out << "  tn.validateScene(path)" << Qt::endl;
    out << "  tn.runBenchmark(path)" << Qt::endl;
    out << "  tn.traceScene({ scene, rays, seed, noExport: true })" << Qt::endl;
}

int HeadlessCommandRunner::printUsageError(const QString& message) const
{
    QTextStream err(stderr);
    err << message << Qt::endl;
    err << "Use tonatiuhpp --headless --help for usage." << Qt::endl;
    return 2;
}
