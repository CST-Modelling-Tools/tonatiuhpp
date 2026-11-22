#include "InstanceNode.h"

#include <iostream>
#include <memory>
#include <cmath>
#include <stdexcept>

#include <QDataStream>
#include <QtAlgorithms>
#include <QtGlobal>

#include <Inventor/nodes/SoNode.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>

#include "kernel/node/TonatiuhFunctions.h"
#include "kernel/material/MaterialTransparent.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/profiles/ProfileRT.h"
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
    qDeleteAll(m_children);
}

bool InstanceNode::canAdopt(const InstanceNode* child) const
{
    if (!child || child->m_parent)
        return false;
    // Prevent a detached tree root from becoming a child of its descendant.
    for (const InstanceNode* ancestor = this; ancestor; ancestor = ancestor->m_parent)
        if (ancestor == child)
            return false;
    return true;
}

void InstanceNode::addChild(InstanceNode* child)
{
    if (!canAdopt(child))
        throw std::invalid_argument("InstanceNode child must be unparented and acyclic.");
    m_children << child;
    child->m_parent = this;
}

void InstanceNode::insertChild(int row, InstanceNode* child)
{
    if (!canAdopt(child))
        throw std::invalid_argument("InstanceNode child must be unparented and acyclic.");
    if (row < 0)
        row = 0;
    if (row > m_children.size())
        row = m_children.size();
    m_children.insert(row, child);
    child->m_parent = this;
}

void InstanceNode::replaceChild(int row, InstanceNode* child)
{
    if (row < 0 || row >= m_children.size())
        throw std::out_of_range("InstanceNode child replacement index is invalid.");
    if (m_children[row] == child)
        return;
    if (!canAdopt(child))
        throw std::invalid_argument("InstanceNode child must be unparented and acyclic.");
    std::unique_ptr<InstanceNode> replaced(m_children[row]);
    m_children[row] = child;
    child->m_parent = this;
    if (replaced)
        replaced->m_parent = nullptr;
}

InstanceNode* InstanceNode::takeChildAt(int row)
{
    if (row < 0 || row >= m_children.size())
        return nullptr;
    InstanceNode* child = m_children.takeAt(row);
    if (child)
        child->m_parent = nullptr;
    return child;
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
              << " has " << m_children.size()
              << " children " << std::endl;

    for (InstanceNode* child : m_children)
        child->Print(level + 1); // was level++
}

// The nearest hit is resolved geometrically BEFORE calling material physics.
// Previously, occluded farther surfaces could consume RNG draws and even
// report invalid physical samples before the actual closest hit was known.
bool InstanceNode::findNearestHit(const Ray& rayIn, Intersection& best) const
{
    Ray boundedRay = rayIn;
    boundedRay.tMax = best.distance;
    if (!m_box.intersect(boundedRay))
        return false;

    const InstanceNode* instance1 = this;
    while (instance1->m_children.size() == 1)
        instance1 = instance1->m_children[0];
    if (instance1 != this)
        return instance1->findNearestHit(rayIn, best);

    if (m_node->getTypeId() == TShapeKit::getClassTypeId())
    {
        auto* kit = static_cast<TShapeKit*>(m_node);
        auto* material = static_cast<MaterialRT*>(kit->materialRT.getValue());
        if (!material || material->getTypeId() == MaterialTransparent::getClassTypeId())
            return false;
        auto* shape = static_cast<ShapeRT*>(kit->shapeRT.getValue());
        if (!shape)
            return false;
        auto* profile = static_cast<ProfileRT*>(kit->profileRT.getValue());

        Ray rayLocal = m_transform.transformInverse(boundedRay);
        double distance = 0.;
        DifferentialGeometry geometry;
        if (!shape->intersect(rayLocal, &distance, &geometry, profile))
            return false;
        if (!std::isfinite(distance) || distance < rayIn.tMin)
            throw std::domain_error("Scientific integrity failure: invalid surface-intersection distance.");
        if (distance >= best.distance)
            return false;

        geometry.point = m_transform.transformPoint(geometry.point);
        geometry.dpdu = m_transform.transformVector(geometry.dpdu);
        geometry.dpdv = m_transform.transformVector(geometry.dpdv);
        geometry.normal = m_transform.transformNormal(geometry.normal);

        best.instance = const_cast<InstanceNode*>(this);
        best.material = material;
        best.geometry = geometry;
        best.distance = distance;
        return true;
    }

    if (m_node->getTypeId() == TSeparatorKit::getClassTypeId())
    {
        bool found = false;
        for (InstanceNode* child : m_children)
            if (child->findNearestHit(rayIn, best))
                found = true;
        return found;
    }
    return false;
}

MaterialInteraction InstanceNode::sampleMaterial(
    const Ray& rayIn, const Intersection& nearest, Random& random, Ray& outgoing)
{
    // Nonvirtual contract enforces one validated shared surface-roulette draw.
    return nearest.material->interact(rayIn, nearest.geometry, random, outgoing);
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
        for (InstanceNode* child : m_children)
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
        for (InstanceNode* child : m_children)
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
