#include "SceneBvh.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include <Inventor/nodes/SoNode.h>

#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "libraries/math/3D/Ray.h"

namespace
{
constexpr std::size_t kLeafCapacity = 4;

// A planar surface may have a zero-width coordinate interval; don't use
// Box3D::isValid(), whose contract differs for degenerate intervals.
bool isFiniteBox(const Box3D& box)
{
    for (int axis = 0; axis < 3; ++axis) {
        const double a = box.min()[axis];
        const double b = box.max()[axis];
        if (!std::isfinite(a) || !std::isfinite(b) || a > b)
            return false;
    }
    return true;
}

bool boxContains(const Box3D& outer, const Box3D& inner)
{
    if (!isFiniteBox(outer) || !isFiniteBox(inner))
        return false;
    for (int axis = 0; axis < 3; ++axis) {
        if (outer.min()[axis] > inner.min()[axis] ||
            outer.max()[axis] < inner.max()[axis])
            return false;
    }
    return true;
}

double candidateLimit(double distance)
{
    // Give equal-distance occurrences a chance to participate in the
    // original-order tie break, without introducing a geometric tolerance.
    return std::nextafter(distance, std::numeric_limits<double>::infinity());
}
}

bool SceneBvh::CentroidLess::operator()(const Leaf& a, const Leaf& b) const
{
    const double left = a.center[axis];
    const double right = b.center[axis];
    return left < right || (left == right && a.originalOrder < b.originalOrder);
}

SceneBvh::SceneBvh(const InstanceNode* layoutRoot):
    m_layoutRoot(layoutRoot)
{
    if (!layoutRoot)
        throw std::invalid_argument("Scene BVH requires a non-null layout root.");
    // Never silently fall back to hierarchical traversal. Reject uncertain
    // bounds at preparation time rather than risk changing ray propagation.
    if (!collect(layoutRoot))
        throw std::domain_error("Scene BVH requires finite, conservative world-space bounds.");
    if (m_leaves.empty())
        return;
    // Children and leaves are immutable during one prepared trace. Builders
    // are constructed after updateTree() and before workers are launched.
    m_nodes.reserve(2 * m_leaves.size());
    buildNode(0, m_leaves.size());
}

bool SceneBvh::collect(const InstanceNode* instance)
{
    if (!instance || !instance->getNode())
        return false;

    // Match the authored tree's unary-chain node semantics while
    // gathering canonical, distinct shape occurrences in authored order.
    if (instance->childCount() == 1) {
        const std::size_t previousCount = m_leaves.size();
        if (!collect(instance->childAt(0)))
            return false;
        // All ancestor bounds must conservatively contain the shapes
        // represented by this BVH, or preparation must fail.
        for (std::size_t i = previousCount; i < m_leaves.size(); ++i) {
            if (!boxContains(instance->getBox(), m_leaves[i].instance->getBox()))
                return false;
        }
        return true;
    }

    const SoType nodeType = instance->getNode()->getTypeId();
    if (nodeType == TShapeKit::getClassTypeId()) {
        if (!isFiniteBox(instance->getBox()))
            return false; // Reject uncertain bounds during preparation.
        m_leaves.push_back(Leaf{instance, instance->getBox().center(),
                                m_leaves.size() + 1});
    } else if (nodeType == TSeparatorKit::getClassTypeId()) {
        const std::size_t previousCount = m_leaves.size();
        for (const InstanceNode* child : instance->children()) {
            if (!collect(child))
                return false;
        }
        for (std::size_t i = previousCount; i < m_leaves.size(); ++i) {
            if (!boxContains(instance->getBox(), m_leaves[i].instance->getBox()))
                return false;
        }
    }
    // Non-separator, non-shape nodes are not traceable geometry.
    return true;
}

int SceneBvh::buildNode(std::size_t begin, std::size_t end)
{
    Node node;
    node.begin = begin;
    node.end = end;
    for (std::size_t i = begin; i < end; ++i)
        node.box.expand(m_leaves[i].instance->getBox());

    const int index = static_cast<int>(m_nodes.size());
    m_nodes.push_back(node);
    if (end - begin <= kLeafCapacity)
        return index;

    Box3D centers;
    for (std::size_t i = begin; i < end; ++i)
        centers.expand(m_leaves[i].center);
    const vec3d extent = centers.size();
    int axis = 0;
    if (extent.y > extent.x)
        axis = 1;
    if (extent.z > extent[axis])
        axis = 2;

    const std::size_t middle = begin + (end - begin) / 2;
    std::nth_element(m_leaves.begin() + begin,
                     m_leaves.begin() + middle,
                     m_leaves.begin() + end, CentroidLess{axis});
    const int left = buildNode(begin, middle);
    const int right = buildNode(middle, end);
    m_nodes[index].left = left;
    m_nodes[index].right = right;
    return index;
}

bool SceneBvh::findNearestHit(const Ray& ray, InstanceNode::Intersection& best) const
{
    if (!isActive())
        return false;

    // Retain the authored layout's entry culling and its initial hit bound.
    Ray boundedRay = ray;
    boundedRay.tMax = best.distance;
    if (!m_layoutRoot->getBox().intersect(boundedRay))
        return false;

    // Zero means an existing caller-supplied hit takes priority at a tie.
    // Indexed leaf orders are one-based and follow authored traversal order.
    std::size_t bestOrder = 0;
    return traverse(0, ray, best, bestOrder);
}

bool SceneBvh::traverse(int nodeIndex, const Ray& ray,
                        InstanceNode::Intersection& best,
                        std::size_t& bestOrder) const
{
    const Node& node = m_nodes[nodeIndex];
    Ray boundedRay = ray;
    boundedRay.tMax = candidateLimit(best.distance);
    if (!node.box.intersect(boundedRay))
        return false;

    if (node.isLeaf()) {
        bool found = false;
        for (std::size_t i = node.begin; i < node.end; ++i) {
            const Leaf& leaf = m_leaves[i];
            InstanceNode::Intersection candidate;
            candidate.distance = candidateLimit(best.distance);
            if (!leaf.instance->intersectShape(ray, candidate))
                continue;
            if (candidate.distance < best.distance ||
                (candidate.distance == best.distance && bestOrder != 0 &&
                 leaf.originalOrder < bestOrder)) {
                best = candidate;
                bestOrder = leaf.originalOrder;
                found = true;
            }
        }
        return found;
    }

    double leftNear = 0.;
    double rightNear = 0.;
    const bool intersectsLeft = m_nodes[node.left].box.intersect(boundedRay, &leftNear);
    const bool intersectsRight = m_nodes[node.right].box.intersect(boundedRay, &rightNear);
    if (!intersectsLeft && !intersectsRight)
        return false;

    int first = node.left;
    int second = node.right;
    if (!intersectsLeft || (intersectsRight && rightNear < leftNear))
        std::swap(first, second);

    bool found = false;
    if ((first == node.left ? intersectsLeft : intersectsRight))
        found = traverse(first, ray, best, bestOrder);
    if ((second == node.left ? intersectsLeft : intersectsRight))
        found = traverse(second, ray, best, bestOrder) || found;
    return found;
}
