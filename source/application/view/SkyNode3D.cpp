#include "SkyNode3D.h"

#include <cmath>

#include <Inventor/SbColor.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoQuadMesh.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTransform.h>

#include <QColor>
#include <QVector>
#include <QVector3D>

SO_NODE_SOURCE(SkyNode3D)

namespace
{
    constexpr float kSkyRadius = 500.0f;
    constexpr float kLabelRadius = 140.0f;
    constexpr float kLabelHeight = 8.0f;
    constexpr float kLabelShadowOffset = 4.0f;

    struct SkyGradient
    {
        QColor horizon;
        QColor zenith;
        QVector3D transition;

        QVector3D mix(double elevation) const
        {
            double alpha = elevation / (M_PI / 2.0);
            if (alpha < 0.001)
                alpha = 0.001;
            else if (alpha >= 0.999)
                alpha = 0.999;

            alpha = 1.0 - alpha;
            double temp = alpha * alpha;
            temp = temp / (temp - 1.0);
            return QVector3D(
                std::exp(transition.x() * temp),
                std::exp(transition.y() * temp),
                std::exp(transition.z() * temp));
        }

        QColor sample(double elevation) const
        {
            const QVector3D m = mix(elevation);
            return QColor(
                horizon.red()   + (zenith.red()   - horizon.red())   * m.x(),
                horizon.green() + (zenith.green() - horizon.green()) * m.y(),
                horizon.blue()  + (zenith.blue()  - horizon.blue())  * m.z());
        }
    };
}

void SkyNode3D::initClass()
{
    SO_NODE_INIT_CLASS(SkyNode3D, SoSeparator, "Separator");
}

SkyNode3D::SkyNode3D()
{
    SO_NODE_CONSTRUCTOR(SkyNode3D);

    m_skyTransform = new SoTransform;
    addChild(m_skyTransform);

    SoDepthBuffer* depth = new SoDepthBuffer;
    depth->test = FALSE;
    depth->write = FALSE;
    addChild(depth);

    SoLightModel* lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    addChild(lightModel);

    addChild(makeSky());
    addChild(makeLabels());
}

SkyNode3D::~SkyNode3D() = default;

void SkyNode3D::updateSkyCamera(SoPerspectiveCamera* camera)
{
    if (!camera || !m_skyTransform)
        return;

    // Keep the sky centered on the viewer so the dome and compass remain visible
    // without reviving the old custom GLRender path.
    m_skyTransform->translation = camera->position.getValue();
}

SoSeparator* SkyNode3D::makeSky()
{
    SoSeparator* root = new SoSeparator;
    root->renderCulling = SoSeparator::OFF;

    const SkyGradient skyGradient{
        QColor("#b2c3d2"),
        QColor("#60769d"),
        QVector3D(0.2f, 0.2f, 0.2f)
    };
    const SkyGradient groundGradient{
        QColor("#e0ddd0"),
        QColor("#e6e3d1"),
        QVector3D(0.1f, 0.1f, 0.1f)
    };

    const QVector<float> elevationsDeg = {
        -90.f, -60.f, -30.f, -10.f, -5.f, -2.5f, -0.01f,
         0.01f,  2.5f,   5.f,  10.f,  30.f,  60.f,  90.f
    };
    constexpr int kAzimuthSamples = 24;

    QVector<SbVec3f> vertices;
    QVector<SbColor> colors;
    vertices.reserve(kAzimuthSamples * elevationsDeg.size());
    colors.reserve(kAzimuthSamples * elevationsDeg.size());

    for (int azimuthIndex = 0; azimuthIndex < kAzimuthSamples; ++azimuthIndex)
    {
        const float phi = static_cast<float>(2.0 * M_PI * azimuthIndex / (kAzimuthSamples - 1));

        for (float elevationDeg : elevationsDeg)
        {
            const float elevation = elevationDeg * static_cast<float>(M_PI / 180.0);
            const float cosElevation = std::cos(elevation);
            const float radius = elevation >= 0.f ? kSkyRadius : 0.8f * kSkyRadius;

            vertices << SbVec3f(
                radius * std::cos(phi) * cosElevation,
                radius * std::sin(phi) * cosElevation,
                radius * std::sin(elevation));

            const QColor color = elevation >= 0.f
                ? skyGradient.sample(elevation)
                : groundGradient.sample(-elevation);
            colors << SbColor(color.redF(), color.greenF(), color.blueF());
        }
    }

    SoCoordinate3* coordinates = new SoCoordinate3;
    coordinates->point.setValues(0, vertices.size(), vertices.constData());
    root->addChild(coordinates);

    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValues(0, colors.size(), colors.constData());
    root->addChild(material);

    SoMaterialBinding* materialBinding = new SoMaterialBinding;
    materialBinding->value = SoMaterialBinding::PER_VERTEX;
    root->addChild(materialBinding);

    SoQuadMesh* skyMesh = new SoQuadMesh;
    skyMesh->verticesPerRow = elevationsDeg.size();
    skyMesh->verticesPerColumn = kAzimuthSamples;
    root->addChild(skyMesh);

    return root;
}

SoSeparator* SkyNode3D::makeLabels()
{
    SoSeparator* root = new SoSeparator;

    SoFont* font = new SoFont;
    font->name = "Arial:Bold";
    font->size = 18.0f;
    root->addChild(font);

    auto addLabelAt = [&](float x, float y, const char* text, const char* shortText)
    {
        SoSeparator* labelRoot = new SoSeparator;

        auto addTextLayer = [&](const SbColor& color, const SbVec3f& offset, const char* value)
        {
            SoSeparator* layer = new SoSeparator;

            SoMaterial* material = new SoMaterial;
            material->diffuseColor = color;
            layer->addChild(material);

            SoTransform* transform = new SoTransform;
            transform->translation = offset;
            layer->addChild(transform);

            SoText2* textNode = new SoText2;
            textNode->string.setValue(value);
            textNode->justification = SoText2::CENTER;
            layer->addChild(textNode);

            labelRoot->addChild(layer);
        };

        SoTransform* placement = new SoTransform;
        placement->translation = SbVec3f(x, y, kLabelHeight);
        labelRoot->addChild(placement);

        addTextLayer(SbColor(0.08f, 0.10f, 0.14f), SbVec3f(kLabelShadowOffset, -kLabelShadowOffset, 0.f), text);
        addTextLayer(SbColor(1.f, 1.f, 1.f), SbVec3f(0.f, 0.f, 0.f), text);

        SoTransform* shortPlacement = new SoTransform;
        shortPlacement->translation = SbVec3f(0.f, -22.f, 0.f);
        labelRoot->addChild(shortPlacement);

        addTextLayer(SbColor(0.08f, 0.10f, 0.14f), SbVec3f(kLabelShadowOffset, -kLabelShadowOffset, 0.f), shortText);
        addTextLayer(SbColor(0.92f, 0.96f, 1.f), SbVec3f(0.f, 0.f, 0.f), shortText);

        root->addChild(labelRoot);
    };

    // Coordinate system:
    //   +X -> East
    //   +Y -> North
    addLabelAt(0.f,           kLabelRadius,  "North", "N");
    addLabelAt(kLabelRadius,  0.f,           "East",  "E");
    addLabelAt(0.f,          -kLabelRadius,  "South", "S");
    addLabelAt(-kLabelRadius, 0.f,           "West",  "W");

    return root;
}
