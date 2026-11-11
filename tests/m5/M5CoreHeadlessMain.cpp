// M5.1: one production TonatiuhKernel, libraries, simulation and script service,
// linked by GUI and CLI; this file is only a test probe.
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJSValue>
#include <QTemporaryDir>
#include <QStringList>

#include "core/CorePluginRegistry.h"
#include "core/PhotonExportFileService.h"
#include "core/SimulationSession.h"
#include "core/TonatiuhCore.h"
#include "kernel/photons/PhotonsSettings.h"
#include "script/ScriptHost.h"
#include "script/ScriptSceneDocument.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tonatiuh"));
    app.setApplicationVersion(QString::fromLatin1(APP_VERSION));
    TonatiuhCore::setHeadlessPluginMode(true);
    if (app.arguments().contains(QStringLiteral("--m5-core-smoke"))) {
        TonatiuhCore::initializeCoin();
        CorePluginRegistry registry;
        ScriptSceneDocument scene;
        if (!scene.scene()) return 2;
        SimulationSession session;
        if (session.state() != SimulationSession::State::Idle) return 3;
        ScriptHost host;
        const QJSValue result = host.evaluate(QStringLiteral("6 * 7"),
                                               QStringLiteral("<m5-core-smoke>"));
        if (result.isError() || result.toInt() != 42) return 4;
        if (QCoreApplication::instance() != &app) return 5;
        return 0;
    }
    if (app.arguments().contains(QStringLiteral("--m5-plugin-smoke"))) {
        TonatiuhCore::initializeCoin();
        const QStringList roots = TonatiuhCore::pluginSearchPaths(
            QCoreApplication::applicationDirPath());
        CorePluginRegistry registry;
        registry.loadScenePlugins(roots);
        // Exercise an external first-party scene-plugin class in the same
        // kernel type registry, not an alternate duplicated DLL implementation.
        SoNode* specular = registry.createSceneNode(QStringLiteral("material"),
                                                   QStringLiteral("Specular"));
        if (!specular) return 6;
        QTemporaryDir temporary;
        if (!temporary.isValid()) return 7;
        PhotonExportFileService::Plugin plugin;
        QString error;
        if (!PhotonExportFileService::createFileExporter(
                roots, &plugin.loader, &plugin.exporter, &error)) return 8;
        PhotonsSettings settings;
        settings.name = QStringLiteral("File");
        settings.parameters.insert(QStringLiteral("ExportDirectory"), temporary.path());
        settings.parameters.insert(QStringLiteral("ExportFile"), QStringLiteral("M5PluginSmoke"));
        settings.parameters.insert(QStringLiteral("FileSize"), QStringLiteral("-1"));
        plugin.exporter->setPhotonSettings(&settings);
        if (!plugin.exporter->startExport()) return 9;
        if (!plugin.exporter->endExport()) return 10;
        if (!QFileInfo(QDir(temporary.path()).filePath(
                QStringLiteral("M5PluginSmoke_parameters.txt"))).isFile()) return 11;
        return 0;
    }
    return 64; // Test-only executable: no production CLI fallback.
}
