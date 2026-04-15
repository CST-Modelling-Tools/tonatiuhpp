#include "InstanceNode.h"

#include <iostream>

#include <QDataStream>
#include <QtAlgorithms>
#include <QtGlobal>

#include <Inventor/nodes/SoNode.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>

#include "kernel/node/TonatiuhFunctions.h"
#include "kernel/material/MaterialTransparent.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/shape/ShapeRT.h"
#include "libraries/math/3D/Box3D.h"
#include "libraries/math/3D/Ray.h"
#include "libraries/math/3D/Transform.h"
#include "scene/TShapeKit.h"
#include "shape/DifferentialGeometry.h"
#include "sun/SunKit.h"
#include "trackers/TrackerArmature.h"

InstanceNode::InstanceNode(SoNode* node)
    : m_node(node)
    , m_parent(nullptr)
{
}

InstanceNode::~InstanceNode()
{
    qDeleteAll(children);
}

/**
 * Appends new \a child node to the end of the child list.
 **/
void InstanceNode::addChild(InstanceNode* child)
{
    children << child;
    child->m_parent = this;
}

/**
 * Inserts the \a instanceChild node as child number \a row.
 **/
void InstanceNode::insertChild(int row, InstanceNode* child)
{
    if (row > children.size())
        row = children.size();
    children.insert(row, child);
    child->m_parent = this;
}

void InstanceNode::replaceChild(int row, InstanceNode* child)
{
    delete children[row];
    children[row] = child;
    child->m_parent = this;
}

bool InstanceNode::operator==(const InstanceNode& other)
{
    if (m_node != other.m_node) return false;

    // Root-safe comparison
    if (m_parent == nullptr && other.m_parent == nullptr) return true;
    if (m_parent == nullptr || other.m_parent == nullptr) return false;

    return m_parent->m_node == other.m_parent->m_node;
}

QString InstanceNode::getURL() const
{
    QString url;
    if (m_parent) url = m_parent->getURL();
    return url + "/" + m_node->getName().getString();
}

void InstanceNode::Print(int level) const
{
    for (int i = 0; i < level; ++i)
        std::cout << " ";

    std::cout << m_node->getTypeId().getName().getString()
              << " has " << children.size()
              << " children " << std::endl;

    for (InstanceNode* child : children)
        child->Print(level + 1); // was level++
}

bool InstanceNode::intersect(const Ray& rayIn, Random& rand, bool& isFront, InstanceNode*& instance, Ray& rayOut)
{
    if (!m_box.intersect(rayIn)) return false;

    InstanceNode* instance1 = this;
    while (instance1->children.size() == 1)
        instance1 = instance1->children[0];
    if (instance1 != this)
        return instance1->intersect(rayIn, rand, isFront, instance, rayOut);

    // if (TShapeKit* kit = dynamic_cast<TShapeKit*>(m_node)) // slower
    if (m_node->getTypeId() == TShapeKit::getClassTypeId()) // faster
    {
        TShapeKit* kit = (TShapeKit*) m_node;

        MaterialRT* material = (MaterialRT*) kit->materialRT.getValue();
        if (!material) return false;
        if (material->getTypeId() == MaterialTransparent::getClassTypeId()) return false;

        ShapeRT* shape = (ShapeRT*) kit->shapeRT.getValue();
        if (!shape) return false;
        ProfileRT* profile = (ProfileRT*) kit->profileRT.getValue();

        Ray rayLocal = m_transform.transformInverse(rayIn);
        double tHit = 0.;
        DifferentialGeometry dg;
        if (!shape->intersect(rayLocal, &tHit, &dg, profile)) return false;

        rayIn.tMax = tHit;
        isFront = dg.isFront;
        instance = this;

        dg.point = m_transform.transformPoint(dg.point);
        dg.dpdu = m_transform.transformVector(dg.dpdu);
        dg.dpdv = m_transform.transformVector(dg.dpdv);
        dg.normal = m_transform.transformNormal(dg.normal);

        return material->OutputRay(rayIn, dg, rand, rayOut);
    }
    else if (m_node->getTypeId() == TSeparatorKit::getClassTypeId())
    {
        bool hasRayOut = false;
        double t = rayIn.tMax;

        for (InstanceNode* instanceChild : children)
        {
            Ray rayOutChild;
            bool isFrontChild = true;
            bool hasRayOutChild = instanceChild->intersect(rayIn, rand, isFrontChild, instanceChild, rayOutChild);

            if (rayIn.tMax < t) // tMax mutable
            {
                t = rayIn.tMax;
                isFront = isFrontChild;
                instance = instanceChild;
                hasRayOut = hasRayOutChild;
                rayOut = rayOutChild;
            }
        }
        return hasRayOut;
    }

    return false;
}

void InstanceNode::extendBoxForLight(SbBox3f* extendedBox)
{
    SoGetBoundingBoxAction* action = new SoGetBoundingBoxAction(SbViewportRegion());
    getNode()->getBoundingBox(action);
    SbBox3f box = action->getXfBoundingBox().project();
    delete action;
    extendedBox->extendBy(box);
}

/**
 * Set node world to object transform to \a nodeTransform .
 */
void InstanceNode::updateTree(const Transform& tParent)
{
    if (TSeparatorKit* separatorKit = dynamic_cast<TSeparatorKit*>(m_node))
    {
        TTransform* t = SO_GET_PART(separatorKit, "transform", TTransform);
        m_transform = tParent * tgf::makeTransform(t);

        Box3D box;
        for (InstanceNode* child : children)
        {
            child->updateTree(m_transform);
            box.expand(child->m_box);
        }
        m_box = box;
    }
    else if (TShapeKit* shapeKit = dynamic_cast<TShapeKit*>(m_node))
    {
        TShapeKit* kit = (TShapeKit*) m_node;

        if (TTransform* t = (TTransform*) shapeKit->getPart("transform", false))
            m_transform = tParent * tgf::makeTransform(t);
        else
            m_transform = tParent;

        ShapeRT* shape = (ShapeRT*) kit->shapeRT.getValue();
        ProfileRT* profile = (ProfileRT*) kit->profileRT.getValue();
        m_box = m_transform(shape->getBox(profile));
    }
}

void InstanceNode::collectShapeTransforms(QStringList disabledNodes, QVector<QPair<TShapeKit*, Transform>>& shapes)
{
    if (disabledNodes.contains(getURL())) return;

    if (dynamic_cast<TSeparatorKit*>(m_node))
    {
        for (InstanceNode* child : children)
            child->collectShapeTransforms(disabledNodes, shapes);
    }
    else if (TShapeKit* shape = dynamic_cast<TShapeKit*>(m_node))
    {
        shapes << QPair<TShapeKit*, Transform>(shape, m_transform);
    }
}

// -----------------------------------------------------------------------------
// QDataStream operators
// NOTE: operator>> must take a NON-CONST reference.
// Also ensure your InstanceNode.h declares these with the same signatures.
// -----------------------------------------------------------------------------

QDataStream& operator<<(QDataStream& s, const InstanceNode& node)
{
    // Minimal serialization placeholder.
    // Replace with whatever you actually want to serialize.
    s << node.getURL();
    return s;
}

QDataStream& operator>>(QDataStream& s, InstanceNode& node)
{
    // Minimal deserialization placeholder.
    // This exists mainly so the symbol is available to the linker.
    QString dummy;
    s >> dummy;
    Q_UNUSED(node);
    return s;
}
