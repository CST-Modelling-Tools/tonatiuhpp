#pragma once

#include "kernel/material/MaterialRT.h"


class TONATIUH_KERNEL MaterialFresnelUnpolarized: public MaterialRT
{
    SO_NODE_HEADER(MaterialFresnelUnpolarized);

public:
    enum Distribution {
        pillbox,
        Gaussian
    };

    static void initClass();
    MaterialFresnelUnpolarized();

    SoSFDouble nFront;
    SoSFDouble nBack;

    SoSFEnum distribution;
    SoSFDouble slope;


    NAME_ICON_FUNCTIONS("Fresnel (unpolarized)", ":/material/MaterialFresnel.png")

protected:
    MaterialProbabilities probabilities(const Ray&, const DifferentialGeometry&,
        Random&, MaterialInteractionState&) const override;
    void scatter(MaterialInteraction, const Ray&, const DifferentialGeometry&,
        Random&, const MaterialInteractionState&, Ray&) const override;

    ~MaterialFresnelUnpolarized() {}
};

