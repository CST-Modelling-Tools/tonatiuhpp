#pragma once

#include "kernel/shape/ShapeRT.h"


class ShapeHyperbolic: public ShapeRT
{
    SO_NODE_HEADER(ShapeHyperbolic);

public:
    static void initClass();
    ShapeHyperbolic();

    vec3d getPoint(double u, double v) const;
    vec3d getNormal(double u, double v) const;
    Box3D getBox(ProfileRT* profile) const;
    bool intersect(const Ray& ray, double* tHit, DifferentialGeometry* dg, ProfileRT* aperture) const;

    SoSFDouble aX;
    SoSFDouble aY;
    SoSFDouble aZ;

    NAME_ICON_FUNCTIONS("Hyperbolic", ":/ShapeHyperbolic.png")
    void updateShapeGL(TShapeKit* parent);
};



class ShapeHyperbolicFactory:
    public QObject, public ShapeFactoryT<ShapeHyperbolic>
{
    Q_OBJECT
    Q_INTERFACES(ShapeFactory)
    Q_PLUGIN_METADATA(IID "tonatiuh.ShapeFactory" FILE "../../tonatiuhpp-headless-plugin.json")
};

// main equation
// (z/rZ + 1)^2 = 1 + (x/rX)^2 + (y/rY)^2

// points
// z/rZ = sqrt[1 + (x/rX)^2 + (y/rY)^2] - 1

// dpdx
// z/rZ = x/rX^2/sqrt[1 + (x/rX)^2 + (y/rY)^2]

// normals
// [-x/rx^2, -y/rY^2, (z/rZ + 1)/rZ]

// relation to parabola
// z/rZ = [(x/rX)^2 + (y/rY)^2]/[1 + sqrt[1 + (x/rX)^2 + (y/rY)^2]]
// 4*fX*z = x^2
// fX = rX^2/(2*rZ)
