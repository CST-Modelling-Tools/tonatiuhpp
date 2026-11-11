// Tonatiuh++ production command-line entrypoint: QCoreApplication only.
// The GUI owns its own QApplication entrypoint. They share one kernel.
#include <QCoreApplication>
#include "core/TonatiuhCore.h"
#include "headless/HeadlessCommandRunner.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tonatiuh++ CLI"));
    app.setApplicationVersion(QString::fromLatin1(APP_VERSION));
    TonatiuhCore::setHeadlessPluginMode(true);
    HeadlessCommandRunner runner;
    return runner.run(app.arguments());
}
