#pragma once

#include "kernel/material/MaterialRT.h"


class TONATIUH_KERNEL MaterialRough: public MaterialRT
{
    SO_NODE_HEADER(MaterialRough);

public:
    enum Distribution {
        Beckmann, // Beckmann–Spizzichino, similar to Gaussian
        Trowbridge // Trowbridge–Reitz, similar to Lorentzian
    };

    static void initClass();
    MaterialRough();


    SoSFDouble diffuse;
    SoSFDouble specular;
    SoSFEnum distribution;
    SoSFDouble roughness;

    NAME_ICON_FUNCTIONS("Rough", ":/material/MaterialRough.png")

protected:
    MaterialProbabilities probabilities(const Ray&, const DifferentialGeometry&,
        Random&, MaterialInteractionState&) const override;
    void scatter(MaterialInteraction, const Ray&, const DifferentialGeometry&,
        Random&, const MaterialInteractionState&, Ray&) const override;

    ~MaterialRough() {}
};

