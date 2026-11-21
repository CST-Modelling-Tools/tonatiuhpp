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


    SoSFDouble reflectivity;
    SoSFEnum distribution;
    SoSFDouble slope;

    NAME_ICON_FUNCTIONS("Specular", ":/MaterialSpecular.png")

protected:
    MaterialProbabilities probabilities(const Ray&, const DifferentialGeometry&,
        Random&, MaterialInteractionState&) const override;
    void scatter(MaterialInteraction, const Ray&, const DifferentialGeometry&,
        Random&, const MaterialInteractionState&, Ray&) const override;

    ~MaterialSpecular();

};


class MaterialSpecularFactory:
    public QObject, public MaterialFactoryT<MaterialSpecular>
{
    Q_OBJECT
    Q_INTERFACES(MaterialFactory)
    Q_PLUGIN_METADATA(IID "tonatiuh.MaterialFactory" FILE "../../tonatiuhpp-headless-plugin.json")
};
