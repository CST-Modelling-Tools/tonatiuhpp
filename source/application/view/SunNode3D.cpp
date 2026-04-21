#include "SunNode3D.h"

#include <cmath>

#include <QImage>

#include "kernel/sun/SunPosition.h"

#include <Inventor/SbColor.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoTextureCoordinateBinding.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/sensors/SoNodeSensor.h>

#include "libraries/math/gcf.h"

SO_NODE_SOURCE(SunNode3D)

namespace
{
    constexpr float kSkyDomeRadius = 500.0f;
    constexpr float kSolarAngularDiameterRad = 0.00925f;

    float solarDiskHalfExtent(float domeRadius)
    {
        // For a disk centered on the viewer's sightline at distance R,
        // half-size = R * tan(angularDiameter / 2).
        return domeRadius * std::tan(0.5f * kSolarAngularDiameterRad);
    }

    SbVec3f toSbVec3f(const vec3d& value)
    {
        return SbVec3f(
            static_cast<float>(value.x),
            static_cast<float>(value.y),
            static_cast<float>(value.z));
    }
}

void SunNode3D::initClass()
{
    SO_NODE_INIT_CLASS(SunNode3D, SoSeparator, "Separator");
}

SunNode3D::SunNode3D()
{
    SO_NODE_CONSTRUCTOR(SunNode3D);

    create();
    m_sensor = new SoNodeSensor(update, this);
}

SunNode3D::~SunNode3D()
{
    delete m_sensor;
}

void SunNode3D::attach(SunPosition* sp)
{
    if (!m_sensor)
        return;

    m_sensor->detach();
    if (sp)
    {
        m_sensor->attach(sp);
        update(this, nullptr);
    }
}

void SunNode3D::create()
{
    renderCulling = SoSeparator::OFF;

    SoSeparator* lightTransformRoot = new SoSeparator;
    m_transform = new SoTransform;
    m_transform->setName("transformSun");
    lightTransformRoot->addChild(m_transform);
    addChild(lightTransformRoot);

    SoSeparator* markerRoot = new SoSeparator;
    markerRoot->renderCulling = SoSeparator::OFF;
    addChild(markerRoot);

    m_markerTransform = new SoTransform;
    markerRoot->addChild(m_markerTransform);

    SoTexture2* texture = new SoTexture2;
    texture->model = SoTexture2::REPLACE;
    const QImage sourceImage(":/images/sky/sun.png");
    if (!sourceImage.isNull())
    {
        const QImage textureImage = sourceImage.convertToFormat(QImage::Format_RGBA8888);
        texture->image.setValue(
            SbVec2s(
                static_cast<short>(textureImage.width()),
                static_cast<short>(textureImage.height())),
            4,
            textureImage.constBits());
    }
    markerRoot->addChild(texture);

    SoTextureCoordinate2* tCoords = new SoTextureCoordinate2;
    tCoords->point.set1Value(0, SbVec2f(0.f, 0.f));
    tCoords->point.set1Value(1, SbVec2f(1.f, 0.f));
    tCoords->point.set1Value(2, SbVec2f(1.f, 1.f));
    tCoords->point.set1Value(3, SbVec2f(0.f, 1.f));
    markerRoot->addChild(tCoords);

    SoTextureCoordinateBinding* tBind = new SoTextureCoordinateBinding;
    tBind->value = SoTextureCoordinateBinding::PER_VERTEX;
    markerRoot->addChild(tBind);

    SoMaterial* material = new SoMaterial;
    material->diffuseColor = SbColor(1.f, 1.f, 1.f);
    material->emissiveColor = SbColor(1.f, 1.f, 1.f);
    markerRoot->addChild(material);

    SoCoordinate3* coords = new SoCoordinate3;
    const float halfExtent = solarDiskHalfExtent(kSkyDomeRadius);
    coords->point.set1Value(0, SbVec3f(-halfExtent, 0.f, -halfExtent));
    coords->point.set1Value(1, SbVec3f( halfExtent, 0.f, -halfExtent));
    coords->point.set1Value(2, SbVec3f( halfExtent, 0.f,  halfExtent));
    coords->point.set1Value(3, SbVec3f(-halfExtent, 0.f,  halfExtent));
    markerRoot->addChild(coords);

    SoFaceSet* face = new SoFaceSet;
    face->numVertices.set1Value(0, 4);
    markerRoot->addChild(face);
}

void SunNode3D::update(void* data, SoSensor*)
{
    auto* node = static_cast<SunNode3D*>(data);
    if (!node || !node->m_sensor || !node->m_transform || !node->m_markerTransform)
        return;

    auto* sp = static_cast<SunPosition*>(node->m_sensor->getAttachedNode());
    if (!sp)
        return;

    // Rz(-gamma) Rx(alpha)
    node->m_transform->rotation =
        SbRotation(SbVec3f(1.f, 0.f, 0.f),
                   static_cast<float>((90.0 + sp->elevation.getValue()) *
                                      gcf::degree)) *
        SbRotation(SbVec3f(0.f, 0.f, 1.f),
                   static_cast<float>(-sp->azimuth.getValue() *
                                      gcf::degree));

    const SbVec3f sunDirection = toSbVec3f(sp->getSunVector());
    node->m_markerTransform->translation = sunDirection * kSkyDomeRadius;
    node->m_markerTransform->rotation = SbRotation(SbVec3f(0.f, 1.f, 0.f), sunDirection);
}
