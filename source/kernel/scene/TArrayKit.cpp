#include "TArrayKit.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedFaceSetT.h>
#include <Inventor/nodes/SoTransformSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMultipleCopy.h>

#include <Inventor/elements/SoGLVBOElement.h>

#include "kernel/scene/MaterialGL.h"
//#include "kernel/scene/TIndexedFaceSet.h"

//SO_KIT_SOURCE(TArrayKit)
SO_NODE_SOURCE(TArrayKit)



void TArrayKit::initClass()
{
//    SO_KIT_INIT_CLASS(TArrayKit, SoNode, "Node");
    SO_NODE_INIT_CLASS(TArrayKit, SoNode, "Node");
    SoIndexedFaceSetT::initClass();
//    TIndexedFaceSet::initClass();
}

TArrayKit::TArrayKit()
{
//    SO_KIT_CONSTRUCTOR(TArrayKit);
    SO_NODE_CONSTRUCTOR(TArrayKit);

    float vs[][3] = {
        {1, 0, 0},
        {0, 1, 0},
    };
    positions.setValues(0, 2, vs);
    positions.setContainer(this);
    fieldData->addField(this, "positions", &positions);

    SO_NODE_ADD_FIELD(nMax, (5));

    m_vs_matrixViewModel = 0;

    m_root = new SoSeparator;
    m_root->ref();
    m_sensor = new SoFieldSensor(onSensor, this);
//    m_sensor->attach(&positions);
    m_sensor->attach(&nMax);
    m_parent = 0;
    onSensor();
     enableNotify(true);
}

TArrayKit::~TArrayKit()
{
    m_root->unref();
}

void TArrayKit::onSensor(void* data, SoSensor*)
{
    TArrayKit* kit = (TArrayKit*) data;
    kit->onSensor();
}

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoShaderParameter.h>
#include <QDir>

#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/annex/FXViz/nodes/SoShadowStyle.h>
#include <Inventor/nodes/SoLightModel.h>


void TArrayKit::onSensor()
{
    m_root->removeAllChildren();


    SoLightModel* lightmodel = new SoLightModel;
    lightmodel->model = SoLightModel::BASE_COLOR;
    lightmodel->setOverride(TRUE);
    m_root->addChild(lightmodel);

    // shape
    MaterialGL* material = new MaterialGL;
    m_root->addChild(material);

    float sv[][3] = {
        {1., -0.5, 0},
        {0.5, 0.5, 0},
        {-0.5, 0.5, 0},
        {-0.5, -0.5, 0}
    };
    SoCoordinate3* sVertices = new SoCoordinate3;
    sVertices->point.setValues(0, 4, sv);
//    sVertices->point.setValues(0, 20, sv); // for vbo min 20
    m_root->addChild(sVertices);



//    float sn[][3] = {
//        {0., 0., 1.},
//        {0., 0., 1.},
//        {0., 0., 1.},
//        {0., 0., 1.}
//    };
//    SoNormal* sNormals = new SoNormal;
//    sNormals->vector.setValues(0, 4, sn);
////    sVertices->point.setValues(0, 20, sv);
//    m_root->addChild(sNormals);



    // shader
    SoVertexShader* vs = new SoVertexShader;
    vs->sourceProgram = QFileInfo("resources:/shaders/light_vs.glsl").filePath().toLatin1().data();

    SoFragmentShader* fs = new SoFragmentShader;
    fs->sourceProgram = QFileInfo("resources:/shaders/light_fs.glsl").filePath().toLatin1().data();

    m_vs_matrixViewModel = new SoShaderParameterMatrix;
    m_vs_matrixViewModel->name = "matrixViewModel";
    m_vs_matrixViewModel->ref();
//    vs->parameter.addNode(m_matrixViewModel);

    m_vs_matrixProjectionViewModel = new SoShaderParameterMatrix;
    m_vs_matrixProjectionViewModel->name = "matrixProjectionViewModel";
    m_vs_matrixProjectionViewModel->ref();
    vs->parameter.addNode(m_vs_matrixProjectionViewModel);

    m_vs_stride = new SoShaderParameter1i;
    m_vs_stride->name = "stride";
    m_vs_stride->ref();
    vs->parameter.addNode(m_vs_stride);


    SoShaderProgram* shaderProgram = new SoShaderProgram;
    shaderProgram->shaderObject.addNode(vs);
    shaderProgram->shaderObject.addNode(fs);

    m_root->addChild(shaderProgram);

//sstyles
//    SoShapeHints* sHintsB = new SoShapeHints;
//    sHintsB->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
//    m_root->addChild(sHintsB);

//    SoShadowStyle* style = new SoShadowStyle;
//    style->style = SoShadowStyleElement::NO_SHADOWING;
//   m_root->addChild(style);

    // mesh
    int faces[] = {// for VBO 20 indices
        0, 1, 2, -1,
        2, 3, 0, -1
    };
    m_mesh = new SoIndexedFaceSetT;
    m_mesh->coordIndex.setValues(0, 2*4, faces);
//    sMesh->normalIndex.setValues(0, 5*4, faces);

    // instances
//    int kMax = nMax.getValue();
//    int mMax = kMax;
//    for (int n = 0; n < kMax; ++n)
//        for (int m = 0; m < mMax; ++m) {
//            SoTransformSeparator* sep = new SoTransformSeparator;
//            SoTransform* transform = new SoTransform;
//            transform->translation = SbVec3f(n*3, m*3, 0);
//            sep->addChild(transform);
//            sep->addChild(sMesh);
//            m_root->addChild(sep);
//        }

//    SoMultipleCopy* mc = new SoMultipleCopy;
//    mc->addChild(sMesh);
//    int counter = 0;
//    for (int n = 0; n < kMax; ++n)
//        for (int m = 0; m < mMax; ++m) {
//            SbMatrix matrix;
//            matrix.setTranslate(SbVec3f(n*3, m*3, 0));
//            mc->matrix.set1Value(counter++, matrix);
//        }
//    m_root->addChild(mc);
    m_root->addChild(m_mesh);
}

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
void TArrayKit::getBoundingBox(SoGetBoundingBoxAction* action)
{
    float sep = 2.;
    int stride = int(sqrt(nMax.getValue()));
    float q = stride*sep;
    SbBox3f box(-sep, -sep, -sep, q, q, sep);
    action->extendBy(box);
}
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>

#include <QDebug>
void TArrayKit::GLRender(SoGLRenderAction* action)
{

    SoState * state = action->getState();

//    int major, minor;
//        SoGLCacheContextElement::getOpenGLVersion(state, major, minor);
//        qDebug() << "opengl "<< major << minor;
    SbMatrix matrixModel = SoModelMatrixElement::get(state).getValue();
    SbMatrix matrixView = SoViewingMatrixElement::get(state).getValue();
    SbMatrix matrixProjection = SoProjectionMatrixElement::get(state).getValue();

    if (m_vs_matrixViewModel) {
        m_vs_matrixViewModel->value = matrixModel*matrixView; // reversed order
        m_vs_matrixProjectionViewModel->value = matrixModel*matrixView*matrixProjection;
        m_vs_stride->value = int(sqrt(nMax.getValue()));
        m_mesh->m_renderCount = nMax.getValue();
    }

    m_root->GLRender(action);
}
