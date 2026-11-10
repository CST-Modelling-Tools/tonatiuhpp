#pragma once

#include <QSharedPointer>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoSFVec2i32.h>

#include "kernel/shape/ShapeRT.h"
#include "libraries/math/3D/Box3D.h"
#include "BVH.h"


class ShapeFunctionZ: public ShapeRT
{
    SO_NODE_HEADER(ShapeFunctionZ);

public:
    static void initClass();
    ShapeFunctionZ();

    Box3D getBox(ProfileRT* profile) const;
    bool intersect(const Ray& ray, double* tHit, DifferentialGeometry* dg, ProfileRT* profile) const;

    SoSFString functionZ;
    SoSFVec2i32 dims;

    QVector<SbVec3f> vertices;
    QVector<SbVec3f> normals;
    QVector<int> faces;

    NAME_ICON_FUNCTIONS("FunctionZ", ":/ShapeFunctionZ.png")
    void updateShapeGL(TShapeKit* parent);

protected:
    ~ShapeFunctionZ();

    std::vector<Triangle*> m_triangles;
    BVH* m_bvh;

    void buildMesh(TShapeKit* parent);
};



class ShapeFunctionZFactory:
    public QObject, public ShapeFactoryT<ShapeFunctionZ>
{
    Q_OBJECT
    Q_INTERFACES(ShapeFactory)
    Q_PLUGIN_METADATA(IID "tonatiuh.ShapeFactory" FILE "../../tonatiuhpp-headless-plugin.json")
};

// https://en.wikipedia.org/wiki/Wavefront_.obj_file
