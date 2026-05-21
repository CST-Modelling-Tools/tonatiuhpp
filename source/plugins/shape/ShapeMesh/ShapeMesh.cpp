#include "ShapeMesh.h"

#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <QFileInfo>
#include <QMessageBox>

#include "kernel/profiles/ProfileRT.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/DifferentialGeometry.h"
#include "libraries/math/3D/Box3D.h"
#include "libraries/math/3D/Ray.h"
#include "libraries/auxiliary/tiny_obj_loader.h"
#include "kernel/node/TonatiuhFunctions.h"
using gcf::pow2;

SO_NODE_SOURCE(ShapeMesh)


void ShapeMesh::initClass()
{
    SO_NODE_INIT_CLASS(ShapeMesh, ShapeRT, "ShapeRT");
}

ShapeMesh::ShapeMesh()
{  
    m_bvh = 0;
    SO_NODE_CONSTRUCTOR(ShapeMesh);
    isBuiltIn = TRUE;

//    float vs[][3] = {
//        {1., 0., 0.},
//        {0., 1., 0.},
//        {0., 0., 0.}
//    };
//    vertices.setValues(0, 3, vs);
//    vertices.setContainer(this);
//    fieldData->addField(this, "vertices", &vertices);

//    float ns[][3] = {
//        {0., 0., 1.},
//        {0., 0., 1.},
//        {0., 0., 1.}
//    };
//    normals.setValues(0, 3, ns);
//    normals.setContainer(this);
//    fieldData->addField(this, "normals", &normals);

    SO_NODE_ADD_FIELD( file, ("") );
    SO_NODE_ADD_FIELD( group, ("") );

    m_sensor = QSharedPointer<SoNodeSensor>::create(onSensor, this);
    m_sensor->setPriority(0);
    m_sensor->attach(this);
    onSensor(this, 0);
}

Box3D ShapeMesh::getBox(ProfileRT* profile) const
{
    Q_UNUSED(profile)
    if (m_bvh) return m_bvh->box();
    return Box3D();
}

bool ShapeMesh::intersect(const Ray& ray, double* tHit, DifferentialGeometry* dg, ProfileRT* profile) const
{  
    Q_UNUSED(profile)
    if (!m_bvh) return false;
    double tHitT = ray.tMax;
    DifferentialGeometry dgT;
    if (!m_bvh->intersect(ray, &tHitT, &dgT)) return false;

    if (tHit == 0 && dg == 0) return true;
    if (tHit == 0 || dg == 0) gcf::SevereError( "ShapeMesh::intersect");

    *tHit = tHitT;
    *dg = dgT;
    dg->shape = this;
    return true;
}

#include "kernel/scene/MaterialGL.h"
void ShapeMesh::updateShapeGL(TShapeKit* parent)
{
    SoShapeKit* shapeKit = parent->m_shapeKit;


    SoCoordinate3* sVertices = new SoCoordinate3;
    sVertices->point.setValues(0, vertices.getNum(), vertices.getValues(0));
    shapeKit->setPart("coordinate3", sVertices);

    SoNormal* sNormals = new SoNormal;
    sNormals->vector.setValues(0, normals.getNum(), normals.getValues(0));
    MaterialGL* mGL = (MaterialGL*) parent->material.getValue();
    if (mGL->reverseNormals.getValue()) {
         for (int n = 0; n < normals.getNum(); ++n) {
            const SbVec3f& v = *sNormals->vector.getValues(n);
            sNormals->vector.set1Value(n, -v[0], -v[1], -v[2]);
        }
    }
    shapeKit->setPart("normal", sNormals);

    //    SoShapeHints* sHints = new SoShapeHints;
    //    sHints->shapeType = SoShapeHints::SOLID;
    //    sHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    //    sHints->creaseAngle = 30*gcf::degree;
    //    shapeKit->setPart("shapeHints", sHints);


    SoNodeKitListPart* childList = (SoNodeKitListPart*) shapeKit->getPart("childList", true);
//      qDebug() << "cmmmbeh = " << childList->getNumChildren();
    while (childList->getNumChildren() > 0) {
//        qDebug() << "ch = " << childList->getNumChildren();
        childList->removeChild(childList->getNumChildren() - 1);
    }


    //    SoGroup* cont = (SoGroup*) ((*childList->getChildren())[0]);
//    cont->removeAllChildren();
//    SoNodeKitListPart* childList = new SoNodeKitListPart;
//    shapeKit->setPart("childList", childList);

    for (SoIndexedFaceSet* fs : m_faceSets) {
        SoShapeKit* shapePart = new SoShapeKit;
        shapePart->pickCulling = SoShapeKit::ON;
//        qDebug() << "name = " << fs->getName().getString();
        shapePart->setName(fs->getName());
        shapePart->setPart("shape", fs);
        childList->addChild(shapePart);
    }
    shapeKit->setPart("shape", new SoIndexedFaceSet); // hide default cube
}

ShapeMesh::~ShapeMesh()
{
    delete m_bvh;
    for (Triangle* t : m_triangles)
        delete t;
}

#include <QDir>
void ShapeMesh::onSensor(void* data, SoSensor*)
{
    ShapeMesh* shape = (ShapeMesh*) data;
    shape->vertices.deleteValues(0); // todo move
    shape->normals.deleteValues(0);
    shape->m_faceSets.clear();

    QString fileName = shape->file.getValue().getString();
    if (fileName.isEmpty()) return;
    QString groupName = shape->group.getValue().getString();

    fileName = QString("project:") + fileName;
    QFileInfo info(fileName);

    if (info.suffix() != "obj") {
        QMessageBox::warning(0, "Warning", "File is not in obj-format");
        return;
    }
    if (!info.exists()) {
        QMessageBox::warning(0, "Warning", QString("File not found:\n") + fileName);
        return;
    }
    fileName = info.absoluteFilePath();


    // import
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warnings;
    std::string errors;

    bool returnCode = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, fileName.toLatin1().data());
    if (!returnCode) return;


    // mesh for rendering
    shape->vertices.setValues(0, attrib.vertices.size()/3, (SbVec3f*) attrib.vertices.data());
    shape->normals.setValues(0, attrib.normals.size()/3, (SbVec3f*) attrib.normals.data());

//    int nMax = attrib.normals.size()/3;
//    shape->normals.setNum(nMax);
//    for (int n = 0, m = 0; n < nMax; n++, m += 3) {
//        SbVec3f nv(&attrib.normals[m]);
//        nv.normalize();
//        shape->normals.set1Value(n, nv);
//    }

    for (auto& shapeGroup : shapes) {
        if (!groupName.isEmpty() && groupName != shapeGroup.name.c_str())
            continue;
        tinyobj::mesh_t& mesh = shapeGroup.mesh;

        SoMFInt32 facesVertices;
        SoMFInt32 facesNormals;
        int nMax = mesh.num_face_vertices.size()*4; // approx
        facesVertices.setNum(nMax);
        facesNormals.setNum(nMax);

        int n = 0;
        size_t v0 = 0;
        for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
            uchar vMax = mesh.num_face_vertices[f]; // 3 or more
            for (size_t v = 0; v < vMax; v++) {
                const tinyobj::index_t& index = mesh.indices[v0 + v];
                facesVertices.set1Value(n, index.vertex_index);
                facesNormals.set1Value(n, index.normal_index);
                n++;
            }
            facesVertices.set1Value(n, -1);
            facesNormals.set1Value(n, -1);
            n++;
            v0 += vMax;
        }

        SoIndexedFaceSet* faceSet = new SoIndexedFaceSet;
        faceSet->ref();
        faceSet->setName(shapeGroup.name.c_str());
        faceSet->coordIndex.setValues(0, facesVertices.getNum(), facesVertices.getValues(0));
        faceSet->normalIndex.setValues(0, facesNormals.getNum(), facesNormals.getValues(0));
        shape->m_faceSets << faceSet;
    }


    // mesh for raytracing
    // quad facet are not triangulated!
    for (Triangle* t : shape->m_triangles)
        delete t;
    shape->m_triangles.clear();

    for (auto& shapeGroup : shapes) {
        if (!groupName.isEmpty() && groupName != shapeGroup.name.c_str())
            continue;
         tinyobj::mesh_t& mesh = shapeGroup.mesh;

         size_t v0 = 0;
         for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
             int vMax = mesh.num_face_vertices[f];
             const tinyobj::index_t& i0 = mesh.indices[v0];
             const tinyobj::index_t& i1 = mesh.indices[v0 + 1];
             const tinyobj::index_t& i2 = mesh.indices[v0 + 2];

             vec3d vA(&attrib.vertices[3*i0.vertex_index]);
             vec3d vB(&attrib.vertices[3*i1.vertex_index]);
             vec3d vC(&attrib.vertices[3*i2.vertex_index]);
             vec3d nA(&attrib.normals[3*i0.normal_index]);
             vec3d nB(&attrib.normals[3*i1.normal_index]);
             vec3d nC(&attrib.normals[3*i2.normal_index]);
             Triangle* triangle = new Triangle(vA, vB, vC, nA, nB, nC);
             shape->m_triangles.push_back(triangle);
             v0 += vMax;
         }
    }

    if (shape->m_bvh) delete shape->m_bvh;
    shape->m_bvh = new BVH(&shape->m_triangles, 1);
}
