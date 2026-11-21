#pragma once

#include "kernel/material/MaterialRT.h"


class TONATIUH_KERNEL MaterialVirtual: public MaterialRT
{
	SO_NODE_HEADER(MaterialVirtual);

public:
    static void initClass();
    MaterialVirtual();


    NAME_ICON_FUNCTIONS("Virtual", ":/material/MaterialVirtual.png")

protected:
    MaterialProbabilities probabilities(const Ray&, const DifferentialGeometry&,
        Random&, MaterialInteractionState&) const override;
    void scatter(MaterialInteraction, const Ray&, const DifferentialGeometry&,
        Random&, const MaterialInteractionState&, Ray&) const override;

    ~MaterialVirtual() {}
};
