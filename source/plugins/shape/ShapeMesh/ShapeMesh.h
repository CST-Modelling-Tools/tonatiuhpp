#pragma once

#include <QSharedPointer>
#include <Inventor/fields/SoMFInt32.h>

#include "kernel/shape/ShapeRT.h"
#include "libraries/math/3D/Box3D.h"
#include "BVH.h"

class SoIndexedFaceSet;

class ShapeMesh: public ShapeRT
{
    SO_NODE_HEADER(ShapeMesh);

public:
    static void initClass();
    ShapeMesh();

    Box3D getBox(ProfileRT* profile) const;
    bool intersect(const Ray& ray, double* tHit, DifferentialGeometry* dg, ProfileRT* profile) const;

    SoMFVec3f vertices;
    SoMFVec3f normals;

    SoSFString file;
    SoSFString group;

    NAME_ICON_FUNCTIONS("Mesh", ":/ShapeMesh.png")
    void updateShapeGL(TShapeKit* parent);

protected:
    ~ShapeMesh();

    QVector<SoIndexedFaceSet*> m_faceSets;
    std::vector<Triangle*> m_triangles;
    BVH* m_bvh;

    QSharedPointer<SoNodeSensor> m_sensor;
    static void onSensor(void* data, SoSensor*);
};



class ShapeMeshFactory:
    public QObject, public ShapeFactoryT<ShapeMesh>
{
    Q_OBJECT
    Q_INTERFACES(ShapeFactory)
    Q_PLUGIN_METADATA(IID "tonatiuh.ShapeFactory" FILE "../../tonatiuhpp-headless-plugin.json")
};
