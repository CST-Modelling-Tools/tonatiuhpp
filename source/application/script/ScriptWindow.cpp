#include "ScriptWindow.h"
#include "ui_ScriptWindow.h"

#include <iostream>

#include <QDateTime>
#include <QElapsedTimer>
#include <QJSEngine>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTextStream>

Q_DECLARE_METATYPE(QVector<QVariant>)

#include "NodeObject.h"
#include "DataObject.h"
#include "main/MainWindow.h"
#include "kernel/scene/TSceneKit.h"
#include "main/PluginManager.h"
#include "SyntaxHighlighter.h"
#include "ScriptRayTracer.h"
#include "tonatiuh_script.h"
#include "AboutScriptDialog.h"

//Q_SCRIPT_DECLARE_QMETAOBJECT(NodeObject, QObject*)
//Q_SCRIPT_DECLARE_QMETAOBJECT(DataObject, QObject*)
#include <QQmlEngine>
#include <QDebug>

namespace {
QString appBundleDataPath(const QString& relativePath)
{
    QDir dir(QCoreApplication::applicationDirPath());
#ifdef Q_OS_MACOS
    return dir.absoluteFilePath("../Resources/" + relativePath);
#else
    return dir.absoluteFilePath("../" + relativePath);
#endif
}

QString existingDirectoryOrFallback(const QString& preferredPath, const QString& fallbackPath = QString())
{
    QFileInfo preferredInfo(preferredPath);
    if (preferredInfo.exists()) {
        if (preferredInfo.isDir()) return preferredInfo.absoluteFilePath();
        return preferredInfo.absolutePath();
    }

    QFileInfo fallbackInfo(fallbackPath);
    if (fallbackInfo.exists()) {
        if (fallbackInfo.isDir()) return fallbackInfo.absoluteFilePath();
        return fallbackInfo.absolutePath();
    }

    QString docsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!docsDir.isEmpty() && QDir(docsDir).exists())
        return docsDir;

    return QDir::homePath();
}

QString openFileWithDirectory(QWidget* parent,
                              const QString& title,
                              const QString& initialDirectory,
                              const QString& filter)
{
    QFileDialog dialog(parent, title);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(filter);
    dialog.setDirectory(existingDirectoryOrFallback(initialDirectory));
#ifdef Q_OS_MACOS
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
#endif
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
        return QString();
    return dialog.selectedFiles().first();
}

QString saveFileWithDirectory(QWidget* parent,
                              const QString& title,
                              const QString& initialDirectory,
                              const QString& filter)
{
    QFileDialog dialog(parent, title);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(filter);
    dialog.setDirectory(existingDirectoryOrFallback(initialDirectory));
#ifdef Q_OS_MACOS
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
#endif
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
        return QString();
    return dialog.selectedFiles().first();
}
}

QString timeString()
{
    return QString("[%1] ").arg(QDateTime::currentDateTime().toString("hh:mm:ss"));
}

ScriptWindow::ScriptWindow(MainWindow* mw, QWidget* parent):
    QMainWindow(parent),
    ui(new Ui::ScriptWindow)
{
    ui->setupUi(this);

    int q = fontMetrics().height();
    resize(64*q, 48*q);

    int h = height();
    ui->splitter->setSizes({ int(0.8*h), int(0.2*h) });

    ui->logWidget->setFont(ui->codeEditor->font());

    m_syntax = new SyntaxHighlighter(ui->codeEditor->document());

    // engine
    m_engine = new QJSEngine(this);
//    qScriptRegisterSequenceMetaType<QVector<QVariant>>(m_engine);

    // objects
    QJSValue tonatiuhObject = m_engine->newQObject(mw);
//    qDebug() << QQmlEngine::objectOwnership(tonatiuhObject.toQObject());
    QQmlEngine::setObjectOwnership(tonatiuhObject.toQObject(), QQmlEngine::CppOwnership); // important
    m_engine->globalObject().setProperty("tonatiuh", tonatiuhObject);
    m_engine->globalObject().setProperty("tn", tonatiuhObject);

    QJSValue consoleObject = m_engine->newQObject(ui->logWidget);
    m_engine->globalObject().setProperty("console", consoleObject);

    QObject* rayTracer = new ScriptRayTracer(mw->getPlugins()->getRandomFactories());
    QJSValue rayTracerObject = m_engine->newQObject(rayTracer);
    m_engine->globalObject().setProperty("rayTracer", rayTracerObject);

    NodeObject::setMainWindow(mw);
    NodeObject::setEngine(m_engine);
    QJSValue nodeObjectClass = m_engine->newQMetaObject(&NodeObject::staticMetaObject);
    m_engine->globalObject().setProperty("NodeObject", nodeObjectClass);

    DataObject::setEngine(m_engine);
    QJSValue fileObjectClass = m_engine->newQMetaObject(&DataObject::staticMetaObject);
    m_engine->globalObject().setProperty("DataObject", fileObjectClass);

    // functions
    //    QString pluginsDirectory = QApplication::applicationDirPath() + QDir::separator() + "plugins";
    //    QCoreApplication::addLibraryPath(pluginsDirectory);
    //    QCoreApplication::addLibraryPath(QApplication::applicationDirPath());
//    QJSValue importFunction = m_engine->newFunction(ScriptWindow::ImportExtension);
//    m_engine->globalObject().setProperty("Import", importFunction, QJSValue::ReadOnly);

    QJSValue myExt = m_engine->newQObject(this);
    QQmlEngine::setObjectOwnership(myExt.toQObject(), QQmlEngine::CppOwnership); // important


//    QJSValue printFunction = m_engine->newFunction(ScriptWindow::print);
    m_engine->globalObject().setProperty("print", myExt.property("print"));

//    QJSValue timeFunction = m_engine->newFunction(ScriptWindow::printTimed);
    m_engine->globalObject().setProperty("printTimed", myExt.property("printTimed"));

    connect(mw, SIGNAL(Abort(QString)), this, SLOT(abortScript(QString)) );

    connect(ui->actionFileNew, SIGNAL(triggered()), this, SLOT(fileNew()) );
    connect(ui->actionFileOpen, SIGNAL(triggered()), this, SLOT(fileOpen()) );
    connect(ui->actionFileReopen, SIGNAL(triggered()), this, SLOT(fileReopen()) );
    connect(ui->actionFileSave, SIGNAL(triggered()), this, SLOT(fileSave()) );
    connect(ui->actionFileSaveAs, SIGNAL(triggered()), this, SLOT(fileSaveAs()) );
    connect(ui->actionRunScript, SIGNAL(triggered()), this, SLOT(runScript()) );
}

ScriptWindow::~ScriptWindow()
{
    delete ui;
}

void ScriptWindow::runScript(QString fileName)
{
    if (fileName.isEmpty()) return;
    fileOpen(fileName);
    runScript();
    close();
}

bool ScriptWindow::isReady()
{
    if (!ui->codeEditor->document()->isModified())
        return true;

    QMessageBox::StandardButton answer = QMessageBox::warning(
        this, "Tonatiuh",
        "The document was modified.\n"
        "Do you want to save changes?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes
    );

    if (answer == QMessageBox::Yes)
        return fileSave();
    if (answer == QMessageBox::No)
        return true;
    return false;
}

void ScriptWindow::fileOpen(QString fileName)
{
    if (!isReady()) return;

    if (fileName.isEmpty())
    {
        QSettings settings("Tonatiuh", "Cyprus");
        QString dirName = settings.value("dirProjects", "").toString();

        fileName = openFileWithDirectory(
            this, "Open File", dirName,
            "Tonatiuh script file (*.tnhpps)"
        );
        if (fileName.isEmpty()) return;

        QFileInfo fileInfo(fileName);
        settings.setValue("dirProjects", fileInfo.path());
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(
            this, "Tonatiuh",
            tr("Cannot open file %1.").arg(fileName)
        );
        return;
    }

    QTextDocument* document = ui->codeEditor->document();
    document->clear();
    document->setPlainText(file.readAll());
    document->setModified(false);

    setTitle(fileName);
}

void ScriptWindow::fileReopen()
{
    fileOpen(m_fileName);
}

void ScriptWindow::fileNew()
{
    if (!isReady()) return;

    QTextDocument* document = ui->codeEditor->document();
    document->clear();
    document->setModified(false);

    setTitle("");
}

bool ScriptWindow::fileSaveAs(QString fileName)
{
    if (fileName.isEmpty())
    {
        QSettings settings("Tonatiuh", "Cyprus");
        QString dirName = settings.value("dirProjects", "").toString();

        fileName = saveFileWithDirectory(
            this, "Save File", dirName,
            "Tonatiuh script file (*.tnhpps)"
        );
        if (fileName.isEmpty()) return false;

        QFileInfo fileInfo(fileName);
        settings.setValue("dirProjects", fileInfo.path());
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(
            this, "Tonatiuh",
            tr("Cannot open file %1.").arg(fileName)
        );
        return false;
    }
    QTextStream out(&file); //? without stream

    QTextDocument* document = ui->codeEditor->document();
    out << document->toPlainText();
    document->setModified(false);

    setTitle(fileName);
    return true;
}

bool ScriptWindow::fileSave()
{
    return fileSaveAs(m_fileName);
}

// ScriptRayTracer is less general than MainWindow
// its functions are added by tonatiuh_script::init
// todo: why not to use as NodeObject?
// better make a class interface as NodeObject on top of ScriptRayTracer
// instead of global functions

void ScriptWindow::runScript()
{
    QStringList searchPaths = QDir::searchPaths("project");
    QFileInfo info(m_fileName);
    if (info.exists()) {
        QStringList temp;
        temp << info.absolutePath() << searchPaths;
        QDir::setSearchPaths("project", temp);
    }

    try {
//        int initialized = tonatiuh_script::init(m_engine);
//        if (!initialized)
//            throw timeString() + "Script engine could not start.";

        QJSValue rayTracerObject = m_engine->globalObject().property("rayTracer");
        ScriptRayTracer* rayTracer = (ScriptRayTracer*) rayTracerObject.toQObject();
        QFileInfo fileInfo(m_fileName);
        rayTracer->setDir(fileInfo.absoluteDir().absolutePath());

        QTextDocument* document = ui->codeEditor->document();
//        QString program = document->toPlainText();

//        QScriptSyntaxCheckResult check = m_engine->checkSyntax(program);
//        if (check.state() != QScriptSyntaxCheckResult::Valid)
//            throw timeString() + QString("Syntax error in line %1.\n%2")
//                .arg(check.errorLineNumber())
//                .arg(check.errorMessage());

        QElapsedTimer timer;
        timer.start();
        writeMessage(timeString() + "Script started.");

        QJSValue result = m_engine->evaluate(document->toPlainText());
        if (result.isError())
            throw timeString() + QString("Runtime error in line %1\n%2")
                .arg(result.property("lineNumber").toNumber())
                .arg(result.toString());

        QString msg = timeString() + QString("Script finished in %1 s.")
                .arg(timer.elapsed()/1000., 0, 'f', 3);
        writeMessage(msg);

        m_engine->collectGarbage();
    }
    catch (QString msg)
    {
        writeMessage(msg);
        std::cerr << msg.toStdString() << std::endl;
    }

    QDir::setSearchPaths("project", searchPaths);
}

void ScriptWindow::setTitle(QString fileName)
{
    m_fileName = fileName;

    QFileInfo fileInfo(fileName);
    QString title = "Untitled";
    if (!fileName.isEmpty())
        title = fileInfo.fileName();
    setWindowTitle(tr("%1[*] - Tonatiuh").arg(title));
}

void ScriptWindow::closeEvent(QCloseEvent* event)
{
    if (isReady())
        event->accept();
    else
        event->ignore();
}

void ScriptWindow::writeMessage(QString message)
{
    ui->logWidget->appendPlainText(message);
//    ui->logWidget->repaint();
    QCoreApplication::processEvents();
}

void ScriptWindow::abortScript(QString /*error*/)
{
//    QScriptContext* context = m_engine->currentContext();
//    context->throwError(error);
}

//QJSValue ScriptWindow::ImportExtension(QScriptContext* context, QJSEngine* engine)
//{
//    if (context->argumentCount() != 1) return false;
//    if (!context->argument(0).isString()) return false;
//    QString name = context->argument(0).toString();

//    if (!engine->importExtension(name).isUndefined())
//    {
//        QString msg = QString("Warning! %1 not found!").arg(name);

//        //    writeMessage(msg); //?
//        QJSValue object = engine->globalObject().property("console");
//        QPlainTextEdit* console = (QPlainTextEdit*) object.toQObject();
//        if (!console) return false;
//        console->appendPlainText(msg);
//        return false;
//    }

//    return true;
//}

QJSValue ScriptWindow::print(QString msg)
{
//    if (context->argumentCount() < 1) return false;
//    if (!context->argument(0).isString()) return false;
//    QString msg = context->argument(0).toString();

    QJSValue object = m_engine->globalObject().property("console");
    QPlainTextEdit* console = (QPlainTextEdit*) object.toQObject();
    if (!console) return false;
    console->appendPlainText(msg);
//    console->repaint();
//    repaint();
    QCoreApplication::processEvents();
    return true;
}


QJSValue ScriptWindow::printTimed(QString msg)
{
//    if (context->argumentCount() < 1) return false;
//    if (!context->argument(0).isString()) return false;
//    QString msg = timeString() + context->argument(0).toString();

    QJSValue object = m_engine->globalObject().property("console");
    QPlainTextEdit* console = (QPlainTextEdit*) object.toQObject();
    if (!console) return false;
    console->appendPlainText(timeString() + msg);
//    console->repaint();
//    repaint(); // force refresh
    QCoreApplication::processEvents();
    return true;
}

void ScriptWindow::on_actionExamples_triggered()
{
    if (!isReady()) return;

    QString fileName = openFileWithDirectory(
        this, "Open File", appBundleDataPath("examples/scripts"),
        "Tonatiuh script file (*.tnhpps)"
    );
    if (fileName.isEmpty()) return;

    fileOpen(fileName);
}

void ScriptWindow::on_actionAbout_triggered()
{
    AboutScriptDialog dialog;
    dialog.exec();
}
