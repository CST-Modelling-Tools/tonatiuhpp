#pragma once

#include <QStringList>
#include <QPersistentModelIndex>
#include <QSet>
#include <QTreeView>

#include "ParametersModel.h"

class SoNode;

class ParametersView: public QTreeView
{
    Q_OBJECT

public:
    ParametersView(QWidget* parent);

    ParametersModel* getModel();

    void setNode(SoNode* node);
    void reset();

private slots:
    void onPressed(const QModelIndex &index);
    void onExpanded(const QModelIndex& index);
    void onCollapsed(const QModelIndex& index);

private:
    void restoreExpansionAndColumnWidth(QPersistentModelIndex rootIndex, int generation);
    int restoreStoredExpansion();
    void applyColumnWidth();
    void expandDefaultParameterGroups();
    void expandParameterPath(const QStringList& path);
    bool hasCollapsedPreference(const QStringList& path) const;
    QStringList parameterPath(const QModelIndex& index) const;
    QString pathKey(const QStringList& path) const;
    QStringList pathFromKey(const QString& key) const;
    QModelIndex findChildByName(const QModelIndex& parent, const QString& name) const;
    QString parameterName(const QModelIndex& index) const;
    void updateRootIndex();

    QSet<QString> m_expandedPathKeys;
    QSet<QString> m_collapsedPathKeys;
    bool m_ignoreExpansionSignals = false;
    int m_restoreGeneration = 0;
};
