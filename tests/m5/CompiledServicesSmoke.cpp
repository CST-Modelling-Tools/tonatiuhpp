// M5.1A: verify the two compiled domain libraries can be consumed by a
// QCoreApplication-only entry point. Does NOT certify transitive GUI-free
// linkage while TonatiuhKernel and legacy plugin factories still depend on UI.
#include <QCoreApplication>
#include <QJSValue>
#include <QString>

#include "core/SimulationSession.h"
#include "script/ScriptHost.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    SimulationSession simulation;
    if (simulation.state() != SimulationSession::State::Idle)
        return 2;

    ScriptHost scripts;
    const QJSValue evaluated = scripts.evaluate(QStringLiteral("6 * 7"),
                                                QStringLiteral("<m5-smoke>"));
    if (!evaluated.isNumber() || evaluated.toInt() != 42)
        return 3;
    if (QCoreApplication::instance() != &app)
        return 4;
    return 0;
}
