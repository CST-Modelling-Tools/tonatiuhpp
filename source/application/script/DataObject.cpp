#include "DataObject.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QJSEngine>
#include <QString>
#include <QStringList>

#include "script/ScriptHost.h"


namespace
{
QString dataFilePath(QObject* object, const QString& fileName)
{
    if (QJSEngine* engine = qjsEngine(object)) {
        const QJSValue context = engine->globalObject().property("__tonatiuhppScriptHost");
        if (auto* host = qobject_cast<ScriptHost*>(context.toQObject()))
            return host->resolvePath(fileName);
    }
    // Standalone C++ fallback; never index an empty "project" path list.
    if (QDir::isAbsolutePath(fileName))
        return QDir::cleanPath(fileName);
    const QStringList paths = QDir::searchPaths("project");
    if (!paths.isEmpty())
        return QDir(paths.constFirst()).absoluteFilePath(fileName);
    return QFileInfo(fileName).absoluteFilePath();
}

QJSValue dataFileError(QObject* object, const QString& error)
{
    if (QJSEngine* engine = qjsEngine(object))
        engine->throwError(error);
    return QJSValue(false);
}
}

DataObject::DataObject(QObject* parent):
    QObject(parent)
{

}

QJSValue DataObject::read(const QString& fileName)
{
    m_data.clear();
    if (fileName.trimmed().isEmpty())
        return dataFileError(this, "DataObject.read requires a non-empty file path.");

    const QString path = dataFilePath(this, fileName);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return dataFileError(this, QString("DataObject.read cannot open %1: %2")
                                         .arg(path, file.errorString()));

    QTextStream in(&file);
    QString line;
    while (in.readLineInto(&line))
        m_data << line;
    if (in.status() == QTextStream::ReadCorruptData || file.error() != QFileDevice::NoError) {
        m_data.clear();
        return dataFileError(this, QString("DataObject.read failed for %1").arg(path));
    }
    return true;
}

QJSValue DataObject::write(const QString& fileName)
{
    if (fileName.trimmed().isEmpty())
        return dataFileError(this, "DataObject.write requires a non-empty file path.");

    const QString path = dataFilePath(this, fileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return dataFileError(this, QString("DataObject.write cannot open %1: %2")
                                         .arg(path, file.errorString()));

    QTextStream out(&file);
    for (const QString& line : m_data)
        out << line << Qt::endl;
    out.flush();
    if (out.status() != QTextStream::Ok || file.error() != QFileDevice::NoError)
        return dataFileError(this, QString("DataObject.write failed for %1: %2")
                                         .arg(path, file.errorString()));
    return true;
}

void DataObject::clear()
{
    m_data.clear();
}

QJSValue DataObject::rows()
{
    return (int) m_data.size();
}

QJSValue DataObject::row(int n)
{
    if (n < 0 || n >= m_data.size()) return "";
    return m_data[n];
}

void DataObject::setRow(int n, const QString& line)
{
    if (n < 0 || n >= m_data.size()) return;
    m_data[n] = line;
}

void DataObject::addRow(const QString& line)
{
    m_data << line;
}

QJSValue DataObject::array(int n)
{
    if (n < 0 || n >= m_data.size()) return "";
    QJSEngine* engine = qjsEngine(this);
    if (!engine)
        return QJSValue();
    const QStringList list = m_data[n].split(',');
    const int iMax = list.size();

    QJSValue jsArray = engine->newArray(static_cast<uint>(iMax));
    for (int i = 0; i < iMax; ++i)
        jsArray.setProperty(i, list[i]);
    return jsArray;
}

void DataObject::setArray(int n, QJSValue value)
{
    if (n < 0 || n >= m_data.size()) return;

    QStringList list;
    const int iMax = value.property("length").toInt();
    for (int i = 0; i < iMax; ++i)
        list << value.property(i).toString();

    m_data[n] = list.join(',');
}

void DataObject::addArray(QJSValue value)
{
    QStringList list;
    const int iMax = value.property("length").toInt();
    for (int i = 0; i < iMax; ++i)
        list << value.property(i).toString();

    m_data << list.join(',');
}

//QJSValue DataCSV::part(int n, int m)
//{
//    if (n < 0 || n >= m_data.size()) return "";
//    QVector<QStringRef> refs = m_data[n].splitRef(',');
//    if (m < 0 || m >= refs.size()) return "";
//    return refs[m].toString();
//}
