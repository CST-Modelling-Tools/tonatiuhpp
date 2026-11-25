#pragma once

#include "kernel/TonatiuhKernel.h"

#include <QVector>
#include <QString>
#include <QStringList>
#include <QPair>
#include <QDataStream>

#include <Inventor/SbBox3f.h>

#include "libraries/math/3D/Box3D.h"
#include "libraries/math/3D/Transform.h"
#include "kernel/material/MaterialRT.h"
#include "kernel/shape/DifferentialGeometry.h"

#include <limits>

class Random;
class Ray;
class SoNode;
class TShapeKit;

//! InstanceNode class represents an instance of a node in the scene.
/*!
 * In a scene, a node can be shared by more than one parent. Each of these shared
 * instances is represented in a scene as an InstanceNode object.
 * Any change made within a shared node is reflected in all node's InstanceNode.
 */
class TONATIUH_KERNEL InstanceNode
{
public:
    explicit InstanceNode(SoNode* node);
    ~InstanceNode();

    // Child nodes are owned and deleted by this instance. Never copy
    // the owning raw-pointer collection.
    InstanceNode(const InstanceNode&) = delete;
    InstanceNode& operator=(const InstanceNode&) = delete;

    SoNode* getNode() const { return m_node; }
    void setNode(SoNode* node) { m_node = node; }

    // Borrowed parent; only the parent owns this instance.
    InstanceNode* getParent() const { return m_parent; }

    const Box3D& getBox() const { return m_box; }
    void setBox(const Box3D& box) { m_box = box; }

    const Transform& getTransform() const { return m_transform; }
    void setTransform(const Transform& t) { m_transform = t; }

    // These accessors never transfer ownership or permit list mutation.
    int childCount() const { return m_children.size(); }
    InstanceNode* childAt(int row) const {
        return row >= 0 && row < m_children.size() ? m_children.at(row) : nullptr;
    }
    int indexOfChild(InstanceNode* child) const { return m_children.indexOf(child); }
    const QVector<InstanceNode*>& children() const { return m_children; }

    // Adopt an unparented instance. The parent deletes its children.
    void addChild(InstanceNode* child);
    void insertChild(int row, InstanceNode* child);
    void replaceChild(int row, InstanceNode* child);
    // Relinquish ownership without deleting; caller must adopt or delete.
    InstanceNode* takeChildAt(int row);

    bool operator==(const InstanceNode& other);
    QString getURL() const;
    void Print(int level) const;

    struct Intersection {
        InstanceNode* instance = nullptr;
        MaterialRT* material = nullptr;
        DifferentialGeometry geometry;
        double distance = std::numeric_limits<double>::infinity();
    };
    // Shape-leaf primitive only; the scene BVH is the sole nearest-hit traversal.
    bool intersectShape(const Ray& rayIn, Intersection& hit) const;
    static MaterialInteraction sampleMaterial(const Ray& rayIn, const Intersection& hit,
                                              Random& random, Ray& outgoing);
    // Compatibility convenience for callers inside this codebase.
    MaterialInteraction intersect(const Ray& rayIn, Random& rand, bool& isFront,
                                 InstanceNode*& instance, Ray& rayOut);


    void extendBoxForLight(SbBox3f* extendedBox);

    void updateTree(const Transform& tParent);
    void collectShapeTransforms(QStringList disabledNodes, QVector<QPair<TShapeKit*, Transform>>& shapes);

private:
    bool canAdopt(const InstanceNode* child) const;
    QVector<InstanceNode*> m_children; // owning raw pointers; retain the existing container type
    SoNode* m_node = nullptr;
    InstanceNode* m_parent = nullptr;
    Box3D m_box;            // in world frame
    Transform m_transform;  // from object to world
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TONATIUH_KERNEL QDataStream& operator<<(QDataStream& s, const InstanceNode& node);

// NOTE: must be NON-const (reading modifies the object / satisfies Qt's streaming expectations)
// This also fixes your link error: the MOC-generated code wants a real symbol to link against.
TONATIUH_KERNEL QDataStream& operator>>(QDataStream& s, InstanceNode& node);
#endif /* DOXYGEN_SHOULD_SKIP_THIS */