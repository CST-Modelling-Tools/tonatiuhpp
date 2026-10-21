#pragma once

#include <functional>
#include <utility>

#include <QObject>
#include <QJSEngine>
#include <QString>

// One JS evaluation and script-local file-path service for all Tonatiuh++
// launchers. Scene capabilities are injected by the caller; this class does
// not require a MainWindow, a widget, or SoQt.
class ScriptHost final : public QObject
{
    Q_OBJECT

public:
    using PrintCallback = std::function<void(const QString&)>;

    explicit ScriptHost(QObject* parent = nullptr);

    QJSEngine* engine() { return &m_engine; }
    const QJSEngine* engine() const { return &m_engine; }

    // Each real source filename updates this host's script-directory context.
    // Synthetic names such as "<headless-bootstrap>" leave it unchanged.
    QJSValue evaluate(const QString& source, const QString& fileName = QString(),
                      int firstLine = 1);
    QString resolvePath(const QString& path) const;
    QString scriptDirectory() const { return m_scriptDirectory; }

    void setPrintCallback(PrintCallback callback) { m_print = std::move(callback); }

    static QString formatError(const QJSValue& error, const QString& sourceName);

    Q_INVOKABLE void print(const QJSValue& value);
    Q_INVOKABLE void printTimed(const QJSValue& value);

private:
    void output(const QJSValue& value, bool timed);

    QJSEngine m_engine;
    QString m_scriptDirectory;
    PrintCallback m_print;
};
