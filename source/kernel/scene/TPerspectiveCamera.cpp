#include "TPerspectiveCamera.h"

#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbViewportRegion.h>
#include <atomic>

#include "libraries/math/gcf.h"
#include <QDebug>
#include <Inventor/SoPickedPoint.h>
#include "libraries/math/3D/Transform.h"
#include "kernel/node/TonatiuhFunctions.h"
//#include "GraphicRoot.h"
#include "kernel/scene/TSeparatorKit.h"
#include "libraries/math/gcf.h"


namespace {
std::atomic<TonatiuhViewerCameraGetter> s_cameraGetter{nullptr};
std::atomic<TonatiuhViewerViewportGetter> s_viewportGetter{nullptr};
}

void tonatiuhSetCameraViewerBridge(TonatiuhViewerCameraGetter camera,
                                    TonatiuhViewerViewportGetter viewport)
{
    s_cameraGetter.store(camera);
    s_viewportGetter.store(viewport);
}

TPerspectiveCamera::TPerspectiveCamera()
{
    m_position = vec3d(0., 0., 1.);
    m_rotation = vec3d(0., 0., 0.);
}

void TPerspectiveCamera::setCamera(SoPerspectiveCamera* camera)
{
    m_camera = camera;
    updateTransform();
}

void TPerspectiveCamera::saveTransform()
{
    m_position0 = m_position;
    m_rotation0 = m_rotation;
}

void TPerspectiveCamera::updateTransform()
{
    m_camera->position.setValue(m_position.x, m_position.y, m_position.z);
    SbRotation rx(SbVec3f(1, 0, 0), (m_rotation.y + 90.)*gcf::degree);
    SbRotation rz(SbVec3f(0, 0, 1), -m_rotation.x*gcf::degree);
    m_camera->orientation = rx*rz;
}

void TPerspectiveCamera::shift(vec3d s)
{
    m_position = m_position0 + s;
    updateTransform();
}

void TPerspectiveCamera::rotate(double dAz, double dEl)
{
    m_rotation.x = m_rotation0.x + dAz;
    m_rotation.y = gcf::clamp(m_rotation0.y + dEl, -89.9, 89.9);
    updateTransform();
}

void TPerspectiveCamera::setRotation(double az, double el)
{
    m_rotation.x = az;
    m_rotation.y = el;
    updateTransform();
}

void TPerspectiveCamera::setRotationAt(vec3d aim)
{
    vec3d d = (aim - m_position).normalized();
    m_rotation.x = atan2(d[0], d[1])/gcf::degree;
    m_rotation.y = asin(d[2])/gcf::degree;
    updateTransform();
}

void TPerspectiveCamera::orbit(double dAz, double dEl)
{
    if (!m_isAnchored) return;

    m_rotation.x = m_rotation0.x + dAz;
    m_rotation.y = gcf::clamp(m_rotation0.y + dEl, -89.9, 89.9);

    vec3d direction = vec3d::directionAE(
        m_rotation.x*gcf::degree,
        m_rotation.y*gcf::degree
    );
    double distance = (m_position0 - m_anchor).norm();
    m_position = m_anchor - distance*direction;

    updateTransform();
}

void TPerspectiveCamera::setOrbit(double az, double el, vec3d anchor)
{
    m_rotation.x = az;
    m_rotation.y = gcf::clamp(el, -89.9, 89.9);

    vec3d direction = vec3d::directionAE(
        m_rotation.x*gcf::degree,
        m_rotation.y*gcf::degree
    );
    double distance = (m_position - anchor).norm();
    m_position = anchor - distance*direction;

    updateTransform();
}

vec3d TPerspectiveCamera::findRayGlobal0()
{
    vec3d rd0(0, 0, -1);
    rd0 = Transform::rotateX((m_rotation.y + 90.)*gcf::degree).transformVector(rd0);
    rd0 = Transform::rotateZ(-m_rotation.x*gcf::degree).transformVector(rd0);
    return rd0;
}

vec3d TPerspectiveCamera::findRayGlobal(SoQtExaminerViewer* /*viewer*/, QPointF pos)
{
//    SbVec2s vs = viewer->getViewportRegion().getViewportSizePixels();
    double x = 2.*pos.x()/double(m_size.width()) - 1;
    double y = 2.*pos.y()/double(m_size.height()) - 1;
    if (m_size.width() > m_size.height())
        x *= m_size.width()/double(m_size.height());
    else
        y *= m_size.height()/double(m_size.width());
    double z = 1./std::tan(m_camera->heightAngle.getValue()/2);
    vec3d rd(x, -y, -z);
    rd.normalize();

    rd = Transform::rotateX((m_rotation.y + 90.)*gcf::degree).transformVector(rd);
    rd = Transform::rotateZ(-m_rotation.x*gcf::degree).transformVector(rd);
    return rd;
}

vec3d TPerspectiveCamera::findRayLocal(SoQtExaminerViewer* /*viewer*/, QPoint pos)
{
//    SbVec2s vs = viewer->getViewportRegion().getViewportSizePixels();
    double x = pos.x()/double(m_size.width())*2 - 1;
    double y = pos.y()/double(m_size.height())*2 - 1;
    if (m_size.width() > m_size.height())
        x *= double(m_size.width())/m_size.height();
    else
        y *= double(m_size.height())/m_size.width();

    double z = 1./std::tan(m_camera->heightAngle.getValue()/2);
    vec3d d0(x, z, -y);
    d0.normalize();
    return d0;
}

bool TPerspectiveCamera::findAnchor(SoQtExaminerViewer* viewer, QPointF pos, SoNode* root, vec3d& anchor)
{
    const auto cameraGetter = s_cameraGetter.load();
    const auto viewportGetter = s_viewportGetter.load();
    if (!viewer || !cameraGetter || !viewportGetter) return false;
    SoCamera* camera = cameraGetter(viewer);
    if (!camera) return false;
    vec3d rd = findRayGlobal(viewer, pos);
    double tMin = camera->nearDistance.getValue();
    tMin = std::min(0.1, tMin);

    SoRayPickAction rpa(viewportGetter(viewer));
//    rpa.setPoint(SbVec2s(pos.x(), pos.y())); // does not work
    rpa.setRay(SbVec3f(m_position.x, m_position.y, m_position.z), SbVec3f(rd.x, rd.y, rd.z), tMin);
    rpa.setRadius(1.);
    rpa.apply(root);

    const SoPickedPoint* picked = rpa.getPickedPoint();
    if (picked) {
        anchor = tgf::makeVector3D(picked->getPoint());
    } else {
        // horizon ro + trd = 0
        double t = -m_position.z/rd.z;
        if (t < tMin) return false;
        anchor = m_position + t*rd;
    }
    return true;
}

bool TPerspectiveCamera::findAnchor0(SoQtExaminerViewer* viewer, SoNode* root, vec3d& anchor)
{
    const auto viewportGetter = s_viewportGetter.load();
    if (!viewer || !viewportGetter) return false;
    SbVec2s vs = viewportGetter(viewer).getViewportSizePixels();
    return findAnchor(viewer, QPoint(vs[0]/2, vs[1]/2), root, anchor);
}

void TPerspectiveCamera::findMoveAnchor(SoQtExaminerViewer* viewer, QPointF pos, SoNode* root)
{
    m_isAnchored = findAnchor(viewer, pos, root, m_anchor);
}

void TPerspectiveCamera::moveShiftAnchor(SoQtExaminerViewer* viewer, QPointF pos, double zoom)
{
    if (!m_isAnchored) return;
    vec3d rd = findRayGlobal(viewer, pos);

    // keep z
    // (ro + trd).z = m_anchor.z
    double t = (m_anchor.z - m_position0.z)/rd.z;
    t *= std::pow(0.7, zoom);
    m_position = m_anchor - t*rd;

    updateTransform();
}

void TPerspectiveCamera::movePanAnchor(SoQtExaminerViewer* viewer, QPoint pos, double /*zoom*/)
{
    if (!m_isAnchored) return;
    vec3d rd = findRayGlobal(viewer, pos);
    vec3d rd0 = findRayGlobal0();

    // keep plane
    // (ro + trd).rd0 = m_anchor.rd0
    double t = dot(m_anchor - m_position0, rd0)/dot(rd, rd0);
    m_position = m_anchor - t*rd;

    updateTransform();
}

void TPerspectiveCamera::findRotationAnchor(SoQtExaminerViewer* viewer, QPoint pos)
{
    m_rotationAnchor = findRayGlobal(viewer, pos);
}

void TPerspectiveCamera::moveRotationAnchor(SoQtExaminerViewer* viewer, QPoint pos)
{
    vec3d d0 = findRayLocal(viewer, pos);

    // find angles
    // Rcamera(gamma, alpha) d0 = d
    vec3d a(0., 0., -1.);
    vec3d b(1., 0., 0.);
//    vec3d k(0., -1., 0.);

    double ma = -m_rotationAnchor.z;
    double mb = d0.x;
    double mk = 1. - ma*ma - mb*mb;
    if (mk < 0) return;
    mk = std::sqrt(mk);
//    if (m_rotationAnchor.y < 0) mk = -mk; // smaller rotations?
    vec3d m(mb, mk, -ma); // y > 0 half-plane, smaller alpha

    double gamma = std::atan2(triple(a, m, m_rotationAnchor), dot(m, m_rotationAnchor) - ma*ma)/gcf::degree;
    double alpha = std::atan2(triple(b, d0, m), dot(m, d0) - mb*mb)/gcf::degree;
    setRotation(gamma, alpha);
}

void TPerspectiveCamera::findOrbitAnchor(SoQtExaminerViewer* viewer, QPoint pos, SoNode* root)
{
    m_isOrbitAnchored = findAnchor(viewer, pos, root, m_orbitAnchor);

    // near or far hemisphere
    double gamma = m_rotation0.x*gcf::degree;
    double alpha = m_rotation0.y*gcf::degree;
    vec3d direction = vec3d::directionAE(gamma, alpha);
    m_useNear = dot(m_anchor - m_orbitAnchor, direction) > 0;
}

void TPerspectiveCamera::moveOrbitAnchor(SoQtExaminerViewer* viewer, QPoint pos)
{
    if (!m_isAnchored) return;
    if (!m_isOrbitAnchored) return;

    vec3d d0 = findRayLocal(viewer, pos);

    // find angles
    //   m_position = m_anchor - distance*direction;
    // Rcamera(gamma, alpha) {d0 - p} = rMouse - rAnchor
    vec3d rMA = m_orbitAnchor - m_anchor;
    double distance = (m_position0 - m_anchor).norm();
    vec3d p(0, distance, 0);
    double t1, t2;
    if (!gcf::solveQuadratic(1., -2.*dot(d0, p), p.norm2() - rMA.norm2(), &t1, &t2))
        return;
    double t = m_useNear ? t1 : t2;
    d0 = t*d0 - p;

    // angles
    vec3d a(0., 0., -1.);
    vec3d b(1., 0., 0.);
//    vec3d k(0., -1., 0.);

    d0.normalize();
    rMA.normalize();
    double ma = -rMA.z;
    double mb = d0.x;
    double mk = 1. - ma*ma - mb*mb;
    if (mk < 0) return;
    mk = std::sqrt(mk); // t1*d0 - p is mostly -y axis
    if (m_useNear) mk = -mk;
    vec3d m(mb, mk, -ma); // y > 0 half-plane, smaller alpha

    double gamma = std::atan2(triple(a, m, rMA), dot(m, rMA) - ma*ma)/gcf::degree;
    double alpha = std::atan2(triple(b, d0, m), dot(m, d0) - mb*mb)/gcf::degree;
    setOrbit(gamma, alpha, m_anchor);
}

void TPerspectiveCamera::zoomCenter(SoQtExaminerViewer* viewer, SoNode* root, double zoom)
{
    // find ray
    const auto cameraGetter = s_cameraGetter.load();
    const auto viewportGetter = s_viewportGetter.load();
    if (!viewer || !cameraGetter || !viewportGetter) return;
    SoCamera* camera = cameraGetter(viewer);
    if (!camera) return;
    vec3d rd = findRayGlobal0();
    double tMin = camera->nearDistance.getValue();
    tMin = std::min(0.1, tMin);

    // pick object
    SoRayPickAction rpa(viewportGetter(viewer));
    rpa.setRadius(1.);
    rpa.setRay(SbVec3f(m_position.x, m_position.y, m_position.z), SbVec3f(rd.x, rd.y, rd.z), tMin);
    rpa.apply(root);

    vec3d anchor;
    const SoPickedPoint* picked = rpa.getPickedPoint();
    if (picked) {
        anchor = tgf::makeVector3D(picked->getPoint());
    } else {
        // horizon ro + trd = 0
        double t = -m_position.z/rd.z;
        if (t > 0) {
            anchor = m_position + t*rd;
        } else
            return;
    }

    vec3d q = anchor - m_position;
    q *= std::pow(0.7, zoom);
    m_position = anchor - q;
//    qDebug() << zoom << " " << m_position;
    updateTransform();
}


// merge with movePanAnchor
void TPerspectiveCamera::moveCameraPlane(SoQtExaminerViewer* viewer, QPointF pos, SoNode* root, double zoom)
{
    m_isAnchored = findAnchor(viewer, pos, root, m_anchor);
    if (!m_isAnchored) return;

    vec3d rd = findRayGlobal0();

    // keep orientation, pan camera
    double t = dot(m_anchor - m_position0, rd);
    t *= std::pow(0.7, zoom);
    m_position = m_anchor - t*rd;

    updateTransform();
}
