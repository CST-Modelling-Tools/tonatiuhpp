#include "FluxAnalysis.h"
#include "run/FluxAnalysisTsvWriter.h"

#include <cmath>
#include <vector>
#include <utility>

#include <QFileDialog>
#include <QFutureWatcher>
#include <QDebug>
#include <QPair>
#include <QProgressDialog>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoGroup.h>

#include "tree/SceneTreeModel.h"
#include "core/RayTraceExecutor.h"
#include "core/SimulationSession.h"
#include "core/TracePreparation.h"
#include "core/FluxGridComputation.h"
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
bool FluxAnalysis::run(QString nodeURL, QString surfaceSide, ulong nRays, bool photonBufferAppend, int uDivs, int vDivs, bool /*silent*/)
{
    m_surfaceURL = nodeURL;
    m_surfaceSide = surfaceSide;

    m_binsPhotons.resize(uDivs, vDivs);

    if (!m_sceneKit) return false;

    AirTransmission* air = dynamic_cast<AirTransmission*>(m_sceneKit->getPart("world.air.transmission", false));

    if (!m_instanceLayout) return false;
    InstanceNode* instanceScene = m_instanceLayout->getParent();
    if (!instanceScene) return false;

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
    if (!nodeIndex.isValid()) return false;
    InstanceNode* instanceNode = m_sceneModel->getInstance(nodeIndex);
    if (!instanceNode) return false;
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

    // Do not consume an RNG draw for unity vacuum transmission after reflections.
    // Match the effective air policy used by the headless interception path.
    AirTransmission* airTemp = TracePreparation::effectiveTracingAir(air);

    GuiTracePreparationInput preparationInput;
    preparationInput.scene = m_sceneKit;
    preparationInput.layoutRoot = m_instanceLayout;
    preparationInput.sunInstance = &instanceSun;
    // Surface-analysis replay and append auditing need the effective seed.
    // Preserve the existing increment-per-run policy (also for append).
    const std::uint64_t fluxSeed = m_masterSeed++;
    qInfo().noquote() << "flux_analysis_master_seed:" << QString::number(static_cast<qulonglong>(fluxSeed));
    preparationInput.configuration.masterSeed = fluxSeed;
    preparationInput.photonBuffer = m_photons;
    preparationInput.exportSurfaceList = exportSuraceList;
    preparationInput.tracingAir = airTemp;
    preparationInput.configuration.rays = nRays;
    preparationInput.configuration.sunWidthDivisions = m_sunDivs.x;
    preparationInput.configuration.sunHeightDivisions = m_sunDivs.y;
    preparationInput.synchronizeScene = [this]() { m_sceneModel->UpdateSceneModel(); };
    SimulationSession session;
    QString preparationError;
    if (!session.prepareGui(preparationInput, &preparationError)) {
        qWarning() << "Flux analysis preparation failed:" << preparationError;
        clear();
        return false;
    }

    if (!session.start(&preparationError)) {
        qWarning() << preparationError;
        clear();
        return false;
    }

    watcher.setFuture(session.future());

    // Display the dialog and start the event loop.
//    if (!silent) {
//        dialog.exec();
//    }

//    dialog.setModal(false);
//    dialog.show();
    QString executionError;
    if (!session.waitForFinished(&executionError)) {
        qWarning() << executionError;
        // Partial photons cannot be appended to an accepted flux result.
        clear();
        return false;
    }

    m_tracedRays += session.result().raysTraced;

    double irradiance = sunPosition->irradiance.getValue();
    double area = sunAperture->getArea();
    m_powerPhoton = area*irradiance/m_tracedRays;

    return fillBins();
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
bool FluxAnalysis::write(QString fileName, bool withCoords, QString* error)
{
    if (error) error->clear();
    if (!m_photons || m_binsPhotons.rows() <= 0 || m_binsPhotons.cols() <= 0) {
        if (error) *error = "Flux analysis has no completed photon grid.";
        return false;
    }
    const QModelIndex nodeIndex = m_sceneModel->indexFromUrl(m_surfaceURL);
    InstanceNode* instanceNode = m_sceneModel->getInstance(nodeIndex);
    if (!instanceNode) {
        if (error) *error = "Flux analysis target surface is unavailable.";
        return false;
    }
    vec3d sJ = instanceNode->getTransform().getScales();
    TShapeKit* kit = (TShapeKit*) instanceNode->getNode();
    ShapeRT* shape = kit ? (ShapeRT*) kit->shapeRT.getValue() : nullptr;
    if (!shape) {
        if (error) *error = "Flux analysis target shape is unavailable.";
        return false;
    }

    // Preserve the legacy flux export normalizer exactly, including its
    // projected-derivative area expression. Scientific formula review is
    // separate from this fail-closed I/O increment.
    const double uStep = m_box.size().x / m_binsPhotons.rows();
    const double vStep = m_box.size().y / m_binsPhotons.cols();
    double areaCell = dot(sJ, shape->getDerivativeU(0., 0.)) * uStep;
    areaCell *= dot(sJ, shape->getDerivativeV(0., 0.)) * vStep;
    if (!std::isfinite(areaCell) || areaCell == 0.) {
        if (error) *error = "Flux analysis cell area is zero or non-finite.";
        return false;
    }
    const double coeff = m_powerPhoton / areaCell;
    return FluxAnalysisTsvWriter::write(fileName, m_binsPhotons, m_box,
                                        coeff, withCoords, error);
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
bool FluxAnalysis::fillBins()
{
    if (!m_photons || !m_sceneModel)
        return false;

    const QModelIndex index = m_sceneModel->indexFromUrl(m_surfaceURL);
    InstanceNode* instance = m_sceneModel->getInstance(index);
    FluxGridComputationResult result;
    QString error;
    if (!FluxGridComputation::calculate(m_photons->getPhotons(), instance,
                                        m_surfaceSide, m_binsPhotons.rows(),
                                        m_binsPhotons.cols(), m_powerPhoton,
                                        &result, &error)) {
        qWarning().noquote() << "Flux grid computation failed:" << error;
        return false;
    }

    // Preserve every public result used by FluxAnalysisDialog or exporters.
    m_binsPhotons = std::move(result.binsPhotons);
    m_binsFlux = std::move(result.binsFlux);
    m_box = result.box;
    m_photonsMax = result.photonsMax;
    m_photonsMaxPos = result.photonsMaxPos;
    m_photonsError = result.photonsError;
    m_powerTotal = result.powerTotal;
    return true;
}
