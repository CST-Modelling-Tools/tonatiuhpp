#pragma once

#include <memory>

#include <QObject>
#include <QJSValue>
#include <QString>
#include <QStringList>

class QJSEngine;
class CorePluginRegistry;
class SimulationSession;

class HeadlessScriptApi : public QObject
{
    Q_OBJECT

public:
    explicit HeadlessScriptApi(QJSEngine* engine, QObject* parent = nullptr);
    ~HeadlessScriptApi() override;

    Q_INVOKABLE void print(const QJSValue& value);
    Q_INVOKABLE bool writeJson(const QString& fileName, const QJSValue& value);
    Q_INVOKABLE bool validateScene(const QString& fileName);
    Q_INVOKABLE int runBenchmark(const QString& configFileName);
    Q_INVOKABLE QJSValue traceScene(const QJSValue& options);
    // No-export file-based async session: cancellation is explicit, and a
    // cancelled result never contains accepted scientific output fields.
    Q_INVOKABLE QJSValue startTrace(const QJSValue& options);
    Q_INVOKABLE QJSValue cancelTrace();
    Q_INVOKABLE QJSValue finishTrace();

    // Host end-of-evaluation cleanup: abort unfinished traces, join workers.
    // A script that omitted finishTrace is a failure, never accepted output.
    bool abandonTrace();

    bool hasErrors() const { return !m_errors.isEmpty(); }
    QStringList errors() const { return m_errors; }

private:
    void initializeSceneServices(const QString& fileName, CorePluginRegistry* plugins) const;
    void recordError(const QString& message);
    QString toDisplayString(const QJSValue& value) const;
    QString toJsonString(const QJSValue& value, QString* errorMessage) const;

    QJSEngine* m_engine = nullptr;
    QStringList m_errors;
    // A host can own at most one script-controlled trace at a time.
    // SimulationSession joins workers and releases scene/plugin pins on exit.
    std::unique_ptr<SimulationSession> m_pendingTrace;
    QString m_pendingSceneFile;
    ulong m_pendingRays = 0;
    ulong m_pendingSeed = 0;
};

class HeadlessScriptHost
{
public:
    int runScript(const QString& fileName) const;

private:
    void printScriptError(const QJSValue& error, const QString& fileName, const QString& source) const;
};
