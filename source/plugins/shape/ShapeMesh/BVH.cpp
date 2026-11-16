#include "BVH.h"

#include <algorithm>
#include "kernel/shape/DifferentialGeometry.h"
#include "libraries/math/3D/Ray.h"

namespace {
struct TriangleCenterXLess {
    bool operator()(const Triangle* a, const Triangle* b) const
    { return a->center().x < b->center().x; }
};
struct TriangleCenterYLess {
    bool operator()(const Triangle* a, const Triangle* b) const
    { return a->center().y < b->center().y; }
};
struct TriangleCenterZLess {
    bool operator()(const Triangle* a, const Triangle* b) const
    { return a->center().z < b->center().z; }
};
} // namespace


BVHNode::BVHNode():
    m_isLeaf(false),
    m_index(0),
    m_size(0),
    m_nodeLeft(0),
    m_nodeRight(0)
{

}

BVHNode::~BVHNode()
{
    if (!m_isLeaf) {
        if (m_nodeLeft) delete m_nodeLeft;
        if (m_nodeRight) delete m_nodeRight;
    }
}

void BVHNode::makeLeaf(uint index, uint size)
{
    m_isLeaf = true;
    m_index = index;
    m_size = size;
}




BVH::BVH(std::vector<Triangle*>* triangles, int leafSize):
    m_triangles(triangles),
    m_leafSize(leafSize),
    m_nNodes(0),
    m_nLeafs(0)
{
    Box3D box;
    for (Triangle* t : *m_triangles)
        box << t->box();

    m_nodeRoot = new BVHNode;
    m_nodeRoot->setBox(box);
    m_nNodes++;

    build(0, m_triangles->size(), m_nodeRoot, 0);
}

BVH::~BVH()
{
    delete m_nodeRoot;
}

Box3D BVH::box() const
{
    if (m_nodeRoot) return m_nodeRoot->box();
    return Box3D();
}

bool BVH::intersect(const Ray& ray, double* tHit, DifferentialGeometry* dg) const
{
    if (!m_nodeRoot) return false;
    if (!m_nodeRoot->box().intersect(ray)) return false;

    double t = ray.tMax;
    if (!intersect(m_nodeRoot, ray, &t, dg)) return false;

    if (t < *tHit) {
        *tHit = t;
        return true;
    }
    return false;
}

bool BVH::intersect(BVHNode* node, const Ray& ray, double *tHit, DifferentialGeometry* dg) const
{
    double tHitNode = *tHit;
    bool isIntersection = false;

    if (!node->isLeaf()) {
        BVHNode* firstNode = 0;
        BVHNode* secondNode = 0;

        double t0, t1;
        BVHNode* leftNode = node->nodeLeft();
        if (leftNode) {
            bool intersectedL = leftNode->box().intersect(ray, &t0, &t1);
            if (intersectedL && t0 <= tHitNode)
                firstNode = leftNode;
        }
        BVHNode* rightNode = node->nodeRight();
        if (rightNode) {
            bool intersectedR = rightNode->box().intersect(ray, &t0, &t1);
            if (intersectedR && t0 <= tHitNode)
                secondNode = rightNode;
        }

        if (firstNode) {
            double thitT = tHitNode;
            DifferentialGeometry dgT;
            bool isIntersection1 = intersect(firstNode, ray, &thitT, &dgT);
            if (isIntersection1 && thitT < tHitNode) {
                tHitNode = thitT;
                *tHit = thitT;
                *dg = dgT;
                isIntersection = true;
            }
        }

        if (secondNode) {
            double thitT = tHitNode;
            DifferentialGeometry dgT;
            bool isIntersection2 = intersect(rightNode, ray, &thitT, &dgT);
            if (isIntersection2 && thitT < tHitNode) {
                tHitNode = thitT;
                *tHit = thitT;
                *dg = dgT;
                isIntersection = true;
            }
        }

    } else {
        for (int f = 0; f < node->triangles(); f++) {
            Triangle* triangle = m_triangles->at(node->index() + f);
            if (!triangle) continue;
            double tHitT = tHitNode;
            DifferentialGeometry dgT;
            bool isIntersectionT = triangle->intersect(ray, &tHitT, &dgT);
            if (isIntersectionT && tHitT < tHitNode) {
                tHitNode = tHitT;
                *tHit = tHitT;
                *dg = dgT;
                isIntersection = true;
            }
        }
    }

    return isIntersection;
}

void BVH::build(int indexStart, int indexEnd, BVHNode* parent, int depth)
{
    if (indexEnd - indexStart <= m_leafSize) {
        parent->makeLeaf(indexStart, indexEnd - indexStart);
        m_nLeafs++;
    } else {
        Box3D parentBox = parent->box();
        vec3d parentSize = parentBox.size();
        vec3d parentCenter = parentBox.center();

        int dimension1 = 0;
        double dLength1 = parentSize.x;
        double dMean1 = parentCenter.x;
        if (parentSize.y > dLength1) {
            dimension1 = 1;
            dLength1 = parentSize.y;
            dMean1 = parentCenter.y;
        }
        if (parentSize.z > dLength1) {
            dimension1 = 2;
            dLength1 = parentSize.z;
            dMean1 = parentCenter.z;
        }

        sortTrinagles(indexStart, indexEnd, dimension1);

        Box3D leftBox;
        int splitIndex = indexEnd;
        for (int f = indexStart; f < indexEnd; f++) {
            Triangle* triangle = m_triangles->at(f);
            if (dimension1 == 0 && triangle->center().x > dMean1) {
                splitIndex = f;
                break;
            } else if (dimension1 == 1 && triangle->center().y > dMean1) {
                splitIndex = f;
                break;
            } else if (dimension1 == 2 && triangle->center().z > dMean1) {
                splitIndex = f;
                break;
            }
            leftBox << triangle->box();
        }

        if (splitIndex == indexEnd || splitIndex == indexStart) {
            splitIndex = (indexStart + indexEnd)/2;
            leftBox = Box3D();
            for (int f = indexStart; f < splitIndex; f++) {
                Triangle* triangle = m_triangles->at(f);
                leftBox << triangle->box();
            }
        }

        Box3D rightBox;
        for (int f = splitIndex; f < indexEnd; f++) {
            Triangle* triangle = m_triangles->at(f);
            rightBox << triangle->box();
        }

        BVHNode* leftNode = new BVHNode;
        leftNode->setBox(leftBox);
        parent->setNodeLeft(leftNode);
        m_nNodes++;

        BVHNode* rightNode = new BVHNode;
        rightNode->setBox(rightBox);
        parent->setNodeRight(rightNode);
        m_nNodes++;

        build(indexStart, splitIndex, leftNode, depth + 1);
        build(splitIndex, indexEnd, rightNode, depth + 1);
    }
}

void BVH::sortTrinagles(int indexStart, int indexEnd, int dimension)
{
    if (dimension == 0)
        std::sort(m_triangles->begin() + indexStart, m_triangles->begin() + indexEnd,
        TriangleCenterXLess{});
    else if (dimension == 1)
        std::sort(m_triangles->begin() + indexStart, m_triangles->begin() + indexEnd,
        TriangleCenterYLess{});
    else if (dimension == 2)
        std::sort(m_triangles->begin() + indexStart, m_triangles->begin() + indexEnd,
        TriangleCenterZLess{});
}
