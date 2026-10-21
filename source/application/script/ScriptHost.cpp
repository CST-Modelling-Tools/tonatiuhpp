#include "ScriptHost.h"

#include <utility>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

#include "script/DataObject.h"

ScriptHost::ScriptHost(QObject* parent)
    : QObject(parent)
    , m_scriptDirectory(QDir::currentPath())
{
    // A pointer to the host is used for context lookup by DataObject only.
    // It is owned by C++; exposing it must not transfer ownership to JS.
    QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
    const QJSValue context = m_engine.newQObject(this);
    m_engine.globalObject().setProperty("__tonatiuhppScriptHost", context);
    m_engine.globalObject().setProperty("print", context.property("print"));
    m_engine.globalObject().setProperty("printTimed", context.property("printTimed"));
    m_engine.globalObject().setProperty(
        "DataObject", m_engine.newQMetaObject(&DataObject::staticMetaObject));
}

QJSValue ScriptHost::evaluate(const QString& source, const QString& fileName, int firstLine)
{
    if (!fileName.isEmpty() && !fileName.startsWith(QLatin1Char('<'))) {
        m_scriptDirectory = QFileInfo(fileName).absoluteDir().absolutePath();
    }
    return m_engine.evaluate(source, fileName, firstLine);
}

QString ScriptHost::resolvePath(const QString& path) const
{
    // Legacy DataObject files used the "project:" prefix. Keep the prefix as
    // an input alias while resolving against this *host*, not QDir globals.
    const QString relative = path.startsWith(QStringLiteral("project:"))
        ? path.mid(8) : path;
    if (relative.startsWith(QStringLiteral(":/")))
        return relative;
    if (QDir::isAbsolutePath(relative))
        return QDir::cleanPath(relative);
    return QDir(m_scriptDirectory).absoluteFilePath(relative);
}

QString ScriptHost::formatError(const QJSValue& error, const QString& sourceName)
{
    const QJSValue fileProperty = error.property("fileName");
    const QString fileName = fileProperty.isString() ? fileProperty.toString() : QString();
    const int line = error.property("lineNumber").toInt();
    QString result = fileName.isEmpty() ? sourceName : fileName;
    if (line > 0)
        result += QStringLiteral(":%1").arg(line);
    result += QStringLiteral(": %1").arg(error.toString());
    return result;
}

void ScriptHost::print(const QJSValue& value)
{
    output(value, false);
}

void ScriptHost::printTimed(const QJSValue& value)
{
    output(value, true);
}

void ScriptHost::output(const QJSValue& value, bool timed)
{
    QString message = value.toString();
    if (timed) {
        message.prepend(QStringLiteral("[%1] ").arg(
            QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))));
    }
    if (m_print) {
        m_print(message);
    } else {
        QTextStream out(stdout);
        out << message << Qt::endl;
    }
}
