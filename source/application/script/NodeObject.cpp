#include "NodeObject.h"

#include <QJSEngine>

#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/profiles/ProfileRT.h"
#include "kernel/material/MaterialRT.h"
#include "kernel/trackers/TrackerArmature.h"
#include "kernel/trackers/TrackerKit.h"
#include "kernel/sun/SunKit.h"
#include "kernel/air/AirKit.h"
#include "kernel/sun/SunShape.h"
#include "kernel/air/AirTransmission.h"

#include "script/ScriptSceneServices.h"

NodeObject::NodeObject(QObject* parent):
    QObject(parent),
    m_node(new TSeparatorKit)
{
    m_node->ref();
}

NodeObject::NodeObject(SoNode* node):
    QObject(nullptr),
    m_node(node)
{
    if (m_node)
        m_node->ref();
}

NodeObject::~NodeObject()
{
    if (m_node)
        m_node->unref();
}

QJSValue NodeObject::wrapNode(SoNode* node) const
{
    QJSEngine* engine = qjsEngine(this);
    if (!engine || !node)
        return QJSValue();
    auto* wrapper = new NodeObject(node);
    QJSEngine::setObjectOwnership(wrapper, QJSEngine::JavaScriptOwnership);
    return engine->newQObject(wrapper);
}

QJSValue NodeObject::reportError(const QString& message) const
{
    if (QJSEngine* engine = qjsEngine(this))
        engine->throwError(message);
    return QJSValue();
}

QJSValue NodeObject::getScene()
{
    const ScriptSceneServices* services = ScriptSceneServices::forObject(this);
    return services ? wrapNode(services->scene())
                    : reportError("NodeObject.getScene requires a script scene context.");
}

QJSValue NodeObject::getRoot()
{
    const ScriptSceneServices* services = ScriptSceneServices::forObject(this);
    TSceneKit* scene = services ? services->scene() : nullptr;
    return scene ? wrapNode(scene->getLayout())
                 : reportError("NodeObject.getRoot requires a loaded scene.");
}

QJSValue NodeObject::createNode(const QString& name)
{
    if (!m_node || m_node->getTypeId() != TSeparatorKit::getClassTypeId())
        return QJSValue();
    auto* kit = new TSeparatorKit;
    auto* parent = static_cast<TSeparatorKit*>(m_node);
    auto* group = static_cast<SoGroup*>(parent->getPart("group", true));
    group->addChild(kit);
    NodeObject* answer = new NodeObject(kit);
    if (!name.isEmpty()) answer->setName(name);
    if (QJSEngine* engine = qjsEngine(this)) {
        QJSEngine::setObjectOwnership(answer, QJSEngine::JavaScriptOwnership);
        return engine->newQObject(answer);
    }
    delete answer;
    return QJSValue();
}

QJSValue NodeObject::createShape()
{
    if (!m_node || m_node->getTypeId() != TSeparatorKit::getClassTypeId())
        return QJSValue();
    auto* kit = new TShapeKit;
    auto* parent = static_cast<TSeparatorKit*>(m_node);
    auto* group = static_cast<SoGroup*>(parent->getPart("group", true));
    group->addChild(kit);
    return wrapNode(kit);
}

QJSValue NodeObject::createTracker()
{
    if (!m_node || m_node->getTypeId() != TSeparatorKit::getClassTypeId())
        return QJSValue();
    auto* kit = new TrackerKit;
    auto* parent = static_cast<TSeparatorKit*>(m_node);
    auto* group = static_cast<SoGroup*>(parent->getPart("group", true));
    kit->m_parent = parent;
    group->addChild(kit);
    return wrapNode(kit);
}

QJSValue NodeObject::getPart(const QString& name)
{
    if (!m_node || !m_node->getTypeId().isDerivedFrom(SoBaseKit::getClassTypeId()))
        return QJSValue(0);

    auto* kit = static_cast<SoBaseKit*>(m_node);
    SoNode* node = kit->getPart(name.toLatin1().data(), false);
    if (!node) {
        SoField* field = m_node->getField(name.toLatin1().data());
        auto* fieldNode = dynamic_cast<SoSFNode*>(field);
        if (fieldNode) node = fieldNode->getValue();
    }
    return node ? wrapNode(node) : QJSValue(0);
}

void NodeObject::setPart(const QString& name, QJSValue node)
{
    if (!m_node || m_node->getTypeId() != TrackerKit::getClassTypeId() || name != "armature")
        return;
    auto* wrapper = qobject_cast<NodeObject*>(node.toQObject());
    if (!wrapper || !wrapper->m_node ||
        !wrapper->m_node->getTypeId().isDerivedFrom(TrackerArmature::getClassTypeId()) ||
        qjsEngine(wrapper) != qjsEngine(this)) {
        reportError("NodeObject.setPart requires a tracker armature NodeObject from this script host.");
        return;
    }
    static_cast<TrackerKit*>(m_node)->armature = wrapper->m_node;
}

QJSValue NodeObject::insertSurface(const QString& name)
{
    if (!m_node || m_node->getTypeId() != TShapeKit::getClassTypeId())
        return QJSValue(0);
    const auto* services = ScriptSceneServices::forObject(this);
    auto* shape = services ? dynamic_cast<ShapeRT*>(services->create(
        ScriptSceneServices::FactoryKind::Shape, name)) : nullptr;
    if (!shape)
        return reportError(QString("NodeObject.insertSurface: unknown shape '%1'.").arg(name));
    auto* parent = static_cast<TShapeKit*>(m_node);
    parent->shapeRT = shape;
    parent->profileRT = shape->getDefaultProfile();
    return wrapNode(shape);
}

QJSValue NodeObject::insertProfile(const QString& name)
{
    if (!m_node || m_node->getTypeId() != TShapeKit::getClassTypeId())
        return QJSValue(0);
    const auto* services = ScriptSceneServices::forObject(this);
    auto* profile = services ? dynamic_cast<ProfileRT*>(services->create(
        ScriptSceneServices::FactoryKind::Profile, name)) : nullptr;
    if (!profile)
        return reportError(QString("NodeObject.insertProfile: unknown profile '%1'.").arg(name));
    static_cast<TShapeKit*>(m_node)->profileRT = profile;
    return wrapNode(profile);
}

QJSValue NodeObject::insertMaterial(const QString& name)
{
    if (!m_node || m_node->getTypeId() != TShapeKit::getClassTypeId())
        return QJSValue(0);
    const auto* services = ScriptSceneServices::forObject(this);
    auto* material = services ? dynamic_cast<MaterialRT*>(services->create(
        ScriptSceneServices::FactoryKind::Material, name)) : nullptr;
    if (!material)
        return reportError(QString("NodeObject.insertMaterial: unknown material '%1'.").arg(name));
    static_cast<TShapeKit*>(m_node)->materialRT = material;
    return wrapNode(material);
}

QJSValue NodeObject::insertArmature(const QString& name)
{
    if (!m_node || m_node->getTypeId() != TrackerKit::getClassTypeId())
        return QJSValue(0);
    const auto* services = ScriptSceneServices::forObject(this);
    auto* armature = services ? dynamic_cast<TrackerArmature*>(services->create(
        ScriptSceneServices::FactoryKind::Tracker, name)) : nullptr;
    if (!armature)
        return reportError(QString("NodeObject.insertArmature: unknown tracker armature '%1'.").arg(name));
    static_cast<TrackerKit*>(m_node)->armature = armature;
    return wrapNode(armature);
}

void NodeObject::setName(const QString& name)
{
    if (m_node)
        m_node->setName(name.toLatin1().data());
}

void NodeObject::setParameter(const QString& name, const QString& value)
{
    if (!m_node)
        return;
    SoNode* node = m_node;
    if (m_node->getTypeId() == TSeparatorKit::getClassTypeId()) {
        node = static_cast<TSeparatorKit*>(m_node)->getPart("transform", true);
    } else if (m_node->getTypeId() == SunKit::getClassTypeId() && name == "shape") {
        const auto* services = ScriptSceneServices::forObject(this);
        auto* shape = services ? dynamic_cast<SunShape*>(services->create(
            ScriptSceneServices::FactoryKind::Sun, value)) : nullptr;
        if (!shape) {
            reportError(QString("NodeObject.setParameter: unknown sun shape '%1'.").arg(value));
            return;
        }
        if (auto* field = dynamic_cast<SoSFNode*>(m_node->getField(name.toLatin1().data())))
            field->setValue(shape);
        return;
    } else if (m_node->getTypeId() == AirKit::getClassTypeId() && name == "transmission") {
        const auto* services = ScriptSceneServices::forObject(this);
        auto* transmission = services ? dynamic_cast<AirTransmission*>(services->create(
            ScriptSceneServices::FactoryKind::Air, value)) : nullptr;
        if (!transmission) {
            reportError(QString("NodeObject.setParameter: unknown air transmission '%1'.").arg(value));
            return;
        }
        if (auto* field = dynamic_cast<SoSFNode*>(m_node->getField(name.toLatin1().data())))
            field->setValue(transmission);
        return;
    }

    SoField* field = node ? node->getField(name.toLatin1().data()) : nullptr;
    if (!field)
        return;
    if (field->getTypeId() == SoSFString::getClassTypeId())
        static_cast<SoSFString*>(field)->setValue(value.toLatin1().data());
    else
        field->set(value.toLatin1().data());
}

QJSValue NodeObject::FindInterception(QJSValue surface, QJSValue rays)
{
    const auto* services = ScriptSceneServices::forObject(this);
    const auto result = services ? services->findInterception(surface.toString(), rays.toUInt())
                                 : std::nullopt;
    return result ? QJSValue(*result) : reportError(
        "NodeObject.FindInterception is unavailable in this script context.");
}
