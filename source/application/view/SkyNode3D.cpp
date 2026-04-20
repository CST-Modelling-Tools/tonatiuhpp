#include "SkyNode3D.h"

#include <cmath>

#include <Inventor/SbColor.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoQuadMesh.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTransform.h>

#include <QByteArray>
#include <QColor>
#include <QVector>
#include <QVector3D>

SO_NODE_SOURCE(SkyNode3D)

namespace
{
    constexpr float kSkyRadius = 500.0f;
    constexpr float kScaleRadius = 180.0f;
    constexpr float kTickBaseHeight = 0.0f;
    constexpr float kTickShortHeight = 3.5f;
    constexpr float kTickLongHeight = 7.0f;
    constexpr float kNumericLabelHeight = 11.0f;
    constexpr float kCardinalLabelHeight = 16.0f;
    constexpr float kSouthLabelHeight = 13.0f;
    constexpr float kNumericLabelRadius = 181.5f;
    constexpr float kCardinalLabelRadius = 183.5f;
    constexpr float kSouthLabelRadius = 182.0f;
    constexpr float kMinorTickLineWidth = 1.0f;
    constexpr float kMajorTickLineWidth = 1.8f;
    constexpr float kNumericFontSize = 16.0f;
    constexpr float kCardinalFontSize = 21.0f;

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

    SbVec3f pointOnHorizon(float azimuthDeg, float radius, float z)
    {
        const float azimuth = azimuthDeg * static_cast<float>(M_PI / 180.0);
        return SbVec3f(
            radius * std::sin(azimuth),
            radius * std::cos(azimuth),
            z);
    }

    int normalizeAzimuth(int azimuthDeg)
    {
        int normalized = azimuthDeg % 360;
        if (normalized < 0)
            normalized += 360;
        return normalized;
    }
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

    // Keep the sky centered on the viewer so the dome remains an environment
    // background instead of ordinary world geometry.
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

    QVector<SbVec3f> minorTickPoints;
    QVector<int> minorTickSizes;
    QVector<SbVec3f> majorTickPoints;
    QVector<int> majorTickSizes;
    minorTickPoints.reserve((360 / 5) * 2);
    minorTickSizes.reserve(360 / 5);
    majorTickPoints.reserve((360 / 15) * 2);
    majorTickSizes.reserve(360 / 15);

    // Only draw the visible East-North-West horizon arc.
    // A full 360-degree text ring would show back-side labels through the sky
    // because the sky is intentionally rendered as a background overlay.
    for (int azimuthDeg = 270; azimuthDeg <= 450; azimuthDeg += 5)
    {
        const bool isMajorTick = (azimuthDeg % 15) == 0;
        const int normalizedAzimuth = normalizeAzimuth(azimuthDeg);
        QVector<SbVec3f>& tickPoints = isMajorTick ? majorTickPoints : minorTickPoints;
        QVector<int>& tickSizes = isMajorTick ? majorTickSizes : minorTickSizes;
        const float tickHeight = isMajorTick ? kTickLongHeight : kTickShortHeight;
        tickPoints << pointOnHorizon(static_cast<float>(normalizedAzimuth), kScaleRadius, kTickBaseHeight);
        tickPoints << pointOnHorizon(static_cast<float>(normalizedAzimuth), kScaleRadius, tickHeight);
        tickSizes << 2;
    }

    auto addTickSet = [&](const QVector<SbVec3f>& points,
                          const QVector<int>& sizes,
                          const SbColor& color,
                          float lineWidth)
    {
        SoSeparator* tickRoot = new SoSeparator;

        SoMaterial* tickMaterial = new SoMaterial;
        tickMaterial->diffuseColor = color;
        tickRoot->addChild(tickMaterial);

        SoDrawStyle* drawStyle = new SoDrawStyle;
        drawStyle->lineWidth = lineWidth;
        tickRoot->addChild(drawStyle);

        SoCoordinate3* tickCoordinates = new SoCoordinate3;
        tickCoordinates->point.setValues(0, points.size(), points.constData());
        tickRoot->addChild(tickCoordinates);

        SoLineSet* tickLines = new SoLineSet;
        tickLines->numVertices.setValues(0, sizes.size(), sizes.constData());
        tickRoot->addChild(tickLines);

        root->addChild(tickRoot);
    };

    addTickSet(minorTickPoints, minorTickSizes, SbColor(0.76f, 0.82f, 0.88f), kMinorTickLineWidth);
    addTickSet(majorTickPoints, majorTickSizes, SbColor(0.96f, 0.98f, 1.0f), kMajorTickLineWidth);

    auto addTextLabel = [&](int azimuthDeg,
                            const char* text,
                            float radius,
                            float height,
                            float fontSize,
                            const SbColor& color)
    {
        SoSeparator* labelRoot = new SoSeparator;

        SoMaterial* textMaterial = new SoMaterial;
        textMaterial->diffuseColor = color;
        labelRoot->addChild(textMaterial);

        SoFont* font = new SoFont;
        font->name = "Arial:Bold";
        font->size = fontSize;
        labelRoot->addChild(font);

        SoTransform* transform = new SoTransform;
        transform->translation = pointOnHorizon(static_cast<float>(azimuthDeg), radius, height);
        labelRoot->addChild(transform);

        SoText2* textNode = new SoText2;
        textNode->string.setValue(text);
        textNode->justification = SoText2::CENTER;
        labelRoot->addChild(textNode);

        root->addChild(labelRoot);
    };

    for (int azimuthDeg = 270; azimuthDeg <= 450; azimuthDeg += 15)
    {
        const int normalizedAzimuth = normalizeAzimuth(azimuthDeg);

        switch (normalizedAzimuth)
        {
        case 0:
            addTextLabel(normalizedAzimuth, "N", kCardinalLabelRadius, kCardinalLabelHeight, kCardinalFontSize, SbColor(1.0f, 1.0f, 1.0f));
            break;
        case 90:
            addTextLabel(normalizedAzimuth, "E", kCardinalLabelRadius, kCardinalLabelHeight, kCardinalFontSize, SbColor(1.0f, 1.0f, 1.0f));
            break;
        case 270:
            addTextLabel(normalizedAzimuth, "W", kCardinalLabelRadius, kCardinalLabelHeight, kCardinalFontSize, SbColor(1.0f, 1.0f, 1.0f));
            break;
        default:
            {
                QByteArray angleText = QByteArray::number(normalizedAzimuth);
                addTextLabel(normalizedAzimuth, angleText.constData(), kNumericLabelRadius, kNumericLabelHeight, kNumericFontSize, SbColor(0.92f, 0.95f, 0.98f));
            }
            break;
        }
    }

    addTextLabel(180, "S", kSouthLabelRadius, kSouthLabelHeight, kCardinalFontSize, SbColor(1.0f, 1.0f, 1.0f));

    return root;
}
