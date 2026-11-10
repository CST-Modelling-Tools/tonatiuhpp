#pragma once

#include "kernel/material/MaterialRT.h"


class MaterialSpecular: public MaterialRT
{
    SO_NODE_HEADER(MaterialSpecular);

public:
    enum Distribution {
        pillbox,
        Gaussian
    };

    static void initClass();
    MaterialSpecular();

    bool OutputRay(const Ray& rayIn, const DifferentialGeometry& dg, Random& rand, Ray& rayOut) const;

    SoSFDouble reflectivity;
    SoSFEnum distribution;
    SoSFDouble slope;

    NAME_ICON_FUNCTIONS("Specular", ":/MaterialSpecular.png")

protected:
    ~MaterialSpecular();

    SoNodeSensor* m_sensor;
    static void onSensor(void* data, SoSensor*);
};


class MaterialSpecularFactory:
    public QObject, public MaterialFactoryT<MaterialSpecular>
{
    Q_OBJECT
    Q_INTERFACES(MaterialFactory)
    Q_PLUGIN_METADATA(IID "tonatiuh.MaterialFactory" FILE "../../tonatiuhpp-headless-plugin.json")
};
