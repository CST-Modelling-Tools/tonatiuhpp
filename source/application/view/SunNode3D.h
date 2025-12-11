#pragma once

#include <Inventor/nodes/SoSeparator.h>

class SoTransform;
class SoNodeSensor;
class SoSensor;
class SunPosition;

// Node that renders a textured quad representing the sun and
// keeps its orientation in sync with a SunPosition node.
class SunNode3D : public SoSeparator
{
    SO_NODE_HEADER(SunNode3D);

public:
    static void initClass();
    SunNode3D();

    // Expose the transform so other code (GraphicRoot) can access it.
    SoTransform* getTransform() const { return m_transform; }

    // Attach a SunPosition node so the sun follows its azimuth/elevation.
    void attach(SunPosition* sp);

protected:
    ~SunNode3D() override;

    static void update(void* data, SoSensor* sensor);
    void create();

    SoTransform*  m_transform = nullptr;
    SoNodeSensor* m_sensor    = nullptr;
};