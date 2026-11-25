#pragma once

#include <cstddef>
#include <vector>

#include "kernel/TonatiuhKernel.h"
#include "kernel/run/InstanceNode.h"
#include "libraries/math/3D/Box3D.h"

class Ray;

// Immutable, per-prepared-trace spatial index. Leaves borrow the canonical
// InstanceNode occurrences; neither their identity nor their ownership changes.
class TONATIUH_KERNEL SceneBvh
{
public:
    explicit SceneBvh(const InstanceNode* layoutRoot);

    // A BVH is constructed even for a single shape; empty scenes have no hits.
    bool isActive() const { return !m_nodes.empty(); }
    std::size_t shapeCount() const { return m_leaves.size(); }
    bool findNearestHit(const Ray& ray, InstanceNode::Intersection& hit) const;

private:
    struct Leaf
    {
        const InstanceNode* instance = nullptr;
        vec3d center;
        std::size_t originalOrder = 0;
    };

    struct Node
    {
        Box3D box;
        std::size_t begin = 0;
        std::size_t end = 0;
        int left = -1;
        int right = -1;
        bool isLeaf() const { return left < 0; }
    };

    struct CentroidLess
    {
        int axis = 0;
        bool operator()(const Leaf& a, const Leaf& b) const;
    };

    bool collect(const InstanceNode* instance);
    int buildNode(std::size_t begin, std::size_t end);
    bool traverse(int nodeIndex, const Ray& ray,
                  InstanceNode::Intersection& best,
                  std::size_t& bestOrder) const;

    const InstanceNode* m_layoutRoot = nullptr;
    std::vector<Leaf> m_leaves;
    std::vector<Node> m_nodes;
};
