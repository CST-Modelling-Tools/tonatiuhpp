#include "SkyNode3D.h"

#include <Inventor/SbColor.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTransform.h>

SO_NODE_SOURCE(SkyNode3D)

namespace
{
    // Radius of the sky dome.
    // Keep it large, but comfortably inside typical camera far planes.
    constexpr float kSkyRadius = 500.0f;
}

//------------------------------------------------------------------------------
// Class registration
//------------------------------------------------------------------------------

void SkyNode3D::initClass()
{
    SO_NODE_INIT_CLASS(SkyNode3D, SoSeparator, "Separator");
}

//------------------------------------------------------------------------------
// Construction / destruction
//------------------------------------------------------------------------------

SkyNode3D::SkyNode3D()
{
    SO_NODE_CONSTRUCTOR(SkyNode3D);

    // Optional top-level transform (currently unused).
    m_skyTransform = new SoTransform;
    addChild(m_skyTransform);

    // Depth setup so the sky acts as a background:
    //  - depth test OFF (so it never competes with scene geometry)
    //  - depth writes OFF (so it never blocks later draws)
    SoDepthBuffer* depth = new SoDepthBuffer;
    depth->test  = FALSE;
    depth->write = FALSE;
    addChild(depth);

    // Simple base-color lighting (no shading complexity).
    SoLightModel* lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    addChild(lightModel);

    addChild(makeSky());
    addChild(makeLabels());
}

SkyNode3D::~SkyNode3D() = default;

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void SkyNode3D::updateSkyCamera(SoPerspectiveCamera* camera)
{
    // Currently keep the sky fixed in world space.
    (void)camera;
}

//------------------------------------------------------------------------------
// Sky geometry (simple sphere)
//------------------------------------------------------------------------------

SoSeparator* SkyNode3D::makeSky()
{
    SoSeparator* root = new SoSeparator;
    root->renderCulling = SoSeparator::OFF;

    // Simple uniform sky color (adjust to taste).
    SoMaterial* material = new SoMaterial;
    material->diffuseColor = SbColor(0.38f, 0.46f, 0.62f); // soft blue
    material->ambientColor = material->diffuseColor;
    root->addChild(material);

    // Large sphere centered at origin; camera sits inside.
    SoSphere* sphere = new SoSphere;
    sphere->radius = kSkyRadius;
    root->addChild(sphere);

    return root;
}

//------------------------------------------------------------------------------
// Labels (N/E/S/W in world space)
//------------------------------------------------------------------------------

SoSeparator* SkyNode3D::makeLabels()
{
    SoSeparator* root = new SoSeparator;

    // Font reasonably large in world units.
    SoFont* font = new SoFont;
    font->name = "Arial:Bold";
    font->size = 20.0f;   // tweak after you see it
    root->addChild(font);

    SoMaterial* material = new SoMaterial;
    material->diffuseColor = SbColor(1.f, 1.f, 1.f);
    material->transparency = 0.0f;
    root->addChild(material);

    // Place labels around the origin in the X–Y plane.
    // Coordinate system:
    //   +X → East
    //   +Y → North
    const float r = 100.0f;  // distance from origin (well inside kSkyRadius)

    auto addLabelAt = [&](float x, float y, const char* text)
    {
        SoSeparator* labelRoot = new SoSeparator;

        SoTransform* tr = new SoTransform;
        tr->translation = SbVec3f(x, y, 0.f);
        labelRoot->addChild(tr);

        SoText2* textNode = new SoText2;
        textNode->string.setValue(text);
        textNode->justification = SoText2::CENTER;
        labelRoot->addChild(textNode);

        root->addChild(labelRoot);
    };

    addLabelAt( 0.f,  r,  "North");
    addLabelAt( r,   0.f, "East");
    addLabelAt( 0.f, -r,  "South");
    addLabelAt(-r,   0.f, "West");

    return root;
}