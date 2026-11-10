#pragma once

#include "kernel/sun/SunShape.h"


class SunGaussian: public SunShape
{
    SO_NODE_HEADER(SunGaussian);

public:
    static void initClass();
    SunGaussian();
    SoNode* copy(SbBool copyConnections) const;

    vec3d generateRay(Random& rand) const;
    double getThetaMax() const;
    double shape(double theta) const;

    SoSFDouble sigma;

    NAME_ICON_FUNCTIONS("Gaussian", ":/SunGaussian.png")

protected:
    ~SunGaussian();

    void updateSigma();
    double zenithAngle(Random& rand) const;
    double pdfTheta(double theta) const;

    double m_thetaMax; // sun disk
    double m_h; // maximum of pdf

    SoNodeSensor* m_sensor;
    static void onSensor(void* data, SoSensor*);
};


class SunGaussianFactory:
    public QObject, public SunFactoryT<SunGaussian>
{
    Q_OBJECT
    Q_INTERFACES(SunFactory)
    Q_PLUGIN_METADATA(IID "tonatiuh.SunFactory" FILE "../../tonatiuhpp-headless-plugin.json")
};
