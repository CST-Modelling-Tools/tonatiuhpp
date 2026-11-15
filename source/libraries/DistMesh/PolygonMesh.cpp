#include "PolygonMesh.h"

#include "libraries/DistMesh/distmesh.h"

static vec2d makeVector2D(const QPointF& p)
{
    return vec2d(p.x(), p.y());
}

PolygonMesh::PolygonMesh(const QPolygonF& polygon)
{
    m_polygon = polygon;
}

bool PolygonMesh::makeMesh(double step)
{
    for (int n = 0, m = m_polygon.size() - 1; n < m_polygon.size(); m = n++) {
        vec2d prev = makeVector2D(m_polygon[m]);
        vec2d next = makeVector2D(m_polygon[n]);
        double se = 0.3*(next - prev).norm();
        if (se < step) step = se;
    }

//    QVector<vec2d> fixed;
//    for (int n = 0, m = m_polygon.size() - 1; n < m_polygon.size(); m = n++) {
//        vec2d prev = makeVector2D(m_polygon[m]);
//        vec2d next = makeVector2D(m_polygon[n]);
//        vec2d diff = next - prev;
//        int qMax = std::ceil(diff.norm()/step/2);
//        diff /= qMax;
//        for (int q = 0; q < qMax; ++q)
//            fixed << prev + diff*q;
//    }

//    Eigen::ArrayXXd fixedPoints(fixed.size(), 2);
//    for (int n = 0; n < fixed.size(); ++n) {
//        fixedPoints(n, 0) = fixed[n].x;
//        fixedPoints(n, 1) = fixed[n].y;
//    }
//    QRectF rect = m_polygon.boundingRect();
//    double w = std::min(rect.width(), rect.height());
//    step = 0.9*w*2/sqrt(3);

    try {
        Eigen::ArrayXXd polygon(m_polygon.size(), 2);
        for (int n = 0; n < m_polygon.size(); ++n) {
            polygon(n, 0) = m_polygon[n].x();
            polygon(n, 1) = m_polygon[n].y();
        }

        Eigen::ArrayXXd boundingBox(2, 2);
        QRectF rect = m_polygon.boundingRect();
        // top gives min y
        boundingBox << rect.left(), rect.top(),
                       rect.right(), rect.bottom();

        Eigen::ArrayXXd points;
        Eigen::ArrayXXi elements;
        std::tie(points, elements) = distmesh::distmesh(
            distmesh::distanceFunction::polygon(polygon),
            step,
            1.,
            boundingBox,
            polygon);

        // convert
        m_points.clear();
        for (int n = 0; n < points.rows(); ++n)
            m_points << vec2d(points(n, 0), points(n, 1));

        m_triangles.clear();
        for (int n = 0; n < elements.rows(); ++n)
            m_triangles << Triangle(elements(n, 0), elements(n, 1), elements(n, 2));

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool PolygonMesh::makeMesh(double step, MeshDensity& mds)
{
    step = mds(0., 0.);
    for (int n = 0, m = m_polygon.size() - 1; n < m_polygon.size(); m = n++) {
        vec2d prev = makeVector2D(m_polygon[m]);
        vec2d next = makeVector2D(m_polygon[n]);
        double se = 0.3*(next - prev).norm();
        if (se < step) step = se;
    }

    distmesh::Functional funcStep(1.0);


    try {
        Eigen::ArrayXXd polygon(m_polygon.size(), 2);
        for (int n = 0; n < m_polygon.size(); ++n) {
            polygon(n, 0) = m_polygon[n].x();
            polygon(n, 1) = m_polygon[n].y();
        }

        Eigen::ArrayXXd boundingBox(2, 2);
        QRectF rect = m_polygon.boundingRect();
        // top gives min y
        boundingBox << rect.left(), rect.top(),
                       rect.right(), rect.bottom();

        Eigen::ArrayXXd points;
        Eigen::ArrayXXi elements;
        std::tie(points, elements) = distmesh::distmesh(
            distmesh::distanceFunction::polygon(polygon),
            step,
            // 1,
            funcStep,
            boundingBox,
            polygon);

        // convert
        m_points.clear();
        for (int n = 0; n < points.rows(); ++n)
            m_points << vec2d(points(n, 0), points(n, 1));

        m_triangles.clear();
        for (int n = 0; n < elements.rows(); ++n)
            m_triangles << Triangle(elements(n, 0), elements(n, 1), elements(n, 2));

        return true;
    }
    catch (...)
    {
        return false;
    }
}
