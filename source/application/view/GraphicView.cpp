#include <iostream>
#include <QVariant>
#include <QVBoxLayout>
#include <QAction>
#include <QMenu>
#include <QActionGroup>

#include <Inventor/actions/SoBoxHighlightRenderAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/sensors/SoNodeSensor.h>
//#include <Inventor/Qt/viewers/SoQtConstrainedViewer.h>


//class ConstrainedViewer: public SoQtConstrainedViewer
//{
//    public:
//    ConstrainedViewer(QWidget* parent): SoQtConstrainedViewer(parent, "", TRUE, BUILD_ALL, BROWSER, TRUE) {}
//    ~ConstrainedViewer(){}
//};

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "KeyboardHook.h"
//    HHOOK m_hook;
//void MainWindow::setHook()
//{
//    //If parameter is NULL, GetModuleHandle returns a pseudo handle to the current process.
//    m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), NULL); // Ставим хук WH_KEYBOARD_LL
//    if (!m_hook)
//    { // Проверка на правильность установки хука
//        QMessageBox::critical(this, QString("Error in keyboard lock"),QString("Unable to set keyboard lock! [keyHook enable error]"), QMessageBox::Ok);
//        return;
//    }
//    //MessageBox(0,TEXT("Press OK for exit"),TEXT("OK"),MB_OK); // ждем пока надоест играться с хуком
//    //UnhookWindowsHookEx(hHook);
//    return;
//}

//void MainWindow::unsetHook()
//{
//    if (m_hook)
//    { // Если хук был установлен, снимемс его
//        UnhookWindowsHookEx(m_hook);
//        return;
//    }
//}

#include "GraphicRoot.h"
#include "GraphicView.h"
#include "tree/SoPathVariant.h"
#include "kernel/scene/TPerspectiveCamera.h"
#include "KeyFilter.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/sun/SunPosition.h"
#include <QLabel>
#include <QPushButton>
//#include <QGLWidget>
#include <QDebug>
#include <QWindow>

//SbVec2f functionClip(void* data, const SbVec2f& nearfar)
//{
//    qDebug() << nearfar[0] << " " << nearfar[1];
//    return SbVec2f(0.1, 10);
//    return nearfar;
//}

/**
 * Creates a new GraphicView with a \a parerent for the model data 3D representation.
 *
 * Use setModel() to set the model.
 */
GraphicView::GraphicView(QWidget* parent):
    QFrame(parent),
    m_graphicRoot(0),
    m_viewerCameraSensor(nullptr)
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    QWidget* w = new QWidget;
    layout->addWidget(w);

    m_viewer = new SoQtExaminerViewer(w);
    m_viewer->setAntialiasing(true, 1); // disable if slow
    m_viewer->setViewing(false); // viewing or editing

    SoBoxHighlightRenderAction* highlighter = new SoBoxHighlightRenderAction(m_viewer->getViewportRegion());
    highlighter->setColor(SbColor(100/255., 180/255., 120/255.));
    highlighter->setLineWidth(2.);
    m_viewer->setGLRenderAction(highlighter);
//    m_viewer->setWireframeOverlayColor(SbColor(96/255., 123/255., 155/255.)); // overriden

    m_viewer->setDrawStyle(SoQtViewer::INTERACTIVE, SoQtViewer::VIEW_SAME_AS_STILL);
    m_viewer->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND); // do not move
    m_viewer->setFeedbackVisibility(true); // show frame axes in the corner
    m_viewer->setDecoration(false); // hide controls on the sides
    m_viewer->setHeadlight(false); // camera light

//    m_viewer->setAutoClipping(true);
//    m_viewer->setAutoClippingStrategy(SoQtViewer::CONSTANT_NEAR_PLANE, 0.1, functionClip);

    m_camera = new TPerspectiveCamera;

    // hud menu
//    QGLWidget* gw = () m_viewer->getGLWidget();
//    QPushButton* label  = new QPushButton("asdvasdv", w);
//    m_label  = new QPushButton("asdvasdv", w);
//    label->setGeometry(QRect(0, 0, 504, 14));
//    connect(m_viewer->getGLWidget(),SIGNAL()

    // do not propogate key and mouse events
//    w->setEnabled(false);
//    w->setFocusPolicy(Qt::NoFocus);
//    m_viewer->getGLWidget()->setFocusPolicy(Qt::NoFocus);
//    m_viewer->getGLWidget()->setEnabled(false);
//m_viewer->getGLWidget()->setFocusPolicy(Qt::StrongFocus);
QWindow * hw  = m_viewer->getGLWidget()->property("SoQtGLArea").value<QWindow*>();
hw->installEventFilter(this);
setFocusPolicy(Qt::StrongFocus);

m_viewer->getGLWidget()->setFocusPolicy(Qt::NoFocus);

//m_viewer->getBaseWidget()->setFocusPolicy(Qt::NoFocus);
//m_viewer->getParentWidget()->setFocusPolicy(Qt::NoFocus);
//m_viewer->getShellWidget()->setFocusPolicy(Qt::NoFocus);
//m_viewer->getWidget()->setFocusPolicy(Qt::NoFocus);

//m_viewer->getOverlayWidget()->setFocusPolicy(Qt::NoFocus);
//    w->setFocusPolicy(Qt::StrongFocus);
//    m_filter = new KeyFilter(this);
    m_modifiersKeys = Qt::NoModifier; //delete


    initCursors();
    initContextMenu();

    setStyleSheet(R"(
QFrame {
    border: 1px solid lightgray;
}

QFrame[inFocus=true] {
    border: 1px solid #828790;
}
    )");
}

GraphicView::~GraphicView()
{
    delete m_viewerCameraSensor;
    delete m_viewer;
    delete m_camera;
}

#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodekits/SoCameraKit.h>
#include "kernel/scene/TCameraKit.h"
void GraphicView::setSceneGraph(GraphicRoot* sceneGraphRoot)
{
//    m_viewer = new ExaminerViewer2(this);
    m_graphicRoot = sceneGraphRoot;
    m_viewer->setSceneGraph(m_graphicRoot->getRoot());
    SoPerspectiveCamera* camera = (SoPerspectiveCamera*) m_viewer->getCamera();

    m_camera->setCamera(camera);

    if (!m_viewerCameraSensor)
        m_viewerCameraSensor = new SoNodeSensor(onViewerCameraChanged, this);
    m_viewerCameraSensor->attach(camera);
    syncSkyCamera();

    TCameraKit* cameraKit = (TCameraKit*) m_graphicRoot->getScene()->getPart("world.camera", true);
    cameraKit->setCamera(m_camera);
//    m_cameraKit = m_graphicRoot->getCameraKit();
}

SbViewportRegion GraphicView::getViewportRegion() const
{
    return SbViewportRegion(); //?
    return m_viewer->getViewportRegion();
}

SoCamera* GraphicView::getCamera() const
{
    return m_viewer->getCamera();
}

void GraphicView::showRays()
{
    m_graphicRoot->showRays(actionShowRays->isChecked());
    m_graphicRoot->showPhotons(actionShowPhotons->isChecked());
}

void GraphicView::render()
{
    m_viewer->render();
}

void GraphicView::syncSkyCamera()
{
    if (!m_graphicRoot || !m_camera || !m_camera->camera())
        return;

    m_graphicRoot->updateSkyCamera(m_camera->camera());
}

void GraphicView::onViewerCameraChanged(void* data, SoSensor*)
{
    GraphicView* view = static_cast<GraphicView*>(data);
    if (!view)
        return;

    view->syncSkyCamera();
}

#include "tree/SceneTreeModel.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/scene/TSeparatorKit.h"
void GraphicView::currentChanged(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    if (!m_graphicRoot) return;
    m_graphicRoot->deselectAll();

    QVariant variant = current.data(Qt::UserRole);
    if (!variant.canConvert<SoPathVariant>()) return;
    SoPath* path = variant.value<SoPathVariant>().getPath();
    {
        SceneTreeModel* model = (SceneTreeModel*) current.model();
        if (!model) return;
        InstanceNode* instance = model->getInstance(current);
        if (!instance) return;
        SoNode* node = instance->getNode();
//        if (!node->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId())) return;
        if (!node->getTypeId().isDerivedFrom(SoBaseKit::getClassTypeId()))
        {
            path->unref();
            return;
        }
        if (!m_viewer->isViewing())
            m_graphicRoot->select(path);
//        else
//            m_graphicRoot->deselectAll();
        path->unref();
    }

    //    m_sceneGraphRoot->touch();
}

#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include "kernel/scene/TCameraKit.h"
#include <Inventor/nodes/SoTransform.h>
#include "libraries/math/gcf.h"
#include "view/OverlayNode.h"

void GraphicView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        showContextMenu(event->pos());
        return;
    }

//    qDebug() << "pressed" << event->localPos();
    m_mousePressed = event->pos();
    m_modifiersPressed = event->modifiers();
    m_camera->saveTransform();

//    if (!hasFocus()) {
        QKeyEvent ev(QEvent::KeyPress, 0, m_modifiersPressed);
        keyPressEvent(&ev);
//    }

    if (m_modifiersPressed & Qt::AltModifier)
    {
//        SbVec2s vs = m_viewer->getViewportRegion().getViewportSizePixels();
        m_camera->findMoveAnchor(m_viewer, QPoint(width()/2, height()/2), m_graphicRoot->getScene()->getLayout());
        if (m_modifiersPressed & Qt::ShiftModifier) {
            qApp->changeOverrideCursor(m_cursors["orbitB"]);
            m_camera->findOrbitAnchor(m_viewer, m_mousePressed, m_graphicRoot->getScene()->getLayout());
        }
    }
    else if (m_modifiersPressed & Qt::ControlModifier)
    {
        if (m_modifiersPressed & Qt::ShiftModifier) {
            qApp->changeOverrideCursor(m_cursors["rotationB"]);
            m_camera->findRotationAnchor(m_viewer, m_mousePressed);
        }
    }
    else if (m_modifiersPressed & Qt::ShiftModifier)
    {
        qApp->changeOverrideCursor(m_cursors["shiftB"]);
        m_camera->findMoveAnchor(m_viewer, m_mousePressed, m_graphicRoot->getScene()->getLayout());
    }
    event->accept();
}

void GraphicView::mouseReleaseEvent(QMouseEvent* event)
{
//    qDebug() << "release" << event->pos();
    if (m_modifiersPressed & Qt::AltModifier)
    {
        if (m_modifiersPressed & Qt::ShiftModifier) {
            qApp->changeOverrideCursor(m_cursors["orbitA"]);
        }
    }
    else if (m_modifiersPressed & Qt::ControlModifier)
    {
        if (m_modifiersPressed & Qt::ShiftModifier) {
            qApp->changeOverrideCursor(m_cursors["rotationA"]);
        }
    }
    else if (m_modifiersPressed & Qt::ShiftModifier)
    {
        qApp->changeOverrideCursor(m_cursors["shiftA"]);
    }
    m_modifiersPressed = Qt::NoModifier;
    event->accept();
}

void GraphicView::mouseMoveEvent(QMouseEvent* event)
{
//    qDebug() << "move " << diff;
    if (m_modifiersPressed & Qt::AltModifier)
    {
        if (m_modifiersPressed & Qt::ShiftModifier) {
             m_camera->moveOrbitAnchor(m_viewer, event->pos());
        } else {
            QPoint diff = event->pos() - m_mousePressed;
            double s = 0.25;
            double dAz = s*diff.x();
            double dEl = -s*diff.y();
            m_camera->orbit(dAz, dEl);
        }
    }
    else if (m_modifiersPressed & Qt::ControlModifier)
    {
        if (m_modifiersPressed & Qt::ShiftModifier) {
            m_camera->moveRotationAnchor(m_viewer, event->pos());
        } else {
            QPoint diff = event->pos() - m_mousePressed;
            double s = 0.25;
            double dAz = s*diff.x();
            double dEl = -s*diff.y();
            m_camera->rotate(dAz, dEl);
        }
    }
    else if (m_modifiersPressed & Qt::ShiftModifier)
    {
    #ifdef __linux__
       m_camera->movePanAnchor(m_viewer, event->pos());
    #elif _WIN32
       if (GetKeyState(VK_LSHIFT) < 0) // left pressed
           m_camera->movePanAnchor(m_viewer, event->pos());
       else
           m_camera->moveShiftAnchor(m_viewer, event->pos());
    #endif
    }

    //    m_viewer->render();
    event->accept();
}

void GraphicView::wheelEvent(QWheelEvent* event)
{
    if (!event->modifiers()) {
        double z = event->angleDelta().y()/120.; // 15 degrees * 8 units
        m_camera->saveTransform();
        m_camera->findMoveAnchor(m_viewer, event->position(), m_graphicRoot->getScene()->getLayout());
        m_camera->moveShiftAnchor(m_viewer, event->position(), z);
    } else if (event->modifiers() & Qt::AltModifier) {
        double z = event->angleDelta().x()/120.;
        m_camera->zoomCenter(m_viewer, m_graphicRoot->getScene()->getLayout(), z);
    } else if (event->modifiers() & Qt::ControlModifier)  {
        double z = event->angleDelta().y()/120.;
        m_camera->zoomCenter(m_viewer, m_graphicRoot->getScene()->getLayout(), z);
    } else if (event->modifiers() & Qt::ShiftModifier)  {//recenter
        double z = event->angleDelta().y()/120.;
//        m_camera->zoomCenter(m_viewer, m_graphicRoot->getScene()->getLayout(), z);
        m_camera->saveTransform();
        m_camera->moveCameraPlane(m_viewer, event->position(), m_graphicRoot->getScene()->getLayout(),z);
        cursor().setPos(mapToGlobal(geometry().center()));
    }
    event->accept();
}

void GraphicView::keyPressEvent(QKeyEvent* event)
{
    QString nc;
    if (event->modifiers() & Qt::AltModifier) {
        nc = "orbit";
        if (event->modifiers() & Qt::ShiftModifier)
            nc += "A";
    } else if (event->modifiers() & Qt::ControlModifier) {
        nc = "rotation";
        if (event->modifiers() & Qt::ShiftModifier)
            nc += "A";
    } else if (event->modifiers() & Qt::ShiftModifier)
        nc = "shiftA";

    if (!nc.isEmpty()) {
        m_graphicRoot->enableSelection(false);
        QCursor& cursor = m_cursors[nc];
//        setCursor(); // coin overrides?
//        unsetCursor();
        if (qApp->overrideCursor())
            qApp->changeOverrideCursor(cursor);
        else
            qApp->setOverrideCursor(cursor);
        m_graphicRoot->getOverlayNode()->showAim(true);
        m_viewer->render();
    } else {
        m_graphicRoot->enableSelection(true);
        qApp->restoreOverrideCursor();
        m_graphicRoot->getOverlayNode()->showAim(false);
        m_viewer->render();
    }
//    QFrame::keyPressEvent(event);
    event->accept();
}

void GraphicView::keyReleaseEvent(QKeyEvent* event)
{
    keyPressEvent(event);
    event->accept();
//    QFrame::keyReleaseEvent(event);
}

#include <QApplication>
#include "MenuStyle.h"
#include <QDebug>
#include <QMainWindow>
#include <QMenuBar>

#include "main/MainWindow.h"
void GraphicView::focusInEvent(QFocusEvent* /*event*/)
{
    QWindow * hw  = m_viewer->getGLWidget()->property("SoQtGLArea").value<QWindow*>();
    hw->requestActivate();

//    MainWindow* mw = (MainWindow*) m_window;
//    if (mw) {
//            mw->menuBar()->installEventFilter(m_filter);
//       qApp->removeEventFilter(mw->menuBar());
//    }

//
//    ((QMainWindow*)m_window)->menuBar();
//    qApp->removeEventFilter();
//    if (m_window)
//        m_window->installEventFilter(m_filter);
//    qApp->installEventFilter(m_filter);
//    installEventFilter(m_filter);

    setProperty("inFocus", true);
    style()->unpolish(this);
    update();
//    qDebug() << "foc in";

//        setHook();
//    qDebug() << "disa";


//    MenuStyle* ms = dynamic_cast<MenuStyle*>( m_window->style());
//    if (ms)
//    {
//        qApp->installEventFilter(this);
//        QMainWindow* mw = (QMainWindow*) m_window;
////        mw->menuBar()->installEventFilter(this);
////        ms->unpolish(m_window);
////      ms->m_useAlt = false;
////      ms->polish(m_window);
////      m_window->update();

//    }

//    MenuStyle* ms = (MenuStyle*) qApp->style();
//    ms->unpolish(qApp);
//        qDebug() << ms->m_useAlt;
////    ms->m_useAlt = true;
//    ms->polish(qApp);
//    qApp->setStyle(new QProxyStyle);
//    QWidget::focusInEvent(event);
}

void GraphicView::focusOutEvent(QFocusEvent* /*event*/)
{
//        qApp->installEventFilter(((QMainWindow*)m_window)->menuBar());
    setProperty("inFocus", false);
    style()->unpolish(this);
    update();

    qApp->restoreOverrideCursor();
//  qDebug() << "foc out";
//    qDebug() << "ena";
//    if (m_window)
//        qApp->removeEventFilter(m_filter);

//    MenuStyle* ms = dynamic_cast<MenuStyle*> (m_window->style());
//    if (ms)
//    {
//        ms->unpolish(m_window);
//      ms->m_useAlt = true;
//      ms->polish(m_window);
//    }
//
//        qApp->setStyle(new MenuStyle);
//    QWidget::focusOutEvent(event);
}

//#include <QDesktopWidget>
#include <QScreen>
void GraphicView::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    int hMax = qApp->screens()[0]->geometry().height();
//    SbVec2s vs = m_viewer->getViewportRegion().getViewportSizePixels();
//    int h = std::min(vs[0], vs[1]);
    QSize size = event->size();
    int h = std::min(size.width(), size.height());
    m_camera->camera()->heightAngle = 30*gcf::degree*h/hMax;
    syncSkyCamera();
    m_camera->setWindowSize(size);
}

bool GraphicView::eventFilter(QObject* /*obj*/, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonRelease ||
        event->type() == QEvent::MouseMove ||
        event->type() == QEvent::Wheel
    ) {
        QApplication::sendEvent(this, event);
        return false;
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = (QKeyEvent*) event;
//        qDebug() <<  event->type()<< " " << ke->key();
        keyPressEvent(ke);
        return true;
    } else if (event->type() == QEvent::KeyRelease) {
        QKeyEvent* ke = (QKeyEvent*) event;
//        qDebug() <<  event->type()<< " " << ke->key();
        keyReleaseEvent(ke);
        return true;
    } else if (event->type() == QEvent::FocusIn) {
//        qDebug() << event->type();
        QFocusEvent* ke = (QFocusEvent*)event;
        focusInEvent(ke);
        return true;

    } else if (event->type() == QEvent::FocusOut) {
//        qDebug() << event->type();
        QFocusEvent* ke = (QFocusEvent*)event;
        focusOutEvent(ke);
        return true;
    }

    return false;
//    return obj->eventFilter(obj, event);
}

void GraphicView::initCursors()
{
    QStringList cursorNames = {
        "ShiftA", "ShiftB",
        "Rotation", "RotationA", "RotationB",
        "Orbit", "OrbitA", "OrbitB"
    };

    QPixmap pixmap;
    for (QString cn : cursorNames) {
        pixmap.load(QString(":/images/cursors/cursor%1.png").arg(cn));
        pixmap = pixmap.scaledToWidth(48, Qt::SmoothTransformation);
        pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
        cn[0] = cn[0].toLower(); // redo
        m_cursors[cn] = QCursor(pixmap);
    }
}

void GraphicView::initContextMenu()
{
//    setContextMenuPolicy(Qt::CustomContextMenu);
//    connect(
//        this, SIGNAL(customContextMenuRequested(QPoint)),
//        this, SLOT(showContextMenu(QPoint))
//    );

    m_menu = new QMenu(this);
//    connect(
//        m_menu, SIGNAL(aboutToHide()),
//        this, SLOT(hideMenu())
//    );

    // menu camera
    QMenu* menuCamera = new QMenu("Camera", this);
    menuCamera->setIcon(QIcon(":/images/scene/nodeCamera.png"));
    m_menu->addMenu(menuCamera);

    m_actionViewHome = new QAction("Default", this);
    m_actionViewHome->setShortcut(QKeySequence("Ctrl+,"));
    connect(
        m_actionViewHome, SIGNAL(triggered()),
        this, SLOT(onViewHome())
    );
    menuCamera->addAction(m_actionViewHome);

    m_actionViewSelected = new QAction("Selected", this);
    m_actionViewSelected->setShortcut(QKeySequence("Ctrl+."));
    connect(
        m_actionViewSelected, SIGNAL(triggered()),
        this, SLOT(onViewSelected())
    );
    menuCamera->addAction(m_actionViewSelected);

    m_actionViewAll = new QAction("All", this);
    m_actionViewAll->setShortcut(QKeySequence("Ctrl+/"));
    connect(
        m_actionViewAll, SIGNAL(triggered()),
        this, SLOT(onViewAll())
    );
    menuCamera->addAction(m_actionViewAll);

    menuCamera->addSeparator();

    actionViewX = new QAction("East", this);
    actionViewX->setObjectName("actionViewX");
    actionViewX->setIcon(QIcon(":/images/view/viewX.png"));
    actionViewX->setShortcuts(QKeySequence::listFromString(
        "Ctrl+3; Ctrl+Shift+3; Ctrl+Alt+3; Ctrl+Alt+Shift+3"));
    menuCamera->addAction(actionViewX);

    actionViewY = new QAction("North", this);
    actionViewY->setObjectName("actionViewY");
    actionViewY->setIcon(QIcon(":/images/view/viewY.png"));
    actionViewY->setShortcuts(QKeySequence::listFromString(
        "Ctrl+7; Ctrl+Shift+7; Ctrl+Alt+7; Ctrl+Alt+Shift+7"));
    menuCamera->addAction(actionViewY);

    actionViewZ = new QAction("Zenith", this);
    actionViewZ->setObjectName("actionViewZ");
    actionViewZ->setIcon(QIcon(":/images/view/viewZ.png"));
    actionViewZ->setShortcuts(QKeySequence::listFromString(
        "Ctrl+1; Ctrl+Shift+1; Ctrl+Alt+1; Ctrl+Alt+Shift+1"));
    menuCamera->addAction(actionViewZ);

    menuCamera->addSeparator();

//    actionViewSun = new QAction(QIcon(":/images/view/viewSun.png"), "to Sun", this);
    actionViewSun = new QAction("Sun (to)", this);
    actionViewSun->setObjectName("actionViewSun");
    actionViewSun->setIcon(QIcon(":/images/view/viewSun.png"));
    actionViewSun->setShortcuts(QKeySequence::listFromString(
        "Ctrl+9")); // 0?
    menuCamera->addAction(actionViewSun);

    actionViewSunFrom = new QAction("Sun (from)", this);
    actionViewSunFrom->setObjectName("actionViewSunFrom");
    actionViewSunFrom->setIcon(QIcon(":/images/view/viewSunFrom.png"));
    actionViewSunFrom->setShortcut(QKeySequence("Ctrl+Shift+9"));
    menuCamera->addAction(actionViewSunFrom);

    actionViewSunAnchoredTo = new QAction("Sun (to, anchored)", this);
    actionViewSunAnchoredTo->setObjectName("actionViewSunAnchoredTo");
    actionViewSunAnchoredTo->setIcon(QIcon(":/images/view/viewSun.png"));
    actionViewSunAnchoredTo->setShortcut(QKeySequence("Ctrl+Alt+9"));
    menuCamera->addAction(actionViewSunAnchoredTo);

    actionViewSunAnchoredFrom = new QAction("Sun (from, anchored)", this);
    actionViewSunAnchoredFrom->setObjectName("actionViewSunAnchoredFrom");
    actionViewSunAnchoredFrom->setIcon(QIcon(":/images/view/viewSunFrom.png"));
    actionViewSunAnchoredFrom->setShortcut(QKeySequence("Ctrl+Alt+Shift+9"));
    menuCamera->addAction(actionViewSunAnchoredFrom);


    // menu show
    QMenu* menuShow = new QMenu("Show", this);
    m_menu->addMenu(menuShow);

    actionShowRays = new QAction("Rays", this);
    actionShowRays->setCheckable(true);
    actionShowRays->setChecked(true);
    actionShowRays->setShortcut(QKeySequence("Ctrl+E,R"));
    connect(
        actionShowRays, SIGNAL(triggered(bool)),
        this, SLOT(onShowRays(bool))
    );
    menuShow->addAction(actionShowRays);

    actionShowPhotons = new QAction("Photons", this);
    actionShowPhotons->setCheckable(true);
    actionShowPhotons->setChecked(false);
    actionShowPhotons->setShortcut(QKeySequence("Ctrl+E,P"));
    connect(
        actionShowPhotons, SIGNAL(triggered(bool)),
        this, SLOT(onShowPhotons(bool))
    );
    menuShow->addAction(actionShowPhotons);


    // menu rendering
    QMenu* menuRendering = new QMenu("Rendering", this);
    m_menu->addMenu(menuRendering);

    actionDrawMesh = new QAction("Mesh", this);
    actionDrawMesh->setCheckable(true);

    actionDrawMaterial = new QAction("Material", this);
    actionDrawMaterial->setCheckable(true);
    actionDrawMaterial->setChecked(true);

    actionDrawMaterialMesh = new QAction("Material + Mesh", this);
    actionDrawMaterialMesh->setCheckable(true);

    actionDrawSwitch = new QAction("actionDrawSwitch", this);
    actionDrawSwitch->setObjectName("actionDrawSwitch");
    actionDrawSwitch->setShortcut(QKeySequence("Ctrl+Tab"));

    actionViewGroup = new QActionGroup(this);
    actionViewGroup->setObjectName("actionViewGroup");
    actionViewGroup->addAction(actionDrawMesh);
    actionViewGroup->addAction(actionDrawMaterial);
    actionViewGroup->addAction(actionDrawMaterialMesh);
//    actionViewGroup->setExclusive(false);

    menuRendering->addActions(actionViewGroup->actions());

    addActions(m_menu->actions()); // for shortcuts
    addAction(actionDrawSwitch); //?
    QMetaObject::connectSlotsByName(this);
}

void GraphicView::showContextMenu(QPoint pos)
{
    m_menu->popup(mapToGlobal(pos));
}

//#include <QPainter>
//void GraphicView::paintEvent(QPaintEvent* event)
//{
//    QPainter painter(this);
//    painter.drawText(0, 20 ," XVXV");
//    qDebug()<<"painter";
//}

void GraphicView::onViewHome()
{
    double gamma = 10;
    double alpha = -10.;
    vec3d d = vec3d::directionAE(gamma*gcf::degree, alpha*gcf::degree);
    d *= 2./d.z;   // look at origin from 2 m height

//    m_camera->m_position = d;
//    m_camera->setRotation(gamma, alpha);

    TCameraKit* cameraKit = (TCameraKit*) m_graphicRoot->getScene()->getPart("world.camera", true);
    cameraKit->position.setValue(d[0], d[1], d[2]);
    cameraKit->rotation.setValue(gamma, alpha);
}

void GraphicView::onViewSelected()
{
//    QModelIndex index = ui->sceneView->currentIndex();
//    InstanceNode* instance = m_modelScene->getInstance(index);
//    SoNode* node = instance->getNode();
//    TSeparatorKit* kit = dynamic_cast<TSeparatorKit*>(node);
//    if (kit) node = kit->getPart("group", false);

//    SoSearchAction actionSearch;
//    actionSearch.setNode(node);
//    actionSearch.apply(m_document->getSceneKit()->getLayout());
//    SoPath* path = actionSearch.getPath();

    int c = m_graphicRoot->getSelection()->getNumSelected();
    if (c == 0) return;
    SoPath* path = m_graphicRoot->getSelection()->getPath(0); // without nodekits

    SoFullPath* kp = (SoFullPath*) path; // includes nodekits

    SoNode* tail = kp->getTail();
//    qDebug() << "tail " <<tail->getName().getString();
    if (tail->getTypeId() == TSeparatorKit::getClassTypeId()) {
        TSeparatorKit* sk = (TSeparatorKit*) tail;
        SoNode* node = ((SoSFNode*)sk->getField("topSeparator"))->getValue();
        path->append(node);
        node = sk->getPart("transform", false);
        path->append(node);
//        qDebug() << "added";
    }
//    for (int q = 0; q < kp->getLength(); ++q) {
//        qDebug() << q << " " << kp->getNode(q)->getName().getString() << " " << kp->getNode(q)->getTypeId().getName().getString();
//    }

    SbViewportRegion vpr;
    SoGetMatrixAction actionMatrix(vpr);
    actionMatrix.apply(path);
    SbMatrix matrix = actionMatrix.getMatrix().getValue();

    SbVec3f vL(0., 0., 0.);
    SbVec3f vG;
    matrix.multVecMatrix(vL, vG);

//    SbVec3f qq, ww;
//    SbRotation r1, r2;
//    matrix.getTransform(qq,r1,ww,r2);
//    qDebug() << qq.toString().getString();
//    qDebug() << vG.toString().getString();

//    SoCamera* camera = m_viewer->getCamera();
//    camera->pointAt(vG, SbVec3f(0., 0., 1.));

//    SbVec3f pos = camera->position.getValue();
//    m_camera->m_position = vec3d(pos[0], pos[1], pos[2]);

//    SbRotation q = camera->orientation.getValue();
//    SbVec3f d(0, 0, -1);
//    q.multVec(d, d);
//    qDebug() << d.toString().getString();
//    double alpha = asin(d[2])/gcf::degree;
//    double gamma = atan2(d[0], d[1])/gcf::degree;
//    m_camera->m_rotation = vec3d(gamma, alpha, 0);

    m_camera->setRotationAt(tgf::makeVector3D(vG));
}

void GraphicView::onViewAll()
{
    SbViewportRegion vpr = m_viewer->getViewportRegion();
    SoCamera* camera = m_viewer->getCamera();
    camera->viewAll(m_graphicRoot->getScene()->getLayout(), vpr);

    SbVec3f pos = camera->position.getValue();
    m_camera->m_position = vec3d(pos[0], pos[1], pos[2]);

    SbRotation q = camera->orientation.getValue();
    SbVec3f d(0, 0, -1);
    q.multVec(d, d);

    double alpha = asin(d[2])/gcf::degree;
    double gamma = atan2(d[0], d[1])/gcf::degree;
    m_camera->setRotation(gamma, alpha);
}

void GraphicView::on_actionViewSun_triggered()
{
    SunPosition* sp = (SunPosition*) m_graphicRoot->getScene()->getPart("world.sun.position", false);
    double gamma = sp->azimuth.getValue();
    double alpha = sp->elevation.getValue();
    setCameraViewTemp(gamma, alpha);
}

void GraphicView::on_actionViewSunFrom_triggered()
{
    SunPosition* sp = (SunPosition*) m_graphicRoot->getScene()->getPart("world.sun.position", false);
    double gamma = sp->azimuth.getValue();
    double alpha = sp->elevation.getValue();
    setCameraView(gamma, alpha, true, false);
}

void GraphicView::on_actionViewSunAnchoredTo_triggered()
{
    SunPosition* sp = (SunPosition*) m_graphicRoot->getScene()->getPart("world.sun.position", false);
    double gamma = sp->azimuth.getValue();
    double alpha = sp->elevation.getValue();
    setCameraView(gamma, alpha, false, true);
}

void GraphicView::on_actionViewSunAnchoredFrom_triggered()
{
    SunPosition* sp = (SunPosition*) m_graphicRoot->getScene()->getPart("world.sun.position", false);
    double gamma = sp->azimuth.getValue();
    double alpha = sp->elevation.getValue();
    setCameraView(gamma, alpha, true, true);
}

void GraphicView::on_actionViewX_triggered()
{
    setCameraViewTemp(90., 0.);
}

void GraphicView::on_actionViewY_triggered()
{
    setCameraViewTemp(0., 0.);
}

void GraphicView::on_actionViewZ_triggered()
{
    setCameraViewTemp(180., 90.);
}

void GraphicView::setCameraView(double azimuth, double elevation, bool shift, bool alt)
{
    if (shift) {
        azimuth = azimuth - 180.;
        elevation = -elevation;
    }

    if (alt) {
        vec3d anchor;
        bool isAnchored = m_camera->findAnchor0(m_viewer, m_graphicRoot->getScene()->getLayout(), anchor);
        if (isAnchored) {
            m_camera->setOrbit(azimuth, elevation, anchor);
            return;
        }
    }
    m_camera->setRotation(azimuth, elevation);
}

void GraphicView::setCameraViewTemp(double azimuth, double elevation)
{
#ifdef __linux__

#elif _WIN32
    bool shift = GetKeyState(VK_SHIFT) < 0;
    bool alt = GetKeyState(VK_MENU) < 0;
    setCameraView(azimuth, elevation, shift, alt);
#endif
}

void GraphicView::on_actionViewGroup_triggered(QAction* action)
{
    if (action == actionDrawMaterial)
        m_graphicRoot->setDrawStyle(true, false);
    else if (action == actionDrawMesh)
        m_graphicRoot->setDrawStyle(false, true);
    else if (action == actionDrawMaterialMesh)
        m_graphicRoot->setDrawStyle(true, true);

    m_viewer->render();
}

void GraphicView::on_actionDrawSwitch_triggered()
{
    int s = actionViewGroup->actions().indexOf(actionViewGroup->checkedAction());
    s++;
    if (s == actionViewGroup->actions().size()) s = 0;
    actionViewGroup->actions()[s]->trigger();
}

void GraphicView::onShowRays(bool on)
{
     m_graphicRoot->showRays(on);
}

void GraphicView::onShowPhotons(bool on)
{
    m_graphicRoot->showPhotons(on);
}

void GraphicView::hideMenu()
{
//    QWindow * hw  = m_viewer->getGLWidget()->property("SoQtGLArea").value<QWindow*>();
//    hw->requestActivate();
}

//void MainWindow::showGrid() //?
//{
//    GridNode* node = (GridNode*) m_document->getSceneKit()->getPart("world.terrain.grid", false);
//    if (node) node->show = ui->actionViewGrid->isChecked();
//}
