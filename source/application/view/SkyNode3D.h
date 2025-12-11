#pragma once

#include <Inventor/nodes/SoSeparator.h>

class SoTransform;
class SoPerspectiveCamera;

// Background sky node: big sphere + cardinal labels.
class SkyNode3D : public SoSeparator
{
    SO_NODE_HEADER(SkyNode3D);

public:
    static void initClass();
    SkyNode3D();

    // Old API compatibility: some code used getRoot()
    SoSeparator* getRoot() { return this; }

    // Currently a no-op. Kept for compatibility.
    void updateSkyCamera(SoPerspectiveCamera* camera);

protected:
    ~SkyNode3D() override;

    SoSeparator* makeSky();
    SoSeparator* makeLabels();

    // Root-level transform; currently unused but kept for future use.
    SoTransform* m_skyTransform = nullptr;
};