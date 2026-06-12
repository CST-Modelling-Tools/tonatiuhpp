#include "HeadlessCommandRunner.h"

#include <limits>

#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>
#include <QThread>

#include "benchmark/BenchmarkRunner.h"
#include "core/CorePluginRegistry.h"
#include "core/RayTraceRunner.h"
#include "core/SceneLoader.h"
#include "core/TonatiuhCore.h"

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

    RayTraceOptions options;
    options.rays = parsed.rays;
    options.seed = parsed.seed;
    options.workerCount = qMax(1, QThread::idealThreadCount());
    options.chunkSize = 10000;

    RayTraceResult result;
    RayTraceRunner runner;
    if (!runner.trace(scene.get(), options, &result, &errorMessage, [&out](const QString& message) {
            out << message << Qt::endl;
        })) {
        err << "Trace failed: " << errorMessage << Qt::endl;
        return 1;
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

    auto fail = [errorMessage](const QString& message) {
        if (errorMessage)
            *errorMessage = message;
        return false;
    };

    if (!parsed)
        return fail("Internal argument parser error.");
    if (args.isEmpty())
        return fail("trace-scene requires a scene file path.");

    parsed->sceneFileName = args[0];
    if (parsed->sceneFileName.startsWith("--"))
        return fail("trace-scene requires a scene file path before options.");

    for (int i = 1; i < args.size(); ++i) {
        const QString option = args[i];

        if (option == "--rays") {
            if (parsed->hasRays)
                return fail("--rays was specified more than once.");
            if (++i >= args.size())
                return fail("--rays requires a positive integer value.");
            if (!parseUnsignedLongOption("--rays", args[i], false, &parsed->rays, errorMessage))
                return false;
            parsed->hasRays = true;
        } else if (option == "--seed") {
            if (parsed->hasSeed)
                return fail("--seed was specified more than once.");
            if (++i >= args.size())
                return fail("--seed requires an integer value.");
            if (!parseUnsignedLongOption("--seed", args[i], true, &parsed->seed, errorMessage))
                return false;
            parsed->hasSeed = true;
        } else if (option == "--no-export") {
            if (parsed->noExport)
                return fail("--no-export was specified more than once.");
            parsed->noExport = true;
        } else {
            return fail(QString("Unknown trace-scene option: %1.").arg(option));
        }
    }

    if (!parsed->hasRays)
        return fail("trace-scene requires --rays N.");
    if (!parsed->hasSeed)
        return fail("trace-scene requires --seed S.");
    if (!parsed->noExport)
        return fail("trace-scene currently requires --no-export.");

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
    out << "  tonatiuhpp --headless trace-scene <scene.tnhpp> --rays N --seed S --no-export" << Qt::endl;
    out << "  tonatiuhpp --headless benchmark <benchmark_config.json>" << Qt::endl;
    out << Qt::endl;
    out << "Commands:" << Qt::endl;
    out << "  validate-scene <scene.tnhpp>                         Validate that a Tonatiuh++ scene can be loaded." << Qt::endl;
    out << "  trace-scene <scene.tnhpp> --rays N --seed S --no-export" << Qt::endl;
    out << "                                                     Run ray tracing without photon export." << Qt::endl;
    out << "  benchmark <benchmark_config.json>                  Run a headless benchmark and write JSON results." << Qt::endl;
}

int HeadlessCommandRunner::printUsageError(const QString& message) const
{
    QTextStream err(stderr);
    err << message << Qt::endl;
    err << "Use tonatiuhpp --headless --help for usage." << Qt::endl;
    return 2;
}
