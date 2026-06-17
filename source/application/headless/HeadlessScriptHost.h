#pragma once

#include <QObject>
#include <QJSValue>
#include <QString>
#include <QStringList>

class QJSEngine;
class CorePluginRegistry;

class HeadlessScriptApi : public QObject
{
    Q_OBJECT

public:
    explicit HeadlessScriptApi(QJSEngine* engine, QObject* parent = nullptr);

    Q_INVOKABLE void print(const QJSValue& value);
    Q_INVOKABLE bool writeJson(const QString& fileName, const QJSValue& value);
    Q_INVOKABLE bool validateScene(const QString& fileName);
    Q_INVOKABLE int runBenchmark(const QString& configFileName);
    Q_INVOKABLE QJSValue traceScene(const QJSValue& options);

    bool hasErrors() const { return !m_errors.isEmpty(); }
    QStringList errors() const { return m_errors; }

private:
    void initializeSceneServices(const QString& fileName, CorePluginRegistry* plugins) const;
    void recordError(const QString& message);
    QString toDisplayString(const QJSValue& value) const;
    QString toJsonString(const QJSValue& value, QString* errorMessage) const;

    QJSEngine* m_engine = nullptr;
    QStringList m_errors;
};

class HeadlessScriptHost
{
public:
    int runScript(const QString& fileName) const;

private:
    void printScriptError(const QJSValue& error, const QString& fileName, const QString& source) const;
};
