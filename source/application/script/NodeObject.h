#pragma once

#include <QObject>
#include <QJSValue>
#include <Inventor/nodes/SoNode.h>

// Script-facing Coin node wrapper. The engine and scene/factory context are
// resolved per script host; no process-global GUI or QJSEngine pointers.
class NodeObject: public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE NodeObject(QObject* parent = nullptr);
    explicit NodeObject(SoNode* node);
    ~NodeObject() override;

    SoNode* getNode() const { return m_node; }

public slots:
    QJSValue getScene();
    QJSValue getRoot();

    QJSValue createNode(const QString& name = "");
    QJSValue createShape();
    QJSValue createTracker();

    QJSValue getPart(const QString& name = "");
    void setPart(const QString& name, QJSValue node);

    QJSValue insertSurface(const QString& name = "");
    QJSValue insertProfile(const QString& name = "");
    QJSValue insertMaterial(const QString& name = "");
    QJSValue insertArmature(const QString& name = "");

    void setName(const QString& name);
    void setParameter(const QString& name, const QString& value);

    QJSValue FindInterception(QJSValue surface, QJSValue rays);

private:
    QJSValue wrapNode(SoNode* node) const;
    QJSValue reportError(const QString& message) const;
    SoNode* m_node = nullptr;
};
