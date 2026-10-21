#pragma once

#include <functional>
#include <optional>
#include <utility>

#include <QObject>
#include <QJSValue>
#include <QString>

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
    using Interception = std::function<std::optional<double>(const QString&, uint)>;
    using ClearDocument = std::function<bool(QString*)>;
    using InsertNode = std::function<bool(SoNode*, QString*)>;
    using SceneFileOperation = std::function<bool(const QString&, QString*)>;

    explicit ScriptSceneServices(QObject* parent = nullptr) : QObject(parent) {}

    void setSceneProvider(SceneProvider callback) { m_scene = std::move(callback); }
    void setNodeCreator(NodeCreator callback) { m_create = std::move(callback); }
    void setInterception(Interception callback) { m_interception = std::move(callback); }
    void setClearDocument(ClearDocument callback) { m_clear = std::move(callback); }
    void setInsertNode(InsertNode callback) { m_insert = std::move(callback); }
    void setSaveDocument(SceneFileOperation callback) { m_save = std::move(callback); }
    void setLoadDocument(SceneFileOperation callback) { m_load = std::move(callback); }

    TSceneKit* scene() const { return m_scene ? m_scene() : nullptr; }
    SoNode* create(FactoryKind kind, const QString& name) const
    {
        return m_create ? m_create(kind, name) : nullptr;
    }
    std::optional<double> findInterception(const QString& surface, uint rays) const
    {
        return m_interception ? m_interception(surface, rays) : std::nullopt;
    }

    // Script-facing, shared names and signatures. Clear/InsertScene retain the
    // legacy undefined return value on success; errors are JS exceptions.
    Q_INVOKABLE QJSValue getScene();
    Q_INVOKABLE QJSValue Clear();
    Q_INVOKABLE QJSValue InsertScene(const QJSValue& value);
    Q_INVOKABLE QJSValue saveScene(const QJSValue& path);
    Q_INVOKABLE QJSValue loadScene(const QJSValue& path);
    Q_INVOKABLE QJSValue FindInterception(const QJSValue& surface, const QJSValue& rays);

    static ScriptSceneServices* forObject(QObject* object);

private:
    QJSValue fail(const QString& message) const;

    SceneProvider m_scene;
    NodeCreator m_create;
    Interception m_interception;
    ClearDocument m_clear;
    InsertNode m_insert;
    SceneFileOperation m_save;
    SceneFileOperation m_load;
};

// Register the same domain facade and NodeObject class on each engine.
// `services` stays C++-owned and must outlive JS evaluation.
void registerScriptSceneServices(QJSEngine* engine, ScriptSceneServices* services);
