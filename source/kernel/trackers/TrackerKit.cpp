#include "TrackerKit.h"

#include "TrackerArmature2A.h"
#include "TrackerTarget.h"
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <QFileInfo>
#include "kernel/node/TonatiuhDiagnostics.h"
#include <Inventor/sensors/SoFieldSensor.h>

#include "libraries/auxiliary/tiny_obj_loader.h"


SO_KIT_SOURCE(TrackerKit)


void TrackerKit::initClass()
{
    SO_KIT_INIT_CLASS(TrackerKit, SoBaseKit, "BaseKit");

    TrackerArmature::initClass();
    TrackerTarget::initClass();
}

TrackerKit::TrackerKit()
{
    SO_KIT_CONSTRUCTOR(TrackerKit);
    isBuiltIn = TRUE;

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoGroup, FALSE, this, "", FALSE);

    SO_NODE_ADD_FIELD( enabled, (TRUE) );
    SO_NODE_ADD_FIELD( armature, (0) );
    SO_NODE_ADD_FIELD( shape, ("") );
    SO_NODE_ADD_FIELD( target, (0) );

    SO_KIT_INIT_INSTANCE();

    m_parent = 0;
    m_shapeKit = new SoShapeKit;
    m_shapeKit->ref();
    m_shapeKit->setPart("shape", new SoIndexedFaceSet); // hide default cube

//    m_shapeKit->setSearchingChildren(TRUE);
    SoGroup* g = (SoGroup*) topSeparator.getValue();
    g->addChild(m_shapeKit);

    armature = new TrackerArmature2A;
    target = new TrackerTarget;

    m_sensor_shape = new SoFieldSensor(onSensor_shape, this);
    m_sensor_shape->attach(&shape);

    m_sensor_target = new SoFieldSensor(onSensor_target, this);
//    m_sensor_shape->setPriority(0);
    m_sensor_target->attach(&target);
}

void TrackerKit::update(TSeparatorKit* parent, const Transform& toGlobal, const vec3d& vSun)
{
    if (!enabled.getValue()) return;
    TrackerArmature* ta = (TrackerArmature*) armature.getValue();
    TrackerTarget* tt = (TrackerTarget*) target.getValue();
    ta->update(parent, toGlobal, vSun, tt);
    onSensor_target(this, 0);
}

TrackerKit::~TrackerKit()
{
    delete m_sensor_target;
}

void TrackerKit::setDefaultOnNonWritingFields()
{
    topSeparator.setDefault(TRUE);
    SoBaseKit::setDefaultOnNonWritingFields();
}

void TrackerKit::onSensor_shape(void* data, SoSensor*)
{
    TrackerKit* kit = (TrackerKit*) data;

    SoMFVec3f vertices;
    SoMFVec3f normals;
    QVector<SoIndexedFaceSet*> m_faceSets;

    QString fileName = kit->shape.getValue().getString();
    if (fileName.isEmpty()) return;

    fileName = QString("project:") + fileName;
    QFileInfo info(fileName);
    if (info.suffix() != "obj") {
        tonatiuhReportWarning("Warning", "File is not in obj-format");
        return;
    }
    if (!info.exists()) {
        tonatiuhReportWarning("Warning", QString("File not found:\n") + fileName);
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
    vertices.setValues(0, attrib.vertices.size()/3, (SbVec3f*) attrib.vertices.data());
    normals.setValues(0, attrib.normals.size()/3, (SbVec3f*) attrib.normals.data());

//    int nMax = attrib.normals.size()/3;
//    shape->normals.setNum(nMax);
//    for (int n = 0, m = 0; n < nMax; n++, m += 3) {
//        SbVec3f nv(&attrib.normals[m]);
//        nv.normalize();
//        shape->normals.set1Value(n, nv);
//    }

    for (auto& shapeGroup : shapes) {
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
        m_faceSets << faceSet;
    }

    //
    SoCoordinate3* sVertices = new SoCoordinate3;
    sVertices->point.setValues(0, vertices.getNum(), vertices.getValues(0));
    kit->m_shapeKit->setPart("coordinate3", sVertices);

    SoNormal* sNormals = new SoNormal;
    sNormals->vector.setValues(0, normals.getNum(), normals.getValues(0));
    if (true) { // reverse normals
         for (int n = 0; n < normals.getNum(); ++n) {
            const SbVec3f& v = *sNormals->vector.getValues(n);
            sNormals->vector.set1Value(n, -v[0], -v[1], -v[2]);
        }
    }
    kit->m_shapeKit->setPart("normal", sNormals);

    //    SoShapeHints* sHints = new SoShapeHints;
    //    sHints->shapeType = SoShapeHints::SOLID;
    //    sHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    //    sHints->creaseAngle = 30*gcf::degree;
    //    shapeKit->setPart("shapeHints", sHints);

    SoNodeKitListPart* childList = (SoNodeKitListPart*) kit->m_shapeKit->getPart("childList", true);
    while (childList->getNumChildren() > 0)
        childList->removeChild(childList->getNumChildren() - 1);

    for (SoIndexedFaceSet* fs : m_faceSets) {
        SoShapeKit* shapePart = new SoShapeKit;
        shapePart->pickCulling = SoShapeKit::ON;
        shapePart->setName(fs->getName());
        shapePart->setPart("shape", fs);
        childList->addChild(shapePart);
    }
}

void TrackerKit::onSensor_target(void* data, SoSensor*)
{
    TrackerKit* kit = (TrackerKit*) data;
    if (!kit->m_parent) return;
    TrackerArmature* ta = (TrackerArmature*) kit->armature.getValue();
    TrackerTarget* tt = (TrackerTarget*) kit->target.getValue();
    ta->updateShape(kit->m_parent, kit->m_shapeKit, tt);
}
