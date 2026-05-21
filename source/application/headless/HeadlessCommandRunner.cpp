#include "HeadlessCommandRunner.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>

#include "core/CorePluginRegistry.h"
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

    return printUsageError(QString("Unknown headless command: %1.").arg(command));
}

int HeadlessCommandRunner::validateScene(const QString& fileName) const
{
    QTextStream out(stdout);
    QTextStream err(stderr);

    TonatiuhCore::initializeCoin();
    CorePluginRegistry plugins;
    plugins.loadScenePlugins(TonatiuhCore::pluginSearchPaths(QCoreApplication::applicationDirPath()));
    TonatiuhCore::setProjectSearchPaths(fileName);

    LoadedScene scene;
    QString errorMessage;
    if (!SceneLoader::readFile(fileName, &scene, &errorMessage)) {
        err << "Scene validation failed: " << errorMessage << Qt::endl;
        return 1;
    }

    out << "Scene validation succeeded: " << QFileInfo(fileName).absoluteFilePath() << Qt::endl;
    return 0;
}

void HeadlessCommandRunner::printUsage() const
{
    QTextStream out(stdout);
    out << "Tonatiuh++ headless mode" << Qt::endl;
    out << Qt::endl;
    out << "Usage:" << Qt::endl;
    out << "  tonatiuhpp --headless --help" << Qt::endl;
    out << "  tonatiuhpp --headless validate-scene <scene.tnhpp>" << Qt::endl;
    out << Qt::endl;
    out << "Commands:" << Qt::endl;
    out << "  validate-scene <scene.tnhpp>  Validate that a Tonatiuh++ scene can be loaded." << Qt::endl;
}

int HeadlessCommandRunner::printUsageError(const QString& message) const
{
    QTextStream err(stderr);
    err << message << Qt::endl;
    err << "Use tonatiuhpp --headless --help for usage." << Qt::endl;
    return 2;
}
