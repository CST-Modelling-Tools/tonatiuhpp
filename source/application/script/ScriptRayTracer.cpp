#include "ScriptRayTracer.h"

#include <QFutureWatcher>
#include <QMutex>
#include <QPoint>
//#include <QScriptContext>
#include <QtConcurrentMap>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodekits/SoSceneKit.h>
#include <Inventor/nodes/SoSelection.h>

#include "kernel/air/AirTransmission.h"
#include "kernel/node/TonatiuhFunctions.h"
#include "kernel/photons/PhotonsBuffer.h"
#include "kernel/run/RayTracer.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/sun/SunAperture.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunShape.h"
#include "libraries/math/gcf.h"
#include "main/Document.h"
#include "tree/SceneTreeModel.h"
#include "view/GraphicRoot.h"


ScriptRayTracer::ScriptRayTracer():
    m_document(0),
    m_irradiance(-1),
    m_numberOfRays(0),
    m_photonMap(0),
    m_sceneModel(0),
    m_widthDivisions(200),
    m_heightDivisions(200),
    m_sunPosistionChanged(false),
    m_sunAzimuth(0),
    m_sunElevation(0),
    m_wPhoton(0),
    m_dirName("")
{

}

ScriptRayTracer::~ScriptRayTracer()
{
    delete m_document;
    delete m_photonMap;
    delete m_sceneModel;
}

void ScriptRayTracer::Clear()
{
    delete m_document;
    m_document = 0;
    m_irradiance = -1;
    m_numberOfRays = 0;
    delete m_photonMap;
    m_photonMap = 0;
    delete m_sceneModel;
    m_sceneModel = 0;
    m_sunAzimuth = 0;
    m_sunElevation = 0;
    m_wPhoton = 0;
    m_dirName.clear();
}

bool ScriptRayTracer::IsValidSurface(QString surfaceName)
{
    if (!m_sceneModel) return false;

    QModelIndex surfaceIndex = m_sceneModel->indexFromUrl(surfaceName);
    InstanceNode* selectedSurface = m_sceneModel->getInstance(surfaceIndex);
    if (!selectedSurface) return false;

    return true;
}

int ScriptRayTracer::setDir(QString dir)
{
    m_dirName = dir;
    return 1;
}

int ScriptRayTracer::setIrradiance(double irradiance)
{
    m_irradiance = irradiance;
    return 1;
}

int ScriptRayTracer::setNumberOfRays(double nrays)
{
    m_numberOfRays = nrays;
    return 1;
}

int ScriptRayTracer::SetNumberOfWidthDivisions(int ndivisions)
{
    m_widthDivisions = ndivisions;
    return 1;
}

int ScriptRayTracer::SetNumberOfHeightDivisions(int ndivisions)
{
    m_heightDivisions = ndivisions;
    return 1;
}

int ScriptRayTracer::SetPhotonMapExportMode(QString typeName)
{
    if (typeName == QLatin1String("File") ) m_photonMapToFile = true;
    else if (typeName == QLatin1String("DB") ) m_photonMapToFile = false;
    else return 0;

    return 1;
}

/*!
 * Saves the sun position \a azimuth value. \a azimuth  is in degrees.
 */
void ScriptRayTracer::SetSunAzimtuh(double azimuth)
{
    m_sunAzimuth = azimuth*gcf::degree;
    m_sunPosistionChanged = true;
}

/*!
 * Saves the sun position \a elevation value. \a elevation  is in degrees.
 */
void ScriptRayTracer::SetSunElevation(double elevation)
{
    m_sunElevation = elevation*gcf::degree;
    m_sunPosistionChanged = true;
}

int ScriptRayTracer::SetSunPositionToScene()
{
    if (m_sceneModel)
    {
        QModelIndex sceneIndex;
        InstanceNode* sceneInstance = m_sceneModel->getInstance(sceneIndex);
        SoSceneKit* coinScene = static_cast<SoSceneKit*>(sceneInstance->getNode() );

        if ((coinScene)&& (coinScene->getPart("lightList[0]", false) ))
        {
//            SunKit* sunKit = static_cast< SunKit* >(coinScene->getPart("world.sun", false) ); //?
//            if (m_sunPosistionChanged) {
//                sunKit->azimuth.setValue(m_sunAzimuth);
//                sunKit->elevation.setValue(m_sunElevation);
//                sunKit->updateTransform();
//            }
//            return 1;
        }
        std::cerr << "ScriptRayTracer::SetSunPositionToScene() light not found in scene" << std::endl;
        return 0;
    }
    std::cerr << "ScriptRayTracer::SetSunPositionToScene() sceneModel not found" << std::endl;
    return 0;
}

int ScriptRayTracer::Save(const QString& fileName)
{
    if (!m_document->WriteFile(fileName) )
    {
        std::cerr << "Saving canceled";
        return 0;
    }

    std::cerr << "File saved";
    return 1;
}

int ScriptRayTracer::openFile(QString filename)
{
    delete m_document;
    m_document = new Document;
    if (!m_document->ReadFile(filename) ) return 0;

    delete m_sceneModel;
    m_sceneModel = new SceneTreeModel;
    m_sceneModel->setDocument(m_document);

    return 1;
}

int ScriptRayTracer::Trace()
{
    // Dormant C++ compatibility method, not a Q_INVOKABLE script API.
    // Never report success without executing a trace.
    std::cerr << "ScriptRayTracer::Trace is unsupported; use the shared tn.traceScene API."
              << std::endl;
    return 0;
}
