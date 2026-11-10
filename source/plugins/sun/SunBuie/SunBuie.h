#pragma once

#include "kernel/sun/SunShape.h"


class SunBuie: public SunShape
{
    SO_NODE_HEADER(SunBuie);

public:
    static void initClass();
    SunBuie();
    SoNode* copy(SbBool copyConnections) const;

    vec3d generateRay(Random& rand) const;
    double getThetaMax() const;
    double shape(double theta) const;

    SoSFDouble csr;

    NAME_ICON_FUNCTIONS("Buie", ":/SunBuie.png")

protected:
     ~SunBuie();

     double chiValue(double csr) const;
     double phi(double theta) const;
     double pdfTheta(double theta) const;
     double zenithAngle(Random& rand) const;
     void updateState(double csrValue);

	 double m_chi;
	 double m_k;
	 double m_gamma;
     double m_exp;

     double m_thetaSD; // solar disk
     double m_thetaCS; // circumsolar
     double m_deltaThetaCSSD; // difference

	 double m_alpha;
     double m_heightSD;
     double m_heightCS;
     double m_probabilitySD;

     static const double s_csrMin;
     static const double s_csrMax;

     SoNodeSensor* m_sensor;
     static void onSensor(void* data, SoSensor*);
};


class SunBuieFactory:
    public QObject, public SunFactoryT<SunBuie>
{
    Q_OBJECT
    Q_INTERFACES(SunFactory)
    Q_PLUGIN_METADATA(IID "tonatiuh.SunFactory" FILE "../../tonatiuhpp-headless-plugin.json")
};
