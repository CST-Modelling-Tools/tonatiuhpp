#include "ScriptSceneServices.h"

#include <QJSEngine>

#include <algorithm>
#include <cmath>
#include <limits>

#include "script/ScriptInterceptionService.h"

#include "script/NodeObject.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"

namespace
{
constexpr auto kSceneServicesProperty = "__tonatiuhppSceneServices";
constexpr auto kSceneApiProperty = "__tonatiuhppSceneApi";
}

ScriptSceneServices* ScriptSceneServices::forObject(QObject* object)
{
    QJSEngine* engine = object ? qjsEngine(object) : nullptr;
    if (!engine)
        return nullptr;
    return qobject_cast<ScriptSceneServices*>(
        engine->globalObject().property(kSceneServicesProperty).toQObject());
}

QJSValue ScriptSceneServices::fail(const QString& message) const
{
    if (auto* engine = qjsEngine(this))
        engine->throwError(message);
    return QJSValue();
}

QJSValue ScriptSceneServices::getScene()
{
    auto* engine = qjsEngine(this);
    TSceneKit* activeScene = scene();
    if (!engine || !activeScene)
        return fail("tn.getScene requires an active scene document.");
    auto* wrapper = new NodeObject(activeScene);
    QJSEngine::setObjectOwnership(wrapper, QJSEngine::JavaScriptOwnership);
    return engine->newQObject(wrapper);
}

QJSValue ScriptSceneServices::Clear()
{
    QString error;
    if (!m_clear || !m_clear(&error))
        return fail(error.isEmpty() ? "tn.Clear: scene document is unavailable." : error);
    return QJSValue();
}

QJSValue ScriptSceneServices::InsertScene(const QJSValue& value)
{
    auto* wrapper = qobject_cast<NodeObject*>(value.toQObject());
    if (!wrapper || !wrapper->getNode() || qjsEngine(wrapper) != qjsEngine(this))
        return fail("tn.InsertScene requires a NodeObject from this script host.");
    TSceneKit* active = scene();
    if (active && (wrapper->getNode() == active || wrapper->getNode() == active->getLayout()))
        return fail("tn.InsertScene cannot insert the active scene or its layout into itself.");
    QString error;
    if (!m_insert || !m_insert(wrapper->getNode(), &error))
        return fail(error.isEmpty() ? "tn.InsertScene: scene insertion failed." : error);
    return QJSValue();
}

QJSValue ScriptSceneServices::FindInterception(const QJSValue& surface,
                                               const QJSValue& rays)
{
    if (!surface.isString() || surface.toString().trimmed().isEmpty())
        return fail("tn.FindInterception requires a non-empty surface URL.");
    if (!rays.isNumber())
        return fail("tn.FindInterception requires a positive integer ray count.");

    const double count = rays.toNumber();
    constexpr double kMaxSafeInteger = 9007199254740991.;
    const double maxCount = std::min(kMaxSafeInteger,
        static_cast<double>(std::numeric_limits<ulong>::max()));
    if (!std::isfinite(count) || std::floor(count) != count
        || count < 1. || count > maxCount)
        return fail("tn.FindInterception requires a positive integer ray count in range.");

    // The GUI/legacy bridge intentionally keeps its existing FluxAnalysis
    // implementation for this intermediate package. Only true-headless tn
    // invokes this new, widget-free method until native parity is measured.
    QString error;
    double power = 0.;
    if (!ScriptInterceptionService::findPower(scene(), surface.toString(),
                                              static_cast<ulong>(count), 0,
                                              &power, &error))
        return fail(error.isEmpty() ? "tn.FindInterception failed." : error);
    return QJSValue(power);
}

void registerScriptSceneServices(QJSEngine* engine, ScriptSceneServices* services)
{
    if (!engine || !services)
        return;
    QJSEngine::setObjectOwnership(services, QJSEngine::CppOwnership);
    const QJSValue facade = engine->newQObject(services);
    engine->globalObject().setProperty(kSceneServicesProperty, facade);
    engine->globalObject().setProperty(kSceneApiProperty, facade);
    engine->globalObject().setProperty("NodeObject", engine->newQMetaObject(&NodeObject::staticMetaObject));
}
