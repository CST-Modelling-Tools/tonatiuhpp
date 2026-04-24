#pragma once

#include <QMainWindow>
#include <QVariant>
#include <QJSValue>

class QItemSelectionModel;
class QSplitter;
class CustomSplashScreen;

#include <Inventor/SbVec3f.h>

class Document;
class GraphicRoot;
class GraphicView;
class InstanceNode;
class PhotonsAbstract;
class PluginManager;
class QDir;
class QUndoStack;
class QUndoView;
class Random;
class SoDragger;
class SoSelection;
class SoSeparator;
class SoTransform;
class ComponentFactory;
class SunAperture;
class MaterialFactory;
class PhotonsBuffer;
class PhotonToMemory;
class ShapeFactory;
class SunShape;
class TrackerFactory;
class AirTransmission;
class SoCamera;
class SoGroup;
class SceneTreeModel;
struct PhotonsSettings;
class ProfileFactory;
class UndoView;
class NodeObject;
class TFactory;
class SoBaseKit;
class TNode;
class QTreeWidgetItem;
class TSceneKit;
class SoNode;

namespace Ui {
class MainWindow;
}

class MainWindow;
double findInterception(QString surface, uint rays, MainWindow* mw);

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString tonatiuhFile = "", CustomSplashScreen* splash = 0, QWidget* parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MainWindow();

    PluginManager* getPlugins() {return m_pluginManager;}
    TSceneKit* getSceneKit();
    void openFileScript(QString fileName);

    void StartManipulation(SoDragger* dragger);
    void FinishManipulation();

    //    void ExecuteScriptFile(QString fileName); // del?
friend double findInterception(QString surface, uint rays, MainWindow *mw);

public slots:
    void fileNew();
    void Clear();
    void fileOpen(QString fileName);
    bool fileSave();
    void fileSaveAs(QString fileName);

    void Copy();
    void Copy(QString url);
    void Cut();
    void Cut(QString url);
    void Delete();
    void Delete(QString url);
    void Paste(QString url, QString pasteType = "Shared");
    void PasteCopy();
    void PasteLink();

    void InsertNode();
    void InsertShape();
    void InsertTracker();
    void InsertArray();
    void nodeExport(QString fileName = "");
    void nodeImport(QString fileName = "");

    void InsertShapeSurface(QString shapeType, int numberOfParameters, QVector<QVariant> parametersList);
    void CreateComponentNode(QString componentType, QString nodeName, int numberofParameters, QVector<QVariant> parametersList);
    void InsertScene(QJSValue v);
    QJSValue getScene();

    QJSValue FindInterception(QJSValue surface, QJSValue rays,  QJSValue show = false);
    void Screenshot(QJSValue filename);

//    QJSValue getSceneKit()
    Q_INVOKABLE QJSValue print(QString text);
    Q_INVOKABLE QJSValue printTimed(QString text);
    QJSValue render();

    void Select(QString url);
    void SetName(QString name);
    void SetValue(QString url, QString parameter, QString value);

    void ChangeSunPosition(double azimuth, double elevation);
    void ChangeSunPosition(int year, int month, int day, double hours, double minutes, double seconds, double latitude, double longitude);

    void Run();
    void RunFluxAnalysis(QString nodeURL, QString surfaceSide, uint nOfRays, int heightDivisions, int widthDivisions, QString fileName, bool saveCoords);
    void SetExportAllPhotonMap();
    void SetExportCoordinates(bool enabled, bool global);
    void SetExportIntersectionSurface(bool enabled);
    void SetExportIntersectionSurfaceSide(bool enabled);
    void SetExportPhotonMapType(QString name);
    void SetExportPreviousNextPhotonID(bool enabled);
    void SetExportTypeParameterValue(QString parameterName, QString value);
    void AddExportSurfaceURL(QString nodeURL);
    void SetRaysNumber(uint rays) {m_raysNumber = rays;}
    void SetRaysScreen(uint rays) {m_raysScreen = rays;}
    void SetRaysRandomFactory(QString name);
    void SetRaysGrid(int width, int height);
    void SetPhotonBufferSize(uint size) {m_photonBufferSize = size;}
    void SetPhotonBufferAppend(bool on) {m_photonBufferAppend = on;}

private slots:
    void fileOpen();
    void fileOpenRecent();
    void showOpenRecent();
    void fileReopen();
    bool fileSaveAs();

    void ChangeNodeName(const QModelIndex& index, const QString& name);
    void ChangeSelection(const QModelIndex& index);
    void CreateComponent(ComponentFactory* factory);

//    void InsertShapeSurface(TFactory* f);
    void InsertShapeSurface(ShapeFactory* factory, int numberOfParameters, QVector<QVariant> parametersList);

    void onNodeDragAndDrop(const QModelIndex& indexParent, const QModelIndex& index, bool isMove = true);

    void onUndoStack();

    void RunCompleteRayTracer();
    void RunFluxAnalysisDialog();

    void SelectionFinish(SoSelection* selection);

    void setFieldText(SoNode* node, QString field, QString value);
    void setFieldNode(SoNode* node, QString field, SoNode* value);

    void ShowMenu(const QModelIndex& index);

    void onAbort(QString error);
    void showWarning(QString message);
    void showInStatusBar(QString message, int time = 2000);

    // manipulators actions
    void SoTransform_to_SoCenterballManip(); // move to view
    void SoTransform_to_SoJackManip();
    void SoTransform_to_SoHandleBoxManip();
    void SoTransform_to_SoTabBoxManip();
    void SoTransform_to_SoTrackballManip();
    void SoTransform_to_SoTransformBoxManip();
    void SoTransform_to_SoTransformerManip();
    void SoManip_to_SoTransform();

    // view actions
    void on_actionQuadView_toggled();
    void on_actionRunScript_triggered();

    // help actions
    void on_actionHelpDocumentation_triggered();
    void on_actionHelpExamples_triggered();
    void on_actionHelpScripts_triggered();
    void on_actionHelpAbout_triggered();

//    void on_actionViewAxes_toggled();

    void treeWorldClicked(QTreeWidgetItem* item, int column); // temporary
    void treeWorldDoubleClicked(QTreeWidgetItem* item, int column);
    void tabChanged(int);

    void on_actionSunPosition_triggered();

    void on_action_Updates_triggered();

signals:
    void Abort(QString error);

protected:
    void closeEvent(QCloseEvent* event);
//    void mousePressEvent(QMouseEvent* e);

private:   
    void ChangeModelScene();
    PhotonsAbstract* CreatePhotonMapExport() const;

    bool fileSave(const QString& fileName);
    void SetCurrentFile(const QString& filePath);
    bool OkToContinue();
    bool openFileProject(const QString& fileName);
    void setDocumentModified(bool value);

    bool Delete(QModelIndex index);
    bool Paste(QModelIndex index, bool isShared);

    bool ReadyForRaytracing(InstanceNode*& instanceLayout, InstanceNode* instanceSun, AirTransmission*& air);

    void SetupDocument();
    void SetupViews();
    void SetupGraphicView();
    void SetupTreeView();
    void SetupTriggers();

    void readSettings();
    void writeSettings();
    void updateRecentFiles();

    void onSunDialog();
    void onAirDialog();

    void ShowRaysIn3DView();
    void UpdateLightSize();

private:
    Ui::MainWindow* ui;
    PluginManager* m_pluginManager;

    QString m_fileName;
    static const int m_filesRecentMax = 8;
    QStringList m_filesRecent;

    QUndoStack* m_undoStack;
    UndoView* m_undoView;

    Document* m_document;
    GraphicRoot* m_graphicsRoot;
    SceneTreeModel* m_modelScene;
    QItemSelectionModel* m_modelSelection;

    QString m_lastExportFileName;
    QString m_lastExportSurfaceUrl;
    bool m_lastExportInGlobal;

    SoNode* m_clipboardNodes;

    ulong m_raysNumber;
    ulong m_raysScreen;
    int m_raysRandomFactoryIndex;
    int m_raysGridWidth;
    int m_raysGridHeight;

    ulong m_raysTracedTotal;
    Random* m_rand;

    PhotonsBuffer* m_photonsBuffer;
    ulong m_photonBufferSize;
    bool m_photonBufferAppend;
    PhotonsSettings* m_photonsSettings;

    QVector<GraphicView*> m_graphicView;
    int m_focusView;
    bool m_saveCoordinates;
    bool m_saveSide;

    // QWidget interface
protected:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
};
