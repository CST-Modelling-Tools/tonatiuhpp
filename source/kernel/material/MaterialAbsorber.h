#pragma once

#include "kernel/material/MaterialRT.h"


class TONATIUH_KERNEL MaterialAbsorber: public MaterialRT
{
    SO_NODE_HEADER(MaterialAbsorber);

public:
    static void initClass();
    MaterialAbsorber();


    NAME_ICON_FUNCTIONS("Absorber", ":/material/MaterialAbsorber.png")

protected:
    MaterialProbabilities probabilities(const Ray&, const DifferentialGeometry&,
        Random&, MaterialInteractionState&) const override;
    void scatter(MaterialInteraction, const Ray&, const DifferentialGeometry&,
        Random&, const MaterialInteractionState&, Ray&) const override;

    ~MaterialAbsorber() {}
};
