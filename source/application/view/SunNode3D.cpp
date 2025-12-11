#include "SunNode3D.h"

#include <QFileInfo>

#include "kernel/sun/SunPosition.h"

#include <Inventor/SbRotation.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoTextureCoordinateBinding.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/sensors/SoNodeSensor.h>

#include "libraries/math/gcf.h"

SO_NODE_SOURCE(SunNode3D)

//------------------------------------------------------------------------------
// Class registration
//------------------------------------------------------------------------------

void SunNode3D::initClass()
{
    SO_NODE_INIT_CLASS(SunNode3D, SoSeparator, "Separator");
}

//------------------------------------------------------------------------------
// Construction / destruction
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

void SunNode3D::create()
{
    // Base transform that will be rotated according to the sun position.
    m_transform = new SoTransform;
    m_transform->setName("transformSun");
    addChild(m_transform);

    // Move the quad in front of the camera.
    SoTransform* t = new SoTransform;
    t->translation = SbVec3f(0.f, 0.f, -1.f);
    addChild(t);

    // Sun texture.
    SoTexture2* texture = new SoTexture2;
    texture->filename =
        QFileInfo("resources:/images/sun.png").filePath().toLatin1().data();
    texture->model = SoTexture2::REPLACE;
    addChild(texture);

    // Texture coordinates.
    SoTextureCoordinate2* tCoords = new SoTextureCoordinate2;
    tCoords->point.set1Value(0, SbVec2f(0.f, 0.f));
    tCoords->point.set1Value(1, SbVec2f(1.f, 0.f));
    tCoords->point.set1Value(2, SbVec2f(1.f, 1.f));
    tCoords->point.set1Value(3, SbVec2f(0.f, 1.f));
    addChild(tCoords);

    SoTextureCoordinateBinding* tBind = new SoTextureCoordinateBinding;
    tBind->value = SoTextureCoordinateBinding::PER_VERTEX;
    addChild(tBind);

    // Quad geometry.
    SoCoordinate3* coords = new SoCoordinate3;
    const double s = 0.05;
    coords->point.set1Value(0, SbVec3f(-s, -s, 0.f));
    coords->point.set1Value(1, SbVec3f( s, -s, 0.f));
    coords->point.set1Value(2, SbVec3f( s,  s, 0.f));
    coords->point.set1Value(3, SbVec3f(-s,  s, 0.f));
    addChild(coords);

    SoFaceSet* face = new SoFaceSet;
    face->numVertices.set1Value(0, 4);
    addChild(face);
}

//------------------------------------------------------------------------------

void SunNode3D::update(void* data, SoSensor*)
{
    auto* node = static_cast<SunNode3D*>(data);
    if (!node || !node->m_sensor || !node->m_transform)
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
}