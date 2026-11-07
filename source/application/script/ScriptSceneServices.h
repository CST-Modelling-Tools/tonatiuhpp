#pragma once

#include <functional>
#include <utility>

#include "core/SimulationConfig.h"

#include <QObject>
#include <QJSValue>
#include <QString>

#include "script/ScriptPhotonExportConfig.h"
#include "script/ScriptRunRequest.h"

class QJSEngine;
class SoNode;
class TSceneKit;

// One widget-free, per-engine domain facade. GUI binds its existing document
// and undo stack; true-headless scripts bind an owned ScriptSceneDocument.
class ScriptSceneServices final : public QObject
{
    Q_OBJECT

public:
    enum class FactoryKind { Shape, Profile, Material, Tracker, Sun, Air };
    using SceneProvider = std::function<TSceneKit*()>;
    using NodeCreator = std::function<SoNode*(FactoryKind, const QString&)>;
    using InterceptionConfiguration = std::function<SimulationConfig(ulong)>;
    // GUI-only presentation callback; never supplies the scientific power.
    using InterceptionDisplay = std::function<bool(const QString&, uint,
                                                     std::uint64_t, QString*)>;
    using ClearDocument = std::function<bool(QString*)>;
    using InsertNode = std::function<bool(SoNode*, QString*)>;
    using SceneFileOperation = std::function<bool(const QString&, QString*)>;
    // GUI performs a QUndoStack edit; headless writes the same Coin field.
    // Target resolution and field selection are shared and widget-free.
    using FieldWriter = std::function<bool(SoNode*, const QString&, const QString&, QString*)>;
    // Keeps existing GUI ray-grid settings synchronized with script changes.
    using SunGridWriter = std::function<bool(int, int, QString*)>;
    // Shared scientific run-ray setting; host-specific GUI settings remain
    // synchronized through a callback without exposing MainWindow to JS.
    using RunRayCountWriter = std::function<bool(uint, QString*)>;
    // Phase 3B2A: validated nonvisual export-option requests; only GUI adapter
    // applies the existing exporter state; no tracing is started by setters.
    using PhotonExportWriter = std::function<bool(const ScriptPhotonExportChange&, QString*)>;
    // Phase 3B2B1: one shared JS Run API, with explicit host execution adapter.
    using RunExecutor = std::function<bool(const ScriptRunRequest&, QString*)>;

    explicit ScriptSceneServices(QObject* parent = nullptr) : QObject(parent) {}

    void setSceneProvider(SceneProvider callback) { m_scene = std::move(callback); }
    void setNodeCreator(NodeCreator callback) { m_create = std::move(callback); }
    void setInterceptionConfiguration(InterceptionConfiguration callback)
    {
        m_interceptionConfiguration = std::move(callback);
    }
    void setInterceptionDisplay(InterceptionDisplay callback)
    {
        m_interceptionDisplay = std::move(callback);
    }
    void setClearDocument(ClearDocument callback) { m_clear = std::move(callback); }
    void setInsertNode(InsertNode callback) { m_insert = std::move(callback); }
    void setSaveDocument(SceneFileOperation callback) { m_save = std::move(callback); }
    void setLoadDocument(SceneFileOperation callback) { m_load = std::move(callback); }
    void setFieldWriter(FieldWriter callback) { m_writeField = std::move(callback); }
    void setSunGridWriter(SunGridWriter callback) { m_writeGrid = std::move(callback); }
    void setRunRayCountWriter(RunRayCountWriter callback) { m_writeRunRays = std::move(callback); }
    // Nullopt means this script host has not explicitly selected a run count.
    // This setting does NOT override explicit FindInterception/traceScene rays.
    std::optional<uint> scriptRunRayCount() const { return m_scriptRunRays; }
    void setPhotonExportWriter(PhotonExportWriter callback) { m_writePhotonExport = std::move(callback); }
    void setRunExecutor(RunExecutor callback) { m_runExecutor = std::move(callback); }
    // GUI editors can begin with settings selected through the normal GUI.
    // Optional fields record only explicit script changes, not an initial GUI snapshot.
    void setPhotonExportSettingsPresent(bool present) { m_exportState.settingsPresent = present; }
    const ScriptPhotonExportState& scriptPhotonExportState() const { return m_exportState; }

    TSceneKit* scene() const { return m_scene ? m_scene() : nullptr; }
    SoNode* create(FactoryKind kind, const QString& name) const
    {
        return m_create ? m_create(kind, name) : nullptr;
    }

    // Script-facing, shared names and signatures. Clear/InsertScene retain the
    // legacy undefined return value on success; errors are JS exceptions.
    Q_INVOKABLE QJSValue getScene();
    Q_INVOKABLE QJSValue Clear();
    Q_INVOKABLE QJSValue InsertScene(const QJSValue& value);
    Q_INVOKABLE QJSValue saveScene(const QJSValue& path);
    Q_INVOKABLE QJSValue loadScene(const QJSValue& path);
    Q_INVOKABLE QJSValue SetValue(const QJSValue& nodeUrl,
                                 const QJSValue& fieldName, const QJSValue& value);
    Q_INVOKABLE QJSValue SetRaysGrid(const QJSValue& width, const QJSValue& height);
    Q_INVOKABLE QJSValue SetRaysNumber(const QJSValue& count = QJSValue());
    Q_INVOKABLE QJSValue SetExportAllPhotonMap();
    Q_INVOKABLE QJSValue SetExportCoordinates(const QJSValue& enabled = QJSValue(),
                                             const QJSValue& global = QJSValue());
    Q_INVOKABLE QJSValue SetExportIntersectionSurface(const QJSValue& enabled = QJSValue());
    Q_INVOKABLE QJSValue SetExportIntersectionSurfaceSide(const QJSValue& enabled = QJSValue());
    Q_INVOKABLE QJSValue SetExportPhotonMapType(const QJSValue& name = QJSValue());
    Q_INVOKABLE QJSValue SetExportPreviousNextPhotonID(const QJSValue& enabled = QJSValue());
    Q_INVOKABLE QJSValue SetExportTypeParameterValue(const QJSValue& name = QJSValue(),
                                                    const QJSValue& value = QJSValue());
    Q_INVOKABLE QJSValue AddExportSurfaceURL(const QJSValue& url = QJSValue());
    Q_INVOKABLE QJSValue SetPhotonBufferSize(const QJSValue& size = QJSValue());
    Q_INVOKABLE QJSValue SetPhotonBufferAppend(const QJSValue& enabled = QJSValue());
    Q_INVOKABLE QJSValue Run();
    Q_INVOKABLE QJSValue FindInterception(const QJSValue& surface, const QJSValue& rays);
    // Third argument retains GUI-only ray display. Power still comes from the
    // same nonvisual interception service; headless presentation fails explicitly.
    Q_INVOKABLE QJSValue FindInterception(const QJSValue& surface, const QJSValue& rays,
                                          const QJSValue& show);

    // Historical signatures now share one widget-free sun-position operation.
    // Return undefined on success as the former MainWindow void slots did.
    Q_INVOKABLE QJSValue ChangeSunPosition(const QJSValue& azimuth,
                                           const QJSValue& elevation);
    Q_INVOKABLE QJSValue ChangeSunPosition(const QJSValue& year,
                                           const QJSValue& month,
                                           const QJSValue& day,
                                           const QJSValue& hours,
                                           const QJSValue& minutes,
                                           const QJSValue& seconds,
                                           const QJSValue& latitude,
                                           const QJSValue& longitude);

    static ScriptSceneServices* forObject(QObject* object);

private:
    QJSValue fail(const QString& message) const;

    SceneProvider m_scene;
    NodeCreator m_create;
    InterceptionConfiguration m_interceptionConfiguration;
    InterceptionDisplay m_interceptionDisplay;
    ClearDocument m_clear;
    InsertNode m_insert;
    SceneFileOperation m_save;
    SceneFileOperation m_load;
    FieldWriter m_writeField;
    SunGridWriter m_writeGrid;
    RunRayCountWriter m_writeRunRays;
    std::optional<uint> m_scriptRunRays;
    PhotonExportWriter m_writePhotonExport;
    ScriptPhotonExportState m_exportState;
    RunExecutor m_runExecutor;
    QJSValue applyPhotonExportChange(const ScriptPhotonExportChange& change,
                                     const QString& method);
    int m_scriptGridWidth = SimulationConfig::kDefaultSunGridDivisions;
    int m_scriptGridHeight = SimulationConfig::kDefaultSunGridDivisions;
};

// Register the same domain facade and NodeObject class on each engine.
// `services` stays C++-owned and must outlive JS evaluation.
void registerScriptSceneServices(QJSEngine* engine, ScriptSceneServices* services);
