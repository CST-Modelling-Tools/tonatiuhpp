#include "ScriptSceneServices.h"

#include <QJSEngine>
#include <QFileInfo>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "script/ScriptInterceptionService.h"
#include "script/ScriptHost.h"

#include "script/NodeObject.h"
#include "core/SceneInstanceBuilder.h"
#include "kernel/run/InstanceNode.h"

#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/nodes/SoNode.h>
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/sun/SunPosition.h"
#include "libraries/sun/sunpos.h"

namespace
{
constexpr auto kSceneServicesProperty = "__tonatiuhppSceneServices";
constexpr auto kSceneApiProperty = "__tonatiuhppSceneApi";

bool scriptFiniteNumber(const QJSValue& value, double* result)
{
    if (!result || (!value.isNumber() && !value.isString()))
        return false;
    if (value.isString() && value.toString().trimmed().isEmpty())
        return false;
    const double number = value.toNumber();
    if (!std::isfinite(number))
        return false;
    *result = number;
    return true;
}
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

QJSValue ScriptSceneServices::saveScene(const QJSValue& path)
{
    if (!path.isString() || path.toString().trimmed().isEmpty())
        return fail("tn.saveScene requires a non-empty scene file path.");
    auto* host = qobject_cast<ScriptHost*>(parent());
    if (!host || !m_save)
        return fail("tn.saveScene: scene document is unavailable.");
    const QString resolved = host->resolvePath(path.toString());
    if (QFileInfo(resolved).suffix().compare(QStringLiteral("tnhpp"), Qt::CaseInsensitive) != 0)
        return fail("tn.saveScene requires a .tnhpp file path.");
    QString error;
    if (!m_save(resolved, &error))
        return fail(QString("tn.saveScene failed: %1").arg(error));
    return QJSValue(true);
}

QJSValue ScriptSceneServices::loadScene(const QJSValue& path)
{
    if (!path.isString() || path.toString().trimmed().isEmpty())
        return fail("tn.loadScene requires a non-empty scene file path.");
    auto* host = qobject_cast<ScriptHost*>(parent());
    if (!host || !m_load)
        return fail("tn.loadScene: scene document is unavailable.");
    const QString resolved = host->resolvePath(path.toString());
    if (QFileInfo(resolved).suffix().compare(QStringLiteral("tnhpp"), Qt::CaseInsensitive) != 0)
        return fail("tn.loadScene requires a .tnhpp file path.");
    QString error;
    if (!m_load(resolved, &error))
        return fail(QString("tn.loadScene failed: %1").arg(error));
    return QJSValue(true);
}

QJSValue ScriptSceneServices::SetValue(const QJSValue& nodeUrl,
                                        const QJSValue& fieldName,
                                        const QJSValue& value)
{
    if (!nodeUrl.isString() || nodeUrl.toString().trimmed().isEmpty()
        || !fieldName.isString() || fieldName.toString().trimmed().isEmpty()
        || value.isNull() || value.isUndefined()
        || (!value.isString() && !value.isNumber() && !value.isBool())
        || value.toString().isEmpty())
        return fail("tn.SetValue requires a scene URL, field name, and non-empty field value.");

    TSceneKit* active = scene();
    if (!active)
        return fail("tn.SetValue requires an active scene.");
    const QString url = nodeUrl.toString();
    const QString fieldNameText = fieldName.toString();
    const QString fieldValue = value.toString();

    // Resolve instance URLs through the same nonvisual tree as scripted
    // interception, not the GUI SceneTreeModel. A shared Coin node can appear
    // in several locations; reject ambiguous URL names instead of guessing.
    SceneInstanceTree tree = SceneInstanceBuilder::build(active);
    int matches = 0;
    SoNode* target = nullptr;
    const std::function<void(const InstanceNode*)> visit = [&](const InstanceNode* instance) {
        if (!instance)
            return;
        if (instance->getURL() == url && instance->getNode()) {
            ++matches;
            target = instance->getNode();
        }
        for (const InstanceNode* child : instance->children)
            visit(child);
    };
    visit(tree.sceneRoot.get());
    if (matches == 0)
        return fail(QString("tn.SetValue: no scene node at '%1'.").arg(url));
    if (matches != 1)
        return fail(QString("tn.SetValue: ambiguous scene URL '%1'.").arg(url));

    // Historical tn.SetValue edits separator TRANSFORM fields, not fields on
    // the separator kit; retain this convention in both script modes.
    if (auto* separator = dynamic_cast<TSeparatorKit*>(target))
        target = separator->getPart("transform", true);
    SoField* field = target ? target->getField(fieldNameText.toLatin1().constData()) : nullptr;
    if (!field)
        return fail(QString("tn.SetValue: unknown field '%1'.").arg(fieldNameText));

    QString error;
    if (m_writeField) {
        if (!m_writeField(target, fieldNameText, fieldValue, &error))
            return fail(error.isEmpty() ? "tn.SetValue: host rejected the field edit." : error);
    } else if (field->getTypeId() == SoSFString::getClassTypeId()) {
        static_cast<SoSFString*>(field)->setValue(fieldValue.toUtf8().constData());
    } else if (!field->set(fieldValue.toUtf8().constData())) {
        return fail(QString("tn.SetValue: invalid value for field '%1'.").arg(fieldNameText));
    }
    return QJSValue(); // Existing GUI void slot returned undefined.
}

// Phase 3B2A export control plane. No file is written and no trace is started.
QJSValue ScriptSceneServices::applyPhotonExportChange(
    const ScriptPhotonExportChange& change, const QString& method)
{
    // The historical GUI ignores most export settings until a map type is
    // selected. Preserve this initial no-op behavior in both script hosts.
    if (!m_exportState.settingsPresent &&
        change.kind != ScriptPhotonExportChange::Kind::MapType &&
        change.kind != ScriptPhotonExportChange::Kind::BufferSize &&
        change.kind != ScriptPhotonExportChange::Kind::BufferAppend)
        return QJSValue();

    QString error;
    if (m_writePhotonExport && !m_writePhotonExport(change, &error))
        return fail(error.isEmpty()
            ? QString("tn.%1: host rejected photon export setting.").arg(method) : error);
    m_exportState.apply(change); // Atomic after GUI adapter acceptance.
    return QJSValue(); // Historical GUI void setters return undefined.
}

QJSValue ScriptSceneServices::SetExportAllPhotonMap()
{
    return applyPhotonExportChange({ScriptPhotonExportChange::Kind::AllSurfaces},
                                   "SetExportAllPhotonMap");
}

QJSValue ScriptSceneServices::SetExportCoordinates(const QJSValue& enabled,
                                                   const QJSValue& global)
{
    if (!enabled.isBool() || !global.isBool())
        return fail("tn.SetExportCoordinates requires two boolean values.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::Coordinates};
    change.flag = enabled.toBool();
    change.aux = global.toBool();
    return applyPhotonExportChange(change, "SetExportCoordinates");
}

QJSValue ScriptSceneServices::SetExportIntersectionSurface(const QJSValue& enabled)
{
    if (!enabled.isBool())
        return fail("tn.SetExportIntersectionSurface requires a boolean value.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::SurfaceID};
    change.flag = enabled.toBool();
    return applyPhotonExportChange(change, "SetExportIntersectionSurface");
}

QJSValue ScriptSceneServices::SetExportIntersectionSurfaceSide(const QJSValue& enabled)
{
    if (!enabled.isBool())
        return fail("tn.SetExportIntersectionSurfaceSide requires a boolean value.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::SurfaceSide};
    change.flag = enabled.toBool();
    return applyPhotonExportChange(change, "SetExportIntersectionSurfaceSide");
}

QJSValue ScriptSceneServices::SetExportPhotonMapType(const QJSValue& name)
{
    if (!name.isString() || name.toString().trimmed().isEmpty())
        return fail("tn.SetExportPhotonMapType requires a non-empty exporter name.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::MapType};
    change.name = name.toString();
    // The GUI adapter checks plugin availability immediately; a headless
    // script stores the request for a future nonvisual exporter backend.
    return applyPhotonExportChange(change, "SetExportPhotonMapType");
}

QJSValue ScriptSceneServices::SetExportPreviousNextPhotonID(const QJSValue& enabled)
{
    if (!enabled.isBool())
        return fail("tn.SetExportPreviousNextPhotonID requires a boolean value.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::PhotonsID};
    change.flag = enabled.toBool();
    return applyPhotonExportChange(change, "SetExportPreviousNextPhotonID");
}

QJSValue ScriptSceneServices::SetExportTypeParameterValue(const QJSValue& name,
                                                          const QJSValue& value)
{
    if (!name.isString() || name.toString().trimmed().isEmpty() || !value.isString())
        return fail("tn.SetExportTypeParameterValue requires parameter name and string value.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::Parameter};
    change.name = name.toString();
    change.value = value.toString(); // Empty parameter values historically valid.
    return applyPhotonExportChange(change, "SetExportTypeParameterValue");
}

QJSValue ScriptSceneServices::AddExportSurfaceURL(const QJSValue& url)
{
    if (!url.isString() || url.toString().trimmed().isEmpty())
        return fail("tn.AddExportSurfaceURL requires a non-empty scene URL.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::AddSurface};
    change.name = url.toString();
    return applyPhotonExportChange(change, "AddExportSurfaceURL");
}

QJSValue ScriptSceneServices::SetPhotonBufferSize(const QJSValue& size)
{
    if (!size.isNumber())
        return fail("tn.SetPhotonBufferSize requires a positive integer size in range.");
    const double number = size.toNumber();
    if (!std::isfinite(number) || std::floor(number) != number || number < 1.
        || number > static_cast<double>(std::numeric_limits<uint>::max()))
        return fail("tn.SetPhotonBufferSize requires a positive integer size in range.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::BufferSize};
    change.count = static_cast<uint>(number);
    return applyPhotonExportChange(change, "SetPhotonBufferSize");
}

QJSValue ScriptSceneServices::SetPhotonBufferAppend(const QJSValue& enabled)
{
    if (!enabled.isBool())
        return fail("tn.SetPhotonBufferAppend requires a boolean value.");
    ScriptPhotonExportChange change{ScriptPhotonExportChange::Kind::BufferAppend};
    change.flag = enabled.toBool();
    return applyPhotonExportChange(change, "SetPhotonBufferAppend");
}

// Phase 3B2B1: share Run's script-facing request and error semantics.
// The GUI still owns its historical exporter/progress lifecycle; the headless
// adapter supports bounded one-shot File / No export, not append/retry yet.
QJSValue ScriptSceneServices::Run()
{
    if (!m_runExecutor)
        return fail("tn.Run: this script host has no run execution backend.");
    ScriptRunRequest request;
    request.scene = scene();
    if (!request.scene)
        return fail("tn.Run: active scene is unavailable.");
    request.explicitRays = m_scriptRunRays;
    request.rays = m_scriptRunRays.value_or(10000u);
    request.sunWidthDivisions = m_scriptGridWidth;
    request.sunHeightDivisions = m_scriptGridHeight;
    request.exportOptions = m_exportState;
    QString error;
    if (!m_runExecutor(request, &error))
        return fail(error.isEmpty() ? "tn.Run: execution was rejected or failed." : error);
    return QJSValue(); // Historical void Run returns undefined.
}

QJSValue ScriptSceneServices::SetRaysNumber(const QJSValue& count)
{
    // Historical GUI SetRaysNumber takes a uint. Reject truncation, signed
    // wraparound and fractional/NaN/Infinity inputs before calling the host.
    if (!count.isNumber())
        return fail("tn.SetRaysNumber requires a positive integer ray count in range.");
    const double number = count.toNumber();
    if (!std::isfinite(number) || std::floor(number) != number
        || number < 1. || number > static_cast<double>(std::numeric_limits<uint>::max()))
        return fail("tn.SetRaysNumber requires a positive integer ray count in range.");

    const uint requested = static_cast<uint>(number);
    QString error;
    if (m_writeRunRays && !m_writeRunRays(requested, &error))
        return fail(error.isEmpty() ? "tn.SetRaysNumber: host rejected the run ray count." : error);
    // Commit only after a successful GUI callback, so failed writes leave the
    // accepted per-script value untouched. No trace/export is started here.
    m_scriptRunRays = requested;
    return QJSValue(); // Historical GUI void slot returned undefined.
}

QJSValue ScriptSceneServices::SetRaysGrid(const QJSValue& width, const QJSValue& height)
{
    double w = 0.;
    double h = 0.;
    if (!scriptFiniteNumber(width, &w) || !scriptFiniteNumber(height, &h)
        || std::floor(w) != w || std::floor(h) != h
        || w < 1. || h < 1.
        || w > static_cast<double>(std::numeric_limits<int>::max())
        || h > static_cast<double>(std::numeric_limits<int>::max()))
        return fail("tn.SetRaysGrid requires positive integer grid dimensions.");

    const int columns = static_cast<int>(w);
    const int rows = static_cast<int>(h);
    QString error;
    if (m_writeGrid && !m_writeGrid(columns, rows, &error))
        return fail(error.isEmpty() ? "tn.SetRaysGrid: host rejected the grid." : error);
    // No partial state update when the GUI adapter rejects the operation.
    m_scriptGridWidth = columns;
    m_scriptGridHeight = rows;
    return QJSValue();
}

QJSValue ScriptSceneServices::FindInterception(const QJSValue& surface,
                                               const QJSValue& rays)
{
    return FindInterception(surface, rays, QJSValue(false));
}

QJSValue ScriptSceneServices::FindInterception(const QJSValue& surface,
                                               const QJSValue& rays,
                                               const QJSValue& show)
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

    const ulong requestedRays = static_cast<ulong>(count);
    SimulationConfig configuration;
    configuration.rays = requestedRays;
    configuration.sunWidthDivisions = m_scriptGridWidth;
    configuration.sunHeightDivisions = m_scriptGridHeight;
    if (m_interceptionConfiguration)
        configuration = m_interceptionConfiguration(requestedRays);
    if (configuration.rays != requestedRays)
        return fail("tn.FindInterception: host cannot change the requested ray count.");

    // Only ray visualization is platform-specific; invalid GUI-only requests
    // are rejected BEFORE tracing, and no scientific values are fabricated.
    const bool drawRays = show.toBool();
    if (drawRays && !m_interceptionDisplay)
        return fail("tn.FindInterception(show=true) is GUI-only presentation.");
    if (drawRays && requestedRays > std::numeric_limits<uint>::max())
        return fail("tn.FindInterception(show=true) ray count exceeds GUI range.");

    QString error;
    double power = 0.;
    if (!ScriptInterceptionService::findPower(scene(), surface.toString(),
                                              configuration, &power, &error))
        return fail(error.isEmpty() ? "tn.FindInterception failed." : error);

    if (drawRays && !m_interceptionDisplay(surface.toString(),
                                            static_cast<uint>(requestedRays),
                                            configuration.masterSeed, &error))
        return fail(error.isEmpty() ? "tn.FindInterception: GUI ray display failed." : error);
    return QJSValue(power);
}

QJSValue ScriptSceneServices::ChangeSunPosition(const QJSValue& azimuth,
                                                     const QJSValue& elevation)
{
    double azimuthDegrees = 0.;
    double elevationDegrees = 0.;
    if (!scriptFiniteNumber(azimuth, &azimuthDegrees)
        || !scriptFiniteNumber(elevation, &elevationDegrees))
        return fail("tn.ChangeSunPosition requires two finite azimuth/elevation values in degrees.");

    TSceneKit* active = scene();
    auto* position = active ? dynamic_cast<SunPosition*>(
        active->getPart("world.sun.position", false)) : nullptr;
    if (!position)
        return fail("tn.ChangeSunPosition requires a scene with sun position.");

    // Preserve MainWindow's historical field semantics (degrees, no UI work).
    position->azimuth.setValue(azimuthDegrees);
    position->elevation.setValue(elevationDegrees);
    return QJSValue();
}

QJSValue ScriptSceneServices::ChangeSunPosition(const QJSValue& year,
                                                     const QJSValue& month,
                                                     const QJSValue& day,
                                                     const QJSValue& hours,
                                                     const QJSValue& minutes,
                                                     const QJSValue& seconds,
                                                     const QJSValue& latitude,
                                                     const QJSValue& longitude)
{
    const std::array<QJSValue, 8> inputs = {
        year, month, day, hours, minutes, seconds, latitude, longitude
    };
    std::array<double, 8> values{};
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        if (!scriptFiniteNumber(inputs[i], &values[i]))
            return fail("tn.ChangeSunPosition requires eight finite calendar/location values.");
    }
    const auto integerField = [](double value) {
        return value == std::floor(value)
            && value >= static_cast<double>(std::numeric_limits<int>::min())
            && value <= static_cast<double>(std::numeric_limits<int>::max());
    };
    if (!integerField(values[0]) || !integerField(values[1]) || !integerField(values[2])
        || values[1] < 0 || values[1] > 12 || values[2] < 0 || values[2] > 31
        || values[3] < 0 || values[3] > 23 || values[4] < 0 || values[4] > 59
        || values[5] < 0 || values[5] > 59 || values[6] < -90 || values[6] > 90
        || values[7] < -180 || values[7] > 180)
        return fail("tn.ChangeSunPosition: calendar, UTC time or geographic position is out of range.");

    cTime time = {static_cast<int>(values[0]), static_cast<int>(values[1]),
                  static_cast<int>(values[2]), values[3], values[4], values[5]};
    // The historical GUI function takes latitude then longitude, but cLocation
    // stores longitude first. Preserve this ordering exactly.
    cLocation location = {values[7], values[6]};
    cSunCoordinates coordinates{};
    sunpos(time, location, &coordinates);
    const double computedElevation = 90. - coordinates.dZenithAngle;
    if (!std::isfinite(coordinates.dAzimuth) || !std::isfinite(computedElevation))
        return fail("tn.ChangeSunPosition: solar position calculation produced non-finite coordinates.");
    return ChangeSunPosition(QJSValue(coordinates.dAzimuth), QJSValue(computedElevation));
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
