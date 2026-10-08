#include "FluxAnalysis.h"

#include <cmath>
#include <vector>

#include <QFileDialog>
#include <QFutureWatcher>
#include <QDebug>
#include <QPair>
#include <QProgressDialog>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoGroup.h>

#include "tree/SceneTreeModel.h"
#include "core/RayTraceExecutor.h"
#include "core/TracePreparation.h"
#include "kernel/air/AirTransmission.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunPosition.h"
#include "kernel/sun/SunAperture.h"
#include "libraries/math/gcf.h"
#include "kernel/profiles/ProfileRT.h"
#include "libraries/math/2D/Matrix2D.h"
#include "kernel/photons/Photon.h"
#include <QCoreApplication>

FluxAnalysis::FluxAnalysis(TSceneKit* sceneKit,
    SceneTreeModel* sceneModel,
    int sunWidthDivisions,
    int sunHeightDivisions,
    std::uint64_t masterSeed
):
    QObject(0),
    m_sceneKit(sceneKit),
    m_sceneModel(sceneModel),
    m_sunDivs(sunWidthDivisions, sunHeightDivisions),
    m_masterSeed(masterSeed),
    m_photons(0),
    m_surfaceURL(""),
    m_tracedRays(0),
    m_powerTotal(0.),
    m_powerPhoton(0.),
    m_photonsMax(0),
    m_photonsError(0)
{
    m_instanceLayout = m_sceneModel->getInstance(QModelIndex());
    m_instanceLayout = m_instanceLayout->children[0];
}

FluxAnalysis::~FluxAnalysis()
{
    stop();
    clear();
}

/*
 * Returns the type of the surface
 */
QString FluxAnalysis::getShapeType(QString nodeURL)
{
    QModelIndex index = m_sceneModel->indexFromUrl(nodeURL);
    if (!index.isValid()) return "";

    InstanceNode* instance = m_sceneModel->getInstance(index);
    if (!instance) return "";

    TShapeKit* shapeKit = static_cast<TShapeKit*>(instance->getNode());
    if (!shapeKit) return "";

    ShapeRT* shape = (ShapeRT*) shapeKit->shapeRT.getValue();
    if (!shape) return "";

    return shape->getTypeName();
}

/*
 * Fun flux analysis
 */
void FluxAnalysis::run(QString nodeURL, QString surfaceSide, ulong nRays, bool photonBufferAppend, int uDivs, int vDivs, bool /*silent*/)
{
    m_surfaceURL = nodeURL;
    m_surfaceSide = surfaceSide;

    m_binsPhotons.resize(uDivs, vDivs);

    if (!m_sceneKit) return;

    AirTransmission* air = dynamic_cast<AirTransmission*>(m_sceneKit->getPart("world.air.transmission", false));

    if (!m_instanceLayout) return;
    InstanceNode* instanceScene = m_instanceLayout->getParent();
    if (!instanceScene) return;

    SunKit* sunKit = static_cast<SunKit*>(m_sceneKit->getPart("world.sun", false));
    InstanceNode instanceSun(sunKit);

    SunPosition* sunPosition = (SunPosition*) sunKit->getPart("position", false);
    SunAperture* sunAperture = (SunAperture*) sunKit->getPart("aperture", false);

    //Check if the surface and the surface side defined is suitable
//    QString shapeType = getShapeType(m_surfaceURL);
//    QStringList list = {"Planar", "Cylinder"};
//    if (!list.contains(shapeType)) return;

//    if (m_surfaceSide != "front" && m_surfaceSide != "back")
//        return;

    //Create the photon map where photons are going to be stored
    if (!m_photons || !photonBufferAppend)
    {
        if (m_photons) m_photons->endExport(-1);
        delete m_photons;
//        long q = std::vector<Photon>::max_size();
        m_photons = new PhotonsBuffer(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()/64);
        m_tracedRays = 0;
        m_powerPhoton = 0;
        m_powerTotal = 0;
    }

    QVector<InstanceNode*> exportSuraceList;
    QModelIndex nodeIndex = m_sceneModel->indexFromUrl(m_surfaceURL);
    if (!nodeIndex.isValid()) return;
    InstanceNode* instanceNode = m_sceneModel->getInstance(nodeIndex);
    if (!instanceNode) return;
    exportSuraceList << instanceNode;

//    QThreadPool::globalInstance()->setMaxThreadCount(1);
    // Create a progress dialog.
    QProgressDialog dialog;
    dialog.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    dialog.setLabelText(QString("Progressing using %1 thread(s)...").arg(QThread::idealThreadCount() ) );

    // Create a QFutureWatcher and connect signals and slots.
    QFutureWatcher<void> watcher;
//    QObject::connect(&watcher, SIGNAL(finished()), &dialog, SLOT(reset()));
    QObject::connect(&dialog, SIGNAL(canceled()), &watcher, SLOT(cancel()));
//    QObject::connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &dialog, SLOT(setRange(int,int)));
//    QObject::connect(&watcher, SIGNAL(progressValueChanged(int)), &dialog, SLOT(setValue(int)));

    QObject::connect(&watcher, SIGNAL(progressValueChanged(int)), this, SLOT(processEvents()));
    QObject::connect(this, SIGNAL(stopSignal()), &watcher, SLOT(cancel()));

    AirTransmission* airTemp = 0;
    if (air->getTypeId() != AirTransmission::getClassTypeId())
        airTemp = air;

    GuiTracePreparationInput preparationInput;
    preparationInput.scene = m_sceneKit;
    preparationInput.layoutRoot = m_instanceLayout;
    preparationInput.sunInstance = &instanceSun;
    preparationInput.masterSeed = m_masterSeed++;
    preparationInput.photonBuffer = m_photons;
    preparationInput.exportSurfaceList = exportSuraceList;
    preparationInput.tracingAir = airTemp;
    preparationInput.rays = nRays;
    preparationInput.sunWidthDivisions = m_sunDivs.x;
    preparationInput.sunHeightDivisions = m_sunDivs.y;
    preparationInput.synchronizeScene = [this]() { m_sceneModel->UpdateSceneModel(); };
    PreparedTraceContext context;
    QString preparationError;
    if (!TracePreparation::prepareGuiTrace(preparationInput, &context, &preparationError))
        return;

    RayTraceExecutor executor;
    RayTraceExecution execution = executor.start(std::move(context));
    if (!execution.started) {
        qWarning() << execution.errorMessage;
        return;
    }

    watcher.setFuture(execution.future);

    // Display the dialog and start the event loop.
//    if (!silent) {
//        dialog.exec();
//    }

//    dialog.setModal(false);
//    dialog.show();
    QString executionError;
    if (!executor.waitForFinished(&execution, &executionError)) {
        qWarning() << executionError;
        return;
    }

    m_tracedRays += nRays;

    double irradiance = sunPosition->irradiance.getValue();
    double area = sunAperture->getArea();
    m_powerPhoton = area*irradiance/m_tracedRays;

    fillBins();
}

/*
 * Update photon counts for a specific grid divisions
 */
void FluxAnalysis::setBins(int rows, int cols)
{
    m_binsPhotons.resize(rows, cols);
    fillBins();
}

/*
 * Export the flux distribution
 */
void FluxAnalysis::write(QString fileName, bool withCoords)
{
    if (!m_photons) return;

    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);

    double uStep = m_box.size().x/m_binsPhotons.rows();
    double vStep = m_box.size().y/m_binsPhotons.cols();
    QModelIndex nodeIndex = m_sceneModel->indexFromUrl(m_surfaceURL);
    InstanceNode* instanceNode = m_sceneModel->getInstance(nodeIndex);
    vec3d sJ = instanceNode->getTransform().getScales();
    TShapeKit* kit = (TShapeKit*) instanceNode->getNode();
    ShapeRT* shape = (ShapeRT*) kit->shapeRT.getValue();
    // works for cylinder, not sphere
    double areaCell = dot(sJ, shape->getDerivativeU(0., 0.))*uStep;
    areaCell *= dot(sJ, shape->getDerivativeV(0., 0.))*vStep;
    // cross product better?

    double coeff = m_powerPhoton/areaCell;

    if (withCoords)
    {
        out << "x(m)\ty(m)\tFlux(W/m2)\n";
        for (int r = 0; r < m_binsPhotons.rows(); ++r) {
            for (int c = 0; c < m_binsPhotons.cols(); ++c)
                out << m_box.min().x + uStep*(r + 0.5)  << "\t" << m_box.min().y + vStep*(c + 0.5) << "\t" << m_binsPhotons(r, c)*coeff << "\n";
        }
    }
    else
    {
        for (int r = 0; r < m_binsPhotons.rows(); ++r) {
            for (int c = 0; c < m_binsPhotons.cols(); c++)
                out << m_binsPhotons(r, c)*coeff << "\t";
            out << "\n";
        }
    }
}

/*
 * Clear photon map
 */
void FluxAnalysis::clear()
{
    if (m_photons) m_photons->endExport(-1);
    delete m_photons;
    m_photons = 0;
    m_tracedRays = 0;
    m_powerPhoton = 0.;
    m_powerTotal = 0.;
}

void FluxAnalysis::processEvents()
{
    QCoreApplication::processEvents();
}

void FluxAnalysis::stop()
{
    emit stopSignal();
}

/*
 * Update photon counts
 */
void FluxAnalysis::fillBins()
{
    if (!m_photons) return;

    m_binsPhotons.fill(0);
    m_photonsMax = 0;
    m_photonsMaxPos = vec2i(0, 0);
    m_photonsError = 0;

//    QString shapeType = getShapeType(m_surfaceURL);
    QModelIndex index = m_sceneModel->indexFromUrl(m_surfaceURL);
    InstanceNode* instance = m_sceneModel->getInstance(index);
    if (!instance) return;
    TShapeKit* shapeKit = static_cast<TShapeKit*>(instance->getNode());
    if (!shapeKit) return;
    ShapeRT* shape = (ShapeRT*) shapeKit->shapeRT.getValue();
    if (!shape) return;
    ProfileRT* profile = (ProfileRT*) shapeKit->profileRT.getValue();
    m_box = profile->getBox();

    int activeSideID = m_surfaceSide == "back" ? 0 : 1;
    Transform toWorld = instance->getTransform();
    Transform toObject = toWorld.inversed();

    Matrix2D<int> binErrors(m_binsPhotons.rows() - 1, m_binsPhotons.cols() - 1);
    binErrors.fill(0);

    int photonsTotal = 0;
    for (const Photon& photon : m_photons->getPhotons())
    {
        if (photon.isFront != activeSideID) continue;
        photonsTotal++;
        vec3d p = toObject.transformPoint(photon.pos);
        vec2d uv = shape->getUV(p);
        vec2d q = (uv - m_box.min())/m_box.size();

        int r = floor(q.x*m_binsPhotons.rows());
        int c = floor(q.y*m_binsPhotons.cols());
        if (r == m_binsPhotons.rows()) r--;
        if (c == m_binsPhotons.cols()) c--;
        int& bin = m_binsPhotons(r, c);
        bin++;
        if (m_photonsMax < bin)
        {
            m_photonsMax = bin;
            m_photonsMaxPos = vec2i(r, c);
        }

        int rE = floor(q.x*binErrors.rows());
        int cE = floor(q.y*binErrors.cols());
        if (rE == binErrors.rows()) rE--;
        if (cE == binErrors.cols()) cE--;
        int& binE = binErrors(rE, cE);
        binE++;
        if (m_photonsError < binE)
            m_photonsError = binE;
    }

    m_powerTotal = photonsTotal*m_powerPhoton;

    // flux
    vec2i dims(m_binsPhotons.rows(), m_binsPhotons.cols());
    m_binsFlux.resize(dims.x, dims.y);
    double uStep = m_box.size().x/dims.x;
    double vStep = m_box.size().y/dims.y;
    for (int r = 0; r < dims.x; ++r) {
        for (int c = 0; c < dims.y; ++c) {
            double u0 = m_box.min().x + r*uStep;
            double v0 = m_box.min().y + c*vStep;
            m_binsFlux(r, c) = m_binsPhotons(r, c)*m_powerPhoton/shape->findArea(u0, v0, u0 + uStep, v0 + vStep, toWorld);
        }
    }
}
