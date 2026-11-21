#pragma once

#include "kernel/material/MaterialRT.h"


class TONATIUH_KERNEL MaterialTransparent: public MaterialRT
{
    SO_NODE_HEADER(MaterialTransparent);

public:
    static void initClass();
    MaterialTransparent();


    NAME_ICON_FUNCTIONS("Transparent", ":/material/MaterialTransparent.png")

protected:
    MaterialProbabilities probabilities(const Ray&, const DifferentialGeometry&,
        Random&, MaterialInteractionState&) const override;
    void scatter(MaterialInteraction, const Ray&, const DifferentialGeometry&,
        Random&, const MaterialInteractionState&, Ray&) const override;

    ~MaterialTransparent() {}
};
