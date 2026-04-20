#pragma once

#include <QAbstractItemView>

class GraphicRoot;
class QModelIndex;
class SbViewportRegion;
class SoCamera;
class SoQtExaminerViewer;
//class ConstrainedViewer;
class TSeparatorKit;
class SoPerspectiveCamera;
class SoNodeSensor;
class SoSensor;
//!  GraphicView class implements a 3D representation of items from a model.
class QActionGroup;
class TPerspectiveCamera;
class QPushButton;


class GraphicView: public QFrame
{
    Q_OBJECT

public:
    GraphicView(QWidget* parent = 0);
    ~GraphicView();

    void setSceneGraph(GraphicRoot* sceneGraphRoot);

    SbViewportRegion getViewportRegion() const;
    SoCamera* getCamera() const;
    void showRays();
    void render();
    void syncSkyCamera();

public slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    void showContextMenu(QPoint);

    void onViewHome();
    void onViewSelected();
    void onViewAll();

    void on_actionViewX_triggered();
    void on_actionViewY_triggered();
    void on_actionViewZ_triggered();

    void on_actionViewSun_triggered();
    void on_actionViewSunFrom_triggered();
    void on_actionViewSunAnchoredTo_triggered();
    void on_actionViewSunAnchoredFrom_triggered();

    void on_actionViewGroup_triggered(QAction* action);
    void on_actionDrawSwitch_triggered();
    void onShowRays(bool on);
    void onShowPhotons(bool on);
    void hideMenu();

public:
    QObject* m_filter;

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void focusInEvent(QFocusEvent* event);
    void focusOutEvent(QFocusEvent* event);
    void resizeEvent(QResizeEvent* event);

//    void paintEvent(QPaintEvent* event);
    bool eventFilter(QObject *obj, QEvent *event);


    void initCursors();
    void initContextMenu();
    void setCameraView(double azimuth, double elevation, bool shift = false, bool alt = false);
    void setCameraViewTemp(double azimuth, double elevation);

private:
    static void onViewerCameraChanged(void* data, SoSensor*);

    GraphicRoot* m_graphicRoot;
    SoQtExaminerViewer* m_viewer;
    SoNodeSensor* m_viewerCameraSensor;

    QPoint m_mousePressed;
    Qt::KeyboardModifiers m_modifiersPressed;
    Qt::KeyboardModifiers m_modifiersKeys;
    TPerspectiveCamera* m_camera;
    QMap<QString, QCursor> m_cursors;

    QMenu* m_menu;

    QAction* m_actionViewHome;
    QAction* m_actionViewSelected;
    QAction* m_actionViewAll;

    QAction* actionViewX;
    QAction* actionViewY;
    QAction* actionViewZ;
    QAction* actionViewSun;
    QAction* actionViewSunFrom;
    QAction* actionViewSunAnchoredTo;
    QAction* actionViewSunAnchoredFrom;

    QActionGroup* actionViewGroup;
    QAction* actionDrawMaterial;
    QAction* actionDrawMaterialMesh;
    QAction* actionDrawMesh;
    QAction* actionDrawSwitch;

    QAction* actionShowRays;
    QAction* actionShowPhotons;
};
