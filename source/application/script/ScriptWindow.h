#pragma once

#include <QMainWindow>
#include <QJSValue>

class FilesModel;
class QItemSelectionModel;
class QLineEdit;
class QScriptContext;
class QJSEngine;
class MainWindow;
class ScriptHost;
class SyntaxHighlighter;

namespace Ui {
class ScriptWindow;
}


class ScriptWindow: public QMainWindow
{
    Q_OBJECT

public:
    ScriptWindow(MainWindow* mw, QWidget* parent = 0);
    ~ScriptWindow();

    void runScript(QString fileName);
    bool isReady();

//    static QJSValue ImportExtension(QScriptContext* context, QJSEngine* engine);
    Q_INVOKABLE QJSValue print(QString text);
    Q_INVOKABLE QJSValue printTimed(QString text);

public slots:
    void fileOpen(QString fileName = QString());
    void fileReopen();
    void fileNew();
    bool fileSaveAs(QString fileName = QString());
    bool fileSave();
    void runScript();

protected:
    void setTitle(QString fileName);
    void closeEvent(QCloseEvent* event);

private slots:
    void writeMessage(QString message);
    void abortScript(QString error);

    void on_actionExamples_triggered();
    void on_actionAbout_triggered();

private:
    Ui::ScriptWindow* ui;

    QString m_fileName;
    ScriptHost* m_host = nullptr;
    QJSEngine* m_engine = nullptr; // Borrowed from m_host.
    SyntaxHighlighter* m_syntax;
};
