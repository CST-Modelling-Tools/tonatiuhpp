#include <QApplication>
#include <QCoreApplication>
#include <QStyleFactory>
#include <QCommandLineParser>
#include <QSettings>
#include "CustomSplashScreen.h"

#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QMessageBox>
#include <QStringList>
#include <QTextStream>

#include <Inventor/Qt/SoQt.h>
#include "headless/HeadlessCommandRunner.h"
#include "MainWindow.h"

QTextStream cerr(stderr);

/*!
   \mainpage
   The Tonatiuh project aims to create an open source, cutting-edge, accurate, and easy to use Monte Carlo ray tracer for the optical simulation of solar concentrating systems. It intends to advance the state-of-the-art of the simulation tools available for the design and analysis of solar concentrating systems, and to make those tools freely available to anyone interested in using and improving them. Some of the most relevant design goals of Tonatiuh are:
   <ol>
   <li>To develop a robust theoretical foundation that will facilitate the optical simulation of almost any type of solar concentrating systems.</li>
   <li>To exhibit a clean and flexible software architecture, that will allow the user to adapt, expand, increase, and modify its functionalities with ease.</li>
   <li>To achieve operating system independence at source level, and run on all major platforms with none, or minor, modifications to its source code.</li>
   <li>To provide the users with an advanced and easy-of-use Graphic User Interface (GUI).</li>
   </ol>
 */

#include "script/NodeObject.h"
#include "script/DataObject.h"
//Q_SCRIPT_DECLARE_QMETAOBJECT(NodeObject, QObject*)
//Q_SCRIPT_DECLARE_QMETAOBJECT(DataObject, QObject*)
#include <QQmlEngine>

namespace
{
bool hasHeadlessArgument(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == "--headless")
            return true;
    }

    return false;
}

QString startupFileFromPositionalArguments(const QStringList& arguments, QString* warningMessage)
{
    QStringList startupFiles;
    for (const QString& argument : arguments) {
        const QString suffix = QFileInfo(argument).suffix();
        if (suffix.compare("tnhpp", Qt::CaseInsensitive) == 0 ||
            suffix.compare("tnhpps", Qt::CaseInsensitive) == 0)
            startupFiles.append(argument);
    }

    if (startupFiles.isEmpty())
        return QString();

    if (startupFiles.size() > 1 && warningMessage) {
        *warningMessage = QString(
            "Multiple Tonatiuh++ files were provided.\n"
            "Opening the first file and ignoring the rest:\n%1"
        ).arg(startupFiles.first());
    }

    return startupFiles.first();
}
}

int main(int argc, char** argv)
{  
    if (hasHeadlessArgument(argc, argv)) {
        QCoreApplication app(argc, argv);
        app.setApplicationName("Tonatiuh");
        app.setApplicationVersion(APP_VERSION);

        HeadlessCommandRunner runner;
        return runner.run(app.arguments());
    }

    // application
    QApplication app(argc, argv);
    app.setApplicationName("Tonatiuh");
    app.setApplicationVersion(APP_VERSION);

    //    QApplication::setColorSpec(QApplication::CustomColor);
    //    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    //    qDebug() << QStyleFactory::keys();
    //    app.setStyle(QStyleFactory::create("Fusion"));

    // parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Description");
    parser.addHelpOption(); // -h
    parser.addVersionOption(); // -v

    // options
    QCommandLineOption optionInput( // -i=input.xml
        "i", "File with input parameters", // option name and description
        "file", "" // value type and default
    );
    parser.addOption(optionInput);

    QCommandLineOption optionTest( // -t
        "t", "Test mode"
    );
    parser.addOption(optionTest);

    QCommandLineOption optionWindow( // -w
        "w", "Window mode"
    );
    parser.addOption(optionWindow);
    parser.addPositionalArgument(
        "project",
        "Tonatiuh++ project or script file to open.",
        "[file.tnhpp|file.tnhpps]"
    );

    // processing
    parser.process(app);
//    bool isTest = parser.isSet(optionTest);

    QSettings settings("Tonatiuh", "Cyprus");
    QString theme = settings.value("theme", "").toString();

    QString fileIcon;
    QString filePixmap;
    if (theme == "") {
        fileIcon = ":/images/about/Tonatiuh.ico";
        filePixmap = ":/images/about/SplashScreen.png";
    } else {
        fileIcon = ":/images/about/Tonatiuh.ico";
        filePixmap = ":/images/about/SplashScreen.png";
    }
    app.setWindowIcon(QIcon(fileIcon));

//    QString fileName = parser.positionalArguments()[0];
    QString fileName = parser.value(optionInput);
    QString startupWarning;
    bool fileFromPositionalArgument = false;
    if (fileName.isEmpty()) {
        fileName = startupFileFromPositionalArguments(parser.positionalArguments(), &startupWarning);
        fileFromPositionalArgument = !fileName.isEmpty();
    }

    QFileInfo fileInfo(fileName);
    const bool isScriptFile = fileInfo.suffix().compare("tnhpps", Qt::CaseInsensitive) == 0;
    const bool openScriptWindow = isScriptFile && (fileFromPositionalArgument || parser.isSet(optionWindow));
    if (!isScriptFile || openScriptWindow)
    {
        QPixmap pixmap(filePixmap);
//        pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
        CustomSplashScreen splash(pixmap);
        splash.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        splash.show();

        splash.setMessage("Launching Coin3D");
        SoQt::init((QWidget*) NULL);

        splash.setMessage("Creating window");

        MainWindow mw(fileName, &splash);
        mw.show();
        splash.setFinishWindow();
        if (!startupWarning.isEmpty())
            QMessageBox::warning(&mw, "Tonatiuh", startupWarning);
        if (isScriptFile)
            mw.openFileScript(fileName);

        int code = app.exec();
        return code;
    }
    else
    {
        SoQt::init((QWidget*) 0);

        QJSEngine* engine = new QJSEngine;
//        qScriptRegisterSequenceMetaType<QVector<QVariant>>(engine);

        MainWindow mw;
        QJSValue tonatiuh = engine->newQObject(&mw);
        engine->globalObject().setProperty("tonatiuh", tonatiuh);
        engine->globalObject().setProperty("tn", tonatiuh);

        NodeObject::setMainWindow(&mw);
        NodeObject::setEngine(engine);
        DataObject::setEngine(engine);
//        QJSValue nodeObjectClass = engine->scriptValueFromQMetaObject<NodeObject>();
        QJSValue nodeObjectClass = engine->newQMetaObject(&NodeObject::staticMetaObject);
        engine->globalObject().setProperty("NodeObject", nodeObjectClass);

        //https://doc.qt.io/qt-5/qjsengine.html???
//        QJSValue jsMetaObject = engine->newQMetaObject(&NodeObject::staticMetaObject);??
//        engine.globalObject().setProperty("MyObject", jsMetaObject);

//        QJSValue fileObjectClass = engine->scriptValueFromQMetaObject<DataObject>();
        QJSValue fileObjectClass = engine->newQMetaObject(&DataObject::staticMetaObject);
        engine->globalObject().setProperty("DataObject", fileObjectClass);

        QJSValue myExt = engine->newQObject(&mw);
        QQmlEngine::setObjectOwnership(myExt.toQObject(), QQmlEngine::CppOwnership); // important
        engine->globalObject().setProperty("print", myExt.property("print"));
        engine->globalObject().setProperty("printTimed", myExt.property("printTimed"));

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            QString text = QString("Cannot open file %1.\n%2")
                .arg(fileName)
                .arg(file.errorString());
            cerr << text << Qt::endl;
            return 1;
        }
        QTextStream in(&file);
        QString program = in.readAll();
        if (in.status() != QTextStream::Ok)
        {
            QString text = QString("Cannot read file %1.").arg(fileName);
            cerr << text << Qt::endl;
            return 1;
        }
        file.close();

//        QScriptSyntaxCheckResult check = engine->checkSyntax(program);
//        if (check.state() != QScriptSyntaxCheckResult::Valid)
//        {
//            QString text = QString("Syntax error in line %1.\n%2")
//                .arg(check.errorLineNumber())
//                .arg(check.errorMessage());
//            cerr << text << endl;
//            return -1;
//        }

        QJSValue result = engine->evaluate(program);
        if (result.isError())
        {
            QString text = QString("Runtime error.\nLine %1. %2")
                .arg(result.property("lineNumber").toNumber())
                .arg(result.toString());
            cerr << text << Qt::endl;
            return 1;
        }

        delete engine;
        return 0;
    }
}
