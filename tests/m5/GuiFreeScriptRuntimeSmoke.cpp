// M5.1B1: a real, separately linked JS runtime without any Tonatiuh kernel,
// simulation, scene/plugin or GUI dependency. Uses QCoreApplication only.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJSValue>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

#include "script/ScriptHost.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir scratch;
    if (!scratch.isValid()) return 2;
    const QString scriptDirectory = scratch.path() + QStringLiteral("/source");
    if (!QDir().mkpath(scriptDirectory)) return 3;
    const QString sourceName = scriptDirectory + QStringLiteral("/runtime.tnhpps");

    ScriptHost host;
    QStringList output;
    host.setPrintCallback([&output](const QString& message) { output << message; });

    const QJSValue result = host.evaluate(QStringLiteral(
        "var m5Data = new DataObject();"
        "m5Data.addRow('alpha,beta');"
        "if (!m5Data.write('m5-runtime.csv')) throw new Error('write failed');"
        "m5Data.clear();"
        "if (!m5Data.read('m5-runtime.csv')) throw new Error('read failed');"
        "print(6 * 7);"
        "m5Data.rows() === 1 && m5Data.row(0) === 'alpha,beta';"), sourceName);
    if (result.isError() || !result.isBool() || !result.toBool()) return 4;
    if (output != QStringList{QStringLiteral("42")}) return 5;
    const QString expectedOutput = scriptDirectory + QStringLiteral("/m5-runtime.csv");
    if (!QFile::exists(expectedOutput)) return 6;
    if (host.resolvePath(QStringLiteral("m5-runtime.csv")) != expectedOutput) return 7;

    const QJSValue exception = host.evaluate(QStringLiteral(
        "throw new Error('m5-intentional-error');"), sourceName);
    if (!exception.isError()) return 8;
    if (!ScriptHost::formatError(exception, sourceName).contains(
            QStringLiteral("m5-intentional-error"))) return 9;
    if (QCoreApplication::instance() != &app) return 10;
    return 0;
}
