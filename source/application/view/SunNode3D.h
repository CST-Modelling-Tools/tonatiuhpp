#pragma once

#include <Inventor/nodes/SoSeparator.h>

class SoTransform;
class SoNodeSensor;
class SoSensor;
class SunPosition;

// Node that renders the visible sun marker on the sky dome and keeps it in
// sync with the scene SunPosition node.
class SunNode3D : public SoSeparator
{
    SO_NODE_HEADER(SunNode3D);

public:
    static void initClass();
    SunNode3D();

    // Expose the light-orientation transform so GraphicRoot can reuse it for
    // the directional light.
    SoTransform* getTransform() const { return m_transform; }

    // Attach a SunPosition node so the rendered sun follows its
    // azimuth/elevation.
    void attach(SunPosition* sp);

protected:
    ~SunNode3D() override;

    static void update(void* data, SoSensor* sensor);
    void create();

    SoTransform*  m_transform = nullptr;
    SoTransform*  m_markerTransform = nullptr;
    SoNodeSensor* m_sensor    = nullptr;
};
