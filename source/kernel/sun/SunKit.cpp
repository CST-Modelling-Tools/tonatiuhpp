#include "SunKit.h"

#include "SunPosition.h"
#include "SunShapePillbox.h"
#include "SunAperture.h"

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoLabel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodekits/SoNodekitCatalog.h>

#include "kernel/node/TonatiuhFunctions.h"
#include "scene/TSceneKit.h"
#include "scene/TSeparatorKit.h"
#include "kernel/run/InstanceNode.h"
#include "libraries/math/3D/Box3D.h"
#include "libraries/math/3D/Matrix4x4.h"
#include "libraries/math/3D/Transform.h"
#include "libraries/math/gcf.h"
#include "scene/TShapeKit.h"

SO_KIT_SOURCE(SunKit)


void SunKit::initClass()
{
    SO_KIT_INIT_CLASS(SunKit, SoBaseKit, "BaseKit");

    SunPosition::initClass();
    SunShape::initClass();
    SunAperture::initClass();
}

SunKit::SunKit()
{
    SO_KIT_CONSTRUCTOR(SunKit);
    isBuiltIn = TRUE;

    SO_KIT_ADD_CATALOG_ENTRY(position, SunPosition, FALSE, this, "", TRUE);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SunShape, SunShapePillbox, FALSE, this, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(aperture, SunAperture, FALSE, this, "", TRUE);

    SO_KIT_INIT_INSTANCE();
    setSearchingChildren(true);

    // temp
    m_transform = new SoTransform;
    updateTransform();
    SbMatrix mr;
    mr.setRotate(m_transform->rotation.getValue());
    SbVec3f res;
    mr.multVecMatrix(SbVec3f(0., 0., -10.), res);
    m_transform->translation = res;

    m_imageMaterial = new SoMaterial;
    m_imageMaterial->transparency = 0.75f;

    int sizeX = 10;
    int sizeY = 10;
    QVector<uchar> bitmap(sizeX*sizeY);
    bitmap.fill(255);

    m_imageTexture = new SoTexture2;
    m_imageTexture->image.setValue(SbVec2s(sizeX, sizeY), 1, bitmap.data());
    m_imageTexture->model = SoTexture2::BLEND;
    m_imageTexture->blendColor.setValue(1.f, 1.f, 1.f);

    setName("Sun");
}

void SunKit::updateTransform()
{
    SunPosition* sp = (SunPosition*) getPart("position", true);
    m_transform->rotation =
        SbRotation(SbVec3f(1., 0., 0.), (90. + sp->elevation.getValue())*gcf::degree)*
        SbRotation(SbVec3f(0., 0., 1.), -sp->azimuth.getValue()*gcf::degree);
}

void SunKit::setBox(Box3D box)
{
    updateTransform();
    SbMatrix mr;
    mr.setRotate(m_transform->rotation.getValue());

    Transform tr = tgf::makeTransform(mr).inversed();
    box = tr(box);

    //box is global
    vec3d vA = box.min();
    vec3d vB = box.max();
    double distMax = vB.z + 10. - vA.z;
    double back = vA.z - 10.;

    if (-gcf::infinity == box.volume() )
    {
        vA.x = 0.;
        vB.x = 0.;
        vA.y = 0.;
        vB.y = 0.;
        distMax = 0.;
    }

    double delta = 0.01;
    SunShape* sunshape = static_cast<SunShape*>(getPart("shape", false));
    if (!sunshape) return;
    double thetaMax = sunshape->getThetaMax();
    if (thetaMax > 0.)
        delta = distMax*tan(thetaMax);

    SunAperture* shape = static_cast<SunAperture*>(getPart("aperture", false));
    if (!shape) return;
    shape->setSize(vA.x, vB.x, vA.y, vB.y, delta);

    SbVec3f res;
    mr.multVecMatrix(SbVec3f(0., 0., back), res);
    m_transform->translation = res;
}

void SunKit::setBox(TSceneKit* scene)
{
    if (!scene) return;

    TSeparatorKit* separatorKit = scene->getLayout();
    if (!separatorKit) return;

    // Dispatch through SoAction::apply() to initialize Coin's traversal state.
    // Calling getBoundingBox() on the node directly bypasses this setup.
    SoGetBoundingBoxAction action{SbViewportRegion()};
    action.apply(separatorKit);
    SbBox3f box = action.getBoundingBox();

    if (!box.isEmpty() )
    {
        Box3D sceneBox(
            vec3d(box.getMin()[0], box.getMin()[1], box.getMin()[2]),
            vec3d(box.getMax()[0], box.getMax()[1], box.getMax()[2])
        );
        setBox(sceneBox);
    }
}

bool SunKit::findTexture(int sizeX, int sizeY, InstanceNode* instanceRoot)
{
    if (!instanceRoot) return false;

    SunAperture* aperture = static_cast<SunAperture*>(getPart("aperture", false));
    if (!aperture) return false;

    QStringList disabledList = QString(aperture->disabledNodes.getValue().getString()).split(";", Qt::SkipEmptyParts);
    QVector< QPair<TShapeKit*, Transform> > surfacesList;
    instanceRoot->collectShapeTransforms(disabledList, surfacesList);
    if (surfacesList.isEmpty()) return false;

    SbMatrix mr;
    mr.setRotate(m_transform->rotation.getValue());
    Transform tSun = tgf::makeTransform(mr).inversed();
    for (auto& s : surfacesList)
        s.second = tSun*s.second;

    aperture->findTexture(sizeX, sizeY, surfacesList, this);
    return true;
}
