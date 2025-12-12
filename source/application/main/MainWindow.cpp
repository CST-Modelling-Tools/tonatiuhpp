#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <iostream>

#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QMutex>
#include <QPluginLoader>
#include <QProgressDialog>
#include <QSettings>
#include <QtConcurrentMap>
#include <QTime>
#include <QUndoStack>
#include <QUndoView>
#include "CustomSplashScreen.h"
#include <QElapsedTimer>
#include <QPushButton>
#include <QDebug>
#include <QShortcut>

#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/draggers/SoDragger.h>
#include <Inventor/manips/SoCenterballManip.h>
#include <Inventor/manips/SoHandleBoxManip.h>
#include <Inventor/manips/SoJackManip.h>
#include <Inventor/manips/SoTabBoxManip.h>
#include <Inventor/manips/SoTrackballManip.h>
#include <Inventor/manips/SoTransformBoxManip.h>
#include <Inventor/manips/SoTransformerManip.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodekits/SoSceneKit.h>

#include "libraries/math/gcf.h"
#include "libraries/math/3D/Ray.h"
#include "libraries/math/3D/Transform.h"

#include "calculator/SunCalculatorDialog.h"
#include "commands/ActionInsert.h"
#include "commands/CmdRename.h"
#include "commands/CmdCopy.h"
#include "commands/CmdCut.h"
#include "commands/CmdDelete.h"
#include "commands/CmdInsertNode.h"
#include "commands/CmdSetFieldNode.h"
#include "commands/CmdSetFieldText.h"
#include "commands/CmdPaste.h"

#include "PluginManager.h"
#include "kernel/node/TonatiuhFunctions.h"
#include "kernel/air/AirVacuum.h"
#include "kernel/air/AirKit.h"
#include "kernel/component/ComponentFactory.h"
#include "kernel/material/MaterialRT.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/photons/PhotonsAbstract.h"
#include "kernel/photons/PhotonsSettings.h"
#include "kernel/random/Random.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/run/RayTracer.h"
#include "kernel/profiles/ProfileRT.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/scene/TArrayKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunShape.h"
#include "kernel/sun/SunPosition.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunKit.h"
#include "kernel/trackers/TrackerKit.h"
#include "kernel/trackers/TrackerArmature.h"
#include "view/trf.h"
#include "main/Document.h"
#include "run/FluxAnalysis.h"
#include "run/FluxAnalysisDialog.h"
#include "run/RayTracingDialog.h"
#include "script/ScriptWindow.h"
#include "tree/SceneTreeModel.h"
#include "view/GraphicRoot.h"
#include "view/GraphicView.h"
#include "widgets/AboutDialog.h"
#include "widgets/AirDialog.h"
#include "widgets/NetworkConnectionsDialog.h"
#include "widgets/SunDialog.h"
#include "UndoView.h"
#include "kernel/scene/GridNode.h"

void startManipulator(void* data, SoDragger* dragger)
{
    MainWindow* w = static_cast<MainWindow*>(data);
    w->StartManipulation(dragger);
}

void finishManipulator(void* data, SoDragger* /*dragger*/)
{
    MainWindow* w = static_cast<MainWindow*>(data);
    w->FinishManipulation();
}

void setSearchPaths(const QString& fileName)
{
    QStringList searchPaths;

    QFileInfo info(fileName);
    if (info.exists())
        searchPaths << info.absolutePath();

    searchPaths << QDir::currentPath();
    searchPaths << QCoreApplication::applicationDirPath();

    QDir::setSearchPaths("project", searchPaths);
}

MainWindow::MainWindow(QString fileName, CustomSplashScreen* splash, QWidget* parent, Qt::WindowFlags flags):
    QMainWindow(parent, flags),
    ui(new Ui::MainWindow),
    m_document(0),
    m_graphicsRoot(0),
    m_modelScene(0),
    m_modelSelection(0),
    m_lastExportFileName(""),
    m_lastExportSurfaceUrl(""),
    m_lastExportInGlobal(true),
    m_clipboardNodes(0),

    m_raysNumber(10'000),
    m_raysScreen(1'000),
    m_raysRandomFactoryIndex(0),
    m_raysGridWidth(200),
    m_raysGridHeight(200),

    m_raysTracedTotal(0),
    m_rand(0),

    m_photonsBuffer(0),
    m_photonBufferSize(1'000'000),
    m_photonBufferAppend(false),
    m_photonsSettings(0),

    m_graphicView(0),
    m_focusView(0)
{
    ui->setupUi(this);

    if (splash) splash->setMessage("Loading plugins");
    m_pluginManager = new PluginManager;
    QDir dir(qApp->applicationDirPath());
    dir.cd("plugins");
    m_pluginManager->load(dir);

    QDir dirApp(QCoreApplication::applicationDirPath());
    QStringList paths = {dirApp.absoluteFilePath("../resources")};
    QDir::setSearchPaths("resources", paths);

    if (splash) splash->setMessage("Creating views");
    SetupDocument();
    SetupViews();
    SetupTriggers();
    readSettings();

    if (splash) splash->setMessage("Opening file");
    QFileInfo fileInfo(fileName);
    if (!fileName.isEmpty() && fileInfo.completeSuffix() != "tnhpps") {
        openFileProject(fileName);
    } else {
        setSearchPaths(""); // refactor
        SetCurrentFile("");
        m_graphicView[0]->onViewHome();
    }
    ui->parametersTabs->setNode(0);

//    Select("//Node");
//    m_graphicsRoot->deselectAll();

//    qDebug() << "counter " << m_document->getSceneKit()->getRefCount();
//    qDebug() << "layout " << m_modelScene->getInstance(m_modelScene->indexFromUrl("//Layout"))->getNode()->getRefCount();

//    ui->toolbarFile->hide();
    ui->toolbarEdit->hide();

    ui->centralWidget->setFocus();

    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_pluginManager;
    delete m_modelScene;
    delete m_document;
    delete m_rand;
    delete m_photonsBuffer;
}

TSceneKit* MainWindow::getSceneKit()
{
    return m_document->getSceneKit();
}

void MainWindow::openFileScript(QString fileName)
{
    // refactor
    QStringList searchPaths = QDir::searchPaths("project");

    QFileInfo info(fileName);
    if (info.exists()) {
        searchPaths << info.absolutePath();
//        searchPaths << QDir::currentPath();
//        searchPaths << QCoreApplication::applicationDirPath();
    }
    QDir::setSearchPaths("project", searchPaths);

    ScriptWindow* window = new ScriptWindow(this, 0);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
    window->fileOpen(fileName);
}

void MainWindow::SetupDocument()
{
    m_document = new Document;
    connect(
        m_document, SIGNAL(Warning(QString)),
        this, SLOT(showWarning(QString))
    );

    // graphic root
    m_graphicsRoot = new GraphicRoot;
    m_graphicsRoot->setDocument(m_document);

    connect(
        m_graphicsRoot, SIGNAL(selectionChanged(SoSelection*)),
        this, SLOT(SelectionFinish(SoSelection*))
    );

    // models
    m_modelScene = new SceneTreeModel;
    m_modelScene->setDocument(m_document);

    m_modelSelection = new QItemSelectionModel(m_modelScene);
}

void MainWindow::SetupViews()
{
    // undo
    m_undoStack = new QUndoStack(this);
    m_undoStack->setUndoLimit(10);

    connect(m_undoStack, SIGNAL(canRedoChanged(bool)),
            ui->actionEditRedo, SLOT(setEnabled(bool)) );
    connect(m_undoStack, SIGNAL(canUndoChanged(bool)),
            ui->actionEditUndo, SLOT(setEnabled(bool)) );
    connect(m_undoStack, SIGNAL(indexChanged(int)),
            this, SLOT(onUndoStack()));

    m_undoView = new UndoView(m_undoStack, this);

    // graphic
    SetupGraphicView();

    // tree
    SetupTreeView();
}

void MainWindow::SetupGraphicView()
{
    m_graphicView << ui->widgetView3D;
    m_graphicView[0]->setSceneGraph(m_graphicsRoot);
//    m_graphicView[0]->m_window = this;

//    QList<int> sizes = {500, 200};
//    int q = 50*fontMetrics().xHeight();
//    QList<int> sizes = {width() - q, q};
//    ui->splitterH->setSizes(sizes);

//    ui->splitterH->setStretchFactor(0, 1);

    connect(
        m_modelSelection, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
        m_graphicView[0], SLOT(currentChanged(QModelIndex,QModelIndex))
    );

    m_focusView = 0;

//    delete splitter->widget(0);
//    splitter->insertWidget(0, m_graphicView[0]);

//    int q = 100*fontMetrics().xHeight();
//    QList<int> sizes = {ui->splitterH->width() - q, q};
//    ui->splitterH->setSizes(sizes);

/*
    QSplitter* splitter = ui->splitterH;

    QSplitter* splitterV = new QSplitter();
    splitterV->setObjectName("graphicSplitterV");
    splitterV->setOrientation(Qt::Vertical);

//    delete splitter->replaceWidget(0, splitterV);
    delete splitter->widget(0);
    splitter->insertWidget(0, splitterV);

    QList<int> sizes;
    sizes << 700 << 200;
    splitter->setSizes(sizes);

    QSplitter* splitterH1 = new QSplitter(splitterV);
    splitterH1->setObjectName("graphicSplitterH1");
    splitterH1->setOrientation(Qt::Horizontal);

    QSplitter* splitterH2 = new QSplitter(splitterV);
    splitterH2->setObjectName("graphicSplitterH2");
    splitterH2->setOrientation(Qt::Horizontal);

    m_graphicView << new GraphicView(splitterH1);
    m_graphicView << new GraphicView(splitterH1);
    m_graphicView << new GraphicView(splitterH2);
    m_graphicView << new GraphicView(splitterH2);

    for (int n = 0; n < 4; n++){
        m_graphicView[n]->setSceneGraph(m_graphicsRoot);
        m_graphicView[n]->setModel(m_modelScene);
        m_graphicView[n]->setSelectionModel(m_modelSelection);
        if (n > 0) m_graphicView[n]->showDecoration(false);
    }

    m_focusView = 1;
    on_actionViewY_triggered();

    m_focusView = 2;
    on_actionViewX_triggered();

    m_focusView = 3;
    on_actionLookWest_triggered();

    // 0 splitterH1 1              tree
    //   splitterV      splitter
    // 2 spliter H2 3              parameters
    on_actionQuadView_toggled();*/

}

void MainWindow::SetupTreeView()
{
    // tree
    ui->sceneView->setModel(m_modelScene);
    ui->sceneView->setSelectionModel(m_modelSelection);
//    ui->sceneView->setRootIndex(m_modelScene->indexFromUrl("//Layout")); // also in ChangeModelScene
//    ui->sceneView->setRootIsDecorated(false);

    connect(
        ui->sceneView, SIGNAL(nodeRenamed(const QModelIndex&,const QString&)),
        this, SLOT(ChangeNodeName(const QModelIndex&,const QString&))
    );
    connect(
        ui->sceneView, SIGNAL(dragAndDrop(const QModelIndex&,const QModelIndex&, bool)),
        this, SLOT(onNodeDragAndDrop(const QModelIndex&,const QModelIndex&, bool))
    );
    connect(
        ui->sceneView, SIGNAL(showMenu(const QModelIndex&)),
        this, SLOT(ShowMenu(const QModelIndex&))
    );

    // tree world
    ui->treeWorldWidget->setCurrentItem(ui->treeWorldWidget->invisibleRootItem()->child(1)); // select sun
    connect(
        ui->treeWorldWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
        this, SLOT(treeWorldClicked(QTreeWidgetItem*, int))
    );
    connect(
        ui->treeWorldWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
        this, SLOT(treeWorldDoubleClicked(QTreeWidgetItem*, int))
    );
    connect(
        ui->tabWidget, SIGNAL(currentChanged(int)),
        this, SLOT(tabChanged(int))
    );

    // parameters
    ui->parametersTabs->removeTab(1);
    ui->parametersTabs->getView()->getModel()->setMain(this);

    connect(
        ui->parametersTabs->getView()->getModel(), SIGNAL(fieldTextModified(SoNode*, QString, QString)),
        this, SLOT(setFieldText(SoNode*, QString, QString))
    );
    connect(
        ui->parametersTabs->getView()->getModel(), SIGNAL(fieldNodeModified(SoNode*, QString, SoNode*)),
        this, SLOT(setFieldNode(SoNode*, QString, SoNode*))
    );

    connect(
        m_modelSelection, SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)),
        this, SLOT(ChangeSelection(const QModelIndex&))
    );
}

/*!
 * Defines slots function for main window signals.
 */
void MainWindow::SetupTriggers()
{
    QMenu* menu = ui->menuComponent;
    for (ComponentFactory* f : m_pluginManager->getComponentFactories())
    {
        ActionInsert* a = new ActionInsert(f, this);
        menu->addAction(a);
        connect(
            a, SIGNAL(InsertComponent(ComponentFactory*)),
            this, SLOT(CreateComponent(ComponentFactory*))
        );
    }

    connect(this, SIGNAL(Abort(QString)), this, SLOT(onAbort(QString)));

    // file
    connect(ui->actionFileNew, SIGNAL(triggered()), this, SLOT(fileNew()) );
    connect(ui->actionFileOpen, SIGNAL(triggered()), this, SLOT(fileOpen()) );
    connect(ui->actionFileReopen, SIGNAL(triggered()), this, SLOT(fileReopen()) );
    connect(ui->actionFileSave, SIGNAL(triggered()), this, SLOT(fileSave()) );
    connect(ui->actionFileSaveAs, SIGNAL(triggered()), this, SLOT(fileSaveAs()) );
    connect(ui->actionFileExit, SIGNAL(triggered()), this, SLOT(close()) );

//    ui->menuFileRecent->menuAction()->setShortcut(QKeySequence("Ctrl+Shift+O"));
//    connect(ui->menuFileRecent->menuAction(), SIGNAL(triggered()), this, SLOT(showOpenRecent()));
    QShortcut* fileshortcut = new QShortcut(QKeySequence("Ctrl+Shift+O"), this);
    connect(fileshortcut, SIGNAL(activated()), this, SLOT(showOpenRecent()));

    // edit
    connect(ui->actionEditUndo, SIGNAL(triggered()), m_undoStack, SLOT(undo()) );
    connect(ui->actionEditRedo, SIGNAL(triggered()), m_undoStack, SLOT(redo()) );
    connect(ui->actionEditUndoView, SIGNAL(triggered()), m_undoView, SLOT(show()) );
    connect(ui->actionEditCut, SIGNAL(triggered()), this, SLOT(Cut()) );
    connect(ui->actionEditCopy, SIGNAL(triggered()), this, SLOT(Copy()) );
    connect(ui->actionEditPaste, SIGNAL(triggered()), this, SLOT(PasteCopy()) );
    connect(ui->actionEditPasteLink, SIGNAL(triggered()), this, SLOT(PasteLink()) );
    connect(ui->actionEditDelete, SIGNAL(triggered()), this, SLOT(Delete()) );

    // run
    connect(ui->actionRun, SIGNAL(triggered()), this, SLOT(RunCompleteRayTracer()) );
    connect(ui->actionRunFlux, SIGNAL(triggered()), this, SLOT(RunFluxAnalysisDialog()) );

    // layout
    connect(ui->buttonInsertNode, SIGNAL(pressed()), this, SLOT(InsertNode()) );
    connect(ui->buttonInsertShape, SIGNAL(pressed()), this, SLOT(InsertShape()) );
    connect(ui->buttonInsertTracker, SIGNAL(pressed()), this, SLOT(InsertTracker()) );
    connect(ui->buttonInsertArray, SIGNAL(pressed()), this, SLOT(InsertArray()) );
    connect(ui->buttonNodeExport, SIGNAL(pressed()), this, SLOT(nodeExport()) );
    connect(ui->buttonNodeImport, SIGNAL(pressed()), this, SLOT(nodeImport()) );
}

/*!
 * Finish the manipulation of the current selected node.
 */
void MainWindow::FinishManipulation()
{
    QModelIndex currentIndex = ui->sceneView->currentIndex();
    SoBaseKit* coinNode = static_cast<SoBaseKit*>(m_modelScene->getInstance(currentIndex)->getNode() );

    SoTransform* nodeTransform = static_cast<SoTransform*>(coinNode->getPart("transform", true) );

    QUndoCommand* command = new QUndoCommand();

    QString translationValue = QString("%1 %2 %3").arg(
        QString::number(nodeTransform->translation.getValue()[0]),
        QString::number(nodeTransform->translation.getValue()[1]),
        QString::number(nodeTransform->translation.getValue()[2])
    );
    new CmdSetFieldText(nodeTransform, "translation", translationValue, command);
    m_undoStack->push(command);

    UpdateLightSize();
    setDocumentModified(true);
}

//void MainWindow::ExecuteScriptFile(QString fileName)
//{
//    ScriptEditorDialog dialog(m_pluginManager->getRandomFactories(), this);
//    dialog.show();
//    dialog.ExecuteScript(fileName);
//    dialog.done(0);
//}

void MainWindow::onAbort(QString error)
{
    showInStatusBar(error);
}

void MainWindow::showWarning(QString message)
{
    QMessageBox::warning(this, "Tonatiuh", message);
}

void MainWindow::showInStatusBar(QString message, int time)
{
    statusBar()->showMessage(message, time);
}

void MainWindow::onUndoStack()
{
    ChangeSelection(m_modelSelection->currentIndex());
    //    UpdateLightSize();
}

/*!
 * Starts manipulating current selected node with \a dragger.
 */
void MainWindow::StartManipulation(SoDragger* dragger)
{
    SoSearchAction coinSearch;
    coinSearch.setNode(dragger);
    coinSearch.setInterest(SoSearchAction::FIRST);

    coinSearch.apply(m_graphicsRoot->getRoot() );

    SoPath* coinScenePath = coinSearch.getPath();
    if (!coinScenePath) gcf::SevereError("PathFromIndex Null coinScenePath.");

    SoNodeKitPath* nodePath = static_cast<SoNodeKitPath*> (coinScenePath);
    if (!nodePath) gcf::SevereError("PathFromIndex Null nodePath.");


    nodePath->truncate(nodePath->getLength() - 1);
    SoBaseKit* coinNode = static_cast<SoBaseKit*> (nodePath->getTail() );

    QModelIndex nodeIndex = m_modelScene->indexFromPath(*nodePath);
    m_modelSelection->setCurrentIndex(nodeIndex, QItemSelectionModel::ClearAndSelect);

    SoNode* manipulator = coinNode->getPart("transform", true);
    QStringList m_manipulators_Buffer;

    SoFieldList fieldList;
    int totalFields = manipulator->getFields(fieldList);

    SoField* pField = 0;
    SbString fieldValue = "null";

    for (int index = 0; index < totalFields; ++index)
    {
        pField = fieldList.get(index);
        if (pField)
        {
            pField->get(fieldValue);
            m_manipulators_Buffer << fieldValue.getString();
        }
    }
}

void MainWindow::onSunDialog()
{
    SunDialog dialog(m_modelScene, this);
    if (!dialog.exec()) return;
    setFieldNode(m_document->getSceneKit()->getPart("world", false), "sun", dialog.getSun());
}

void MainWindow::onAirDialog()
{
    AirDialog dialog(m_document->getSceneKit(), this);
    if (!dialog.exec()) return;
    setFieldNode(m_document->getSceneKit()->getPart("world", false), "air", dialog.getAir());
}

/*!
 * Moves the scene node with index \a node to the parent with index \a newParent.
 */
void MainWindow::onNodeDragAndDrop(const QModelIndex& indexParent, const QModelIndex& index, bool isMove)
{
    InstanceNode* instance = m_modelScene->getInstance(index);
    SoNode* node = instance->getNode();

    QUndoCommand* cmd = new QUndoCommand;
    if (isMove)
        cmd->setText("Drag and Drop: move node");
    else
        cmd->setText("Drag and Drop: copy node");

    new CmdCut(index, m_clipboardNodes, m_modelScene, cmd);
    new CmdPaste(!isMove, indexParent, node, m_modelScene, cmd);

    m_undoStack->push(cmd);

    UpdateLightSize();
    setDocumentModified(true);
}

void MainWindow::fileOpen()
{
    if (!OkToContinue()) return;

    // HKEY_CURRENT_USER\Software\Tonatiuh\Cyprus
    QSettings settings("Tonatiuh", "Cyprus");
    QString dirName = settings.value("dirProjects", "").toString();

    QString fileName = QFileDialog::getOpenFileName(
        this, "Open File", dirName,
        "All files (*);;"
        "Tonatiuh++ projects (*.tnhpp);;"
        "Tonatiuh++ scripts (*.tnhpps)"
    );
    //"Tonatiuh++ files (*.tnhpp *.tnhpps);;"
    if (fileName.isEmpty()) return;

    QFileInfo info(fileName);
    settings.setValue("dirProjects", info.path());

    fileOpen(fileName);
}

void MainWindow::on_actionHelpExamples_triggered()
{
//    if (!OkToContinue()) return;

//    QDir dir = QCoreApplication::applicationDirPath();
//    QString fileName = QFileDialog::getOpenFileName(
//        this, "Open File", dir.filePath("../examples"),
//        "Tonatiuh files (*.tnhpp);; All files (*)"
//    );
//    if (fileName.isEmpty()) return;

//    openFileProject(fileName);

    if (!OkToContinue()) return;

    QDir dir = QCoreApplication::applicationDirPath();
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open File", dir.filePath("../examples"),
        "All files (*);;"
        "Tonatiuh++ projects (*.tnhpp);;"
        "Tonatiuh++ scripts (*.tnhpps)"
    );
//    "Tonatiuh++ files (*.tnhpp *.tnhpps);;"
    if (fileName.isEmpty()) return;

    setFocus();
    fileOpen(fileName);
}

void MainWindow::on_actionHelpScripts_triggered()
{
    QDir dir = QCoreApplication::applicationDirPath();
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open File", dir.filePath("../examples/scripts"),
        "Tonatiuh script files (*.tnhpps);; All files (*)"
    );
    if (fileName.isEmpty()) return;

//    QFileDialog dialog(this);
//    dialog.setAcceptMode(QFileDialog::AcceptOpen);
//    dialog.setWindowTitle("Open File");
//    dialog.setDirectory(dir.filePath("../examples/scripts"));
//    dialog.setNameFilter("Tonatiuh script files (*.tnhs)");
//    dialog.setOption(QFileDialog::DontUseNativeDialog);
//    dialog.exec();

//    if (dialog.selectedFiles().isEmpty()) return;
//    QString fileName = dialog.selectedFiles().first();

    openFileScript(fileName);
}

void MainWindow::fileOpenRecent()
{
    if (!OkToContinue()) return;

    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;
    QString fileName = action->data().toString();

    openFileProject(fileName);
}

void MainWindow::showOpenRecent()
{
//    ui->menuFile->show();
//    ui->menuFileRecent->popup(QPoint(0, 0));
//    ui->menuFileRecent->show();

//    ui->menubar->setActiveAction(ui->menuFile->menuAction());
//    ui->menuFile->setActiveAction(ui->menuFileRecent->menuAction());
//    QPoint p = ui->menuFileRecent->pos();
//    ui->menuFileRecent->hide();
//    ui->menuFile->setActiveAction(ui->actionFileNew);
//    ui->menuFile->setActiveAction(ui->actionFileOpen);

    ui->menuFileRecent->popup(mapToGlobal(ui->menubar->geometry().bottomLeft()));
//    ui->menubar->setActiveAction(ui->menuFileRecent->menuAction());

//    ui->menubar->setActiveAction(ui->menuFileRecent->menuAction());
}


void MainWindow::fileReopen()
{
    if (!OkToContinue()) return;
    openFileProject(m_fileName);
}

/*!
 * Runs complete ray tracer. First defines export settings if they are not defined and then runs ray tracer.
 */
void MainWindow::RunCompleteRayTracer()
{
    InstanceNode* instanceRoot = 0;
    InstanceNode instanceSun(0);
    AirTransmission* air = 0;

//    QElapsedTimer timer;
//    timer.start();

    if (!ReadyForRaytracing(instanceRoot, &instanceSun, air) ) // ? 2 times
        return;

//    std::cout << "Elapsed time (ReadyForRaytracing): " << timer.elapsed() << std::endl;


    QVector<PhotonsFactory*> exportFactories = m_pluginManager->getExportFactories();
    QVector<RandomFactory*> randomFactories = m_pluginManager->getRandomFactories();

    RayTracingDialog dialog;
    dialog.setParameters(m_raysNumber, m_raysScreen,
                         randomFactories, m_raysRandomFactoryIndex,
                         m_raysGridWidth, m_raysGridHeight,
                         m_photonBufferSize, m_photonBufferAppend);
    dialog.setPhotonSettings(m_modelScene, exportFactories, m_photonsSettings);
    if (!dialog.exec()) return;

    SetRaysNumber(dialog.raysNumber());
    SetRaysScreen(dialog.raysScreen());
    SetRaysRandomFactory(randomFactories[dialog.raysRandomFactory()]->name());
    SetRaysGrid(dialog.raysGridWidth(), dialog.raysGridHeight());
    SetPhotonBufferSize(dialog.photonBufferSize());
    SetPhotonBufferAppend(dialog.photonBufferAppend());

    if (!m_photonsBuffer->getExporter()) {
        if (m_photonsSettings) delete m_photonsSettings;
        m_photonsSettings = new PhotonsSettings;
        *m_photonsSettings = dialog.getPhotonSettings();
    }

    Run();
}

/*!
 * Runs the ray tracer for the analysis of the surface. A dialog will be opened to set the surface and flux calculation parameters.
 */
void MainWindow::RunFluxAnalysisDialog()
{
    TSceneKit* sceneKit = m_document->getSceneKit();
    if (!sceneKit) return;

    Random* rand = m_pluginManager->getRandomFactories()[m_raysRandomFactoryIndex]->create(0);

    FluxAnalysisDialog dialog(sceneKit, m_modelScene, m_raysGridWidth, m_raysGridHeight, rand, this);
    dialog.exec();
}

/*!
 * Returns \a true if the tonatiuh model is correctly saved in the current file. Otherwise, returns \a false.
 *
 * If a current file is not defined, it calls to SaveAs funtios.
 *
 * \sa SaveAs, SaveFile.
 */
bool MainWindow::fileSave()
{
    if (m_fileName.isEmpty())
        return fileSaveAs();
    else
        return fileSave(m_fileName);
}

/*!
 * Returns \a true if the tonatiuh model is correctly saved. Otherwise, returns \a false. A file dialog is created to select a file.
 *
 * \sa Save, SaveFile.
 */
bool MainWindow::fileSaveAs()
{
    QSettings settings("Tonatiuh", "Cyprus");
    QString dirName = settings.value("dirProjects", "").toString();

    QString fileName = QFileDialog::getSaveFileName(
        this, "Save", dirName,
        "Tonatiuh files (*.tnhpp);;Tonatiuh debug (*.tnhd)"
    );
    if (fileName.isEmpty()) return false;

    QFileInfo info(fileName);
    settings.setValue("dirProjects", info.path());

    return fileSave(fileName);
}

void MainWindow::nodeExport(QString fileName)
{
    if (!m_modelSelection->hasSelection()) return;

    QModelIndex index = ui->sceneView->currentIndex();
    SoNode* node = m_modelScene->getInstance(index)->getNode();
    if (!node->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId()))
    {
        showWarning("Exported node shoud be TSeparatorKit");
        return;
    }
    TSeparatorKit* kit = dynamic_cast< TSeparatorKit*>(node);
    if (!kit) return;

    // file
    if (fileName.isEmpty()) {
        QSettings settings("Tonatiuh", "Cyprus");
        QString dirName = settings.value("dirProjects", "").toString();

        fileName = QFileDialog::getSaveFileName(
            this, "Export Node", dirName,
            "Tonatiuh component (*.tcmp)"
        );
        if (fileName.isEmpty()) return;

        QFileInfo info(fileName);
        settings.setValue("dirProjects", info.path());
    }

    // do
    SoWriteAction action;
    if (!action.getOutput()->openFile(fileName.toLatin1().constData()))
    {
        showWarning(tr("Cannot open file %1.").arg(fileName));
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    action.getOutput()->setBinary(false);
    action.apply(kit);
    action.getOutput()->closeFile();
    QApplication::restoreOverrideCursor();
}

void MainWindow::nodeImport(QString fileName)
{
    QModelIndex index = ui->sceneView->currentIndex();
    if (!index.isValid())
        index = ui->sceneView->rootIndex();

    SoNode* node = m_modelScene->getInstance(index)->getNode();
    if (!node->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId()))
    {
        emit Abort(tr("Parent node should be TSeparatorKit"));
        return;
    }

    // file
    if (fileName.isEmpty()) {
        QSettings settings("Tonatiuh", "Cyprus");
        QString dirName = settings.value("dirProjects", "").toString();

        fileName = QFileDialog::getOpenFileName(
            this, "Import Node", dirName,
            "Tonatiuh component (*.tcmp)"
        );
        if (fileName.isEmpty()) return;

        QFileInfo info(fileName);
        settings.setValue("dirProjects", info.path());
    }

    // do
    SoInput input;
    if (!input.openFile(fileName.toLatin1().constData()))
    {
        emit Abort(tr("Cannot open file:\n%1.").arg(fileName));
        return;
    }
    SoSeparator* separator = SoDB::readAll(&input);
    input.closeFile();

    if (!separator)
    {
        emit Abort(tr("Error reading file: \n%1.").arg(fileName));
        return;
    }
    SoNode* nodeImported = separator->getChild(0);

    CmdInsertNode* cmd = new CmdInsertNode(nodeImported, index);
    cmd->setText("Insert Node from file");
    m_undoStack->push(cmd);

    UpdateLightSize();
    setDocumentModified(true);
}

/*!
 * Changes the selected elements to the nodes into \a selection.
 */
void MainWindow::SelectionFinish(SoSelection* selection)
{
    if (selection->getNumSelected() == 0) {
        m_graphicsRoot->deselectAll();
        m_modelSelection->clearSelection();
        m_graphicView[0]->render();
        return;
    }
    SoPath* path = selection->getPath(0);
    if (!path) return;
    if (!path->containsNode(m_document->getSceneKit())) return;

    SoNodeKitPath* nodeKitPath = static_cast<SoNodeKitPath*>(path);
//    if (nodeKitPath->getTail()->getTypeId().isDerivedFrom(SoDragger::getClassTypeId() ) )
//        return;

    QModelIndex index = m_modelScene->indexFromPath(*nodeKitPath);
    if (!index.isValid()) return;
    m_modelSelection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
//    m_selectionModel->select(index, QItemSelectionModel::ClearAndSelect);

    SoPath* pathCorrected = m_modelScene->pathFromIndex(index);
    m_graphicsRoot->deselectAll();
    m_graphicsRoot->select(pathCorrected);
    m_graphicView[0]->render();
}

void MainWindow::setFieldText(SoNode* node, QString field, QString value)
{
    CmdSetFieldText* cmd = new CmdSetFieldText(node, field, value);
    m_undoStack->push(cmd);
    setDocumentModified(true);
}

void MainWindow::setFieldNode(SoNode* node, QString field, SoNode* value)
{
    CmdSetFieldNode* cmd = new CmdSetFieldNode(node, field, value);
    m_undoStack->push(cmd);
    setDocumentModified(true);
}

/*!
 * Shows selected node right menu.
 */
void MainWindow::ShowMenu(const QModelIndex& index)
{
    if (!index.isValid()) return;
    m_modelSelection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);

    InstanceNode* instanceNode = m_modelScene->getInstance(index);
    SoNode* coinNode = instanceNode->getNode();
    SoType type = coinNode->getTypeId();

    QMenu popupmenu(this);

    if (instanceNode->getParent() && instanceNode->getParent()->getNode()->getTypeId().isDerivedFrom(SoBaseKit::getClassTypeId() ) )
    {
        popupmenu.addAction(ui->actionEditCut);
        popupmenu.addAction(ui->actionEditCopy);
        popupmenu.addAction(ui->actionEditPaste);
        popupmenu.addAction(ui->actionEditPasteLink);
        popupmenu.addAction(ui->actionEditDelete);
        popupmenu.addAction("Collapse all", ui->sceneView, SLOT(collapseAll()));
    }

    if (type.isDerivedFrom(TSeparatorKit::getClassTypeId() ) )
    {
        QMenu* transformMenu = popupmenu.addMenu("Convert to");
        TSeparatorKit* coinKit = dynamic_cast<TSeparatorKit*> (coinNode);
        SoTransform* transform = static_cast<SoTransform*>(coinKit->getPart("transform", true));
        SoType transformType = transform->getTypeId();

        if (!transformType.isDerivedFrom(SoCenterballManip::getClassTypeId()) )
            transformMenu->addAction("SoCenterballManip", this, SLOT(SoTransform_to_SoCenterballManip()));
        if (!transformType.isDerivedFrom(SoHandleBoxManip::getClassTypeId()) )
            transformMenu->addAction("SoHandleBoxManip", this, SLOT(SoTransform_to_SoHandleBoxManip()));
        if (!transformType.isDerivedFrom(SoJackManip::getClassTypeId()) )
            transformMenu->addAction("SoJackManip", this, SLOT(SoTransform_to_SoJackManip()));
        if (!transformType.isDerivedFrom(SoTabBoxManip::getClassTypeId()) )
            transformMenu->addAction("SoTabBoxManip", this, SLOT(SoTransform_to_SoTabBoxManip()));
        if (!transformType.isDerivedFrom(SoTrackballManip::getClassTypeId()) )
            transformMenu->addAction("SoTrackballManip", this, SLOT(SoTransform_to_SoTrackballManip()));
        if (!transformType.isDerivedFrom(SoTransformBoxManip::getClassTypeId()) )
            transformMenu->addAction("SoTransformBoxManip", this, SLOT(SoTransform_to_SoTransformBoxManip()));
        if (!transformType.isDerivedFrom(SoTransformerManip::getClassTypeId()) )
            transformMenu->addAction("SoTransformerManip", this, SLOT(SoTransform_to_SoTransformerManip()));
        if (transformType.isDerivedFrom(SoTransformManip::getClassTypeId()) )
            transformMenu->addAction("SoTransform", this, SLOT(SoManip_to_SoTransform()) );
    }

    popupmenu.exec(QCursor::pos());
}

//View menu actions
//void MainWindow::on_actionViewAxes_toggled()
//{
//    m_graphicView[0]->showAxes(ui->actionViewAxes->isChecked());
//    m_graphicView[1]->showAxes(ui->actionViewAxes->isChecked());
//    m_graphicView[2]->showAxes(ui->actionViewAxes->isChecked());
//    m_graphicView[3]->showAxes(ui->actionViewAxes->isChecked());
//}

void MainWindow::on_actionQuadView_toggled()
{
    // use SoQtPlaneViewer
    if (!ui->actionQuadView->isChecked()) {
        m_graphicView[1]->hide();
        m_graphicView[2]->hide();
        m_graphicView[3]->hide();
        m_focusView = 0;
        QSplitter* splitter = findChild<QSplitter*>("graphicSplitterH2");
        splitter->hide();
    } else {       
        m_graphicView[1]->show();
        m_graphicView[2]->show();
        m_graphicView[3]->show();
        QSplitter* splitter = findChild<QSplitter*>("graphicSplitterH2");
        splitter->show();
    }
}

void MainWindow::on_actionRunScript_triggered()
{
//    m_undoStack->setActive(false);
//    m_undoStack->beginMacro("Script");
    ScriptWindow* window = new ScriptWindow(this, 0);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
//    m_undoStack->setActive(true);
//    m_undoStack->endMacro();
}

void MainWindow::on_actionHelpAbout_triggered()
{
    AboutDialog dialog;
    dialog.exec();
}

void MainWindow::treeWorldClicked(QTreeWidgetItem* item, int /*column*/)
{
    TSceneKit* sceneKit = m_document->getSceneKit();
    SoNode* node = 0;
    QString name = item->text(0);
    if (name == "Location")
        node = sceneKit->getPart("world.location", false);
    else if (name == "Sun")
        node = sceneKit->getPart("world.sun", false);
    else if (name == "Air")
        node = sceneKit->getPart("world.air", false);
    else if (name == "Terrain")
        node = sceneKit->getPart("world.terrain", false);
    else if (name == "Camera")
        node = sceneKit->getPart("world.camera", false);
//    else if (name == "SoCamera")
//        node = m_graphicView[m_focusView]->getCamera();

    if (node)
        ui->parametersTabs->setNode(node);
}

void MainWindow::treeWorldDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    QString name = item->text(0);
    if (name == "Sun")
        onSunDialog();
    else if (name == "Air")
        onAirDialog();
}

void MainWindow::tabChanged(int tab)
{
    if (tab == 2) { // Layout
        QModelIndex index = ui->sceneView->currentIndex();
        SoNode* node;
        if (!index.isValid())
            node = 0;
        else if (index == ui->sceneView->rootIndex())
            node = 0;
        else {
            InstanceNode* instance = m_modelScene->getInstance(index);
            node = instance->getNode();
        }
        ui->parametersTabs->setNode(node);
        m_graphicsRoot->enableSelection(true);
    } else if (tab == 0) { // World
        treeWorldClicked(ui->treeWorldWidget->currentItem(), 0); // redo
        m_graphicsRoot->enableSelection(false);
    }
}

void MainWindow::ChangeNodeName(const QModelIndex& index, const QString& name)
{
    if (!index.isValid()) return;

    InstanceNode* instance = m_modelScene->getInstance(index);
    if (!instance) return;
    if (!instance->getNode()) return;

    CmdRename* cmd = new CmdRename(index, name, m_modelScene);
    m_undoStack->push(cmd);

    setDocumentModified(true);
}

/*!
 * Adds a surface to the export sufaces list.
 */
void MainWindow::AddExportSurfaceURL(QString nodeURL)
{
    if (!m_photonsSettings) return;
    m_photonsSettings->surfaces << nodeURL;
}

/*!
 * Changes the light position to the position defined by \a azimuth and \a elevation.
 * The parameters are defined in degree.
 */
void MainWindow::ChangeSunPosition(double azimuth, double elevation)
{
    TSceneKit* scene = m_document->getSceneKit();
    SunKit* sk = (SunKit*) scene->getPart("world.sun", false);
    SunPosition* sp = (SunPosition*) sk->getPart("position", false);
    sp->azimuth = azimuth;
    sp->elevation = elevation;

//    TSceneKit* sceneKit = m_document->getSceneKit();
//    SunKit* sunKit = (SunKit*) sceneKit->getPart("lightList[0]", false);
//    if (!sunKit)
//    {
//        QMessageBox::warning(this, "Tonatiuh warning", tr("ChangeSunPosition:: Sun not defined in scene") );
//        return;
//    }

//    CmdLightPositionModified* command = new CmdLightPositionModified(sunKit, azimuth*gcf::degree, elevation*gcf::degree);
//    m_undoStack->push(command);

//    //UpdateLightDimensions();
//    sceneKit->updateTrackers();
//    UpdateLightSize();
//    setDocumentModified(true);

//    ui->actionViewRays->setEnabled(false);
//    ui->actionViewRays->setChecked(false);
}

/*!
 * Sets the sun position for the location given by \a latitude and \a longitude coordinates in degrees.
 * Latitude is positive for North and positive longitude for east.
 * The time  defined by \a year, \a month and \a day, \a hours, \a minustes and \a seconds is ut time.
 */
void MainWindow::ChangeSunPosition(int year, int month, int day, double hours, double minutes, double seconds, double latitude, double longitude)
{
    if ( month < 0 || month > 12 ||
         day < 0 || day > 31 ||
         hours < 0 || hours > 23 ||
         minutes < 0 || minutes > 59 ||
         seconds < 0 || seconds > 59 ||
         longitude < -180. || longitude > 180. ||
         latitude < -90. || latitude > 90. )
    {
        showWarning("ChangeSunPosition:: Not valid value define to new sun position.");
        return;
    }

    cTime myTime = {year, month, day, hours, minutes, seconds};
    cLocation myLocation = {longitude, latitude};
    cSunCoordinates results;
    sunpos(myTime, myLocation, &results);
    ChangeSunPosition(results.dAzimuth, (90 - results.dZenithAngle) );

//    ui->actionViewRays->setEnabled(false);
//    ui->actionViewRays->setChecked(false);
}


/*!
 * Copies current node to the clipboard.
 * The current node cannot be the model root node or concentrator node.
 */
void MainWindow::Copy()
{
    if (!m_modelSelection->hasSelection() )
    {
        emit Abort(tr("Copy: No node seleted to copy.") );
        return;
    }
    if (m_modelSelection->currentIndex() == ui->sceneView->rootIndex() )
    {
        emit Abort(tr("Copy: The root node can not bee copied.") );
        return;
    }
    if (m_modelSelection->currentIndex().parent() == ui->sceneView->rootIndex() )
    {
        emit Abort(tr("Copy: The root node can not bee copied.") );
        return;
    }

    CmdCopy* cmd = new CmdCopy(m_modelSelection->currentIndex(), m_clipboardNodes, m_modelScene);
    m_undoStack->push(cmd);

    setDocumentModified(true);
}

/*!
 * Copies the node defined with the \a nodeURL to the clipboard.
 *
 * The current node cannot be the model root node or concentrator node.
 */
void MainWindow::Copy(QString url)
{
    QModelIndex nodeIndex = m_modelScene->indexFromUrl(url);

    if (!nodeIndex.isValid() )
    {
        emit Abort(tr("Copy: There is no node with defined url.") );
        return;
    }
    if (nodeIndex == ui->sceneView->rootIndex() || nodeIndex.parent() == ui->sceneView->rootIndex() )
    {
        emit Abort(tr("Copy: The root node can not bee copied.") );
        return;
    }

    CmdCopy* command = new CmdCopy(nodeIndex, m_clipboardNodes, m_modelScene);
    m_undoStack->push(command);

    setDocumentModified(true);
}

/*!
 * If there is a node selected and this node is not the root node, cuts current selected node from the model. Otherwise, nothing is done.
 */
void MainWindow::Cut()
{
    bool isValid = true;
    if (!m_modelSelection->hasSelection())
        isValid = false;
    if (m_modelSelection->currentIndex() == ui->sceneView->rootIndex())
        isValid = false;
    if (m_modelSelection->currentIndex().parent() == ui->sceneView->rootIndex())
        isValid = false;

    if (!isValid)
    {
        emit Abort("Cut: No valid node selected to cut.");
        return;
    }

    CmdCut* cmd = new CmdCut(m_modelSelection->currentIndex(), m_clipboardNodes, m_modelScene);
    m_undoStack->push(cmd);

    UpdateLightSize();
    setDocumentModified(true);
}

/*!
 * If there \a nodeURL is a valid node url and this node is not the root node, cuts current selected node from the model. Otherwise, nothing is done.
 */
void MainWindow::Cut(QString url)
{
    if (url.isEmpty() )
    {
        emit Abort(tr("Cut: There is no node with defined url.") );
        return;
    }

    QModelIndex nodeIndex = m_modelScene->indexFromUrl(url);

    if (!nodeIndex.isValid() )
    {
        emit Abort(tr("Cut: There is no node with defined url.") );
        return;
    }

    if (nodeIndex == ui->sceneView->rootIndex() )
    {
        emit Abort(tr("Cut: Selected node is not valid node to cut.") );
        return;
    }
    if (nodeIndex.parent() == ui->sceneView->rootIndex() )
    {
        emit Abort(tr("Cut: Selected node is not valid node to cut.") );
        return;
    }

    CmdCut* command = new CmdCut(nodeIndex, m_clipboardNodes, m_modelScene);
    m_undoStack->push(command);

    UpdateLightSize();
    setDocumentModified(true);
}

/*!
 * * Deletes selected node if there is a valid node to delete.
 */
void MainWindow::Delete()
{
    if (!m_modelSelection->hasSelection())
    {
        emit Abort("Delete: There is no node selected to delete");
        return;
    }
    QModelIndex index = m_modelSelection->currentIndex();
    if (!index.isValid()) return;
    if (index == ui->sceneView->rootIndex()) return;
    if (index.parent() == ui->sceneView->rootIndex()) return;

    m_modelSelection->clearSelection();
    if (index.parent() != ui->sceneView->rootIndex() ) {
        InstanceNode* instance = m_modelScene->getInstance(index);
        m_modelSelection->setCurrentIndex(m_modelScene->indexFromUrl(instance->getParent()->getURL()), QItemSelectionModel::ClearAndSelect);
    }

    Delete(index);
}

/*!
 * Creates a new delete command, where the node with the \a nodeURL was deleted.
 *
 * If \a nodeURL is not a valid node url, nothing is done.
 */
void MainWindow::Delete(QString url)
{
    QModelIndex index = m_modelScene->indexFromUrl(url);
    if (!index.isValid())
    {
        emit Abort(tr("Delete: There is no node with defined url.") );
        return;
    }

    Delete(index);
    if (m_modelSelection->isSelected(index))
        m_modelSelection->clearSelection();
}

/*!
 * Creates a new delete command, where the \a index node was deleted.
 *
 * Returns \a true if the node was successfully deleted, otherwise returns \a false.
 */
bool MainWindow::Delete(QModelIndex index)
{
    CmdDelete* cmd = new CmdDelete(index, m_modelScene);
    m_undoStack->push(cmd);

//    UpdateLightSize();
    setDocumentModified(true);
    return true;
}

/*!
 * Inserts a new node as a \a nodeURL node child. The new node is a copy to node saved into the clipboard.
 * The \a pasteType take "Shared" or "Copied" values.
 */
void MainWindow::Paste(QString url, QString pasteType)
{
    if (!m_clipboardNodes)
    {
        emit Abort(tr("Paste: There is not node copied.") );
        return;
    }

    QModelIndex nodeIndex = m_modelScene->indexFromUrl(url);
    if (!nodeIndex.isValid() )
    {
        emit Abort(tr("Paste: The node url is not valid.") );
        return;
    }

    Paste(nodeIndex, pasteType == "Shared");
}

/*!
 * Inserts a new node as a current node child. The new node is a copy to node saved into the clipboard.
 */
void MainWindow::PasteCopy()
{
    if (!m_modelSelection->hasSelection())
    {
        emit Abort("PasteCopy: There is not node copied.");
        return;
    }
    Paste(m_modelSelection->currentIndex(), false);
}

/*!
 * Inserts a new node as a current node child. The new node is a link to node saved into the clipboard.
 */
void MainWindow::PasteLink()
{
    if (!m_modelSelection->hasSelection())
    {
        emit Abort("PasteCopy: There is not node copied.");
        return;
    }
    Paste(m_modelSelection->currentIndex(), true);
}

/*!
 * Creates a new group node as a selected node child.
 */
void MainWindow::InsertNode()
{
    QModelIndex index = ui->sceneView->currentIndex();
    if (!index.isValid()) return;
    ui->sceneView->expand(index);

    InstanceNode* instance = m_modelScene->getInstance(index);
    if (!instance) {
        emit Abort("InsertNode: Error creating new group node.");
        return;
    }
    SoNode* node = instance->getNode();
    if (!node) {
        emit Abort("InsertNode: Error creating new group node.");
        return;
    }
    if (node->getTypeId() != TSeparatorKit::getClassTypeId()) {
        showInStatusBar("Parent must be a Group node");
        return;
    }

    TSeparatorKit* kit = new TSeparatorKit;
//    qDebug() << "InsertNode: refs " << kit->getRefCount();
    CmdInsertNode* cmd = new CmdInsertNode(kit, index);
    m_undoStack->push(cmd);
//    qDebug() << "InsertNode: refs " << kit->getRefCount();

    m_modelScene->setNodeNameUnique(kit, "Node");
    setDocumentModified(true);
}

void MainWindow::InsertShape()
{
    QModelIndex index = ui->sceneView->currentIndex();
    if (!index.isValid()) return;
    ui->sceneView->expand(index);

    InstanceNode* instance = m_modelScene->getInstance(index);
    if (!instance) return;
    SoNode* node = instance->getNode();
    if (!node) return;
    if (node->getTypeId() != TSeparatorKit::getClassTypeId()) {
        showInStatusBar("Parent must be a Group node");
        return;
    }

    TShapeKit* kit = new TShapeKit;
    CmdInsertNode* cmd = new CmdInsertNode(kit, index);
    m_undoStack->push(cmd);

    m_modelScene->setNodeNameUnique(kit, "Shape");
    setDocumentModified(true);
}

void MainWindow::InsertTracker()
{
    QModelIndex index = ui->sceneView->currentIndex();
    if (!index.isValid()) return;
    ui->sceneView->expand(index);

    InstanceNode* instance = m_modelScene->getInstance(index);
    SoNode* node = instance->getNode();
    if (node->getTypeId() != TSeparatorKit::getClassTypeId()) {
        showInStatusBar("Parent must be a Group node");
        return;
    }

    TSeparatorKit* parent = (TSeparatorKit*) node;
    if (m_modelScene->hasChild(TrackerKit::getClassTypeId(), parent)) {
        showInStatusBar("Parent already has a Tracker node");
        return;
    }

    TrackerKit* kit = new TrackerKit;
    kit->m_parent = parent;
    CmdInsertNode* cmd = new CmdInsertNode(kit, index);
    m_undoStack->push(cmd);

    m_modelScene->setNodeNameUnique(kit, "Tracker");
    setDocumentModified(true);
}

void MainWindow::InsertArray()
{
    QModelIndex index = ui->sceneView->currentIndex();
    if (!index.isValid()) return;
    ui->sceneView->expand(index);

    InstanceNode* instance = m_modelScene->getInstance(index);
    SoNode* node = instance->getNode();
    if (node->getTypeId() != TSeparatorKit::getClassTypeId()) {
        showInStatusBar("Parent must be a Group node");
        return;
    }

    TArrayKit* kit = new TArrayKit;
    CmdInsertNode* cmd = new CmdInsertNode(kit, index);
    m_undoStack->push(cmd);
    kit->m_parent = (TSeparatorKit*) node;

    m_modelScene->setNodeName(kit, "Array");
    setDocumentModified(true);
}

/*!
 * Creates a \a shapeType shape node from the as current selected node child with the parameters defined in \a parametersList. \a numberOfParameters is the
 * number of parametners in the vector \a numberOfParameters
 *
 * If the current node is not a valid parent node or \a shapeType is not a valid type, the shape node will not be created.
 *
 */
void MainWindow::InsertShapeSurface(QString name, int numberOfParameters, QVector<QVariant> parametersList)
{
    ShapeFactory* f = m_pluginManager->getShapeMap().value(name, 0);
    if (f)
        InsertShapeSurface(f, numberOfParameters, parametersList);
    else
        emit Abort("CreateShape: Selected shape type is not valid.");
}

/*!
 * Creates a \a componentType component node with the name \a nodeName.
 */
void MainWindow::CreateComponentNode(QString componentType, QString nodeName, int numberofParameters, QVector<QVariant> parametersList)
{
    QModelIndex parentIndex = ui->sceneView->currentIndex();
    if (!parentIndex.isValid()) return;

    InstanceNode* parentInstance = m_modelScene->getInstance(parentIndex);
    SoNode* parentNode = parentInstance->getNode();
    if (!parentNode->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId() ) ) return;

    QVector<ComponentFactory*> factoryList = m_pluginManager->getComponentFactories();
    if (factoryList.size() == 0) return;

    QVector<QString> componentNames;
    for (int i = 0; i < factoryList.size(); i++)
        componentNames << factoryList[i]->name();

    int selectedCompoent = componentNames.indexOf(componentType);
    if (selectedCompoent < 0)
    {
        emit Abort(tr("CreateComponentNode: Selected component type is not valid.") );
        return;
    }
    setDocumentModified(true);

    ComponentFactory* pComponentFactory = factoryList[selectedCompoent];
    if (!pComponentFactory) return;

    //CreateComponent( pComponentFactory );

    TSeparatorKit* componentLayout = pComponentFactory->create(m_pluginManager, numberofParameters, parametersList);
    if (!componentLayout) return;

//    QString typeName = pComponentFactory->name();
    componentLayout->setName(nodeName.toStdString().c_str() );

    CmdInsertNode* cmd = new CmdInsertNode(componentLayout, parentIndex);
    QString commandText = QString("Create Component: %1").arg(pComponentFactory->name().toLatin1().constData() );
    cmd->setText(commandText);
    m_undoStack->push(cmd);

    UpdateLightSize();
    setDocumentModified(true);
}



#include "script/NodeObject.h"
#include <QQmlEngine>
void MainWindow::InsertScene(QJSValue v)
{
//    QQmlEngine::setObjectOwnership(v.toQObject(), QQmlEngine::CppOwnership);

//    v.toQObject()->setParent(this);

    QModelIndex index = ui->sceneView->currentIndex();
    if (!index.isValid())
        index = m_modelScene->index(0, 0, ui->sceneView->rootIndex()); //! root is invisible

    SoNode* node = m_modelScene->getInstance(index)->getNode();
    if (!node->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId()))
    {
        emit Abort("InsertFileComponent: Parent node is not valid node type.");
        return;
    }
    NodeObject* nodeObject = (NodeObject*) v.toQObject();
    CmdInsertNode* cmd = new CmdInsertNode(nodeObject->getNode(), index);
    cmd->setText("Insert Node from script");
    m_undoStack->push(cmd);

    UpdateLightSize();
    setDocumentModified(true);
}

QJSValue MainWindow::getScene()
{
    NodeObject* ans = new NodeObject(getSceneKit());
    return qjsEngine(this)->newQObject(ans);
}

QJSValue MainWindow::FindInterception(QJSValue surface, QJSValue rays, QJSValue show)
{
//    m_graphicView[0]->render();
//    double ans = findInterception(surface.toString(), rays.toUInt(), this);
//    return ans;

    if (!m_rand)
        m_rand = m_pluginManager->getRandomFactories()[m_raysRandomFactoryIndex]->create(0);

    FluxAnalysis fa(m_document->getSceneKit(), m_modelScene, m_raysGridWidth, m_raysGridHeight, m_rand);
    fa.run(surface.toString(), "front", rays.toUInt(), false, 5, 5, true);
    double ans = fa.powerTotal();

    if (show.toBool() == true) {
         trf::DrawRays(m_graphicsRoot->rays(), *fa.getPhotonsBuffer(), m_raysScreen);
        m_graphicsRoot->showRays(true);
//       m_graphicView[0]->render();
    }
    return ans;
}

void MainWindow::Screenshot(QJSValue filename)
{
    QScreen* screen = QGuiApplication::primaryScreen();
//    if (const QWindow *window = windowHandle())
//        screen = window->screen();
//    if (!screen) return;

    raise();
    repaint();
    m_graphicView[0]->render();

    QDir dir = QDir::searchPaths("project")[0];
    QString fn = dir.absoluteFilePath(filename.toString());
    QFileInfo fi(fn);
    dir.mkpath(fi.absolutePath());

    QPixmap pm = screen->grabWindow(winId());
    pm.save(fn);
}

QJSValue MainWindow::print(QString text)
{
    qDebug() << text;
    return true;
}

QJSValue MainWindow::printTimed(QString text)
{
    qDebug() << text;
    return true;
}

#include "application/view/GraphicView.h"
QJSValue MainWindow::render()
{
//    repaint();
    m_graphicView[0]->repaint();
    m_graphicView[0]->render();
    m_graphicView[0]->repaint();
    return true;
}

/*!
 * Changes selected node to the node with \a nodeUrl. If model does not contains a node with defined url,
 * the selection will be null.
 */
void MainWindow::Select(QString url)
{
    QModelIndex index = m_modelScene->indexFromUrl(url);
    m_modelSelection->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
}

/*!
 * Sets \a nodeName as the current node name.
 */
void MainWindow::SetName(QString name)
{
    if (!m_modelSelection->hasSelection())
    {
        emit Abort(tr("SetNodeName: No node selected.") );
        return;
    }

    if (m_modelSelection->currentIndex() == ui->sceneView->rootIndex())
    {
        emit Abort(tr("SetNodeName: Cannot change the name of the current selected node cannot.") );
        return;
    }

    ChangeNodeName(m_modelSelection->currentIndex(), name);
}

/*!
 * Sets to the \a parameter of the node \a nodeUrl the value defined at \a value.
 */
void MainWindow::SetValue(QString url, QString parameter, QString value)
{
    if (url.isEmpty() || parameter.isEmpty() || value.isEmpty() )
    {
        emit Abort(tr("SetValue: You must define a node url, a parameter name and a value.") );
        return;
    }

    QModelIndex nodeIndex = m_modelScene->indexFromUrl(url);
    if (!nodeIndex.isValid() )
    {
        emit Abort(tr("SetValue: Defined node url is not a valid url.") );
        return;
    }

    InstanceNode* nodeInstance = m_modelScene->getInstance(nodeIndex);
    if (!nodeInstance)
    {
        emit Abort(tr("SetValue: Defined node url is not a valid url.") );
        return;
    }

    SoNode* node = nodeInstance->getNode();
    if (!node) return;
    if (node->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId() ) )
    {
        TSeparatorKit* separatorNode = static_cast< TSeparatorKit* >(node);
        SoTransform* nodeTransform = static_cast<SoTransform*>(separatorNode->getPart("transform", true) );
        node = nodeTransform;
    }

    SoField* parameterField = node->getField(parameter.toStdString().c_str() );
    if (!parameterField)
    {
        showWarning(tr("Defined node does not contain the parameter %1.").arg(parameter));
        return;
    }

    setFieldText(node, parameter, value);
}

/*!
 * Starts new tonatiuh empty model.
 */
void MainWindow::fileNew()
{
    if (!OkToContinue())
    {
        emit Abort("Current Tonatiuh model cannot be closed.");
        return;
    }
    openFileProject("");
}

/*!
 * Clears current design in Tonatiuh without verify if the changes have been saved.
 */
void MainWindow::Clear()
{
    openFileProject("");
}


/*!
 * Opens \a fileName file is there is a valid tonatiuh file.
 */
void MainWindow::fileOpen(QString fileName)
{
    QFileInfo info(fileName);
    if (!info.exists() || !info.isFile())
    {
        QString text = tr("Open: Cannot open file:\n%1.").arg(fileName);
        showWarning(text);
        emit Abort(text);
        return;
    }

    if (info.suffix() == "tnhpp")
        openFileProject(fileName);
    else if (info.suffix() == "tnhpps")
        openFileScript(fileName);
    else {
        QString text = tr("Open: Unknown format:\n%1.").arg(fileName);
        showWarning(text);
        emit Abort(text);
        return;
    }
}

/*!
 * Runs ray tracer to defined model and paramenters.
 */
void MainWindow::Run()
{
    InstanceNode* instanceLayout = 0;
    InstanceNode instanceSun(0);


    AirTransmission* air = 0;

    QElapsedTimer timer;
    timer.start();

    UpdateLightSize();// better use tree below

    if (!ReadyForRaytracing(instanceLayout, &instanceSun, air) ) return;

    if (!m_photonsBuffer->getExporter() )
    {
//        if (!m_photonsSettings) return;
        if (!m_photonsSettings) {
            m_photonsSettings = new PhotonsSettings;
            PhotonsSettings& settings = *m_photonsSettings;
            settings.name = "No export";

            settings.saveCoordinates = true;
            settings.saveCoordinatesGlobal = true;
            settings.saveSurfaceID = true;
            settings.saveSurfaceSide = true;
            settings.savePhotonsID = true;
        }

        PhotonsAbstract* photonsExporter = CreatePhotonMapExport();
        if (!photonsExporter) return;
        if (!m_photonsBuffer->setExporter(photonsExporter)) return;
    }

    QVector<InstanceNode*> exportSurfaceList;
    for (QString s : m_photonsSettings->surfaces)
    {
        m_modelScene->indexFromUrl(s);
        InstanceNode* node = m_modelScene->getInstance(m_modelScene->indexFromUrl(s));
        exportSurfaceList << node;
    }

    instanceLayout->updateTree(Transform::Identity);

    SunKit* sunKit = (SunKit*) instanceSun.getNode();
    SunPosition* sunPosition = (SunPosition*) sunKit->getPart("position", false);
    SunShape* sunShape = (SunShape*) sunKit->getPart("shape", false);
    SunAperture* sunAperture = (SunAperture*) sunKit->getPart("aperture", false);
    if (!sunKit->findTexture(m_raysGridWidth, m_raysGridHeight, instanceLayout))
    {
        emit Abort(tr("There are no surfaces defined for ray tracing") );
        ShowRaysIn3DView(); // cleaning?
        return;
    }

    QVector<long> raysPerThread;
    int progressMax = 100;
    ulong t1 = m_raysNumber/progressMax;
    for (int progress = 0; progress < progressMax; ++progress)
        raysPerThread << t1;

    if (t1*progressMax < m_raysNumber)
        raysPerThread << m_raysNumber - t1*progressMax;



    // single thread for gprof
    //        QThreadPool::globalInstance()->setMaxThreadCount(1);
    // change ideal

    // Create a progress dialog.
    QProgressDialog dialog;
    dialog.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    dialog.setLabelText(QString("Progressing using %1 thread(s)...").arg(QThread::idealThreadCount() ) );

    QFutureWatcher<void> watcher;
    connect(&watcher, SIGNAL(finished()), &dialog, SLOT(reset()));
    connect(&dialog, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    connect(&watcher, SIGNAL(progressRangeChanged(int, int)), &dialog, SLOT(setRange(int, int)));
    connect(&watcher, SIGNAL(progressValueChanged(int)), &dialog, SLOT(setValue(int)));

    std::cout << "QtConcurrent started: " << timer.elapsed() << std::endl;
    QMutex mutex;
    QMutex mutexPhotonMap;
    QFuture<void> photonMap;
    AirTransmission* airTemp = 0;
    if (air->getTypeId() != AirVacuum::getClassTypeId())
        airTemp = air;

    photonMap = QtConcurrent::map(raysPerThread, RayTracer(instanceLayout,
                                                           &instanceSun, sunAperture, sunShape, airTemp,
                                                           m_rand,
                                                           &mutex, m_photonsBuffer, &mutexPhotonMap,
                                                           exportSurfaceList) );
    watcher.setFuture(photonMap);

    dialog.exec();
    watcher.waitForFinished();
    std::cout << "QtConcurrent finished: " << timer.elapsed() << std::endl;

    m_raysTracedTotal += m_raysNumber;

    if (exportSurfaceList.empty())
        ShowRaysIn3DView(); // all photons must be stored
    else {
        //            ui->actionViewRays->setEnabled(false);
        //            ui->actionViewRays->setChecked(false);
    }

    double area = sunAperture->getArea();
    double irradiance = sunPosition->irradiance.getValue();
    double power = area*irradiance/m_raysTracedTotal;
    m_photonsBuffer->endExport(power);

    std::cout << "Elapsed time (Run): " << timer.elapsed() << std::endl;

    QString msg = QString("Timing: %1 s").arg(timer.elapsed()/1000., 0, 'f', 3);
    showInStatusBar(msg, 2000);
}

/*
 * Runs ray trace to calculate a flux distribution map in the surface of the node \a nodeURL related to the side \a surfaceSide.
 * The map will be calculated with the parameters \a nOfRays, \a heightDivisions and \a heightDivisions.
 * The results will save in a file \a directory \a QString fileName, the coordinates of the cells depending on the variable \a saveCoord.
 */
void MainWindow::RunFluxAnalysis(QString nodeURL, QString surfaceSide, uint nOfRays, int heightDivisions, int widthDivisions, QString fileName, bool saveCoords)
{
    TSceneKit* sceneKit = m_document->getSceneKit();
    if (!sceneKit) return;

    if (!m_rand)
        m_rand = m_pluginManager->getRandomFactories()[m_raysRandomFactoryIndex]->create(0);

    FluxAnalysis fa(sceneKit, m_modelScene, m_raysGridWidth, m_raysGridHeight, m_rand);
    fa.run(nodeURL, surfaceSide, nOfRays, false, heightDivisions, widthDivisions); //?
    fa.write(fileName, saveCoords);
}

/*!
 * Saves current tonatiuh model into \a fileName file.
 */
void MainWindow::fileSaveAs(QString fileName)
{
    if (fileName.isEmpty())
    {
        emit Abort(tr("SaveAs: There is no file defined."));
        return;
    }
    QFileInfo info(fileName);
    if (info.completeSuffix() != "tnhpp")
    {
        emit Abort(tr("SaveAs: The file defined is not a tonatiuh file. The suffix must be tnh.") );
        return;
    }
    fileSave(fileName);
}

/*!
 * Sets to export all surfaces photons.
 */
void MainWindow::SetExportAllPhotonMap()
{
    if (!m_photonsSettings) return;
    m_photonsSettings->surfaces.clear();
}

/*!
 * If \a enabled is true, sets to export photons coordinates. Otherwise, the photons will not be exported.
 * If \a global is true, the coordinates will be exported in global coordinate system. Otherwise, into local to surface.
 */
void MainWindow::SetExportCoordinates(bool enabled, bool global)
{
    if (!m_photonsSettings) return;
    m_photonsSettings->saveCoordinates = enabled;
    m_photonsSettings->saveCoordinatesGlobal = global;
}

/*!
 * If \a enabled is true, sets to export intersection surface.
 * Otherwise, the intersection surface will not be exported.
 */
void MainWindow::SetExportIntersectionSurface(bool enabled)
{
    if (!m_photonsSettings) return;
    m_photonsSettings->saveSurfaceID = enabled;
}

/*!
 * If \a enabled is true, sets to export intersection surface side.
 * Otherwise, the intersection surface side will not be exported.
 */
void MainWindow::SetExportIntersectionSurfaceSide(bool enabled)
{
    if (!m_photonsSettings) return;
    m_photonsSettings->saveSurfaceSide = enabled;
}

/*!
 * Sets the export photon mode type to \a exportModeType, for photon map.
 */
void MainWindow::SetExportPhotonMapType(QString name)
{
    QVector<PhotonsFactory*> factoryList = m_pluginManager->getExportFactories();
    if (factoryList.size() == 0) return;

    QVector<QString> exportPMModeNames;
    for (int i = 0; i < factoryList.size(); i++)
        exportPMModeNames << factoryList[i]->name();

    if (exportPMModeNames.indexOf(name) < 0)
    {
        emit Abort(tr("exportModeType: Defined export mode is not valid type."));
        return;
    }

    if (!m_photonsSettings)
        m_photonsSettings = new PhotonsSettings;

    m_photonsSettings->name = name;
}

/*!
 * If \a enabled is true, sets to export for each photon the prevous and next photoan id.
 * Otherwise, this information will not be exported.
 */
void MainWindow::SetExportPreviousNextPhotonID(bool enabled)
{
    if (!m_photonsSettings) return;
    m_photonsSettings->savePhotonsID = enabled;
}

/*!
 * Sets to selected export type parameter named \a parameterName the value \a parameterValue.
 */
void MainWindow::SetExportTypeParameterValue(QString name, QString value)
{
    if (!m_photonsSettings) return;
    m_photonsSettings->parameters.insert(name, value);
}

/*!
 * Sets the random number generator type, \a typeName, for ray tracing.
 */
void MainWindow::SetRaysRandomFactory(QString name)
{
//    RandomFactory* f = m_pluginManager->getRandomMap().value(name, 0);
//    if (!f) return;

    QVector<RandomFactory*> factories = m_pluginManager->getRandomFactories();
    if (factories.size() == 0) return;

    QVector< QString > randomNames;
    for (int i = 0; i < factories.size(); i++)
        randomNames << factories[i]->name();

    int oldSelectedRandomDeviate = m_raysRandomFactoryIndex;

    if (randomNames.indexOf(name) < 0)
    {
        emit Abort(tr("SetRandomDeviateType: Defined random generator is not valid type.") );
        return;
    }
    m_raysRandomFactoryIndex = randomNames.indexOf(name);
    if (oldSelectedRandomDeviate != m_raysRandomFactoryIndex)
    {
        delete m_rand;
        m_rand = 0;
    }
}

/*!
 * Sets the ray casting surface grid elemets to \a widthDivisions x \a heightDivisions.
 */
void MainWindow::SetRaysGrid(int width, int height)
{
    m_raysGridWidth = width;
    m_raysGridHeight = height;
}

//Manipulators actions
void MainWindow::SoTransform_to_SoCenterballManip()
{
    //Transform to a SoCenterballManip manipulator
    QModelIndex currentIndex = ui->sceneView->currentIndex();

    InstanceNode* instanceNode = m_modelScene->getInstance(currentIndex);
    SoBaseKit* coinNode = static_cast<SoBaseKit*>(instanceNode->getNode() );
    SoTransform* transform = static_cast<SoTransform*>(coinNode->getPart("transform", false) );

    SoCenterballManip* manipulator = new SoCenterballManip;
    manipulator->rotation.setValue(transform->rotation.getValue());
    manipulator->translation.setValue(transform->translation.getValue());
    manipulator->scaleFactor.setValue(transform->scaleFactor.getValue());
    manipulator->scaleOrientation.setValue(transform->scaleOrientation.getValue());
    manipulator->center.setValue(transform->center.getValue());

    coinNode->setPart("transform", manipulator);
    ChangeSelection(currentIndex);

    SoDragger* dragger = manipulator->getDragger();
    dragger->addStartCallback (startManipulator, static_cast<void*>(this) );
    dragger->addFinishCallback(finishManipulator, static_cast<void*>(this) );

    setDocumentModified(true);
}

void MainWindow::SoTransform_to_SoJackManip()
{
    //Transform to a SoJackManip manipulator
    QModelIndex currentIndex = ui->sceneView->currentIndex();

    InstanceNode* instanceNode = m_modelScene->getInstance(currentIndex);
    SoBaseKit* coinNode = static_cast< SoBaseKit* >(instanceNode->getNode() );
    SoTransform* transform = static_cast< SoTransform* >(coinNode->getPart("transform", false) );

    SoJackManip* manipulator = new SoJackManip;
    manipulator->rotation.setValue(transform->rotation.getValue());
    manipulator->translation.setValue(transform->translation.getValue());
    manipulator->scaleFactor.setValue(transform->scaleFactor.getValue());
    manipulator->scaleOrientation.setValue(transform->scaleOrientation.getValue());
    manipulator->center.setValue(transform->center.getValue());

    coinNode->setPart("transform", manipulator);
    ChangeSelection(currentIndex);

    SoDragger* dragger = manipulator->getDragger();
    dragger->addStartCallback (startManipulator, static_cast< void*>(this) );
    dragger->addFinishCallback(finishManipulator, static_cast< void*>(this) );

    setDocumentModified(true);
}

void MainWindow::SoTransform_to_SoHandleBoxManip()
{
    //Transform to a SoHandleBoxManip manipulator
    QModelIndex currentIndex = ui->sceneView->currentIndex();

    InstanceNode* instanceNode = m_modelScene->getInstance(currentIndex);
    SoBaseKit* coinNode = static_cast< SoBaseKit* >(instanceNode->getNode() );
    SoTransform* transform = static_cast< SoTransform* >(coinNode->getPart("transform", false) );

    SoHandleBoxManip* manipulator = new SoHandleBoxManip;
    manipulator->rotation.setValue(transform->rotation.getValue());
    manipulator->translation.setValue(transform->translation.getValue());
    manipulator->scaleFactor.setValue(transform->scaleFactor.getValue());
    manipulator->scaleOrientation.setValue(transform->scaleOrientation.getValue());
    manipulator->center.setValue(transform->center.getValue());

    coinNode->setPart("transform", manipulator);
    ChangeSelection(currentIndex);

    SoDragger* dragger = manipulator->getDragger();
    dragger->addStartCallback (startManipulator, static_cast< void*>(this) );
    dragger->addFinishCallback(finishManipulator, static_cast< void*>(this) );

    setDocumentModified(true);
}

void MainWindow::SoTransform_to_SoTabBoxManip()
{
    //Transform to a SoTabBoxManip manipulator
    QModelIndex currentIndex = ui->sceneView->currentIndex();

    InstanceNode* instanceNode = m_modelScene->getInstance(currentIndex);
    SoBaseKit* coinNode = static_cast< SoBaseKit* >(instanceNode->getNode() );
    SoTransform* transform = static_cast< SoTransform* >(coinNode->getPart("transform", false) );

    SoTabBoxManip* manipulator = new SoTabBoxManip;
    manipulator->rotation.setValue(transform->rotation.getValue());
    manipulator->translation.setValue(transform->translation.getValue());
    manipulator->scaleFactor.setValue(transform->scaleFactor.getValue());
    manipulator->scaleOrientation.setValue(transform->scaleOrientation.getValue());
    manipulator->center.setValue(transform->center.getValue());

    coinNode->setPart("transform", manipulator);
    ChangeSelection(currentIndex);

    SoDragger* dragger = manipulator->getDragger();
    dragger->addStartCallback (startManipulator, static_cast< void*>(this) );
    dragger->addFinishCallback(finishManipulator, static_cast< void*>(this) );

    setDocumentModified(true);
}

void MainWindow::SoTransform_to_SoTrackballManip()
{
    //Transform to a SoTrackballManip manipulator
    QModelIndex currentIndex = ui->sceneView->currentIndex();

    InstanceNode* instanceNode = m_modelScene->getInstance(currentIndex);
    SoBaseKit* coinNode = static_cast< SoBaseKit* >(instanceNode->getNode() );
    SoTransform* transform = static_cast< SoTransform* >(coinNode->getPart("transform", false) );

    SoTrackballManip* manipulator = new SoTrackballManip;
    manipulator->rotation.setValue(transform->rotation.getValue());
    manipulator->translation.setValue(transform->translation.getValue());
    manipulator->scaleFactor.setValue(transform->scaleFactor.getValue());
    manipulator->scaleOrientation.setValue(transform->scaleOrientation.getValue());
    manipulator->center.setValue(transform->center.getValue());

    coinNode->setPart("transform", manipulator);
    ChangeSelection(currentIndex);

    SoDragger* dragger = manipulator->getDragger();
    dragger->addStartCallback (startManipulator, static_cast< void*>(this) );
    dragger->addFinishCallback(finishManipulator, static_cast< void*>(this) );

    setDocumentModified(true);
}

void MainWindow::SoTransform_to_SoTransformBoxManip()
{
    //Transform to a SoTransformBoxManip manipulator
    QModelIndex currentIndex = ui->sceneView->currentIndex();

    InstanceNode* instanceNode = m_modelScene->getInstance(currentIndex);
    SoBaseKit* coinNode = static_cast< SoBaseKit* >(instanceNode->getNode() );
    SoTransform* transform = static_cast< SoTransform* >(coinNode->getPart("transform", false) );

    SoTransformBoxManip* manipulator = new SoTransformBoxManip;
    manipulator->rotation.setValue(transform->rotation.getValue());
    manipulator->translation.setValue(transform->translation.getValue());
    manipulator->scaleFactor.setValue(transform->scaleFactor.getValue());
    manipulator->scaleOrientation.setValue(transform->scaleOrientation.getValue());
    manipulator->center.setValue(transform->center.getValue());

    coinNode->setPart("transform", manipulator);

    ChangeSelection(currentIndex);

    SoDragger* dragger = manipulator->getDragger();
    dragger->addStartCallback (startManipulator, static_cast< void*>(this) );
    dragger->addFinishCallback(finishManipulator, static_cast< void*>(this) );

    setDocumentModified(true);
}

void MainWindow::SoTransform_to_SoTransformerManip()
{
    //Transform to a SoTransformerManip manipulator
    QModelIndex currentIndex = ui->sceneView->currentIndex();

    InstanceNode* instanceNode = m_modelScene->getInstance(currentIndex);
    SoBaseKit* coinNode = static_cast< SoBaseKit* >(instanceNode->getNode() );
    SoTransform* transform = static_cast< SoTransform* >(coinNode->getPart("transform", false) );

    SoTransformerManip* manipulator = new SoTransformerManip;
    manipulator->rotation.setValue(transform->rotation.getValue());
    manipulator->translation.setValue(transform->translation.getValue());
    manipulator->scaleFactor.setValue(transform->scaleFactor.getValue());
    manipulator->scaleOrientation.setValue(transform->scaleOrientation.getValue());
    manipulator->center.setValue(transform->center.getValue());


    coinNode->setPart("transform", manipulator);
    ChangeSelection(currentIndex);

    SoDragger* dragger = manipulator->getDragger();
    dragger->addStartCallback (startManipulator, static_cast< void*>(this) );
    dragger->addFinishCallback(finishManipulator, static_cast< void*>(this) );

    setDocumentModified(true);
}

void MainWindow::SoManip_to_SoTransform()
{
    //Transform manipulator to a SoTransform
    QModelIndex currentIndex = ui->sceneView->currentIndex();

    InstanceNode* instanceNode = m_modelScene->getInstance(currentIndex);
    SoBaseKit* coinNode = static_cast< SoBaseKit* >(instanceNode->getNode() );
    SoTransformManip* manipulator = static_cast< SoTransformManip* >(coinNode->getPart("transform", false) );
    if (!manipulator) return;

    SoTransform* transform = new SoTransform;
    transform->rotation.setValue(manipulator->rotation.getValue());
    transform->translation.setValue(manipulator->translation.getValue());
    transform->scaleFactor.setValue(manipulator->scaleFactor.getValue());
    transform->scaleOrientation.setValue(manipulator->scaleOrientation.getValue());
    transform->center.setValue(manipulator->center.getValue());

    coinNode->setPart("transform", transform);
    ChangeSelection(currentIndex);

    setDocumentModified(true);
}

void MainWindow::ChangeSelection(const QModelIndex& index)
{
    if (ui->tabWidget->currentIndex() == 2) { // Layout
        SoNode* node;
        if (!index.isValid())
            node = 0;
        else if (index == ui->sceneView->rootIndex())
            node = 0;
        else {
            InstanceNode* instance = m_modelScene->getInstance(index);
            node = instance->getNode();
        }
        ui->parametersTabs->setNode(node);
    } else if (ui->tabWidget->currentIndex() == 0) { // World
        treeWorldClicked(ui->treeWorldWidget->currentItem(), 0); // redo
    }
}

/*!
 * Creates a componet subtree from the \a pComponentFactory as current selected node child.
 *
 * If the current node is not a group type node, the component subtree will not be created.
 */
void MainWindow::CreateComponent(ComponentFactory* factory)
{
    QModelIndex index = ui->sceneView->currentIndex();
    if (!index.isValid())
        index = ui->sceneView->rootIndex();

    SoNode* node = m_modelScene->getInstance(index)->getNode();
    if (!node->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId())) return;

    TSeparatorKit* nodeCreated = factory->create(m_pluginManager);
    if (!nodeCreated) return;
    nodeCreated->setName(factory->name().toLatin1().data());

    // cmd
    CmdInsertNode* cmd = new CmdInsertNode(nodeCreated, index);
    QString text = QString("Create Component: %1").arg(factory->name());
    cmd->setText(text);
    m_undoStack->push(cmd);

    UpdateLightSize();
    setDocumentModified(true);
}

/*!
 * Creates a shape node from the \a pShapeFactory as current selected node child.
 *
 * If the current node is not a surface type node, the shape node will not be created.
 */
void MainWindow::InsertShapeSurface(ShapeFactory* factory, int /*numberofParameters*/, QVector<QVariant> parametersList)
{
    QModelIndex index = ui->sceneView->currentIndex();
    if (!index.isValid()) return;

    InstanceNode* instance = m_modelScene->getInstance(index);
    SoNode* node = instance->getNode();
    TShapeKit* kit = dynamic_cast<TShapeKit*>(node);
    if (!kit) return;

    /*ShapeRT* shape = */factory->create(parametersList);

//    CmdInsertSurface* cmd = new CmdInsertSurface(kit, shape);
//    m_undoStack->push(cmd);

//    UpdateLightSize();
    setDocumentModified(true);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (OkToContinue())
    {
        writeSettings();
        event->accept();
        qApp->quit(); // closes script windows
    }
    else
        event->ignore();
}

/*!
 * Receives mouse press event \a e, an sets as active 3D view the view that contains the mouse press position.
 */
//void MainWindow::mousePressEvent(QMouseEvent* e)
//{
//    QPoint pos = e->pos();
//    int x = pos.x();
//    int y = pos.y() - 64;

//    QSplitter* pSplitter = findChild<QSplitter*>("graphicSplitterV");
//    QRect mainViewRect = pSplitter->geometry();

//    if (mainViewRect.contains(x, y))
//    {
//        QSplitter* pvSplitter1 = findChild<QSplitter*>("graphicSplitterH1");
//        QSplitter* pvSplitter2 = findChild<QSplitter*>("graphicSplitterH1");
//        QRect vViewRect1 = pvSplitter1->geometry();
//        if (vViewRect1.contains(x,y))
//        {
//            QList<int> size(pvSplitter2->sizes());
//            if (x < size[0])
//                m_focusView = 0;
//            else
//                m_focusView = 1;
//        }
//        else
//        {
//            QList<int> size(pvSplitter1->sizes());
//            if (x < size[0])
//                m_focusView = 2;
//            else
//                m_focusView = 3;
//        }
//    }
//}

/*!
 * Set current document coin scene into Tonatiuh Models and views.
 */
void MainWindow::ChangeModelScene()
{
    m_graphicsRoot->setDocument(m_document);
    m_modelScene->setDocument(m_document);
    m_document->getSceneKit()->updateParents();
    m_graphicView[0]->setSceneGraph(m_graphicsRoot);

//    QModelIndex index = m_modelScene->indexFromUrl("//Layout");
//    ui->sceneView->setRootIndex(index);

//    InstanceNode* viewLayout = m_modelScene->getInstance(index);
//    InstanceNode* concentratorRoot = viewLayout->children[0];
//    m_modelSelection->setCurrentIndex(m_modelScene->indexFromUrl(concentratorRoot->getURL() ), QItemSelectionModel::ClearAndSelect);
}

/*!
 * Creates a export mode object form export mode settings.
 */
PhotonsAbstract* MainWindow::CreatePhotonMapExport() const
{
    PhotonsFactory* f = m_pluginManager->getExportMap().value(m_photonsSettings->name, 0);
    if (!f) return 0;
    PhotonsAbstract* photonExport = f->create(0);
    if (!photonExport) return 0;

    photonExport->setSceneModel(*m_modelScene);
    photonExport->setPhotonSettings(m_photonsSettings);
    return photonExport;
}

/*!
 * Returns \a true if the application is ready to start with other model. Otherwise,
 * returns \a false.
 */
bool MainWindow::OkToContinue()
{
    if (!m_document->isModified())
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

/*!
 * Creates a new \a type paste command. The clipboard node is inserted as \a nodeIndex node
 * child.
 *
 * Returns \a true if the node was successfully pasted, otherwise returns \a false.
 */
bool MainWindow::Paste(QModelIndex index, bool isShared)
{
    if (!index.isValid()) return false;
    if (!m_clipboardNodes) return false;

    InstanceNode* instance = m_modelScene->getInstance(index);
    SoNode* node = instance->getNode();

    if (!node->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId()))
        return false;
//    if (node->getTypeId().isDerivedFrom(TShapeKit::getClassTypeId()) &&
//         m_coinNode_Buffer->getTypeId().isDerivedFrom(SoBaseKit::getClassTypeId()))
//        return false;

    if (instance->getNode() == m_clipboardNodes) return false;

    while (instance->getParent()) {
        if (instance->getParent()->getNode() == m_clipboardNodes) return false;
        instance = instance->getParent();
    }

    CmdPaste* cmd = new CmdPaste(isShared, m_modelSelection->currentIndex(), m_clipboardNodes, m_modelScene);
    m_undoStack->push(cmd);

    UpdateLightSize();
    setDocumentModified(true);
    return true;
}

void MainWindow::readSettings()
{
    QSettings settings("Tonatiuh", "Cyprus");

    // geometry
    QRect rect = settings.value("windowGeometry", QRect(200, 200, 400, 400)).toRect();
    setGeometry(rect);

    // window state
    Qt::WindowStates ws = Qt::WindowNoState;
    if (settings.value("windowNoState", false).toBool())
        ws ^= Qt::WindowNoState;
//    if (settings.value("windowMinimized", false).toBool())
//        ws ^= Qt::WindowMinimized;
    if (settings.value("windowMaximized", true).toBool())
        ws ^= Qt::WindowMaximized;
//    if (settings.value("windowFullScreen", false).toBool())
//        ws ^= Qt::WindowFullScreen;
    if (settings.value("windowActive", false).toBool())
        ws ^= Qt::WindowActive;
    setWindowState(ws);

    // recent files
    for (int n = 0; n < m_filesRecentMax; ++n) {
        QAction* a = new QAction;
        connect(
            a, SIGNAL(triggered()),
            this, SLOT(fileOpenRecent())
        );
        ui->menuFileRecent->addAction(a);
    }
    m_filesRecent = settings.value("recentFiles").toStringList();
    updateRecentFiles();
}

void MainWindow::writeSettings()
{
    QSettings settings("Tonatiuh", "Cyprus");

    settings.setValue("windowGeometry", geometry());

    Qt::WindowStates ws = windowState();
    settings.setValue("windowNoState", ws.testFlag(Qt::WindowNoState));
//    settings.setValue("windowMinimized", ws.testFlag(Qt::WindowMinimized));
    settings.setValue("windowMaximized", ws.testFlag(Qt::WindowMaximized));
//    settings.setValue("windowFullScreen", ws.testFlag(Qt::WindowFullScreen));
    settings.setValue("windowActive", ws.testFlag(Qt::WindowActive));

    settings.setValue("recentFiles", m_filesRecent);
}

/*!
 * Updates the recently opened files actions list.
 */
void MainWindow::updateRecentFiles()
{
    QMutableStringListIterator iterator(m_filesRecent);
    while (iterator.hasNext())
        if (!QFile::exists(iterator.next()))
            iterator.remove();

    QIcon icon(":/images/about/Tonatiuh.ico");
    QList<QAction*> actions = ui->menuFileRecent->actions();
    for (int n = 0; n < m_filesRecentMax; ++n) {
        if (n < m_filesRecent.count()) {
            QString path = m_filesRecent[n];
            QString name = QFileInfo(path).fileName();
            QString text = tr("%2 \t&%1").arg(n + 1).arg(name);
//            QString text = tr("%1").arg(name);
            actions[n]->setText(text);
            actions[n]->setToolTip(path); // if enabled in menu
            actions[n]->setStatusTip(path);
            actions[n]->setData(path);
            actions[n]->setIcon(icon);
            actions[n]->setVisible(true);
        } else
            actions[n]->setVisible(false);
    }
}

/*!
 * Checks whether a ray tracing can be started with the current light and model.
 */
bool MainWindow::ReadyForRaytracing(InstanceNode*& instanceLayout,
                                    InstanceNode* instanceSun,
                                    AirTransmission*& air)
{
    TSceneKit* sceneKit = m_document->getSceneKit();
    if (!sceneKit) return false;

    InstanceNode* instanceScene = m_modelScene->getInstance(QModelIndex());
    if (!instanceScene) return false;

    instanceLayout = instanceScene->children[0];
    if (!instanceLayout) return false;

    SunKit* sunKit = (SunKit*) sceneKit->getPart("world.sun", false);
    instanceSun->setNode(sunKit);
    instanceSun->setTransform(tgf::makeTransform(sunKit->m_transform));

    air = (AirTransmission*) sceneKit->getPart("world.air.transmission", false);

    QVector<RandomFactory*> randomFactories = m_pluginManager->getRandomFactories();
    if (!m_rand) m_rand = randomFactories[m_raysRandomFactoryIndex]->create(0);

    if (!m_photonBufferAppend)
    {
        delete m_photonsBuffer;
        m_photonsBuffer = 0;
    }

    if (!m_photonsBuffer)
    {
        m_photonsBuffer = new PhotonsBuffer(m_photonBufferSize, m_photonBufferSize);
        m_raysTracedTotal = 0;
    }

    return true;
}

/*!
 * Returns \a true if the tonatiuh model is correctly saved into the the given \a fileName. Otherwise, returns \a false.
 *
 * \sa Save, SaveAs.
 */
bool MainWindow::fileSave(const QString& fileName)
{
    if (!m_document->WriteFile(fileName))
    {
        showInStatusBar("Saving canceled");
        return false;
    }

    SetCurrentFile(fileName);
    showInStatusBar("File saved");
    return true;
}

/*!
 * Sets \a fileName files as current file.
 */
void MainWindow::SetCurrentFile(const QString& filePath)
{
    m_fileName = filePath;
    setDocumentModified(false);

    QString title = "Untitled";
    if (!m_fileName.isEmpty())
    {
        title = QFileInfo(filePath).fileName();
        m_filesRecent.removeAll(m_fileName);
        m_filesRecent.prepend(m_fileName);
        updateRecentFiles();
    }

    setWindowTitle(tr("%1[*] - Tonatiuh").arg(title));
}

/*!
 * Shows the rays and photons stored at the photon map in the 3D view.
 */
void MainWindow::ShowRaysIn3DView()
{
//    if (true || ui->actionViewRays->isChecked() || ui->actionViewPhotons->isChecked())
//    {
        trf::DrawRays(m_graphicsRoot->rays(), *m_photonsBuffer, m_raysScreen);
        m_graphicView[0]->showRays();
//        m_graphicsRoot->showRays(ui->actionViewRays->isChecked());
//        m_graphicsRoot->showPhotons(ui->actionViewPhotons->isChecked());
//    } else {
//        m_graphicsRoot->rays()->removeAllChildren();
//    }
}

/*!
 * Returns to the start origin state and starts with a new model defined in \a fileName.
 * If the file name is not defined, it starts with an empty scene.
 */
bool MainWindow::openFileProject(const QString& fileName)
{
//    InstanceNode* sceneInstance = m_modelScene->getInstance(ui->sceneView->rootIndex() );

//    InstanceNode* concentratorRoot = sceneInstance->children[sceneInstance->children.size() - 1];
//    m_modelSelection->setCurrentIndex(m_modelScene->indexFromUrl(concentratorRoot->getURL() ), QItemSelectionModel::ClearAndSelect);

//    ui->actionViewRays->setEnabled(false);
//    ui->actionViewRays->setChecked(false);

    m_undoStack->clear();
    m_modelScene->clear();
    m_graphicsRoot->removeScene();
    ui->parametersTabs->setNode(0);

    if (!fileName.isEmpty()) setSearchPaths(fileName);
    if (m_document->ReadFile(fileName))
    {
        showInStatusBar("File loaded");
        SetCurrentFile(fileName);
    }
    else
    {
        m_document->New();
        showInStatusBar("New file");
        SetCurrentFile("");
    }

    ChangeModelScene();
    if (fileName.isEmpty()) {
        m_graphicView[0]->onViewHome();
    }
    ui->sceneView->expandToDepth(1);


    m_graphicsRoot->deselectAll();
    m_graphicView[0]->render();
    m_document->getSceneKit()->updateTrackers(); //todo

//    Select("//Node"); // ?
//    on_actionViewAll_triggered(); // discard sun

//    GridNode* node = (GridNode*) m_document->getSceneKit()->getPart("world.terrain.grid", true);
//    if (node) ui->actionViewGrid->setChecked(node->show.getValue());
    return true;
}

void MainWindow::setDocumentModified(bool value)
{
    m_document->setModified(value);
    setWindowModified(value);
}

/*!
 * Computes the new light size to the scene current dimensions.
 */
void MainWindow::UpdateLightSize()
{
    TSceneKit* sceneKit = m_document->getSceneKit();
    SunKit* sunKit = (SunKit*) sceneKit->getPart("world.sun", false);
    if (!sunKit) return;

    sunKit->setBox(sceneKit);
    m_modelScene->UpdateSceneModel();
}
#include <QMimeData>

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    QString url = event->mimeData()->urls()[0].toLocalFile();

    if (OkToContinue())
        openFileProject(url);

    event->acceptProposedAction();
}

double findInterception(QString surface, uint rays, MainWindow* mw)
{
    if (!mw->m_rand)
        mw->m_rand = mw->m_pluginManager->getRandomFactories()[mw->m_raysRandomFactoryIndex]->create(0);

    FluxAnalysis fa(mw->m_document->getSceneKit(), mw->m_modelScene, mw->m_raysGridWidth, mw->m_raysGridHeight, mw->m_rand);
    fa.run(surface, "front", rays, false, 5, 5, true);
    return fa.powerTotal();
}

//#include "widgets/HelpDialog.h"
#include <QDesktopServices>
void MainWindow::on_actionHelpDocumentation_triggered()
{
    QDesktopServices::openUrl(QUrl("file:///" + qApp->applicationDirPath() + "/../help/html/index.html"));
//    HelpDialog dialog(this);
//    dialog.exec();
}

void MainWindow::on_actionSunPosition_triggered()
{
    SunCalculatorDialog dialog(this);
//    connect(
//        &dialog, SIGNAL(changeSunLight(double, double)),
//        this, SLOT(setAzEl(double, double))
//    );
    dialog.exec();
}

#include "updater/UpdateDialog.h"
void MainWindow::on_action_Updates_triggered()
{
    UpdateDialog dialog(this);
    dialog.exec();
}
