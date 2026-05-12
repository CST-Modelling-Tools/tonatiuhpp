#include "GridNode3D.h"

#include <QVector>

#include <Inventor/nodes/SoSeparator.h>

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodekits/SoShapeKit.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoCube.h>

#include <QApplication>
#include <QDir>
#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoShaderParameter.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/annex/FXViz/nodes/SoShadowStyle.h>

#include <Inventor/sensors/SoNodeSensor.h>


GridNode3D::GridNode3D()
{
    m_grid = new SoSwitch;
    m_grid->setName("grid");
    m_grid->whichChild = SO_SWITCH_ALL;
    addChild(m_grid);

    m_sensor = new SoNodeSensor(update, this);
}

GridNode3D::~GridNode3D()
{
    delete m_sensor;
}

void GridNode3D::attach(GridNode* grid)
{
    GridNode* attachedGrid = static_cast<GridNode*>(m_sensor->getAttachedNode());
    if (attachedGrid == grid) {
        if (grid)
            create();
        else
            m_grid->removeAllChildren();
        return;
    }

    if (attachedGrid)
        m_sensor->detach();

    if (!grid) {
        m_grid->removeAllChildren();
        return;
    }

    m_sensor->attach(grid);
    create();
}

void GridNode3D::create()
{
    // grid parameters
    GridNode* grid = (GridNode*) m_sensor->getAttachedNode();
    if (!grid) {
        m_grid->removeAllChildren();
        return;
    }

    double dx = grid->steps.getValue()[0];
    int divsX = grid->divisions.getValue()[0];
    double xMin = grid->min.getValue()[0];
    double xMax = grid->max.getValue()[0];
    int nxMin = floor(xMin/dx);
    int nxMax = ceil(xMax/dx);
    xMin = nxMin*dx;
    xMax = nxMax*dx;

    double dy = grid->steps.getValue()[1];
    int divsY = grid->divisions.getValue()[1];
    double yMin = grid->min.getValue()[1];
    double yMax = grid->max.getValue()[1];
    int nyMin = floor(yMin/dy);
    int nyMax = ceil(yMax/dy);
    yMin = nyMin*dy;
    yMax = nyMax*dy;

    double zMean = (grid->min.getValue()[2] + grid->max.getValue()[2])/2;

    // init
    m_grid->removeAllChildren();
//    m_grid->whichChild = grid->show.getValue() ? SO_SWITCH_ALL : SO_SWITCH_NONE;

    // terrain
    QString fileName = grid->file.getValue().getString();
    if (!fileName.isEmpty()) {
        makeTerrain(fileName);
        return;
    }

    // fill
    if (grid->fill.getValue())
        makeGround(xMin, xMax, yMin, yMax, zMean);

    // grid
    if (grid->grid.getValue())
    {
        // points
        QVector<SbVec3f> pointsMajor;
        QVector<SbVec3f> pointsMinor;
        QVector<int> sizesMajor;
        QVector<int> sizesMinor;

        for (int n = nxMin; n <= nxMax; ++n) {
            double x = n*dx;
            if (n % divsX == 0) {
                if (n == 0) continue;
                pointsMajor << SbVec3f(x, yMin, zMean);
                pointsMajor << SbVec3f(x, yMax, zMean);
                sizesMajor << 2;
            } else {
                pointsMinor << SbVec3f(x, yMin, zMean);
                pointsMinor << SbVec3f(x, yMax, zMean);
                sizesMinor << 2;
            }
        }

        for (int n = nyMin; n <= nyMax; ++n) {
            double y = n*dy;
            if (n % divsY == 0) {
                if (n == 0) continue;
                pointsMajor << SbVec3f(xMin, y, zMean);
                pointsMajor << SbVec3f(xMax, y, zMean);
                sizesMajor << 2;
            } else {
                pointsMinor << SbVec3f(xMin, y, zMean);
                pointsMinor << SbVec3f(xMax, y, zMean);
                sizesMinor << 2;
            }
        }

        // major grid
        SoMaterial* sMaterial = new SoMaterial;
        sMaterial->diffuseColor.setValue(0., 0., 0.);
        sMaterial->transparency = 0.8;
        m_grid->addChild(sMaterial);

        SoCoordinate3* sPoints = new SoCoordinate3;
        sPoints->point.setValues(0, pointsMajor.size(), pointsMajor.data());
        m_grid->addChild(sPoints);

        SoLineSet* sLines = new SoLineSet;
        sLines->numVertices.setValues(0, sizesMajor.size(), sizesMajor.data());
        m_grid->addChild(sLines);

        // minor grid
        sMaterial = new SoMaterial;
        sMaterial->diffuseColor.setValue(0., 0., 0.);
        sMaterial->transparency = 0.95;
        m_grid->addChild(sMaterial);

        sPoints = new SoCoordinate3;
        sPoints->point.setValues(0, pointsMinor.size(), pointsMinor.data());
        m_grid->addChild(sPoints);

        sLines = new SoLineSet;
        sLines->numVertices.setValues(0, sizesMinor.size(), sizesMinor.data());
        m_grid->addChild(sLines);

        // axes
        makeAxes(xMin - dx, xMax + dx, yMin - dy, yMax + dy, zMean);
    }
}

void GridNode3D::makeAxes(double xMin, double xMax, double yMin, double yMax, double zMean)
{
    // axes
    QVector<SbVec3f> points;
    QVector<int> sizes;

    points << SbVec3f(xMin, 0., zMean);
    points << SbVec3f(xMax, 0., zMean);
    sizes << 2;

    points << SbVec3f(0., yMin, zMean);
    points << SbVec3f(0., yMax, zMean);
    sizes << 2;

    SoMaterial* sMaterial = new SoMaterial;
    sMaterial->diffuseColor.setValue(0., 0., 0.);
    sMaterial->transparency = 0.7;
    m_grid->addChild(sMaterial);

    SoDrawStyle* sStyle = new SoDrawStyle;
    sStyle->style = SoDrawStyleElement::LINES;
    sStyle->lineWidth = 1.5;
    m_grid->addChild(sStyle);

    SoCoordinate3* sPoints = new SoCoordinate3;
    sPoints->point.setValues(0, points.size(), points.data());
    m_grid->addChild(sPoints);

    SoLineSet* sLines = new SoLineSet;
    sLines->numVertices.setValues(0, sizes.size(), sizes.data());
    m_grid->addChild(sLines);
}

void GridNode3D::makeGround(double xMin, double xMax, double yMin, double yMax, double zMean)
{
    SoShapeKit* kit = new SoShapeKit;

    float sv[][3] = {
        {float(xMin), float(yMin), float(zMean)},
        {float(xMax), float(yMin), float(zMean)},
        {float(xMax), float(yMax), float(zMean)},
        {float(xMin), float(yMax), float(zMean)}
    };
    SoCoordinate3* sVertices = new SoCoordinate3;
    sVertices->point.setValues(0, 4, sv);
    kit->setPart("coordinate3", sVertices);

    int faces[] = {0, 1, 2, 3, -1};
    SoIndexedFaceSet* sMesh = new SoIndexedFaceSet;
    sMesh->coordIndex.setValues(0, 5, faces);
    kit->setPart("shape", sMesh);

    SoMaterial* material = new SoMaterial;
    material->ambientColor = SbColor(0.85, 0.85, 0.8);
    material->diffuseColor = SbColor(0.05, 0.05, 0.05);
    material->specularColor = SbColor(0.02, 0.02, 0.02);
    material->emissiveColor = SbColor(0, 0, 0);
    material->shininess = 0.1;
//    material->transparency = 0.;
    kit->setPart("material", material);

    // shift fill from grid
    SoPolygonOffset* po = new SoPolygonOffset;
    po->styles = SoPolygonOffset::FILLED;
    po->factor = 1.;
    po->units = 1.;
    m_grid->addChild(po);

    // to show in mesh mode
    SoDrawStyle* drawStyle = new SoDrawStyle;
    drawStyle->style = SoDrawStyleElement::FILLED;
    m_grid->addChild(drawStyle);

    // cull from bottom
    SoShapeHints* hints = new SoShapeHints;
    hints->shapeType = SoShapeHints::SOLID;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    kit->setPart("shapeHints", hints);

    m_grid->addChild(kit);
}

#include "libraries/auxiliary/tiny_obj_loader.h"
#include <QFileInfo>
#include <QMessageBox>

#include <Inventor/nodes/SoDepthBuffer.h>
void GridNode3D::makeTerrain(QString fileName)
{
    GridNode* grid = (GridNode*) m_sensor->getAttachedNode();

    QStringList searchPaths = QDir::searchPaths("project");

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

    SoMFVec3f vertices;
    SoMFVec3f normals;
    SoMFInt32 facesVertices;
    SoMFInt32 facesNormals;

    // import
    std::string inputfile = fileName.toStdString();
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inputfile.c_str());
    if (!ret) return;
    if (shapes.empty()) return;
    tinyobj::mesh_t& mesh = shapes[0].mesh;

    // conversion
    int nMax = attrib.vertices.size()/3;
    vertices.setNum(nMax);
    for (int n = 0, m = 0; n < nMax; n++, m += 3)
        vertices.set1Value(n, &attrib.vertices[m]);

    nMax = attrib.normals.size()/3;
    normals.setNum(nMax);
    for (int n = 0, m = 0; n < nMax; n++, m += 3) {
//        shape->normals.set1Value(n, &attrib.normals[m]);
        SbVec3f nv(&attrib.normals[m]);
        nv.normalize();
        normals.set1Value(n, nv);
    }

    nMax = mesh.num_face_vertices.size()*4;
    facesVertices.setNum(nMax);
    facesNormals.setNum(nMax);
    int n = 0;
    size_t v0 = 0;
    for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
        uchar vMax = mesh.num_face_vertices[f]; // 3?
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

    // shape
    SoShapeKit* kit = new SoShapeKit;

    SoMaterial* material = new SoMaterial;
    material->ambientColor = SbColor(0.85, 0.85, 0.8);
    material->diffuseColor = SbColor(0.05, 0.05, 0.05);
    material->specularColor = SbColor(0.02, 0.02, 0.02);
    material->emissiveColor = SbColor(0, 0, 0);
    material->shininess = 0.1;
    material->transparency = 0.001;
    kit->setPart("material", material);

    SoCoordinate3* sVertices = new SoCoordinate3;
    sVertices->point.setValues(0, vertices.getNum(), vertices.getValues(0));
    kit->setPart("coordinate3", sVertices);

    SoNormal* sNormals = new SoNormal;
    sNormals->vector.setValues(0, normals.getNum(), normals.getValues(0));
    kit->setPart("normal", sNormals);

    SoIndexedFaceSet* sMesh = new SoIndexedFaceSet;
    sMesh->coordIndex.setValues(0, facesVertices.getNum(), facesVertices.getValues(0));
    sMesh->normalIndex.setValues(0, facesNormals.getNum(), facesNormals.getValues(0));
    kit->setPart("shape", sMesh);

    // shaders
    SoVertexShader* vs = new SoVertexShader;
    vs->sourceProgram = QFileInfo("resources:/shaders/terrain_vs.glsl").filePath().toLatin1().data();

    SoFragmentShader* fs = new SoFragmentShader;
    fs->sourceProgram = QFileInfo("resources:/shaders/terrain_fs.glsl").filePath().toLatin1().data();

    SoShaderParameter3f* paramSteps = new SoShaderParameter3f;
    paramSteps->name = "steps";
    paramSteps->value = SbVec3f(grid->steps.getValue());
    fs->parameter.addNode(paramSteps);

    SoShaderParameter3i* paramDivs = new SoShaderParameter3i;
    paramDivs->name = "divs";
    paramDivs->value = grid->divisions.getValue();
    fs->parameter.addNode(paramDivs);

    SoShaderProgram* shaderProgram = new SoShaderProgram;
    shaderProgram->shaderObject.addNode(vs);
    shaderProgram->shaderObject.addNode(fs);

    SoShapeHints* sHintsA = new SoShapeHints;
    sHintsA->shapeType = SoShapeHints::SOLID;
    sHintsA->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
//    sHintsA->creaseAngle = 30*gcf::degree;

    SoShapeHints* sHintsB = new SoShapeHints;
    sHintsB->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    SoShadowStyle* style = new SoShadowStyle;
    style->style = SoShadowStyleElement::NO_SHADOWING;

    SoDepthBuffer* depth = new SoDepthBuffer;
    depth->test = true;
    depth->write = false;
    depth->function = SoDepthBufferElement::LEQUAL;
    depth->range.setValue(0., 1.);

    SoSeparator* sep = new SoSeparator;
    if (grid->fill.getValue()) {
        sep->addChild(sHintsA);
        sep->addChild(kit);
    }
    if (grid->grid.getValue()) {
        sep->addChild(sHintsB);
        sep->addChild(depth);
        sep->addChild(style);
        sep->addChild(shaderProgram);
        sep->addChild(kit);
    }
    m_grid->addChild(sep);
}

void GridNode3D::update(void* data, SoSensor*)
{
    GridNode3D* node = (GridNode3D*) data;
    node->create();
}
