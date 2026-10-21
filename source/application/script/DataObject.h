#pragma once

#include <QObject>
//#include <QScriptable>
#include <QJSValue>


class DataObject: public QObject//, protected QScriptable
{
    Q_OBJECT

public:
    Q_INVOKABLE DataObject(QObject* parent = 0);

    // Engine and file-directory context are resolved from qjsEngine(this).

public slots:
    QJSValue read(const QString& fileName = "");
    QJSValue write(const QString& fileName = "");
    void clear();

    QJSValue rows();
    QJSValue row(int n);
    void setRow(int n, const QString& line);
    void addRow(const QString& line);

    QJSValue array(int n);
    void setArray(int n, QJSValue value);
    void addArray(QJSValue value);

//    QJSValue part(int n, int m);

private:
    QStringList m_data;
};

